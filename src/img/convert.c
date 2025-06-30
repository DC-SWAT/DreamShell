/* DreamShell ##version##
   convert.c
   Copyright (C) 2024-2025 Maniac Vera
*/

#include "ds.h"
#include "img/utils.h"
#include "img/load.h"
#include "img/convert.h"
#include "img/SegaPVRImage.h"

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
	#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "img/stb_image_write.h"

#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
	#define STB_IMAGE_RESIZE_IMPLEMENTATION
#endif
#include "img/stb_image_resize2.h"

extern int stbi_write_png_compression_level;


uint8_t convert_kos_flags_to_pvr_texture_type(uint16_t kos_flags)
{
	// Check for Vector Quantized with twiddle flags first
	if ((kos_flags & PVR_TXRFMT_VQ_ENABLE) && !(kos_flags & PVR_TXRFMT_NONTWIDDLED))
	{
		// VQ + Twiddled
		return TTM_VectorQuantized; // 0x03
	}

	if ((kos_flags & PVR_TXRFMT_VQ_ENABLE) && (kos_flags & PVR_TXRFMT_NONTWIDDLED))
	{
		// VQ + Non-twiddled (Custom CodeBook)
		return TTM_VectorQuantizedCustomCodeBook; // 0x10
	}

	if (!(kos_flags & PVR_TXRFMT_VQ_ENABLE))
	{
		if (kos_flags & PVR_TXRFMT_STRIDE)
		{
			// Strided textures, usually raw format
			return TTM_Raw; // 0x0B
		}
		else if (kos_flags & PVR_TXRFMT_TWIDDLED)
		{
			// Twiddled textures
			return TTM_Twiddled; // 0x01
		}
		else if (kos_flags & PVR_TXRFMT_NONTWIDDLED)
		{
			// Non-twiddled but no VQ or stride, possibly RawNonSquare
			return TTM_RawNonSquare; // 0x09
		}
	}

	// Fallback to RawNonSquare as safe default
	return TTM_RawNonSquare; // 0x09
}

static int convert_pvr_image_data(const uint16_t *src, uint8_t **out, int width, int height, int format)
{
	int comp = 0;
	uint8_t *dst = NULL;

	switch (format)
	{
	case KOS_IMG_FMT_RGB565:
		comp = 3;
		dst = malloc(width * height * comp);
		if (!dst)
			return 0;
		rgb565_to_rgb888(src, dst, width, height);
		break;

	case KOS_IMG_FMT_ARGB1555:
		comp = 4;
		dst = malloc(width * height * comp);
		if (!dst)
			return 0;
		argb1555_to_rgba8888(src, dst, width, height);
		break;

	case KOS_IMG_FMT_ARGB4444:
		comp = 4;
		dst = malloc(width * height * comp);
		if (!dst)
			return 0;
		argb4444_to_rgba8888(src, dst, width, height);
		break;

	default:
		return 0;
	}

	*out = dst;
	return comp;
}

static void kos_fs_writer(void *context, void *data, int size)
{
	file_t fd = (file_t)(intptr_t)context;
	fs_write(fd, data, size);
}

