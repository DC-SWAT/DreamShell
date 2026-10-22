/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#include <ds.h>
#include <string.h>
#include <stdio.h>
#include <SDL/SDL_dreamcast.h>

#include "app_module.h"
#include "draw.h"
#include "patterns.h"
#include "view.h"

DEFAULT_MODULE_EXPORTS(app_crttest);

enum {
	ACT_NONE = 0,
	ACT_UP,
	ACT_DOWN,
	ACT_LEFT,
	ACT_RIGHT,
	ACT_A,
	ACT_B,
	ACT_X,
	ACT_Y,
	ACT_START
};

static struct {
	App_t *app;
	Event_t *video_ev;
	Event_t *input_ev;
	int active;
	int closing;
	int in_menu;
	int menu_sel;
	int menu_scroll;
	int pattern;
	int show_help;
	int panel;
	int option_sel;
	int music_off;
	unsigned idle_frame;
	unsigned frame;
	int variant[CRT_PATTERN_MAX];
	int param[CRT_PATTERN_MAX];
	int axis_y;
	int wheel_ready;
	struct {
		int primed;
		int rest;
		int ready;
		int down;
		int coupled;
	} pedal[2];
} crt;

static void crt_video(void *ds_event, void *param, int action);

static void crt_video_event_open(void) {
	if(crt.video_ev) {
		return;
	}
	crt.video_ev = AddEvent("CRT_Test_Video", EVENT_TYPE_VIDEO, EVENT_PRIO_DEFAULT,
		crt_video, NULL);
	if(crt.video_ev) {
		SetEventActive(crt.video_ev, 0);
	}
}

static void crt_events_remove(void) {
	crt.active = 0;
	if(crt.video_ev) {
		SetEventActive(crt.video_ev, 0);
	}
	if(crt.input_ev) {
		RemoveEvent(crt.input_ev);
		crt.input_ev = NULL;
	}
}

static void crt_events_shutdown(void) {
	crt_events_remove();
	if(crt.video_ev) {
		RemoveEvent(crt.video_ev);
		crt.video_ev = NULL;
	}
}

enum {
	OPT_MUSIC = 0,
	OPT_EXIT,
	OPT_RESUME,
	OPT_COUNT
};

typedef int (*crt_wav_init_fn)(void);
typedef void (*crt_wav_void_fn)(void);
typedef void (*crt_wav_hnd_fn)(int);
typedef int (*crt_wav_create_fn)(const char *, int);
typedef void (*crt_wav_vol_fn)(int, int);

static crt_wav_init_fn crt_wav_init;
static crt_wav_void_fn crt_wav_shutdown;
static crt_wav_hnd_fn crt_wav_destroy;
static crt_wav_create_fn crt_wav_create;
static crt_wav_hnd_fn crt_wav_play;
static crt_wav_hnd_fn crt_wav_stop;
static crt_wav_vol_fn crt_wav_volume;
static Module_t *crt_wave;
static int crt_wave_owned;
static int crt_wav_ready;
static int crt_wav_hnd = -1;
static int crt_music_warned;

static uint32 crt_sym(const char *name) {
	return GetExportSymAddr(name, NULL);
}

static int crt_wav_bind(void) {
	if(crt_wav_init) {
		return 1;
	}
	crt_wav_init = (crt_wav_init_fn)crt_sym("wav_init");
	crt_wav_shutdown = (crt_wav_void_fn)crt_sym("wav_shutdown");
	crt_wav_destroy = (crt_wav_hnd_fn)crt_sym("wav_destroy");
	crt_wav_create = (crt_wav_create_fn)crt_sym("wav_create");
	crt_wav_play = (crt_wav_hnd_fn)crt_sym("wav_play");
	crt_wav_stop = (crt_wav_hnd_fn)crt_sym("wav_stop");
	crt_wav_volume = (crt_wav_vol_fn)crt_sym("wav_volume");
	if(!crt_wav_init || !crt_wav_shutdown || !crt_wav_destroy || !crt_wav_create ||
		!crt_wav_play || !crt_wav_stop || !crt_wav_volume) {
		crt_wav_init = NULL;
		return 0;
	}
	return 1;
}

