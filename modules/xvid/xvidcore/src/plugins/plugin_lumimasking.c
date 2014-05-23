/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Xvid plugin: performs a lumimasking algorithm on encoded frame  -
 *
 *  Copyright(C) 2002-2003 Peter Ross <pross@xvid.org>
 *               2002      Christoph Lampert <gruel@web.de>
 *               2008      Jason Garrett-Glaser <darkshikari@gmail.com>
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
 * $Id: plugin_lumimasking.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../xvid.h"
#include "../global.h"
#include "../portab.h"
#include "../utils/emms.h"

/*****************************************************************************
 * Private data type
 ****************************************************************************/

typedef struct
{
	float *quant;
	float *val;
	int method;      
} lumi_data_t;

/*****************************************************************************
 * Sub plugin functions
 ****************************************************************************/

static int lumi_plg_info(xvid_plg_info_t *info);
static int lumi_plg_create(xvid_plg_create_t *create, lumi_data_t **handle);
static int lumi_plg_destroy(lumi_data_t *handle, xvid_plg_destroy_t * destroy);
static int lumi_plg_frame(lumi_data_t *handle, xvid_plg_data_t *data);
static int lumi_plg_after(lumi_data_t *handle, xvid_plg_data_t *data);

/*****************************************************************************
 * The plugin entry function
 ****************************************************************************/

int
xvid_plugin_lumimasking(void * handle, int opt, void * param1, void * param2)
{
	switch(opt) {
	case XVID_PLG_INFO:
		return(lumi_plg_info((xvid_plg_info_t*)param1));
	case XVID_PLG_CREATE:
		return(lumi_plg_create((xvid_plg_create_t *)param1, (lumi_data_t **)param2));
	case XVID_PLG_DESTROY:
		return(lumi_plg_destroy((lumi_data_t *)handle, (xvid_plg_destroy_t*)param1));
	case XVID_PLG_BEFORE :
		return 0;
	case XVID_PLG_FRAME :
		return(lumi_plg_frame((lumi_data_t *)handle, (xvid_plg_data_t *)param1));
	case XVID_PLG_AFTER :
		return(lumi_plg_after((lumi_data_t *)handle, (xvid_plg_data_t *)param1));
	}

	return(XVID_ERR_FAIL);
}

/*----------------------------------------------------------------------------
 * Info plugin function
 *--------------------------------------------------------------------------*/

static int
lumi_plg_info(xvid_plg_info_t *info)
{
	/* We just require a diff quant array access */
	info->flags = XVID_REQDQUANTS;
	return(0);
}

/*----------------------------------------------------------------------------
 * Create plugin function
 *
 * Allocates the private plugin data arrays
 *--------------------------------------------------------------------------*/

static int
lumi_plg_create(xvid_plg_create_t *create, lumi_data_t **handle)
{
	lumi_data_t *lumi;
	xvid_plugin_lumimasking_t *param = (xvid_plugin_lumimasking_t *) create->param;

	if ((lumi = (lumi_data_t*)malloc(sizeof(lumi_data_t))) == NULL)
		return(XVID_ERR_MEMORY);

	lumi->method = 0;
	lumi->quant = (float*)malloc(create->mb_width*create->mb_height*sizeof(float));
	if (lumi->quant == NULL) {
		free(lumi);
		return(XVID_ERR_MEMORY);
	}

	lumi->val = (float*)malloc(create->mb_width*create->mb_height*sizeof(float));
	if (lumi->val == NULL) {
		free(lumi->quant);
		free(lumi);
		return(XVID_ERR_MEMORY);
	}

	if (param != NULL) 
		lumi->method = param->method;

	/* Bind the data structure to the handle */
	*handle = lumi;

	return(0);
}

/*----------------------------------------------------------------------------
 * Destroy plugin function
 *
 * Free the private plugin data arrays
 *--------------------------------------------------------------------------*/

static int
lumi_plg_destroy(lumi_data_t *handle, xvid_plg_destroy_t *destroy)
{
	if (handle) {
		if (handle->quant) {
			free(handle->quant);
			handle->quant = NULL;
		}
		if (handle->val) {
			free(handle->val);
			handle->val = NULL;
		}
		free(handle);
	}
	return(0);
}

/*----------------------------------------------------------------------------
 * Before plugin function
 *
 * Here is all the magic about lumimasking.
 *--------------------------------------------------------------------------*/

/* Helper function defined later */
static int normalize_quantizer_field(float *in,
									 int *out,
									 int num,
									 int min_quant,
									 int max_quant);

