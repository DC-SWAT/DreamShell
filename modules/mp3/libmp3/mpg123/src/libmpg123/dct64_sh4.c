
/*
 * Discrete Cosine Tansform (DCT) for subband synthesis
 * optimized for machines with no auto-increment. 
 * The performance is highly compiler dependend. Maybe
 * the dct64.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

#include "mpg123lib_intern.h"

#define	FIXR(n)	(n)

#define COS0_0  FIXR(0.50060299823519630134)
#define COS0_1  FIXR(0.50547095989754365998)
#define COS0_2  FIXR(0.51544730992262454697)
#define COS0_3  FIXR(0.53104259108978417447)
#define COS0_4  FIXR(0.55310389603444452782)
#define COS0_5  FIXR(0.58293496820613387367)
#define COS0_6  FIXR(0.62250412303566481615)
#define COS0_7  FIXR(0.67480834145500574602)
#define COS0_8  FIXR(0.74453627100229844977)
#define COS0_9  FIXR(0.83934964541552703873)
#define COS0_10 FIXR(0.97256823786196069369)
#define COS0_11 FIXR(1.16943993343288495515)
#define COS0_12 FIXR(1.48416461631416627724)
#define COS0_13 FIXR(2.05778100995341155085)
#define COS0_14 FIXR(3.40760841846871878570)
#define COS0_15 FIXR(10.19000812354805681150)

#define COS1_0 FIXR(0.50241928618815570551)
#define COS1_1 FIXR(0.52249861493968888062)
#define COS1_2 FIXR(0.56694403481635770368)
#define COS1_3 FIXR(0.64682178335999012954)
#define COS1_4 FIXR(0.78815462345125022473)
#define COS1_5 FIXR(1.06067768599034747134)
#define COS1_6 FIXR(1.72244709823833392782)
#define COS1_7 FIXR(5.10114861868916385802)

#define COS2_0 FIXR(0.50979557910415916894)
#define COS2_1 FIXR(0.60134488693504528054)
#define COS2_2 FIXR(0.89997622313641570463)
#define COS2_3 FIXR(2.56291544774150617881)

#define COS3_0 FIXR(0.54119610014619698439)
#define COS3_1 FIXR(1.30656296487637652785)

#define COS4_0 FIXR(0.70710678118654752439)

#define	load_matrix(table) \
	__asm__( \
	"	fschg\n" \
	"	fmov	@%0+,xd0\n" \
	"	fmov	@%0+,xd2\n" \
	"	fmov	@%0+,xd4\n" \
	"	fmov	@%0+,xd6\n" \
	"	fmov	@%0+,xd8\n" \
	"	fmov	@%0+,xd10\n" \
	"	fmov	@%0+,xd12\n" \
	"	fmov	@%0+,xd14\n" \
	"	fschg\n" \
	:\
	: "r"(table)\
	: "0" \
	)

#define	ftrv() \
		asm("ftrv xmtrx,fv0" \
		: "=f"(fr0),"=f"(fr1),"=f"(fr2),"=f"(fr3) \
		:  "0"(fr0), "1"(fr1), "2"(fr2), "3"(fr3) );

	

void dct64(real *out0,real *out1,real *samples)
{
	real b1[32];

#define	DEFVAR	\
	register float fr0 __asm__("fr0"); \
	register float fr1 __asm__("fr1"); \
	register float fr2 __asm__("fr2"); \
	register float fr3 __asm__("fr3")

#define SETMAT(c0,c1,c2) { \
	const static float matrix[16] __attribute__ ((aligned(8))) = { \
	1.0f, c0*c2, c2, c0, \
	1.0f,-c0*c2, c2,-c0, \
	1.0f,-c1*c2,-c2, c1, \
	1.0f, c1*c2,-c2,-c1, \
	}; \
	load_matrix(matrix); \
	}

#define BF12(out,in,n,c0,c1,c2) { \
	DEFVAR; \
	SETMAT(c0,c1,c2); \
	fr0 = in[0x00+n]; \
	fr1 = in[0x1F-n]; \
	fr2 = in[0x0F-n]; \
	fr3 = in[0x10+n]; \
	ftrv(); \
	out[0x00+n] = fr0; \
	out[0x0F-n] = fr2; \
	out[0x10+n] = fr3; \
	out[0x1F-n] = fr1; \
}

	BF12(b1,samples,0,COS0_0,COS0_15,COS1_0);
	BF12(b1,samples,1,COS0_1,COS0_14,COS1_1);
	BF12(b1,samples,2,COS0_2,COS0_13,COS1_2);
	BF12(b1,samples,3,COS0_3,COS0_12,COS1_3);
	BF12(b1,samples,4,COS0_4,COS0_11,COS1_4);
	BF12(b1,samples,5,COS0_5,COS0_10,COS1_5);
	BF12(b1,samples,6,COS0_6,COS0_9 ,COS1_6);
	BF12(b1,samples,7,COS0_7,COS0_8 ,COS1_7);

#define BF34A(out,in,a,b,c,d,c0,c1,c2) \
{\
	DEFVAR; \
	fr0 = in[a]; \
	fr1 = in[b]; \
	fr2 = in[c]; \
	fr3 = in[d]; \
	ftrv(); \
	out[a] = fr0; \
	out[c] = fr2; \
	out[d] = fr3; \
	out[b] = fr1; \
}

/*
	normal: 12op + 4 move + load 3 = 19

	real t0,t1,t2,t3; \
	t0 = in[a] + in[b]; \
	t1 = (in[a] - in[b]) * c0; \
	t2 = in[c] + in[d]; \
	t3 = (in[c] - in[d]) * c1; \
	out[a] = (t0 + t2); \
	out[c] = (t0 - t2)*c2; \
	out[d] = (t1 + t3); \
	out[b] = (t1 - t3)*c2; \

	matrix: 4clk + load 16 float = 4 clock + load 8 double + 2 modechange = 14

	a = t0 + t2 = a + b + c + d
	b = (t1 - t3)*c2 = (a-b)*c0*c2 - (c-d)*c1*c2 = a*c0*c2 - b*c0*c2 -c*c1*c2 + d*c1*c2
	c = (t0 - t2)*c2 = ((a+b)-(c+d))*c2 = a*c2 + b*c2 - c*c2 - d*c2
	d = t1 + t3 = (a-b)*c0 + (c-d)*c1 = a*c0 - b*c0 + c*c1 - d*c1
	
	1        1   1   1
	c0c2 -c0c2 -c1c2 c1c2
	c2   c2    -c2   -c2
	c0  -c0     c1   -c1
*/


