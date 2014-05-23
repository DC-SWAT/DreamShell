/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Sum Of Absolute Difference related code -
 *
 *  Copyright(C) 2001-2010 Peter Ross <pross@xvid.org>
 *               2010      Michael Militzer <michael@xvid.org>
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
 * $Id: sad.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

#include "../portab.h"
#include "../global.h"
#include "sad.h"

#include <stdlib.h>

sad16FuncPtr sad16;
sad8FuncPtr sad8;
sad16biFuncPtr sad16bi;
sad8biFuncPtr sad8bi;		/* not really sad16, but no difference in prototype */
dev16FuncPtr dev16;
sad16vFuncPtr sad16v;
sse8Func_16bitPtr sse8_16bit;
sse8Func_8bitPtr sse8_8bit;

sseh8Func_16bitPtr sseh8_16bit;
coeff8_energyFunc_Ptr coeff8_energy;
blocksum8Func_Ptr blocksum8;

sadInitFuncPtr sadInit;


uint32_t
sad16_c(const uint8_t * const cur,
		const uint8_t * const ref,
		const uint32_t stride,
		const uint32_t best_sad)
{

	uint32_t sad = 0;
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
			sad += abs(ptr_cur[0] - ptr_ref[0]);
			sad += abs(ptr_cur[1] - ptr_ref[1]);
			sad += abs(ptr_cur[2] - ptr_ref[2]);
			sad += abs(ptr_cur[3] - ptr_ref[3]);
			sad += abs(ptr_cur[4] - ptr_ref[4]);
			sad += abs(ptr_cur[5] - ptr_ref[5]);
			sad += abs(ptr_cur[6] - ptr_ref[6]);
			sad += abs(ptr_cur[7] - ptr_ref[7]);
			sad += abs(ptr_cur[8] - ptr_ref[8]);
			sad += abs(ptr_cur[9] - ptr_ref[9]);
			sad += abs(ptr_cur[10] - ptr_ref[10]);
			sad += abs(ptr_cur[11] - ptr_ref[11]);
			sad += abs(ptr_cur[12] - ptr_ref[12]);
			sad += abs(ptr_cur[13] - ptr_ref[13]);
			sad += abs(ptr_cur[14] - ptr_ref[14]);
			sad += abs(ptr_cur[15] - ptr_ref[15]);

			if (sad >= best_sad)
				return sad;

			ptr_cur += stride;
			ptr_ref += stride;

	}

	return sad;

}

uint32_t
sad16bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;
			sad += abs(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}

uint32_t
sad8bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 8; j++) {

		for (i = 0; i < 8; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;
			sad += abs(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}



uint32_t
sad8_c(const uint8_t * const cur,
	   const uint8_t * const ref,
	   const uint32_t stride)
{
	uint32_t sad = 0;
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 8; j++) {

		sad += abs(ptr_cur[0] - ptr_ref[0]);
		sad += abs(ptr_cur[1] - ptr_ref[1]);
		sad += abs(ptr_cur[2] - ptr_ref[2]);
		sad += abs(ptr_cur[3] - ptr_ref[3]);
		sad += abs(ptr_cur[4] - ptr_ref[4]);
		sad += abs(ptr_cur[5] - ptr_ref[5]);
		sad += abs(ptr_cur[6] - ptr_ref[6]);
		sad += abs(ptr_cur[7] - ptr_ref[7]);

		ptr_cur += stride;
		ptr_ref += stride;

	}

	return sad;
}


/* average deviation from mean */

uint32_t
dev16_c(const uint8_t * const cur,
		const uint32_t stride)
{

	uint32_t mean = 0;
	uint32_t dev = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			mean += *(ptr_cur + i);

		ptr_cur += stride;

	}

	mean /= (16 * 16);
	ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			dev += abs(*(ptr_cur + i) - (int32_t) mean);

		ptr_cur += stride;

	}

	return dev;
}

uint32_t sad16v_c(const uint8_t * const cur,
			   const uint8_t * const ref,
			   const uint32_t stride,
			   int32_t *sad)
{
	sad[0] = sad8(cur, ref, stride);
	sad[1] = sad8(cur + 8, ref + 8, stride);
	sad[2] = sad8(cur + 8*stride, ref + 8*stride, stride);
	sad[3] = sad8(cur + 8*stride + 8, ref + 8*stride + 8, stride);

	return sad[0]+sad[1]+sad[2]+sad[3];
}

uint32_t sad32v_c(const uint8_t * const cur,
			   const uint8_t * const ref,
			   const uint32_t stride,
			   int32_t *sad)
{
	sad[0] = sad16(cur, ref, stride, 256*4096);
	sad[1] = sad16(cur + 16, ref + 16, stride, 256*4096);
	sad[2] = sad16(cur + 16*stride, ref + 16*stride, stride, 256*4096);
	sad[3] = sad16(cur + 16*stride + 16, ref + 16*stride + 16, stride, 256*4096);

	return sad[0]+sad[1]+sad[2]+sad[3];
}



#define MRSAD16_CORRFACTOR 8
uint32_t
mrsad16_c(const uint8_t * const cur,
		  const uint8_t * const ref,
		  const uint32_t stride,
		  const uint32_t best_sad)
{

	uint32_t sad = 0;
	int32_t mean = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			mean += ((int) *(ptr_cur + i) - (int) *(ptr_ref + i));
		}
		ptr_cur += stride;
		ptr_ref += stride;

	}
	mean /= 256;

	for (j = 0; j < 16; j++) {

		ptr_cur -= stride;
		ptr_ref -= stride;

		for (i = 0; i < 16; i++) {

			sad += abs(*(ptr_cur + i) - *(ptr_ref + i) - mean);
			if (sad >= best_sad) {
				return MRSAD16_CORRFACTOR * sad;
			}
		}
	}

	return MRSAD16_CORRFACTOR * sad;
}

