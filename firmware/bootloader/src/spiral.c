/**
 * DreamShell boot loader
 * Spiral
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#include "main.h"

static pvr_ptr_t txr_dot, txr_logo;
static float phase;
static int frame;
uint32 spiral_color = 0x44ed1800;


static int gzip_kmg_to_img(const char * fn, kos_img_t * rv) {
	gzFile f;
	kmg_header_t	hdr;

	/* Open the file */
	f = gzopen(fn, "r");
	
	if (f == NULL) {
		dbglog(DBG_ERROR, "%s: can't open file '%s'\n", __func__, fn);
		return -1;
	}

	/* Read the header */
	if (gzread(f, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		gzclose(f);
		dbglog(DBG_ERROR, "%s: can't read header from file '%s'\n", __func__, fn);
		return -2;
	}

	/* Verify a few things */
	if (hdr.magic != KMG_MAGIC || hdr.version != KMG_VERSION ||
		hdr.platform != KMG_PLAT_DC)
	{
		gzclose(f);
		dbglog(DBG_ERROR, "%s: file '%s' is incompatible:\n"
			"   magic %08lx version %d platform %d\n",
			__func__, fn, hdr.magic, (int)hdr.version, (int)hdr.platform);
		return -3;
	}


	/* Setup the kimg struct */
	rv->w = hdr.width;
	rv->h = hdr.height;
	rv->byte_count = hdr.byte_count;
	rv->data = malloc(hdr.byte_count);
	
	if (!rv->data) {
		dbglog(DBG_ERROR, "%s: can't malloc(%d) while loading '%s'\n",
			__func__, (int)hdr.byte_count, fn);
		gzclose(f);
		return -4;
	}
	
	
	int dep = 0;
	if (hdr.format & KMG_DCFMT_VQ)
		dep |= PVR_TXRLOAD_FMT_VQ;
	if (hdr.format & KMG_DCFMT_TWIDDLED)
		dep |= PVR_TXRLOAD_FMT_TWIDDLED;

	switch (hdr.format & KMG_DCFMT_MASK) {
	case KMG_DCFMT_RGB565:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
		break;

	case KMG_DCFMT_ARGB4444:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB4444, dep);
		break;

	case KMG_DCFMT_ARGB1555:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, dep);
		break;

	case KMG_DCFMT_YUV422:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_YUV422, dep);
		break;

	case KMG_DCFMT_BUMP:
		/* XXX */
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
		break;

	case KMG_DCFMT_4BPP_PAL:
	case KMG_DCFMT_8BPP_PAL:
	default:
		dbglog(DBG_ERROR, "%s: currently-unsupported KMG pixel format", __func__);
		gzclose(f);
		free(rv->data);
		return -5;
	}
	
	if (gzread(f, rv->data, rv->byte_count) != rv->byte_count) {
		dbglog(DBG_ERROR, "%s: can't read %d bytes while loading '%s'\n",
			__func__, (int)hdr.byte_count, fn);
		gzclose(f);
		free(rv->data);
		return -6;
	}

	/* Ok, all done */
	gzclose(f);

	/* If the byte count is not a multiple of 32, bump it up as well.
	   This is for DMA/SQ usage. */
	rv->byte_count = (rv->byte_count + 31) & ~31;
	
	return 0;
}

static void load_txr(const char *fn, pvr_ptr_t *txr) {
	
	char path[NAME_MAX];
	sprintf(path, "%s/%s", RES_PATH, fn);
	
	kos_img_t img;
	if (gzip_kmg_to_img(path, &img) < 0)
		assert(0);
	dbglog(DBG_INFO, "Loaded %s: %dx%d, format %d\n",
		path, (int)img.w, (int)img.h, (int)img.fmt);

	assert((img.fmt & KOS_IMG_FMT_MASK) == KOS_IMG_FMT_ARGB4444);
	*txr = pvr_mem_malloc(img.w * img.h * 2);
	pvr_txr_load_kimg(&img, *txr, 0);
	//kos_img_free(&img, 0);
}

static void draw_one_dot(float x, float y, float z) {
	pvr_vertex_t	v;

	v.flags = PVR_CMD_VERTEX;
	v.x = x-32.0/2;
	v.y = y+32.0/2;
	v.z = z;
	v.u = 0.0f; v.v = 1.0f;
	v.argb = spiral_color;
	v.oargb = 0;
	pvr_prim(&v, sizeof(v));

	v.y = y-32.0/2;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));

	v.x = x+32.0/2;
	v.y = y+32.0/2;
	v.u = 1.0f;
	v.v = 1.0f;
	pvr_prim(&v, sizeof(v));

	v.flags = PVR_CMD_VERTEX_EOL;
	v.y = y-32.0/2;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));
}

static void draw_spiral(float phase) {
	pvr_poly_hdr_t	hdr;
	pvr_poly_cxt_t	cxt;
	float		x, y, t, r, z, scale;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
		PVR_TXRFMT_ARGB4444, 32, 32, txr_dot, PVR_FILTER_BILINEAR);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	for (z=1.0f, r=15.0, t=M_PI/2 - M_PI/4 + phase; t<(6*M_PI+phase); ) {
		x = r*fcos(t); y = r*fsin(t);
		draw_one_dot(320 + x, 189 + y, z);

		scale = 12.0f - 11.0f * ((t-phase) / (6*M_PI));
		t+=scale*2*M_PI/360.0;
		r+=scale*0.6f/(2*M_PI);
		z+=0.1f;
	}
}

static float _y = 235.0f;

static void draw_logo() {
	pvr_poly_hdr_t	hdr;
	pvr_poly_cxt_t	cxt;
	pvr_vertex_t	v;
	float		x, y;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
		PVR_TXRFMT_ARGB4444, 1024, 512, txr_logo, PVR_FILTER_BILINEAR);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	x = 810.0f;
	y = _y;
	v.flags = PVR_CMD_VERTEX;
	v.x = x - 1024.0f;
	v.y = y + 512.0f;
	v.z = 0.1f;
	v.u = 0.0f; v.v = 1.0f;
	v.argb = 0xffffffff;
	v.oargb = 0;
	pvr_prim(&v, sizeof(v));

	v.y = y;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));

	v.x = x;
	v.y = y + 512.0f;
	v.u = 1.0f;
	v.v = 1.0f;
	pvr_prim(&v, sizeof(v));

	v.flags = PVR_CMD_VERTEX_EOL;
	v.y = y;
	v.v = 0.0f;
	pvr_prim(&v, sizeof(v));
	
	if(_y > 53.5f) {
		_y -= M_PI * fcos(frame * M_PI / 180.0f);
		//printf("Y = %f\n", _y);
	}
}

int spiral_init() {
	load_txr("dot.kmg.gz", &txr_dot);
	load_txr("DreamShell.kmg.gz", &txr_logo);

	phase = 0.0f;
	frame = 0;

	return 0;
}

/* Call during trans poly */
void spiral_frame() {
	draw_spiral(phase);
	draw_logo();

	frame++;
	if(frame > 360) frame = 0;
	phase = 2*M_PI * fsin(frame * 2*M_PI / 360.0f);
}

