/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter

   Tsunami scene. Still charts are drawn once into a linear texture.
   Scroll, grid scroll, and drop shadow move as widgets, so a frame
   does not upload the whole screen. The menu bar eases between rows.
*/

#include "view.h"
#include "gpu.h"
#include "patterns.h"
#include "scroll_art.h"
#include "scroll_clouds.h"

#include <ds.h>
#include <stdio.h>
#include <string.h>
#include <tsunami/tsunami.h>

#define CHART_W 1024
#define CHART_H 512
#define SCROLL_TEX_W 1024
#define SCROLL_TEX_H 256
#define VERT_TEX 512
#define GRID_LINES 40
#define TEXT_MAX_W 512
#define TEXT_MAX_H 64
#define FRAME_COUNT 6

typedef struct {
	Banner *banner;
	Texture *tex;
	int vis_w;
	int vis_h;
	int tex_w;
	int tex_h;
} glyph_t;

typedef struct {
	Rectangle *edge[4];
} frame_t;

static App_t *view_app;
static int screen_w = 640;
static int screen_h = 480;
static int ready;

static uint16_t chart_pix[CHART_W * CHART_H] __attribute__((aligned(32)));
static uint16_t text_pix[TEXT_MAX_W * TEXT_MAX_H];
static Texture *chart_tex;
static Banner *chart_banner;
static int chart_on;
static int chart_owner;
static int soft_pattern = -1;
static int soft_variant = -999;
static int soft_param = -999;
static int soft_bake = -1;
static Texture *help_tex;
static Banner *help_banner;
static int help_on;
static int help_failed;
static int help_valid;
static int help_pattern = -1;
static int help_variant = -999;
static int help_param = -999;
#define STRIP_SKY 0
#define STRIP_MID 1
#define STRIP_NEAR 2
#define STRIP_COUNT 3
#define CLOUD_TEX_W 512
#define CLOUD_TEX_H 128

typedef struct {
	Texture *tex;
	Banner *banner;
	int on;
	int src_y;
	int src_h;
	int crop_y;
	int crop_h;
	int tex_h;
	int argb;
	int ready;
} strip_t;

static strip_t strip_layer[STRIP_COUNT];
static Texture *cloud_tex;
static Banner *cloud_ban[SCROLL_CLOUD_COUNT];
static Banner *cloud_wrap[SCROLL_CLOUD_COUNT];
static int cloud_on[SCROLL_CLOUD_COUNT];
static int cloud_wrap_on[SCROLL_CLOUD_COUNT];
static int clouds_ready;
static int near_phase = -1;
#define SHAFT_ROWS (SCROLL_SHAFT_Y1 - SCROLL_SHAFT_Y0)
static uint16_t shaft_base[SHAFT_ROWS * SCROLL_TEX_W];
static int shaft_saved;
static Texture *floor_tex;
static Texture *vert_tex;
static Banner *floor_banner;
static Banner *vert_banner;
static int floor_on;
static int vert_on;
static int scroll_ready;
static int scroll_failed;
static int scroll_geom;
static int floor_built;
static int vert_built;
static int geom_scale;
static int geom_back_w;
static int geom_back_h;
static int geom_y0;
static int geom_vert_w;
static int geom_vert_h;
static int geom_x0;
static Rectangle *grid_v[GRID_LINES];
static Rectangle *grid_h[GRID_LINES];
static Rectangle *grid_border[4];
static Rectangle *grid_cross[2];
static int grid_made;
static int grid_on;
static Rectangle *shadow_fill;
static Rectangle *shadow_box;
static Rectangle *shadow_edge[4];
static int shadow_made;
static int shadow_on;
static int shadow_variant = -999;

static glyph_t title_glyph;
static glyph_t author_glyph;
static glyph_t version_glyph;
static glyph_t index_glyph;
static glyph_t mode_glyph;
static glyph_t hint_glyph[4];
static glyph_t name_glyph[CRT_PATTERN_MAX];
static glyph_t up_glyph;
static glyph_t down_glyph;
static glyph_t option_title;
static glyph_t option_music;
static glyph_t option_exit;
static glyph_t option_resume;
static glyph_t option_hint[2];
static glyph_t saver_title;
static glyph_t saver_version;
static char mode_cached[80];
static char index_cached[16];

static frame_t frames[FRAME_COUNT];
static Rectangle *hint_fill;
static Rectangle *backdrop;
static Rectangle *select_bar;
static Rectangle *list_track;
static Rectangle *list_thumb;
static frame_t scroll_frames[2];
static frame_t option_frames[2];
static frame_t saver_frames[2];
static Rectangle *option_fill;
static Rectangle *option_bar;
static Rectangle *check_edge[4];
static Rectangle *check_mark;
static Rectangle *saver_dim;
static Rectangle *saver_fill;
static int saver_x;
static int saver_y;
static int saver_vx;
static int saver_vy;
static int saver_ax;
static int saver_ay;
static int saver_hue;
static int saver_alive;
static int view_panel;
static LogXYZMover *select_mover;
static int bar_h;
static int names_ready;
static int was_menu = -1;
static int last_sel = -1;
static int last_scroll = -1;
static int last_help = -1;

static const Color COL_GREEN = {1.0f, 140.0f / 255.0f, 255.0f / 255.0f, 48.0f / 255.0f};
static const Color COL_DIM = {1.0f, 64.0f / 255.0f, 168.0f / 255.0f, 36.0f / 255.0f};
static const Color COL_RED = {1.0f, 255.0f / 255.0f, 42.0f / 255.0f, 28.0f / 255.0f};
static const Color COL_PAPER = {1.0f, 240.0f / 255.0f, 244.0f / 255.0f, 236.0f / 255.0f};
static const Color COL_INK = {1.0f, 8.0f / 255.0f, 8.0f / 255.0f, 8.0f / 255.0f};
static const Color COL_BLACK = {1.0f, 0.0f, 0.0f, 0.0f};
static const Color COL_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
static const Color COL_PURE = {1.0f, 1.0f, 0.0f, 0.0f};
static const Color COL_CLEAR = {0.0f, 0.0f, 0.0f, 0.0f};

static int ui_scale(void) {
	return screen_h >= 200 ? 2 : 1;
}

static int safe_x(void) {
	int margin = screen_w / 20;

	if(margin < 12) {
		margin = 12;
	}
	if(margin > 32) {
		margin = 32;
	}
	return margin;
}

static int safe_y(void) {
	int margin = screen_h / 10;

	if(margin < 20) {
		margin = 20;
	}
	if(margin > 48) {
		margin = 48;
	}
	return margin;
}

static int pot_size(int value) {
	int pot = 8;

	if(value < 1) {
		value = 1;
	}
	while(pot < value && pot < 1024) {
		pot <<= 1;
	}
	return pot;
}

static void upper_copy(char *dst, int dst_len, const char *src) {
	int i = 0;

	if(!src) {
		src = "";
	}
	if(dst_len < 1) {
		return;
	}
	while(src[i] && i < dst_len - 1) {
		char c = src[i];

		if(c >= 'a' && c <= 'z') {
			c = (char)(c - 'a' + 'A');
		}
		dst[i++] = c;
	}
	dst[i] = '\0';
}

static void banner_uv(Banner *banner, int tex_w, int tex_h, int vis_w, int vis_h) {
	float u = tex_w > 0 ? (float)vis_w / (float)tex_w : 1.0f;
	float v = tex_h > 0 ? (float)vis_h / (float)tex_h : 1.0f;

	TSU_BannerSetUV(banner, 0.0f, v, 0.0f, 0.0f, u, v, u, 0.0f);
}

static void banner_window(Banner *banner, float u0, float v0, float u1, float v1) {
	if(banner) {
		TSU_BannerSetUV(banner, u0, v1, u0, v0, u1, v1, u1, v0);
	}
}

static void banner_place(Banner *banner, float left, float top, float w, float h, float z) {
	Vector pos;

	if(!banner) {
		return;
	}
	pos.x = left + w * 0.5f;
	pos.y = top + h * 0.5f;
	pos.z = z;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)banner, &pos);
}

static void glyph_clear(glyph_t *glyph) {
	glyph->banner = NULL;
	glyph->tex = NULL;
	glyph->vis_w = 0;
	glyph->vis_h = 0;
	glyph->tex_w = 0;
	glyph->tex_h = 0;
}

static void glyph_free(glyph_t *glyph) {
	if(!glyph) {
		return;
	}
	if(glyph->banner && view_app && view_app->tsunami) {
		TSU_AppSubRemoveBanner(view_app->tsunami, glyph->banner);
	}
	if(glyph->banner) {
		TSU_BannerDestroyAll(&glyph->banner);
		glyph->tex = NULL;
	}
	else if(glyph->tex) {
		TSU_TextureDestroy(&glyph->tex);
	}
	glyph_clear(glyph);
}

static int glyph_build(glyph_t *glyph, const char *text, int scale, int span) {
	int advance = span > 0 ? span : 8;
	int vis_w;
	int vis_h;
	int tex_w;
	int tex_h;
	fb_t fb;
	uint16_t white;

	if(!glyph || !view_app || !view_app->tsunami || !text || !text[0]) {
		return 0;
	}
	if(scale < 1) {
		scale = 1;
	}
	vis_w = (int)strlen(text) * advance * scale + 2;
	vis_h = 8 * scale;
	if(vis_w > TEXT_MAX_W || vis_h > TEXT_MAX_H || vis_w < 1 || vis_h < 1) {
		return 0;
	}
	tex_w = pot_size(vis_w);
	tex_h = pot_size(vis_h);
	if(glyph->tex && (glyph->tex_w != tex_w || glyph->tex_h != tex_h)) {
		return 0;
	}
	if(!glyph->tex) {
		glyph->tex = crt_canvas_argb(tex_w, tex_h);
		if(!glyph->tex) {
			return 0;
		}
		glyph->banner = TSU_BannerCreate(PVR_LIST_TR_POLY, glyph->tex);
		if(!glyph->banner) {
			TSU_TextureDestroy(&glyph->tex);
			return 0;
		}
		TSU_AppSubAddBanner(view_app->tsunami, glyph->banner);
		glyph->tex_w = tex_w;
		glyph->tex_h = tex_h;
	}
	memset(text_pix, 0, (size_t)vis_w * (size_t)vis_h * sizeof(uint16_t));
	white = (uint16_t)((31 << 11) | (63 << 5) | 31);
	fb_bind(&fb, text_pix, vis_w, vis_h, vis_w, 3, 2, 3, 11, 5, 0);
	if(span > 0) {
		fb_text_span_strong(&fb, 0, 0, scale, span, white, text);
	}
	else {
		fb_text_strong(&fb, 0, 0, scale, white, text);
	}
	crt_canvas_upload(glyph->tex, text_pix, vis_w, vis_h);
	glyph->vis_w = vis_w;
	glyph->vis_h = vis_h;
	TSU_BannerSetSize(glyph->banner, (float)vis_w, (float)vis_h);
	banner_uv(glyph->banner, tex_w, tex_h, vis_w, vis_h);
	return 1;
}

static void paint_arrow(fb_t *fb, int side, int down) {
	uint16_t white = (uint16_t)((31 << 11) | (63 << 5) | 31);
	int y;

	for(y = 0; y < side; y++) {
		int along = down ? (side - 1 - y) : y;
		int span;
		int x0;

		if(side < 2) {
			fb_pixel(fb, 0, y, white);
			continue;
		}
		span = 1 + (along * (side - 1)) / (side - 1);
		x0 = (side - span) / 2;
		fb_fill_rect(fb, x0, y, span, 1, white);
	}
}

