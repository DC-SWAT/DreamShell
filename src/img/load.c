/* DreamShell ##version##
   load.c
   Copyright (C) 2024-2025 Maniac Vera
*/

#include "ds.h"
#include "img/SegaPVRImage.h"
#include "img/load.h"
#include "img/decode.h"
#include <kmg/kmg.h>
#include <zlib/zlib.h>
#include <malloc.h>

/* Open the pvr texture and send it to VRAM */
int pvr_to_img(const char *filename, kos_img_t *rv)
{
	return pvr_decode(filename, rv, true);
}

int gzip_kmg_to_img(const char * fn, kos_img_t * rv) {
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
