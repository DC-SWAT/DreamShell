/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#ifndef CRT_VIEW_H
#define CRT_VIEW_H

#include "draw.h"

void view_init(void *app);
void view_shutdown(void);
enum {
	CRT_PANEL_MENU = 0,
	CRT_PANEL_OPTIONS = 1,
	CRT_PANEL_SAVER = 2
};

void view_frame(int in_menu, int show_help, int menu_sel, int *menu_scroll,
	int pattern, int variant, int param, unsigned frame, const char *mode_line,
	int panel, int panel_sel, int music_off);
void crt_compose_chart(fb_t *fb, unsigned frame, int pattern, int variant, int param, int show_help);
uint16_t crt_compose_help(fb_t *fb);

#define CRT_HELP_LINE 4

typedef struct {
	int box_x;
	int box_w;
	int scale;
	int help_scale;
	int line_h;
	int arrow;
	int side;
	int name_y;
	int name_h;
	int help_y;
	int help_h;
	int ctrl_y;
	int ctrl_h;
	char head[96];
	char head2[96];
	char status[96];
	char help_line[CRT_HELP_LINE][96];
	int help_n;
	char ctrl[5][64];
} crt_overlay_t;

int crt_overlay_build(int w, int h, int pattern, int variant, int param, crt_overlay_t *out);

#endif
