#ifndef __IMG_CONVERT_H
#define __IMG_CONVERT_H

/* Read PVR and write PNG*/
int pvr_to_png(const char *source_file, const char *dest_file, uint output_width, uint output_height, uint8 level_compression);

/* Read PVR and write JPG*/
int pvr_to_jpg(const char *source_file, const char *dest_file, uint output_width, uint output_height, uint8 resolution);

/* Read PVR and resize*/
unsigned char* resize_image(unsigned char *data, uint width, uint height, uint output_width, uint output_height, uint8 pixel_layout);

int img_to_png(kos_img_t *img, const char *dest_file, uint output_width, uint output_height);

int img_to_jpg(kos_img_t *img, const char *dest_file, uint output_width, uint output_height, uint8 resolution);

#endif // __IMG_CONVERT_H
