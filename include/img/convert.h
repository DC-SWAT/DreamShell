#ifndef _IMG_CONVERT_H
#define _IMG_CONVERT_H

/* Read PRV and write PNG*/
int pvr_to_png(const char *source_file, const char *dest_file, uint32 output_width, uint32 output_height, uint8 resolution);
int pvr_to_jpg(const char *source_file, const char *dest_file, uint32 output_width, uint32 output_height, uint8 resolution);
int zoom_jpg_file(const char *inFileName, const char *outFileName, float factor);
unsigned char* resize_image(unsigned char *data, uint width, uint height, uint output_width, uint output_height, uint8 pixel_layout);

#endif // _IMG_CONVERT_H