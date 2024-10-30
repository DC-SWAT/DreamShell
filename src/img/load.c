/* DreamShell ##version##
   load.c
   Copyright (C) 2024 Maniac Vera
*/

#include "ds.h"
#include "img/SegaPVRImage.h"
#include "img/load.h"
#include <kmg/kmg.h>
#include <zlib/zlib.h>

#if defined(__DREAMCAST__)
#include <malloc.h>
#endif

static bool bPVRTwiddleTable = false;

/* Open the pvr texture and send it to VRAM */
int pvr_to_img(const char *filename, kos_img_t *rv)
{
	unsigned long int imageSize;

	file_t pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		dbglog(DBG_INFO, "pvr_to_img: does not exist");
		return -1;
	}

	size_t fsize = fs_total(pFile);
	if (fsize <= 0)
	{
		dbglog(DBG_INFO, "pvr_to_img: empty file");
	}

	uint8 *data = (uint8 *)memalign(32, fsize);	
	if (data == NULL)
	{
		fs_close(pFile);
		return -1;
	}

	if (fs_read(pFile, data, fsize) == -1)
	{
		fs_close(pFile);
		return -1;
	}

	fs_close(pFile);
	pFile = -1;

	struct PVRTHeader pvrtHeader;
	unsigned int offset = ReadPVRHeader(data, &pvrtHeader);
	if (offset == 0)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_to_img: wrong header");
		return -1;
	}

	enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);

	if (srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_to_img: unsupported format");
		return -1;
	}

	imageSize = pvrtHeader.width * pvrtHeader.height * 4; // RGBA8888
	rv->data = (unsigned char *)memalign(32, imageSize);

	if (bPVRTwiddleTable == false)
	{
		BuildTwiddleTable();
		bPVRTwiddleTable = true;
	}

	memset_sh4(rv->data, 0, imageSize);

	if (!DecodePVR(data + offset, &pvrtHeader, rv->data))
	{
		free(data);
		dbglog(DBG_INFO, "pvr_to_img: can't decode file");
		return -1;
	}
	free(data);

	rv->byte_count = imageSize;
	rv->byte_count = (rv->byte_count + 31) & ~31;
	rv->w = pvrtHeader.width;
	rv->h = pvrtHeader.height;

	uint32 dfmt = 0;	
	switch ((pvrtHeader.textureAttributes >> 8) & 0xFF)
	{
		case TTM_TwiddledMipMaps: // NOT OK
			dfmt |= PVR_TXRLOAD_FMT_TWIDDLED;
			break;

		case TTM_Twiddled: // OK
		case TTM_TwiddledNonSquare:
			dfmt |= PVR_TXRLOAD_FMT_TWIDDLED;
			break;
		
		case TTM_VectorQuantizedMipMaps:
			dfmt |= PVR_TXRLOAD_FMT_TWIDDLED | PVR_TXRLOAD_FMT_VQ;
			break;

		case TTM_VectorQuantized:		
			dfmt |= PVR_TXRLOAD_FMT_TWIDDLED | PVR_TXRLOAD_FMT_VQ;
			break;

		case TTM_VectorQuantizedCustomCodeBookMipMaps:
			dfmt |= PVR_TXRLOAD_FMT_NOTWIDDLE | PVR_TXRLOAD_FMT_VQ;
			break;

		case TTM_VectorQuantizedCustomCodeBook:
			dfmt |= PVR_TXRLOAD_FMT_NOTWIDDLE | PVR_TXRLOAD_FMT_VQ;
			break;
			
		case TTM_Raw:
			dfmt |= PVR_TXRFMT_STRIDE;
			break;
		
		case TTM_RawNonSquare: // OK
			dfmt |= PVR_TXRFMT_NONE;
			break;

		default:
			dfmt |= PVR_TXRFMT_NONE;
			break;
	}

	dfmt = 0;
	switch (srcFormat)
	{
		case TFM_ARGB1555:
			rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, dfmt);
			break;

		case TFM_RGB565:
			rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dfmt);
			break;

		default:
			rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB4444, dfmt);
			break;
	}

	return 0;
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