#define BF34B(out,in,a,b,c,d,c0,c1,c2) \
{\
	DEFVAR; \
	fr0 = in[b]; /* swap a,b and c,d */ \
	fr1 = in[a]; \
	fr2 = in[d]; \
	fr3 = in[c]; \
	ftrv(); \
	out[a] = fr0; \
	out[c] = fr2; \
	out[d] = fr3; \
	out[b] = fr1; \
}

	SETMAT(COS2_0,COS2_3,COS3_0);
	BF34A(b1,b1,0x00,0x07,0x03,0x04,COS2_0,COS2_3,COS3_0);
	BF34B(b1,b1,0x08,0x0F,0x0B,0x0C,COS2_0,COS2_3,COS3_0);
	BF34A(b1,b1,0x10,0x17,0x13,0x14,COS2_0,COS2_3,COS3_0);
	BF34B(b1,b1,0x18,0x1F,0x1B,0x1C,COS2_0,COS2_3,COS3_0);

	SETMAT(COS2_1,COS2_2,COS3_1);
	BF34A(b1,b1,0x01,0x06,0x02,0x05,COS2_1,COS2_2,COS3_1);
	BF34B(b1,b1,0x09,0x0E,0x0A,0x0D,COS2_1,COS2_2,COS3_1);
	BF34A(b1,b1,0x11,0x16,0x12,0x15,COS2_1,COS2_2,COS3_1);
	BF34B(b1,b1,0x19,0x1E,0x1A,0x1D,COS2_1,COS2_2,COS3_1);

