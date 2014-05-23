/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Forward DCT  -
 *
 *  Copyright (C) 2006-2011 Xvid Solutions GmbH
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: fdct.c 1986 2011-05-18 09:07:40Z Isibaar $
 *
 ****************************************************************************/

/*
 *  Authors: Skal
 *
 *  "Fast and precise" LLM implementation of FDCT/IDCT, where
 *  rotations are decomposed using:
 *    tmp = (x+y).cos t
 *    x' = tmp + y.(sin t - cos t)
 *    y' = tmp - x.(sin t + cos t)
 *
 *  See details at the end of this file...
 *
 * Reference (e.g.):
 *  Loeffler C., Ligtenberg A., and Moschytz C.S.:
 *    Practical Fast 1D DCT Algorithm with Eleven Multiplications,
 *  Proc. ICASSP 1989, 988-991.
 *
 *  IEEE-1180-like error specs for FDCT:
 * Peak error:   1.0000
 * Peak MSE:     0.0340
 * Overall MSE:  0.0200
 * Peak ME:      0.0191
 * Overall ME:   -0.0033
 *
 ********************************************************/

#include "fdct.h"

/* function pointer */
fdctFuncPtr fdct;

/*
//////////////////////////////////////////////////////////
*/

#define BUTF(a, b, tmp) \
  (tmp) = (a)+(b);      \
  (b)   = (a)-(b);      \
  (a)   = (tmp)

#define LOAD_BUTF(m1, m2, a, b, tmp, S) \
  (m1) = (S)[(a)] + (S)[(b)];           \
  (m2) = (S)[(a)] - (S)[(b)]

#define ROTATE(m1,m2,c,k1,k2,tmp,Fix,Rnd) \
  (tmp) = ( (m1) + (m2) )*(c);            \
  (m1) *= k1;                             \
  (m2) *= k2;                             \
  (tmp) += (Rnd);                         \
  (m1) = ((m1)+(tmp))>>(Fix);             \
  (m2) = ((m2)+(tmp))>>(Fix);

#define ROTATE2(m1,m2,c,k1,k2,tmp) \
  (tmp) = ( (m1) + (m2) )*(c);     \
  (m1) *= k1;                      \
  (m2) *= k2;                      \
  (m1) = (m1)+(tmp);               \
  (m2) = (m2)+(tmp);

#define ROTATE0(m1,m2,c,k1,k2,tmp) \
  (m1) = ( (m2) )*(c);             \
  (m2) = (m2)*k2+(m1);

#define SHIFTL(x,n)   ((x)<<(n))
#define SHIFTR(x, n)  ((x)>>(n))
#define HALF(n)       (1<<((n)-1))

#define IPASS 3
#define FPASS 2
#define FIX  16

#if 1

#define ROT6_C     35468
#define ROT6_SmC   50159
#define ROT6_SpC  121095
#define ROT17_C    77062
#define ROT17_SmC  25571
#define ROT17_SpC 128553
#define ROT37_C    58981
#define ROT37_SmC  98391
#define ROT37_SpC  19571
#define ROT13_C   167963
#define ROT13_SmC 134553
#define ROT13_SpC 201373

#else

#include <math.h>
#define FX(x) ( (int)floor((x)*(1<<FIX) + .5 ) )

static const double c1 = cos(1.*M_PI/16);
static const double c2 = cos(2.*M_PI/16);
static const double c3 = cos(3.*M_PI/16);
static const double c4 = cos(4.*M_PI/16);
static const double c5 = cos(5.*M_PI/16);
static const double c6 = cos(6.*M_PI/16);
static const double c7 = cos(7.*M_PI/16);

static const int ROT6_C   = FX(c2-c6);  // 0.541
static const int ROT6_SmC = FX(2*c6);   // 0.765
static const int ROT6_SpC = FX(2*c2);   // 1.847

static const int ROT17_C   = FX(c1+c7);  // 1.175
static const int ROT17_SmC = FX(2*c7);   // 0.390
static const int ROT17_SpC = FX(2*c1);   // 1.961

static const int ROT37_C   = FX((c3-c7)/c4);  // 0.899
static const int ROT37_SmC = FX(2*(c5+c7));   // 1.501
static const int ROT37_SpC = FX(2*(c1-c3));   // 0.298

static const int ROT13_C   = FX((c1+c3)/c4);  // 2.562
static const int ROT13_SmC = FX(2*(c3+c7));   // 2.053
static const int ROT13_SpC = FX(2*(c1+c5));   // 3.072

#endif

/*
//////////////////////////////////////////////////////////
// Forward transform 
*/