unsigned char *resize_image(const kos_img_t *img, uint32_t output_width, uint32_t output_height)
{
	if (!img || !img->data || img->w == 0 || img->h == 0)
		return NULL;

	const uint32_t input_width = img->w;
	const uint32_t input_height = img->h;
	const uint32_t pixel_count = input_width * input_height;

	// Extract base format (low 16 bits) from fmt
	const uint16_t base_fmt = img->fmt & 0xFFFF;

	unsigned char *rgba_input = NULL;
	unsigned char *resized_rgba = NULL;
	unsigned char *output_data = NULL;

	// Prepare RGBA input buffer, convert if needed
	if (base_fmt == KOS_IMG_FMT_RGBA8888)
	{
		// Already RGBA8888, just copy
		rgba_input = malloc(pixel_count * 4);
		if (!rgba_input)
			return NULL;
		memcpy(rgba_input, img->data, pixel_count * 4);
	}
	else if (base_fmt == KOS_IMG_FMT_ARGB8888)
	{
		// Convert ARGB8888 to RGBA8888 for resizing
		rgba_input = malloc(pixel_count * 4);
		if (!rgba_input)
			return NULL;
		argb8888_to_rgba8888((const uint8_t *)img->data, rgba_input, input_width, input_height);
	}
	else
	{
		// Convert other formats to RGBA8888
		rgba_input = malloc(pixel_count * 4);
		if (!rgba_input)
			return NULL;

		switch (base_fmt)
		{
		case KOS_IMG_FMT_RGB565:
			rgb565_to_rgba8888((const uint16_t *)img->data, rgba_input, input_width, input_height);
			break;
		case KOS_IMG_FMT_ARGB1555:
			argb1555_to_rgba8888((const uint16_t *)img->data, rgba_input, input_width, input_height);
			break;
		case KOS_IMG_FMT_ARGB4444:
			argb4444_to_rgba8888((const uint16_t *)img->data, rgba_input, input_width, input_height);
			break;
		case KOS_IMG_FMT_RGB888:
			rgb888_to_rgba8888((const uint8_t *)img->data, rgba_input, input_width, input_height);
			break;
		default:
			free(rgba_input);
			return NULL;
		}
	}

	// Resize using stb_image_resize (4 channels)
	resized_rgba = stbir_resize_uint8_linear(
		rgba_input,
		(int)input_width, (int)input_height, 0, NULL,
		(int)output_width, (int)output_height, 0,
		4);

		
	free(rgba_input);

	if (!resized_rgba)
		return NULL;

	// Convert back to original format if needed
	if (base_fmt == KOS_IMG_FMT_RGBA8888)
	{
		output_data = resized_rgba; // no conversion needed
	}
	else if (base_fmt == KOS_IMG_FMT_ARGB8888)
	{
		output_data = malloc(output_width * output_height * 4);
		if (!output_data)
		{
			free(resized_rgba);
			return NULL;
		}
		rgba8888_to_argb8888(resized_rgba, output_data, output_width, output_height);
		free(resized_rgba);
	}
	else
	{
		// Convert from RGBA8888 back to 16bpp formats
		output_data = malloc(output_width * output_height * 2);
		if (!output_data)
		{
			free(resized_rgba);
			return NULL;
		}

		switch (base_fmt)
		{
		case KOS_IMG_FMT_RGB565:
			rgba8888_to_rgb565(resized_rgba, (uint16_t *)output_data, output_width, output_height);
			break;
		case KOS_IMG_FMT_ARGB1555:
			rgba8888_to_argb1555(resized_rgba, (uint16_t *)output_data, output_width, output_height);
			break;
		case KOS_IMG_FMT_ARGB4444:
			rgba8888_to_argb4444(resized_rgba, (uint16_t *)output_data, output_width, output_height);
			break;
		default:
			free(resized_rgba);
			free(output_data);
			return NULL;
		}
		free(resized_rgba);
	}

	return output_data;
}

int pvr_to_jpg(const char *source_file, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t resolution)
{
	kos_img_t img;

	if (pvr_to_img(source_file, &img) == -1)
	{
		dbglog(DBG_INFO, "pvr_to_jpg: Error loading PVR file");
		return 0;
	}

	if (img_to_jpg(&img, dest_file, output_width, output_height, resolution) == 0)
	{
		kos_img_free(&img, 0);
		return 0;
	}

	kos_img_free(&img, 0);
	return 1;
}

int pvr_to_png(const char *source_file, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t level_compression)
{
	kos_img_t img;

	stbi_write_png_compression_level = 0;
	if (level_compression > 0)
	{
		stbi_write_png_compression_level = (int)level_compression;
	}

	if (pvr_to_img(source_file, &img) == -1)
	{
		dbglog(DBG_INFO, "pvr_to_png: Error loading PVR file");
		return 0;
	}

	if (img_to_png(&img, dest_file, output_width, output_height) == 0)
	{
		kos_img_free(&img, 0);
		return 0;
	}

	kos_img_free(&img, 0);
	return 1;
}

