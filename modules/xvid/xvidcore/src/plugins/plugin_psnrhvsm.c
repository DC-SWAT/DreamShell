/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - PSNR-HVS-M plugin: computes the PSNR-HVS-M metric -
 *
 *  Copyright(C) 2010 Michael Militzer <michael@xvid.org>
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
 * $Id: plugin_psnrhvsm.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

/*****************************************************************************
 *
 * The PSNR-HVS-M metric is described in the following paper:
 *
 * "On between-coefficient contrast masking of DCT basis functions", by
 * N. Ponomarenko, F. Silvestri, K. Egiazarian, M. Carli, J. Astola, V. Lukin,
 * in Proceedings of the Third International Workshop on Video Processing and 
 * Quality Metrics for Consumer Electronics VPQM-07, January, 2007, 4 p.
 *
 * http://www.ponomarenko.info/psnrhvsm.htm
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../portab.h"
#include "../xvid.h"
#include "../dct/fdct.h"
#include "../image/image.h"
#include "../motion/sad.h"
#include "../utils/mem_transfer.h"
#include "../utils/emms.h"

typedef struct { 

	uint64_t mse_sum_y; /* for avrg psnr-hvs-m */
	uint64_t mse_sum_u;
	uint64_t mse_sum_v;

	long frame_cnt;

} psnrhvsm_data_t; /* internal plugin data */


#if 0 /* Floating-point implementation: Slow but accurate */

static const float CSF_Coeff[64] = { 
	1.608443f, 2.339554f, 2.573509f, 1.608443f, 1.072295f, 0.643377f, 0.504610f, 0.421887f,
	2.144591f, 2.144591f, 1.838221f, 1.354478f, 0.989811f, 0.443708f, 0.428918f, 0.467911f,
	1.838221f, 1.979622f, 1.608443f, 1.072295f, 0.643377f, 0.451493f, 0.372972f, 0.459555f,
	1.838221f, 1.513829f, 1.169777f, 0.887417f, 0.504610f, 0.295806f, 0.321689f, 0.415082f,
	1.429727f, 1.169777f, 0.695543f, 0.459555f, 0.378457f, 0.236102f, 0.249855f, 0.334222f,
	1.072295f, 0.735288f, 0.467911f, 0.402111f, 0.317717f, 0.247453f, 0.227744f, 0.279729f,
	0.525206f, 0.402111f, 0.329937f, 0.295806f, 0.249855f, 0.212687f, 0.214459f, 0.254803f,
	0.357432f, 0.279729f, 0.270896f, 0.262603f, 0.229778f, 0.257351f, 0.249855f, 0.259950f
};

static const float Mask_Coeff[64] = { 
	0.000000f, 0.826446f, 1.000000f, 0.390625f, 0.173611f, 0.062500f, 0.038447f, 0.026874f,
	0.694444f, 0.694444f, 0.510204f, 0.277008f, 0.147929f, 0.029727f, 0.027778f, 0.033058f,
	0.510204f, 0.591716f, 0.390625f, 0.173611f, 0.062500f, 0.030779f, 0.021004f, 0.031888f,
	0.510204f, 0.346021f, 0.206612f, 0.118906f, 0.038447f, 0.013212f, 0.015625f, 0.026015f,
	0.308642f, 0.206612f, 0.073046f, 0.031888f, 0.021626f, 0.008417f, 0.009426f, 0.016866f,
	0.173611f, 0.081633f, 0.033058f, 0.024414f, 0.015242f, 0.009246f, 0.007831f, 0.011815f,
	0.041649f, 0.024414f, 0.016437f, 0.013212f, 0.009426f, 0.006830f, 0.006944f, 0.009803f,
	0.019290f, 0.011815f, 0.011080f, 0.010412f, 0.007972f, 0.010000f, 0.009426f, 0.010203f
};