void fdct_int32( short *const In )
{
  short *pIn;
  int i;

  pIn = In;
  for(i=8; i>0; --i)
  {
    int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, Spill;

      // even

    LOAD_BUTF(mm1,mm6, 1, 6, mm0, pIn);
    LOAD_BUTF(mm2,mm5, 2, 5, mm0, pIn);
    LOAD_BUTF(mm3,mm4, 3, 4, mm0, pIn);
    LOAD_BUTF(mm0,mm7, 0, 7, Spill, pIn);

    BUTF(mm1, mm2, Spill);
    BUTF(mm0, mm3, Spill);

    ROTATE(mm3, mm2, ROT6_C, ROT6_SmC, -ROT6_SpC, Spill, FIX-FPASS, HALF(FIX-FPASS));
    pIn[2] = mm3;
    pIn[6] = mm2;

    BUTF(mm0, mm1, Spill);
    pIn[0] = SHIFTL(mm0, FPASS);
    pIn[4] = SHIFTL(mm1, FPASS);


      // odd

    mm3 = mm5 + mm7;
    mm2 = mm4 + mm6;
    ROTATE(mm2, mm3,  ROT17_C, -ROT17_SpC, -ROT17_SmC, mm0, FIX-FPASS, HALF(FIX-FPASS));
    ROTATE(mm4, mm7, -ROT37_C,  ROT37_SpC,  ROT37_SmC, mm0, FIX-FPASS, HALF(FIX-FPASS));
    mm7 += mm3;
    mm4 += mm2;
    pIn[1] = mm7;
    pIn[7] = mm4;

    ROTATE(mm5, mm6, -ROT13_C,  ROT13_SmC,  ROT13_SpC, mm0, FIX-FPASS, HALF(FIX-FPASS));
    mm5 += mm3;
    mm6 += mm2;
    pIn[3] = mm6;
    pIn[5] = mm5;

    pIn  += 8;
  }

  pIn = In;
  for(i=8; i>0; --i)
  {
    int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, Spill;

      // even

    LOAD_BUTF(mm1,mm6, 1*8, 6*8, mm0, pIn);
    LOAD_BUTF(mm2,mm5, 2*8, 5*8, mm0, pIn);
    BUTF(mm1, mm2, mm0);

    LOAD_BUTF(mm3,mm4, 3*8, 4*8, mm0, pIn);
    LOAD_BUTF(mm0,mm7, 0*8, 7*8, Spill, pIn);
    BUTF(mm0, mm3, Spill);

    ROTATE(mm3, mm2, ROT6_C, ROT6_SmC, -ROT6_SpC, Spill, 0,  HALF(FIX+FPASS+3));
    pIn[2*8] = (int16_t)SHIFTR(mm3,FIX+FPASS+3);
    pIn[6*8] = (int16_t)SHIFTR(mm2,FIX+FPASS+3);

    mm0 += HALF(FPASS+3) - 1;
    BUTF(mm0, mm1, Spill);
    pIn[0*8] = (int16_t)SHIFTR(mm0, FPASS+3);
    pIn[4*8] = (int16_t)SHIFTR(mm1, FPASS+3);

      // odd

    mm3 = mm5 + mm7;
    mm2 = mm4 + mm6;

    ROTATE(mm2, mm3,  ROT17_C, -ROT17_SpC, -ROT17_SmC, mm0, 0, HALF(FIX+FPASS+3));
    ROTATE2(mm4, mm7, -ROT37_C,  ROT37_SpC,  ROT37_SmC, mm0);
    mm7 += mm3;
    mm4 += mm2;
    pIn[7*8] = (int16_t)SHIFTR(mm4,FIX+FPASS+3);
    pIn[1*8] = (int16_t)SHIFTR(mm7,FIX+FPASS+3);

    ROTATE2(mm5, mm6, -ROT13_C,  ROT13_SmC,  ROT13_SpC, mm0);
    mm5 += mm3;
    mm6 += mm2;
    pIn[5*8] = (int16_t)SHIFTR(mm5,FIX+FPASS+3);
    pIn[3*8] = (int16_t)SHIFTR(mm6,FIX+FPASS+3);

    pIn++;
  }
}
#undef FIX
#undef FPASS
#undef IPASS

#undef BUTF
#undef LOAD_BUTF
#undef ROTATE
#undef ROTATE2
#undef SHIFTL
#undef SHIFTR

//////////////////////////////////////////////////////////
//   - Data flow schematics for FDCT -
// Output is scaled by 2.sqrt(2)
// Initial butterflies (in0/in7, etc.) are not fully depicted.
// Note: Rot6 coeffs are multiplied by sqrt(2).
//////////////////////////////////////////////////////////
/*
 <---------Stage1 =even part=----------->

 in3 mm3  +_____.___-___________.____* out6
  x              \ /            |
 in4 mm4          \             |
                 / \            |
 in0 mm0  +_____o___+__.___-___ | ___* out4
  x                     \ /     |
 in7 mm7                 \    (Rot6)
                        / \     |
 in1 mm1  +_____o___+__o___+___ | ___* out0
  x              \ /            |
 in6 mm6          /             |
                 / \            |
 in2 mm2  +_____.___-___________o____* out2
  x
 in5 mm5

 <---------Stage2 =odd part=---------------->

 mm7*___._________.___-___[xSqrt2]___* out3
        |          \ /
      (Rot3)        \
        |          / \
 mm5*__ | ___o____o___+___.___-______* out7
        |    |             \ /
        |  (Rot1)           \
        |    |             / \
 mm6*__ |____.____o___+___o___+______* out1
        |          \ /
        |           /
        |          / \
 mm4*___o_________.___-___[xSqrt2]___* out5



    Alternative schematics for stage 2:
    -----------------------------------

 mm7 *___[xSqrt2]____o___+____o_______* out1
                      \ /     |
                       /    (Rot1)
                      / \     |
 mm6 *____o___+______.___-___ | __.___* out5
           \ /                |   |
            /                 |   |
           / \                |   |
 mm5 *____.___-______.___-____.__ | __* out7
                      \ /         |
                       \        (Rot3)
                      / \         |
 mm4 *___[xSqrt2]____o___+________o___* out3

*/