static int glyph_build_arrow(glyph_t *glyph, int scale, int down) {
	int side;
	int tex_w;
	int tex_h;
	fb_t fb;

	if(!glyph || !view_app || !view_app->tsunami) {
		return 0;
	}
	if(scale < 1) {
		scale = 1;
	}
	side = 8 * scale;
	if(side > TEXT_MAX_W || side > TEXT_MAX_H) {
		return 0;
	}
	tex_w = pot_size(side);
	tex_h = pot_size(side);
	if(glyph->tex && (glyph->tex_w != tex_w || glyph->tex_h != tex_h)) {
		return 0;
	}
	if(!glyph->tex) {
		glyph->tex = crt_canvas_argb(tex_w, tex_h);
		if(!glyph->tex) {
			return 0;
		}
		glyph->banner = TSU_BannerCreate(PVR_LIST_TR_POLY, glyph->tex);
		if(!glyph->banner) {
			TSU_TextureDestroy(&glyph->tex);
			return 0;
		}
		TSU_AppSubAddBanner(view_app->tsunami, glyph->banner);
		glyph->tex_w = tex_w;
		glyph->tex_h = tex_h;
	}
	memset(text_pix, 0, (size_t)side * (size_t)side * sizeof(uint16_t));
	fb_bind(&fb, text_pix, side, side, side, 3, 2, 3, 11, 5, 0);
	paint_arrow(&fb, side, down);
	crt_canvas_upload(glyph->tex, text_pix, side, side);
	glyph->vis_w = side;
	glyph->vis_h = side;
	TSU_BannerSetSize(glyph->banner, (float)side, (float)side);
	banner_uv(glyph->banner, tex_w, tex_h, side, side);
	return 1;
}

static float glyph_nominal(const glyph_t *glyph) {
	float nominal;

	if(!glyph || glyph->vis_w < 1) {
		return 0.0f;
	}
	nominal = (float)glyph->vis_w - 2.0f;
	if(nominal < 1.0f) {
		nominal = (float)glyph->vis_w;
	}
	return nominal;
}

static void glyph_move_z(glyph_t *glyph, float left, float top, float z) {
	if(!glyph) {
		return;
	}
	banner_place(glyph->banner, left, top, (float)glyph->vis_w, (float)glyph->vis_h, z);
}

static void glyph_move(glyph_t *glyph, float left, float top) {
	glyph_move_z(glyph, left, top, 20.0f);
}

static void glyph_center_z(glyph_t *glyph, float origin, float span, float top, float z) {
	float nominal = glyph_nominal(glyph);

	glyph_move_z(glyph, origin + (span - nominal) * 0.5f, top, z);
}

static void glyph_center(glyph_t *glyph, float origin, float span, float top) {
	glyph_center_z(glyph, origin, span, top, 20.0f);
}

static void glyph_tint(glyph_t *glyph, Color color) {
	if(glyph && glyph->banner) {
		TSU_DrawableSetTint((Drawable *)glyph->banner, &color);
	}
}

static void rect_move(Rectangle *rect, float x, float top, float w, float h) {
	Vector pos;
	const Vector *cur;

	if(!rect) {
		return;
	}
	cur = TSU_DrawableGetTranslate((Drawable *)rect);
	pos.x = x;
	pos.y = top + h;
	pos.z = cur ? cur->z : 8.0f;
	pos.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)rect, &pos);
	TSU_RectangleSetSize(rect, w, h);
}

static void rect_alpha(Rectangle *rect, float alpha) {
	if(rect) {
		TSU_DrawableSetAlpha((Drawable *)rect, alpha);
	}
}

static void rect_free(Rectangle **rect) {
	if(!rect || !*rect) {
		return;
	}
	if(view_app && view_app->tsunami) {
		TSU_AppSubRemoveRectangle(view_app->tsunami, *rect);
	}
	TSU_RectangleDestroy(rect);
}

static Rectangle *make_rect_off(pvr_list_type_t list, Color color, float z) {
	if(!view_app || !view_app->tsunami) {
		return NULL;
	}
	return TSU_RectangleCreate(list, 0.0f, (float)screen_h, 1.0f, 1.0f, &color, z, 0.0f);
}

static Rectangle *make_rect(pvr_list_type_t list, Color color, float z) {
	Rectangle *rect;

	rect = make_rect_off(list, color, z);
	if(rect && view_app && view_app->tsunami) {
		TSU_AppSubAddRectangle(view_app->tsunami, rect);
	}
	return rect;
}

static void frame_make(frame_t *frame, Color color, float z) {
	int i;

	for(i = 0; i < 4; i++) {
		frame->edge[i] = make_rect(PVR_LIST_TR_POLY, color, z);
	}
}

static void frame_place(frame_t *frame, float x, float top, float w, float h) {
	if(!frame || w < 1.0f || h < 1.0f) {
		return;
	}
	rect_move(frame->edge[0], x, top, w, 1.0f);
	rect_move(frame->edge[1], x, top + h - 1.0f, w, 1.0f);
	rect_move(frame->edge[2], x, top, 1.0f, h);
	rect_move(frame->edge[3], x + w - 1.0f, top, 1.0f, h);
}

static void frame_alpha(frame_t *frame, float alpha) {
	int i;

	if(!frame) {
		return;
	}
	for(i = 0; i < 4; i++) {
		rect_alpha(frame->edge[i], alpha);
	}
}

static void frame_free(frame_t *frame) {
	int i;

	if(!frame) {
		return;
	}
	for(i = 0; i < 4; i++) {
		rect_free(&frame->edge[i]);
	}
}

static void bar_go(float x, float top, float w, float h, int snap) {
	Vector pos;

	if(!select_bar) {
		return;
	}
	pos.x = x;
	pos.y = top + h;
	pos.z = 15.0f;
	pos.w = 1.0f;
	TSU_RectangleSetSize(select_bar, w, h);
	bar_h = (int)h;
	if(snap || !select_mover) {
		TSU_DrawableAnimRemove((Drawable *)select_bar, (Animation *)select_mover);
		TSU_DrawableSetTranslate((Drawable *)select_bar, &pos);
		if(!select_mover) {
			select_mover = TSU_LogXYZMoverCreate(pos.x, pos.y, pos.z);
			TSU_LogXYZMoverSetFactor(select_mover, 6.0f);
		}
		else {
			TSU_LogXYZMoverSetTarget(select_mover, pos.x, pos.y, pos.z);
		}
		return;
	}
	TSU_DrawableAnimRemove((Drawable *)select_bar, (Animation *)select_mover);
	TSU_LogXYZMoverSetTarget(select_mover, pos.x, pos.y, pos.z);
	TSU_DrawableAnimAdd((Drawable *)select_bar, (Animation *)select_mover);
}

static int scroll_arrow(int list_scale) {
	int arrow = 8 * list_scale;

	if(arrow < 8) {
		arrow = 8;
	}
	return arrow;
}

static int scroll_gutter_w(int list_scale) {
	return scroll_arrow(list_scale) + 14;
}

static int scroll_gap(void) {
	return 14;
}

static int layout_metrics(int show_help, int *list_x, int *list_w, int *list_top,
	int *list_bottom, int *line_h, int *hint_y, int *hint_h, int *hint_step) {
	int margin_x = safe_x();
	int margin_y = safe_y();
	int scale = ui_scale();
	int list_scale = (screen_h >= 400 && screen_w >= 600) ? 3 : scale;
	int title_scale = list_scale >= 3 ? 4 : scale + 1;
	int y = margin_y;

	y += 8 * title_scale + 8;
	y += 8 * scale + 6;
	y += 8 * scale + 10;
	*hint_y = y;
	*hint_step = 8 * scale + 4;
	*hint_h = 16 + *hint_step * 4;
	if(show_help) {
		y += *hint_h + 12;
	}
	else {
		y += 8;
	}
	*list_x = margin_x + 8;
	*list_w = screen_w - *list_x * 2;
	{
		int reserve = scroll_gap() + scroll_gutter_w(list_scale);

		if(*list_w - reserve >= 8 * list_scale * 6) {
			*list_w -= reserve;
		}
	}
	*list_top = y;
	*list_bottom = screen_h - margin_y - 8;
	{
		int text_h = 8 * list_scale;
		int raw = text_h + 8;
		int span = *list_bottom - *list_top;
		int rows = raw > 0 ? span / raw : 1;
		int min_line = text_h + 4;

		if(rows < 1) {
			rows = 1;
		}
		if(min_line > 0 && rows + 1 <= span / min_line) {
			rows += 1;
			*line_h = span / rows;
			if(*line_h < min_line) {
				*line_h = min_line;
			}
		}
		else {
			*line_h = raw;
		}
	}
	if(*list_bottom < *list_top + *line_h) {
		*list_bottom = screen_h - 4;
	}
	return list_scale;
}

static int list_origin(int list_top, int list_bottom, int line_h, int visible) {
	int span = list_bottom - list_top;
	int used = visible * line_h;

	if(line_h < 1 || visible < 1 || used >= span) {
		return list_top;
	}
	return list_top + (span - used) / 2;
}

static void place_header(int show_help) {
	int margin_x = safe_x();
	int margin_y = safe_y();
	int scale = ui_scale();
	int list_scale = (screen_h >= 400 && screen_w >= 600) ? 3 : scale;
	int title_scale = list_scale >= 3 ? 4 : scale + 1;
	int y = margin_y;
	int hint_y;
	int hint_h;
	int hint_step;
	int list_x;
	int list_w;
	int list_top;
	int list_bottom;
	int line_h;
	int box_x = margin_x;
	int box_w = screen_w - margin_x * 2;
	float hint_alpha = show_help ? 1.0f : 0.0f;
	Color hint_color = COL_RED;
	int i;
	int text_y;

	(void)layout_metrics(show_help, &list_x, &list_w, &list_top, &list_bottom,
		&line_h, &hint_y, &hint_h, &hint_step);
	glyph_center(&title_glyph, 0.0f, (float)screen_w, (float)y);
	y += 8 * title_scale + 8;
	glyph_center(&author_glyph, 0.0f, (float)screen_w, (float)y);
	glyph_move(&index_glyph,
		(float)screen_w - (float)margin_x - glyph_nominal(&index_glyph) - 2.0f, (float)y);
	y += 8 * scale + 6;
	glyph_center(&mode_glyph, 0.0f, (float)screen_w, (float)y);

	rect_move(hint_fill, (float)box_x, (float)hint_y, (float)box_w, (float)hint_h);
	rect_alpha(hint_fill, hint_alpha);
	frame_place(&frames[4], (float)box_x, (float)hint_y, (float)box_w, (float)hint_h);
	frame_place(&frames[5], (float)(box_x + 3), (float)(hint_y + 3),
		(float)(box_w - 6), (float)(hint_h - 6));
	frame_alpha(&frames[4], hint_alpha);
	frame_alpha(&frames[5], hint_alpha);
	hint_color.a = hint_alpha;
	text_y = hint_y + 8;
	for(i = 0; i < 4; i++) {
		glyph_center(&hint_glyph[i], (float)box_x, (float)box_w, (float)text_y);
		glyph_tint(&hint_glyph[i], hint_color);
		text_y += hint_step;
	}

	frame_place(&frames[0], (float)(margin_x / 2), (float)(margin_y / 2),
		(float)(screen_w - margin_x), (float)(screen_h - margin_y));
	frame_place(&frames[1], (float)(margin_x / 2 + 3), (float)(margin_y / 2 + 3),
		(float)(screen_w - margin_x - 6), (float)(screen_h - margin_y - 6));
	frame_place(&frames[2], (float)(list_x - 6), (float)(list_top - 6),
		(float)(list_w + 12), (float)(list_bottom - list_top + 8));
	frame_place(&frames[3], (float)(list_x - 4), (float)(list_top - 4),
		(float)(list_w + 8), (float)(list_bottom - list_top + 4));
	frame_alpha(&frames[0], 1.0f);
	frame_alpha(&frames[1], 1.0f);
	frame_alpha(&frames[2], 1.0f);
	frame_alpha(&frames[3], 1.0f);
	glyph_tint(&title_glyph, COL_GREEN);
	glyph_tint(&author_glyph, COL_GREEN);
	glyph_tint(&version_glyph, COL_GREEN);
	glyph_tint(&index_glyph, COL_GREEN);
	glyph_tint(&mode_glyph, COL_DIM);
	{
		int gap_top = list_bottom + 2;
		int gap_bot = screen_h - margin_y / 2 - 4;
		int text_h = version_glyph.vis_h > 0 ? version_glyph.vis_h : 8 * scale;
		int room = gap_bot - gap_top;
		int text_y = gap_top;

		if(room > text_h) {
			text_y = gap_top + (room - text_h) / 2;
		}
		glyph_center(&version_glyph, 0.0f, (float)screen_w, (float)text_y);
	}
}