static Module_t *crt_open_wave(void) {
	char dir[NAME_MAX];
	char file[NAME_MAX];
	Module_t *mod;
	char *slash;

	mod = GetModuleByName("wave");
	if(mod) {
		return mod;
	}
	if(!crt.app) {
		return NULL;
	}
	GetAppPath(dir, sizeof(dir), crt.app->fn);
	snprintf(file, sizeof(file), "%s/modules/wave.klf", dir);
	mod = OpenModule(file);
	if(mod) {
		crt_wave_owned = 1;
		return mod;
	}
	slash = strrchr(dir, '/');
	if(!slash || slash == dir) {
		return NULL;
	}
	*slash = '\0';
	slash = strrchr(dir, '/');
	if(!slash || slash == dir) {
		return NULL;
	}
	*slash = '\0';
	snprintf(file, sizeof(file), "%s/modules/wave.klf", dir);
	mod = OpenModule(file);
	if(mod) {
		crt_wave_owned = 1;
	}
	return mod;
}

static void crt_music_stop(void) {
	if(crt_wav_hnd >= 0 && crt_wav_stop && crt_wav_destroy) {
		crt_wav_stop(crt_wav_hnd);
		crt_wav_destroy(crt_wav_hnd);
	}
	crt_wav_hnd = -1;
}

static void crt_music_start(void) {
	char dir[NAME_MAX];
	char file[NAME_MAX];
	int volume;

	if(crt.music_off || crt_wav_hnd >= 0 || !crt.app) {
		return;
	}
	if(!crt_wave) {
		crt_wave = crt_open_wave();
	}
	if(!crt_wave || !crt_wav_bind()) {
		if(!crt_music_warned) {
			crt_music_warned = 1;
			ds_printf("DS_ERROR: CRT Test: wave module was not opened\n");
		}
		return;
	}
	if(!crt_wav_ready) {
		if(!crt_wav_init()) {
			return;
		}
		crt_wav_ready = 1;
	}
	GetAppPath(dir, sizeof(dir), crt.app->fn);
	snprintf(file, sizeof(file), "%s/theme.wav", dir);
	crt_wav_hnd = crt_wav_create(file, 1);
	if(crt_wav_hnd < 0) {
		if(!crt_music_warned) {
			crt_music_warned = 1;
			ds_printf("DS_ERROR: CRT Test: theme.wav was not opened\n");
		}
		return;
	}
	volume = GetVolumeFromSettings();
	if(volume >= 0) {
		crt_wav_volume(crt_wav_hnd, volume);
	}
	crt_wav_play(crt_wav_hnd);
}

static void crt_music_apply(void) {
	if(crt.music_off) {
		crt_music_stop();
	}
	else {
		crt_music_start();
	}
}

static void crt_cfg_path(char *file, size_t size) {
	char dir[NAME_MAX];

	file[0] = '\0';
	if(!crt.app || !crt.app->fn[0]) {
		return;
	}
	GetAppPath(dir, sizeof(dir), crt.app->fn);
	snprintf(file, size, "%s/crttest.cfg", dir);
}

static void crt_music_load(void) {
	char file[NAME_MAX];
	char buf[16];
	int fd;
	int n;
	int i;

	crt.music_off = 0;
	crt_cfg_path(file, sizeof(file));
	if(!file[0]) {
		return;
	}
	fd = open(file, O_RDONLY);
	if(fd < 0) {
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	for(i = 0; i < n; i++) {
		if(buf[i] == '1') {
			crt.music_off = 1;
			return;
		}
		if(buf[i] == '0') {
			return;
		}
	}
}

static void crt_music_save(void) {
	char file[NAME_MAX];
	const char *text;
	int fd;

	crt_cfg_path(file, sizeof(file));
	if(!file[0]) {
		return;
	}
	text = crt.music_off ? "1\n" : "0\n";
	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd < 0) {
		return;
	}
	write(fd, text, 2);
	close(fd);
}

