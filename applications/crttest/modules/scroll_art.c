/* CRT Test for DreamShell
   Copyright (C) 2026 HeadHunter

   Scroll pictures stay on disk and are read only while that chart is open.
*/

#include "scroll_art.h"

#include <ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SCROLL_CLOUD_W 512
#define SCROLL_CLOUD_H 128

unsigned char *scroll_near_rgb;
unsigned char *scroll_sky_rgb;
unsigned char *scroll_mid_rgb;
unsigned char *scroll_cloud_rgb;
unsigned char *scroll_floor_rgb;
unsigned char *scroll_vert_rgb;

static char art_dir[NAME_MAX];
static int art_loaded;
static int art_warned;

static const struct {
	unsigned char **slot;
	const char *name;
	int bytes;
} art_file[] = {
	{&scroll_near_rgb, "scroll_near.rgb", SCROLL_BACK_W * SCROLL_BACK_H * 3},
	{&scroll_sky_rgb, "scroll_sky.rgb", SCROLL_BACK_W * SCROLL_BACK_H * 3},
	{&scroll_mid_rgb, "scroll_mid.rgb", SCROLL_MID_W * SCROLL_MID_H * 3},
	{&scroll_cloud_rgb, "scroll_cloud.rgb", SCROLL_CLOUD_W * SCROLL_CLOUD_H * 3},
	{&scroll_floor_rgb, "scroll_floor.rgb", SCROLL_FLOOR_W * SCROLL_FLOOR_H * 3},
	{&scroll_vert_rgb, "scroll_vert.rgb", SCROLL_VERT_W * SCROLL_VERT_H * 3}
};

void scroll_art_bind(const char *dir) {
	if(!dir) {
		art_dir[0] = '\0';
		return;
	}
	snprintf(art_dir, sizeof(art_dir), "%s", dir);
}

void scroll_art_free(void) {
	int i;

	for(i = 0; i < (int)(sizeof(art_file) / sizeof(art_file[0])); i++) {
		free(*art_file[i].slot);
		*art_file[i].slot = NULL;
	}
	art_loaded = 0;
}

static int load_file(unsigned char **slot, const char *name, int bytes) {
	char path[NAME_MAX];
	unsigned char *buf;
	int fd;
	int got = 0;

	if(*slot) {
		return 1;
	}
	if(!art_dir[0] || bytes < 1) {
		return 0;
	}
	snprintf(path, sizeof(path), "%s/%s", art_dir, name);
	fd = open(path, O_RDONLY);
	if(fd < 0) {
		return 0;
	}
	buf = (unsigned char *)malloc((size_t)bytes);
	if(!buf) {
		close(fd);
		return 0;
	}
	while(got < bytes) {
		int n = read(fd, buf + got, (size_t)bytes - (size_t)got);

		if(n <= 0) {
			break;
		}
		got += n;
	}
	close(fd);
	if(got != bytes) {
		free(buf);
		return 0;
	}
	*slot = buf;
	return 1;
}

int scroll_art_ensure(void) {
	int i;

	if(art_loaded) {
		return 1;
	}
	for(i = 0; i < (int)(sizeof(art_file) / sizeof(art_file[0])); i++) {
		if(!load_file(art_file[i].slot, art_file[i].name, art_file[i].bytes)) {
			scroll_art_free();
			if(!art_warned) {
				art_warned = 1;
				ds_printf("DS_ERROR: CRT Test: scroll art was not opened\n");
			}
			return 0;
		}
	}
	art_loaded = 1;
	return 1;
}
