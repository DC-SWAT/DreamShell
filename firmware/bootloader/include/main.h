#ifndef __MAIN_H
#define __MAIN_H

/* KOS */
#include <kos.h>
#include <math.h>
#include <assert.h>
#include <kmg/kmg.h>
#include <zlib/zlib.h>

/* spiral.c */
int spiral_init();
void spiral_frame();

/* menu.c */
int menu_init();
void init_menu_txr();
void menu_frame();

void loading_core(int no_thd);
int show_message(const char *fmt, ...);

int FileSize(const char *fn);
int FileExists(const char *fn);
int DirExists(const char *dir);

int flashrom_get_region_only();
void descramble(uint8 *source, uint8 *dest, uint32 size);

extern const char	title[28];
extern uint32 spiral_color;
extern int start_pressed;

#define RES_PATH "/brd"

#endif	/* __MAIN_H */