static void crt_music_shutdown(void) {
	crt_music_stop();
	if(crt_wav_ready && crt_wav_shutdown) {
		crt_wav_shutdown();
	}
	crt_wav_ready = 0;
	if(crt_wave_owned && crt_wave) {
		CloseModule(crt_wave);
	}
	crt_wave = NULL;
	crt_wave_owned = 0;
	crt_wav_init = NULL;
}

static void crt_exit(void) {
	if(crt.closing) {
		return;
	}
	crt.closing = 1;
	crt.active = 0;
	crt_events_remove();
	SDL_DC_EmulateMouse(SDL_TRUE);
	SetVirtKeyboardEnabled(1);
	if(!OpenMainApp() && crt.app) {
		CloseApp(crt.app, 0);
	}
}

static void crt_reset_options(void) {
	int i;
	int count = pattern_count();

	if(count > CRT_PATTERN_MAX) {
		count = CRT_PATTERN_MAX;
	}
	for(i = 0; i < count; i++) {
		crt.variant[i] = 0;
		crt.param[i] = pattern_param_default(i);
	}
}

static const char *cable_name(void) {
	switch(vid_check_cable()) {
		case CT_VGA:
			return "VGA";
		case CT_RGB:
			return "RGB";
		case CT_COMPOSITE:
			return "composite";
		case CT_NONE:
			return "no cable";
		default:
			return "video";
	}
}

static const char *scan_name(int mode) {
	switch(mode) {
		case DM_640x480_NTSC_IL:
		case DM_640x480_PAL_IL:
		case DM_256x256_PAL_IL:
		case DM_768x480_NTSC_IL:
		case DM_768x576_PAL_IL:
		case DM_768x480_PAL_IL:
			return "interlaced";
		default:
			return "progressive";
	}
}

static int ui_scale(const fb_t *fb) {
	return fb->h >= 200 ? 2 : 1;
}

static int safe_x(const fb_t *fb) {
	int margin = fb->w / 20;

	if(margin < 12) {
		margin = 12;
	}
	if(margin > 32) {
		margin = 32;
	}
	return margin;
}

static int safe_y(const fb_t *fb) {
	int margin = fb->h / 10;

	if(margin < 20) {
		margin = 20;
	}
	if(margin > 48) {
		margin = 48;
	}
	return margin;
}

static void draw_menu_box(fb_t *fb, int x, int y, int w, int h, uint16_t fill, uint16_t edge) {
	fb_fill_rect(fb, x, y, w, h, fill);
	fb_rect(fb, x, y, w, h, edge);
	if(w > 8 && h > 8) {
		fb_rect(fb, x + 3, y + 3, w - 6, h - 6, edge);
	}
}

static int wrap_take(const char *cursor, int limit) {
	int length = 0;
	int last_space = -1;
	int take;

	while(cursor[length] && length < limit) {
		if(cursor[length] == ' ') {
			last_space = length;
		}
		length++;
	}
	take = length;
	if(cursor[length] && last_space > 0) {
		take = last_space;
	}
	if(take < 1) {
		take = 1;
	}
	if(take > 120) {
		take = 120;
	}
	return take;
}

static int draw_wrap_box(fb_t *fb, int box_x, int box_w, int y, int scale, uint16_t color,
	const char *text, int max_lines, int draw) {
	int limit;
	int lines = 0;
	const char *cursor = text;

	if(!text || max_lines < 1 || scale < 1) {
		return 0;
	}
	limit = (box_w - 16) / (8 * scale);
	if(limit < 1) {
		return 0;
	}
	if(limit > 96) {
		limit = 96;
	}

	while(*cursor && lines < max_lines) {
		int take;
		int width;
		int x;
		char line[128];
		int i;

		while(*cursor == ' ') {
			cursor++;
		}
		if(!*cursor) {
			break;
		}
		take = wrap_take(cursor, limit);
		if(draw) {
			for(i = 0; i < take; i++) {
				line[i] = cursor[i];
			}
			line[take] = '\0';
			width = take * 8 * scale;
			x = box_x + (box_w - width) / 2;
			if(x < box_x + 8) {
				x = box_x + 8;
			}
			fb_text_strong(fb, x, y + lines * 8 * scale, scale, color, line);
		}
		lines++;
		cursor += take;
	}
	return lines;
}

