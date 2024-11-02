/* DreamShell ##version##
   convert.c
   Copyright (C) 2024 Maniac Vera
*/

#include "ds.h"
#include "img/load.h"
#include "img/convert.h"

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
	#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "img/stb_image_write.h"

#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
	#define STB_IMAGE_RESIZE_IMPLEMENTATION
#endif
#include "img/stb_image_resize2.h"

extern int stbi_write_png_compression_level;

unsigned char* resize_image(unsigned char *data, uint width, uint height, uint output_width, uint output_height, uint8 pixel_layout)
{
	unsigned char *resized_pixels = (unsigned char *)malloc(width * height * 4);

	if (width > 0 && height > 0 
		&& stbir_resize_uint8_linear(data, (int)width, (int)height, 0, resized_pixels
								, (int)output_width, (int)output_height, 0, pixel_layout) == 0)
	{
		dbglog(DBG_INFO, "resize_image error, stbir_resize_uint8_linear");

		if (resized_pixels)
		{
			free(resized_pixels);
		}

		return NULL;
	}

	return resized_pixels;
}

int pvr_to_jpg(const char *source_file, const char *dest_file, uint output_width, uint output_height, uint8 resolution)
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

int pvr_to_png(const char *source_file, const char *dest_file, uint output_width, uint output_height, uint8 level_compression)
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

int img_to_png(kos_img_t *img, const char *dest_file, uint output_width, uint output_height)
{
	unsigned char *data_image = NULL;
	if (output_width > 0 && output_height > 0)
	{	
		data_image = resize_image(img->data, img->w, img->h, output_width, output_height, STBIR_4CHANNEL);
		if (data_image == NULL)
		{
			dbglog(DBG_INFO, "img_to_png: Error resize");
			return 0;
		}
	}
	else
	{
		output_width = img->w;
		output_height = img->h;
	}

	if (stbi_write_png(dest_file, output_width, output_height, 4, data_image != NULL ? data_image : img->data, output_height * 4) == 0)
	{
		free(data_image);
		dbglog(DBG_INFO, "img_to_png: Error saving PNG file");
		return 0;
	}
	
	if (data_image != NULL)
	{
		free(data_image);
	}

	return 1;
}

int img_to_jpg(kos_img_t *img, const char *dest_file, uint output_width, uint output_height, uint8 resolution)
{
	unsigned char *data_image = NULL;
	if (output_width > 0 && output_height > 0)
	{	
		data_image = resize_image(img->data, img->w, img->h, output_width, output_height, STBIR_RGBA);
		if (data_image == NULL)
		{
			dbglog(DBG_INFO, "img_to_jpg: Error resize");
			return 0;
		}
	}
	else
	{
		output_width = img->w;
		output_height = img->h;
	}

	if (stbi_write_jpg(dest_file, output_width, output_height, 4, data_image != NULL ? data_image : img->data, resolution) == 0)
	{
		if (data_image != NULL)
		{
			free(data_image);
		}

		dbglog(DBG_INFO, "img_to_jpg: Error saving JPG file");
		return 0;
	}
	
	if (data_image != NULL)
	{
		free(data_image);
	}

	return 1;
}
