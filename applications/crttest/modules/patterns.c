/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#include "patterns.h"
#include "scroll_art.h"
#include "scroll_clouds.h"

#include <stdio.h>
#include <string.h>

enum {
	PAT_PLUGE = 0,
	PAT_COLOR_BARS,
	PAT_EBU,
	PAT_SMPTE,
	PAT_BARS_GRAY,
	PAT_BLEED,
	PAT_GRAY_RAMP,
	PAT_FLAT,
	PAT_IRE,
	PAT_SHARPNESS,
	PAT_MONOSCOPE,
	PAT_GRID,
	PAT_LINEARITY,
	PAT_OVERSCAN,
	PAT_CONVERGENCE,
	PAT_STRIPES,
	PAT_CHECKER,
	PAT_SCROLL,
	PAT_SHADOW,
	PAT_DIAGONAL,
	PAT_UNIFORM,
	PAT_PHASE,
	PAT_MASK,
	PAT_GRADIENT,
	PAT_BACKSCROLL,
	PAT_COUNT
};

typedef void (*pattern_fn)(fb_t *fb, unsigned frame, int variant, int param);

typedef struct pattern_def {
	const char *name;
	const char *help;
	int variants;
	int param_min;
	int param_max;
	int param_step;
	int param_default;
	pattern_fn draw;
} pattern_def_t;

static uint16_t gray(fb_t *fb, int value) {
	if(value < 0) {
		value = 0;
	}
	if(value > 255) {
		value = 255;
	}
	return fb_rgb(fb, value, value, value);
}

static void vertical_bars(fb_t *fb, int x, int y, int w, int h,
	const uint16_t *colors, int count) {

	int i;
	int x0 = x;

	if(count < 1 || w <= 0 || h <= 0) {
		return;
	}

	for(i = 0; i < count; i++) {
		int x1 = x + (w * (i + 1)) / count;
		fb_fill_rect(fb, x0, y, x1 - x0, h, colors[i]);
		x0 = x1;
	}
}

static void palette7(fb_t *fb, int level, uint16_t out[7]) {
	out[0] = gray(fb, level);
	out[1] = fb_rgb(fb, level, level, 0);
	out[2] = fb_rgb(fb, 0, level, level);
	out[3] = fb_rgb(fb, 0, level, 0);
	out[4] = fb_rgb(fb, level, 0, level);
	out[5] = fb_rgb(fb, level, 0, 0);
	out[6] = fb_rgb(fb, 0, 0, level);
}

static void burst_v(fb_t *fb, int x, int y, int w, int h, int period,
	uint16_t color, uint16_t background) {

	int i;

	if(period < 1) {
		period = 1;
	}
	fb_fill_rect(fb, x, y, w, h, background);
	for(i = 0; i < w; i++) {
		if(((i / period) & 1) == 0) {
			fb_vline(fb, x + i, y, h, color);
		}
	}
}

static void burst_h(fb_t *fb, int x, int y, int w, int h, int period,
	uint16_t color, uint16_t background) {

	int i;

	if(period < 1) {
		period = 1;
	}
	fb_fill_rect(fb, x, y, w, h, background);
	for(i = 0; i < h; i++) {
		if(((i / period) & 1) == 0) {
			fb_hline(fb, x, y + i, w, color);
		}
	}
}

static void checker(fb_t *fb, int x, int y, int w, int h, int cell,
	uint16_t color, uint16_t background) {

	int yy;
	int xx;
	int x1;
	int y1;

	if(w <= 0 || h <= 0) {
		return;
	}
	if(cell < 1) {
		cell = 1;
	}
	if(x < 0) {
		w += x;
		x = 0;
	}
	if(y < 0) {
		h += y;
		y = 0;
	}
	if(w <= 0 || h <= 0 || x >= fb->w || y >= fb->h) {
		return;
	}

	x1 = x + w;
	y1 = y + h;
	if(x1 > fb->w) {
		x1 = fb->w;
	}
	if(y1 > fb->h) {
		y1 = fb->h;
	}

	if(cell == 1) {
		for(yy = y; yy < y1; yy++) {
			uint16_t *row = fb->pix + yy * fb->stride;

			for(xx = x; xx < x1; xx++) {
				row[xx] = (((xx ^ yy) & 1) == 0) ? color : background;
			}
		}
		return;
	}

	fb_fill_rect(fb, x, y, x1 - x, y1 - y, background);
	for(yy = y; yy < y1; yy += cell) {
		for(xx = x; xx < x1; xx += cell) {
			if(((((xx - x) / cell) ^ ((yy - y) / cell)) & 1) == 0) {
				int cw = cell;
				int ch = cell;

				if(xx + cw > x1) {
					cw = x1 - xx;
				}
				if(yy + ch > y1) {
					ch = y1 - yy;
				}
				fb_fill_rect(fb, xx, yy, cw, ch, color);
			}
		}
	}
}

static const int sin90[91] = {
	0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66, 71, 75, 79, 83,
	88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147,
	150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196,
	199, 202, 204, 207, 210, 212, 215, 217, 219, 222, 224, 226, 228, 230, 232,
	234, 236, 237, 239, 241, 242, 243, 245, 246, 247, 248, 249, 250, 251, 252,
	253, 254, 254, 255, 255, 255, 256, 256, 256, 256
};

static int isin_deg(int deg) {
	deg %= 360;
	if(deg < 0) {
		deg += 360;
	}
	if(deg <= 90) {
		return sin90[deg];
	}
	if(deg <= 180) {
		return sin90[180 - deg];
	}
	if(deg <= 270) {
		return -sin90[deg - 180];
	}
	return -sin90[360 - deg];
}

static int icos_deg(int deg) {
	return isin_deg(deg + 90);
}

static void radial_line(fb_t *fb, int cx, int cy, int r0, int r1, int deg, uint16_t color) {
	int s = isin_deg(deg);
	int c = icos_deg(deg);

	fb_line(fb,
		cx + c * r0 / 256, cy + s * r0 / 256,
		cx + c * r1 / 256, cy + s * r1 / 256,
		color);
}

static void wedge(fb_t *fb, int cx, int cy, int r0, int r1, int center,
	int spread, int step, uint16_t color) {

	int angle;

	if(step < 1) {
		step = 1;
	}
	for(angle = center - spread; angle <= center + spread; angle += step) {
		radial_line(fb, cx, cy, r0, r1, angle, color);
	}
}

static int flat_level(int variant) {
	if(variant < 0) {
		return 0;
	}
	if(variant > 7) {
		return 7;
	}
	return variant;
}

static const char *flat_name(int variant) {
	static const char *names[] = {
		"White", "Black", "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta"
	};

	variant = flat_level(variant);
	return names[variant];
}

static int text_scale(const fb_t *fb) {
	return fb->h >= 200 ? 2 : 1;
}

static int text_margin(const fb_t *fb) {
	int margin = fb->h / 10;

	if(margin < 16) {
		margin = 16;
	}
	if(margin > 48) {
		margin = 48;
	}
	return margin;
}

static void draw_pluge(fb_t *fb, unsigned frame, int variant, int param) {
	static const int steps[] = {0, 4, 8, 12, 16, 24, 32, 48, 64};
	uint16_t bars[7];
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int count = (int)(sizeof(steps) / sizeof(steps[0]));
	int y0;
	int y1;
	int col;
	int i;
	int band = 24;
	int label_h = 20;
	int top = text_margin(fb);
	int scale = text_scale(fb);

	(void)frame;
	(void)variant;
	(void)param;

	fb_clear(fb, black);
	if(fb->h < 160) {
		band = fb->h / 8;
		label_h = 12;
		top = 0;
		scale = 1;
	}

	for(i = 0; i < count; i++) {
		int x0 = (fb->w * i) / count;
		int x1 = (fb->w * (i + 1)) / count;
		char label[16];

		fb_fill_rect(fb, x0, top, x1 - x0, band, gray(fb, steps[i]));
		snprintf(label, sizeof(label), "%d", steps[i]);
		fb_text(fb, x0 + 4, top + band + 2, scale, white, label);
	}

	y0 = top + band + label_h + 6;
	y1 = fb->h - 48;
	if(y1 < y0 + 20) {
		y1 = y0 + 20;
	}
	col = fb->w / 3;

	fb_fill_rect(fb, col / 2 - 28, y0, 12, y1 - y0, gray(fb, 8));
	fb_fill_rect(fb, col / 2 - 8, y0, 12, y1 - y0, gray(fb, 16));
	fb_fill_rect(fb, col / 2 + 12, y0, 12, y1 - y0, gray(fb, 32));

	fb_fill_rect(fb, fb->w - col / 2 - 24, y0, 12, y1 - y0, gray(fb, 32));
	fb_fill_rect(fb, fb->w - col / 2 - 4, y0, 12, y1 - y0, gray(fb, 16));
	fb_fill_rect(fb, fb->w - col / 2 + 16, y0, 12, y1 - y0, gray(fb, 8));

	fb_fill_rect(fb, col + 12, y0, col - 24, y1 - y0, gray(fb, 128));
	fb_fill_rect(fb, fb->w / 2 - 36, (y0 + y1) / 2 - 28, 72, 56, white);
	fb_rect(fb, fb->w / 2 - 18, (y0 + y1) / 2 - 12, 36, 24, black);

	palette7(fb, 191, bars);
	vertical_bars(fb, 0, fb->h - 40, fb->w, 40, bars, 7);
}

static void draw_color_bars(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t bars[7];

	(void)frame;
	(void)param;
	palette7(fb, variant ? 255 : 191, bars);
	vertical_bars(fb, 0, 0, fb->w, fb->h, bars, 7);
}

static void draw_ebu(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t top[7];
	uint16_t bottom[7];
	uint16_t black = gray(fb, 0);
	int split = fb->h * 3 / 4;

	(void)frame;
	(void)variant;
	(void)param;

	palette7(fb, 191, top);
	top[0] = gray(fb, 255);
	vertical_bars(fb, 0, 0, fb->w, split, top, 7);

	bottom[0] = fb_rgb(fb, 0, 0, 191);
	bottom[1] = black;
	bottom[2] = fb_rgb(fb, 191, 0, 191);
	bottom[3] = black;
	bottom[4] = fb_rgb(fb, 0, 191, 191);
	bottom[5] = black;
	bottom[6] = gray(fb, 191);
	vertical_bars(fb, 0, split, fb->w, fb->h - split, bottom, 7);
}