static void draw_centered(fb_t *fb, int box_x, int box_w, int y, int scale, uint16_t color,
	const char *text) {
	int width = fb_text_width(text, scale);
	int x = box_x + (box_w - width) / 2;

	if(x < box_x + 8) {
		x = box_x + 8;
	}
	fb_text_strong(fb, x, y, scale, color, text);
}

static void draw_help(fb_t *fb) {
	const char *controls[5];
	char head[96];
	char head2[96];
	char status[96];
	const char *help;
	uint16_t black = fb_rgb(fb, 0, 0, 0);
	uint16_t green = fb_rgb(fb, 140, 255, 48);
	uint16_t dim = fb_rgb(fb, 64, 168, 36);
	uint16_t red = fb_rgb(fb, 255, 42, 28);
	uint16_t white = fb_rgb(fb, 240, 244, 236);
	int scale = ui_scale(fb);
	int big = scale;
	int margin_x = safe_x(fb);
	int margin_y = safe_y(fb);
	int box_x = margin_x;
	int box_w = fb->w - margin_x * 2;
	int line_h = 8 * scale + 4;
	int name_lines;
	int name_h;
	int help_lines;
	int help_h;
	int help_scale;
	int controls_h;
	int y;
	int i;

	if(fb->h < 160 || box_w < 8 * scale * 8) {
		return;
	}
	if(fb->h >= 400) {
		big = scale + 1;
	}

	controls[0] = "Wheel or Left/Right: next chart.";
	controls[1] = "Lever or Up/Down changes value.";
	controls[2] = "Gas or A changes the option.";
	controls[3] = "Brake or B: back to menu.";
	controls[4] = "Start or Y hides this text.";

	pattern_status(crt.pattern, crt.variant[crt.pattern], crt.param[crt.pattern],
		status, (int)sizeof(status));
	{
		const char *chart_name = pattern_name(crt.pattern);

		head2[0] = '\0';
		if(chart_name && !strcmp(chart_name, "Horizontal/Vertical Stripes")) {
			snprintf(head, sizeof(head), "%d/%d  Horizontal/Vertical",
				crt.pattern + 1, pattern_count());
			snprintf(head2, sizeof(head2), "Stripes");
		}
		else if(chart_name && !strcmp(chart_name, "Color Bars with Gray Scale")) {
			snprintf(head, sizeof(head), "%d/%d  Color Bars with",
				crt.pattern + 1, pattern_count());
			snprintf(head2, sizeof(head2), "Gray Scale");
		}
		else {
			snprintf(head, sizeof(head), "%d/%d  %s", crt.pattern + 1, pattern_count(),
				chart_name ? chart_name : "");
		}
	}
	help = pattern_help(crt.pattern, crt.variant[crt.pattern], crt.param[crt.pattern]);
	name_lines = 1;
	if(head2[0]) {
		name_lines++;
	}
	if(status[0]) {
		name_lines++;
	}
	name_h = 12 + line_h * name_lines;
	help_scale = big;
	help_lines = draw_wrap_box(fb, box_x, box_w, 0, help_scale, red, help, 4, 0);
	if(help_lines < 1) {
		help_lines = 1;
	}
	help_h = 12 + help_lines * 8 * help_scale;
	controls_h = 12 + line_h * 5;
	if(margin_y + name_h + 8 + help_h + 8 + controls_h + margin_y > fb->h) {
		help_scale = scale;
		help_lines = draw_wrap_box(fb, box_x, box_w, 0, help_scale, red, help, 2, 0);
		if(help_lines < 1) {
			help_lines = 1;
		}
		help_h = 12 + help_lines * 8 * help_scale;
	}

	y = margin_y;
	draw_menu_box(fb, box_x, y, box_w, name_h, black, green);
	{
		int arrow_w = fb_text_width("<", scale);
		int side = 8 + arrow_w + 8;
		int text_y = y + 8;
		int arrow_y = y + (name_h - 8 * scale) / 2;

		fb_text_strong(fb, box_x + 8, arrow_y, scale, green, "<");
		fb_text_strong(fb, box_x + box_w - 8 - arrow_w, arrow_y, scale, green, ">");
		draw_centered(fb, box_x + side, box_w - side * 2, text_y, scale, green, head);
		if(head2[0]) {
			draw_centered(fb, box_x + side, box_w - side * 2, text_y + line_h,
				scale, green, head2);
			text_y += line_h;
		}
		if(status[0]) {
			draw_centered(fb, box_x + side, box_w - side * 2, text_y + line_h,
				scale, dim, status);
		}
	}

	y += name_h + 8;
	draw_menu_box(fb, box_x, y, box_w, help_h, black, red);
	draw_wrap_box(fb, box_x, box_w, y + 8, help_scale, red, help, help_lines, 1);

	y = fb->h - margin_y - controls_h;
	draw_menu_box(fb, box_x, y, box_w, controls_h, black, white);
	for(i = 0; i < 5; i++) {
		draw_centered(fb, box_x, box_w, y + 8 + i * line_h, scale, white, controls[i]);
	}
}

