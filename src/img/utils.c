/* DreamShell ##version##
   utils.c
   Copyright (C) 2024-2025 Maniac Vera
*/

#include "ds.h"
#include "img/utils.h"
#include <malloc.h>

bool pvr_is_alpha(const char *filename)
{
	struct PVRTHeader pvrtHeader;
	if (!pvr_header(filename, &pvrtHeader))
		return false;

	enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);

	if (srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444)
	{	
		dbglog(DBG_INFO, "pvr_is_alpha: unsupported format");
		return false;
	}
	
	return srcFormat != TFM_RGB565;	
}

bool pvr_header(const char *filename, struct PVRTHeader *pvrtHeader)
{
	file_t pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		return false;
	}

	size_t fsize = 512;
	uint8 data[512] __attribute__((aligned(32)));

	if (fs_read(pFile, data, fsize) == -1)
	{
		fs_close(pFile);
		return false;
	}

	fs_close(pFile);

	unsigned int offset = ReadPVRHeader(data, pvrtHeader);

	if (offset == 0)
	{	
		dbglog(DBG_INFO, "pvr_is_alpha: wrong header");		
		return false;
	}

	return true;
}

void rgb565_to_rgb888(const uint16_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
	for (int i = 0; i < width * height; i++)
	{
		uint16_t px = src[i];
		dst[i * 3 + 0] = ((px >> 11) & 0x1F) << 3; // R
		dst[i * 3 + 1] = ((px >> 5) & 0x3F) << 2;  // G
		dst[i * 3 + 2] = (px & 0x1F) << 3;		   // B
	}
}

void rgb565_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
	for (uint32_t i = 0; i < w * h; i++)
	{
		uint16_t px = src[i];
		uint8_t r = (px >> 11) & 0x1F;
		uint8_t g = (px >> 5) & 0x3F;
		uint8_t b = px & 0x1F;

		dst[i * 4 + 0] = (r << 3) | (r >> 2);
		dst[i * 4 + 1] = (g << 2) | (g >> 4);
		dst[i * 4 + 2] = (b << 3) | (b >> 2);
		dst[i * 4 + 3] = 255;
	}
}

void argb4444_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
	for (uint32_t i = 0; i < w * h; i++)
	{
		uint16_t px = src[i];
		uint8_t a = (px >> 12) & 0xF;
		uint8_t r = (px >> 8) & 0xF;
		uint8_t g = (px >> 4) & 0xF;
		uint8_t b = px & 0xF;

		dst[i * 4 + 0] = (r << 4) | r;
		dst[i * 4 + 1] = (g << 4) | g;
		dst[i * 4 + 2] = (b << 4) | b;
		dst[i * 4 + 3] = (a << 4) | a;
	}
}

void argb1555_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
	for (uint32_t i = 0; i < w * h; i++)
	{
		uint16_t px = src[i];
		uint8_t a = (px & 0x8000) ? 255 : 0;
		uint8_t r = (px >> 10) & 0x1F;
		uint8_t g = (px >> 5) & 0x1F;
		uint8_t b = px & 0x1F;

		dst[i * 4 + 0] = (r << 3) | (r >> 2);
		dst[i * 4 + 1] = (g << 3) | (g >> 2);
		dst[i * 4 + 2] = (b << 3) | (b >> 2);
		dst[i * 4 + 3] = a;
	}
}

void argb8888_to_rgba8888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
	uint32_t pixel_count = width * height;
	for (uint32_t i = 0; i < pixel_count; i++)
	{
		// ARGB: A R G B -> RGBA: R G B A
		dst[i * 4 + 0] = src[i * 4 + 1]; // R
		dst[i * 4 + 1] = src[i * 4 + 2]; // G
		dst[i * 4 + 2] = src[i * 4 + 3]; // B
		dst[i * 4 + 3] = src[i * 4 + 0]; // A
	}
}

void rgba8888_to_argb1555(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
	if (src && dst)
	{
		for (uint32_t i = 0; i < width * height; i++)
		{
			uint8_t r = src[i * 4 + 0] >> 3;			// 5 bits red
			uint8_t g = src[i * 4 + 1] >> 3;			// 5 bits green
			uint8_t b = src[i * 4 + 2] >> 3;			// 5 bits blue
			uint8_t a = (src[i * 4 + 3] > 127) ? 1 : 0; // 1 bit alfa

			dst[i] = (a << 15) | (r << 10) | (g << 5) | b;
		}
	}
}

void rgb888_to_argb1555(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
	if (src && dst)
	{
		for (uint32_t i = 0; i < width * height; i++)
		{
			uint8_t r = src[i * 3 + 0] >> 3; // 5 bits red
			uint8_t g = src[i * 3 + 1] >> 3; // 5 bits green
			uint8_t b = src[i * 3 + 2] >> 3; // 5 bits blue
			uint8_t a = 1;					 // opaque alpha

			dst[i] = (a << 15) | (r << 10) | (g << 5) | b;
		}
	}
}

void rgb888_to_rgba8888(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
	if (src && dst)
	{
		for (uint32_t i = 0; i < w * h; i++)
		{
			dst[i * 4 + 0] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 2] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;  // Full alpha
		}
	}
}

void rgba8888_to_rgb565(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
	for (uint32_t i = 0; i < width * height; i++)
	{
		uint8_t r = src[i * 4 + 0] >> 3;
		uint8_t g = src[i * 4 + 1] >> 2;
		uint8_t b = src[i * 4 + 2] >> 3;

		dst[i] = (r << 11) | (g << 5) | b;
	}
}

void rgba8888_to_argb4444(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
	for (uint32_t i = 0; i < width * height; i++)
	{
		uint8_t r = src[i * 4 + 0] >> 4;
		uint8_t g = src[i * 4 + 1] >> 4;
		uint8_t b = src[i * 4 + 2] >> 4;
		uint8_t a = src[i * 4 + 3] >> 4;

		dst[i] = (a << 12) | (r << 8) | (g << 4) | b;
	}
}

void rgba8888_to_argb8888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    uint32_t pixel_count = width * height;
    for (uint32_t i = 0; i < pixel_count; i++)
    {
        dst[i * 4 + 0] = src[i * 4 + 3]; // A
        dst[i * 4 + 1] = src[i * 4 + 0]; // R
        dst[i * 4 + 2] = src[i * 4 + 1]; // G
        dst[i * 4 + 3] = src[i * 4 + 2]; // B
    }
}
