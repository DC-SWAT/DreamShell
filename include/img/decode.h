#ifndef __IMG_DECODE_H
#define __IMG_DECODE_H

int pvr_decode(const char *filename, kos_img_t *kimg, bool kos_format);

int jpg_decode(const char *filename, kos_img_t *kimg, bool kos_format);

int png_decode(const char *filename, kos_img_t *kimg, bool kos_format);

#endif // __IMG_DECODE_H
