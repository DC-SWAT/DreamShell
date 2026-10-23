/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter

   Upload a top-down RGB565 bitmap into a Tsunami canvas.
   Row 0 stays at v = 0, which a banner shows at the top of the screen.
*/

#include "gpu.h"

#include <dc/pvr.h>
#include <plx/texture.h>
#include <kos/string.h>
#include <stdint.h>
#include <video.h>

static int twiddle_small(int value) {
	return (value & 1)
		| ((value & 2) << 1)
		| ((value & 4) << 2)
		| ((value & 8) << 3)
		| ((value & 16) << 4)
		| ((value & 32) << 5)
		| ((value & 64) << 6)
		| ((value & 128) << 7)
		| ((value & 256) << 8)
		| ((value & 512) << 9);
}

static int twiddle_index(int x, int y, int min_side) {
	int mask = min_side - 1;

	return (twiddle_small(x & mask) << 1 | twiddle_small(y & mask))
		+ (x / min_side + y / min_side) * min_side * min_side;
}

static uint16_t to_argb1555(uint16_t pix) {
	if(pix == 0) {
		return 0;
	}
	return (uint16_t)(0x8000
		| ((pix & 0xF800) >> 1)
		| ((pix & 0x07C0) >> 1)
		| (pix & 0x001F));
}

static plx_texture_t *plx_of(Texture *texture) {
	if(texture == 0) {
		return 0;
	}
	/* C++ Texture is a vtable pointer, then the plx canvas. */
	return *(plx_texture_t **)((unsigned char *)texture + sizeof(void *));
}

static Texture *make_canvas(int w, int h, int fmt) {
	Texture *texture = TSU_TextureCreate(w, h, fmt);
	plx_texture_t *plx;

	if(texture == 0) {
		return 0;
	}
	plx = plx_of(texture);
	if(plx == 0 || plx->ptr == 0) {
		TSU_TextureDestroy(&texture);
		return 0;
	}
	plx_txr_setfilter(plx, PLX_FILTER_NONE);
	plx_txr_setuvclamp(plx, PLX_UV_CLAMP, PLX_UV_CLAMP);
	return texture;
}

Texture *crt_canvas_argb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_ARGB1555);
}

Texture *crt_canvas_linear_rgb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED);
}

Texture *crt_canvas_linear_argb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED);
}

static void vram_copy(void *dst, const void *src, int bytes, int clear) {
	if(dst == 0 || bytes < 4 || (bytes & 3) != 0 || ((uintptr_t)dst & 3) != 0) {
		return;
	}
	if(clear || src == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		memset4(dst, 0, (size_t)bytes);
#pragma GCC diagnostic pop
		return;
	}
	if(((uintptr_t)src & 3) != 0) {
		return;
	}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	memcpy4(dst, src, (size_t)bytes);
#pragma GCC diagnostic pop
}

static int linear_span(Texture *texture, int rows, plx_texture_t **plx_out, int *pixels) {
	plx_texture_t *plx;

	if(texture == 0 || rows < 1) {
		return 0;
	}
	plx = plx_of(texture);
	if(plx == 0 || plx->ptr == 0 || plx->w < 1 || plx->h < 1) {
		return 0;
	}
	if(rows > plx->h) {
		rows = plx->h;
	}
	*plx_out = plx;
	*pixels = rows * plx->w;
	return (*pixels > 0) ? 1 : 0;
}

void crt_linear_clear(Texture *texture) {
	plx_texture_t *plx;
	int pixels = 0;

	if(!linear_span(texture, 1, &plx, &pixels)) {
		return;
	}
	pixels = plx->w * plx->h;
	LockVideo();
	vram_copy(plx->ptr, 0, pixels * (int)sizeof(uint16_t), 1);
	UnlockVideo();
}

void crt_linear_upload(Texture *texture, const uint16_t *src, int rows) {
	plx_texture_t *plx;
	int pixels = 0;

	if(src == 0 || !linear_span(texture, rows, &plx, &pixels)) {
		return;
	}
	LockVideo();
	vram_copy(plx->ptr, src, pixels * (int)sizeof(uint16_t), 0);
	UnlockVideo();
}

void crt_linear_upload_rows(Texture *texture, const uint16_t *src, int row, int rows) {
	plx_texture_t *plx;
	uint8_t *dest;
	int pixels = 0;

	if(src == 0 || row < 0 || !linear_span(texture, rows, &plx, &pixels)) {
		return;
	}
	if(row + rows > plx->h) {
		return;
	}
	dest = (uint8_t *)plx->ptr + (size_t)row * (size_t)plx->w * sizeof(uint16_t);
	LockVideo();
	vram_copy(dest, src, pixels * (int)sizeof(uint16_t), 0);
	UnlockVideo();
}

void crt_linear_upload_key(Texture *texture, uint16_t *src, int rows, uint16_t key) {
	plx_texture_t *plx;
	int pixels = 0;
	int i;

	if(src == 0 || !linear_span(texture, rows, &plx, &pixels)) {
		return;
	}
	for(i = 0; i < pixels; i++) {
		uint16_t pix = src[i];

		if(pix == key) {
			src[i] = 0;
		}
		else if(pix == 0) {
			src[i] = 0x8000;
		}
		else {
			src[i] = to_argb1555(pix);
		}
	}
	crt_linear_upload(texture, src, rows);
}

void crt_canvas_upload(Texture *texture, const uint16_t *src, int src_w, int src_h) {
	plx_texture_t *plx;
	uint16_t *dst;
	int tw;
	int th;
	int min_side;
	int argb;
	int x;
	int y;

	if(texture == 0 || src == 0 || src_w < 1 || src_h < 1) {
		return;
	}
	plx = plx_of(texture);
	if(plx == 0 || plx->ptr == 0) {
		return;
	}
	tw = plx->w;
	th = plx->h;
	if(tw < 1 || th < 1) {
		return;
	}
	min_side = tw < th ? tw : th;
	argb = (plx->fmt & PVR_TXRFMT_RGB565) == 0;
	dst = (uint16_t *)plx->ptr;

	LockVideo();
	for(y = 0; y < th; y++) {
		for(x = 0; x < tw; x++) {
			uint16_t pix = 0;
			int index = twiddle_index(x, y, min_side);

			if(x < src_w && y < src_h) {
				pix = src[y * src_w + x];
				if(argb) {
					pix = to_argb1555(pix);
				}
			}
			dst[index] = pix;
		}
	}
	UnlockVideo();
}
