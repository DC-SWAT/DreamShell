/* DreamShell ##version##
   convert.c
   Copyright (C) 2024 Maniac Vera
*/

#include "ds.h"
#include "img/decode.h"
#include "img/SegaPVRImage.h"
#include "img/stb_image.h"

static bool bPVRTwiddleTableDec = false;

int pvr_decode(const char *filename, kos_img_t *kimg)
{
	unsigned long int imageSize;

	file_t pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		// dbglog(DBG_INFO, "pvr_decode: does not exist");
		return -1;
	}

	size_t fsize = fs_total(pFile);
	if (fsize <= 0)
	{
		dbglog(DBG_INFO, "pvr_decode: empty file");
		fs_close(pFile);
		return -1;
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

	struct PVRTHeader pvrtHeader;
	unsigned int offset = ReadPVRHeader(data, &pvrtHeader);
	if (offset == 0)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_decode: wrong header");
		return -1;
	}

	enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);

	if (srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444)
	{
		free(data);
		dbglog(DBG_INFO, "pvr_decode: unsupported format");
		return -1;
	}

	imageSize = pvrtHeader.width * pvrtHeader.height * 4; // RGBA8888
	kimg->data = (unsigned char *)memalign(32, imageSize);

	if (bPVRTwiddleTableDec == false)
	{
		BuildTwiddleTable();
		bPVRTwiddleTableDec = true;
	}

	memset_sh4(kimg->data, 0, imageSize);

	if (!DecodePVR(data + offset, &pvrtHeader, kimg->data))
	{
		kos_img_free(kimg, 0);
		free(data);

		dbglog(DBG_INFO, "pvr_decode: can't decode file");
		return -1;
	}
	free(data);

	kimg->byte_count = imageSize;
	kimg->byte_count = (kimg->byte_count + 31) & ~31;
	kimg->w = pvrtHeader.width;
	kimg->h = pvrtHeader.height;
	kimg->fmt = srcFormat;

	return 0;
}

int jpg_decode(const char *filename, kos_img_t *kimg)
{
	int width = 0, height = 0, channels = 0;
	if ((kimg->data = (void *)stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha)) == NULL)
	{
		dbglog(DBG_INFO, "jpg_decode: can't decode file");
		return -1;
	}

	kimg->w = (uint32)width;
	kimg->h = (uint32)height;
	kimg->fmt = (uint32)channels;

	return 0;
}

int png_decode(const char *filename, kos_img_t *kimg)
{
	int width = 0, height = 0, channels = 0;
	if ((kimg->data = (void *)stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha)) == NULL)
	{
		dbglog(DBG_INFO, "png_decode: can't decode file");
		return -1;
	}

	kimg->w = (uint32)width;
	kimg->h = (uint32)height;
	kimg->fmt = (uint32)channels;

	return 0;
}
