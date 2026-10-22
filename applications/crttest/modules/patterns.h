/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#ifndef CRT_PATTERNS_H
#define CRT_PATTERNS_H

#include "draw.h"
#include "scroll_clouds.h"

#define CRT_PATTERN_MAX 32

int pattern_count(void);
const char *pattern_name(int id);
const char *pattern_help(int id, int variant, int param);
int pattern_is_convergence(int id);
void pattern_convergence_value(int variant, int *param, int dir);
int pattern_variant_count(int id);
int pattern_has_param(int id);
int pattern_param_default(int id);
void pattern_cycle_variant(int id, int *variant, int dir);
void pattern_cycle_param(int id, int *param, int dir);
void pattern_status(int id, int variant, int param, char *dst, int dst_len);
void pattern_draw(fb_t *fb, int id, unsigned frame, int variant, int param);
int pattern_animates(int id);
int pattern_is_grid_scroll(int id);
int pattern_is_shadow(int id);
int pattern_is_scroll(int id);
void pattern_grid_layout(unsigned frame, int param, int *step, int *offset);
void pattern_shadow_background(fb_t *fb, int variant);
void pattern_shadow_box(unsigned frame, int param, int w, int h,
	int *x, int *y, int *bw, int *bh);

typedef struct scroll_pose {
	int vertical;
	int scale;
	int phase;
	int floor_scroll;
	int vert_scroll;
	int sky_scroll;
	int mid_scroll;
	int near_scroll;
	int cloud_scroll[SCROLL_CLOUD_COUNT];
	int cloud_count;
	int back_src_w;
	int back_src_h;
	int floor_src_w;
	int floor_src_h;
	int vert_src_w;
	int vert_src_h;
} scroll_pose_t;

void pattern_scroll_pose(fb_t *fb, unsigned frame, int variant, int param, scroll_pose_t *pose);
const uint16_t *pattern_scroll_floor(void);
const uint16_t *pattern_scroll_vert(void);

#endif