void crt_compose_chart(fb_t *fb, unsigned frame, int pattern, int variant, int param, int show_help) {
	int id = pattern;

	if(!fb) {
		return;
	}
	fb_clear(fb, fb_rgb(fb, 0, 0, 0));
	if(id < 0 || id >= pattern_count()) {
		id = 0;
	}
	pattern_draw(fb, id, frame, variant, param);
	if(show_help) {
		draw_help(fb);
	}
}

uint16_t crt_compose_help(fb_t *fb) {
	uint16_t key;

	if(!fb) {
		return 0;
	}
	key = fb_rgb(fb, 255, 0, 255);
	fb_clear(fb, key);
	draw_help(fb);
	return key;
}

static void crt_video(void *ds_event, void *param, int action) {
	char mode[96];
	int id;

	(void)ds_event;
	(void)param;

	if(action != EVENT_ACTION_RENDER || !crt.active) {
		return;
	}

	crt.frame++;
	if(crt.in_menu && crt.panel == CRT_PANEL_MENU) {
		int limit = 60 * 30;

		switch(GetVideoMode()) {
			case DM_640x480_PAL_IL:
			case DM_256x256_PAL_IL:
			case DM_768x576_PAL_IL:
			case DM_768x480_PAL_IL:
				limit = 50 * 30;
				break;
			default:
				break;
		}
		if((int)(crt.frame - crt.idle_frame) >= limit) {
			crt.panel = CRT_PANEL_SAVER;
		}
	}
	id = crt.pattern;
	if(id < 0 || id >= pattern_count()) {
		id = 0;
	}
	snprintf(mode, sizeof(mode), "%dx%d  %s  %s",
		GetScreenWidth(), GetScreenHeight(), cable_name(), scan_name(GetVideoMode()));
	view_frame(crt.in_menu, crt.show_help, crt.menu_sel, &crt.menu_scroll,
		id, crt.variant[id], crt.param[id], crt.frame, mode,
		crt.panel, crt.option_sel, crt.music_off);
}

static void crt_move_menu(int delta) {
	int count = pattern_count();
	int sel = crt.menu_sel + delta;

	if(count < 1) {
		return;
	}
	while(sel < 0) {
		sel += count;
	}
	while(sel >= count) {
		sel -= count;
	}
	crt.menu_sel = sel;
}