static int
lumi_plg_frame(lumi_data_t *handle, xvid_plg_data_t *data)
{
	int i, j;

	float global = 0.0f;

	const float DarkAmpl = 14 / 4;
	const float BrightAmpl = 10 / 3;
	float DarkThres = 90;
	float BrightThres = 200;

	const float GlobalDarkThres = 60;
	const float GlobalBrightThres = 170;

	/* Arbitrary centerpoint for variance-based AQ.  Roughly the same as used in x264. */
	float center = 14000.f;
	/* Arbitrary strength for variance-based AQ. */
	float strength = 0.2f;

	if (data->type == XVID_TYPE_BVOP) return 0;

	/* Do this for all macroblocks individually  */
	for (j = 0; j < data->mb_height; j++) {
		for (i = 0; i < data->mb_width; i++) {
			int k, l, sum = 0, sum_of_squares = 0;	
			unsigned char *ptr;

			/* Initialize the current quant value to the frame quant */
			handle->quant[j*data->mb_width + i] = (float)data->quant;

			/* Next steps compute the luminance-masking */

			/* Get the MB address */
			ptr  = data->current.plane[0];
			ptr += 16*j*data->current.stride[0] + 16*i;

			if (handle->method) { /* Variance masking mode */
				int variance = 0;
				/* Accumulate sum and sum of squares over the MB */
				for (k = 0; k < 16; k++) {
 					for (l = 0; l < 16; l++) {
						int val = ptr[k*data->current.stride[0] + l];
						sum += val;
						sum_of_squares += val * val;
					}
				}
				/* Variance = SSD - SAD^2 / (numpixels) */
				variance = sum_of_squares - sum * sum / 256;
				handle->val[j*data->mb_width + i] = (float)variance;
			}
			else { /* Luminance masking mode */
				/* Accumulate luminance */
				for (k = 0; k < 16; k++)
					for (l = 0; l < 16; l++)
						 sum += ptr[k*data->current.stride[0] + l];
			
				handle->val[j*data->mb_width + i] = (float)sum/256.0f;

				/* Accumulate the global frame luminance */
				global += (float)sum/256.0f;
			}
		}
	}

	if (handle->method) { /* Variance masking */
		/* Apply the variance masking formula to all MBs */
		for (i = 0; i < data->mb_height; i++)
		{
			for (j = 0; j < data->mb_width; j++)
			{
				float value = handle->val[i*data->mb_width + j];
				float qscale_diff = strength * logf(value / center);
				handle->quant[i*data->mb_width + j] *= (1.0f + qscale_diff);
 			}
 		}
	}
	else { /* Luminance masking */
		/* Normalize the global luminance accumulator */
		global /= data->mb_width*data->mb_height;

		DarkThres = DarkThres*global/127.0f;
		BrightThres = BrightThres*global/127.0f;


		/* Apply luminance masking only to frames where the global luminance is
		 * higher than DarkThreshold and lower than Bright Threshold */
		 if ((global < GlobalBrightThres) && (global > GlobalDarkThres)) {

			/* Apply the luminance masking formulas to all MBs */
			for (i = 0; i < data->mb_height; i++) {
				for (j = 0; j < data->mb_width; j++) {
					if (handle->val[i*data->mb_width + j] < DarkThres)
						handle->quant[i*data->mb_width + j] *= 1 + DarkAmpl * (DarkThres - handle->val[i*data->mb_width + j]) / DarkThres;
					else if (handle->val[i*data->mb_width + j] > BrightThres)
						handle->quant[i*data->mb_width + j] *= 1 + BrightAmpl * (handle->val[i*data->mb_width + j] - BrightThres) / (255 - BrightThres);
				}
			}
		}
	}

	/* Normalize the quantizer field */
	data->quant = normalize_quantizer_field(handle->quant,
											 data->dquant,
											 data->mb_width*data->mb_height,
											 data->quant,
											 MAX(2,data->quant + data->quant/2));

	/* Plugin job finished */
	return(0);
}

/*----------------------------------------------------------------------------
 * After plugin function (dummy function)
 *--------------------------------------------------------------------------*/

static int
lumi_plg_after(lumi_data_t *handle, xvid_plg_data_t *data)
{
	return(0);
}

/*****************************************************************************
 * Helper functions
 ****************************************************************************/

#define RDIFF(a, b)    ((int)(a+0.5)-(int)(b+0.5))

static int
normalize_quantizer_field(float *in,
						  int *out,
						  int num,
						  int min_quant,
						  int max_quant)
{
	int i;
	int finished;

	do {
		finished = 1;
		for (i = 1; i < num; i++) {
			if (RDIFF(in[i], in[i - 1]) > 2) {
				in[i] -= (float) 0.5;
				finished = 0;
			} else if (RDIFF(in[i], in[i - 1]) < -2) {
				in[i - 1] -= (float) 0.5;
				finished = 0;
			}

			if (in[i] > max_quant) {
				in[i] = (float) max_quant;
				finished = 0;
			}
			if (in[i] < min_quant) {
				in[i] = (float) min_quant;
				finished = 0;
			}
			if (in[i - 1] > max_quant) {
				in[i - 1] = (float) max_quant;
				finished = 0;
			}
			if (in[i - 1] < min_quant) {
				in[i - 1] = (float) min_quant;
				finished = 0;
			}
		}
	} while (!finished);

	out[0] = 0;
	for (i = 1; i < num; i++)
		out[i] = RDIFF(in[i], in[i - 1]);

	return (int) (in[0] + 0.5);
}
