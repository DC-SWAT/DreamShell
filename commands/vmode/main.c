/* DreamShell ##version##

   vmode.c
   Copyright (C) 2011-2014 SWAT
*/

#include "ds.h"

int main(int argc, char *argv[]) { 
     
	if(argc < 2) {
		ds_printf("Usage: %s option args...\n\n"
					"Options: \n"
					" -m, --mode      - Set video mode\n"
					" -l, --list      - Show list of video modes\n"
					" -c, --clear     - Clear the display with color\n"
					" -e, --empty     - Clear VRAM\n"
					" -o, --border    - Set video border color\n\n"
					"Arguments: \n"
					" -d, --display   - Display mode from list\n"
					" -p, --pixel     - Pixel mode from list\n"
					" -r, --red       - Red color for clear screen\n"
					" -g, --green     - green color for clear screen\n"
					" -b, --blue      - Blue color for clear screen\n\n"
					"Example: %s -m -d 1 -p 1\n", argv[0], argv[0]);
		return CMD_NO_ARG; 
	} 

	int mode = 0, list = 0, clear = 0, empty = 0, border = 0, display = 0, pixel = PM_RGB565;
	int r = 0, g = 0, b = 0;

	struct cfg_option options[] = {
		{"mode",   'm', NULL, CFG_BOOL, (void *) &mode,   0},
		{"clear",  'c', NULL, CFG_BOOL, (void *) &clear,  0},
		{"empty",  'e', NULL, CFG_BOOL, (void *) &empty,  0},
		{"border", 'o', NULL, CFG_BOOL, (void *) &border, 0},
		{"list",   'l', NULL, CFG_BOOL, (void *) &list,   0},
		{"pixel",  'p', NULL, CFG_INT,  (void *) &pixel,  0},
		{"display",'d', NULL, CFG_INT,  (void *) &display,0},
		{"red",    'r', NULL, CFG_INT,  (void *) &r,      0},
		{"green",  'g', NULL, CFG_INT,  (void *) &g,      0},
		{"blue",   'b', NULL, CFG_INT,  (void *) &b,      0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(mode) {
		if(pixel == PM_RGB565) {
			SetVideoMode(display);
		} else {
			vid_set_mode(display, pixel);
		}
		return CMD_OK;
	}
	
	if(clear) {
		vid_clear(r, g, b);
		return CMD_OK;
	}
	
	if(empty) {
		vid_empty();
		return CMD_OK;
	}
	
	if(border) {
		vid_border_color(r, g, b);
		return CMD_OK;
	}
	
	if(list) {
		ds_printf(	" Pixel modes: \n"
					"  0 - RGB555 pixel mode (15-bit)\n"
					"  1 - RGB565 pixel mode (16-bit), default\n"
					"  3 - RGB888 pixel mode (24-bit)\n\n");
					
		ds_printf(	" Display modes: \n"
					"  1  - 320x240 VGA 60Hz \n"
					"  2  - 320x240 NTSC 60Hz \n"
					"  3  - 640x480 VGA 60Hz \n"
					"  4  - 640x480 NTSC Interlaced 60Hz \n"
					"  5  - 800x608 VGA 60Hz \n"
					"  6  - 640x480 PAL Interlaced 50Hz \n"
					"  7  - 256x256 PAL Interlaced 50Hz \n"
					"  8  - 768x480 NTSC Interlaced 60Hz \n"
					"  9  - 768x576 PAL Interlaced 50Hz \n"
					"  10 - 768x480 PAL Interlaced 50Hz \n");
		ds_printf(	"  11 - 320x240 PAL 50Hz \n"
					"  12 - 320x240 VGA 60Hz, 4FBs \n"
					"  13 - 320x240 NTSC 60Hz, 4FBs \n"
					"  14 - 640x480 VGA 60Hz, 4FBs \n"
					"  15 - 640x480 NTSC IL 60Hz, 4FBs \n"
					"  16 - 800x608 VGA 60Hz, 4FBs \n"
					"  17 - 640x480 PAL IL 50Hz, 4FBs \n"
					"  18 - 256x256 PAL IL 50Hz, 4FBs \n"
					"  19 - 768x480 NTSC IL 60Hz, 4FBs \n"
					"  20 - 768x576 PAL IL 50Hz, 4FBs \n"
					"  21 - 768x480 PAL IL 50Hz, 4FBs \n"
					"  22 - 320x240 PAL 50Hz, 4FBs \n\n");
		return CMD_OK;
	}

	ds_printf("DS_ERROR: There is no option.\n");
	return CMD_NO_ARG;
} 

