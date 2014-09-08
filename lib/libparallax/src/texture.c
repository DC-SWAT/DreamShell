/* Parallax for KallistiOS ##version##

   texture.c

   (c)2002 Dan Potter
   (c)2013-2014 SWAT

*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <plx/texture.h>
//#include <png/png.h>
#include <jpeg/jpeg.h>
#include <kmg/kmg.h>
#include "utils.h"

int pvr_to_img(const char *file_name, kos_img_t *rv);
int png_to_img(const char *file_name, kos_img_t *rv);

/* See the header file for all comments and documentation */

/* Utility function to fill out the initial poly contexts */
static void fill_contexts(plx_texture_t * txr) {
	pvr_poly_cxt_txr(&txr->cxt_opaque, PVR_LIST_OP_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, PVR_FILTER_BILINEAR);
	pvr_poly_cxt_txr(&txr->cxt_trans, PVR_LIST_TR_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, PVR_FILTER_BILINEAR);
	pvr_poly_cxt_txr(&txr->cxt_pt, PVR_LIST_PT_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, PVR_FILTER_BILINEAR);

	plx_txr_flush_hdrs(txr);
}

plx_texture_t * plx_txr_load(const char * fn, int use_alpha, int txrload_flags) {
	kos_img_t	img;
	plx_texture_t	* txr;
	int		fnlen;

	/* What type of texture is it? */
	fnlen = strlen(fn);
	if (!strcasecmp(fn + fnlen - 3, "png")) {
		/* Load the texture (or try) */
		if (png_to_img(fn, &img) < 0) {
			dbglog(DBG_WARNING, "plx_txr_load: can't load texture from file '%s'\n", fn);
			return NULL;
		}
	} else if (!strcasecmp(fn + fnlen - 3, "jpg")) {
		/* Load the texture (or try) */
		if (jpeg_to_img(fn, 1, &img) < 0) {
			dbglog(DBG_WARNING, "plx_txr_load: can't load texture from file '%s'\n", fn);
			return NULL;
		}
	} else if (!strcasecmp(fn + fnlen - 3, "kmg")) {
		/* Load the texture (or try) */
		if (kmg_to_img(fn, &img) < 0) {
			dbglog(DBG_WARNING, "plx_txr_load: can't load texture from file '%s'\n", fn);
			return NULL;
		}
		use_alpha = -1;
	} else if (!strcasecmp(fn + fnlen - 3, "pvr")) {
		/* Load the texture (or try) */
		if (pvr_to_img(fn, &img) < 0) {
			dbglog(DBG_WARNING, "plx_txr_load: can't load texture from file '%s'\n", fn);
			return NULL;
		}
	} else {
		dbglog(DBG_WARNING, "plx_txr_load: unknown extension for file '%s'\n", fn);
		return NULL;
	}

	/* We got it -- allocate a texture struct */
	txr = malloc(sizeof(plx_texture_t));
	if (txr == NULL) {
		dbglog(DBG_WARNING, "plx_txr_load: can't allocate memory for texture struct for '%s'\n", fn);
		kos_img_free(&img, 0);
		return NULL;
	}

	/* Setup the struct */
	txr->ptr = pvr_mem_malloc(img.byte_count);
	txr->w = img.w;
	txr->h = img.h;
	if (use_alpha == -1) {
		/* Pull from the image source */
		switch (KOS_IMG_FMT_I(img.fmt) & KOS_IMG_FMT_MASK) {
		case KOS_IMG_FMT_RGB565:
			txr->fmt = PVR_TXRFMT_RGB565;
			break;
		case KOS_IMG_FMT_ARGB4444:
			txr->fmt = PVR_TXRFMT_ARGB4444;
			break;
		case KOS_IMG_FMT_ARGB1555:
			txr->fmt = PVR_TXRFMT_ARGB1555;
			break;
		default:
			/* shrug */
			dbglog(DBG_WARNING, "plx_txr_load: unknown format '%x'\n",
				(int)(KOS_IMG_FMT_I(img.fmt) & KOS_IMG_FMT_MASK));
			txr->fmt = PVR_TXRFMT_RGB565;
			break;
		}
	} else {
		txr->fmt = use_alpha ? PVR_TXRFMT_ARGB4444 : PVR_TXRFMT_RGB565;
	}
	if (KOS_IMG_FMT_D(img.fmt) & PVR_TXRLOAD_FMT_VQ)
		txr->fmt |= PVR_TXRFMT_VQ_ENABLE;

	/* Did we actually get the memory? */
	if (txr->ptr == NULL) {
		dbglog(DBG_WARNING, "plx_txr_load: can't allocate texture ram for '%s'\n", fn);
		kos_img_free(&img, 0);
		free(txr);
		return NULL;
	}

	/* Load it up and twiddle it */
	pvr_txr_load_kimg(&img, txr->ptr, txrload_flags);
	kos_img_free(&img, 0);

	/* Setup the poly context structs */
	fill_contexts(txr);

	return txr;
}