static void rect_tint(Rectangle *rect, Color color) {
	if(rect) {
		TSU_DrawableSetTint((Drawable *)rect, &color);
	}
}

static void frame_tint(frame_t *frame, Color color) {
	int i;

	if(!frame) {
		return;
	}
	for(i = 0; i < 4; i++) {
		rect_tint(frame->edge[i], color);
	}
}

static void box_place(Rectangle **edge, float x, float y, float side) {
	rect_move(edge[0], x, y, side, 1.0f);
	rect_move(edge[1], x, y + side - 1.0f, side, 1.0f);
	rect_move(edge[2], x, y, 1.0f, side);
	rect_move(edge[3], x + side - 1.0f, y, 1.0f, side);
}

static void hide_options(void) {
	int i;

	frame_alpha(&option_frames[0], 0.0f);
	frame_alpha(&option_frames[1], 0.0f);
	rect_alpha(option_fill, 0.0f);
	rect_alpha(option_bar, 0.0f);
	rect_alpha(check_mark, 0.0f);
	for(i = 0; i < 4; i++) {
		rect_alpha(check_edge[i], 0.0f);
	}
	glyph_tint(&option_title, COL_CLEAR);
	glyph_tint(&option_music, COL_CLEAR);
	glyph_tint(&option_exit, COL_CLEAR);
	glyph_tint(&option_resume, COL_CLEAR);
	glyph_tint(&option_hint[0], COL_CLEAR);
	glyph_tint(&option_hint[1], COL_CLEAR);
}

static void hide_saver(void) {
	frame_alpha(&saver_frames[0], 0.0f);
	frame_alpha(&saver_frames[1], 0.0f);
	rect_alpha(saver_dim, 0.0f);
	rect_alpha(saver_fill, 0.0f);
	glyph_tint(&saver_title, COL_CLEAR);
	glyph_tint(&saver_version, COL_CLEAR);
	saver_alive = 0;
}

static void show_options(int sel, int music_off) {
	int scale = ui_scale();
	int list_scale = (screen_h >= 400 && screen_w >= 600) ? 3 : scale;
	int row_h = 8 * list_scale + 8;
	int title_h = option_title.vis_h > 0 ? option_title.vis_h : 8 * list_scale;
	int hint_step = 8 * scale + 4;
	int box = 8 * list_scale;
	int pad = 20;
	int text_w = option_music.vis_w + 12 + box;
	int width;
	int height;
	int x;
	int y;
	int row_y;
	int i;
	Color mark = COL_GREEN;
	const float text_z = 36.0f;

	if(sel < 0) {
		sel = 0;
	}
	if(sel > 2) {
		sel = 2;
	}
	if(option_title.vis_w > text_w) {
		text_w = option_title.vis_w;
	}
	if(option_exit.vis_w > text_w) {
		text_w = option_exit.vis_w;
	}
	if(option_resume.vis_w > text_w) {
		text_w = option_resume.vis_w;
	}
	if(option_hint[0].vis_w > text_w) {
		text_w = option_hint[0].vis_w;
	}
	if(option_hint[1].vis_w > text_w) {
		text_w = option_hint[1].vis_w;
	}
	width = text_w + pad * 2;
	height = 16 + title_h + 12 + row_h * 3 + 12 + hint_step * 2 + 16;
	if(width > screen_w - 16) {
		width = screen_w - 16;
	}
	if(height > screen_h - 16) {
		height = screen_h - 16;
	}
	x = (screen_w - width) / 2;
	y = (screen_h - height) / 2;
	rect_move(option_fill, (float)x, (float)y, (float)width, (float)height);
	rect_alpha(option_fill, 1.0f);
	frame_place(&option_frames[0], (float)x, (float)y, (float)width, (float)height);
	frame_place(&option_frames[1], (float)(x + 3), (float)(y + 3),
		(float)(width - 6), (float)(height - 6));
	frame_alpha(&option_frames[0], 1.0f);
	frame_alpha(&option_frames[1], 1.0f);
	glyph_center_z(&option_title, (float)x, (float)width, (float)(y + 12), text_z);
	glyph_tint(&option_title, COL_PAPER);

	row_y = y + 12 + title_h + 12;
	rect_move(option_bar, (float)(x + 10), (float)row_y + (float)sel * (float)row_h,
		(float)(width - 20), (float)(row_h - 4));
	rect_alpha(option_bar, 1.0f);
	if(sel == 0) {
		mark = COL_INK;
	}
	{
		int group = box + 12 + option_music.vis_w;
		int cx = x + (width - group) / 2;
		int cy = row_y + (row_h - box) / 2;
		int i_edge;

		box_place(check_edge, (float)cx, (float)cy, (float)box);
		for(i_edge = 0; i_edge < 4; i_edge++) {
			rect_alpha(check_edge[i_edge], 1.0f);
			rect_tint(check_edge[i_edge], mark);
		}
		if(music_off && box > 10) {
			rect_move(check_mark, (float)(cx + 5), (float)(cy + 5),
				(float)(box - 10), (float)(box - 10));
			rect_alpha(check_mark, 1.0f);
			rect_tint(check_mark, mark);
		}
		else {
			rect_alpha(check_mark, 0.0f);
		}
		glyph_move_z(&option_music, (float)(cx + box + 12), (float)(row_y + 2), text_z);
		glyph_tint(&option_music, mark);
	}
	glyph_center_z(&option_exit, (float)x, (float)width, (float)(row_y + row_h + 2), text_z);
	glyph_tint(&option_exit, sel == 1 ? COL_INK : COL_GREEN);
	glyph_center_z(&option_resume, (float)x, (float)width,
		(float)(row_y + row_h * 2 + 2), text_z);
	glyph_tint(&option_resume, sel == 2 ? COL_INK : COL_GREEN);
	row_y += row_h * 3 + 8;
	for(i = 0; i < 2; i++) {
		glyph_center_z(&option_hint[i], (float)x, (float)width, (float)row_y, text_z);
		glyph_tint(&option_hint[i], COL_DIM);
		row_y += hint_step;
	}
}

static const Color SAVER_HUE[6] = {
	{1.0f, 1.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 1.0f, 0.0f},
	{1.0f, 0.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 0.0f},
	{1.0f, 0.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 0.0f, 1.0f}
};

static int saver_crawl(int *pos, int *vel, int *acc, int limit) {
	int hit = 0;

	if(limit <= 0) {
		*pos = 0;
		*vel = 0;
		*acc = 0;
		return 0;
	}
	*acc += *vel;
	while(*acc >= 3) {
		(*pos)++;
		*acc -= 3;
	}
	while(*acc <= -3) {
		(*pos)--;
		*acc += 3;
	}
	if(*pos < 0) {
		*pos = 0;
		*vel = -*vel;
		*acc = 0;
		hit = 1;
	}
	if(*pos > limit) {
		*pos = limit;
		*vel = -*vel;
		*acc = 0;
		hit = 1;
	}
	return hit;
}

static void show_saver(void) {
	int pad_x = 16;
	int pad_y = 12;
	int gap = 8;
	int title_h = saver_title.vis_h > 0 ? saver_title.vis_h : 32;
	int ver_h = saver_version.vis_h > 0 ? saver_version.vis_h : 16;
	int content = saver_title.vis_w;
	int box_w;
	int box_h;
	int block;
	int text_y;
	int hit = 0;
	Color hue;
	const float text_z = 44.0f;

	if(saver_version.vis_w > content) {
		content = saver_version.vis_w;
	}
	block = title_h + gap + ver_h;
	box_w = content + pad_x * 2;
	box_h = block + pad_y * 2;
	if(box_w > screen_w - 4) {
		box_w = screen_w - 4;
	}
	if(box_h > screen_h - 4) {
		box_h = screen_h - 4;
	}
	if(!saver_alive) {
		saver_alive = 1;
		saver_vx = 2;
		saver_vy = 2;
		saver_ax = 0;
		saver_ay = 0;
		saver_hue = 0;
		saver_x = (screen_w - box_w) / 2;
		saver_y = (screen_h - box_h) / 2;
	}
	hit = saver_crawl(&saver_x, &saver_vx, &saver_ax, screen_w - box_w);
	if(saver_crawl(&saver_y, &saver_vy, &saver_ay, screen_h - box_h)) {
		hit = 1;
	}
	if(hit) {
		saver_hue = (saver_hue + 1) % 6;
	}
	hue = SAVER_HUE[saver_hue];
	rect_move(saver_dim, 0.0f, 0.0f, (float)screen_w, (float)screen_h);
	rect_alpha(saver_dim, 0.72f);
	rect_move(saver_fill, (float)saver_x, (float)saver_y, (float)box_w, (float)box_h);
	rect_alpha(saver_fill, 1.0f);
	frame_place(&saver_frames[0], (float)saver_x, (float)saver_y, (float)box_w, (float)box_h);
	frame_place(&saver_frames[1], (float)(saver_x + 3), (float)(saver_y + 3),
		(float)(box_w - 6), (float)(box_h - 6));
	frame_tint(&saver_frames[0], hue);
	frame_tint(&saver_frames[1], hue);
	frame_alpha(&saver_frames[0], 1.0f);
	frame_alpha(&saver_frames[1], 1.0f);
	text_y = saver_y + (box_h - block) / 2;
	glyph_center_z(&saver_title, (float)saver_x, (float)box_w, (float)text_y, text_z);
	glyph_center_z(&saver_version, (float)saver_x, (float)box_w,
		(float)(text_y + title_h + gap), text_z);
	glyph_tint(&saver_title, hue);
	glyph_tint(&saver_version, hue);
}

static void hide_menu(void) {
	int i;

	for(i = 0; i < FRAME_COUNT; i++) {
		frame_alpha(&frames[i], 0.0f);
	}
	frame_alpha(&scroll_frames[0], 0.0f);
	frame_alpha(&scroll_frames[1], 0.0f);
	hide_options();
	hide_saver();
	rect_alpha(hint_fill, 0.0f);
	rect_alpha(select_bar, 0.0f);
	glyph_tint(&title_glyph, COL_CLEAR);
	glyph_tint(&author_glyph, COL_CLEAR);
	glyph_tint(&version_glyph, COL_CLEAR);
	glyph_tint(&index_glyph, COL_CLEAR);
	glyph_tint(&mode_glyph, COL_CLEAR);
	glyph_tint(&up_glyph, COL_CLEAR);
	glyph_tint(&down_glyph, COL_CLEAR);
	rect_alpha(list_track, 0.0f);
	rect_alpha(list_thumb, 0.0f);
	for(i = 0; i < 4; i++) {
		glyph_tint(&hint_glyph[i], COL_CLEAR);
	}
	for(i = 0; i < CRT_PATTERN_MAX; i++) {
		glyph_tint(&name_glyph[i], COL_CLEAR);
	}
}

