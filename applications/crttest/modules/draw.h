/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#ifndef CRT_DRAW_H
#define CRT_DRAW_H

#include <stdint.h>

typedef struct fb {
	uint16_t *pix;
	int w;
	int h;
	int stride;
	int rloss;
	int gloss;
	int bloss;
	int rshift;
	int gshift;
	int bshift;
} fb_t;

int fb_bind(fb_t *fb, uint16_t *pix, int w, int h, int stride,
	int rloss, int gloss, int bloss, int rshift, int gshift, int bshift);
uint16_t fb_rgb(const fb_t *fb, int r, int g, int b);

void fb_clear(fb_t *fb, uint16_t color);
void fb_pixel(fb_t *fb, int x, int y, uint16_t color);
void fb_fill_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color);
void fb_hline(fb_t *fb, int x, int y, int w, uint16_t color);
void fb_vline(fb_t *fb, int x, int y, int h, uint16_t color);
void fb_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color);
void fb_line(fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color);
void fb_circle(fb_t *fb, int cx, int cy, int r, uint16_t color);

int fb_text_width(const char *text, int scale);
void fb_text(fb_t *fb, int x, int y, int scale, uint16_t color, const char *text);
void fb_text_center(fb_t *fb, int y, int scale, uint16_t color, const char *text);

#endif