static void crt_shift_pattern(int delta) {
	crt_move_menu(delta);
	crt.pattern = crt.menu_sel;
}

static void crt_change_option(int dir) {
	int id = crt.pattern;

	if(id < 0 || id >= pattern_count()) {
		return;
	}
	if(pattern_variant_count(id) > 1) {
		pattern_cycle_variant(id, &crt.variant[id], dir);
	}
	else if(pattern_has_param(id)) {
		pattern_cycle_param(id, &crt.param[id], dir);
	}
}

static void crt_touch(void) {
	crt.idle_frame = crt.frame;
}

static void crt_action(int act) {
	int id;

	if(!crt.active || crt.closing || act == ACT_NONE) {
		return;
	}
	if(crt.panel == CRT_PANEL_SAVER) {
		crt.panel = CRT_PANEL_MENU;
		crt_touch();
		return;
	}
	crt_touch();
	if(crt.panel == CRT_PANEL_OPTIONS) {
		switch(act) {
			case ACT_UP:
				crt.option_sel--;
				if(crt.option_sel < 0) {
					crt.option_sel = OPT_COUNT - 1;
				}
				break;
			case ACT_DOWN:
				crt.option_sel++;
				if(crt.option_sel >= OPT_COUNT) {
					crt.option_sel = 0;
				}
				break;
			case ACT_A:
				if(crt.option_sel == OPT_MUSIC) {
					crt.music_off = !crt.music_off;
					crt_music_save();
					crt_music_apply();
				}
				else if(crt.option_sel == OPT_EXIT) {
					crt_exit();
				}
				else {
					crt.panel = CRT_PANEL_MENU;
				}
				break;
			case ACT_B:
				crt.panel = CRT_PANEL_MENU;
				break;
			default:
				break;
		}
		return;
	}

	if(crt.in_menu) {
		switch(act) {
			case ACT_UP:
				crt_move_menu(-1);
				break;
			case ACT_DOWN:
				crt_move_menu(1);
				break;
			case ACT_LEFT:
				crt_move_menu(-8);
				break;
			case ACT_RIGHT:
				crt_move_menu(8);
				break;
			case ACT_A:
				crt.pattern = crt.menu_sel;
				crt.in_menu = 0;
				crt.panel = CRT_PANEL_MENU;
				break;
			case ACT_B:
				crt.panel = CRT_PANEL_OPTIONS;
				crt.option_sel = OPT_MUSIC;
				break;
			case ACT_Y:
			case ACT_START:
				crt.show_help = !crt.show_help;
				break;
			default:
				break;
		}
		return;
	}

	id = crt.pattern;
	switch(act) {
		case ACT_LEFT:
			crt_shift_pattern(-1);
			break;
		case ACT_RIGHT:
			crt_shift_pattern(1);
			break;
		case ACT_UP:
			if(pattern_is_convergence(id)) {
				pattern_convergence_value(crt.variant[id], &crt.param[id], 1);
			}
			else if(pattern_has_param(id)) {
				pattern_cycle_param(id, &crt.param[id], 1);
			}
			else {
				crt_change_option(-1);
			}
			break;
		case ACT_DOWN:
			if(pattern_is_convergence(id)) {
				pattern_convergence_value(crt.variant[id], &crt.param[id], -1);
			}
			else if(pattern_has_param(id)) {
				pattern_cycle_param(id, &crt.param[id], -1);
			}
			else {
				crt_change_option(1);
			}
			break;
		case ACT_A:
		case ACT_X:
			if(pattern_is_convergence(id)) {
				pattern_cycle_variant(id, &crt.variant[id], 1);
			}
			else {
				crt_change_option(1);
			}
			break;
		case ACT_Y:
		case ACT_START:
			crt.show_help = !crt.show_help;
			break;
		case ACT_B:
			crt.in_menu = 1;
			crt.menu_sel = crt.pattern;
			crt.panel = CRT_PANEL_MENU;
			break;
		default:
			break;
	}
}

