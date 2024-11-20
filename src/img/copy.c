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
#include "img/decode.h"
#include "img/stb_image.h"

#if defined(__DREAMCAST__)
#include <malloc.h>
#endif


bool copy_image(const char *image_source, const char *image_dest, bool is_alpha, bool rewrite, uint width, uint height, uint output_width, uint output_height, bool yflip)
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
		kos_img_t img;

		if (width > 0 && height > 0)
		{
			if (output_width == 0)
			{
				output_width = width;
			}

			if (output_height == 0)
			{
				output_height = height;
			}

			if (width != output_width || height != output_height)
			{
				has_changes = true;
			}			
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

		if (yflip) 
		{
			stbi_set_flip_vertically_on_load(true);
		}

		if ((strcasecmp(image_type_source, ".png") == 0 && png_decode(image_source, &img) == 0)
			|| (strcasecmp(image_type_source, ".jpg")  == 0 && jpg_decode(image_source, &img) == 0))
		{
			if (!copy_image_memory_to_file(&img, image_dest, true, output_width, output_height))
			{
				dbglog(DBG_INFO, "copy_image: Error saving file");
			}
			kos_img_free(&img, 0);
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
		stbi_set_flip_vertically_on_load(false);

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
			fs_unlink(image_dest);
			
			char *image_type_dest = strrchr(image_dest, '.');

			if (strcasecmp(image_type_dest, ".png") == 0)
			{
				copied = (img_to_png(image_source, image_dest, width, height) == 1);
			}
			else if (strcasecmp(image_type_dest, ".jpg") == 0)
			{	
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