int img_to_pvr(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height)
{
	file_t fd = fs_open(dest_file, O_WRONLY | O_TRUNC);
	if (fd == FILEHND_INVALID)
	{
		dbglog(DBG_ERROR, "img_to_pvr: failed to open file for writing\n");
		return 0;
	}

	const uint32_t width = img->w;
	const uint32_t height = img->h;
	const uint32_t format = img->fmt;
	const void *data = img->data;

	unsigned char *resized_data = NULL;
	unsigned char *data_to_convert = NULL;
	uint16_t *converted_data = NULL;

	// Resize if needed
	if (output_width > 0 && output_height > 0 && (output_width != width || output_height != height))
	{
		resized_data = resize_image(img, output_width, output_height);
		if (!resized_data)
			goto fail_resize;

		data_to_convert = resized_data;
	}
	else
	{
		output_width = width;
		output_height = height;

		// If image is already 16bpp, no need to convert
		switch (format & 0xFF)
		{
		case KOS_IMG_FMT_RGB565:
		case KOS_IMG_FMT_ARGB4444:
		case KOS_IMG_FMT_ARGB1555:
			data_to_convert = (unsigned char *)data;
			break;

		case KOS_IMG_FMT_RGB888:
			converted_data = malloc(output_width * output_height * 2);
			if (!converted_data)
				goto fail_alloc;
			rgb888_to_argb1555((const uint8_t *)data, converted_data, output_width, output_height);
			data_to_convert = (unsigned char *)converted_data;
			break;

		case KOS_IMG_FMT_RGBA8888:
			converted_data = malloc(output_width * output_height * 2);
			if (!converted_data)
				goto fail_alloc;
			rgba8888_to_argb1555((const uint8_t *)data, converted_data, output_width, output_height);
			data_to_convert = (unsigned char *)converted_data;
			break;

		default:
			dbglog(DBG_ERROR, "img_to_pvr: unsupported format without resize\n");
			goto fail_cleanup;
		}
	}

	// Write GBIX header if needed (always write it here)
	struct GBIXHeader gbix = {
		.version = 0x58494247, // 'GBIX'
		.nextTagOffset = 8,
		.globalIndex = 0};

	// Write GBIX header to file
	if (fs_write(fd, &gbix.version, sizeof(gbix.version)) != sizeof(gbix.version) ||
		fs_write(fd, &gbix.nextTagOffset, sizeof(gbix.nextTagOffset)) != sizeof(gbix.nextTagOffset) ||
		fs_write(fd, &gbix.globalIndex, sizeof(gbix.globalIndex)) != sizeof(gbix.globalIndex))
	{
		dbglog(DBG_ERROR, "img_to_pvr: failed to write GBIX header\n");
		goto fail_cleanup;
	}

	// Extract color format (low 16 bits) and texture flags (high 16 bits)
	uint16_t kos_color_fmt = format & 0xFFFF;
	uint16_t kos_flags = (format >> 16) & 0xFFFF;

	// Map KOS flags back to legacy texture type (TTM_*)
	uint16_t texture_type = convert_kos_flags_to_pvr_texture_type(kos_flags);

	// Convert KOS color format to TFM_* used in PVR headers
	uint16_t tfm_color_fmt;
	switch (kos_color_fmt)
	{
	case KOS_IMG_FMT_ARGB1555:
		tfm_color_fmt = TFM_ARGB1555;
		break;
	case KOS_IMG_FMT_RGB565:
		tfm_color_fmt = TFM_RGB565;
		break;
	case KOS_IMG_FMT_ARGB4444:
		tfm_color_fmt = TFM_ARGB4444;
		break;
	default:
		tfm_color_fmt = TFM_RGB565;
		break; // fallback
	}

	// Combine texture type (<<8) and color format into textureAttributes
	uint16_t texture_attributes = (texture_type << 8) | tfm_color_fmt;

	// Compose PVRT header using the format from img->fmt (includes twiddle flags etc.)
	struct PVRTHeader header = {
		.version = 0x54525650, // 'PVRT'
		.textureDataSize = output_width * output_height * 2,
		.textureAttributes = texture_attributes,
		.width = output_width,
		.height = output_height};

	// Write PVRT header to file
	if (fs_write(fd, &header, sizeof(header)) != sizeof(header))
	{
		dbglog(DBG_ERROR, "img_to_pvr: failed to write PVRT header\n");
		goto fail_cleanup;
	}

	// Write image data to file
	if (fs_write(fd, data_to_convert, header.textureDataSize) != header.textureDataSize)
	{
		dbglog(DBG_ERROR, "img_to_pvr: failed to write image data\n");
		goto fail_cleanup;
	}

	fs_close(fd);

	return 1;

fail_alloc:
	dbglog(DBG_ERROR, "img_to_pvr: memory allocation failed\n");
	goto fail_cleanup;

fail_resize:
	dbglog(DBG_ERROR, "img_to_pvr: resizing failed\n");

fail_cleanup:
	fs_close(fd);

	if (resized_data)
		free(resized_data);

	if (converted_data)
		free(converted_data);

	return 0;
}

