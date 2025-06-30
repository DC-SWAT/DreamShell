
#ifndef __IMG_UTILS_H
#define __IMG_UTILS_H

#include "img/SegaPVRImage.h"

bool pvr_is_alpha(const char *filename);
bool pvr_header(const char *filename, struct PVRTHeader *pvrtHeader);
void rgb565_to_rgb888(const uint16_t *src, uint8_t *dst, uint32_t width, uint32_t height);
void rgb565_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h);
void argb4444_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h);
void argb1555_to_rgba8888(const uint16_t *src, uint8_t *dst, uint32_t w, uint32_t h);
void argb8888_to_rgba8888(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h);
void rgba8888_to_argb1555(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height);
void rgb888_to_argb1555(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height);
void rgb888_to_rgba8888(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h);
void rgba8888_to_rgb565(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height);
void rgba8888_to_argb4444(const uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height);
void rgba8888_to_argb8888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);

#endif // __IMG_UTILS_H
