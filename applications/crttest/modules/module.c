/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#include <ds.h>
#include <string.h>
#include <stdio.h>
#include <audio/wav.h>
#include <dc/sound/stream.h>
#include <SDL/SDL_dreamcast.h>

#include "app_module.h"
#include "scroll_art.h"
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

static int crt_wav_ready;
static int crt_wav_hnd = SND_STREAM_INVALID;
static int crt_music_warned;

static void crt_music_stop(void) {
	if(crt_wav_hnd != SND_STREAM_INVALID) {
		wav_stop(crt_wav_hnd);
		wav_destroy(crt_wav_hnd);
	}
	crt_wav_hnd = SND_STREAM_INVALID;
}

static void crt_music_start(void) {
	char dir[NAME_MAX];
	char file[NAME_MAX];
	int volume;

	if(crt.music_off || crt_wav_hnd != SND_STREAM_INVALID || !crt.app) {
		return;
	}
	if(!crt_wav_ready) {
		if(!wav_init()) {
			if(!crt_music_warned) {
				crt_music_warned = 1;
				ds_printf("DS_ERROR: CRT Test: wave module was not opened\n");
			}
			return;
		}
		crt_wav_ready = 1;
	}
	GetAppPath(dir, sizeof(dir), crt.app->fn);
	snprintf(file, sizeof(file), "%s/sounds/theme.wav", dir);
	crt_wav_hnd = wav_create(file, 1);
	if(crt_wav_hnd == SND_STREAM_INVALID) {
		if(!crt_music_warned) {
			crt_music_warned = 1;
			ds_printf("DS_ERROR: CRT Test: theme.wav was not opened\n");
		}
		return;
	}
	volume = GetVolumeFromSettings();
	if(volume >= 0) {
		wav_volume(crt_wav_hnd, volume);
	}
	wav_play(crt_wav_hnd);
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
	if(crt_wav_ready) {
		wav_shutdown();
	}
	crt_wav_ready = 0;
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

static void copy_span(char *dst, int dst_len, const char *src, int take) {
	int i;
	int n = take;

	if(n >= dst_len) {
		n = dst_len - 1;
	}
	if(n < 0) {
		n = 0;
	}
	for(i = 0; i < n; i++) {
		dst[i] = src[i];
	}
	while(n > 0 && dst[n - 1] == ' ') {
		n--;
	}
	dst[n] = '\0';
}

static int fill_lines(const char *text, int box_w, int scale, int max_lines, char lines[][96]) {
	int limit;
	int count = 0;
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

	while(*cursor && count < max_lines) {
		int take;

		while(*cursor == ' ') {
			cursor++;
		}
		if(!*cursor) {
			break;
		}
		take = wrap_take(cursor, limit);
		copy_span(lines[count], 96, cursor, take);
		count++;
		cursor += take;
	}
	return count;
}

int crt_overlay_build(int w, int h, int pattern, int variant, int param, crt_overlay_t *out) {
	const char *chart_name;
	const char *help;
	int scale;
	int big;
	int margin_x;
	int margin_y;
	int box_w;
	int line_h;
	int name_lines;
	int name_h;
	int help_lines;
	int help_h;
	int help_scale;
	int controls_h;
	int list_scale;

	if(!out || w < 1 || h < 1) {
		return 0;
	}
	memset(out, 0, sizeof(*out));
	scale = h >= 200 ? 2 : 1;
	big = scale;
	if(h >= 400) {
		big = scale + 1;
	}
	margin_x = w / 20;
	if(margin_x < 12) {
		margin_x = 12;
	}
	if(margin_x > 32) {
		margin_x = 32;
	}
	margin_y = h / 10;
	if(margin_y < 20) {
		margin_y = 20;
	}
	if(margin_y > 48) {
		margin_y = 48;
	}
	box_w = w - margin_x * 2;
	if(h < 160 || box_w < 8 * scale * 8) {
		return 0;
	}
	line_h = 8 * scale + 4;
	list_scale = (h >= 400 && w >= 600) ? 3 : scale;
	chart_name = pattern_name(pattern);
	if(chart_name && !strcmp(chart_name, "Horizontal/Vertical Stripes")) {
		snprintf(out->head, sizeof(out->head), "%d/%d  Horizontal/Vertical",
			pattern + 1, pattern_count());
		snprintf(out->head2, sizeof(out->head2), "Stripes");
	}
	else if(chart_name && !strcmp(chart_name, "Color Bars with Gray Scale")) {
		snprintf(out->head, sizeof(out->head), "%d/%d  Color Bars with",
			pattern + 1, pattern_count());
		snprintf(out->head2, sizeof(out->head2), "Gray Scale");
	}
	else {
		snprintf(out->head, sizeof(out->head), "%d/%d  %s", pattern + 1, pattern_count(),
			chart_name ? chart_name : "");
	}
	pattern_status(pattern, variant, param, out->status, (int)sizeof(out->status));
	help = pattern_help(pattern, variant, param);
	name_lines = 1;
	if(out->head2[0]) {
		name_lines++;
	}
	if(out->status[0]) {
		name_lines++;
	}
	name_h = 12 + line_h * name_lines;
	help_scale = big;
	help_lines = fill_lines(help, box_w, help_scale, CRT_HELP_LINE, out->help_line);
	if(help_lines < 1) {
		help_lines = 1;
	}
	help_h = 12 + help_lines * 8 * help_scale;
	controls_h = 12 + line_h * 5;
	if(margin_y + name_h + 8 + help_h + 8 + controls_h + margin_y > h) {
		int i;

		help_scale = scale;
		for(i = 0; i < CRT_HELP_LINE; i++) {
			out->help_line[i][0] = '\0';
		}
		help_lines = fill_lines(help, box_w, help_scale, 2, out->help_line);
		if(help_lines < 1) {
			help_lines = 1;
		}
		help_h = 12 + help_lines * 8 * help_scale;
	}
	out->box_x = margin_x;
	out->box_w = box_w;
	out->scale = scale;
	out->help_scale = help_scale;
	out->line_h = line_h;
	out->arrow = 8 * list_scale;
	out->side = 8 + out->arrow + 8;
	out->name_y = margin_y;
	out->name_h = name_h;
	out->help_y = margin_y + name_h + 8;
	out->help_h = help_h;
	out->ctrl_y = h - margin_y - controls_h;
	out->ctrl_h = controls_h;
	out->help_n = help_lines;
	snprintf(out->ctrl[0], sizeof(out->ctrl[0]), "Wheel or Left/Right: next chart.");
	snprintf(out->ctrl[1], sizeof(out->ctrl[1]), "Lever or Up/Down changes value.");
	snprintf(out->ctrl[2], sizeof(out->ctrl[2]), "Gas or A changes the option.");
	snprintf(out->ctrl[3], sizeof(out->ctrl[3]), "Brake or B: back to menu.");
	snprintf(out->ctrl[4], sizeof(out->ctrl[4]), "Start or Y hides this text.");
	return 1;
}

static void draw_help(fb_t *fb) {
	crt_overlay_t lay;
	uint16_t black;
	uint16_t green;
	uint16_t red;
	uint16_t white;

	if(!fb) {
		return;
	}
	if(!crt_overlay_build(fb->w, fb->h, crt.pattern, crt.variant[crt.pattern],
		crt.param[crt.pattern], &lay)) {
		return;
	}
	black = fb_rgb(fb, 0, 0, 0);
	green = fb_rgb(fb, 140, 255, 48);
	red = fb_rgb(fb, 255, 42, 28);
	white = fb_rgb(fb, 240, 244, 236);
	draw_menu_box(fb, lay.box_x, lay.name_y, lay.box_w, lay.name_h, black, green);
	draw_menu_box(fb, lay.box_x, lay.help_y, lay.box_w, lay.help_h, black, red);
	draw_menu_box(fb, lay.box_x, lay.ctrl_y, lay.box_w, lay.ctrl_h, black, white);
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
	char dir[NAME_MAX];

	memset(&crt, 0, sizeof(crt));
	crt.app = app;
	crt.in_menu = 1;
	crt.show_help = 1;
	crt_reset_options();
	if(app) {
		GetAppPath(dir, sizeof(dir), app->fn);
		scroll_art_bind(dir);
	}
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