static int bind_chart(fb_t *fb) {
	return fb_bind(fb, chart_pix, screen_w, screen_h, CHART_W, 3, 2, 3, 11, 5, 0);
}

static void show_chart_banner(void) {
	if(!chart_on && chart_banner && view_app && view_app->tsunami) {
		TSU_AppSubAddBanner(view_app->tsunami, chart_banner);
		chart_on = 1;
	}
}

static void hide_chart_banner(void) {
	if(chart_on && chart_banner && view_app && view_app->tsunami) {
		TSU_AppSubRemoveBanner(view_app->tsunami, chart_banner);
		chart_on = 0;
	}
}

static void banner_visible(Banner *banner, int *flag, int on) {
	if(!banner || !flag || !view_app || !view_app->tsunami) {
		return;
	}
	if(on && !*flag) {
		TSU_AppSubAddBanner(view_app->tsunami, banner);
		*flag = 1;
	}
	else if(!on && *flag) {
		TSU_AppSubRemoveBanner(view_app->tsunami, banner);
		*flag = 0;
	}
}

static void rect_visible(Rectangle *rect, int on) {
	if(!rect || !view_app || !view_app->tsunami) {
		return;
	}
	if(on) {
		TSU_AppSubAddRectangle(view_app->tsunami, rect);
	}
	else {
		TSU_AppSubRemoveRectangle(view_app->tsunami, rect);
	}
}

static void destroy_layer(Banner **banner, Texture **tex) {
	if(banner && *banner) {
		if(view_app && view_app->tsunami) {
			TSU_AppSubRemoveBanner(view_app->tsunami, *banner);
		}
		TSU_BannerDestroyAll(banner);
		if(tex) {
			*tex = NULL;
		}
		return;
	}
	if(tex && *tex) {
		TSU_TextureDestroy(tex);
	}
}

static void hide_scroll(void) {
	int i;

	for(i = 0; i < STRIP_COUNT; i++) {
		banner_visible(strip_layer[i].banner, &strip_layer[i].on, 0);
	}
	for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
		banner_visible(cloud_ban[i], &cloud_on[i], 0);
		banner_visible(cloud_wrap[i], &cloud_wrap_on[i], 0);
	}
	banner_visible(floor_banner, &floor_on, 0);
	banner_visible(vert_banner, &vert_on, 0);
}

static void hide_grid(void) {
	int i;

	if(!grid_on) {
		return;
	}
	for(i = 0; i < GRID_LINES; i++) {
		rect_visible(grid_v[i], 0);
		rect_visible(grid_h[i], 0);
	}
	for(i = 0; i < 4; i++) {
		rect_visible(grid_border[i], 0);
	}
	rect_visible(grid_cross[0], 0);
	rect_visible(grid_cross[1], 0);
	grid_on = 0;
}

static void hide_shadow(void) {
	int i;

	if(!shadow_on) {
		return;
	}
	rect_visible(shadow_fill, 0);
	rect_visible(shadow_box, 0);
	for(i = 0; i < 4; i++) {
		rect_visible(shadow_edge[i], 0);
	}
	shadow_on = 0;
}

static void hide_help(void) {
	banner_visible(help_banner, &help_on, 0);
}

static int grid_create(void) {
	int i;

	if(grid_made) {
		return grid_v[0] != NULL && grid_border[0] != NULL && grid_cross[0] != NULL;
	}
	grid_made = 1;
	for(i = 0; i < GRID_LINES; i++) {
		grid_v[i] = make_rect_off(PVR_LIST_OP_POLY, COL_WHITE, 4.0f);
		grid_h[i] = make_rect_off(PVR_LIST_OP_POLY, COL_WHITE, 4.0f);
	}
	for(i = 0; i < 4; i++) {
		grid_border[i] = make_rect_off(PVR_LIST_OP_POLY, COL_PURE, 5.0f);
	}
	grid_cross[0] = make_rect_off(PVR_LIST_OP_POLY, COL_PURE, 6.0f);
	grid_cross[1] = make_rect_off(PVR_LIST_OP_POLY, COL_PURE, 6.0f);
	return grid_v[0] != NULL && grid_h[0] != NULL && grid_border[0] != NULL && grid_cross[0] != NULL;
}

static void show_grid_lines(void) {
	int i;

	if(grid_on) {
		return;
	}
	for(i = 0; i < GRID_LINES; i++) {
		rect_visible(grid_v[i], 1);
		rect_visible(grid_h[i], 1);
	}
	for(i = 0; i < 4; i++) {
		rect_visible(grid_border[i], 1);
	}
	rect_visible(grid_cross[0], 1);
	rect_visible(grid_cross[1], 1);
	grid_on = 1;
}

static int show_grid_gpu(unsigned frame, int variant, int param) {
	int step = 32;
	int offset = 0;
	int i;
	int x;
	int y;
	float width = (float)screen_w;
	float height = (float)screen_h;

	if(!grid_create()) {
		return 0;
	}
	hide_chart_banner();
	hide_shadow();
	hide_scroll();
	pattern_grid_layout(frame, param, &step, &offset);
	if(step < 1) {
		step = 32;
	}
	i = 0;
	if(variant != 1) {
		for(x = -step; x < screen_w + step && i < GRID_LINES; x += step) {
			rect_move(grid_v[i], (float)(x + offset), 0.0f, 1.0f, height);
			i++;
		}
	}
	for(; i < GRID_LINES; i++) {
		rect_move(grid_v[i], -8.0f, -8.0f, 1.0f, 1.0f);
	}
	i = 0;
	if(variant != 0) {
		for(y = -step; y < screen_h + step && i < GRID_LINES; y += step) {
			rect_move(grid_h[i], 0.0f, (float)(y + offset), width, 1.0f);
			i++;
		}
	}
	for(; i < GRID_LINES; i++) {
		rect_move(grid_h[i], -8.0f, -8.0f, 1.0f, 1.0f);
	}
	rect_move(grid_border[0], 0.0f, 0.0f, width, 1.0f);
	rect_move(grid_border[1], 0.0f, height - 1.0f, width, 1.0f);
	rect_move(grid_border[2], 0.0f, 0.0f, 1.0f, height);
	rect_move(grid_border[3], width - 1.0f, 0.0f, 1.0f, height);
	rect_move(grid_cross[0], width * 0.5f - 16.0f, height * 0.5f, 32.0f, 1.0f);
	rect_move(grid_cross[1], width * 0.5f, height * 0.5f - 16.0f, 1.0f, 32.0f);
	show_grid_lines();
	return 1;
}

static int shadow_create(void) {
	int i;

	if(shadow_made) {
		return shadow_fill != NULL && shadow_box != NULL && shadow_edge[0] != NULL;
	}
	shadow_made = 1;
	shadow_fill = make_rect_off(PVR_LIST_OP_POLY, COL_BLACK, 6.0f);
	shadow_box = make_rect_off(PVR_LIST_OP_POLY, COL_WHITE, 7.0f);
	for(i = 0; i < 4; i++) {
		shadow_edge[i] = make_rect_off(PVR_LIST_OP_POLY, COL_BLACK, 8.0f);
	}
	return shadow_fill != NULL && shadow_box != NULL && shadow_edge[0] != NULL;
}

static void show_shadow_box(void) {
	int i;

	if(shadow_on) {
		return;
	}
	rect_visible(shadow_fill, 1);
	rect_visible(shadow_box, 1);
	for(i = 0; i < 4; i++) {
		rect_visible(shadow_edge[i], 1);
	}
	shadow_on = 1;
}

static int show_shadow_gpu(int variant, unsigned frame, int param) {
	fb_t fb;
	int x;
	int y;
	int bw;
	int bh;

	if(!chart_tex || !chart_banner || !shadow_create() || !bind_chart(&fb)) {
		return 0;
	}
	hide_grid();
	hide_scroll();
	show_chart_banner();
	if(chart_owner != 2 || variant != shadow_variant) {
		pattern_shadow_background(&fb, variant);
		crt_linear_upload(chart_tex, chart_pix, screen_h);
		chart_owner = 2;
		shadow_variant = variant;
	}
	pattern_shadow_box(frame, param, screen_w, screen_h, &x, &y, &bw, &bh);
	if(bw < 1) {
		bw = 1;
	}
	if(bh < 1) {
		bh = 1;
	}
	rect_move(shadow_fill, (float)(x + 10), (float)(y + 10), (float)bw, (float)bh);
	rect_move(shadow_box, (float)x, (float)y, (float)bw, (float)bh);
	rect_move(shadow_edge[0], (float)x, (float)y, (float)bw, 1.0f);
	rect_move(shadow_edge[1], (float)x, (float)(y + bh - 1), (float)bw, 1.0f);
	rect_move(shadow_edge[2], (float)x, (float)y, 1.0f, (float)bh);
	rect_move(shadow_edge[3], (float)(x + bw - 1), (float)y, 1.0f, (float)bh);
	show_shadow_box();
	return 1;
}

static uint16_t argb_of(uint16_t pix) {
	if(pix == 0) {
		return 0;
	}
	return (uint16_t)(0x8000
		| ((pix & 0xF800) >> 1)
		| ((pix & 0x07C0) >> 1)
		| (pix & 0x001F));
}

static void stage_clear_row(uint16_t *row, int from, int stride) {
	int x;

	for(x = from; x < stride; x++) {
		row[x] = 0;
	}
}

static void stage_wrap(const uint16_t *src, int sw, int sh, int stride, int extra, int keyed) {
	int y;

	if(!src || extra < 0 || sw + extra > stride) {
		return;
	}
	for(y = 0; y < sh; y++) {
		uint16_t *drow = chart_pix + y * stride;
		const uint16_t *srow = src + y * sw;
		int x;

		for(x = 0; x < sw; x++) {
			uint16_t pix = srow[x];

			drow[x] = keyed ? argb_of(pix) : pix;
		}
		for(x = 0; x < extra; x++) {
			drow[sw + x] = drow[x];
		}
		stage_clear_row(drow, sw + extra, stride);
	}
}

static void stage_tile(const uint16_t *src, int sw, int sh, int stride, int rows) {
	int y;

	if(!src || sh < 1 || sw > stride) {
		return;
	}
	for(y = 0; y < rows; y++) {
		uint16_t *drow = chart_pix + y * stride;
		const uint16_t *srow = src + (y % sh) * sw;
		int x;

		for(x = 0; x < sw; x++) {
			drow[x] = srow[x];
		}
		stage_clear_row(drow, sw, stride);
	}
}

