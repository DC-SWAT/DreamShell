/* DreamShell ##version##
   convert.c
   Copyright (C) 2024-2025 Maniac Vera
*/

#include "ds.h"
#include "img/utils.h"
#include "img/decode.h"
#include "img/SegaPVRImage.h"

#define STBI_NO_STDIO
#include "img/stb_image.h"

static bool bPVRTwiddleTable = false;

typedef struct
{
	file_t fd;
} kos_file;

static int kos_read(void *user, char *data, int size)
{
	kos_file *f = (kos_file *)user;
	return fs_read(f->fd, data, size);
}

static void kos_skip(void *user, int n)
{
	kos_file *f = (kos_file *)user;
	fs_seek(f->fd, n, SEEK_CUR);
}

static int kos_eof(void *user)
{
	kos_file *f = (kos_file *)user;
	return fs_tell(f->fd) >= fs_total(f->fd);
}

static stbi_io_callbacks kos_fs_callbacks = 
{
	.read = kos_read,
	.skip = kos_skip,
	.eof = kos_eof
};

int pvr_decode(const char *filename, kos_img_t *kimg, bool kos_format)
{
	unsigned long int imageSize;

	// Open file for reading
	file_t pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		dbglog(DBG_INFO, "pvr_decode: can't open file\n");
		return -1;
	}

	// Get file size
	size_t fsize = fs_total(pFile);
	if (fsize <= 0)
	{
		dbglog(DBG_INFO, "pvr_decode: empty file\n");
		fs_close(pFile);
		return -1;
	}

	// Allocate memory for file data
	uint8 *data = (uint8 *)memalign(32, fsize);
	if (!data)
	{
		fs_close(pFile);
		return -1;
	}

	// Read file data into buffer
	if (fs_read(pFile, data, fsize) == -1)
	{
		free(data);
		fs_close(pFile);
		return -1;
	}

	// Close file after reading
	fs_close(pFile);

	// Read PVR header
	struct PVRTHeader pvrtHeader;
	unsigned int offset = ReadPVRHeader(data, &pvrtHeader);
	if (offset == 0)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_decode: wrong header\n");
		return -1;
	}

	enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);
	enum TextureTypeMasks texType = (enum TextureTypeMasks)((pvrtHeader.textureAttributes >> 8) & 0xFF);

	// Validate supported 16-bit formats (only if decoding 16-bit)
	if (kos_format && srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_decode: unsupported format for 16-bit decode\n");
		return -1;
	}

	// Calculate output image size based on desired output format
	imageSize = kos_format ? (pvrtHeader.width * pvrtHeader.height * 2) : // 16-bit output size
					(pvrtHeader.width * pvrtHeader.height * 4);			  // 32-bit output size

	// Allocate output image buffer
	kimg->data = (unsigned char *)memalign(32, imageSize);
	if (!kimg->data)
	{
		free(data);
		return -1;
	}

	// Build twiddle table if not built yet
	if (!bPVRTwiddleTable)
	{
		BuildTwiddleTable();
		bPVRTwiddleTable = true;
	}

	// Clear the output buffer
	memset_sh4(kimg->data, 0, imageSize);

	// Decode based on format requested
	bool decodeSuccess;
	if (kos_format)
		decodeSuccess = DecodePVR16bit(data + offset, &pvrtHeader, kimg->data);
	else
		decodeSuccess = DecodePVR(data + offset, &pvrtHeader, kimg->data);

	if (!decodeSuccess)
	{
		kos_img_free(kimg, 0);
		free(data);
		dbglog(DBG_INFO, "pvr_decode: can't decode file\n");
		return -1;
	}

	free(data);

	// Determine base image format depending on output bits
	unsigned int base_fmt;
	if (kos_format)
	{
		// Map 16-bit PVR formats to KOS image formats
		switch (srcFormat)
		{
		case TFM_ARGB1555:
			base_fmt = KOS_IMG_FMT_ARGB1555;
			break;
		case TFM_RGB565:
			base_fmt = KOS_IMG_FMT_RGB565;
			break;
		case TFM_ARGB4444:
			base_fmt = KOS_IMG_FMT_ARGB4444;
			break;
		default:
			base_fmt = KOS_IMG_FMT_RGB565; // Safe fallback
			break;
		}
	}
	else
	{
		// For 32-bit RGBA8888 output
		base_fmt = KOS_IMG_FMT_ARGB8888;
	}

	// Determine texture format flags (twiddled, VQ, raw, etc.)
	unsigned int texFormat;
	switch (texType)
	{
	case TTM_Twiddled:
	case TTM_TwiddledMipMaps:
	case TTM_TwiddledNonSquare:
		texFormat = PVR_TXRFMT_TWIDDLED;
		break;

	case TTM_VectorQuantized:
	case TTM_VectorQuantizedMipMaps:
		texFormat = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
		break;

	case TTM_VectorQuantizedCustomCodeBook:
	case TTM_VectorQuantizedCustomCodeBookMipMaps:
		texFormat = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED;
		break;

	case TTM_Raw:
		texFormat = PVR_TXRFMT_STRIDE;
		break;

	case TTM_RawNonSquare:
	default:
		texFormat = PVR_TXRFMT_NONE;
		break;
	}

	// Compose the final image format with flags
	kimg->fmt = KOS_IMG_FMT(base_fmt, texFormat);

	// Set image width, height, and align byte count to 32 bytes
	kimg->w = pvrtHeader.width;
	kimg->h = pvrtHeader.height;
	kimg->byte_count = (imageSize + 31) & ~31;

	return 0;
}

