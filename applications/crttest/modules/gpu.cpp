/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter

   Upload a top-down RGB565 bitmap into a Tsunami canvas.
   Row 0 stays at v = 0, which a banner shows at the top of the screen.
*/

#include "gpu.h"

#include <dc/pvr.h>
#include <dc/sq.h>
#include <plx/texture.h>

#include <cstdint>

extern "C" void LockVideo();
extern "C" void UnlockVideo();

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
	return *texture;
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

extern "C" Texture *crt_canvas_argb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_ARGB1555);
}

extern "C" Texture *crt_canvas_linear_rgb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED);
}

extern "C" Texture *crt_canvas_linear_argb(int w, int h) {
	return make_canvas(w, h, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED);
}

#define CRT_QACR0 (*(volatile uint32_t *)(void *)0xff000038)
#define CRT_QACR1 (*(volatile uint32_t *)(void *)0xff00003c)

static void sq_prepare(void *dest) {
	uint32_t bits = (uint32_t)((((uintptr_t)dest) >> 24) & 0x1c);

	CRT_QACR0 = bits;
	CRT_QACR1 = bits;
}

static void sq_wait_idle(void) {
	volatile uint32_t *queue = (volatile uint32_t *)(void *)MEM_AREA_SQ_BASE;

	queue[0] = 0;
	queue[8] = 0;
}

static void sq_store(void *dest, const uint32_t *src, int bytes, int zero) {
	volatile uint32_t *out;
	int blocks;

	if(dest == 0 || bytes < 32) {
		return;
	}
	if(((uintptr_t)dest & 31) != 0) {
		return;
	}
	bytes &= ~31;
	sq_prepare(dest);
	out = (volatile uint32_t *)SQ_MASK_DEST(dest);
	blocks = bytes >> 5;
	while(blocks > 0) {
		if(zero || src == 0) {
			out[0] = 0;
			out[1] = 0;
			out[2] = 0;
			out[3] = 0;
			out[4] = 0;
			out[5] = 0;
			out[6] = 0;
			out[7] = 0;
		}
		else {
			out[0] = src[0];
			out[1] = src[1];
			out[2] = src[2];
			out[3] = src[3];
			out[4] = src[4];
			out[5] = src[5];
			out[6] = src[6];
			out[7] = src[7];
			src += 8;
		}
		sq_flush((void *)out);
		out += 8;
		blocks--;
	}
	sq_wait_idle();
}

static void vram_copy_slow(uint16_t *dst, const uint16_t *src, int pixels) {
	int i;

	for(i = 0; i < pixels; i++) {
		dst[i] = src ? src[i] : 0;
	}
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

extern "C" void crt_linear_clear(Texture *texture) {
	plx_texture_t *plx;
	int pixels = 0;

	if(!linear_span(texture, 1, &plx, &pixels)) {
		return;
	}
	pixels = plx->w * plx->h;
	LockVideo();
	if(((uintptr_t)plx->ptr & 31) == 0) {
		sq_store(plx->ptr, 0, pixels * (int)sizeof(uint16_t), 1);
	}
	else {
		vram_copy_slow((uint16_t *)plx->ptr, 0, pixels);
	}
	UnlockVideo();
}

extern "C" void crt_linear_upload(Texture *texture, const uint16_t *src, int rows) {
	plx_texture_t *plx;
	int pixels = 0;

	if(src == 0 || !linear_span(texture, rows, &plx, &pixels)) {
		return;
	}
	LockVideo();
	if(((uintptr_t)plx->ptr & 31) == 0 && ((uintptr_t)src & 7) == 0) {
		sq_store(plx->ptr, (const uint32_t *)src, pixels * (int)sizeof(uint16_t), 0);
	}
	else {
		vram_copy_slow((uint16_t *)plx->ptr, src, pixels);
	}
	UnlockVideo();
}

extern "C" void crt_linear_upload_rows(Texture *texture, const uint16_t *src, int row, int rows) {
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
	if(((uintptr_t)dest & 31) == 0 && ((uintptr_t)src & 7) == 0) {
		sq_store(dest, (const uint32_t *)src, pixels * (int)sizeof(uint16_t), 0);
	}
	else {
		vram_copy_slow((uint16_t *)dest, src, pixels);
	}
	UnlockVideo();
}

extern "C" void crt_linear_upload_key(Texture *texture, uint16_t *src, int rows, uint16_t key) {
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

extern "C" void crt_canvas_upload(Texture *texture, const uint16_t *src, int src_w, int src_h) {
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
