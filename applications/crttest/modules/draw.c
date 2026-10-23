/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#include "draw.h"

#include <string.h>

#include "draw_font.inc"

static int iabs(int value) {
	return value < 0 ? -value : value;
}

int fb_bind(fb_t *fb, uint16_t *pix, int w, int h, int stride,
	int rloss, int gloss, int bloss, int rshift, int gshift, int bshift) {

	if(!fb || !pix || w < 1 || h < 1 || stride < w) {
		return 0;
	}

	fb->pix = pix;
	fb->w = w;
	fb->h = h;
	fb->stride = stride;
	fb->rloss = rloss;
	fb->gloss = gloss;
	fb->bloss = bloss;
	fb->rshift = rshift;
	fb->gshift = gshift;
	fb->bshift = bshift;
	return 1;
}

uint16_t fb_rgb(const fb_t *fb, int r, int g, int b) {
	if(r < 0) {
		r = 0;
	}
	if(g < 0) {
		g = 0;
	}
	if(b < 0) {
		b = 0;
	}
	if(r > 255) {
		r = 255;
	}
	if(g > 255) {
		g = 255;
	}
	if(b > 255) {
		b = 255;
	}

	return (uint16_t)(((r >> fb->rloss) << fb->rshift) |
		((g >> fb->gloss) << fb->gshift) |
		((b >> fb->bloss) << fb->bshift));
}

void fb_clear(fb_t *fb, uint16_t color) {
	int y;
	int x;

	for(y = 0; y < fb->h; y++) {
		uint16_t *row = fb->pix + y * fb->stride;

		for(x = 0; x < fb->stride; x++) {
			row[x] = color;
		}
	}
}

void fb_pixel(fb_t *fb, int x, int y, uint16_t color) {
	if((unsigned)x >= (unsigned)fb->w || (unsigned)y >= (unsigned)fb->h) {
		return;
	}
	fb->pix[y * fb->stride + x] = color;
}

void fb_fill_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color) {
	int x1;
	int y1;
	int yy;
	int xx;

	if(w <= 0 || h <= 0) {
		return;
	}
	if(x < 0) {
		w += x;
		x = 0;
	}
	if(y < 0) {
		h += y;
		y = 0;
	}
	if(w <= 0 || h <= 0) {
		return;
	}

	x1 = x + w;
	y1 = y + h;
	if(x >= fb->w || y >= fb->h) {
		return;
	}
	if(x1 > fb->w) {
		x1 = fb->w;
	}
	if(y1 > fb->h) {
		y1 = fb->h;
	}

	for(yy = y; yy < y1; yy++) {
		uint16_t *row = fb->pix + yy * fb->stride;

		for(xx = x; xx < x1; xx++) {
			row[xx] = color;
		}
	}
}

void fb_hline(fb_t *fb, int x, int y, int w, uint16_t color) {
	fb_fill_rect(fb, x, y, w, 1, color);
}

void fb_vline(fb_t *fb, int x, int y, int h, uint16_t color) {
	fb_fill_rect(fb, x, y, 1, h, color);
}

void fb_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color) {
	if(w <= 0 || h <= 0) {
		return;
	}

	fb_hline(fb, x, y, w, color);
	if(h > 1) {
		fb_hline(fb, x, y + h - 1, w, color);
	}
	if(h > 2) {
		fb_vline(fb, x, y + 1, h - 2, color);
		if(w > 1) {
			fb_vline(fb, x + w - 1, y + 1, h - 2, color);
		}
	}
}

void fb_line(fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color) {
	int dx = iabs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -iabs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	for(;;) {
		int e2;

		fb_pixel(fb, x0, y0, color);
		if(x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err << 1;
		if(e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if(e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void fb_circle(fb_t *fb, int cx, int cy, int radius, uint16_t color) {
	int x;
	int y;
	int err;

	if(radius < 0) {
		return;
	}
	if(radius == 0) {
		fb_pixel(fb, cx, cy, color);
		return;
	}

	x = radius;
	y = 0;
	err = 1 - x;

	while(x >= y) {
		fb_pixel(fb, cx + x, cy + y, color);
		fb_pixel(fb, cx + y, cy + x, color);
		fb_pixel(fb, cx - y, cy + x, color);
		fb_pixel(fb, cx - x, cy + y, color);
		fb_pixel(fb, cx - x, cy - y, color);
		fb_pixel(fb, cx - y, cy - x, color);
		fb_pixel(fb, cx + y, cy - x, color);
		fb_pixel(fb, cx + x, cy - y, color);
		y++;
		if(err < 0) {
			err += 2 * y + 1;
		}
		else {
			x--;
			err += 2 * (y - x) + 1;
		}
	}
}

static void fb_glyph(fb_t *fb, int x, int y, int scale, uint16_t color, int ch) {
	const uint8_t *glyph;
	int row;
	int col;

	if(ch < 32 || ch > 126) {
		ch = '?';
	}
	glyph = font8x8[ch - 32];

	for(row = 0; row < 8; row++) {
		uint8_t bits = glyph[row];

		for(col = 0; col < 8; col++) {
			if(bits & (uint8_t)(0x80 >> col)) {
				if(scale <= 1) {
					fb_pixel(fb, x + col, y + row, color);
				}
				else {
					fb_fill_rect(fb, x + col * scale, y + row * scale, scale, scale, color);
				}
			}
		}
	}
}

int fb_text_width(const char *text, int scale) {
	if(!text) {
		return 0;
	}
	if(scale < 1) {
		scale = 1;
	}
	return (int)strlen(text) * 8 * scale;
}

void fb_text(fb_t *fb, int x, int y, int scale, uint16_t color, const char *text) {
	if(!text) {
		return;
	}
	if(scale < 1) {
		scale = 1;
	}

	while(*text) {
		if(x >= fb->w) {
			break;
		}
		fb_glyph(fb, x, y, scale, color, (unsigned char)*text);
		x += 8 * scale;
		text++;
	}
}

void fb_text_center(fb_t *fb, int y, int scale, uint16_t color, const char *text) {
	int x = (fb->w - fb_text_width(text, scale)) / 2;

	if(x < 0) {
		x = 0;
	}
	fb_text(fb, x, y, scale, color, text);
}