uint32_t
sse8_16bit_c(const int16_t * b1,
			 const int16_t * b2,
			 const uint32_t stride)
{
	int i;
	int sse = 0;

	for (i=0; i<8; i++) {
		sse += (b1[0] - b2[0])*(b1[0] - b2[0]);
		sse += (b1[1] - b2[1])*(b1[1] - b2[1]);
		sse += (b1[2] - b2[2])*(b1[2] - b2[2]);
		sse += (b1[3] - b2[3])*(b1[3] - b2[3]);
		sse += (b1[4] - b2[4])*(b1[4] - b2[4]);
		sse += (b1[5] - b2[5])*(b1[5] - b2[5]);
		sse += (b1[6] - b2[6])*(b1[6] - b2[6]);
		sse += (b1[7] - b2[7])*(b1[7] - b2[7]);

		b1 = (const int16_t*)((int8_t*)b1+stride);
		b2 = (const int16_t*)((int8_t*)b2+stride);
	}

	return(sse);
}

uint32_t
sse8_8bit_c(const uint8_t * b1,
			const uint8_t * b2,
			const uint32_t stride)
{
	int i;
	int sse = 0;

	for (i=0; i<8; i++) {
		sse += (b1[0] - b2[0])*(b1[0] - b2[0]);
		sse += (b1[1] - b2[1])*(b1[1] - b2[1]);
		sse += (b1[2] - b2[2])*(b1[2] - b2[2]);
		sse += (b1[3] - b2[3])*(b1[3] - b2[3]);
		sse += (b1[4] - b2[4])*(b1[4] - b2[4]);
		sse += (b1[5] - b2[5])*(b1[5] - b2[5]);
		sse += (b1[6] - b2[6])*(b1[6] - b2[6]);
		sse += (b1[7] - b2[7])*(b1[7] - b2[7]);

		b1 = b1+stride;
		b2 = b2+stride;
	}

	return(sse);
}


/* PSNR-HVS-M helper functions */

static const int16_t iMask_Coeff[64] = { 
        0, 29788, 32767, 20479, 13653, 8192, 6425, 5372,
    27306, 27306, 23405, 17246, 12603, 5650, 5461, 5958,
    23405, 25205, 20479, 13653,  8192, 5749, 4749, 5851,
    23405, 19275, 14894, 11299,  6425, 3766, 4096, 5285,
    18204, 14894,  8856,  5851,  4819, 3006, 3181, 4255,
    13653,  9362,  5958,  5120,  4045, 3151, 2900, 3562,
     6687,  5120,  4201,  3766,  3181, 2708, 2730, 3244,
     4551,  3562,  3449,  3344,  2926, 3277, 3181, 3310
};

/* Calculate CSF weighted energy of DCT coefficients */

uint32_t 
coeff8_energy_c(const int16_t * dct)
{
	int x, y;
	uint32_t sum_a = 0;

	for (y = 0; y < 8; y += 2) {
		for (x = 0; x < 8; x += 2) {
			int16_t a0 = ((dct[y*8+x]<<4) * iMask_Coeff[y*8+x]) >> 16;
			int16_t a1 = ((dct[y*8+x+1]<<4) * iMask_Coeff[y*8+x+1]) >> 16;
			int16_t a2 = ((dct[(y+1)*8+x]<<4) * iMask_Coeff[(y+1)*8+x]) >> 16;
			int16_t a3 = ((dct[(y+1)*8+x+1]<<4) * iMask_Coeff[(y+1)*8+x+1]) >> 16;

			sum_a += ((a0*a0 + a1*a1 + a2*a2 + a3*a3) >> 3);
		}
	}

	return sum_a;
}

/* Calculate MSE of DCT coeffs reduced by masking effect */

uint32_t 
sseh8_16bit_c(const int16_t * cur, const int16_t * ref, uint16_t mask)
{
	int j, i;
	uint32_t mse_h = 0;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint32_t t = (mask * Inv_iMask_Coeff[j*8+i] + 32) >> 7;
			uint16_t u = abs(cur[j*8+i] - ref[j*8+i]) << 4;
			uint16_t thresh = (t < 65536) ? t : 65535;

			if (u < thresh)	
				u = 0; /* The error is not perceivable */
			else 
				u -= thresh; 

			u = ((u + iCSF_Round[j*8 + i]) * iCSF_Coeff[j*8 + i]) >> 16;

			mse_h += (uint32_t) (u * u);
		}
	}

	return mse_h;
}

/* Sums all pixels of 8x8 block */

uint32_t
blocksum8_c(const uint8_t * cur, int stride, uint16_t sums[4], uint32_t squares[4])
{
	int i, j;
	uint32_t sum = 0;

	sums[0] = sums[1] = sums[2] = sums[3] = 0;
	squares[0] = squares[1] = squares[2] = squares[3] = 0;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint8_t p = cur[j*stride + i];

			sums[(j>>2)*2 + (i>>2)] += p;
			squares[(j>>2)*2 + (i>>2)] += p*p;

			sum += p;
		}
	}

	return sum;
}