static int scroll_geometry(const scroll_pose_t *pose) {
	int scale;
	int extra_w;
	int extra_h;
	int back_w;
	int vert_h;

	if(!pose || pose->scale < 1 || pose->back_src_w < 1 || pose->vert_src_h < 1) {
		return 0;
	}
	if(pose->floor_src_w != pose->back_src_w || pose->floor_src_h != pose->back_src_h) {
		return 0;
	}
	scale = pose->scale;
	extra_w = (screen_w + scale - 1) / scale;
	extra_h = (screen_h + scale - 1) / scale;
	back_w = pose->back_src_w + extra_w;
	vert_h = pose->vert_src_h + extra_h;
	if(extra_w > pose->back_src_w || back_w > SCROLL_TEX_W || pose->back_src_h > SCROLL_TEX_H) {
		return 0;
	}
	if(pose->vert_src_w > VERT_TEX || vert_h > VERT_TEX) {
		return 0;
	}
	geom_scale = scale;
	geom_back_w = back_w;
	geom_back_h = pose->back_src_h * scale;
	geom_y0 = (screen_h - geom_back_h) / 2;
	if(geom_y0 < 0) {
		geom_y0 = 0;
	}
	geom_vert_w = pose->vert_src_w * scale;
	geom_vert_h = vert_h * scale;
	geom_x0 = (screen_w - geom_vert_w) / 2;
	scroll_geom = 1;
	return 1;
}

static void scroll_place_h(Banner *banner, int scroll, float z, int crop_y, int crop_h) {
	int step;
	int src0;
	float left;
	float width;
	float top;
	float height;
	float u0;
	float u1;
	float v0;
	float v1;

	if(!banner || geom_scale < 1 || crop_h < 1) {
		return;
	}
	if(scroll < 0) {
		scroll = 0;
	}
	step = scroll % geom_scale;
	src0 = scroll / geom_scale;
	left = (float)(-step);
	width = (float)(screen_w + step);
	top = (float)(geom_y0 + crop_y * geom_scale);
	height = (float)(crop_h * geom_scale);
	u0 = (float)src0 / (float)SCROLL_TEX_W;
	u1 = ((float)src0 + width / (float)geom_scale) / (float)SCROLL_TEX_W;
	v0 = (float)crop_y / (float)SCROLL_TEX_H;
	v1 = (float)(crop_y + crop_h) / (float)SCROLL_TEX_H;
	TSU_BannerSetSize(banner, width, height);
	banner_window(banner, u0, v0, u1, v1);
	banner_place(banner, left, top, width, height, z);
}

static void scroll_place_v(Banner *banner, int scroll, float z) {
	int step;
	int src0;
	float top;
	float height;
	float width;
	float u1;
	float v0;
	float v1;

	if(!banner || geom_scale < 1) {
		return;
	}
	if(scroll < 0) {
		scroll = 0;
	}
	step = scroll % geom_scale;
	src0 = scroll / geom_scale;
	top = (float)(-step);
	height = (float)(screen_h + step);
	width = (float)geom_vert_w;
	u1 = (float)SCROLL_VERT_W / (float)VERT_TEX;
	v0 = (float)src0 / (float)VERT_TEX;
	v1 = ((float)src0 + height / (float)geom_scale) / (float)VERT_TEX;
	TSU_BannerSetSize(banner, width, height);
	banner_window(banner, 0.0f, v0, u1, v1);
	banner_place(banner, (float)geom_x0, top, width, height, z);
}

static uint16_t pack_rgb(fb_t *fb, int r, int g, int b, int argb) {
	uint16_t pix = fb_rgb(fb, r, g, b);

	if(!argb) {
		return pix;
	}
	if(pix == 0) {
		pix = 1;
	}
	return argb_of(pix);
}

static void scroll_place_strip(Banner *banner, int scroll, float z, int crop_y, int crop_h, int tex_h) {
	int step;
	int src0;
	float left;
	float width;
	float top;
	float height;
	float u0;
	float u1;
	float v0;
	float v1;

	if(!banner || geom_scale < 1 || crop_h < 1 || tex_h < 1) {
		return;
	}
	if(scroll < 0) {
		scroll = 0;
	}
	step = scroll % geom_scale;
	src0 = scroll / geom_scale;
	left = (float)(-step);
	width = (float)(screen_w + step);
	top = (float)(geom_y0 + crop_y * geom_scale);
	height = (float)(crop_h * geom_scale);
	u0 = (float)src0 / (float)SCROLL_TEX_W;
	u1 = ((float)src0 + width / (float)geom_scale) / (float)SCROLL_TEX_W;
	v0 = (float)crop_y / (float)tex_h;
	v1 = (float)(crop_y + crop_h) / (float)tex_h;
	TSU_BannerSetSize(banner, width, height);
	banner_window(banner, u0, v0, u1, v1);
	banner_place(banner, left, top, width, height, z);
}

static int fall_column(int x) {
	if(x >= SCROLL_FALL_X0 && x < SCROLL_FALL_X1) {
		return 1;
	}
	if(x >= SCROLL_FALL2_X0 && x < SCROLL_FALL2_X1) {
		return 1;
	}
	return 0;
}

static int water_rgb(int r, int g, int b) {
	return b > 145 && g > 140 && b + 15 >= r && r > 120;
}

static int pix_magenta(int r, int g, int b) {
	return r == 255 && g == 0 && b == 255;
}

static void paint_span(fb_t *fb, const unsigned char *rgb, int sw, int y0, int y1,
	int keyed, int argb, int shift, int extra_w) {
	int y;

	if(!rgb || sw < 1 || y1 <= y0 || sw + extra_w > SCROLL_TEX_W) {
		return;
	}
	for(y = y0; y < y1; y++) {
		uint16_t *row = chart_pix + (y - y0) * SCROLL_TEX_W;
		int x;

		for(x = 0; x < sw; x++) {
			int sy = y;
			const unsigned char *pix;

			if(shift && keyed && fall_column(x) &&
				y >= SCROLL_SHAFT_Y0 && y < SCROLL_SHAFT_Y1) {
				const unsigned char *dest = rgb + (y * sw + x) * 3;

				if(water_rgb(dest[0], dest[1], dest[2])) {
					int alt = scroll_shaft_row(y, shift);
					const unsigned char *moved = rgb + (alt * sw + x) * 3;

					if(water_rgb(moved[0], moved[1], moved[2])) {
						sy = alt;
					}
				}
			}
			pix = rgb + (sy * sw + x) * 3;
			if(keyed && pix_magenta(pix[0], pix[1], pix[2])) {
				row[x] = 0;
			}
			else {
				row[x] = pack_rgb(fb, pix[0], pix[1], pix[2], argb);
			}
		}
		for(x = 0; x < extra_w; x++) {
			row[sw + x] = row[x];
		}
		stage_clear_row(row, sw + extra_w, SCROLL_TEX_W);
	}
}

static void destroy_clouds(void) {
	int i;

	for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
		banner_visible(cloud_ban[i], &cloud_on[i], 0);
		banner_visible(cloud_wrap[i], &cloud_wrap_on[i], 0);
		if(cloud_ban[i]) {
			TSU_BannerDestroy(&cloud_ban[i]);
		}
		if(cloud_wrap[i]) {
			TSU_BannerDestroy(&cloud_wrap[i]);
		}
	}
	if(cloud_tex) {
		TSU_TextureDestroy(&cloud_tex);
	}
	clouds_ready = 0;
}

static void destroy_strips(void) {
	int i;

	for(i = 0; i < STRIP_COUNT; i++) {
		banner_visible(strip_layer[i].banner, &strip_layer[i].on, 0);
		destroy_layer(&strip_layer[i].banner, &strip_layer[i].tex);
		strip_layer[i].ready = 0;
		strip_layer[i].src_h = 0;
		strip_layer[i].crop_h = 0;
	}
	shaft_saved = 0;
	near_phase = -1;
}

static int make_strip(fb_t *fb, int index, const unsigned char *rgb, int sw, int sh,
	int src_y, int crop_y, int crop_h, int argb, int extra_w) {
	strip_t *layer = &strip_layer[index];
	int tex_h;

	if(layer->ready) {
		return 1;
	}
	if(!rgb || sh < 1 || sw < 1) {
		return 1;
	}
	paint_span(fb, rgb, sw, 0, sh, argb, argb, 0, extra_w);
	tex_h = pot_size(sh);
	layer->tex = argb ? crt_canvas_linear_argb(SCROLL_TEX_W, tex_h)
		: crt_canvas_linear_rgb(SCROLL_TEX_W, tex_h);
	if(!layer->tex) {
		return 0;
	}
	crt_linear_clear(layer->tex);
	crt_linear_upload(layer->tex, chart_pix, sh);
	layer->banner = TSU_BannerCreate(argb ? PVR_LIST_TR_POLY : PVR_LIST_OP_POLY, layer->tex);
	if(!layer->banner) {
		TSU_TextureDestroy(&layer->tex);
		return 0;
	}
	layer->src_y = src_y;
	layer->src_h = sh;
	layer->crop_y = crop_y;
	layer->crop_h = crop_h;
	layer->tex_h = tex_h;
	layer->argb = argb;
	layer->ready = 1;
	return 1;
}

static void save_shaft(void) {
	memcpy(shaft_base, chart_pix + SCROLL_SHAFT_Y0 * SCROLL_TEX_W, sizeof(shaft_base));
	shaft_saved = 1;
}

static void patch_shaft_row(uint16_t *row, int y, int shift, int extra_w) {
	int local = y - SCROLL_SHAFT_Y0;
	int band;
	const int x0[2] = {SCROLL_FALL_X0, SCROLL_FALL2_X0};
	const int x1[2] = {SCROLL_FALL_X1, SCROLL_FALL2_X1};

	memcpy(row, shaft_base + local * SCROLL_TEX_W, SCROLL_TEX_W * sizeof(uint16_t));
	if(!shift) {
		return;
	}
	if(!scroll_near_rgb) {
		return;
	}
	for(band = 0; band < 2; band++) {
		int x;

		for(x = x0[band]; x < x1[band]; x++) {
			const unsigned char *dest = scroll_near_rgb + (y * SCROLL_BACK_W + x) * 3;
			int alt;
			const unsigned char *moved;

			if(x < 0 || x >= SCROLL_BACK_W || !water_rgb(dest[0], dest[1], dest[2])) {
				continue;
			}
			alt = scroll_shaft_row(y, shift);
			moved = scroll_near_rgb + (alt * SCROLL_BACK_W + x) * 3;
			if(!water_rgb(moved[0], moved[1], moved[2])) {
				continue;
			}
			row[x] = shaft_base[(alt - SCROLL_SHAFT_Y0) * SCROLL_TEX_W + x];
			if(extra_w > 0 && x < extra_w) {
				row[SCROLL_BACK_W + x] = row[x];
			}
		}
	}
}

static void repaint_falls(fb_t *fb, int phase, int extra_w) {
	strip_t *layer = &strip_layer[STRIP_NEAR];
	int y_begin = SCROLL_SHAFT_Y0;
	int y_end = SCROLL_SHAFT_Y1;
	int rows;
	int y;

	if(!layer->ready || layer->src_h < 1) {
		return;
	}
	if(y_begin < layer->src_y) {
		y_begin = layer->src_y;
	}
	if(y_end > layer->src_y + layer->src_h) {
		y_end = layer->src_y + layer->src_h;
	}
	if(y_end <= y_begin) {
		return;
	}
	if(!shaft_saved) {
		paint_span(fb, scroll_near_rgb, SCROLL_BACK_W, y_begin, y_end, 1, 1, phase, extra_w);
	}
	else {
		for(y = y_begin; y < y_end; y++) {
			patch_shaft_row(chart_pix + (y - y_begin) * SCROLL_TEX_W, y, phase, extra_w);
		}
	}
	rows = y_end - y_begin;
	crt_linear_upload_rows(layer->tex, chart_pix, y_begin - layer->src_y, rows);
}