#define	BF5A(out,in,a,b,c,d) \
{ \
	real t0,t1,t2,t3; \
	t0 = (in[a] + in[b]); \
	t1 = (in[a] - in[b])*COS4_0; \
	t2 = (in[c] + in[d]); \
	t3 = (in[d] - in[c])*COS4_0; \
	out[a] = t0; \
	out[b] = t1; \
	out[c] = t2 + t3; \
	out[d] = t3; \
}

/*
	7clk
*/

#define	BF5B(out,in,a,b,c,d) \
{ \
	real t0,t1,t2,t3; \
	t0 = (in[a] + in[b]); \
	t1 = (in[a] - in[b])*COS4_0; \
	t2 = (in[c] + in[d]); \
	t3 = (in[d] - in[c])*COS4_0; \
	out[a] = t0 + t2 + t3; \
	out[b] = t1 + t3; \
	out[c] = t2 + t3 + t1; \
	out[d] = t3; \
}

/*
	12clk
*/


	BF5A(b1,b1,0x00,0x01,0x02,0x03);
	BF5B(b1,b1,0x04,0x05,0x06,0x07);
	BF5A(b1,b1,0x08,0x09,0x0A,0x0B);
	BF5B(b1,b1,0x0C,0x0D,0x0E,0x0F);
	BF5A(b1,b1,0x10,0x11,0x12,0x13);
	BF5B(b1,b1,0x14,0x15,0x16,0x17);
	BF5A(b1,b1,0x18,0x19,0x1A,0x1B);
	BF5B(b1,b1,0x1C,0x1D,0x1E,0x1F);

 out0[0x10*16] = b1[0x00];
 out0[0x10*12] = b1[0x04];
 out0[0x10* 8] = b1[0x02];
 out0[0x10* 4] = b1[0x06];
 out0[0x10* 0] = b1[0x01];
 out1[0x10* 0] = b1[0x01];
 out1[0x10* 4] = b1[0x05];
 out1[0x10* 8] = b1[0x03];
 out1[0x10*12] = b1[0x07];

 out0[0x10*14] = b1[0x08] + b1[0x0C];
 out0[0x10*10] = b1[0x0C] + b1[0x0a];
 out0[0x10* 6] = b1[0x0A] + b1[0x0E];
 out0[0x10* 2] = b1[0x0E] + b1[0x09];
 out1[0x10* 2] = b1[0x09] + b1[0x0D];
 out1[0x10* 6] = b1[0x0D] + b1[0x0B];
 out1[0x10*10] = b1[0x0B] + b1[0x0F];
 out1[0x10*14] = b1[0x0F];

 b1[0x18] += b1[0x1C];
 out0[0x10*15] = b1[0x10] + b1[0x18];
 out0[0x10*13] = b1[0x18] + b1[0x14];
 b1[0x1C] += b1[0x1a];
 out0[0x10*11] = b1[0x14] + b1[0x1C];
 out0[0x10* 9] = b1[0x1C] + b1[0x12];
 b1[0x1A] += b1[0x1E];
 out0[0x10* 7] = b1[0x12] + b1[0x1A];
 out0[0x10* 5] = b1[0x1A] + b1[0x16];
 b1[0x1E] += b1[0x19];
 out0[0x10* 3] = b1[0x16] + b1[0x1E];
 out0[0x10* 1] = b1[0x1E] + b1[0x11];
 b1[0x19] += b1[0x1D];
 out1[0x10* 1] = b1[0x11] + b1[0x19];
 out1[0x10* 3] = b1[0x19] + b1[0x15];
 b1[0x1D] += b1[0x1B];
 out1[0x10* 5] = b1[0x15] + b1[0x1D];
 out1[0x10* 7] = b1[0x1D] + b1[0x13];
 b1[0x1B] += b1[0x1F];
 out1[0x10* 9] = b1[0x13] + b1[0x1B];
 out1[0x10*11] = b1[0x1B] + b1[0x17];
 out1[0x10*13] = b1[0x17] + b1[0x1F];
 out1[0x10*15] = b1[0x1F];
}