static int action_from_key(SDLKey key) {
	switch(key) {
		case SDLK_UP:
			return ACT_UP;
		case SDLK_DOWN:
			return ACT_DOWN;
		case SDLK_LEFT:
			return ACT_LEFT;
		case SDLK_RIGHT:
			return ACT_RIGHT;
		case SDLK_RETURN:
		case SDLK_SPACE:
		case SDLK_a:
			return ACT_A;
		case SDLK_ESCAPE:
		case SDLK_BACKSPACE:
		case SDLK_b:
			return ACT_B;
		case SDLK_x:
		case SDLK_LCTRL:
			return ACT_X;
		case SDLK_y:
		case SDLK_LALT:
			return ACT_Y;
		case SDLK_TAB:
			return ACT_START;
		case SDLK_l:
			return ACT_LEFT;
		case SDLK_r:
			return ACT_RIGHT;
		default:
			return ACT_NONE;
	}
}

static int action_from_button(int button) {
	switch(button) {
		case SDL_DC_A:
			return ACT_A;
		case SDL_DC_B:
			return ACT_B;
		case SDL_DC_X:
			return ACT_X;
		case SDL_DC_Y:
			return ACT_Y;
		case SDL_DC_START:
			return ACT_START;
		case SDL_DC_L:
		case SDL_DC_R:
		default:
			return ACT_NONE;
	}
}

static int stick_side(int value, int current) {
	int wide = value > 255 || value < -255;
	int dead = wide ? 14000 : 48;
	int back = wide ? 6000 : 24;

	if(value <= -dead) {
		return -1;
	}
	if(value >= dead) {
		return 1;
	}
	if(value > -back && value < back) {
		return 0;
	}
	return current;
}

static int axis_span(int value) {
	if(value > 255 || value < -255) {
		return 1;
	}
	return 0;
}

static int axis_abs(int value) {
	if(value < 0) {
		return -value;
	}
	return value;
}

static void crt_pedal(int index, int value, int act) {
	typeof(crt.pedal[0]) *pedal = &crt.pedal[index];
	typeof(crt.pedal[0]) *other = &crt.pedal[index ^ 1];
	int press = 64;
	int release = 20;
	int delta;

	if(axis_span(value) || axis_span(pedal->rest)) {
		press = 8000;
		release = 2500;
	}

	if(!pedal->primed) {
		pedal->primed = 1;
		pedal->rest = value;
		pedal->ready = 0;
		pedal->down = 0;
		pedal->coupled = 0;
		return;
	}

	delta = axis_abs(value - pedal->rest);
	if(axis_abs(value) <= release && axis_abs(value) < axis_abs(pedal->rest)) {
		pedal->rest = value;
		delta = 0;
	}

	if(pedal->coupled) {
		if(delta <= release) {
			pedal->coupled = 0;
			pedal->ready = 1;
			pedal->down = 0;
		}
		return;
	}

	if(!pedal->ready && !pedal->down) {
		if(delta <= release) {
			pedal->ready = 1;
			return;
		}
		pedal->ready = 1;
	}

	if(delta <= release) {
		pedal->ready = 1;
		pedal->down = 0;
		return;
	}
	if(pedal->down || !pedal->ready || delta < press) {
		return;
	}
	if(other->down) {
		pedal->coupled = 1;
		pedal->ready = 0;
		return;
	}

	pedal->ready = 0;
	pedal->down = 1;
	crt_action(act);
}

static void crt_wheel(int value) {
	int wide = axis_span(value);
	int fire = wide ? 8000 : 56;
	int center = wide ? 2500 : 20;

	if(value > -center && value < center) {
		crt.wheel_ready = 1;
		return;
	}
	if(!crt.wheel_ready) {
		return;
	}
	if(value <= -fire || value >= fire) {
		crt.wheel_ready = 0;
		crt_action(value < 0 ? ACT_LEFT : ACT_RIGHT);
	}
}

