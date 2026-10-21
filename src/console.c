/****************************
 * DreamShell ##version##   *
 * console.c                *
 * DreamShell console       *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/


#include "ds.h"
#include "video.h"
#include "console.h"
#include "SDL/SDL_dreamcast.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#define CONSOLE_FONT_SIZE 16.0f
#define CONSOLE_FONT_UNIT_MIN 0.01f
#define CONSOLE_WIDTH_RANK_NUM 9
#define CONSOLE_WIDTH_RANK_DEN 10
#define CONSOLE_WIDTH_MONO 1.2f
#define CONSOLE_SLANT 0.18f
#define CONSOLE_LINE_LEAD 2.0f
#define CONSOLE_BORDER 8.0f
#define CONSOLE_BLINK_MS 1000
#define CONSOLE_CURSOR_ALPHA 1.0f
#define CONSOLE_CURSOR_PI 3.14159265f
#define CONSOLE_SLIDE_MS 220
#define CONSOLE_PAGE_LINES 2
#define CONSOLE_TRIG_AXIS_RIGHT 2
#define CONSOLE_TRIG_AXIS_LEFT 3
#define CONSOLE_TRIG_THRESHOLD 192
#define CONSOLE_TRIG_REPEAT_MS 50
#define CONSOLE_TRIG_STEP 1
#define CONSOLE_TAB_WIDTH 8
#define CONSOLE_HINT_COLUMNS 4
#define CONSOLE_HINT_GAP 2
#define CONSOLE_HINT_LINE 160
#define CONSOLE_MAX_COLS 160
#define CONSOLE_CMD_MAX 256
#define CONSOLE_SCROLL_EVERY 5
#define CONSOLE_PROMPT_MAX 64
#define CONSOLE_SGR_PARAMS 16
#define CONSOLE_CSI_MAX 64
#define CONSOLE_Z_BG 450.0f
#define CONSOLE_Z_IMAGE 451.0f
#define CONSOLE_GLASS_R 8
#define CONSOLE_GLASS_G 10
#define CONSOLE_GLASS_B 14
#define CONSOLE_GLASS_ALPHA 0.9f
#define CONSOLE_Z_CELL 452.0f
#define CONSOLE_Z_TEXT 453.0f
#define CONSOLE_Z_DECOR 454.0f
#define CONSOLE_UNDERLINE_PX 1.0f
#define CONSOLE_FG_R 220
#define CONSOLE_FG_G 220
#define CONSOLE_FG_B 220
#define CONSOLE_PROMPT_R 90
#define CONSOLE_PROMPT_G 220
#define CONSOLE_PROMPT_B 220
#define CONSOLE_TAG_ERROR "DS_ERROR"
#define CONSOLE_TAG_OK "DS_OK"
#define CONSOLE_TAG_INFO "DS_INFO"
#define CONSOLE_TAG_PROCESS "DS_PROCESS"
#define CONSOLE_TAG_WARNING "DS_WARNING"
#define CONSOLE_TAG_ERROR_R 255
#define CONSOLE_TAG_ERROR_G 80
#define CONSOLE_TAG_ERROR_B 80
#define CONSOLE_TAG_OK_R 90
#define CONSOLE_TAG_OK_G 220
#define CONSOLE_TAG_OK_B 110
#define CONSOLE_TAG_INFO_R 80
#define CONSOLE_TAG_INFO_G 200
#define CONSOLE_TAG_INFO_B 255
#define CONSOLE_TAG_PROCESS_R 220
#define CONSOLE_TAG_PROCESS_G 90
#define CONSOLE_TAG_PROCESS_B 230
#define CONSOLE_TAG_WARNING_R 255
#define CONSOLE_TAG_WARNING_G 150
#define CONSOLE_TAG_WARNING_B 40
#define CONSOLE_DIM_NUM 2
#define CONSOLE_DIM_DEN 3
#define CONSOLE_BOLD_MIX 3

#define CONSOLE_COLOR_DEFAULT 0
#define CONSOLE_COLOR_INDEX 1
#define CONSOLE_COLOR_RGB 2
#define CONSOLE_EL_TO_END 0
#define CONSOLE_EL_TO_CURSOR 1
#define CONSOLE_EL_ALL 2

#define CONSOLE_ATTR_ITALIC 0x01
#define CONSOLE_ATTR_UNDERLINE 0x02
#define CONSOLE_ATTR_REVERSE 0x04

#define CONSOLE_ANSI_COUNT 16
#define CONSOLE_ANSI_BRIGHT 8
#define CONSOLE_GRAY_START 232
#define CONSOLE_CUBE_START 16
#define CONSOLE_CUBE_STEP 6

typedef struct {
	unsigned char ch;
	unsigned char attr;
	unsigned char fg_r, fg_g, fg_b;
	unsigned char bg_r, bg_g, bg_b;
	unsigned char bg_on;
} ConCell;

typedef struct {
	int bold;
	int dim;
	int italic;
	int underline;
	int reverse;
	int fg_mode;
	int bg_mode;
	int fg_idx;
	int bg_idx;
	unsigned char fg_r, fg_g, fg_b;
	unsigned char bg_r, bg_g, bg_b;
} ConStyle;

typedef struct {
	int visible;
	int x, y, w, h;
	int alpha;
	int vchars;
	int vis_rows;
	int line_count;
	int scroll;
	int col;
	float char_w;
	float line_h;
	float ascent;
	float slant;
	ConStyle style;
	ConCell **lines;
	char **commands;
	int command_count;
	int command_scroll;
	char prompt[CONSOLE_PROMPT_MAX];
	char command[CONSOLE_CMD_MAX + 1];
	char lcmd[CONSOLE_CMD_MAX + 1];
	char rcmd[CONSOLE_CMD_MAX + 1];
	int cursor;
	int offset;
	int insert;
	int back_x;
	int back_y;
	plx_font_t *font;
	plx_fcxt_t *fcxt;
	plx_texture_t *background;
} ConsoleState;

static ConsoleState *console = NULL;
static Event_t *con_input_event = NULL;
static Event_t *con_video_event = NULL;
static int console_debug = 0;

static mutex_t console_mutex = RECURSIVE_MUTEX_INITIALIZER;

static const unsigned char ansi_palette[CONSOLE_ANSI_COUNT][3] = {
	{0, 0, 0},
	{170, 0, 0},
	{0, 170, 0},
	{170, 85, 0},
	{0, 0, 170},
	{170, 0, 170},
	{0, 170, 170},
	{170, 170, 170},
	{85, 85, 85},
	{255, 85, 85},
	{85, 255, 85},
	{255, 255, 85},
	{85, 85, 255},
	{255, 85, 255},
	{85, 255, 255},
	{255, 255, 255}
};

static const unsigned char ansi_cube[CONSOLE_CUBE_STEP] = {
	0, 95, 135, 175, 215, 255
};

static int flat_open = 0;
static int cursor_draw = 0;
static float cursor_alpha = 0.0f;
static float cursor_x = 0.0f;
static float cursor_y = 0.0f;
static float slide_pos = 0.0f;
static float slide_from = 0.0f;
static int slide_open = 0;
static int slide_run = 0;
static uint64 slide_start = 0;
static uint32 slide_span = 1;
static int console_mouse_was = 1;
static int feed_pending_nl = 0;
static int feed_tag_on = 0;
static ConStyle feed_tag_saved;
static uint64 trig_left_next = 0;
static uint64 trig_right_next = 0;


static void style_reset(ConStyle *st) {
	memset(st, 0, sizeof(*st));
}

static void color_from_index(int idx, unsigned char *r, unsigned char *g, unsigned char *b) {
	int n;

	if(idx < 0) {
		idx = 0;
	}
	if(idx > 255) {
		idx = 255;
	}

	if(idx < CONSOLE_ANSI_COUNT) {
		*r = ansi_palette[idx][0];
		*g = ansi_palette[idx][1];
		*b = ansi_palette[idx][2];
		return;
	}

	if(idx >= CONSOLE_GRAY_START) {
		unsigned char v = (unsigned char)(8 + (idx - CONSOLE_GRAY_START) * 10);
		*r = v;
		*g = v;
		*b = v;
		return;
	}

	n = idx - CONSOLE_CUBE_START;
	*r = ansi_cube[n / 36];
	*g = ansi_cube[(n / CONSOLE_CUBE_STEP) % CONSOLE_CUBE_STEP];
	*b = ansi_cube[n % CONSOLE_CUBE_STEP];
}

static void lighten(unsigned char *c) {
	*c = (unsigned char)(*c + (255 - *c) / CONSOLE_BOLD_MIX);
}

static void style_fg(const ConStyle *st, unsigned char *r, unsigned char *g, unsigned char *b) {
	if(st->fg_mode == CONSOLE_COLOR_INDEX) {
		int idx = st->fg_idx;

		if(st->bold && idx < CONSOLE_ANSI_BRIGHT) {
			idx += CONSOLE_ANSI_BRIGHT;
		}
		color_from_index(idx, r, g, b);
		if(st->bold && st->fg_idx >= CONSOLE_ANSI_COUNT) {
			lighten(r);
			lighten(g);
			lighten(b);
		}
	}
	else if(st->fg_mode == CONSOLE_COLOR_RGB) {
		*r = st->fg_r;
		*g = st->fg_g;
		*b = st->fg_b;
		if(st->bold) {
			lighten(r);
			lighten(g);
			lighten(b);
		}
	}
	else {
		*r = CONSOLE_FG_R;
		*g = CONSOLE_FG_G;
		*b = CONSOLE_FG_B;
		if(st->bold) {
			*r = 255;
			*g = 255;
			*b = 255;
		}
	}

	if(st->dim) {
		*r = (unsigned char)(*r * CONSOLE_DIM_NUM / CONSOLE_DIM_DEN);
		*g = (unsigned char)(*g * CONSOLE_DIM_NUM / CONSOLE_DIM_DEN);
		*b = (unsigned char)(*b * CONSOLE_DIM_NUM / CONSOLE_DIM_DEN);
	}
}

static void style_bg(const ConStyle *st, unsigned char *r, unsigned char *g, unsigned char *b) {
	if(st->bg_mode == CONSOLE_COLOR_INDEX) {
		color_from_index(st->bg_idx, r, g, b);
	}
	else if(st->bg_mode == CONSOLE_COLOR_RGB) {
		*r = st->bg_r;
		*g = st->bg_g;
		*b = st->bg_b;
			}
			else {
		*r = 0;
		*g = 0;
		*b = 0;
	}
}

static void style_set_index(ConStyle *st, int bg, int idx) {
	if(idx < 0) {
		idx = 0;
	}
	if(idx > 255) {
		idx = 255;
	}

	if(bg) {
		st->bg_mode = CONSOLE_COLOR_INDEX;
		st->bg_idx = idx;
	}
	else {
		st->fg_mode = CONSOLE_COLOR_INDEX;
		st->fg_idx = idx;
	}
}

static void style_set_rgb(ConStyle *st, int bg, int r, int g, int b) {
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

	if(bg) {
		st->bg_mode = CONSOLE_COLOR_RGB;
		st->bg_r = (unsigned char)r;
		st->bg_g = (unsigned char)g;
		st->bg_b = (unsigned char)b;
			}
			else {
		st->fg_mode = CONSOLE_COLOR_RGB;
		st->fg_r = (unsigned char)r;
		st->fg_g = (unsigned char)g;
		st->fg_b = (unsigned char)b;
	}
}

static void style_apply(ConStyle *st, const int *params, int count) {
	int i;

	if(count <= 0) {
		style_reset(st);
		return;
	}

	for(i = 0; i < count; i++) {
		int p = params[i];

		if(p == 0) {
			style_reset(st);
		}
		else if(p == 1) {
			st->bold = 1;
		}
		else if(p == 2) {
			st->dim = 1;
		}
		else if(p == 3) {
			st->italic = 1;
		}
		else if(p == 4) {
			st->underline = 1;
		}
		else if(p == 7) {
			st->reverse = 1;
		}
		else if(p == 21 || p == 22) {
			st->bold = 0;
			st->dim = 0;
		}
		else if(p == 23) {
			st->italic = 0;
		}
		else if(p == 24) {
			st->underline = 0;
		}
		else if(p == 27) {
			st->reverse = 0;
		}
		else if(p == 39) {
			st->fg_mode = CONSOLE_COLOR_DEFAULT;
		}
		else if(p == 49) {
			st->bg_mode = CONSOLE_COLOR_DEFAULT;
		}
		else if(p >= 30 && p <= 37) {
			style_set_index(st, 0, p - 30);
		}
		else if(p >= 40 && p <= 47) {
			style_set_index(st, 1, p - 40);
		}
		else if(p >= 90 && p <= 97) {
			style_set_index(st, 0, p - 90 + CONSOLE_ANSI_BRIGHT);
		}
		else if(p >= 100 && p <= 107) {
			style_set_index(st, 1, p - 100 + CONSOLE_ANSI_BRIGHT);
		}
		else if(p == 38 || p == 48) {
			int bg = (p == 48);

			if(i + 2 < count && params[i + 1] == 5) {
				style_set_index(st, bg, params[i + 2]);
				i += 2;
			}
			else if(i + 4 < count && params[i + 1] == 2) {
				style_set_rgb(st, bg, params[i + 2], params[i + 3], params[i + 4]);
				i += 4;
			}
		}
	}
}

static void console_erase_line(int mode);

static int apply_csi(const char *s, int n, ConStyle *st) {
	int params[CONSOLE_SGR_PARAMS];
	int count = 0;
	int acc = 0;
	int any = 0;
	int i = 0;

	while(i < n && i < CONSOLE_CSI_MAX) {
		char c = s[i];

		if(c >= '0' && c <= '9') {
			acc = acc * 10 + (c - '0');
			if(acc > 255) {
				acc = 255;
			}
			any = 1;
			i++;
			continue;
		}

		if(c == ';') {
			if(count < CONSOLE_SGR_PARAMS) {
				params[count++] = any ? acc : 0;
			}
			acc = 0;
			any = 0;
			i++;
			continue;
		}

		if(c >= '@' && c <= '~') {
			if(count < CONSOLE_SGR_PARAMS) {
				params[count++] = any ? acc : 0;
			}
			if(c == 'm') {
				style_apply(st, params, count);
			}
			else if(c == 'K') {
				int mode = CONSOLE_EL_TO_END;

				if(count > 0) {
					mode = params[0];
				}
				console_erase_line(mode);
			}
			return i + 1;
		}
		break;
	}

	return i;
}

static void console_clear_line(ConCell *line) {
	memset(line, 0, sizeof(ConCell) * CONSOLE_MAX_COLS);
}

static void console_erase_line(int mode) {
	int from;
	int to;
	int i;

	if(!console || !console->lines || !console->lines[0]) {
		return;
	}

	if(mode == CONSOLE_EL_ALL) {
		console_clear_line(console->lines[0]);
		return;
	}

	if(mode == CONSOLE_EL_TO_CURSOR) {
		from = 0;
		to = console->col + 1;
	}
	else {
		from = console->col;
		to = CONSOLE_MAX_COLS;
	}

	if(from < 0) {
		from = 0;
	}
	if(to > CONSOLE_MAX_COLS) {
		to = CONSOLE_MAX_COLS;
	}
	for(i = from; i < to; i++) {
		memset(&console->lines[0][i], 0, sizeof(ConCell));
	}
}

static void console_newline(void) {
	ConCell *last;
	int i;

	if(!console || !console->lines) {
		return;
	}

	last = console->lines[console->line_count - 1];
	for(i = console->line_count - 1; i > 0; i--) {
		console->lines[i] = console->lines[i - 1];
	}
	console->lines[0] = last;
	console_clear_line(console->lines[0]);
	console->col = 0;

	if(console->scroll != 0) {
		console->scroll++;
		if(console->scroll > console->line_count - 1) {
			console->scroll = console->line_count - 1;
		}
	}
}

static void console_putc(unsigned char ch) {
	ConCell *cell;

	if(!console || console->vchars < 1) {
		return;
	}

	if(console->col >= console->vchars) {
		console_newline();
	}

	cell = &console->lines[0][console->col];
	cell->ch = ch;
	cell->attr = 0;
	if(console->style.italic) {
		cell->attr |= CONSOLE_ATTR_ITALIC;
	}
	if(console->style.underline) {
		cell->attr |= CONSOLE_ATTR_UNDERLINE;
	}
	if(console->style.reverse) {
		cell->attr |= CONSOLE_ATTR_REVERSE;
	}
	style_fg(&console->style, &cell->fg_r, &cell->fg_g, &cell->fg_b);
	style_bg(&console->style, &cell->bg_r, &cell->bg_g, &cell->bg_b);
	cell->bg_on = console->style.bg_mode != CONSOLE_COLOR_DEFAULT;
	console->col++;
}

static void console_put_span(const char *s, int n) {
	int i = 0;

	while(i < n) {
		unsigned char ch = (unsigned char)s[i];

		if(ch == '\b') {
			if(console->col > 0) {
				console->col--;
			}
			i++;
			continue;
		}

		if(ch == '\t') {
			int next = (console->col + CONSOLE_TAB_WIDTH) & ~(CONSOLE_TAB_WIDTH - 1);

			if(next > console->vchars) {
				next = console->vchars;
			}
			while(console->col < next) {
				console_putc(' ');
			}
			i++;
			continue;
		}

		if(ch == 0x1b) {
			if(i + 1 < n && s[i + 1] == '[') {
				int used = apply_csi(s + i + 2, n - (i + 2), &console->style);

				if(used > 0) {
					i += 2 + used;
					continue;
				}
			}
			i++;
			continue;
		}

		if(ch < 32) {
			i++;
			continue;
		}

		console_putc(ch);
		i++;
	}
}

static int tag_prefix(const char *s, int n, const char *prefix) {
	int i;

	for(i = 0; prefix[i] != 0; i++) {
		if(i >= n || s[i] != prefix[i]) {
			return 0;
		}
	}
	return 1;
}

static int tag_word(const char *s, int n, const char *prefix) {
	int i;

	for(i = 0; prefix[i] != 0; i++) {
		if(i >= n || s[i] != prefix[i]) {
			return 0;
		}
	}
	if(i < n && isalnum((unsigned char)s[i])) {
		return 0;
	}
	return 1;
}

static int line_tag_color(const char *s, int n, int *r, int *g, int *b) {
	if(tag_prefix(s, n, CONSOLE_TAG_ERROR)) {
		*r = CONSOLE_TAG_ERROR_R;
		*g = CONSOLE_TAG_ERROR_G;
		*b = CONSOLE_TAG_ERROR_B;
		return 1;
	}
	if(tag_word(s, n, CONSOLE_TAG_OK)) {
		*r = CONSOLE_TAG_OK_R;
		*g = CONSOLE_TAG_OK_G;
		*b = CONSOLE_TAG_OK_B;
		return 1;
	}
	if(tag_word(s, n, CONSOLE_TAG_INFO)) {
		*r = CONSOLE_TAG_INFO_R;
		*g = CONSOLE_TAG_INFO_G;
		*b = CONSOLE_TAG_INFO_B;
		return 1;
	}
	if(tag_word(s, n, CONSOLE_TAG_PROCESS)) {
		*r = CONSOLE_TAG_PROCESS_R;
		*g = CONSOLE_TAG_PROCESS_G;
		*b = CONSOLE_TAG_PROCESS_B;
		return 1;
	}
	if(tag_word(s, n, CONSOLE_TAG_WARNING)) {
		*r = CONSOLE_TAG_WARNING_R;
		*g = CONSOLE_TAG_WARNING_G;
		*b = CONSOLE_TAG_WARNING_B;
		return 1;
	}
	return 0;
}

static void feed_tag_off(void) {
	if(!feed_tag_on) {
		return;
	}
	console->style = feed_tag_saved;
	feed_tag_on = 0;
}

static void feed_tag_try(const char *s, int n) {
	int r;
	int g;
	int b;

	feed_tag_off();
	if(!line_tag_color(s, n, &r, &g, &b)) {
		return;
	}
	feed_tag_saved = console->style;
	style_set_rgb(&console->style, 0, r, g, b);
	feed_tag_on = 1;
}

static void feed_commit_nl(void) {
	if(!feed_pending_nl) {
		return;
	}
	feed_tag_off();
	console_newline();
	feed_pending_nl = 0;
}

static void feed_newline(void) {
	feed_tag_off();
	if(feed_pending_nl) {
		console_newline();
	}
	feed_pending_nl = 1;
}

static void feed_cr(void) {
	feed_tag_off();
	if(feed_pending_nl) {
		console_newline();
		feed_pending_nl = 0;
		return;
	}
	console->col = 0;
}

static void feed_write(const char *s, int n) {
	if(n <= 0) {
		return;
	}
	feed_commit_nl();
	if(console->col == 0) {
		feed_tag_try(s, n);
	}
	console_put_span(s, n);
}

static void feed_reset(void) {
	feed_pending_nl = 0;
	feed_tag_on = 0;
}

static void console_feed_n(const char *s, int n) {
	int i = 0;

	if(!console || !console->lines || n <= 0) {
		return;
	}

	while(i < n) {
		int start;

		if(s[i] == '\r' && i + 1 < n && s[i + 1] == '\n') {
			feed_newline();
			i += 2;
			continue;
		}
		if(s[i] == '\n') {
			feed_newline();
			i++;
			continue;
		}
		if(s[i] == '\r') {
			feed_cr();
			i++;
			continue;
		}
		start = i;
		while(i < n && s[i] != '\n' && s[i] != '\r') {
			i++;
		}
		feed_write(s + start, i - start);
	}
}

static int console_cmp_width(const void *a, const void *b) {
	float da;
	float db;

	da = *(const float *)a;
	db = *(const float *)b;
	if(da < db) {
		return -1;
	}
	if(da > db) {
		return 1;
	}
	return 0;
}

static float console_cell_unit(plx_font_t *font, float max_w) {
	float *widths;
	float cell;
	int n;
	int i;
	int rank;

	widths = (float *)malloc(sizeof(float) * (size_t)font->glyph_cnt);
	if(!widths) {
		return max_w;
	}

	n = 0;
	for(i = 0; i < font->glyph_cnt; i++) {
		if(font->vert_ur[i].x < CONSOLE_FONT_UNIT_MIN) {
			continue;
		}
		widths[n] = font->vert_ur[i].x;
		n++;
	}
	if(n < 1) {
		free(widths);
		return max_w;
	}

	qsort(widths, (size_t)n, sizeof(float), console_cmp_width);
	rank = (n - 1) * CONSOLE_WIDTH_RANK_NUM / CONSOLE_WIDTH_RANK_DEN;
	cell = widths[rank];
	free(widths);
	if(cell < CONSOLE_FONT_UNIT_MIN) {
		return max_w;
	}
	if(max_w <= cell * CONSOLE_WIDTH_MONO) {
		return max_w;
	}
	return cell;
}

static void console_fit_widths(plx_font_t *font, float cell) {
	int i;

	for(i = 0; i < font->glyph_cnt; i++) {
		float w;
		float k;

		w = font->vert_ur[i].x;
		if(w <= cell || w < CONSOLE_FONT_UNIT_MIN) {
			continue;
		}
		k = cell / w;
		font->vert_ll[i].x *= k;
		font->vert_ur[i].x *= k;
	}
}

static void console_center_glyphs(plx_font_t *font, float cell) {
	int i;

	for(i = 0; i < font->glyph_cnt; i++) {
		float left;
		float right;
		float width;
		float shift;

		left = font->vert_ll[i].x;
		right = font->vert_ur[i].x;
		width = right - left;
		if(width < CONSOLE_FONT_UNIT_MIN) {
			continue;
		}
		shift = (cell - width) * 0.5f - left;
		font->vert_ll[i].x = left + shift;
		font->vert_ur[i].x = right + shift;
	}
}

static int console_open_font(const char *path) {
	plx_font_t *font;
	plx_fcxt_t *fcxt;
	float max_w = 0.5f;
	float asc = 1.0f;
	float desc = 0.0f;
	int i;

	font = plx_font_load(path);
	if(!font) {
		return 0;
	}

	fcxt = plx_fcxt_create(font, PVR_LIST_TR_POLY);
	if(!fcxt) {
		plx_font_destroy(font);
		return 0;
	}

	for(i = 0; i < font->glyph_cnt; i++) {
		if(font->vert_ur[i].x > max_w) {
			max_w = font->vert_ur[i].x;
		}
		if(font->vert_ur[i].y > asc) {
			asc = font->vert_ur[i].y;
		}
		if(-font->vert_ll[i].y > desc) {
			desc = -font->vert_ll[i].y;
		}
	}

	if(asc < CONSOLE_FONT_UNIT_MIN) {
		asc = CONSOLE_FONT_UNIT_MIN;
	}
	if(max_w < CONSOLE_FONT_UNIT_MIN) {
		max_w = CONSOLE_FONT_UNIT_MIN;
	}

	{
		float cell = console_cell_unit(font, max_w);

		if(cell < max_w) {
			console_fit_widths(font, cell);
			console_center_glyphs(font, cell);
			max_w = cell;
		}
	}

	fcxt->gap = 0.0f;
	fcxt->fixed_width = max_w;
	fcxt->flags |= PLX_FCXT_FIXED;
	plx_fcxt_setsize(fcxt, CONSOLE_FONT_SIZE / asc);
	plx_txr_setfilter(font->txr, PVR_FILTER_BILINEAR);

	console->font = font;
	console->fcxt = fcxt;
	console->char_w = max_w * console->fcxt->size;
	if(console->char_w < 1.0f) {
		console->char_w = 1.0f;
	}
	console->ascent = asc * console->fcxt->size;
	console->line_h = (asc + desc) * console->fcxt->size + CONSOLE_LINE_LEAD;
	console->slant = console->fcxt->size * CONSOLE_SLANT;
	return 1;
}

static int console_load_font(const char *path) {
	static const char *names[] = {
		"fonts/txf/typewriter.txf",
		"fonts/txf/lucida.txf",
		"fonts/txf/dreamshell.txf",
		NULL
	};
	char fn[NAME_MAX];
	const char *base;
	size_t len;
	int i;

	if(path && path[0]) {
		len = strlen(path);
		if(len >= 4 && !strcasecmp(path + len - 4, ".txf") && console_open_font(path)) {
			return 1;
		}
	}

	base = getenv("PATH");
	if(!base) {
		base = "";
	}

	for(i = 0; names[i] != NULL; i++) {
		snprintf(fn, sizeof(fn), "%s/%s", base, names[i]);
		if(console_open_font(fn)) {
			return 1;
		}
	}

	return 0;
}

static void console_layout(void) {
	int cols;
	int rows;

	cols = (int)(((float)console->w - CONSOLE_BORDER * 2.0f) / console->char_w);
	if(cols < 1) {
		cols = 1;
	}
	if(cols > CONSOLE_MAX_COLS) {
		cols = CONSOLE_MAX_COLS;
	}
	console->vchars = cols;

	rows = (int)(((float)console->h - CONSOLE_BORDER * 2.0f) / console->line_h);
	if(rows < 2) {
		rows = 2;
	}
	console->vis_rows = rows - 1;
}

static void command_assemble(void) {
	int left = (int)strlen(console->lcmd);
	int right = (int)strlen(console->rcmd);

	if(left > CONSOLE_CMD_MAX) {
		left = CONSOLE_CMD_MAX;
	}
	if(left + right > CONSOLE_CMD_MAX) {
		right = CONSOLE_CMD_MAX - left;
	}

	memcpy(console->command, console->lcmd, (size_t)left);
	memcpy(console->command + left, console->rcmd, (size_t)right);
	console->command[left + right] = '\0';
}

static void command_clear(void) {
	console->cursor = 0;
	console->offset = 0;
	console->command[0] = '\0';
	console->lcmd[0] = '\0';
	console->rcmd[0] = '\0';
}

static void command_newline(void) {
	char *last;
	int i;

	last = console->commands[console->line_count - 1];
	for(i = console->line_count - 1; i > 0; i--) {
		console->commands[i] = console->commands[i - 1];
	}
	console->commands[0] = last;
	console->commands[0][0] = '\0';

	if(console->command_count < console->line_count - 1) {
		console->command_count++;
	}
}

static void cursor_left(void) {
	size_t n;

	if(console->cursor <= 0) {
		return;
	}

	n = strlen(console->lcmd);
	if(n == 0) {
		return;
	}

	memmove(console->rcmd + 1, console->rcmd, strlen(console->rcmd) + 1);
	console->rcmd[0] = console->lcmd[n - 1];
	console->lcmd[n - 1] = '\0';
	console->cursor--;
	command_assemble();
}

static void cursor_right(void) {
	size_t n;

	if(console->cursor >= (int)strlen(console->command)) {
		return;
	}
	if(console->rcmd[0] == '\0') {
		return;
	}

	n = strlen(console->lcmd);
	if(n >= CONSOLE_CMD_MAX) {
		return;
	}
	console->lcmd[n] = console->rcmd[0];
	console->lcmd[n + 1] = '\0';
	memmove(console->rcmd, console->rcmd + 1, strlen(console->rcmd + 1) + 1);
	console->cursor++;
	command_assemble();
}

static void cursor_home(void) {
	char tmp[CONSOLE_CMD_MAX + 1];

	snprintf(tmp, sizeof(tmp), "%s%s", console->lcmd, console->rcmd);
	snprintf(console->rcmd, sizeof(console->rcmd), "%s", tmp);
	console->lcmd[0] = '\0';
	console->cursor = 0;
	command_assemble();
}

static void cursor_end(void) {
	size_t n = strlen(console->lcmd);
	size_t r = strlen(console->rcmd);

	if(n + r > CONSOLE_CMD_MAX) {
		r = CONSOLE_CMD_MAX - n;
	}
	memcpy(console->lcmd + n, console->rcmd, r);
	console->lcmd[n + r] = '\0';
	console->rcmd[0] = '\0';
	console->cursor = (int)strlen(console->lcmd);
	command_assemble();
}

static void cursor_backspace(void) {
	size_t n;

	if(console->cursor <= 0) {
		return;
	}

	n = strlen(console->lcmd);
	if(n == 0) {
		return;
	}
	console->lcmd[n - 1] = '\0';
	console->cursor--;
	if(console->offset > 0) {
		console->offset--;
	}
	command_assemble();
}

static void cursor_delete(void) {
	if(console->rcmd[0] == '\0') {
		return;
	}
	memmove(console->rcmd, console->rcmd + 1, strlen(console->rcmd + 1) + 1);
	command_assemble();
}

static void cursor_add(unsigned char ch) {
	size_t n;

	if(strlen(console->command) + 1 >= CONSOLE_CMD_MAX) {
		return;
	}

	n = strlen(console->lcmd);
	if(n >= CONSOLE_CMD_MAX) {
		return;
	}

	if(!console->insert && console->rcmd[0] != '\0') {
		console->lcmd[n] = (char)ch;
		console->lcmd[n + 1] = '\0';
		memmove(console->rcmd, console->rcmd + 1, strlen(console->rcmd + 1) + 1);
	}
	else {
		console->lcmd[n] = (char)ch;
		console->lcmd[n + 1] = '\0';
	}
	console->cursor++;
	command_assemble();
}

static void command_up(void) {
	if(console->command_scroll >= console->command_count - 1) {
		return;
	}

	console->command_scroll++;
	console->rcmd[0] = '\0';
	console->offset = 0;
	snprintf(console->lcmd, sizeof(console->lcmd), "%s", console->commands[console->command_scroll]);
	console->cursor = (int)strlen(console->lcmd);
	command_assemble();
}

static void command_down(void) {
	if(console->command_scroll <= -1) {
		return;
	}

	console->command_scroll--;
	console->rcmd[0] = '\0';
	console->offset = 0;
	if(console->command_scroll > -1) {
		snprintf(console->lcmd, sizeof(console->lcmd), "%s", console->commands[console->command_scroll]);
	}
	else {
		console->lcmd[0] = '\0';
	}
	console->cursor = (int)strlen(console->lcmd);
	command_assemble();
}

typedef struct {
	char common[CONSOLE_CMD_MAX + 1];
	int count;
	int started;
} TabScan;

typedef struct {
	char line[CONSOLE_HINT_LINE];
	int col;
	int len;
} TabPrint;

static int tab_prefix(const char *name, const char *prefix) {
	size_t n;

	if(!name || !name[0]) {
		return 0;
	}
	n = strlen(prefix);
	return strncmp(name, prefix, n) == 0;
}

static void tab_note(const char *name, void *arg) {
	TabScan *scan = (TabScan *)arg;
	size_t i;

	if(!scan->started) {
		snprintf(scan->common, sizeof(scan->common), "%s", name);
		scan->started = 1;
		scan->count = 1;
		return;
	}

	i = 0;
	while(scan->common[i] && name[i] && scan->common[i] == name[i]) {
		i++;
	}
	scan->common[i] = '\0';
	scan->count++;
}

static void tab_flush(TabPrint *out) {
	if(out->len <= 0) {
		return;
	}
	ds_printf("%s\n", out->line);
	out->line[0] = '\0';
	out->col = 0;
	out->len = 0;
}

static void tab_print_name(const char *name, void *arg) {
	TabPrint *out = (TabPrint *)arg;
	int n;
	int gap;
	int i;

	n = (int)strlen(name);
	if(n >= CONSOLE_HINT_LINE) {
		tab_flush(out);
		ds_printf("%s\n", name);
		return;
	}

	gap = out->len > 0 ? CONSOLE_HINT_GAP : 0;
	if(out->col >= CONSOLE_HINT_COLUMNS || out->len + gap + n >= CONSOLE_HINT_LINE) {
		tab_flush(out);
		gap = 0;
	}
	for(i = 0; i < gap; i++) {
		out->line[out->len++] = ' ';
	}
	memcpy(out->line + out->len, name, (size_t)n);
	out->len += n;
	out->line[out->len] = '\0';
	out->col++;
}

static void tab_each(const char *prefix, void (*fn)(const char *name, void *arg), void *arg) {
	Item_list_t *list;
	Item_t *item;
	file_t fd;
	const dirent_t *ent;
	char dir[NAME_MAX];
	const char *base;

	list = GetCmdList();
	if(list) {
		SLIST_FOREACH(item, list, list) {
			Cmd_t *cmd = (Cmd_t *)item->data;

			if(cmd && tab_prefix(cmd->command, prefix)) {
				fn(cmd->command, arg);
			}
		}
	}

	base = getenv("PATH");
	if(!base) {
		return;
	}
	snprintf(dir, sizeof(dir), "%s/cmds", base);
	fd = fs_open(dir, O_RDONLY | O_DIR);
	if(fd == FILEHND_INVALID) {
		return;
	}
	while((ent = fs_readdir(fd)) != NULL) {
		if(ent->attr == O_DIR) {
			continue;
		}
		if(tab_prefix(ent->name, prefix)) {
			fn(ent->name, arg);
		}
	}
	fs_close(fd);
}

static void console_tab_apply(const char *text, int add_space) {
	int n;

	n = (int)strlen(text);
	if(n + (add_space ? 2 : 1) > CONSOLE_CMD_MAX) {
		n = CONSOLE_CMD_MAX - (add_space ? 2 : 1);
	}
	memcpy(console->lcmd, text, (size_t)n);
	if(add_space) {
		console->lcmd[n] = ' ';
		n++;
	}
	console->lcmd[n] = '\0';
	console->rcmd[0] = '\0';
	console->cursor = n;
	console->offset = 0;
	command_assemble();
}

static void console_tab(void) {
	TabScan scan;
	TabPrint out;
	const char *prefix;

	prefix = console->lcmd;
	if(strchr(prefix, ' ') != NULL) {
		return;
	}

	memset(&scan, 0, sizeof(scan));
	tab_each(prefix, tab_note, &scan);
	if(scan.count < 1) {
		return;
	}
	if(scan.count == 1) {
		console_tab_apply(scan.common, 1);
		return;
	}

	memset(&out, 0, sizeof(out));
	tab_each(prefix, tab_print_name, &out);
	tab_flush(&out);
	if(strlen(scan.common) > strlen(prefix)) {
		console_tab_apply(scan.common, 0);
	}
}

static uint32 pack_rgb(unsigned char r, unsigned char g, unsigned char b, float extra_a) {
	float a = ((float)console->alpha / 255.0f) * extra_a;

	if(a < 0.0f) {
		a = 0.0f;
	}
	if(a > 1.0f) {
		a = 1.0f;
	}
	return PVR_PACK_COLOR(a, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
}

static void flat_begin(void) {
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;

	pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));
	flat_open = 1;
}

static void flat_quad(float x, float y, float w, float h, float z, uint32 color) {
	if(w <= 0.0f || h <= 0.0f) {
		return;
	}
	if(!flat_open) {
		flat_begin();
	}

	plx_vert_ifp(PLX_VERT, x, y, z, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + w, y, z, color, 1.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x, y + h, z, color, 0.0f, 1.0f);
	plx_vert_ifp(PLX_VERT_EOS, x + w, y + h, z, color, 1.0f, 1.0f);
}

static void cell_colors(const ConCell *cell, uint32 *fg, uint32 *bg, int *has_bg) {
	unsigned char fr = cell->fg_r;
	unsigned char fg_g = cell->fg_g;
	unsigned char fb = cell->fg_b;
	unsigned char br = cell->bg_r;
	unsigned char bg_g = cell->bg_g;
	unsigned char bb = cell->bg_b;
	int on = cell->bg_on;

	if(cell->attr & CONSOLE_ATTR_REVERSE) {
		unsigned char tr = fr;
		unsigned char tg = fg_g;
		unsigned char tb = fb;

		fr = on ? br : 0;
		fg_g = on ? bg_g : 0;
		fb = on ? bb : 0;
		br = tr;
		bg_g = tg;
		bb = tb;
		on = 1;
	}

	*fg = pack_rgb(fr, fg_g, fb, 1.0f);
	*bg = pack_rgb(br, bg_g, bb, 1.0f);
	*has_bg = on;
}

static void draw_line_decals(const ConCell *line, float x0, float y) {
	int col = 0;

	while(col < console->vchars) {
		uint32 fg, bg, run_bg;
		int has_bg;
		int start;
		int underline = 0;
		uint32 ul = 0;

		if(line[col].ch == 0) {
			col++;
			continue;
		}

		cell_colors(&line[col], &fg, &bg, &has_bg);
		start = col;
		run_bg = bg;
		underline = (line[col].attr & CONSOLE_ATTR_UNDERLINE) ? 1 : 0;
		ul = fg;
		col++;

		while(col < console->vchars && line[col].ch != 0) {
			uint32 nfg, nbg;
			int non;
			int nul;

			cell_colors(&line[col], &nfg, &nbg, &non);
			nul = (line[col].attr & CONSOLE_ATTR_UNDERLINE) ? 1 : 0;
			if(non != has_bg || (has_bg && nbg != run_bg) || nul != underline || (nul && nfg != ul)) {
				break;
			}
			col++;
		}

		if(has_bg) {
			flat_quad(x0 + (float)start * console->char_w, y,
				(float)(col - start) * console->char_w, console->line_h,
				CONSOLE_Z_CELL, run_bg);
		}
		if(underline) {
			flat_quad(x0 + (float)start * console->char_w,
				y + console->line_h - CONSOLE_UNDERLINE_PX,
				(float)(col - start) * console->char_w, CONSOLE_UNDERLINE_PX,
				CONSOLE_Z_DECOR, ul);
		}
	}
}

static void draw_line_text(const ConCell *line, float x0, float y) {
	int col;

	for(col = 0; col < console->vchars; col++) {
		const ConCell *cell = &line[col];
		uint32 fg, bg;
		int has_bg;

		if(cell->ch == 0 || cell->ch == ' ') {
			continue;
		}

		cell_colors(cell, &fg, &bg, &has_bg);
		console->fcxt->color = fg;
		console->fcxt->slant = (cell->attr & CONSOLE_ATTR_ITALIC) ? console->slant : 0.0f;
		plx_fcxt_setpos(console->fcxt,
			x0 + (float)col * console->char_w,
			y + console->ascent,
			CONSOLE_Z_TEXT);
		plx_fcxt_draw_ch(console->fcxt, cell->ch);
	}
}

static void draw_string(const char *text, int n, float x, float y, uint32 color) {
	int i;

	console->fcxt->color = color;
	console->fcxt->slant = 0.0f;
	for(i = 0; i < n; i++) {
		unsigned char ch = (unsigned char)text[i];

		if(ch == 0 || ch == ' ') {
			continue;
		}
		plx_fcxt_setpos(console->fcxt, x + (float)i * console->char_w, y + console->ascent, CONSOLE_Z_TEXT);
		plx_fcxt_draw_ch(console->fcxt, ch);
	}
}

static void draw_scroll_mark(float x0, float y) {
	int count = (console->vchars / CONSOLE_SCROLL_EVERY) + 1;
	int i;
	uint32 color = pack_rgb(CONSOLE_FG_R, CONSOLE_FG_G, CONSOLE_FG_B, 1.0f);

	console->fcxt->color = color;
	console->fcxt->slant = 0.0f;
	for(i = 0; i < count; i++) {
		plx_fcxt_setpos(console->fcxt,
			x0 + (float)(i * CONSOLE_SCROLL_EVERY) * console->char_w,
			y + console->ascent,
			CONSOLE_Z_TEXT);
		plx_fcxt_draw_ch(console->fcxt, '^');
	}
}

static void draw_background(float x, float y) {
	uint32 color = pack_rgb(CONSOLE_GLASS_R, CONSOLE_GLASS_G, CONSOLE_GLASS_B, CONSOLE_GLASS_ALPHA);

	flat_open = 0;
	flat_begin();
	flat_quad(x, y, (float)console->w, (float)console->h, CONSOLE_Z_BG, color);
	flat_open = 0;

	if(!console->background) {
		return;
	}

	{
		float dw = (float)console->background->w;
		float dh = (float)console->background->h;
		float u, v;
		uint32 tint = pack_rgb(255, 255, 255, 1.0f);

		if((float)console->back_x + dw > (float)console->w) {
			dw = (float)console->w - (float)console->back_x;
		}
		if((float)console->back_y + dh > (float)console->h) {
			dh = (float)console->h - (float)console->back_y;
		}
		if(dw <= 0.0f || dh <= 0.0f) {
			return;
		}

		u = dw / (float)console->background->w;
		v = dh / (float)console->background->h;
		plx_txr_send_hdr(console->background, PVR_LIST_TR_POLY, 0);
		plx_vert_ifp(PLX_VERT, x + (float)console->back_x, y + (float)console->back_y, CONSOLE_Z_IMAGE, tint, 0.0f, 0.0f);
		plx_vert_ifp(PLX_VERT, x + (float)console->back_x + dw, y + (float)console->back_y, CONSOLE_Z_IMAGE, tint, u, 0.0f);
		plx_vert_ifp(PLX_VERT, x + (float)console->back_x, y + (float)console->back_y + dh, CONSOLE_Z_IMAGE, tint, 0.0f, v);
		plx_vert_ifp(PLX_VERT_EOS, x + (float)console->back_x + dw, y + (float)console->back_y + dh, CONSOLE_Z_IMAGE, tint, u, v);
	}
}

static void draw_command(float x0, float y) {
	int prompt_len = (int)strlen(console->prompt);
	int window;
	int visible;
	int cmd_len;
	uint32 prompt_color;
	uint32 text_color;
	uint64 now;
	float phase;
	float wave;

	if(prompt_len > console->vchars) {
		prompt_len = console->vchars;
	}
	window = console->vchars - prompt_len - 1;
	if(window < 1) {
		window = 1;
	}
	if(console->offset < console->cursor - window) {
		console->offset = console->cursor - window;
	}
	if(console->offset > console->cursor) {
		console->offset = console->cursor;
	}
	if(console->offset < 0) {
		console->offset = 0;
	}

	cmd_len = (int)strlen(console->command + console->offset);
	visible = cmd_len;
	if(visible > window) {
		visible = window;
	}

	prompt_color = pack_rgb(CONSOLE_PROMPT_R, CONSOLE_PROMPT_G, CONSOLE_PROMPT_B, 1.0f);
	text_color = pack_rgb(CONSOLE_FG_R, CONSOLE_FG_G, CONSOLE_FG_B, 1.0f);
	draw_string(console->prompt, prompt_len, x0, y, prompt_color);
	draw_string(console->command + console->offset, visible, x0 + (float)prompt_len * console->char_w, y, text_color);

	now = timer_ms_gettime64();
	phase = (float)(now % (uint64)CONSOLE_BLINK_MS) / (float)CONSOLE_BLINK_MS;
	wave = 0.5f + 0.5f * cosf(phase * 2.0f * CONSOLE_CURSOR_PI);
	cursor_alpha = CONSOLE_CURSOR_ALPHA * wave;
	cursor_draw = 1;
	{
		int cx = prompt_len + console->cursor - console->offset;

		if(cx < 0) {
			cx = 0;
		}
		if(cx > console->vchars - 1) {
			cx = console->vchars - 1;
		}
		cursor_x = x0 + (float)cx * console->char_w;
		cursor_y = y;
	}
}

static float console_ease(float t) {
	float inv;

	if(t <= 0.0f) {
		return 0.0f;
	}
	if(t >= 1.0f) {
		return 1.0f;
	}
	inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

static void console_slide_begin(int open) {
	float travel;

	slide_from = slide_pos;
	slide_open = open;
	travel = open ? (1.0f - slide_from) : slide_from;
	if(travel < 0.0f) {
		travel = 0.0f;
	}
	slide_span = (uint32)((float)CONSOLE_SLIDE_MS * travel + 0.5f);
	if(slide_span < 1) {
		slide_span = 1;
	}
	slide_start = timer_ms_gettime64();
	slide_run = 1;
}

static int console_slide_step(void) {
	float t;
	float eased;
	uint64 now;

	if(!slide_run) {
		return 0;
	}

	now = timer_ms_gettime64();
	if(now <= slide_start) {
		t = 0.0f;
	}
	else {
		t = (float)(now - slide_start) / (float)slide_span;
	}
	eased = console_ease(t);
	if(slide_open) {
		slide_pos = slide_from + (1.0f - slide_from) * eased;
	}
	else {
		slide_pos = slide_from * (1.0f - eased);
	}
	if(t < 1.0f) {
		return 0;
	}

	slide_run = 0;
	if(slide_open) {
		slide_pos = 1.0f;
		return 0;
	}
	slide_pos = 0.0f;
	console->visible = 0;
	return 1;
}

static void console_slide_closed(void) {
	if(con_video_event) {
		SetEventActive(con_video_event, 0);
	}
	GUI_Enable();
	if(VirtKeyboardShown()) {
		SDL_DC_EmulateMouse(SDL_FALSE);
	}
	else {
		SDL_DC_EmulateMouse(console_mouse_was ? SDL_TRUE : SDL_FALSE);
	}
}

static void console_draw(void) {
	float x;
	float y;
	float x0;
	int i;
	int closed;

	if(irq_inside_int()) {
		return;
	}

	mutex_lock(&console_mutex);
	if(!console || !console->fcxt || !console->lines || console->w < 1 || console->h < 1) {
		mutex_unlock(&console_mutex);
		return;
	}
	closed = console_slide_step();
	if(!console->visible || slide_pos <= 0.0f) {
		mutex_unlock(&console_mutex);
		if(closed && !ConsoleIsVisible()) {
			console_slide_closed();
		}
		return;
	}

	x = (float)console->x;
	y = (float)console->y - (float)console->h * (1.0f - slide_pos);
	x0 = x + CONSOLE_BORDER;

	draw_background(x, y);

	flat_open = 0;
	flat_begin();
	for(i = 0; i < console->vis_rows; i++) {
		int idx = console->scroll + i;
		float ly;

		if(idx < 0 || idx >= console->line_count) {
			break;
		}
		if(console->scroll != 0 && i == 0) {
			continue;
		}
		ly = y + CONSOLE_BORDER + (float)(console->vis_rows - 1 - i) * console->line_h;
		draw_line_decals(console->lines[idx], x0, ly);
	}
	flat_open = 0;

	plx_fcxt_begin(console->fcxt);
	for(i = 0; i < console->vis_rows; i++) {
		int idx = console->scroll + i;
		float ly = y + CONSOLE_BORDER + (float)(console->vis_rows - 1 - i) * console->line_h;

		if(idx < 0 || idx >= console->line_count) {
			break;
		}
		if(console->scroll != 0 && i == 0) {
			draw_scroll_mark(x0, ly);
		}
		else {
			draw_line_text(console->lines[idx], x0, ly);
		}
	}
	cursor_draw = 0;
	draw_command(x0, y + (float)console->h - CONSOLE_BORDER - console->line_h);
	plx_fcxt_end(console->fcxt);

	if(cursor_draw && cursor_alpha > 0.0f) {
		uint32 color = pack_rgb(CONSOLE_FG_R, CONSOLE_FG_G, CONSOLE_FG_B, cursor_alpha);

		flat_open = 0;
		flat_begin();
		if(console->insert) {
			flat_quad(cursor_x, cursor_y + console->line_h - CONSOLE_UNDERLINE_PX * 2.0f,
				console->char_w, CONSOLE_UNDERLINE_PX * 2.0f, CONSOLE_Z_DECOR, color);
		}
		else {
			flat_quad(cursor_x, cursor_y, CONSOLE_UNDERLINE_PX * 2.0f,
				console->line_h, CONSOLE_Z_DECOR, color);
		}
		flat_open = 0;
	}
	mutex_unlock(&console_mutex);
}

static void Command_Handler(const char *command) {
	dsystem(command);
}

static void console_submit_command(void) {
	char shown[CONSOLE_CMD_MAX + CONSOLE_PROMPT_MAX + 16];
	char run[CONSOLE_CMD_MAX + 1];

	if(irq_inside_int() || !console || console->command[0] == '\0') {
		return;
	}

	mutex_lock(&console_mutex);
	if(!console || console->command[0] == '\0') {
		mutex_unlock(&console_mutex);
		return;
	}
	command_newline();
	snprintf(console->commands[0], CONSOLE_CMD_MAX + 1, "%s", console->command);
	snprintf(shown, sizeof(shown), "\033[36m%s\033[0m%s", console->prompt, console->command);
	snprintf(run, sizeof(run), "%s", console->command);
	command_clear();
	console->command_scroll = -1;
	mutex_unlock(&console_mutex);

	ds_printf("%s\n", shown);
	Command_Handler(run);
}

static void console_scroll_by(int delta) {
	int next;

	if(!console) {
		return;
	}
	next = console->scroll + delta;
	if(next < 0) {
		next = 0;
	}
	if(next > console->line_count - 1) {
		next = console->line_count - 1;
	}
	console->scroll = next;
}

static void console_trigger_scroll(int axis, int value) {
	uint64 *due;
	uint64 now;
	int delta;
	int held;

	if(axis == CONSOLE_TRIG_AXIS_LEFT) {
		due = &trig_left_next;
		delta = CONSOLE_TRIG_STEP;
	}
	else if(axis == CONSOLE_TRIG_AXIS_RIGHT) {
		due = &trig_right_next;
		delta = -CONSOLE_TRIG_STEP;
	}
	else {
		return;
	}

	held = value >= CONSOLE_TRIG_THRESHOLD;
	if(!held) {
		*due = 0;
		return;
	}

	now = timer_ms_gettime64();
	if(*due != 0 && now < *due) {
		return;
	}

	mutex_lock(&console_mutex);
	console_scroll_by(delta);
	mutex_unlock(&console_mutex);
	*due = now + CONSOLE_TRIG_REPEAT_MS;
}

static void ConsoleEventHandler(void *ds_event, void *param, int action) {
	SDL_Event *event = (SDL_Event *)param;

	(void)ds_event;
	(void)action;

	if(!console) {
		return;
	}

	if(console->visible) {
		if(event->type == SDL_JOYAXISMOTION) {
			if(!VirtKeyboardShown() && event->jaxis.which == 0) {
				console_trigger_scroll(event->jaxis.axis, event->jaxis.value);
			}
			return;
		}
		if(event->type == SDL_KEYDOWN) {
			if(event->key.keysym.sym == SDLK_F1 || event->key.keysym.sym == SDLK_ESCAPE) {
				if(slide_run && !slide_open) {
					ShowConsole();
				}
				else {
					HideConsole();
				}
				return;
			}

			if(irq_inside_int()) {
				return;
			}
			mutex_lock(&console_mutex);
			if(!console) {
				mutex_unlock(&console_mutex);
				return;
			}

			if(event->key.keysym.mod & KMOD_CTRL) {
				switch(event->key.keysym.sym) {
					case SDLK_a:
						cursor_home();
						break;
					case SDLK_e:
						cursor_end();
						break;
					case SDLK_c:
						command_clear();
						break;
					case SDLK_l:
						ConsoleClear();
						break;
					default:
						break;
				}
			}
			else if(!(event->key.keysym.mod & KMOD_ALT)) {
				switch(event->key.keysym.sym) {
					case SDLK_HOME:
						if(event->key.keysym.mod & KMOD_SHIFT) {
							console->scroll = console->line_count - 1;
						}
						else {
							cursor_home();
				}
				break;
					case SDLK_END:
						if(event->key.keysym.mod & KMOD_SHIFT) {
							console->scroll = 0;
						}
						else {
							cursor_end();
						}
						break;
					case SDLK_PAGEUP:
						console->scroll += CONSOLE_PAGE_LINES;
						if(console->scroll > console->line_count - 1) {
							console->scroll = console->line_count - 1;
						}
						break;
					case SDLK_PAGEDOWN:
						console->scroll -= CONSOLE_PAGE_LINES;
						if(console->scroll < 0) {
							console->scroll = 0;
						}
						break;
					case SDLK_UP:
						command_up();
						break;
					case SDLK_DOWN:
						command_down();
						break;
					case SDLK_LEFT:
						cursor_left();
						break;
					case SDLK_RIGHT:
						cursor_right();
						break;
					case SDLK_BACKSPACE:
						cursor_backspace();
						break;
					case SDLK_DELETE:
						cursor_delete();
						break;
					case SDLK_INSERT:
						console->insert = !console->insert;
						break;
					case SDLK_TAB:
						console_tab();
						break;
					case SDLK_RETURN:
						mutex_unlock(&console_mutex);
						console_submit_command();
						return;
			default:
						if(event->key.keysym.unicode > 0 && event->key.keysym.unicode < 256 &&
							isprint((int)event->key.keysym.unicode)) {
							cursor_add((unsigned char)event->key.keysym.unicode);
						}
				break;
		} 
			}
			mutex_unlock(&console_mutex);
		}
	}
	else if(event->type == SDL_KEYDOWN) {
				switch(event->key.keysym.sym) {
					case SDLK_F1:
						ShowConsole();
						break;
					case SDLK_ESCAPE:
				if(GUI_ScreenGetFocusWidget(GUI_GetScreen()) == NULL) {
							ShowConsole();
						}
						break;
					default:
						break;
				}
	}
}

static void ConsoleDrawHandler(void *ds_event, void *param, int action) {
	(void)ds_event;
	(void)param;

	if(!console || !console->visible) {
		return;
	}
	if(action == EVENT_ACTION_RENDER_POST) {
		console_draw();
	}
}

static void console_free_lines(ConsoleState *con) {
	int i;

	if(!con) {
		return;
	}

	if(con->lines) {
		for(i = 0; i < con->line_count; i++) {
			free(con->lines[i]);
		}
		free(con->lines);
		con->lines = NULL;
	}
	if(con->commands) {
		for(i = 0; i < con->line_count; i++) {
			free(con->commands[i]);
		}
		free(con->commands);
		con->commands = NULL;
	}
}

void SetConsoleDebug(int mode) {
	console_debug = mode;
}

int ds_printf(const char *fmt, ...) {
	va_list args;
	char buf[1024];
	int i;
	int n;

	va_start(args, fmt);
	i = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	n = i;
	if(n < 0 || n >= (int)sizeof(buf)) {
		n = (int)strlen(buf);
	}

	if(irq_inside_int()) {
		dbglog(DBG_DEBUG, "%s", buf);
		return i;
	}

	mutex_lock(&console_mutex);
	if(console_debug) {
		dbglog(DBG_DEBUG, "%s", buf);
	}
	console_feed_n(buf, n);
	mutex_unlock(&console_mutex);
	return i;
}

void ConsoleClear(void) {
	int i;

	if(irq_inside_int()) {
		return;
	}

	mutex_lock(&console_mutex);
	if(console && console->lines) {
		for(i = 0; i < console->line_count; i++) {
			console_clear_line(console->lines[i]);
		}
		console->col = 0;
		console->scroll = 0;
		feed_reset();
		style_reset(&console->style);
	}
	mutex_unlock(&console_mutex);
}

void ConsoleSetBackground(const char *image, int x, int y) {
	plx_texture_t *txr;

	if(!console) {
		return;
	}

	if(!image || !image[0]) {
		if(console->background) {
			plx_txr_destroy(console->background);
			console->background = NULL;
		}
		return;
	}

	txr = plx_txr_load(image, 1, 0);
	if(!txr) {
		ds_printf("DS_ERROR: Cannot load background %s\n", image);
		return;
	}

	txr->cxt_trans.gen.culling = PVR_CULLING_NONE;
	plx_txr_flush_hdrs(txr);
	if(console->background) {
		plx_txr_destroy(console->background);
	}
	console->background = txr;
	console->back_x = x;
	console->back_y = y;
}

void ConsoleSetPosition(int x, int y) {
	if(!console) {
		return;
	}
	if(x < 0) {
		x = 0;
	}
	if(y < 0) {
		y = 0;
	}
	console->x = x;
	console->y = y;
}

void ConsoleResize(int x, int y, int w, int h) {
	if(!console) {
		return;
	}
	if(w < 1) {
		w = 1;
	}
	if(h < 1) {
		h = 1;
	}
	console->x = x;
	console->y = y;
	console->w = w;
	console->h = h;
	console_layout();
	console->scroll = 0;
}

void ConsoleSetAlpha(int alpha) {
	if(!console) {
		return;
	}
	if(alpha < 0) {
		alpha = 0;
	}
	if(alpha > 255) {
		alpha = 255;
	}
	console->alpha = alpha;
}

void ConsoleGetRect(int *x, int *y, int *w, int *h) {
	if(!console) {
		if(x) {
			*x = 0;
		}
		if(y) {
			*y = 0;
		}
		if(w) {
			*w = 0;
		}
		if(h) {
			*h = 0;
		}
		return;
	}

	if(x) {
		*x = console->x;
	}
	if(y) {
		*y = console->y;
	}
	if(w) {
		*w = console->w;
	}
	if(h) {
		*h = console->h;
	}
}

int InitConsole(const char *font, const char *background, int lines, int x, int y, int w, int h, int alpha) {
	int i;

	if(lines < 2) {
		lines = 2;
	}

	console = (ConsoleState *)calloc(1, sizeof(ConsoleState));
	if(!console) {
		dbglog(DBG_ERROR, "console: out of memory\n");
		return 0;
	}

	console->command_scroll = -1;
	console->insert = 1;
	feed_reset();
	style_reset(&console->style);

	if(!console_load_font(font)) {
		dbglog(DBG_ERROR, "console: can't load a TXF font\n");
		free(console);
		console = NULL;
		return 0;
	}

	console->x = x;
	console->y = y;
	console->w = w > 0 ? w : 1;
	console->h = h > 0 ? h : 1;
	console_layout();

	if(alpha < 0) {
		alpha = 0;
	}
	if(alpha > 255) {
		alpha = 255;
	}
	console->alpha = alpha;
	snprintf(console->prompt, sizeof(console->prompt), "D$: ");

	console->line_count = lines;
	console->lines = (ConCell **)calloc((size_t)lines, sizeof(ConCell *));
	console->commands = (char **)calloc((size_t)lines, sizeof(char *));
	if(!console->lines || !console->commands) {
		dbglog(DBG_ERROR, "console: out of memory\n");
		ShutdownConsole();
		return 0;
	}

	for(i = 0; i < lines; i++) {
		console->lines[i] = (ConCell *)calloc(CONSOLE_MAX_COLS, sizeof(ConCell));
		console->commands[i] = (char *)calloc(CONSOLE_CMD_MAX + 1, 1);
		if(!console->lines[i] || !console->commands[i]) {
			dbglog(DBG_ERROR, "console: out of memory\n");
			ShutdownConsole();
			return 0;
		}
	}

	if(background && background[0]) {
		ConsoleSetBackground(background, 0, 0);
	}

	con_input_event = AddEvent("Console_Input", EVENT_TYPE_INPUT, EVENT_PRIO_OVERLAY, ConsoleEventHandler, NULL);
	con_video_event = AddEvent("Console_Video", EVENT_TYPE_VIDEO, EVENT_PRIO_DEFAULT, ConsoleDrawHandler, NULL);

	ds_printf("Enter 'help' for print command list.\n");
	ds_printf("Colors: \033[31mred\033[0m \033[32mgreen\033[0m \033[33myellow\033[0m \033[36mcyan\033[0m\n\n");
	return 1;
}

void ShutdownConsole(void) {
	ConsoleState *con;

	if(irq_inside_int()) {
		return;
	}

	mutex_lock(&console_mutex);
	con = console;
	console = NULL;
	mutex_unlock(&console_mutex);

	if(con_input_event) {
	RemoveEvent(con_input_event);
		con_input_event = NULL;
	}
	if(con_video_event) {
	RemoveEvent(con_video_event);
		con_video_event = NULL;
	}

	if(!con) {
		return;
	}

	if(con->background) {
		plx_txr_destroy(con->background);
	}
	if(con->fcxt) {
		plx_fcxt_destroy(con->fcxt);
	}
	if(con->font) {
		plx_font_destroy(con->font);
	}
	console_free_lines(con);
	free(con);
}

int ToggleConsole(void) {
	int vis = 0;

	if(console) {
		if((vis = ConsoleIsVisible())) {
			HideConsole();
		}
		else {
			ShowConsole();
		}
	}
	return vis;
}

void ShowConsole(void) {
	int enable;

	if(!console) {
		return;
	}

	mutex_lock(&console_mutex);
	if(!console || (console->visible && !slide_run && slide_pos >= 1.0f)) {
		mutex_unlock(&console_mutex);
		return;
	}
	enable = !console->visible;
	console_slide_begin(1);
	console->visible = 1;
	mutex_unlock(&console_mutex);

	if(enable) {
		console_mouse_was = SDL_DC_MouseEmulated();
		GUI_Disable();
		if(con_video_event) {
			SetEventActive(con_video_event, 1);
		}
	}
}

void HideConsole(void) {
	if(!console) {
		return;
	}

	mutex_lock(&console_mutex);
	if(!console || !console->visible || (slide_run && !slide_open)) {
		mutex_unlock(&console_mutex);
		return;
	}
	console_slide_begin(0);
	mutex_unlock(&console_mutex);
}

int ConsoleIsVisible(void) {
	return console && console->visible;
}