plx_texture_t * plx_txr_canvas(int w, int h, int fmt) {
	plx_texture_t	* txr;

	/* Allocate a texture struct */
	txr = malloc(sizeof(plx_texture_t));
	if (txr == NULL) {
		dbglog(DBG_WARNING, "plx_txr_canvas: can't allocate memory for %dx%d canvas texture\n", w, h);
		return NULL;
	}

	/* Setup the struct */
	txr->ptr = pvr_mem_malloc(w * h * 2);
	txr->w = w;
	txr->h = h;
	txr->fmt = fmt;

	/* Did we actually get the memory? */
	if (txr->ptr == NULL) {
		dbglog(DBG_WARNING, "plx_txr_canvas: can't allocate texture ram for %dx%d canvas texture\n", w, h);
		free(txr);
		return NULL;
	}

	/* Setup the poly context structs */
	fill_contexts(txr);

	return txr;
}

void plx_txr_destroy(plx_texture_t * txr) {
	assert( txr != NULL );
	if (txr == NULL) return;

	if (txr->ptr != NULL) {
		/* Free the PVR memory */
		pvr_mem_free(txr->ptr);
	}

	/* Free the struct itself */
	free(txr);
}

void plx_txr_setfilter(plx_texture_t * txr, int mode) {
	assert( txr != NULL );
	if (txr == NULL) return;

	txr->cxt_opaque.txr.filter = mode;
	txr->cxt_trans.txr.filter = mode;
	txr->cxt_pt.txr.filter = mode;
	plx_txr_flush_hdrs(txr);
}

void plx_txr_setuvclamp(plx_texture_t * txr, int umode, int vmode) {
	int mode;
	
	assert( txr != NULL );
	if (txr == NULL) return;

	if (umode == PLX_UV_REPEAT && vmode == PLX_UV_REPEAT)
		mode = PVR_UVCLAMP_NONE;
	else if (umode == PLX_UV_REPEAT && vmode == PLX_UV_CLAMP)
		mode = PVR_UVCLAMP_V;
	else if (umode == PLX_UV_CLAMP && vmode == PLX_UV_REPEAT)
		mode = PVR_UVCLAMP_U;
	else if (umode == PLX_UV_CLAMP && vmode == PLX_UV_CLAMP)
		mode = PVR_UVCLAMP_UV;
	else {
		assert_msg( 0, "Invalid UV clamp mode" );
		mode = PVR_UVCLAMP_NONE;
	}

	txr->cxt_opaque.txr.uv_clamp = mode;
	txr->cxt_trans.txr.uv_clamp = mode;
	txr->cxt_pt.txr.uv_clamp = mode;
	plx_txr_flush_hdrs(txr);
}

void plx_txr_flush_hdrs(plx_texture_t * txr) {
	assert( txr != NULL );
	if (txr == NULL) return;

	pvr_poly_compile(&txr->hdr_opaque, &txr->cxt_opaque);
	pvr_poly_compile(&txr->hdr_trans, &txr->cxt_trans);
	pvr_poly_compile(&txr->hdr_pt, &txr->cxt_pt);
}

void plx_txr_send_hdr(plx_texture_t * txr, int list, int flush) {
	assert( txr != NULL );
	if (txr == NULL) return;

	/* Flush the poly hdrs if necessary */
	if (flush)
		plx_txr_flush_hdrs(txr);

	/* Figure out which list to send for */
	switch (list) {
	case PVR_LIST_OP_POLY:
		pvr_prim(&txr->hdr_opaque, sizeof(txr->hdr_opaque));
		break;
	case PVR_LIST_TR_POLY:
		pvr_prim(&txr->hdr_trans, sizeof(txr->hdr_trans));
		break;
	case PVR_LIST_PT_POLY:
		pvr_prim(&txr->hdr_pt, sizeof(txr->hdr_pt));
		break;
	default:
		assert_msg( 0, "Invalid list specification" );
	}
}