static void draw_smpte(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t top[7];
	uint16_t middle[7];
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int y1 = fb->h * 2 / 3;
	int y2 = fb->h * 3 / 4;
	int c0;
	int c3;
	int c6;
	int span;
	int bar;
	int mid;

	(void)frame;
	(void)variant;
	(void)param;

	palette7(fb, 191, top);
	vertical_bars(fb, 0, 0, fb->w, y1, top, 7);

	middle[0] = fb_rgb(fb, 0, 0, 191);
	middle[1] = black;
	middle[2] = fb_rgb(fb, 191, 0, 191);
	middle[3] = black;
	middle[4] = fb_rgb(fb, 0, 191, 191);
	middle[5] = black;
	middle[6] = gray(fb, 191);
	vertical_bars(fb, 0, y1, fb->w, y2 - y1, middle, 7);

	fb_fill_rect(fb, 0, y2, fb->w, fb->h - y2, black);
	c0 = 0;
	c3 = (fb->w * 3) / 7;
	c6 = (fb->w * 6) / 7;
	fb_fill_rect(fb, c0, y2, fb->w / 7, fb->h - y2, fb_rgb(fb, 0, 68, 118));
	fb_fill_rect(fb, fb->w / 7, y2, fb->w / 7, fb->h - y2, white);
	fb_fill_rect(fb, (fb->w * 2) / 7, y2, c3 - (fb->w * 2) / 7, fb->h - y2,
		fb_rgb(fb, 84, 0, 136));
	fb_fill_rect(fb, c6, y2, fb->w - c6, fb->h - y2, white);

	span = c6 - c3;
	bar = span / 9;
	if(bar < 2) {
		bar = 2;
	}
	mid = (c3 + c6) / 2;
	fb_fill_rect(fb, mid - bar * 2, y2, bar, fb->h - y2, gray(fb, 0));
	fb_fill_rect(fb, mid - bar / 2, y2, bar, fb->h - y2, gray(fb, 16));
	fb_fill_rect(fb, mid + bar, y2, bar, fb->h - y2, gray(fb, 32));
}

static void draw_bars_gray(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t bars[7];
	int split = fb->h * 3 / 4;
	int i;

	(void)frame;
	(void)variant;
	(void)param;

	palette7(fb, 191, bars);
	vertical_bars(fb, 0, 0, fb->w, split, bars, 7);
	for(i = 0; i < 16; i++) {
		int x0 = (fb->w * i) / 16;
		int x1 = (fb->w * (i + 1)) / 16;
		fb_fill_rect(fb, x0, split, x1 - x0, fb->h - split, gray(fb, (i * 255) / 15));
	}
}

static void draw_bleed(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t bars[7];
	int width = variant == 0 ? 1 : (variant == 1 ? 2 : 4);
	int group;
	uint16_t black = gray(fb, 0);

	(void)frame;
	(void)param;

	palette7(fb, 255, bars);
	fb_clear(fb, black);
	for(group = 0; group < 7; group++) {
		int x0 = (fb->w * group) / 7;
		int x1 = (fb->w * (group + 1)) / 7;
		int x;

		for(x = x0; x < x1; x++) {
			if((((x - x0) / width) & 1) == 0) {
				fb_vline(fb, x, 0, fb->h, bars[group]);
			}
		}
	}
}

static void draw_gray_ramp(fb_t *fb, unsigned frame, int variant, int param) {
	int i;
	int steps = 11;
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);

	(void)frame;
	(void)param;

	if(variant == 0) {
		int split = fb->h * 4 / 5;

		for(i = 0; i < fb->w; i++) {
			fb_vline(fb, i, 0, split, gray(fb, (i * 255) / (fb->w > 1 ? fb->w - 1 : 1)));
		}
		for(i = 0; i < steps; i++) {
			int x0 = (fb->w * i) / steps;
			int x1 = (fb->w * (i + 1)) / steps;
			int level = (i * 255) / (steps - 1);
			char label[16];

			fb_fill_rect(fb, x0, split, x1 - x0, fb->h - split, gray(fb, level));
			snprintf(label, sizeof(label), "%d", i * 10);
			fb_text(fb, x0 + 4, split + 4, text_scale(fb), level > 128 ? black : white, label);
		}
	}
	else {
		int split = fb->w * 4 / 5;

		for(i = 0; i < fb->h; i++) {
			fb_hline(fb, 0, i, split, gray(fb, (i * 255) / (fb->h > 1 ? fb->h - 1 : 1)));
		}
		for(i = 0; i < steps; i++) {
			int y0 = (fb->h * i) / steps;
			int y1 = (fb->h * (i + 1)) / steps;
			fb_fill_rect(fb, split, y0, fb->w - split, y1 - y0,
				gray(fb, (i * 255) / (steps - 1)));
		}
	}
}

static void draw_flat(fb_t *fb, unsigned frame, int variant, int param) {
	static const int value[8][3] = {
		{255, 255, 255},
		{0, 0, 0},
		{255, 0, 0},
		{0, 255, 0},
		{0, 0, 255},
		{255, 255, 0},
		{0, 255, 255},
		{255, 0, 255}
	};
	int index = flat_level(variant);

	(void)frame;
	(void)param;
	fb_clear(fb, fb_rgb(fb, value[index][0], value[index][1], value[index][2]));
}

static void draw_ire(fb_t *fb, unsigned frame, int variant, int param) {
	int ire = param;
	int level;
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	char label[16];
	int i;
	int scale = text_scale(fb);
	int margin = text_margin(fb);

	(void)frame;
	if(ire < 0) {
		ire = 0;
	}
	if(ire > 100) {
		ire = 100;
	}
	level = (ire * 255) / 100;

	if(variant == 0) {
		int box_w = 8 * scale * 4 + 8;
		int box_h = 8 * scale + 8;
		int y = fb->h - margin - box_h;

		if(y < 0) {
			y = 0;
		}
		fb_clear(fb, gray(fb, level));
		fb_fill_rect(fb, margin, y, box_w, box_h, black);
		snprintf(label, sizeof(label), "%d", ire);
		fb_text(fb, margin + 4, y + 4, scale, white, label);
		return;
	}

	fb_clear(fb, black);
	for(i = 0; i < 11; i++) {
		int x0 = (fb->w * i) / 11;
		int x1 = (fb->w * (i + 1)) / 11;
		int step = i * 10;
		int y = margin;
		int label_y = fb->h - margin - 8 * scale;

		if(label_y < y + 8) {
			label_y = y + 8;
		}
		fb_fill_rect(fb, x0, y, x1 - x0, label_y - y - 4, gray(fb, (step * 255) / 100));
		snprintf(label, sizeof(label), "%d", step);
		fb_text(fb, x0 + 4, label_y, scale, white, label);
		if(step == ire) {
			fb_rect(fb, x0, y, x1 - x0, label_y - y - 4, white);
		}
	}
}