static uint32_t calc_SSE_H(int16_t *DCT_A, int16_t *DCT_B, uint8_t *IMG_A, uint8_t *IMG_B, int stride)
{
	int x, y, i, j;
	uint32_t Global_A, Global_B, Sum_A = 0, Sum_B = 0;
	uint32_t Local[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t Local_Square[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	float MASK_A = 0.f, MASK_B = 0.f;
	float Mult1 = 1.f, Mult2 = 1.f;
	uint32_t MSE_H = 0;

	/* Step 1: Calculate CSF weighted energy of DCT coefficients */
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			MASK_A += (float)(DCT_A[y*8 + x]*DCT_A[y*8 + x])*Mask_Coeff[y*8 + x];
			MASK_B += (float)(DCT_B[y*8 + x]*DCT_B[y*8 + x])*Mask_Coeff[y*8 + x];
		}
	}

	/* Step 2: Determine local variances compared to entire block variance */
	for (y = 0; y < 2; y++) {
		for (x = 0; x < 2; x++) {
			for (j = 0; j < 4; j++) {
				for (i = 0; i < 4; i++) {
					uint8_t A = IMG_A[(y*4+j)*stride + 4*x + i];
					uint8_t B = IMG_B[(y*4+j)*stride + 4*x + i];

					Local[y*2 + x] += A;
					Local[y*2 + x + 4] += B;
					Local_Square[y*2 + x] += A*A;
					Local_Square[y*2 + x + 4] += B*B;
				}
			}
		}
	}

	Global_A = Local[0] + Local[1] + Local[2] + Local[3];
	Global_B = Local[4] + Local[5] + Local[6] + Local[7];

	for (i = 0; i < 8; i++)
		Local[i] = (Local_Square[i]<<4) - (Local[i]*Local[i]); /* 16*Var(Di) */

	Local_Square[0] += (Local_Square[1] + Local_Square[2] + Local_Square[3]);
	Local_Square[4] += (Local_Square[5] + Local_Square[6] + Local_Square[7]);

	Global_A = (Local_Square[0]<<6) - Global_A*Global_A; /* 64*Var(D) */
	Global_B = (Local_Square[4]<<6) - Global_B*Global_B; /* 64*Var(D) */

	/* Step 3: Calculate contrast masking threshold */
	if (Global_A) 
		Mult1 = (float)(Local[0]+Local[1]+Local[2]+Local[3])/((float)(Global_A)/4.f);

	if (Global_B)
		Mult2 = (float)(Local[4]+Local[5]+Local[6]+Local[7])/((float)(Global_B)/4.f);

	MASK_A = (float)sqrt(MASK_A * Mult1) / 32.f;
	MASK_B = (float)sqrt(MASK_B * Mult2) / 32.f;

	if (MASK_B > MASK_A) MASK_A = MASK_B; /* MAX(MASK_A, MASK_B) */

	/* Step 4: Calculate MSE of DCT coeffs reduced by masking effect */
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			float u = (float)abs(DCT_A[j*8 + i] - DCT_B[j*8 + i]);

			if ((i|j)>0) {
				if (u < (MASK_A / Mask_Coeff[j*8 + i])) 
					u = 0; /* The error is not perceivable */
				else
					u -= (MASK_A / Mask_Coeff[j*8 + i]);
			}
			
			MSE_H += (uint32_t) ((16.f*(u * CSF_Coeff[j*8 + i])*(u * CSF_Coeff[j*8 + i])) + 0.5f);
		}
	}
	return MSE_H; /* Fixed-point value right-shifted by four */
}

#else 

