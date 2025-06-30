#ifndef __IMG_CONVERT_H
#define __IMG_CONVERT_H

/* Read PVR and write PNG*/
int pvr_to_png(const char *source_file, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t level_compression);

/* Read PVR and write JPG*/
int pvr_to_jpg(const char *source_file, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t resolution);

/* Read PVR and resize*/
unsigned char* resize_image(const kos_img_t *img, uint32_t output_width, uint32_t output_height);

int img_to_pvr(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height);

int img_to_png(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height);

int img_to_jpg(kos_img_t *img, const char *dest_file, uint32_t output_width, uint32_t output_height, uint8_t resolution);

#endif // __IMG_CONVERT_H
