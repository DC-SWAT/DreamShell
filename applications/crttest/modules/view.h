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

#endif
