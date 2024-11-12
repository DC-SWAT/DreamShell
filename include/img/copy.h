
#ifndef __IMG_COPY_H
#define __IMG_COPY_H

bool copy_image(const char *image_source, const char *image_dest, bool is_alpha, bool rewrite, uint width, uint height, uint output_width, uint output_height, bool yflip);
bool copy_image_memory_to_file(kos_img_t *image_source, const char *image_dest, bool rewrite, uint width, uint height);

#endif // __IMG_COPY_H