static int build_clouds(fb_t *fb) {
	int i;
	int y;
	int x;

	if(clouds_ready) {
		return 1;
	}
	if(!scroll_cloud_rgb) {
		return 0;
	}
	for(y = 0; y < CLOUD_TEX_H; y++) {
		uint16_t *row = chart_pix + y * CLOUD_TEX_W;
		const unsigned char *src = scroll_cloud_rgb + y * CLOUD_TEX_W * 3;

		for(x = 0; x < CLOUD_TEX_W; x++) {
			int r = src[x * 3];
			int g = src[x * 3 + 1];
			int b = src[x * 3 + 2];

			if(pix_magenta(r, g, b)) {
				row[x] = 0;
			}
			else {
				row[x] = pack_rgb(fb, r, g, b, 1);
			}
		}
	}
	cloud_tex = crt_canvas_linear_argb(CLOUD_TEX_W, CLOUD_TEX_H);
	if(!cloud_tex) {
		clouds_ready = 1;
		return 0;
	}
	crt_linear_upload(cloud_tex, chart_pix, CLOUD_TEX_H);
	for(i = 0; i < SCROLL_CLOUD_COUNT; i++) {
		const scroll_cloud_def_t *cloud = &scroll_clouds[i];
		float u0;
		float v0;
		float u1;
		float v1;
		float z = 2.20f + (float)i * 0.012f;
		Vector pos;

		if(cloud->w < 1 || cloud->h < 1) {
			continue;
		}
		u0 = (float)cloud->ax / (float)CLOUD_TEX_W;
		v0 = (float)cloud->ay / (float)CLOUD_TEX_H;
		u1 = (float)(cloud->ax + cloud->w) / (float)CLOUD_TEX_W;
		v1 = (float)(cloud->ay + cloud->h) / (float)CLOUD_TEX_H;
		cloud_ban[i] = TSU_BannerCreate(PVR_LIST_TR_POLY, cloud_tex);
		cloud_wrap[i] = TSU_BannerCreate(PVR_LIST_TR_POLY, cloud_tex);
		if(!cloud_ban[i] || !cloud_wrap[i]) {
			if(cloud_ban[i]) {
				TSU_BannerDestroy(&cloud_ban[i]);
			}
			if(cloud_wrap[i]) {
				TSU_BannerDestroy(&cloud_wrap[i]);
			}
			continue;
		}
		banner_window(cloud_ban[i], u0, v0, u1, v1);
		banner_window(cloud_wrap[i], u0, v0, u1, v1);
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = z;
		pos.w = 1.0f;
		TSU_DrawableSetTranslate((Drawable *)cloud_ban[i], &pos);
		TSU_DrawableSetTranslate((Drawable *)cloud_wrap[i], &pos);
	}
	clouds_ready = 1;
	return 1;
}


static void show_cloud_at(Banner *banner, int *flag, int index, int left, float z) {
	int w;
	int h;
	float top;

	if(!banner || geom_scale < 1) {
		return;
	}
	w = scroll_clouds[index].w * geom_scale;
	h = scroll_clouds[index].h * geom_scale;
	if(left >= screen_w || left + w <= 0) {
		banner_visible(banner, flag, 0);
		return;
	}
	top = (float)(geom_y0 + scroll_clouds[index].y * geom_scale);
	TSU_BannerSetSize(banner, (float)w, (float)h);
	banner_place(banner, (float)left, top, (float)w, (float)h, z);
	banner_visible(banner, flag, 1);
}

static void place_clouds(const scroll_pose_t *pose) {
	int i;
	int span = SCROLL_BACK_W * geom_scale;
	int count = pose->cloud_count;

	if(count > SCROLL_CLOUD_COUNT) {
		count = SCROLL_CLOUD_COUNT;
	}
	for(i = 0; i < count; i++) {
		int base;
		float z = 2.30f + (float)i * 0.004f;

		if(!cloud_ban[i]) {
			continue;
		}
		base = scroll_clouds[i].x * geom_scale - pose->cloud_scroll[i];
		if(span > 0) {
			base %= span;
			if(base < 0) {
				base += span;
			}
		}
		show_cloud_at(cloud_ban[i], &cloud_on[i], i, base, z);
		show_cloud_at(cloud_wrap[i], &cloud_wrap_on[i], i, base - span, z);
	}
}

static int strip_scroll_of(const scroll_pose_t *pose, int index) {
	if(index == STRIP_SKY) {
		return pose->sky_scroll;
	}
	if(index == STRIP_MID) {
		return pose->mid_scroll;
	}
	return pose->near_scroll;
}

static int vert_ensure(void) {
	Vector center;

	if(vert_banner) {
		return 1;
	}
	vert_tex = crt_canvas_linear_rgb(VERT_TEX, VERT_TEX);
	if(!vert_tex) {
		return 0;
	}
	crt_linear_clear(vert_tex);
	vert_banner = TSU_BannerCreate(PVR_LIST_OP_POLY, vert_tex);
	if(!vert_banner) {
		TSU_TextureDestroy(&vert_tex);
		return 0;
	}
	center.x = 0.0f;
	center.y = 0.0f;
	center.z = 3.0f;
	center.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)vert_banner, &center);
	return 1;
}

static int scroll_ensure(void) {
	Vector center;

	if(scroll_ready) {
		return 1;
	}
	if(scroll_failed || !view_app || !view_app->tsunami) {
		return 0;
	}
	floor_tex = crt_canvas_linear_argb(SCROLL_TEX_W, SCROLL_TEX_H);
	if(!floor_tex) {
		scroll_failed = 1;
		return 0;
	}
	crt_linear_clear(floor_tex);
	floor_banner = TSU_BannerCreate(PVR_LIST_TR_POLY, floor_tex);
	if(!floor_banner) {
		scroll_failed = 1;
		destroy_layer(&floor_banner, &floor_tex);
		return 0;
	}
	center.x = 0.0f;
	center.y = 0.0f;
	center.z = 5.0f;
	center.w = 1.0f;
	TSU_DrawableSetTranslate((Drawable *)floor_banner, &center);
	scroll_ready = 1;
	return 1;
}

static int show_scroll_gpu(unsigned frame, int variant, int param) {
	fb_t fb;
	scroll_pose_t pose;
	int extra_w;
	int rows;
	int i;
	static const float strip_z[STRIP_COUNT] = {2.0f, 3.2f, 3.8f};

	if(!scroll_art_ensure()) {
		return 0;
	}
	if(!scroll_ensure() || !bind_chart(&fb)) {
		return 0;
	}
	pattern_scroll_pose(&fb, frame, variant, param, &pose);
	if(!scroll_geom && !scroll_geometry(&pose)) {
		return 0;
	}
	if(!scroll_geom) {
		return 0;
	}
	extra_w = geom_back_w - pose.back_src_w;
	if(!floor_built) {
		stage_wrap(pattern_scroll_floor(), pose.floor_src_w, pose.floor_src_h,
			SCROLL_TEX_W, extra_w, 1);
		crt_linear_upload(floor_tex, chart_pix, pose.floor_src_h);
		floor_built = 1;
	}
	if(!strip_layer[STRIP_SKY].ready) {
		if(!make_strip(&fb, STRIP_SKY, scroll_sky_rgb, SCROLL_BACK_W, SCROLL_BACK_H,
				0, 0, SCROLL_BACK_H, 0, extra_w)
			|| !strip_layer[STRIP_SKY].ready
			|| !make_strip(&fb, STRIP_MID, scroll_mid_rgb, SCROLL_MID_W, SCROLL_MID_H,
				SCROLL_MID_Y, SCROLL_MID_CROP_Y, SCROLL_MID_CROP_H, 1, extra_w)
			|| !strip_layer[STRIP_MID].ready
			|| !make_strip(&fb, STRIP_NEAR, scroll_near_rgb, SCROLL_BACK_W, SCROLL_BACK_H,
				0, SCROLL_NEAR_CROP_Y, SCROLL_NEAR_CROP_H, 1, extra_w)
			|| !strip_layer[STRIP_NEAR].ready) {
			destroy_strips();
			destroy_clouds();
			scroll_failed = 1;
			return 0;
		}
		save_shaft();
		build_clouds(&fb);
		near_phase = 0;
	}
	if(near_phase != pose.phase) {
		repaint_falls(&fb, pose.phase, extra_w);
		near_phase = pose.phase;
	}
	rows = pose.vert_src_h + (screen_h + geom_scale - 1) / geom_scale;
	hide_chart_banner();
	hide_grid();
	hide_shadow();
	if(pose.vertical) {
		hide_scroll();
		if(!vert_ensure()) {
			return 0;
		}
		if(!vert_built) {
			stage_tile(pattern_scroll_vert(), pose.vert_src_w, pose.vert_src_h, VERT_TEX, rows);
			crt_linear_upload(vert_tex, chart_pix, rows);
			vert_built = 1;
		}
		scroll_place_v(vert_banner, pose.vert_scroll, 3.0f);
		banner_visible(vert_banner, &vert_on, 1);
		return 1;
	}
	banner_visible(vert_banner, &vert_on, 0);
	for(i = 0; i < STRIP_COUNT; i++) {
		if(!strip_layer[i].banner) {
			continue;
		}
		scroll_place_strip(strip_layer[i].banner, strip_scroll_of(&pose, i), strip_z[i],
			strip_layer[i].crop_y, strip_layer[i].crop_h, strip_layer[i].tex_h);
		banner_visible(strip_layer[i].banner, &strip_layer[i].on, 1);
	}
	place_clouds(&pose);
	scroll_place_h(floor_banner, pose.floor_scroll, 5.0f, SCROLL_FLOOR_CROP_Y, SCROLL_FLOOR_CROP_H);
	banner_visible(floor_banner, &floor_on, 1);
	return 1;
}

static void show_soft(int pattern, int variant, int param, unsigned frame, int animating, int bake) {
	fb_t fb;
	int dirty;

	hide_grid();
	hide_shadow();
	hide_scroll();
	if(!chart_tex || !chart_banner || !view_app || !view_app->tsunami) {
		return;
	}
	show_chart_banner();
	dirty = chart_owner != 1 || pattern != soft_pattern || variant != soft_variant
		|| param != soft_param || bake != soft_bake || animating;
	if(!dirty) {
		return;
	}
	if(!bind_chart(&fb)) {
		return;
	}
	crt_compose_chart(&fb, frame, pattern, variant, param, bake);
	crt_linear_upload(chart_tex, chart_pix, screen_h);
	chart_owner = 1;
	soft_pattern = pattern;
	soft_variant = variant;
	soft_param = param;
	soft_bake = bake;
}

static int help_ensure(void) {
	Vector center;

	if(help_banner) {
		return 1;
	}
	if(help_failed || !view_app || !view_app->tsunami) {
		return 0;
	}
	help_tex = crt_canvas_linear_argb(CHART_W, CHART_H);
	if(!help_tex) {
		help_failed = 1;
		return 0;
	}
	crt_linear_clear(help_tex);
	help_banner = TSU_BannerCreate(PVR_LIST_TR_POLY, help_tex);
	if(!help_banner) {
		help_failed = 1;
		TSU_TextureDestroy(&help_tex);
		return 0;
	}
	center.x = (float)screen_w * 0.5f;
	center.y = (float)screen_h * 0.5f;
	center.z = 12.0f;
	center.w = 1.0f;
	TSU_BannerSetSize(help_banner, (float)screen_w, (float)screen_h);
	banner_uv(help_banner, CHART_W, CHART_H, screen_w, screen_h);
	TSU_DrawableSetTranslate((Drawable *)help_banner, &center);
	return 1;
}

