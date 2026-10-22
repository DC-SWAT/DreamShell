/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#ifndef CRT_GPU_H
#define CRT_GPU_H

#include <tsunami/texture.h>

#ifdef __cplusplus
extern "C" {
#endif

Texture *crt_canvas_argb(int w, int h);
Texture *crt_canvas_linear_rgb(int w, int h);
Texture *crt_canvas_linear_argb(int w, int h);
void crt_canvas_upload(Texture *texture, const uint16_t *src, int src_w, int src_h);
void crt_linear_clear(Texture *texture);
void crt_linear_upload(Texture *texture, const uint16_t *src, int rows);
void crt_linear_upload_rows(Texture *texture, const uint16_t *src, int row, int rows);
void crt_linear_upload_key(Texture *texture, uint16_t *src, int rows, uint16_t key);

#ifdef __cplusplus
}
#endif

#endif