int jpg_decode(const char *filename, kos_img_t *kimg, bool kos_format)
{
	int width = 0, height = 0, channels = 0;

	kos_file mf;
	mf.fd = fs_open(filename, O_RDONLY);

	if (mf.fd < 0)
	{
		dbglog(DBG_ERROR, "jpg_decode: can't open file\n");
		return -1;
	}

	// Load JPEG as RGBA8888
	unsigned char *rgba_data = (unsigned char *)stbi_load_from_callbacks(&kos_fs_callbacks, &mf, &width, &height, &channels, STBI_rgb_alpha);

	fs_close(mf.fd);

	if (rgba_data == NULL)
	{
		dbglog(DBG_ERROR, "jpg_decode: can't decode image\n");
		return -1;
	}

	if (kos_format)
	{
		// Convert RGBA8888 to ARGB1555
		uint16_t *converted_data = malloc(width * height * 2);
		if (!converted_data)
		{
			dbglog(DBG_ERROR, "jpg_decode: malloc failed\n");
			stbi_image_free(rgba_data);
			return -1;
		}

		rgba8888_to_argb1555(rgba_data, converted_data, width, height);
		stbi_image_free(rgba_data);

		kimg->data = converted_data;
		kimg->w = (uint32)width;
		kimg->h = (uint32)height;
		kimg->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, 0);	
	}
	else
	{
		kimg->data = rgba_data;
		kimg->w = (uint32)width;
		kimg->h = (uint32)height;
		kimg->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGBA8888, 0);
	}

	return 0;
}

int png_decode(const char *filename, kos_img_t *kimg, bool kos_format)
{
	int width = 0, height = 0, channels = 0;

	kos_file mf;
	mf.fd = fs_open(filename, O_RDONLY);

	if (mf.fd < 0)
	{
		dbglog(DBG_ERROR, "png_decode: can't open file\n");
		return -1;
	}

	// Load PNG as RGBA8888
	unsigned char *rgba_data = (unsigned char *)stbi_load_from_callbacks(
		&kos_fs_callbacks, &mf, &width, &height, &channels, STBI_rgb_alpha);

	fs_close(mf.fd);

	if (rgba_data == NULL)
	{
		dbglog(DBG_ERROR, "png_decode: can't decode image\n");
		return -1;
	}

	if (kos_format)
	{
		// Convert RGBA8888 to ARGB1555 (16bpp)
		uint16_t *converted_data = malloc(width * height * 2);
		if (!converted_data)
		{
			dbglog(DBG_ERROR, "png_decode: malloc failed\n");
			stbi_image_free(rgba_data);
			return -1;
		}

		rgba8888_to_argb1555(rgba_data, converted_data, width, height);
		stbi_image_free(rgba_data);

		kimg->data = converted_data;
		kimg->w = (uint32)width;
		kimg->h = (uint32)height;
		kimg->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, 0);	
	}
	else
	{
		// Keep as RGBA8888
		kimg->data = rgba_data;
		kimg->w = (uint32)width;
		kimg->h = (uint32)height;
		kimg->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGBA8888, 0);
	}

	return 0;
}