static void draw_sharpness(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int period = param < 1 ? 1 : param;
	int zone;
	int mid = fb->h / 2;

	(void)frame;
	if(period > 4) {
		period = 4;
	}

	fb_clear(fb, black);
	if(variant == 1) {
		burst_v(fb, 0, 0, fb->w, fb->h, period, white, black);
		return;
	}
	if(variant == 2) {
		burst_h(fb, 0, 0, fb->w, fb->h, period, white, black);
		return;
	}

	for(zone = 0; zone < 3; zone++) {
		int x = (fb->w * zone) / 3;
		int w = (fb->w * (zone + 1)) / 3 - x;
		char label[4];

		burst_v(fb, x, 12, w, mid - 28, zone + 1, white, black);
		burst_h(fb, x, mid + 22, w, fb->h - mid - 30, zone + 1, white, black);
		snprintf(label, sizeof(label), "%d", zone + 1);
		fb_text(fb, x + w / 2 - 8, mid - 8, text_scale(fb), white, label);
	}
	fb_fill_rect(fb, 4, mid - 10, 8 * text_scale(fb) * 2 + 16, 8 * text_scale(fb) + 4, black);
	fb_text(fb, 8, mid - 8, text_scale(fb), white, "H");
	fb_text(fb, 8 + 8 * text_scale(fb) + 8, mid - 8, text_scale(fb), white, "V");
	checker(fb, 0, 0, 16, 16, 1, white, black);
	checker(fb, fb->w - 16, 0, 16, 16, 1, white, black);
	checker(fb, 0, fb->h - 16, 16, 16, 1, white, black);
	checker(fb, fb->w - 16, fb->h - 16, 16, 16, 1, white, black);
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_monoscope(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t dim = gray(fb, 180);
	int cx = fb->w / 2;
	int cy = fb->h / 2;
	int radius = fb->w / 2 - 8;
	int gap;
	int corner;
	int i;
	int burst_w;
	int burst_height;
	int y;

	(void)frame;
	(void)variant;
	(void)param;

	if(fb->h / 2 - 28 < radius) {
		radius = fb->h / 2 - 28;
	}
	if(radius < 8) {
		radius = 8;
	}
	fb_clear(fb, black);
	fb_circle(fb, cx, cy, radius, white);
	fb_circle(fb, cx, cy, radius * 3 / 4, dim);
	fb_circle(fb, cx, cy, radius / 2, white);
	fb_circle(fb, cx, cy, radius / 4, dim);

	gap = radius / 12;
	if(gap < 8) {
		gap = 8;
	}
	fb_hline(fb, 1, cy, cx - gap, white);
	fb_hline(fb, cx + gap, cy, fb->w - cx - gap - 1, white);
	fb_vline(fb, cx, 1, cy - gap, white);
	fb_vline(fb, cx, cy + gap, fb->h - cy - gap - 1, white);

	for(i = 0; i < 360; i += 15) {
		radial_line(fb, cx, cy, radius - 10, radius, i, white);
	}
	wedge(fb, cx, cy, radius * 5 / 8, radius * 7 / 8, 270, 24, 4, white);
	wedge(fb, cx, cy, radius * 5 / 8, radius * 7 / 8, 90, 24, 4, white);
	wedge(fb, cx, cy, radius * 5 / 8, radius * 7 / 8, 0, 24, 4, white);
	wedge(fb, cx, cy, radius * 5 / 8, radius * 7 / 8, 180, 24, 4, white);

	corner = radius / 7;
	if(corner < 10) {
		corner = 10;
	}
	fb_circle(fb, corner + 6, corner + 6, corner, white);
	fb_circle(fb, fb->w - corner - 7, corner + 6, corner, white);
	fb_circle(fb, corner + 6, fb->h - corner - 7, corner, white);
	fb_circle(fb, fb->w - corner - 7, fb->h - corner - 7, corner, white);

	burst_w = radius / 5;
	burst_height = 12;
	if(burst_w < 16) {
		burst_w = 16;
	}
	y = cy - (burst_height * 3 + 8) / 2;
	for(i = 0; i < 3; i++) {
		burst_v(fb, cx - burst_w - 8, y, burst_w, burst_height, i + 1, white, black);
		burst_h(fb, cx + 8, y, burst_w, burst_height, i + 1, white, black);
		y += burst_height + 4;
	}
	checker(fb, cx - 8, cy - 8, 16, 16, 1, white, black);

	fb_fill_rect(fb, 8, fb->h - 22, fb->w - 16, 14, black);
	for(i = 0; i < 8; i++) {
		int bw = (fb->w - 16) / 8;
		fb_fill_rect(fb, 8 + i * bw, fb->h - 20, bw - 1, 10, gray(fb, (i * 255) / 7));
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void plus_mark(fb_t *fb, int cx, int cy, int arm, uint16_t color) {
	if(arm < 2) {
		arm = 2;
	}
	fb_hline(fb, cx - arm, cy, arm * 2 + 1, color);
	fb_vline(fb, cx, cy - arm, arm * 2 + 1, color);
}

static void resolution_cluster(fb_t *fb, int x, int y, int size,
	uint16_t white, uint16_t black) {

	int half = size / 2;
	int cell = half > 16 ? 4 : 2;

	if(half < 8) {
		return;
	}
	fb_fill_rect(fb, x, y, size, size, black);
	fb_rect(fb, x + 2, y + 2, half - 4, half - 4, white);
	burst_h(fb, x + half, y, half, half, 2, white, black);
	burst_v(fb, x, y + half, half, half, 2, white, black);
	checker(fb, x + half, y + half, half, half, cell, white, black);
	fb_rect(fb, x, y, size, size, white);
}

static void draw_grid_chart(fb_t *fb) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	int step = fb->w >= 480 ? 16 : 8;
	int cluster = fb->w / 10;
	int cx = fb->w / 2;
	int cy = fb->h / 2;
	int box;
	int arm;
	int x;
	int y;

	if(cluster < 32) {
		cluster = 32;
	}
	if(cluster > 72) {
		cluster = 72;
	}
	if(cluster & 1) {
		cluster--;
	}

	fb_clear(fb, black);
	for(y = step; y < fb->h - step; y += step) {
		for(x = step; x < fb->w - step; x += step) {
			fb_pixel(fb, x, y, white);
		}
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
	fb_rect(fb, 3, 3, fb->w - 6, fb->h - 6, white);

	resolution_cluster(fb, 8, 8, cluster, white, black);
	resolution_cluster(fb, fb->w - 8 - cluster, 8, cluster, white, black);
	resolution_cluster(fb, 8, fb->h - 8 - cluster, cluster, white, black);
	resolution_cluster(fb, fb->w - 8 - cluster, fb->h - 8 - cluster, cluster, white, black);
	resolution_cluster(fb, cx - cluster / 2, 8, cluster, white, black);
	resolution_cluster(fb, cx - cluster / 2, fb->h - 8 - cluster, cluster, white, black);
	resolution_cluster(fb, 8, cy - cluster / 2, cluster, white, black);
	resolution_cluster(fb, fb->w - 8 - cluster, cy - cluster / 2, cluster, white, black);

	x = fb->w / 8;
	y = fb->h / 10;
	fb_rect(fb, x, y, fb->w - x * 2, fb->h - y * 2, red);
	fb_rect(fb, x + 1, y + 1, fb->w - x * 2 - 2, fb->h - y * 2 - 2, red);
	x = fb->w * 3 / 10;
	y = fb->h * 3 / 10;
	fb_rect(fb, x, y, fb->w - x * 2, fb->h - y * 2, red);

	fb_hline(fb, 0, cy, fb->w, white);
	fb_vline(fb, cx, 0, fb->h, white);
	box = (fb->w < fb->h ? fb->w : fb->h) / 5;
	if(box < 48) {
		box = 48;
	}
	fb_rect(fb, cx - box / 2, cy - box / 2, box, box, white);
	arm = box / 12;
	plus_mark(fb, cx, cy, arm, white);
	plus_mark(fb, cx - box / 4, cy - box / 4, arm, white);
	plus_mark(fb, cx + box / 4, cy - box / 4, arm, white);
	plus_mark(fb, cx - box / 4, cy + box / 4, arm, white);
	plus_mark(fb, cx + box / 4, cy + box / 4, arm, white);
}

static void draw_cell_grid(fb_t *fb, int rows) {
	const int cols = 20;
	const int cell = 16;
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	int scale = fb->w / 320;
	int scale_h = fb->h / 240;
	int grid_w;
	int grid_h;
	int origin_x;
	int origin_y;
	int x;
	int y;

	if(scale_h < scale) {
		scale = scale_h;
	}
	if(scale < 1) {
		scale = 1;
	}
	while(scale > 1 && (cols * cell * scale > fb->w || rows * cell * scale > fb->h)) {
		scale--;
	}

	grid_w = cols * cell * scale;
	grid_h = rows * cell * scale;
	origin_x = (fb->w - grid_w) / 2;
	origin_y = (fb->h - grid_h) / 2;
	fb_clear(fb, black);

	for(y = 0; y < rows * cell; y++) {
		int cell_y = y / cell;
		int local_y = y - cell_y * cell;
		int on_row = local_y == 0 || local_y == cell - 1;
		int dot_row = local_y == 7 || local_y == 8;

		for(x = 0; x < cols * cell; x++) {
			int cell_x = x / cell;
			int local_x = x - cell_x * cell;
			int on_col = local_x == 0 || local_x == cell - 1;
			int dot_col = local_x == 7 || local_x == 8;
			uint16_t color;

			if(!on_row && !on_col && !(dot_row && dot_col)) {
				continue;
			}
			if(cell_y == 0 || cell_y == rows - 1 || cell_x == 0 || cell_x == cols - 1) {
				color = red;
			}
			else {
				color = white;
			}
			fb_fill_rect(fb, origin_x + x * scale, origin_y + y * scale, scale, scale, color);
		}
	}
}

static void draw_grid_lines(fb_t *fb, int param) {
	static const int spacing[] = {8, 16, 32, 64};
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int index = param;
	int step;
	int x;
	int y;
	int radius = (fb->w < fb->h ? fb->w : fb->h) / 4;
	int corner;

	if(index < 0) {
		index = 0;
	}
	if(index > 3) {
		index = 3;
	}
	step = spacing[index];

	fb_clear(fb, black);
	for(x = 0; x < fb->w; x += step) {
		fb_vline(fb, x, 0, fb->h, white);
	}
	for(y = 0; y < fb->h; y += step) {
		fb_hline(fb, 0, y, fb->w, white);
	}
	fb_circle(fb, fb->w / 2, fb->h / 2, radius, white);
	fb_hline(fb, 0, fb->h / 2, fb->w, white);
	fb_vline(fb, fb->w / 2, 0, fb->h, white);
	corner = radius / 3;
	if(corner > 8) {
		fb_circle(fb, corner, corner, corner, white);
		fb_circle(fb, fb->w - corner - 1, corner, corner, white);
		fb_circle(fb, corner, fb->h - corner - 1, corner, white);
		fb_circle(fb, fb->w - corner - 1, fb->h - corner - 1, corner, white);
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_grid(fb_t *fb, unsigned frame, int variant, int param) {
	(void)frame;

	if(variant == 1) {
		draw_cell_grid(fb, 15);
	}
	else if(variant == 2) {
		draw_cell_grid(fb, 14);
	}
	else if(variant == 3) {
		draw_grid_lines(fb, param);
	}
	else {
		draw_grid_chart(fb);
	}
}

static void draw_linearity(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int cols = 4 + variant;
	int rows = 3 + variant;
	int cw;
	int ch;
	int row;
	int col;

	(void)frame;
	(void)param;
	if(cols < 2) {
		cols = 2;
	}
	if(rows < 2) {
		rows = 2;
	}

	fb_clear(fb, black);
	cw = fb->w / cols;
	ch = fb->h / rows;
	for(row = 0; row < rows; row++) {
		for(col = 0; col < cols; col++) {
			int x = col * cw;
			int y = row * ch;
			int radius = (cw < ch ? cw : ch) / 2 - 2;

			fb_rect(fb, x, y, cw, ch, white);
			if(radius > 2) {
				fb_circle(fb, x + cw / 2, y + ch / 2, radius, white);
			}
		}
	}
	fb_line(fb, 0, 0, fb->w - 1, fb->h - 1, white);
	fb_line(fb, 0, fb->h - 1, fb->w - 1, 0, white);
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void inset_box(fb_t *fb, int num, int den, uint16_t color, const char *label, int on_right) {
	int x = (fb->w * num) / den;
	int y = (fb->h * num) / den;
	int w = fb->w - x * 2;
	int h = fb->h - y * 2;
	int scale = text_scale(fb);
	int text_w;

	if(w < 4 || h < 4) {
		return;
	}
	fb_rect(fb, x, y, w, h, color);
	text_w = fb_text_width(label, scale);
	if(on_right) {
		fb_text(fb, x + w - text_w - 8, y + 8, scale, color, label);
	}
	else {
		fb_text(fb, x + 8, y + 8, scale, color, label);
	}
}

static void draw_overscan(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t yellow = fb_rgb(fb, 255, 220, 0);
	uint16_t cyan = fb_rgb(fb, 0, 220, 255);
	int x;
	int y;

	(void)frame;
	(void)variant;
	(void)param;

	fb_clear(fb, black);
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
	fb_rect(fb, 1, 1, fb->w - 2, fb->h - 2, white);
	inset_box(fb, 1, 20, yellow, "5%", 0);
	inset_box(fb, 1, 10, cyan, "10%", 1);

	for(x = 0; x < fb->w; x += 10) {
		int tick = (x % 50) == 0 ? 12 : 5;
		char label[16];

		fb_vline(fb, x, 0, tick, white);
		fb_vline(fb, x, fb->h - tick, tick, white);
		if(x >= 100 && (x % 100) == 0 && x < fb->w - 64) {
			snprintf(label, sizeof(label), "%d", x);
			fb_text(fb, x + 2, fb->h - text_margin(fb) - 8 * text_scale(fb),
				text_scale(fb), white, label);
		}
	}
	for(y = 0; y < fb->h; y += 10) {
		int tick = (y % 50) == 0 ? 12 : 5;
		char label[16];

		fb_hline(fb, 0, y, tick, white);
		fb_hline(fb, fb->w - tick, y, tick, white);
		if(y > text_margin(fb) && (y % 50) == 0 && y < fb->h - text_margin(fb)) {
			snprintf(label, sizeof(label), "%d", y);
			fb_text(fb, fb->w / 10 + 4, y - 4, text_scale(fb), white, label);
		}
	}

	fb_hline(fb, fb->w / 2 - 16, fb->h / 2, 32, white);
	fb_vline(fb, fb->w / 2, fb->h / 2 - 12, 24, white);
	fb_text_center(fb, fb->h / 2 + 20, text_scale(fb), white, "CENTER");
}

static void gun_grid(fb_t *fb, int x0, int x1, int step, uint16_t color) {
	int x;
	int y;

	for(x = x0; x < x1; x += step) {
		fb_vline(fb, x, 0, fb->h, color);
	}
	for(y = 0; y < fb->h; y += step) {
		fb_hline(fb, x0, y, x1 - x0, color);
	}
}

static int isqrt_floor(int n) {
	int r = 0;

	if(n <= 0) {
		return 0;
	}
	while((r + 1) * (r + 1) <= n) {
		r++;
	}
	return r;
}

static int conv_part(int param, int shift, int mask, int count) {
	int value;

	if(param < 0 || count < 1) {
		return 0;
	}
	value = (param >> shift) & mask;
	if(value < 0 || value >= count) {
		return 0;
	}
	return value;
}

static void conv_cycle(int *param, int shift, int mask, int count, int dir) {
	int value;
	int bits;

	if(!param || count < 1) {
		return;
	}
	if(*param < 0) {
		*param = 0;
	}
	value = conv_part(*param, shift, mask, count);
	value += dir > 0 ? 1 : -1;
	if(value >= count) {
		value = 0;
	}
	if(value < 0) {
		value = count - 1;
	}
	bits = mask << shift;
	*param = (*param & ~bits) | (value << shift);
}

static int in_disc(int x, int y, int cx, int cy, int r) {
	int dx = x - cx;
	int dy = y - cy;

	return dx * dx + dy * dy <= r * r;
}

static void fill_disc(fb_t *fb, int cx, int cy, int r, uint16_t color) {
	int y;
	int r2;

	if(r < 1) {
		return;
	}
	r2 = r * r;
	for(y = -r; y <= r; y++) {
		int half = isqrt_floor(r2 - y * y);

		fb_hline(fb, cx - half, cy + y, half * 2 + 1, color);
	}
}

static void hatch_disc(fb_t *fb, int cx, int cy, int r, int step, uint16_t color) {
	int y;
	int r2;

	if(r < 2) {
		return;
	}
	if(step < 2) {
		step = 2;
	}
	r2 = r * r;
	for(y = cy - r; y <= cy + r; y++) {
		int dy = y - cy;
		int x;

		for(x = cx - r; x <= cx + r; x++) {
			int dx = x - cx;
			int a;
			int b;

			if(dx * dx + dy * dy > r2) {
				continue;
			}
			a = dx - dy;
			b = dx + dy;
			a %= step;
			b %= step;
			if(a < 0) {
				a += step;
			}
			if(b < 0) {
				b += step;
			}
			if(a == 0 || b == 0) {
				fb_pixel(fb, x, y, color);
			}
		}
	}
}

static void gun_cross(fb_t *fb, int cx, int cy, int arm,
	uint16_t red, uint16_t green, uint16_t blue) {
	if(arm < 4) {
		arm = 4;
	}
	fb_hline(fb, cx - arm, cy, arm * 2 + 1, red);
	fb_vline(fb, cx, cy - arm, arm * 2 + 1, green);
	fb_line(fb, cx - arm, cy - arm, cx + arm, cy + arm, blue);
	fb_line(fb, cx + arm, cy - arm, cx - arm, cy + arm, blue);
}

static int disc_span(int cx, int r, int y, int cy, int *x0, int *w) {
	int dy = y - cy;
	int half;

	if(dy < -r || dy > r) {
		return 0;
	}
	half = isqrt_floor(r * r - dy * dy) - 10;
	if(half < 12) {
		return 0;
	}
	*x0 = cx - half;
	*w = half * 2;
	return 1;
}

static void ramp_row(fb_t *fb, int x, int y, int w, int h, int invert) {
	int steps = 16;
	int i;

	if(w < steps || h < 1) {
		return;
	}
	for(i = 0; i < steps; i++) {
		int x0 = x + (w * i) / steps;
		int x1 = x + (w * (i + 1)) / steps;
		int level = (i * 255) / (steps - 1);

		if(invert) {
			level = 255 - level;
		}
		fb_fill_rect(fb, x0, y, x1 - x0, h, gray(fb, level));
	}
}

static void draw_conv_white(fb_t *fb) {
	uint16_t white = gray(fb, 255);
	int step = 32;
	int x;
	int y;

	fb_clear(fb, gray(fb, 0));
	for(y = step / 2; y < fb->h; y += step) {
		for(x = step / 2; x < fb->w; x += step) {
			fb_hline(fb, x - 5, y, 11, white);
			fb_vline(fb, x, y - 5, 11, white);
		}
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
	fb_hline(fb, 0, fb->h / 2, fb->w, white);
	fb_vline(fb, fb->w / 2, 0, fb->h, white);
}

static void draw_conv_triads(fb_t *fb) {
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	uint16_t green = fb_rgb(fb, 0, 255, 0);
	uint16_t blue = fb_rgb(fb, 0, 0, 255);
	int step = 32;
	int x;
	int y;

	fb_clear(fb, gray(fb, 0));
	for(y = step; y < fb->h; y += step) {
		for(x = step; x < fb->w; x += step) {
			fb_hline(fb, x - 10, y, 6, red);
			fb_vline(fb, x - 7, y - 3, 7, red);
			fb_hline(fb, x - 2, y, 6, green);
			fb_vline(fb, x + 1, y - 3, 7, green);
			fb_hline(fb, x + 6, y, 6, blue);
			fb_vline(fb, x + 9, y - 3, 7, blue);
		}
	}
}

static void draw_conv_guns(fb_t *fb) {
	uint16_t white = gray(fb, 255);
	int third = fb->w / 3;
	int step = 32;

	fb_clear(fb, gray(fb, 0));
	gun_grid(fb, 0, third, step, fb_rgb(fb, 255, 0, 0));
	gun_grid(fb, third, third * 2, step, fb_rgb(fb, 0, 255, 0));
	gun_grid(fb, third * 2, fb->w, step, fb_rgb(fb, 0, 0, 255));
	fb_vline(fb, third, 0, fb->h, white);
	fb_vline(fb, third * 2, 0, fb->h, white);
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_conv_suite(fb_t *fb) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	uint16_t green = fb_rgb(fb, 0, 255, 0);
	uint16_t blue = fb_rgb(fb, 0, 0, 255);
	uint16_t bars[7];
	int cx = fb->w / 2;
	int cy = fb->h / 2;
	int radius = fb->h / 2 - 48;
	int step = fb->w >= 480 ? 40 : 20;
	int cr = 30;
	int corner_x[4];
	int corner_y[4];
	int x;
	int y;
	int i;
	int box_x;
	int box_w;
	static const int ring_cos[12] = {
		256, 222, 128, 0, -128, -222, -256, -222, -128, 0, 128, 222
	};
	static const int ring_sin[12] = {
		0, 128, 222, 256, 222, 128, 0, -128, -222, -256, -222, -128
	};

	if(fb->w / 2 - 48 < radius) {
		radius = fb->w / 2 - 48;
	}
	if(radius < 64) {
		radius = 64;
	}
	if(cr > radius / 5) {
		cr = radius / 5;
	}
	if(cr < 16) {
		cr = 16;
	}
	corner_x[0] = cr + 8;
	corner_y[0] = cr + 8;
	corner_x[1] = fb->w - 1 - (cr + 8);
	corner_y[1] = cr + 8;
	corner_x[2] = cr + 8;
	corner_y[2] = fb->h - 1 - (cr + 8);
	corner_x[3] = fb->w - 1 - (cr + 8);
	corner_y[3] = fb->h - 1 - (cr + 8);

	fb_clear(fb, black);
	for(x = 0; x < fb->w; x += step) {
		fb_vline(fb, x, 0, fb->h, white);
	}
	for(y = 0; y < fb->h; y += step) {
		fb_hline(fb, 0, y, fb->w, white);
	}
	hatch_disc(fb, cx, cy, radius - 2, 12, white);
	fill_disc(fb, cx, cy, 28, black);
	for(i = 0; i < 4; i++) {
		fill_disc(fb, corner_x[i], corner_y[i], cr, black);
	}
	fb_circle(fb, cx, cy, radius, white);
	fb_circle(fb, cx, cy, radius - 1, white);
	fb_circle(fb, cx, cy, 28, white);
	for(i = 0; i < 4; i++) {
		fb_circle(fb, corner_x[i], corner_y[i], cr, white);
		gun_cross(fb, corner_x[i], corner_y[i], cr - 8, red, green, blue);
	}
	gun_cross(fb, cx, cy, 18, red, green, blue);

	if(disc_span(cx, radius, cy - 84, cy, &box_x, &box_w)) {
		ramp_row(fb, box_x, cy - 84, box_w, 12, 0);
	}
	if(disc_span(cx, radius, cy - 68, cy, &box_x, &box_w)) {
		ramp_row(fb, box_x, cy - 68, box_w, 12, 1);
	}
	bars[0] = white;
	bars[1] = fb_rgb(fb, 255, 255, 0);
	bars[2] = fb_rgb(fb, 0, 255, 255);
	bars[3] = green;
	bars[4] = fb_rgb(fb, 255, 0, 255);
	bars[5] = red;
	bars[6] = blue;
	if(disc_span(cx, radius, cy + 40, cy, &box_x, &box_w)) {
		vertical_bars(fb, box_x, cy + 40, box_w, 16, bars, 7);
	}
	if(disc_span(cx, radius, cy + 60, cy, &box_x, &box_w)) {
		ramp_row(fb, box_x + box_w / 8, cy + 60, box_w * 3 / 4, 10, 0);
	}

	for(i = 0; i < 12; i++) {
		int ox = cx + (radius * ring_cos[i]) / 256;
		int oy = cy + (radius * ring_sin[i]) / 256;
		int ix = cx + ((radius - 8) * ring_cos[i]) / 256;
		int iy = cy + ((radius - 8) * ring_sin[i]) / 256;

		fb_line(fb, ix, iy, ox, oy, white);
	}
	for(y = 0; y < fb->h; y += step) {
		for(x = 0; x < fb->w; x += step) {
			if(in_disc(x, y, cx, cy, radius - 4)) {
				continue;
			}
			if(in_disc(x, y, corner_x[0], corner_y[0], cr + 2)
				|| in_disc(x, y, corner_x[1], corner_y[1], cr + 2)
				|| in_disc(x, y, corner_x[2], corner_y[2], cr + 2)
				|| in_disc(x, y, corner_x[3], corner_y[3], cr + 2)) {
				continue;
			}
			plus_mark(fb, x, y, 4, white);
		}
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_conv_rgb_grid(fb_t *fb, int white_bg) {
	uint16_t bg = white_bg ? gray(fb, 255) : gray(fb, 0);
	uint16_t neutral = white_bg ? gray(fb, 0) : gray(fb, 255);
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	uint16_t green = fb_rgb(fb, 0, 255, 0);
	uint16_t blue = fb_rgb(fb, 0, 0, 255);
	int step = 48;
	int x;
	int y;

	fb_clear(fb, bg);
	for(x = 0; x < fb->w; x += step) {
		fb_vline(fb, x, 0, fb->h, neutral);
	}
	for(y = 0; y < fb->h; y += step) {
		fb_hline(fb, 0, y, fb->w, neutral);
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, neutral);
	for(y = 0; y < fb->h; y += step) {
		for(x = 0; x < fb->w; x += step) {
			fb_hline(fb, x - 10, y, 7, red);
			fb_hline(fb, x - 3, y, 7, green);
			fb_hline(fb, x + 4, y, 7, blue);
			fb_vline(fb, x, y - 10, 7, red);
			fb_vline(fb, x, y - 3, 7, green);
			fb_vline(fb, x, y + 4, 7, blue);
		}
	}
}

static void draw_conv_dots(fb_t *fb, int size) {
	static const int steps[] = {16, 32, 80};
	uint16_t white = gray(fb, 255);
	int step;
	int x;
	int y;

	if(size < 0) {
		size = 0;
	}
	if(size > 2) {
		size = 2;
	}
	step = steps[size];
	fb_clear(fb, gray(fb, 0));
	for(x = 0; x < fb->w; x += step) {
		fb_vline(fb, x, 0, fb->h, white);
	}
	for(y = 0; y < fb->h; y += step) {
		fb_hline(fb, 0, y, fb->w, white);
	}
	for(y = 0; y + step <= fb->h; y += step) {
		for(x = 0; x + step <= fb->w; x += step) {
			fb_pixel(fb, x + step / 2, y + step / 2, white);
		}
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_conv_points(fb_t *fb, int size) {
	static const int steps[] = {16, 32, 80};
	uint16_t white = gray(fb, 255);
	int step;
	int x;
	int y;

	if(size < 0) {
		size = 0;
	}
	if(size > 2) {
		size = 2;
	}
	step = steps[size];
	fb_clear(fb, gray(fb, 0));
	for(y = 0; y + step <= fb->h; y += step) {
		for(x = 0; x + step <= fb->w; x += step) {
			fb_pixel(fb, x + step / 2, y + step / 2, white);
		}
	}
}

static void draw_conv_colors(fb_t *fb, int mode) {
	static const int src[4][3] = {
		{255, 0, 0},
		{0, 255, 0},
		{0, 0, 255},
		{255, 255, 255}
	};
	static const int masks[] = {7, 3, 5, 6, 1, 2, 4};
	int mask;
	int cols = 8;
	int rows = 6;
	int row;
	int col;

	if(mode < 0 || mode > 6) {
		mode = 0;
	}
	mask = masks[mode];
	fb_clear(fb, gray(fb, 0));
	for(row = 0; row < rows; row++) {
		for(col = 0; col < cols; col++) {
			int tile = ((row & 1) << 1) | (col & 1);
			int x0 = (fb->w * col) / cols;
			int x1 = (fb->w * (col + 1)) / cols;
			int y0 = (fb->h * row) / rows;
			int y1 = (fb->h * (row + 1)) / rows;
			int r = (mask & 1) ? src[tile][0] : 0;
			int g = (mask & 2) ? src[tile][1] : 0;
			int b = (mask & 4) ? src[tile][2] : 0;

			fb_fill_rect(fb, x0, y0, x1 - x0, y1 - y0, fb_rgb(fb, r, g, b));
		}
	}
}

static void draw_convergence(fb_t *fb, unsigned frame, int variant, int param) {
	(void)frame;

	if(variant == 1) {
		draw_conv_triads(fb);
	}
	else if(variant == 2) {
		draw_conv_guns(fb);
	}
	else if(variant == 3) {
		draw_conv_suite(fb);
	}
	else if(variant == 4) {
		draw_conv_rgb_grid(fb, conv_part(param, 0, 1, 2));
	}
	else if(variant == 5) {
		draw_conv_dots(fb, conv_part(param, 1, 3, 3));
	}
	else if(variant == 6) {
		draw_conv_points(fb, conv_part(param, 1, 3, 3));
	}
	else if(variant == 7) {
		draw_conv_colors(fb, conv_part(param, 3, 7, 7));
	}
	else {
		draw_conv_white(fb);
	}
}

static int stripe_thickness(int param) {
	if(param < 0) {
		return 1;
	}
	if(param == 0) {
		return 1;
	}
	if(param == 1) {
		return 2;
	}
	return 4;
}

static void draw_stripes(fb_t *fb, unsigned frame, int variant, int param) {
	int thickness = stripe_thickness(param);
	int pitch = thickness * 2;
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int i;

	(void)frame;
	fb_clear(fb, black);
	if(variant == 0) {
		for(i = 0; i < fb->h; i += pitch) {
			fb_fill_rect(fb, 0, i, fb->w, thickness, white);
		}
	}
	else {
		for(i = 0; i < fb->w; i += pitch) {
			fb_fill_rect(fb, i, 0, thickness, fb->h, white);
		}
	}
}

static void draw_checker(fb_t *fb, unsigned frame, int variant, int param) {
	int cell = 1;

	(void)frame;
	(void)variant;
	if(param < 0) {
		param = 0;
	}
	if(param > 4) {
		param = 4;
	}
	cell = 1 << param;
	checker(fb, 0, 0, fb->w, fb->h, cell, gray(fb, 255), gray(fb, 0));
}

static void grid_motion(unsigned frame, int param, int *step, int *offset) {
	int speed = param;

	if(speed < 1) {
		speed = 1;
	}
	if(speed > 6) {
		speed = 6;
	}
	*step = 32;
	*offset = (int)((frame * (unsigned)speed) % 32u);
}

static void draw_scroll(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	int step = 32;
	int offset = 0;
	int x;
	int y;

	grid_motion(frame, param, &step, &offset);

	fb_clear(fb, black);
	if(variant != 1) {
		for(x = -step; x < fb->w + step; x += step) {
			fb_vline(fb, x + offset, 0, fb->h, white);
		}
	}
	if(variant != 0) {
		for(y = -step; y < fb->h + step; y += step) {
			fb_hline(fb, 0, y + offset, fb->w, white);
		}
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, red);
	fb_hline(fb, fb->w / 2 - 16, fb->h / 2, 32, red);
	fb_vline(fb, fb->w / 2, fb->h / 2 - 16, 32, red);
}

static int pingpong(unsigned frame, int span) {
	unsigned cycle;
	unsigned pos;

	if(span < 1) {
		return 0;
	}
	cycle = (unsigned)span * 2;
	pos = frame % cycle;
	if(pos < (unsigned)span) {
		return (int)pos;
	}
	return (int)(cycle - pos);
}

static void shadow_background(fb_t *fb, int variant) {
	if(variant == 1) {
		fb_clear(fb, gray(fb, 96));
	}
	else if(variant == 2) {
		checker(fb, 0, 0, fb->w, fb->h, 8, gray(fb, 48), gray(fb, 96));
	}
	else {
		int i;

		fb_clear(fb, gray(fb, 40));
		for(i = 0; i < fb->w; i += 8) {
			fb_fill_rect(fb, i, 0, 4, fb->h, gray(fb, 80));
		}
	}
}

static void shadow_box_geom(unsigned frame, int param, int w, int h,
	int *x, int *y, int *bw, int *bh) {
	int box = w / 8;
	int speed = param < 1 ? 1 : param;
	int span_x;
	int span_y;

	if(box < 28) {
		box = 28;
	}
	if(box > 72) {
		box = 72;
	}
	if(speed > 4) {
		speed = 4;
	}
	span_x = w - box - 28;
	span_y = h - box - 28;
	*bw = box;
	*bh = box * 3 / 4;
	*x = 12 + pingpong(frame * (unsigned)speed, span_x);
	*y = 12 + pingpong(frame * (unsigned)speed / 2 + 20, span_y);
}

static void draw_shadow(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t white = gray(fb, 255);
	uint16_t shadow = gray(fb, 0);
	int x;
	int y;
	int bw;
	int bh;

	shadow_background(fb, variant);
	shadow_box_geom(frame, param, fb->w, fb->h, &x, &y, &bw, &bh);
	fb_fill_rect(fb, x + 10, y + 10, bw, bh, shadow);
	fb_fill_rect(fb, x, y, bw, bh, white);
	fb_rect(fb, x, y, bw, bh, shadow);
}

static void draw_diagonal(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int step = variant == 0 ? 8 : (variant == 1 ? 16 : 4);
	int x;

	(void)frame;
	(void)param;
	fb_clear(fb, black);
	for(x = -fb->h; x < fb->w; x += step) {
		fb_line(fb, x, 0, x + fb->h, fb->h - 1, white);
		fb_line(fb, x, fb->h - 1, x + fb->h, 0, white);
	}
	fb_rect(fb, 0, 0, fb->w, fb->h, white);
}

static void draw_uniform(fb_t *fb, unsigned frame, int variant, int param) {
	int cols = 4;
	int rows = 3;
	int gap_x = fb->w / 24;
	int gap_y = fb->h / 18;
	int cell_w;
	int cell_h;
	int row;
	int col;
	uint16_t black = gray(fb, 0);

	(void)frame;
	(void)param;
	if(gap_x < 4) {
		gap_x = 4;
	}
	if(gap_y < 4) {
		gap_y = 4;
	}
	cell_w = (fb->w - gap_x * (cols + 1)) / cols;
	cell_h = (fb->h - gap_y * (rows + 1)) / rows;
	fb_clear(fb, black);

	for(row = 0; row < rows; row++) {
		for(col = 0; col < cols; col++) {
			int x = gap_x + col * (cell_w + gap_x);
			int y = gap_y + row * (cell_h + gap_y);
			int level = 255;

			if(variant == 1) {
				level = 128;
			}
			else if(variant == 2) {
				level = 40 + ((row * cols + col) * 180) / (rows * cols - 1);
			}
			fb_fill_rect(fb, x, y, cell_w, cell_h, gray(fb, level));
		}
	}
}

static void draw_phase(fb_t *fb, unsigned frame, int variant, int param) {
	uint16_t black = gray(fb, 0);
	uint16_t white = gray(fb, 255);
	int y;
	int zone;

	(void)frame;
	(void)param;

	if(variant == 1) {
		burst_v(fb, 0, 0, fb->w, fb->h, 1, white, black);
		fb_fill_rect(fb, fb->w / 2 - 40, fb->h / 2 - 40, 80, 80, gray(fb, 128));
		return;
	}

	fb_clear(fb, black);
	burst_v(fb, 0, 0, fb->w, fb->h * 2 / 5, 1, white, black);
	for(zone = 0; zone < 3; zone++) {
		int period = 1 << zone;
		int x = (fb->w * zone) / 3;
		int w = (fb->w * (zone + 1)) / 3 - x;
		char label[4];

		y = fb->h * 2 / 5 + 16;
		burst_v(fb, x, y, w, fb->h - y - 4, period, white, black);
		snprintf(label, sizeof(label), "%d", period);
		fb_fill_rect(fb, x, fb->h * 2 / 5, 8 * text_scale(fb) + 8, 8 * text_scale(fb) + 4, black);
		fb_text(fb, x + 4, fb->h * 2 / 5 + 2, text_scale(fb), white, label);
	}
}

static void draw_mask(fb_t *fb, unsigned frame, int variant, int param) {
	int x;
	int y;
	uint16_t red = fb_rgb(fb, 255, 0, 0);
	uint16_t green = fb_rgb(fb, 0, 255, 0);
	uint16_t blue = fb_rgb(fb, 0, 0, 255);
	uint16_t black = gray(fb, 0);

	(void)frame;
	(void)param;

	for(y = 0; y < fb->h; y++) {
		uint16_t *row = fb->pix + y * fb->stride;
		int band = y % 4;
		int triad = y % 3;

		for(x = 0; x < fb->w; x++) {
			uint16_t color;
			int index;

			if(variant == 1) {
				int column = x % 5;

				if(band == 3 || column >= 3) {
					color = black;
				}
				else {
					color = column == 0 ? red : (column == 1 ? green : blue);
				}
			}
			else if(variant == 2) {
				int shift = triad == 1 ? 2 : 0;

				index = (x + shift) % 4;
				if(triad == 2 || index == 3) {
					color = black;
				}
				else {
					color = index == 0 ? red : (index == 1 ? green : blue);
				}
			}
			else {
				index = x % 3;
				color = index == 0 ? red : (index == 1 ? green : blue);
			}
			row[x] = color;
		}
	}
}

static void draw_gradient(fb_t *fb, unsigned frame, int variant, int param) {
	int band_h = fb->h / 3;
	int band_w = fb->w / 3;
	int y;
	int x;

	(void)frame;
	(void)param;

	if(variant == 0) {
		for(y = 0; y < fb->h; y++) {
			uint16_t *row = fb->pix + y * fb->stride;
			int band = y / (band_h > 0 ? band_h : 1);

			if(band > 2) {
				band = 2;
			}
			for(x = 0; x < fb->w; x++) {
				int level = fb->w > 1 ? (x * 255) / (fb->w - 1) : 0;

				if(band == 0) {
					row[x] = fb_rgb(fb, level, 0, 0);
				}
				else if(band == 1) {
					row[x] = fb_rgb(fb, 0, level, 0);
				}
				else {
					row[x] = fb_rgb(fb, 0, 0, level);
				}
			}
		}
		return;
	}

	for(y = 0; y < fb->h; y++) {
		uint16_t *row = fb->pix + y * fb->stride;
		int level = fb->h > 1 ? (y * 255) / (fb->h - 1) : 0;

		for(x = 0; x < fb->w; x++) {
			int band = x / (band_w > 0 ? band_w : 1);

			if(band > 2) {
				band = 2;
			}
			if(band == 0) {
				row[x] = fb_rgb(fb, level, 0, 0);
			}
			else if(band == 1) {
				row[x] = fb_rgb(fb, 0, level, 0);
			}
			else {
				row[x] = fb_rgb(fb, 0, 0, level);
			}
		}
	}
}

static uint16_t scroll_back_pix[SCROLL_BACK_W * SCROLL_BACK_H];
static uint16_t scroll_floor_pix[SCROLL_FLOOR_W * SCROLL_FLOOR_H];
static uint16_t scroll_vert_pix[SCROLL_VERT_W * SCROLL_VERT_H];
static int scroll_built_shift = -1;
static int track_q[5 + SCROLL_CLOUD_COUNT];
static int track_div[5 + SCROLL_CLOUD_COUNT];
static unsigned track_seen;
static int track_ready;

#define TRACK_SKY 0
#define TRACK_MID 1
#define TRACK_NEAR 2
#define TRACK_FLOOR 3
#define TRACK_VERT 4
#define TRACK_CLOUD 5

static int wrap_mod(int value, int span) {
	if(span < 1) {
		return 0;
	}
	value %= span;
	if(value < 0) {
		value += span;
	}
	return value;
}

static int scroll_water(int r, int g, int b) {
	return b > 145 && g > 140 && b + 15 >= r && r > 120;
}

static int scroll_fall_column(int x) {
	if(x >= SCROLL_FALL_X0 && x < SCROLL_FALL_X1) {
		return 1;
	}
	if(x >= SCROLL_FALL2_X0 && x < SCROLL_FALL2_X1) {
		return 1;
	}
	return 0;
}

static uint16_t scroll_pack(const fb_t *fb, int r, int g, int b, int keyed) {
	uint16_t pix;

	if(keyed && r == 255 && g == 0 && b == 255) {
		return 0;
	}
	pix = fb_rgb(fb, r, g, b);
	if(keyed && pix == 0) {
		pix = 1;
	}
	return pix;
}

static void scroll_convert(const fb_t *fb, uint16_t *dst, const unsigned char *rgb,
	int w, int h, int keyed, int shift) {
	int y;

	for(y = 0; y < h; y++) {
		int x;

		for(x = 0; x < w; x++) {
			int sy = y;
			const unsigned char *pixel;

			if(shift && keyed && scroll_fall_column(x) &&
				y >= SCROLL_SHAFT_Y0 && y < SCROLL_SHAFT_Y1) {
				const unsigned char *dest = rgb + (y * w + x) * 3;

				if(scroll_water(dest[0], dest[1], dest[2])) {
					int alt = scroll_shaft_row(y, shift);
					const unsigned char *moved = rgb + (alt * w + x) * 3;

					if(scroll_water(moved[0], moved[1], moved[2])) {
						sy = alt;
					}
				}
			}
			pixel = rgb + (sy * w + x) * 3;
			dst[y * w + x] = scroll_pack(fb, pixel[0], pixel[1], pixel[2], keyed);
		}
	}
}

static int scroll_prepare_assets(const fb_t *fb) {
	int shift;

	if(!fb || !scroll_art_ensure() || !scroll_floor_rgb || !scroll_vert_rgb) {
		return 0;
	}
	shift = fb->rshift | (fb->gshift << 8) | (fb->bshift << 16) | (fb->rloss << 24);
	if(scroll_built_shift == shift) {
		return 1;
	}
	scroll_convert(fb, scroll_floor_pix, scroll_floor_rgb,
		SCROLL_FLOOR_W, SCROLL_FLOOR_H, 1, 0);
	scroll_convert(fb, scroll_vert_pix, scroll_vert_rgb,
		SCROLL_VERT_W, SCROLL_VERT_H, 0, 0);
	scroll_built_shift = shift;
	return 1;
}

static void track_wrap(int index, int span) {
	int span_q = span * 256;
	int value;

	if(span_q < 1) {
		track_q[index] = 0;
		return;
	}
	value = track_q[index] % span_q;
	if(value < 0) {
		value += span_q;
	}
	track_q[index] = value;
}

static int track_px(int index, int span) {
	track_wrap(index, span);
	return track_q[index] / 256;
}

static void track_add(int index, int speed, int dir, int num) {
	int numer;
	int px;

	if(speed < 1) {
		speed = 1;
	}
	if(num < 1) {
		num = 1;
	}
	numer = speed * num;
	if(numer >= SCROLL_RATE_DEN) {
		px = (numer + SCROLL_RATE_DEN / 2) / SCROLL_RATE_DEN;
		if(px < 1) {
			px = 1;
		}
		track_q[index] += dir * px * 256;
		track_div[index] = 0;
		return;
	}
	px = (SCROLL_RATE_DEN + numer / 2) / numer;
	if(px < 1) {
		px = 1;
	}
	track_div[index]++;
	if(track_div[index] >= px) {
		track_div[index] = 0;
		track_q[index] += dir * 256;
	}
}

static int scroll_fall_shift(unsigned frame) {
	int span = SCROLL_SHAFT_Y1 - SCROLL_SHAFT_Y0;
	int period = span * 2;

	if(period < 1) {
		return 0;
	}
	return (int)(frame % (unsigned)period);
}

static void scroll_tick(unsigned frame, int speed, int dir, int vertical) {
	int i;

	if(!track_ready) {
		for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
			track_div[TRACK_CLOUD + i] = i;
		}
		track_ready = 1;
		track_seen = frame;
		return;
	}
	if(frame == track_seen) {
		return;
	}
	track_seen = frame;
	if(vertical) {
		track_add(TRACK_VERT, speed, dir, SCROLL_RATE_VERT);
		return;
	}
	track_add(TRACK_SKY, speed, dir, SCROLL_RATE_SKY);
	track_add(TRACK_MID, speed, dir, SCROLL_RATE_MID);
	track_add(TRACK_NEAR, speed, dir, SCROLL_RATE_NEAR);
	track_add(TRACK_FLOOR, speed, dir, SCROLL_RATE_FLOOR);
	for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
		track_add(TRACK_CLOUD + i, speed, dir, scroll_clouds[i].num);
	}
}

static void scroll_setup(unsigned frame, int variant, int param,
	int *speed_out, int *dir_out, int *vertical_out) {
	int speed = param;
	int dir;
	int vertical;

	if(speed < 1) {
		speed = 1;
	}
	if(speed > 8) {
		speed = 8;
	}
	if(variant < 0) {
		variant = 0;
	}
	if(variant > 3) {
		variant = 3;
	}
	dir = (variant & 1) ? -1 : 1;
	vertical = variant >= 2;
	scroll_tick(frame, speed, dir, vertical);
	if(speed_out) {
		*speed_out = speed;
	}
	if(dir_out) {
		*dir_out = dir;
	}
	if(vertical_out) {
		*vertical_out = vertical;
	}
}

static void blit_scroll(fb_t *fb, const uint16_t *src, int sw, int sh, int scroll, int scale,
	int vertical, int keyed, int band_y) {
	int dest_w = sw * scale;
	int dest_h = sh * scale;
	int x0 = 0;
	int y0 = 0;
	int y;

	if(scale < 1) {
		scale = 1;
		dest_w = sw;
		dest_h = sh;
	}
	if(vertical) {
		x0 = (fb->w - dest_w) / 2;
		scroll = wrap_mod(scroll, sh * scale);
	}
	else {
		int full_h = SCROLL_BACK_H * scale;

		y0 = (fb->h - full_h) / 2 + band_y * scale;
		scroll = wrap_mod(scroll, sw * scale);
	}

	for(y = 0; y < fb->h; y++) {
		uint16_t *drow = fb->pix + y * fb->stride;
		int sy;
		const uint16_t *srow;
		int x;

		if(vertical) {
			sy = (scroll + y) / scale;
			sy %= sh;
		}
		else {
			int ly = y - y0;

			if(ly < 0 || ly >= dest_h) {
				continue;
			}
			sy = ly / scale;
		}
		srow = src + sy * sw;
		for(x = 0; x < fb->w; x++) {
			int sx;
			uint16_t pix;

			if(vertical) {
				int lx = x - x0;

				if(lx < 0 || lx >= dest_w) {
					continue;
				}
				sx = lx / scale;
			}
			else {
				sx = (scroll + x) / scale;
				while(sx >= sw) {
					sx -= sw;
				}
			}
			pix = srow[sx];
			if(keyed && pix == 0) {
				continue;
			}
			drow[x] = pix;
		}
	}
}

static void draw_background(fb_t *fb, unsigned frame, int variant, int param) {
	int vertical = 0;
	int scale;
	int phase;
	int span;

	scroll_setup(frame, variant, param, NULL, NULL, &vertical);
	phase = scroll_fall_shift(frame);
	fb_clear(fb, fb_rgb(fb, 0, 0, 0));
	if(!scroll_prepare_assets(fb) || !scroll_sky_rgb || !scroll_mid_rgb || !scroll_near_rgb) {
		return;
	}
	scale = fb->h / SCROLL_BACK_H;
	if(scale < 1) {
		scale = 1;
	}
	span = SCROLL_BACK_W * scale;
	if(!vertical) {
		scroll_convert(fb, scroll_back_pix, scroll_sky_rgb,
			SCROLL_BACK_W, SCROLL_BACK_H, 0, 0);
		blit_scroll(fb, scroll_back_pix, SCROLL_BACK_W, SCROLL_BACK_H,
			track_px(TRACK_SKY, span), scale, 0, 0, 0);
		scroll_convert(fb, scroll_back_pix, scroll_mid_rgb,
			SCROLL_MID_W, SCROLL_MID_H, 1, 0);
		blit_scroll(fb, scroll_back_pix, SCROLL_MID_W, SCROLL_MID_H,
			track_px(TRACK_MID, span), scale, 0, 1, SCROLL_MID_Y);
		scroll_convert(fb, scroll_back_pix, scroll_near_rgb,
			SCROLL_BACK_W, SCROLL_BACK_H, 1, phase);
		blit_scroll(fb, scroll_back_pix, SCROLL_BACK_W, SCROLL_BACK_H,
			track_px(TRACK_NEAR, span), scale, 0, 1, 0);
		blit_scroll(fb, scroll_floor_pix, SCROLL_FLOOR_W, SCROLL_FLOOR_H,
			track_px(TRACK_FLOOR, span), scale, 0, 1, 0);
	}
	else {
		blit_scroll(fb, scroll_vert_pix, SCROLL_VERT_W, SCROLL_VERT_H,
			track_px(TRACK_VERT, SCROLL_VERT_H * scale), scale, 1, 0, 0);
	}
}

static const pattern_def_t patterns[PAT_COUNT] = {
	{"PLUGE", "Near-black steps. 0 matches the background. 8 and 16 stay visible.", 1, 0, 0, 0, 0, draw_pluge},
	{"Color Bars", "Seven bars. Gas, A, or Up/Down switches 75% and 100%.", 2, 0, 0, 0, 0, draw_color_bars},
	{"EBU Color Bars", "EBU bars. The top is bright and the bottom reverses.", 1, 0, 0, 0, 0, draw_ebu},
	{"SMPTE Color Bars", "SMPTE bars, a reverse row, and PLUGE at 0, 16 and 32.", 1, 0, 0, 0, 0, draw_smpte},
	{"Color Bars with Gray Scale", "75% color bars over a gray scale of 16 steps.", 1, 0, 0, 0, 0, draw_bars_gray},
	{"Color Bleed Check", "Stripes on black. Gas, A, or Up/Down sets the width.", 3, 0, 0, 0, 0, draw_bleed},
	{"Gray Ramp", "Smooth ramp and 10% steps. Gas, A, or Up/Down flips it.", 2, 0, 0, 0, 0, draw_gray_ramp},
	{"White & RGB Screens", "Flat color for purity. Gas, A, or Up/Down changes it.", 8, 0, 0, 0, 0, draw_flat},
	{"100 IRE", "Up/Down or lever sets level. Gas or A opens the steps.", 2, 0, 100, 10, 100, draw_ire},
	{"Sharpness", "Gas or A selects a burst. Up/Down or lever sets pitch.", 3, 1, 4, 1, 1, draw_sharpness},
	{"Monoscope", "Circles and bursts. A round circle means straight geometry.", 1, 0, 0, 0, 0, draw_monoscope},
	{"Grid", "Gas or A selects cells or lines. Up/Down sets spacing.", 4, 0, 3, 1, 1, draw_grid},
	{"Linearity", "Equal circles. Gas, A, or Up/Down adds rows. Egg is bow.", 3, 0, 0, 0, 0, draw_linearity},
	{"Overscan", "Edge marks at 5% and 10%. Missing ticks are off screen.", 1, 0, 0, 0, 0, draw_overscan},
	{"Convergence", "White crosses. A colored fringe on the arms is misconvergence.", 8, 0, 0, 0, 0, draw_convergence},
	{"Horizontal/Vertical Stripes", "Gas or A picks the axis. Up/Down or lever sets thickness.", 2, 0, 2, 1, 0, draw_stripes},
	{"Checkerboard", "Up/Down, lever, gas, or A sets the square, from 1 pixel to 16.", 1, 0, 4, 1, 0, draw_checker},
	{"Grid Scroll", "Gas or A sets the axis. Up/Down or lever sets the speed.", 3, 1, 6, 1, 2, draw_scroll},
	{"Drop Shadow", "Moving box and shadow. Gas, A, or Up/Down changes motion.", 3, 1, 4, 1, 1, draw_shadow},
	{"Diagonal", "Both diagonals. Gas, A, or Up/Down changes the spacing.", 3, 0, 0, 0, 0, draw_diagonal},
	{"Uniformity", "Equal patches. Gas, A, or Up/Down changes the brightness.", 3, 0, 0, 0, 0, draw_uniform},
	{"Pixel Phase", "One-pixel columns. Gas, A, or Up/Down flips them.", 2, 0, 0, 0, 0, draw_phase},
	{"Phosphor Mask", "Columns, slots, or triads. Gas, A, or Up/Down changes mask.", 3, 0, 0, 0, 0, draw_mask},
	{"RGB Gradient", "Red, green, and blue ramps. Gas, A, or Up/Down flips them.", 2, 0, 0, 0, 0, draw_gradient},
	{"Scroll", "Gas or A picks axis and direction. Up/Down or lever sets speed.", 4, 1, 8, 1, 2, draw_background}
};

static int valid_id(int id) {
	if(id < 0 || id >= PAT_COUNT) {
		return 0;
	}
	return id;
}

int pattern_count(void) {
	return PAT_COUNT;
}

const char *pattern_name(int id) {
	return patterns[valid_id(id)].name;
}

const char *pattern_help(int id, int variant, int param) {
	(void)param;
	id = valid_id(id);
	if(variant < 0) {
		variant = 0;
	}

	switch(id) {
		case PAT_COLOR_BARS:
			return variant ? "100% white, yellow, cyan, green, magenta, red, and blue. Gas, A, or Up/Down switches."
				: "75% bars, when 100% bloom. Gas, A, or Up/Down switches.";
		case PAT_BLEED:
			if(variant == 1) {
				return "Two-pixel stripes. The black gap stays neutral. Gas, A, or Up/Down sets width.";
			}
			if(variant == 2) {
				return "Four-pixel stripes. Wider bars show bleed. Gas, A, or Up/Down sets the width.";
			}
			return "One-pixel stripes. Bleed tints the black gap. Gas, A, or Up/Down sets the width.";
		case PAT_GRAY_RAMP:
			return variant ? "Gray rises downward. Steps sit on the right. Gas, A, or Up/Down flips it."
				: "Gray rises to the right. Steps sit on the bottom. Gas, A, or Up/Down flips it.";
		case PAT_FLAT:
			switch(variant) {
				case 1:
					return "Flat black. The field stays black. Gas, A, or Up/Down changes the color.";
				case 2:
					return "Flat red. Only the red gun. Gas, A, or Up/Down changes the color.";
				case 3:
					return "Flat green. Only the green gun. Gas, A, or Up/Down changes the color.";
				case 4:
					return "Flat blue. Only the blue gun. Gas, A, or Up/Down changes the color.";
				case 5:
					return "Flat yellow. Red plus green, blue off. Gas, A, or Up/Down changes the color.";
				case 6:
					return "Flat cyan. Green plus blue, red off. Gas, A, or Up/Down changes the color.";
				case 7:
					return "Flat magenta. Red plus blue, green off. Gas, A, or Up/Down changes the color.";
				default:
					return "Flat white. A stain is a purity error. Gas, A, or Up/Down changes the color.";
			}
		case PAT_IRE:
			return variant ? "Steps from 0 to 100 IRE. Up/Down or lever moves the mark. Gas or A shows one level."
				: "One gray level. Up/Down or lever sets 0 to 100. Gas or A shows the steps.";
		case PAT_SHARPNESS:
			if(variant == 1) {
				return "Vertical lines. Gas or A selects the chart. Up/Down or lever sets the pitch.";
			}
			if(variant == 2) {
				return "Horizontal lines. Gas or A selects the chart. Up/Down or lever sets the pitch.";
			}
			return "Bursts of 1, 2, and 3 pixels. Gas or A selects the chart.";
		case PAT_GRID:
			if(variant == 1) {
				return "Twenty by fifteen cells. Red border. Gas or A selects the chart.";
			}
			if(variant == 2) {
				return "Twenty by fourteen cells, like 224 lines. Gas or A selects the chart.";
			}
			if(variant == 3) {
				return "Line grid and circles. Gas or A selects the chart. Up/Down or lever sets 8 to 64.";
			}
			return "Dots, boxes, and crosses. Gas or A selects the chart.";
		case PAT_LINEARITY:
			if(variant == 1) {
				return "Five by four circles. Gas, A, or Up/Down changes the count.";
			}
			if(variant == 2) {
				return "Six by five circles, the finest set. Gas, A, or Up/Down changes the count.";
			}
			return "Four by three circles. Round means straight. Gas, A, or Up/Down adds rows.";
		case PAT_CONVERGENCE:
			if(variant == 1) {
				return "Red, green, and blue crosses. Line the guns up. Gas or A changes the chart.";
			}
			if(variant == 2) {
				return "One gun on each third. Compare the grids. Gas or A changes the chart.";
			}
			if(variant == 3) {
				return "Grid, circles, ramps, and bars. Fringe means misconvergence. Gas or A changes the chart.";
			}
			if(variant == 4) {
				return "RGB crosses on a grid. Up/Down or lever flips the field. Gas or A changes the chart.";
			}
			if(variant == 5) {
				return "White grid, one-pixel dot. Up/Down or lever sets the size. Gas or A changes the chart.";
			}
			if(variant == 6) {
				return "One-pixel dots. Up/Down or lever sets the spacing. Gas or A changes the chart.";
			}
			if(variant == 7) {
				return "Color squares. Up/Down or lever turns guns off. Gas or A changes the chart.";
			}
			return "White crosses. Fringe means misconvergence. Gas or A changes the chart.";
		case PAT_STRIPES:
			return variant ? "Vertical bars. Gas or A picks the axis. Up/Down or lever sets 1, 2, or 4."
				: "Horizontal bars. Gas or A picks the axis. Up/Down or lever sets 1, 2, or 4.";
		case PAT_SCROLL:
			if(variant == 1) {
				return "Horizontal lines move down. Gas or A sets the axis. Up/Down or lever sets the speed.";
			}
			if(variant == 2) {
				return "Both line sets move. Gas or A sets the axis. Up/Down or lever sets the speed.";
			}
			return "Vertical lines move right. Gas or A sets the axis. Up/Down or lever sets the speed.";
		case PAT_SHADOW:
			if(variant == 1) {
				return "Box over gray. Gas or A changes the field. Up/Down or lever sets the speed.";
			}
			if(variant == 2) {
				return "Box over checkers. Gas or A changes the field. Up/Down or lever sets the speed.";
			}
			return "Box over stripes. Gas or A changes the field. Up/Down or lever sets the speed.";
		case PAT_DIAGONAL:
			if(variant == 1) {
				return "Both diagonals, 16 apart. Gas, A, or Up/Down sets the spacing.";
			}
			if(variant == 2) {
				return "Both diagonals, 4 apart, the finest. Gas, A, or Up/Down sets the spacing.";
			}
			return "Diagonals, 8 apart. A bow bends them. Gas, A, or Up/Down sets spacing.";
		case PAT_UNIFORM:
			if(variant == 1) {
				return "Twelve patches at 50%. Dim corners are an error. Gas, A, or Up/Down changes them.";
			}
			if(variant == 2) {
				return "Twelve gray steps. Each patch should be even. Gas, A, or Up/Down changes them.";
			}
			return "Twelve white patches. They should match. Gas, A, or Up/Down changes brightness.";
		case PAT_PHASE:
			return variant ? "One-pixel columns fill the screen. Gas, A, or Up/Down flips the chart."
				: "Columns at 1, 2, and 4 pixels. Gas, A, or Up/Down flips the chart.";
		case PAT_MASK:
			if(variant == 1) {
				return "Slot mask, triads with a black row between. Gas, A, or Up/Down changes the mask.";
			}
			if(variant == 2) {
				return "Shadow mask. Each row of triads is shifted. Gas, A, or Up/Down changes the mask.";
			}
			return "Red, green, and blue columns. Gas, A, or Up/Down changes the mask.";
		case PAT_GRADIENT:
			return variant ? "Red, green, and blue ramps run top to bottom. Gas, A, or Up/Down flips them."
				: "Red, green, and blue ramps run left to right. Gas, A, or Up/Down flips them.";
		case PAT_BACKSCROLL:
			if(variant == 1) {
				return "The scene moves right. Gas or A picks the direction. Up/Down or lever sets the speed.";
			}
			if(variant == 2) {
				return "The scene moves up. Gas or A picks the direction. Up/Down or lever sets the speed.";
			}
			if(variant == 3) {
				return "The scene moves down. Gas or A picks the direction. Up/Down or lever sets the speed.";
			}
			return "The scene moves left. Gas or A picks the direction. Up/Down or lever sets the speed.";
		default:
			return patterns[id].help;
	}
}

int pattern_variant_count(int id) {
	return patterns[valid_id(id)].variants;
}

int pattern_has_param(int id) {
	return patterns[valid_id(id)].param_step > 0;
}

int pattern_param_default(int id) {
	return patterns[valid_id(id)].param_default;
}

void pattern_cycle_variant(int id, int *variant, int dir) {
	const pattern_def_t *pattern = &patterns[valid_id(id)];
	int value;

	if(!variant || pattern->variants <= 1 || dir == 0) {
		return;
	}
	value = *variant + (dir > 0 ? 1 : -1);
	if(value >= pattern->variants) {
		value = 0;
	}
	if(value < 0) {
		value = pattern->variants - 1;
	}
	*variant = value;
}

void pattern_cycle_param(int id, int *param, int dir) {
	const pattern_def_t *pattern = &patterns[valid_id(id)];
	int value;

	if(!param || pattern->param_step <= 0 || dir == 0) {
		return;
	}
	value = *param + (dir > 0 ? pattern->param_step : -pattern->param_step);
	if(value > pattern->param_max) {
		value = pattern->param_min;
	}
	if(value < pattern->param_min) {
		value = pattern->param_max;
	}
	*param = value;
}

void pattern_status(int id, int variant, int param, char *dst, int dst_len) {
	if(!dst || dst_len < 1) {
		return;
	}
	dst[0] = '\0';
	id = valid_id(id);

	switch(id) {
		case PAT_COLOR_BARS:
			snprintf(dst, dst_len, variant ? "100 percent" : "75 percent");
			break;
		case PAT_BLEED:
			snprintf(dst, dst_len, "stripe %d px", variant == 0 ? 1 : (variant == 1 ? 2 : 4));
			break;
		case PAT_GRAY_RAMP:
			snprintf(dst, dst_len, variant ? "vertical ramp" : "horizontal ramp");
			break;
		case PAT_FLAT:
			snprintf(dst, dst_len, "%s", flat_name(variant));
			break;
		case PAT_IRE:
			snprintf(dst, dst_len, variant ? "staircase, marker %d IRE" : "%d IRE", param);
			break;
		case PAT_SHARPNESS:
			if(variant == 1) {
				snprintf(dst, dst_len, "vertical burst, %d px", param);
			}
			else if(variant == 2) {
				snprintf(dst, dst_len, "horizontal burst, %d px", param);
			}
			else {
				snprintf(dst, dst_len, "line pairs 1, 2, 3");
			}
			break;
		case PAT_GRID: {
			static const int spacing[] = {8, 16, 32, 64};
			int index = param;

			if(variant == 1) {
				snprintf(dst, dst_len, "cells, full raster");
			}
			else if(variant == 2) {
				snprintf(dst, dst_len, "cells, 224 lines");
			}
			else if(variant == 3) {
				if(index < 0) {
					index = 0;
				}
				if(index > 3) {
					index = 3;
				}
				snprintf(dst, dst_len, "lines, spacing %d", spacing[index]);
			}
			else {
				snprintf(dst, dst_len, "chart");
			}
			break;
		}
		case PAT_LINEARITY:
			snprintf(dst, dst_len, "%d x %d circles", 4 + variant, 3 + variant);
			break;
		case PAT_CONVERGENCE:
			if(variant == 1) {
				snprintf(dst, dst_len, "RGB crosses");
			}
			else if(variant == 2) {
				snprintf(dst, dst_len, "separate guns");
			}
			else if(variant == 3) {
				snprintf(dst, dst_len, "full chart");
			}
			else if(variant == 4) {
				snprintf(dst, dst_len, conv_part(param, 0, 1, 2) ? "RGB grid, white" : "RGB grid, black");
			}
			else if(variant == 5) {
				int size = conv_part(param, 1, 3, 3);

				snprintf(dst, dst_len, "dot grid, %s",
					size == 1 ? "medium" : (size == 2 ? "coarse" : "fine"));
			}
			else if(variant == 6) {
				int size = conv_part(param, 1, 3, 3);

				snprintf(dst, dst_len, "dots, %s",
					size == 1 ? "medium" : (size == 2 ? "coarse" : "fine"));
			}
			else if(variant == 7) {
				static const char *guns[] = {
					"all guns", "blue off", "green off", "red off",
					"red only", "green only", "blue only"
				};

				snprintf(dst, dst_len, "%s", guns[conv_part(param, 3, 7, 7)]);
			}
			else {
				snprintf(dst, dst_len, "white crosses");
			}
			break;
		case PAT_STRIPES:
			snprintf(dst, dst_len, "%s, %d px", variant ? "vertical" : "horizontal",
				stripe_thickness(param));
			break;
		case PAT_CHECKER:
			snprintf(dst, dst_len, "cell %d px", 1 << (param < 0 ? 0 : (param > 4 ? 4 : param)));
			break;
		case PAT_SCROLL:
			snprintf(dst, dst_len, "%s, speed %d",
				variant == 0 ? "horizontal" : (variant == 1 ? "vertical" : "both"), param);
			break;
		case PAT_SHADOW:
			snprintf(dst, dst_len, "%s, speed %d",
				variant == 1 ? "gray" : (variant == 2 ? "checker" : "stripes"), param);
			break;
		case PAT_DIAGONAL:
			snprintf(dst, dst_len, "spacing %d", variant == 1 ? 16 : (variant == 2 ? 4 : 8));
			break;
		case PAT_UNIFORM:
			snprintf(dst, dst_len, variant == 1 ? "50 percent" :
				(variant == 2 ? "gray steps" : "white"));
			break;
		case PAT_PHASE:
			snprintf(dst, dst_len, variant ? "full screen, gray reference" : "bursts 1, 2, 4");
			break;
		case PAT_MASK:
			snprintf(dst, dst_len, variant == 1 ? "slot" : (variant == 2 ? "shadow triad" : "aperture columns"));
			break;
		case PAT_GRADIENT:
			snprintf(dst, dst_len, variant ? "vertical bands" : "horizontal bands");
			break;
		case PAT_BACKSCROLL: {
			const char *axis = "horizontal";

			if(variant == 1) {
				axis = "horizontal back";
			}
			else if(variant == 2) {
				axis = "vertical";
			}
			else if(variant == 3) {
				axis = "vertical back";
			}
			snprintf(dst, dst_len, "%s, speed %d", axis, param);
			break;
		}
		default:
			dst[0] = '\0';
			break;
	}
}

int pattern_animates(int id) {
	id = valid_id(id);
	return id == PAT_SCROLL || id == PAT_SHADOW || id == PAT_BACKSCROLL;
}

int pattern_is_grid_scroll(int id) {
	return id == PAT_SCROLL;
}

int pattern_is_shadow(int id) {
	return id == PAT_SHADOW;
}

int pattern_is_scroll(int id) {
	return id == PAT_BACKSCROLL;
}

int pattern_is_convergence(int id) {
	return valid_id(id) == PAT_CONVERGENCE && id == PAT_CONVERGENCE;
}

void pattern_convergence_value(int variant, int *param, int dir) {
	if(variant == 4) {
		conv_cycle(param, 0, 1, 2, dir);
	}
	else if(variant == 5 || variant == 6) {
		conv_cycle(param, 1, 3, 3, dir);
	}
	else if(variant == 7) {
		conv_cycle(param, 3, 7, 7, dir);
	}
}

void pattern_grid_layout(unsigned frame, int param, int *step, int *offset) {
	int local_step = 32;
	int local_offset = 0;

	grid_motion(frame, param, &local_step, &local_offset);
	if(step) {
		*step = local_step;
	}
	if(offset) {
		*offset = local_offset;
	}
}

void pattern_shadow_background(fb_t *fb, int variant) {
	if(!fb) {
		return;
	}
	shadow_background(fb, variant);
}

void pattern_shadow_box(unsigned frame, int param, int w, int h,
	int *x, int *y, int *bw, int *bh) {
	int local_x = 0;
	int local_y = 0;
	int local_w = 1;
	int local_h = 1;

	shadow_box_geom(frame, param, w, h, &local_x, &local_y, &local_w, &local_h);
	if(x) {
		*x = local_x;
	}
	if(y) {
		*y = local_y;
	}
	if(bw) {
		*bw = local_w;
	}
	if(bh) {
		*bh = local_h;
	}
}

void pattern_scroll_pose(fb_t *fb, unsigned frame, int variant, int param, scroll_pose_t *pose) {
	int vertical = 0;
	int scale;
	int phase;
	int span;
	int i;

	if(!pose) {
		return;
	}
	memset(pose, 0, sizeof(*pose));
	if(!fb) {
		return;
	}
	scroll_setup(frame, variant, param, NULL, NULL, &vertical);
	phase = scroll_fall_shift(frame);
	scroll_prepare_assets(fb);
	scale = fb->h / SCROLL_BACK_H;
	if(scale < 1) {
		scale = 1;
	}
	span = SCROLL_BACK_W * scale;
	pose->vertical = vertical;
	pose->scale = scale;
	pose->phase = phase;
	pose->sky_scroll = track_px(TRACK_SKY, span);
	pose->mid_scroll = track_px(TRACK_MID, span);
	pose->near_scroll = track_px(TRACK_NEAR, span);
	pose->floor_scroll = track_px(TRACK_FLOOR, span);
	pose->vert_scroll = track_px(TRACK_VERT, SCROLL_VERT_H * scale);
	pose->cloud_count = SCROLL_CLOUD_COUNT;
	for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
		pose->cloud_scroll[i] = track_px(TRACK_CLOUD + i, span);
	}
	pose->back_src_w = SCROLL_BACK_W;
	pose->back_src_h = SCROLL_BACK_H;
	pose->floor_src_w = SCROLL_FLOOR_W;
	pose->floor_src_h = SCROLL_FLOOR_H;
	pose->vert_src_w = SCROLL_VERT_W;
	pose->vert_src_h = SCROLL_VERT_H;
}

const uint16_t *pattern_scroll_floor(void) {
	return scroll_floor_pix;
}

const uint16_t *pattern_scroll_vert(void) {
	return scroll_vert_pix;
}

void pattern_draw(fb_t *fb, int id, unsigned frame, int variant, int param) {
	id = valid_id(id);
	if(variant < 0) {
		variant = 0;
	}
	if(variant >= patterns[id].variants) {
		variant = patterns[id].variants - 1;
	}
	patterns[id].draw(fb, frame, variant, param);
}
