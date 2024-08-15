/* DreamShell ##version##
   pvr.c
   SWAT
*/

#include "ds.h"
#include "img/load.h"
#include "img/convert.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

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


int pvr_to_jpg(const char *source_file, const char *dest_file, uint32 output_width, uint32 output_height, uint8 resolution)
{
	kos_img_t img;

	if (pvr_to_img(source_file, &img) == -1)
	{
		dbglog(DBG_INFO, "pvr_to_jpg: Error loading PVR file");
		return 0;
	}

	if (output_width > 0 && output_height > 0)
	{
		unsigned char *data_image = resize_image(img.data, img.h, img.h, output_width, output_height, STBIR_RGBA);
		if (data_image == NULL)
		{
			return 0;
		}

		img.w = output_width;
		img.h = output_height;
		img.byte_count = output_width * output_height * 4;

		free(img.data);
		img.data = data_image;
		data_image = NULL;
	}

	if (stbi_write_jpg(dest_file, img.w, img.h, 4, img.data, resolution) == 0)
	{
		kos_img_free(&img, 0);
		dbglog(DBG_INFO, "pvr_to_jpg: Error saving PNG file");
		return 0;
	}

	kos_img_free(&img, 0);
	return 1;
}

int pvr_to_png(const char *source_file, const char *dest_file, uint32 output_width, uint32 output_height, uint8 resolution)
{
	kos_img_t img;

	if (resolution > 0)
	{
		stbi_write_png_compression_level = (int)resolution;
	}
	else
	{
		stbi_write_png_compression_level = 8;
	}

	if (pvr_to_img(source_file, &img) == -1)
	{
		dbglog(DBG_INFO, "pvr_to_png: Error loading PVR file");
		return 0;
	}

	if (output_width > 0 && output_height > 0)
	{
		unsigned char *data_image = resize_image(img.data, img.h, img.h, output_width, output_height, STBIR_4CHANNEL); //STBIR_RGBA);
		if (data_image == NULL)
		{
			return 0;
		}

		img.w = output_width;
		img.h = output_height;
		img.byte_count = output_width * output_height * 4;

		free(img.data);
		img.data = data_image;
		data_image = NULL;
	}

	if (stbi_write_png(dest_file, img.h, img.h, 4, img.data, img.h * 4) == 0)
	{
		kos_img_free(&img, 0);
		dbglog(DBG_INFO, "pvr_to_png: Error saving PNG file");
		return 0;
	}

	kos_img_free(&img, 0);

	return 1;
}
