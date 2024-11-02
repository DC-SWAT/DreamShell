/* DreamShell ##version##
   copy.c
   Copyright (C) 2024 Maniac Vera
*/

#include "ds.h"
#include <png/png.h>
#include <jpeg/jpeg.h>
#include "img/copy.h"
#include "img/load.h"
#include "img/convert.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

#if defined(__DREAMCAST__)
#include <malloc.h>
#endif

bool copy_image(const char *image_source, const char *image_dest, bool is_alpha, bool rewrite, uint width, uint height)
{
	bool copied = false;

	if (!rewrite && FileExists(image_dest))
	{
		dbglog(DBG_INFO, "copy_image error: image file exists");
	}
	else if (width >= 0 && height >= 0)
	{
		bool has_changes = false;
		char *image_type_source = strrchr(image_source, '.');
		char *image_type_dest = strrchr(image_dest, '.');
		uint output_width = 0;
		uint output_height = 0;
		kos_img_t img;
		unsigned char *data_image = NULL;

		if (width > 0 && height > 0)
		{
			if (width != output_width || height != output_height)
			{
				has_changes = true;
			}

			output_width = width;
			output_height = height;
		}

		if (strcasecmp(image_type_source, image_type_dest) == 0)
		{
			has_changes = true;
		}

		if (!has_changes)
		{
			if (fs_copy(image_source, image_dest) > 0)
			{
				copied = true;
			}
			else
			{
				dbglog(DBG_INFO, "copy_image: Error copying file");
			}
		}
		else if (strcasecmp(image_type_source, ".png") == 0)
		{
			if (png_to_img(image_source, 2, &img) >= 0)
			{
				if ((data_image = resize_image(img.data, img.w, img.h, output_width, output_height, STBIR_RGBA)) != NULL)
				{
					img.w = output_width;
					img.h = output_height;
					free(img.data);

					img.data = data_image;
					data_image = NULL;
				}

				if ((strcasecmp(image_type_dest, ".png") == 0 && stbi_write_png(image_dest, img.w, img.h, 4, img.data, img.h * 4) != 0)
					|| (strcasecmp(image_type_dest, ".jpg") == 0 && stbi_write_jpg(image_dest, img.w, img.h, 4, img.data, 100) != 0))
				{
					copied = true;
				}
				else
				{
					dbglog(DBG_INFO, "copy_image: Error saving PNG file");
				}

				kos_img_free(&img, 0);
			}
		}
		else if (strcasecmp(image_type_source, ".jpg") == 0)
		{
			if (jpeg_to_img(image_source, 1, &img) >= 0)
			{
				if ((data_image = resize_image(img.data, img.w, img.h, output_width, output_height, STBIR_4CHANNEL)) != NULL)
				{
					img.w = output_width;
					img.h = output_height;
					free(img.data);

					img.data = data_image;
					data_image = NULL;
				}

				if ((strcasecmp(image_type_dest, ".png") == 0 && stbi_write_png(image_dest, img.w, img.h, 4, img.data, img.h * 4) != 0)
					|| (strcasecmp(image_type_dest, ".jpg") == 0 && stbi_write_jpg(image_dest, img.w, img.h, 4, img.data, 100) != 0))
				{
					copied = true;
				}
				else
				{
					dbglog(DBG_INFO, "copy_image: Error saving PNG file");
				}

				kos_img_free(&img, 0);
			}			
		}
		else if (strcasecmp(image_type_source, ".pvr") == 0)
		{
			if (is_alpha)
			{
				copied = (pvr_to_png(image_source, image_dest, output_width, output_height, 0) == 1);
			}
			else
			{
				copied = (pvr_to_jpg(image_source, image_dest, output_width, output_height, 100) == 1);
			}

			if (!copied)
			{
				dbglog(DBG_INFO, "copy_image: Error saving file");
			}
		}

		image_type_source = NULL;
		image_type_dest = NULL;
	}

	return copied;
}

bool copy_image_memory_to_file(kos_img_t *image_source, const char *image_dest, bool rewrite, uint width, uint height)
{
	bool copied = false;

	if (image_source != NULL && image_source->data != NULL)
	{
		if (!rewrite && FileExists(image_dest))
		{
			dbglog(DBG_INFO, "copy_image error: image file exists");
		}
		else
		{
			char *image_type_dest = strrchr(image_dest, '.');

			if (strcasecmp(image_type_dest, ".png") == 0)
			{
				if (FileExists(image_type_dest))
				{
					remove(image_type_dest);
				}

				copied = (img_to_png(image_source, image_dest, width, height) == 1);
			}
			else if (strcasecmp(image_type_dest, ".jpg") == 0)
			{
				if (FileExists(image_type_dest))
				{
					remove(image_type_dest);
				}
				copied = (img_to_jpg(image_source, image_dest, width, height, 100) == 1);
			}
			else
			{
				dbglog(DBG_INFO, "copy_image error: unsupported extension");
			}
		}
	}

	return copied;
}