static uint32_t calc_SSE_H(int16_t *DCT_A, int16_t *DCT_B, uint8_t *IMG_A, uint8_t *IMG_B, int stride)
{
	DECLARE_ALIGNED_MATRIX(sums, 1, 8, uint16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(squares, 1, 8, uint32_t, CACHE_LINE);
	uint32_t i, Global_A, Global_B, Sum_A = 0, Sum_B = 0;
	uint32_t local[8], MASK_A, MASK_B, Mult1 = 64, Mult2 = 64;

	/* Step 1: Calculate CSF weighted energy of DCT coefficients */
	
	Sum_A = coeff8_energy(DCT_A);
	Sum_B = coeff8_energy(DCT_B);

	/* Step 2: Determine local variances compared to entire block variance */
	
	Global_A = blocksum8(IMG_A, stride, sums, squares);
	Global_B = blocksum8(IMG_B, stride, &sums[4], &squares[4]);

	for (i = 0; i < 8; i++)
		local[i] = (squares[i]<<4) - (sums[i]*sums[i]); /* 16*Var(Di) */

	squares[0] += (squares[1] + squares[2] + squares[3]);
	squares[4] += (squares[5] + squares[6] + squares[7]);

	Global_A = (squares[0]<<6) - Global_A*Global_A; /* 64*Var(D) */
	Global_B = (squares[4]<<6) - Global_B*Global_B; /* 64*Var(D) */

	/* Step 3: Calculate contrast masking threshold */
	
	if (Global_A) 
		Mult1 = ((local[0]+local[1]+local[2]+local[3])<<8) / Global_A;

	if (Global_B)
		Mult2 = ((local[4]+local[5]+local[6]+local[7])<<8) / Global_B;

	MASK_A = isqrt(2*Sum_A*Mult1) + 16;
	MASK_B = isqrt(2*Sum_B*Mult2) + 16;

	if (MASK_B > MASK_A)  /* MAX(MASK_A, MASK_B) */
		MASK_A = ((MASK_B + 32) >> 6);
	else
		MASK_A = ((MASK_A + 32) >> 6);

	/* Step 4: Calculate MSE of DCT coeffs reduced by masking effect */
	
	return sseh8_16bit(DCT_A, DCT_B, (uint16_t) MASK_A);
}

#endif

static void psnrhvsm_after(xvid_plg_data_t *data, psnrhvsm_data_t *psnrhvsm)
{
	DECLARE_ALIGNED_MATRIX(DCT, 2, 64, int16_t, CACHE_LINE);
	int32_t x, y, u, v; 
	int16_t *DCT_A = &DCT[0], *DCT_B = &DCT[64];
	uint64_t sse_y = 0, sse_u = 0, sse_v = 0;

	for (y = 0; y < data->height>>3; y++) {
		uint8_t *IMG_A = (uint8_t *) data->original.plane[0];
		uint8_t *IMG_B = (uint8_t *) data->current.plane[0];
		uint32_t stride = data->original.stride[0];

		for (x = 0; x < data->width>>3; x++) { /* non multiple of 8 handling ?? */
			int offset = (y<<3)*stride + (x<<3);

			emms();

			/* Transfer data */
			transfer_8to16copy(DCT_A, IMG_A + offset, stride);
			transfer_8to16copy(DCT_B, IMG_B + offset, stride);

			/* Perform DCT */
			fdct(DCT_A);
			fdct(DCT_B);

			emms();

			/* Calculate SSE_H reduced by contrast masking effect */
			sse_y += calc_SSE_H(DCT_A, DCT_B, IMG_A + offset, IMG_B + offset, stride);
		}
	}

	for (y = 0; y < data->height>>4; y++) {
		uint8_t *U_A = (uint8_t *) data->original.plane[1];
		uint8_t *V_A = (uint8_t *) data->original.plane[2];
		uint8_t *U_B = (uint8_t *) data->current.plane[1];
		uint8_t *V_B = (uint8_t *) data->current.plane[2];
		uint32_t stride_uv = data->current.stride[1];

		for (x = 0; x < data->width>>4; x++) { /* non multiple of 8 handling ?? */
			int offset = (y<<3)*stride_uv + (x<<3);

			emms();

			/* Transfer data */
			transfer_8to16copy(DCT_A, U_A + offset, stride_uv);
			transfer_8to16copy(DCT_B, U_B + offset, stride_uv);

			/* Perform DCT */
			fdct(DCT_A);
			fdct(DCT_B);

			emms();

			/* Calculate SSE_H reduced by contrast masking effect */
			sse_u += calc_SSE_H(DCT_A, DCT_B, U_A + offset, U_B + offset, stride_uv);

			emms();

			/* Transfer data */
			transfer_8to16copy(DCT_A, V_A + offset, stride_uv);
			transfer_8to16copy(DCT_B, V_B + offset, stride_uv);

			/* Perform DCT */
			fdct(DCT_A);
			fdct(DCT_B);

			emms();

			/* Calculate SSE_H reduced by contrast masking effect */
			sse_v += calc_SSE_H(DCT_A, DCT_B, V_A + offset, V_B + offset, stride_uv);
		}
	}

	y = (int32_t) ( 4*16*sse_y / (data->width * data->height));
	u = (int32_t) (16*16*sse_u / (data->width * data->height));
	v = (int32_t) (16*16*sse_v / (data->width * data->height));

	psnrhvsm->mse_sum_y += y;
	psnrhvsm->mse_sum_u += u;
	psnrhvsm->mse_sum_v += v;
	psnrhvsm->frame_cnt++;

	printf("       psnrhvsm y: %2.2f, psnrhvsm u: %2.2f, psnrhvsm v: %2.2f\n", sse_to_PSNR(y, 1024), sse_to_PSNR(u, 1024), sse_to_PSNR(v, 1024));
}

static int psnrhvsm_create(xvid_plg_create_t *create, void **handle)
{
	psnrhvsm_data_t *psnrhvsm;
	psnrhvsm = (psnrhvsm_data_t *) malloc(sizeof(psnrhvsm_data_t));

	psnrhvsm->mse_sum_y = 0;
	psnrhvsm->mse_sum_u = 0;
	psnrhvsm->mse_sum_v = 0;

	psnrhvsm->frame_cnt = 0;

	*(handle) = (void*) psnrhvsm;
	return 0;
}	

int xvid_plugin_psnrhvsm(void *handle, int opt, void *param1, void *param2)
{
	switch(opt) {
		case(XVID_PLG_INFO):
 			((xvid_plg_info_t *)param1)->flags = XVID_REQORIGINAL;
			break;
		case(XVID_PLG_CREATE):
			psnrhvsm_create((xvid_plg_create_t *)param1,(void **)param2);
			break;
		case(XVID_PLG_BEFORE):
		case(XVID_PLG_FRAME):
			break;
		case(XVID_PLG_AFTER):
			psnrhvsm_after((xvid_plg_data_t *)param1, (psnrhvsm_data_t *)handle);
			break;
		case(XVID_PLG_DESTROY):
			{
				uint32_t y, u, v;
				psnrhvsm_data_t *psnrhvsm = (psnrhvsm_data_t *)handle;
				
				if (psnrhvsm) {
					y = (uint32_t) (psnrhvsm->mse_sum_y / psnrhvsm->frame_cnt);
					u = (uint32_t) (psnrhvsm->mse_sum_u / psnrhvsm->frame_cnt);
					v = (uint32_t) (psnrhvsm->mse_sum_v / psnrhvsm->frame_cnt);

					emms();
					printf("Average psnrhvsm y: %2.2f, psnrhvsm u: %2.2f, psnrhvsm v: %2.2f\n", 
						sse_to_PSNR(y, 1024), sse_to_PSNR(u, 1024), sse_to_PSNR(v, 1024));
					free(psnrhvsm);
				}
			}
			break;
		default:
			break;
	}
	return 0;
};