static void crt_stick_y(int value) {
	int side = stick_side(value, crt.axis_y);

	if(side == 0) {
		crt.axis_y = 0;
		return;
	}
	if(side == crt.axis_y) {
		return;
	}
	crt.axis_y = side;
	crt_action(side < 0 ? ACT_UP : ACT_DOWN);
}

static void crt_input(void *ds_event, void *param, int action) {
	SDL_Event *event = (SDL_Event *)param;

	(void)ds_event;
	if(action != EVENT_ACTION_UPDATE || !event || !crt.active || crt.closing) {
		return;
	}

	switch(event->type) {
		case SDL_JOYBUTTONDOWN:
			crt_action(action_from_button(event->jbutton.button));
			break;
		case SDL_JOYHATMOTION:
			if(event->jhat.value & SDL_HAT_UP) {
				crt_action(ACT_UP);
			}
			else if(event->jhat.value & SDL_HAT_DOWN) {
				crt_action(ACT_DOWN);
			}
			else if(event->jhat.value & SDL_HAT_LEFT) {
				crt_action(ACT_LEFT);
			}
			else if(event->jhat.value & SDL_HAT_RIGHT) {
				crt_action(ACT_RIGHT);
			}
			break;
		case SDL_JOYAXISMOTION:
			if(event->jaxis.axis == 0) {
				crt_wheel(event->jaxis.value);
			}
			else if(event->jaxis.axis == 1) {
				crt_stick_y(event->jaxis.value);
			}
			else if(event->jaxis.axis == 2) {
				crt_pedal(0, event->jaxis.value, ACT_A);
			}
			else if(event->jaxis.axis == 3) {
				crt_pedal(1, event->jaxis.value, ACT_B);
			}
			break;
		case SDL_KEYDOWN:
			crt_action(action_from_key(event->key.keysym.sym));
			break;
		default:
			break;
	}
}

static void crt_events_add(void) {
	crt_video_event_open();
	if(crt.video_ev) {
		SetEventActive(crt.video_ev, 1);
	}

	if(!crt.input_ev) {
		crt.input_ev = AddEvent("CRT_Test_Input", EVENT_TYPE_INPUT, EVENT_PRIO_DEFAULT,
			crt_input, NULL);
	}
	else {
		SetEventActive(crt.input_ev, 1);
	}

	if(!crt.video_ev || !crt.input_ev) {
		ds_printf("DS_ERROR: CRT Test: failed to register events\n");
	}
}

void CrtTest_Init(App_t *app) {
	memset(&crt, 0, sizeof(crt));
	crt.app = app;
	crt.in_menu = 1;
	crt.show_help = 1;
	crt_reset_options();
	crt_video_event_open();
	view_init(app);

	if(!app) {
		ds_printf("DS_ERROR: CRT Test: init without an app\n");
	}
}

void CrtTest_Open(App_t *app) {
	if(app) {
		crt.app = app;
	}
	SDL_DC_EmulateMouse(SDL_FALSE);
	SetVirtKeyboardEnabled(0);
	crt.closing = 0;
	crt.active = 1;
	crt.in_menu = 1;
	crt.show_help = 1;
	crt.panel = CRT_PANEL_MENU;
	crt.frame = 0;
	crt.idle_frame = 0;
	crt.axis_y = 0;
	crt.wheel_ready = 1;
	memset(crt.pedal, 0, sizeof(crt.pedal));
	crt_events_add();
	crt_music_load();
	crt_music_apply();
}

void CrtTest_Close(App_t *app) {
	(void)app;
	crt_music_save();
	crt_music_stop();
	crt_events_remove();
	SDL_DC_EmulateMouse(SDL_TRUE);
	SetVirtKeyboardEnabled(1);
}

void CrtTest_Shutdown(App_t *app) {
	(void)app;
	crt_music_shutdown();
	crt_events_shutdown();
	view_shutdown();
	SDL_DC_EmulateMouse(SDL_TRUE);
	SetVirtKeyboardEnabled(1);
}
