#ifndef __IMG_DECODE_H
#define __IMG_DECODE_H

int pvr_decode(const char *filename, kos_img_t *kimg);

int jpg_decode(const char *filename, kos_img_t *kimg);

int png_decode(const char *filename, kos_img_t *kimg);

#endif // __IMG_DECODE_H