int img_to_png(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height)
{
	uint8_t *converted_data = NULL;
	int comp = 0;

	// Check if image format is already 8-bit per channel RGB or RGBA
	if (img->fmt == KOS_IMG_FMT_RGB888)
	{
		comp = 3;
		converted_data = img->data; // Use original data directly
	}
	else if (img->fmt == KOS_IMG_FMT_RGBA8888)
	{
		comp = 4;
		converted_data = img->data; // Use original data directly
	}
	else
	{
		// Convert from 16-bit PVR format to 8-bit per channel format
		comp = convert_pvr_image_data(img->data, &converted_data, img->w, img->h, img->fmt);
		if (comp == 0)
		{
			dbglog(DBG_INFO, "img_to_png: Unsupported image format or conversion failed");
			return 0;
		}
	}

	unsigned char *data_image = NULL;
	bool resized = false;

	// Resize if needed and if output size differs from original
	if (output_width > 0 && output_height > 0 && (output_width != img->w || output_height != img->h))
	{
		kos_img_t temp_img = {
			.data = converted_data,
			.w = img->w,
			.h = img->h,
			.fmt = (comp == 4) ? KOS_IMG_FMT_RGBA8888 : KOS_IMG_FMT_RGB888
		};

		data_image = resize_image(&temp_img, output_width, output_height);
		if (data_image == NULL)
		{
			dbglog(DBG_INFO, "img_to_png: Error resizing image");
			if (converted_data != img->data)
				free(converted_data);
			return 0;
		}
		resized = true;
	}
	else
	{
		output_width = img->w;
		output_height = img->h;
		data_image = converted_data;
	}

	file_t fd = fs_open(dest_file, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		dbglog(DBG_INFO, "img_to_png: Error opening file with fs_open");
		if (resized)
			free(data_image);
		if (converted_data != img->data)
			free(converted_data);
		return 0;
	}

	if (stbi_write_png_to_func(kos_fs_writer, (void *)(intptr_t)fd,
							   output_width, output_height, comp,
							   data_image, output_width * comp) == 0)
	{
		fs_close(fd);
		if (resized)
			free(data_image);
		if (converted_data != img->data)
			free(converted_data);
		dbglog(DBG_INFO, "img_to_png: Error saving PNG with stbi_write_png_to_func");
		return 0;
	}

	fs_close(fd);

	if (resized)
		free(data_image);
	if (converted_data != img->data)
		free(converted_data);

	return 1;
}

int img_to_jpg(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t resolution)
{
	uint8_t *converted_data = NULL;
	int comp = 0;

	// Check if image format is already 8-bit per channel RGB or RGBA
	if (img->fmt == KOS_IMG_FMT_RGB888)
	{
		comp = 3;
		converted_data = img->data; // Use original data directly
	}
	else if (img->fmt == KOS_IMG_FMT_RGBA8888)
	{
		comp = 4;
		converted_data = img->data; // Use original data directly
	}
	else
	{
		// Convert from 16-bit PVR format to 8-bit per channel format
		comp = convert_pvr_image_data(img->data, &converted_data, img->w, img->h, img->fmt);
		if (comp == 0)
		{
			dbglog(DBG_INFO, "img_to_jpg: Unsupported image format or conversion failed");
			return 0;
		}
	}

	unsigned char *data_image = NULL;
	bool resized = false;

	// Resize if needed and if output size differs from original
	if (output_width > 0 && output_height > 0 && (output_width != img->w || output_height != img->h))
	{
		kos_img_t temp_img = {
			.data = converted_data,
			.w = img->w,
			.h = img->h,
			.fmt = (comp == 4) ? KOS_IMG_FMT_RGBA8888 : KOS_IMG_FMT_RGB888
		};

		data_image = resize_image(&temp_img, output_width, output_height);
		
		if (data_image == NULL)
		{
			dbglog(DBG_INFO, "img_to_jpg: Error resizing image");
			if (converted_data != img->data)
				free(converted_data);
			return 0;
		}
		resized = true;
	}
	else
	{
		output_width = img->w;
		output_height = img->h;
		data_image = converted_data;
	}

	// JPG format does not support alpha channel; if comp == 4, drop alpha by converting to 3 channels
	if (comp == 4)
	{
		// Allocate buffer for RGB data
		unsigned char *rgb_data = malloc(output_width * output_height * 3);
		if (!rgb_data)
		{
			dbglog(DBG_INFO, "img_to_jpg: Memory allocation failed for RGB conversion");
			if (resized)
				free(data_image);
			if (converted_data != img->data)
				free(converted_data);
			return 0;
		}

		// Convert RGBA to RGB by discarding alpha
		for (uint32_t i = 0; i < output_width * output_height; i++)
		{
			rgb_data[i * 3 + 0] = data_image[i * 4 + 0];
			rgb_data[i * 3 + 1] = data_image[i * 4 + 1];
			rgb_data[i * 3 + 2] = data_image[i * 4 + 2];
		}

		if (resized)
			free(data_image);
		data_image = rgb_data;
		comp = 3;
		resized = true; // We must free rgb_data later
	}

	file_t fd = fs_open(dest_file, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		dbglog(DBG_INFO, "img_to_jpg: Error opening file with fs_open");
		if (resized)
			free(data_image);
		if (converted_data != img->data)
			free(converted_data);
		return 0;
	}

	if (stbi_write_jpg_to_func(kos_fs_writer, (void *)(intptr_t)fd,
							   output_width, output_height, comp,
							   data_image, resolution) == 0)
	{
		fs_close(fd);
		if (resized)
			free(data_image);
		if (converted_data != img->data)
			free(converted_data);
		dbglog(DBG_INFO, "img_to_jpg: Error writing JPG with stbi_write_jpg_to_func");
		return 0;
	}

	fs_close(fd);

	if (resized)
		free(data_image);
	if (converted_data != img->data)
		free(converted_data);

	return 1;
}