static void sync_help(int pattern, int variant, int param, int show_help) {
	fb_t fb;
	uint16_t key;

	if(!help_ensure()) {
		return;
	}
	if(!show_help) {
		hide_help();
		return;
	}
	if(!help_valid || pattern != help_pattern || variant != help_variant || param != help_param) {
		if(!bind_chart(&fb)) {
			return;
		}
		key = crt_compose_help(&fb);
		crt_linear_upload_key(help_tex, chart_pix, screen_h, key);
		help_pattern = pattern;
		help_variant = variant;
		help_param = param;
		help_valid = 1;
	}
	banner_visible(help_banner, &help_on, 1);
}

static void show_chart(int pattern, int variant, int param, int show_help, unsigned frame) {
	int handled = 0;
	int gpu;

	help_ensure();
	gpu = !show_help || help_banner != NULL;
	if(pattern_is_scroll(pattern)) {
		scroll_art_ensure();
	}
	else {
		scroll_art_free();
	}

	if(gpu && pattern_is_scroll(pattern)) {
		handled = show_scroll_gpu(frame, variant, param);
	}
	else if(gpu && pattern_is_grid_scroll(pattern)) {
		handled = show_grid_gpu(frame, variant, param);
	}
	else if(gpu && pattern_is_shadow(pattern)) {
		handled = show_shadow_gpu(variant, frame, param);
	}
	if(!handled) {
		show_soft(pattern, variant, param, frame, pattern_animates(pattern),
			help_banner ? 0 : show_help);
	}
	sync_help(pattern, variant, param, show_help);
}

static void hide_chart(void) {
	hide_chart_banner();
	hide_help();
	hide_grid();
	hide_shadow();
	hide_scroll();
	scroll_art_free();
}

static int sync_index(int menu_sel, int count) {
	char line[16];

	snprintf(line, sizeof(line), "%d/%d", menu_sel + 1, count);
	if(strcmp(line, index_cached) == 0 && index_glyph.banner) {
		return 0;
	}
	if(!glyph_build(&index_glyph, line, ui_scale(), 0)) {
		return 0;
	}
	snprintf(index_cached, sizeof(index_cached), "%s", line);
	return 1;
}

static int sync_mode(const char *mode_line) {
	char line[80];

	if(!mode_line) {
		mode_line = "";
	}
	upper_copy(line, (int)sizeof(line), mode_line);
	if(line[0] == '\0') {
		return 0;
	}
	if(strcmp(line, mode_cached) == 0 && mode_glyph.banner) {
		return 0;
	}
	if(!glyph_build(&mode_glyph, line, ui_scale(), 0)) {
		return 0;
	}
	snprintf(mode_cached, sizeof(mode_cached), "%s", line);
	return 1;
}

static void build_names(void) {
	int count = pattern_count();
	int list_scale = (screen_h >= 400 && screen_w >= 600) ? 3 : ui_scale();
	int span = list_scale >= 3 ? 6 : 8;
	int i;

	if(count > CRT_PATTERN_MAX) {
		count = CRT_PATTERN_MAX;
	}
	for(i = 0; i < count; i++) {
		char name[80];

		upper_copy(name, (int)sizeof(name), pattern_name(i));
		glyph_build(&name_glyph[i], name, list_scale, span);
	}
	glyph_build_arrow(&up_glyph, list_scale, 0);
	glyph_build_arrow(&down_glyph, list_scale, 1);
	names_ready = 1;
}

static void place_names(int menu_sel, int scroll, int show_help) {
	int list_x;
	int list_w;
	int list_top;
	int list_bottom;
	int line_h;
	int hint_y;
	int hint_h;
	int hint_step;
	int list_scale = layout_metrics(show_help, &list_x, &list_w, &list_top, &list_bottom,
		&line_h, &hint_y, &hint_h, &hint_step);
	int count = pattern_count();
	int visible = (list_bottom - list_top) / line_h;
	int origin;
	int i;
	int bar_row = menu_sel;
	float bar_mid = 0.0f;
	float best = 100000.0f;
	const Vector *bar_pos;

	if(visible < 1) {
		visible = 1;
	}
	if(count > CRT_PATTERN_MAX) {
		count = CRT_PATTERN_MAX;
	}
	if(visible > count) {
		visible = count;
	}
	origin = list_origin(list_top, list_bottom, line_h, visible);
	if(select_bar && bar_h > 0) {
		bar_pos = TSU_DrawableGetTranslate((Drawable *)select_bar);
		if(bar_pos) {
			bar_mid = bar_pos->y - (float)bar_h * 0.5f;
		}
	}
	for(i = 0; i < count; i++) {
		int slot = i - scroll;
		int on = slot >= 0 && slot < visible;
		float row_top = (float)(origin + slot * line_h);
		float row_mid = row_top + (float)(line_h - 4) * 0.5f;
		float dist = row_mid - bar_mid;

		if(dist < 0.0f) {
			dist = -dist;
		}
		if(on && dist < best) {
			best = dist;
			bar_row = i;
		}
		if(!on) {
			glyph_tint(&name_glyph[i], COL_CLEAR);
			continue;
		}
		glyph_center(&name_glyph[i], (float)list_x, (float)list_w, row_top + 2.0f);
	}
	for(i = 0; i < count; i++) {
		int slot = i - scroll;

		if(slot < 0 || slot >= visible) {
			continue;
		}
		glyph_tint(&name_glyph[i], (i == bar_row && view_panel == CRT_PANEL_MENU) ? COL_INK : COL_GREEN);
	}
	{
		int arrow = scroll_arrow(list_scale);
		int gutter_w = scroll_gutter_w(list_scale);
		int gutter_x = list_x + list_w + scroll_gap();
		int inner_x = gutter_x + 4;
		int inner_w = gutter_w - 8;
		int ax = inner_x + (inner_w - arrow) / 2;
		int track_w = arrow - 2;
		int track_x;
		int track_top;
		int track_bot;
		int track_h;
		int thumb_h;
		int thumb_y;
		int max_scroll = count - visible;
		int up_on = scroll > 0;
		int down_on = scroll + visible < count;
		Color arrow_color = COL_GREEN;

		if(track_w < 4) {
			track_w = 4;
		}
		track_x = ax + (arrow - track_w) / 2;
		if(up_on) {
			glyph_move(&up_glyph, (float)ax, (float)(origin + 2));
			glyph_tint(&up_glyph, arrow_color);
		}
		else {
			glyph_tint(&up_glyph, COL_CLEAR);
		}
		if(down_on) {
			glyph_move(&down_glyph, (float)ax,
				(float)(origin + (visible - 1) * line_h + 2));
			glyph_tint(&down_glyph, arrow_color);
		}
		else {
			glyph_tint(&down_glyph, COL_CLEAR);
		}
		track_top = origin + 2 + arrow + 4;
		track_bot = origin + (visible - 1) * line_h - 2;
		track_h = track_bot - track_top;
		if(max_scroll < 1 || track_h < 12 || !list_track || !list_thumb) {
			frame_alpha(&scroll_frames[0], 0.0f);
			frame_alpha(&scroll_frames[1], 0.0f);
			rect_alpha(list_track, 0.0f);
			rect_alpha(list_thumb, 0.0f);
			return;
		}
		frame_place(&scroll_frames[0], (float)gutter_x, (float)(list_top - 6),
			(float)gutter_w, (float)(list_bottom - list_top + 8));
		frame_place(&scroll_frames[1], (float)(gutter_x + 3), (float)(list_top - 3),
			(float)(gutter_w - 6), (float)(list_bottom - list_top + 2));
		frame_alpha(&scroll_frames[0], 1.0f);
		frame_alpha(&scroll_frames[1], 1.0f);
		thumb_h = track_h * visible / count;
		if(thumb_h < arrow / 2) {
			thumb_h = arrow / 2;
		}
		if(thumb_h > track_h) {
			thumb_h = track_h;
		}
		thumb_y = track_top;
		if(max_scroll > 0) {
			thumb_y = track_top + (track_h - thumb_h) * scroll / max_scroll;
		}
		rect_move(list_track, (float)track_x, (float)track_top, (float)track_w, (float)track_h);
		rect_move(list_thumb, (float)track_x, (float)thumb_y, (float)track_w, (float)thumb_h);
		rect_alpha(list_track, 1.0f);
		rect_alpha(list_thumb, 1.0f);
	}
}

static void move_bar(int menu_sel, int scroll, int show_help, int snap) {
	int list_x;
	int list_w;
	int list_top;
	int list_bottom;
	int line_h;
	int hint_y;
	int hint_h;
	int hint_step;
	int slot;
	int visible;
	int origin;

	layout_metrics(show_help, &list_x, &list_w, &list_top, &list_bottom,
		&line_h, &hint_y, &hint_h, &hint_step);
	visible = (list_bottom - list_top) / line_h;
	if(visible < 1) {
		visible = 1;
	}
	if(visible > pattern_count()) {
		visible = pattern_count();
	}
	origin = list_origin(list_top, list_bottom, line_h, visible);
	slot = menu_sel - scroll;
	if(slot < 0) {
		slot = 0;
	}
	bar_go((float)list_x, (float)(origin + slot * line_h), (float)list_w,
		(float)(line_h - 4), snap);
}

static void build_static_text(void) {
	int scale = ui_scale();
	int list_scale = (screen_h >= 400 && screen_w >= 600) ? 3 : scale;
	int title_scale = list_scale >= 3 ? 4 : scale + 1;
	const char *hints[] = {
		"UP/DOWN OR LEVER SELECTS",
		"LEFT/RIGHT OR WHEEL PAGES",
		"GAS OR A OPENS   BRAKE OR B: OPTIONS, EXIT",
		"START OR Y HIDES TEXT"
	};
	int hint_span = 7;
	int hint_w = (int)strlen(hints[2]) * hint_span * scale;
	int i;

	glyph_build(&title_glyph, "CRT TEST", title_scale, 0);
	glyph_build(&author_glyph, "HEADHUNTER", scale, 0);
	{
		char version[40];
		int ver_scale = scale;

		snprintf(version, sizeof(version), "VERSION %d.%d (BUILD %d)",
			VER_MAJOR, VER_MINOR, VER_BUILD);
		if((int)strlen(version) * 8 * ver_scale + 2 > screen_w - 8 && ver_scale > 1) {
			ver_scale = 1;
		}
		glyph_build(&version_glyph, version, ver_scale, 0);
	}
	if(hint_w > screen_w - safe_x() * 2 - 16) {
		hint_span = 6;
	}
	for(i = 0; i < 4; i++) {
		glyph_build(&hint_glyph[i], hints[i], scale, hint_span);
	}
	glyph_build(&option_title, "OPTIONS", title_scale, 0);
	glyph_build(&option_music, "MUSIC OFF", list_scale, 0);
	glyph_build(&option_exit, "EXIT", list_scale, 0);
	glyph_build(&option_resume, "RESUME", list_scale, 0);
	glyph_build(&option_hint[0], "GAS OR A CONFIRMS", scale, 0);
	glyph_build(&option_hint[1], "BRAKE OR B RETURNS", scale, 0);
	glyph_build(&saver_title, "CRT TEST", title_scale, 0);
	{
		char version[40];

		snprintf(version, sizeof(version), "VERSION %d.%d (BUILD %d)",
			VER_MAJOR, VER_MINOR, VER_BUILD);
		glyph_build(&saver_version, version, scale, 0);
	}
	build_names();
}

