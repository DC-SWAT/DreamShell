/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter
*/

#ifndef CRT_SCROLL_ART_H
#define CRT_SCROLL_ART_H

#define SCROLL_BACK_W 640
#define SCROLL_BACK_H 240
#define SCROLL_FLOOR_W 640
#define SCROLL_FLOOR_H 240
#define SCROLL_VERT_W 320
#define SCROLL_VERT_H 196
#define SCROLL_FALL_X0 137
#define SCROLL_FALL_Y0 142
#define SCROLL_FALL_X1 188
#define SCROLL_FALL_Y1 211
#define SCROLL_FALL2_X0 452
#define SCROLL_FALL2_Y0 142
#define SCROLL_FALL2_X1 503
#define SCROLL_FALL2_Y1 211
#define SCROLL_SHAFT_Y0 150
#define SCROLL_SHAFT_Y1 190
#define SCROLL_MID_W 640
#define SCROLL_MID_H 240
#define SCROLL_MID_Y 0
#define SCROLL_MID_CROP_Y 85
#define SCROLL_MID_CROP_H 155
#define SCROLL_NEAR_CROP_Y 110
#define SCROLL_NEAR_CROP_H 130
#define SCROLL_FLOOR_CROP_Y 171
#define SCROLL_FLOOR_CROP_H 64

static inline int scroll_shaft_row(int y, int shift) {
	int span = SCROLL_SHAFT_Y1 - SCROLL_SHAFT_Y0;
	int period = span * 2;
	int pos;

	if(span < 1 || y < SCROLL_SHAFT_Y0 || y >= SCROLL_SHAFT_Y1) {
		return y;
	}
	pos = y - SCROLL_SHAFT_Y0 - shift;
	pos %= period;
	if(pos < 0) {
		pos += period;
	}
	if(pos >= span) {
		pos = period - 1 - pos;
	}
	return SCROLL_SHAFT_Y0 + pos;
}

extern const unsigned char scroll_near_rgb[];
extern const unsigned char scroll_sky_rgb[];
extern const unsigned char scroll_mid_rgb[];
extern const unsigned char scroll_cloud_rgb[];
extern const unsigned char scroll_floor_rgb[];
extern const unsigned char scroll_vert_rgb[];

#endif
