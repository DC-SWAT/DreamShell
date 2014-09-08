/* Parallax for KallistiOS ##version##

   context.c

   (c)2002 Dan Potter

*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <plx/context.h>

/* See the header file for all comments and documentation */

/* Our working context and header */
static pvr_poly_cxt_t	cxt_working;
static pvr_poly_hdr_t	hdr_working_op, hdr_working_tr, hdr_working_pt;

static void compile_cxts() {
	cxt_working.list_type = PVR_LIST_OP_POLY;
	cxt_working.gen.alpha = PVR_ALPHA_DISABLE;
	cxt_working.txr.env = PVR_TXRENV_MODULATE;
	pvr_poly_compile(&hdr_working_op, &cxt_working);

	cxt_working.list_type = PVR_LIST_TR_POLY;
	cxt_working.gen.alpha = PVR_ALPHA_ENABLE;
	cxt_working.txr.env = PVR_TXRENV_MODULATEALPHA;
	pvr_poly_compile(&hdr_working_tr, &cxt_working);

	cxt_working.list_type = PVR_LIST_PT_POLY;
	cxt_working.gen.alpha = PVR_ALPHA_ENABLE;
	cxt_working.txr.env = PVR_TXRENV_MODULATEALPHA;
	pvr_poly_compile(&hdr_working_pt, &cxt_working);
}

void plx_cxt_init() {
	pvr_poly_cxt_col(&cxt_working, PVR_LIST_TR_POLY);
	cxt_working.gen.culling = PVR_CULLING_NONE;
}

void plx_cxt_texture(plx_texture_t * txr) {
	if (txr) {
		memcpy_sh4(&cxt_working.txr, &txr->cxt_opaque.txr, sizeof(cxt_working.txr));
		cxt_working.txr.enable = PVR_TEXTURE_ENABLE;
	} else {
		cxt_working.txr.enable = PVR_TEXTURE_DISABLE;
	}

	compile_cxts();
}

void plx_cxt_blending(int src, int dst) {
	cxt_working.blend.src = src;
	cxt_working.blend.dst = dst;

	compile_cxts();
}

void plx_cxt_culling(int type) {
	cxt_working.gen.culling = type;
	compile_cxts();
}

void plx_cxt_fog(int type) {
	cxt_working.gen.fog_type = type;
	compile_cxts();
}

void plx_cxt_send(int type) {
	switch (type) {
	case PVR_LIST_OP_POLY:
		pvr_prim(&hdr_working_op, sizeof(pvr_poly_hdr_t));
		break;
	case PVR_LIST_TR_POLY:
		pvr_prim(&hdr_working_tr, sizeof(pvr_poly_hdr_t));
		break;
	case PVR_LIST_PT_POLY:
		pvr_prim(&hdr_working_pt, sizeof(pvr_poly_hdr_t));
		break;
	default:
		assert_msg( 0, "List type not handled by plx_cxt_send" );
	}
}