static void current_mode(char *dst, int dst_len) {
	const char *cable = "video";
	const char *scan = "progressive";

	switch(vid_check_cable()) {
		case CT_VGA:
			cable = "VGA";
			break;
		case CT_RGB:
			cable = "RGB";
			break;
		case CT_COMPOSITE:
			cable = "composite";
			break;
		case CT_NONE:
			cable = "no cable";
			break;
		default:
			break;
	}
	switch(GetVideoMode()) {
		case DM_640x480_NTSC_IL:
		case DM_640x480_PAL_IL:
		case DM_256x256_PAL_IL:
		case DM_768x480_NTSC_IL:
		case DM_768x576_PAL_IL:
		case DM_768x480_PAL_IL:
			scan = "interlaced";
			break;
		default:
			break;
	}
	snprintf(dst, dst_len, "%dx%d  %s  %s", screen_w, screen_h, cable, scan);
}

void view_init(void *app_ptr) {
	App_t *app = (App_t *)app_ptr;
	Vector center;
	char mode[80];
	int scroll = 0;
	int i;

	if(ready) {
		return;
	}
	view_app = app;
	if(!app || !app->tsunami) {
		ds_printf("DS_ERROR: CRT Test: tsunami scene is missing\n");
		return;
	}
	screen_w = GetScreenWidth();
	screen_h = GetScreenHeight();
	if(screen_w < 16) {
		screen_w = 640;
	}
	if(screen_h < 16) {
		screen_h = 480;
	}
	if(screen_w > CHART_W) {
		screen_w = CHART_W;
	}
	if(screen_h > CHART_H) {
		screen_h = CHART_H;
	}

	chart_tex = crt_canvas_linear_rgb(CHART_W, CHART_H);
	if(chart_tex) {
		crt_linear_clear(chart_tex);
		chart_banner = TSU_BannerCreate(PVR_LIST_OP_POLY, chart_tex);
	}
	if(chart_banner) {
		center.x = (float)screen_w * 0.5f;
		center.y = (float)screen_h * 0.5f;
		center.z = 4.0f;
		center.w = 1.0f;
		TSU_BannerSetSize(chart_banner, (float)screen_w, (float)screen_h);
		banner_uv(chart_banner, CHART_W, CHART_H, screen_w, screen_h);
		TSU_DrawableSetTranslate((Drawable *)chart_banner, &center);
	}
	else {
		ds_printf("DS_ERROR: CRT Test: chart texture was not created\n");
	}

	build_static_text();
	current_mode(mode, (int)sizeof(mode));
	sync_mode(mode);
	sync_index(0, pattern_count());

	for(i = 0; i < 4; i++) {
		frame_make(&frames[i], COL_GREEN, 6.0f);
	}
	frame_make(&frames[4], COL_RED, 9.0f);
	frame_make(&frames[5], COL_RED, 9.0f);
	hint_fill = make_rect(PVR_LIST_TR_POLY, COL_BLACK, 8.0f);
	backdrop = make_rect(PVR_LIST_OP_POLY, COL_BLACK, 1.0f);
	select_bar = make_rect(PVR_LIST_TR_POLY, COL_PAPER, 15.0f);
	list_track = make_rect(PVR_LIST_TR_POLY, COL_DIM, 16.2f);
	list_thumb = make_rect(PVR_LIST_TR_POLY, COL_GREEN, 16.6f);
	rect_alpha(list_track, 0.0f);
	rect_alpha(list_thumb, 0.0f);
	frame_make(&scroll_frames[0], COL_GREEN, 17.0f);
	frame_make(&scroll_frames[1], COL_GREEN, 17.2f);
	frame_alpha(&scroll_frames[0], 0.0f);
	frame_alpha(&scroll_frames[1], 0.0f);
	option_fill = make_rect(PVR_LIST_TR_POLY, COL_BLACK, 32.0f);
	option_bar = make_rect(PVR_LIST_TR_POLY, COL_PAPER, 33.0f);
	frame_make(&option_frames[0], COL_PAPER, 33.4f);
	frame_make(&option_frames[1], COL_PAPER, 33.6f);
	frame_alpha(&option_frames[0], 0.0f);
	frame_alpha(&option_frames[1], 0.0f);
	for(i = 0; i < 4; i++) {
		check_edge[i] = make_rect(PVR_LIST_TR_POLY, COL_GREEN, 35.0f);
		rect_alpha(check_edge[i], 0.0f);
	}
	check_mark = make_rect(PVR_LIST_TR_POLY, COL_GREEN, 35.2f);
	saver_dim = make_rect(PVR_LIST_TR_POLY, COL_BLACK, 40.0f);
	saver_fill = make_rect(PVR_LIST_TR_POLY, COL_BLACK, 41.0f);
	frame_make(&saver_frames[0], COL_WHITE, 42.0f);
	frame_make(&saver_frames[1], COL_WHITE, 42.2f);
	frame_alpha(&saver_frames[0], 0.0f);
	frame_alpha(&saver_frames[1], 0.0f);
	rect_alpha(option_fill, 0.0f);
	rect_alpha(option_bar, 0.0f);
	rect_alpha(check_mark, 0.0f);
	rect_alpha(saver_dim, 0.0f);
	rect_alpha(saver_fill, 0.0f);
	rect_move(backdrop, 0.0f, 0.0f, (float)screen_w, (float)screen_h);
	ready = select_bar != NULL && backdrop != NULL;
	if(ready) {
		view_frame(1, 1, 0, &scroll, 0, 0, 0, 0, mode, CRT_PANEL_MENU, 0, 0);
	}
}

void view_shutdown(void) {
	int i;

	hide_chart();
	if(select_bar && select_mover) {
		TSU_DrawableAnimRemove((Drawable *)select_bar, (Animation *)select_mover);
	}
	if(select_mover) {
		TSU_LogXYZMoverDestroy(&select_mover);
	}
	if(chart_banner && view_app && view_app->tsunami) {
		TSU_AppSubRemoveBanner(view_app->tsunami, chart_banner);
	}
	chart_on = 0;
	floor_on = 0;
	vert_on = 0;
	help_on = 0;
	destroy_strips();
	destroy_clouds();
	destroy_layer(&floor_banner, &floor_tex);
	destroy_layer(&vert_banner, &vert_tex);
	destroy_layer(&help_banner, &help_tex);
	if(chart_banner) {
		TSU_BannerDestroyAll(&chart_banner);
		chart_tex = NULL;
	}
	else if(chart_tex) {
		TSU_TextureDestroy(&chart_tex);
	}
	for(i = 0; i < GRID_LINES; i++) {
		rect_free(&grid_v[i]);
		rect_free(&grid_h[i]);
	}
	for(i = 0; i < 4; i++) {
		rect_free(&grid_border[i]);
		rect_free(&shadow_edge[i]);
	}
	rect_free(&grid_cross[0]);
	rect_free(&grid_cross[1]);
	rect_free(&shadow_fill);
	rect_free(&shadow_box);
	glyph_free(&title_glyph);
	glyph_free(&author_glyph);
	glyph_free(&version_glyph);
	glyph_free(&index_glyph);
	glyph_free(&mode_glyph);
	glyph_free(&up_glyph);
	glyph_free(&down_glyph);
	glyph_free(&option_title);
	glyph_free(&option_music);
	glyph_free(&option_exit);
	glyph_free(&option_resume);
	glyph_free(&option_hint[0]);
	glyph_free(&option_hint[1]);
	glyph_free(&saver_title);
	glyph_free(&saver_version);
	for(i = 0; i < 4; i++) {
		glyph_free(&hint_glyph[i]);
	}
	for(i = 0; i < CRT_PATTERN_MAX; i++) {
		glyph_free(&name_glyph[i]);
	}
	for(i = 0; i < FRAME_COUNT; i++) {
		frame_free(&frames[i]);
	}
	rect_free(&hint_fill);
	rect_free(&backdrop);
	rect_free(&list_track);
	rect_free(&list_thumb);
	frame_free(&scroll_frames[0]);
	frame_free(&scroll_frames[1]);
	frame_free(&option_frames[0]);
	frame_free(&option_frames[1]);
	frame_free(&saver_frames[0]);
	frame_free(&saver_frames[1]);
	rect_free(&option_fill);
	rect_free(&option_bar);
	rect_free(&check_mark);
	rect_free(&saver_dim);
	rect_free(&saver_fill);
	for(i = 0; i < 4; i++) {
		rect_free(&check_edge[i]);
	}
	rect_free(&select_bar);
	saver_alive = 0;
	mode_cached[0] = '\0';
	index_cached[0] = '\0';
	names_ready = 0;
	chart_owner = 0;
	soft_pattern = -1;
	soft_variant = -999;
	soft_param = -999;
	soft_bake = -1;
	help_valid = 0;
	help_failed = 0;
	help_pattern = -1;
	help_variant = -999;
	help_param = -999;
	scroll_ready = 0;
	scroll_failed = 0;
	scroll_geom = 0;
	floor_built = 0;
	vert_built = 0;
	grid_made = 0;
	grid_on = 0;
	shadow_made = 0;
	shadow_on = 0;
	shadow_variant = -999;
	was_menu = -1;
	last_sel = -1;
	last_scroll = -1;
	last_help = -1;
	view_app = NULL;
	ready = 0;
}

void view_frame(int in_menu, int show_help, int menu_sel, int *menu_scroll,
	int pattern, int variant, int param, unsigned frame, const char *mode_line,
	int panel, int panel_sel, int music_off) {
	int count;
	int visible;
	int list_x;
	int list_w;
	int list_top;
	int list_bottom;
	int line_h;
	int hint_y;
	int hint_h;
	int hint_step;

	if(!ready || !menu_scroll) {
		return;
	}
	count = pattern_count();
	if(count < 1) {
		return;
	}
	if(menu_sel < 0) {
		menu_sel = 0;
	}
	if(menu_sel >= count) {
		menu_sel = count - 1;
	}
	if(!in_menu) {
		hide_menu();
		show_chart(pattern, variant, param, show_help, frame);
		was_menu = 0;
		return;
	}

	hide_chart();
	view_panel = panel;
	if(sync_mode(mode_line) || sync_index(menu_sel, count)) {
		place_header(show_help);
	}
	layout_metrics(show_help, &list_x, &list_w, &list_top, &list_bottom,
		&line_h, &hint_y, &hint_h, &hint_step);
	visible = (list_bottom - list_top) / line_h;
	if(visible < 1) {
		visible = 1;
	}
	if(visible > count) {
		visible = count;
	}
	if(*menu_scroll > count - visible) {
		*menu_scroll = count - visible;
	}
	if(*menu_scroll < 0) {
		*menu_scroll = 0;
	}
	if(*menu_scroll > menu_sel) {
		*menu_scroll = menu_sel;
	}
	if(menu_sel >= *menu_scroll + visible) {
		*menu_scroll = menu_sel - visible + 1;
	}
	if(*menu_scroll > count - visible) {
		*menu_scroll = count - visible;
	}
	if(*menu_scroll < 0) {
		*menu_scroll = 0;
	}
	if(!names_ready) {
		build_names();
	}
	{
		int snap = was_menu != 1 || show_help != last_help;
		int retarget = snap || menu_sel != last_sel || *menu_scroll != last_scroll;

		place_header(show_help);
		rect_alpha(select_bar, 1.0f);
		if(retarget) {
			move_bar(menu_sel, *menu_scroll, show_help, snap);
		}
	}
	place_names(menu_sel, *menu_scroll, show_help);
	if(panel == CRT_PANEL_SAVER) {
		hide_options();
		rect_alpha(select_bar, 0.0f);
		show_saver();
	}
	else if(panel == CRT_PANEL_OPTIONS) {
		hide_saver();
		rect_alpha(select_bar, 0.0f);
		show_options(panel_sel, music_off);
	}
	else {
		hide_options();
		hide_saver();
	}
	was_menu = 1;
	last_sel = menu_sel;
	last_scroll = *menu_scroll;
	last_help = show_help;
}
