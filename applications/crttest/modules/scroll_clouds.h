/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter

   Cloud slots for the five user sprites. */
#ifndef CRT_SCROLL_CLOUDS_H
#define CRT_SCROLL_CLOUDS_H

#define SCROLL_CLOUD_COUNT 5
#define SCROLL_RATE_DEN 32
#define SCROLL_RATE_SKY 4
#define SCROLL_RATE_MID 16
#define SCROLL_RATE_NEAR 32
#define SCROLL_RATE_FLOOR 64
#define SCROLL_RATE_VERT 32

typedef struct scroll_cloud_def {
	short x;
	short y;
	short w;
	short h;
	short ax;
	short ay;
	unsigned char num;
} scroll_cloud_def_t;

static const scroll_cloud_def_t scroll_clouds[SCROLL_CLOUD_COUNT] = {
	{250, 6, 178, 57, 0, 0, 4},
	{8, 2, 213, 66, 179, 0, 5},
	{430, 52, 92, 31, 393, 0, 2},
	{576, 48, 61, 26, 0, 67, 3},
	{450, 4, 124, 40, 62, 67, 8},
};

#endif

