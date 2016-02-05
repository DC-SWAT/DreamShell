/****************************
 * DreamShell ##version##   *
 * vmu.c                    *
 * DreamShell VMU utils     *
 * Created by SWAT          *
 ****************************/

#include <kos.h>
#include <string.h>
#include "vmu_font.h"


inline void vmu_flip_bit(uint8 bitmap[192], uint8 x, uint8 y)
	/* (0, 0) is upper-left corner */
{
	if (x < 48 && y < 32)
		bitmap[6*(31-y)+(5-(x/8))] ^= (1 << (x%8));
}
/*
static void vmu_invert_bitmap(uint8 bitmap[192])
{
	int x, y;

	for (x = 0; x < 48; x++)
		for (y = 0; y < 32; y++)
			vmu_flip_bit(bitmap, x, y);
}*/

inline void vmu_set_bit(uint8 bitmap[192], uint8 x, uint8 y)
	/* (0, 0) is upper-left corner */
{
	if (x < 48 && y < 32)
		bitmap[6*(31-y)+(5-(x/8))] |= (1 << (x%8));
}

static void vmu_draw_char(uint8 bitmap[192], unsigned char c, int x, int y)
	/* (x, y) is position for upper-left corner of character,
		(0, 0) is upper-left corner of screen */
{
	int i, j;

	if (x < -4 || x > 47 || y < -9 || y > 31)
		return;

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 5; j++) {
			if (vmufont[(int)c][i] & (0x80 >> j))
				vmu_set_bit(bitmap, x+j, y+i);
		}
	}
}

static void vmu_draw_str(uint8 bitmap[192], unsigned char *str, int x, int y)
	/* (x, y) is position for upper-left corner of string,
		(0, 0) is upper-left corner of screen */
{
	int i;

	if (y < -9 || y > 31)
		return;
	
	for (i = 0; str[i] != '\0'; i++) {
		if (x < -4) {
			x += 5;
			continue;
		}
		vmu_draw_char(bitmap, str[i], x, y);
		x += 5;
		if (x > 47)
			break;
	}
}

void vmu_draw_string_xy(const char *str, int x, int y) {

	uint8 bitmap[192];
	maple_device_t *mvmu = NULL;

	mvmu = maple_enum_type(0, MAPLE_FUNC_LCD);

	if (mvmu) {
		memset(bitmap, 0, sizeof(bitmap));
		vmu_draw_str(bitmap, (uint8*)str, x, y);
		vmu_draw_lcd(mvmu, bitmap);
	}
}

void vmu_draw_string(const char *str) {
	
	int i;
	char msg[32];
	char *pmsg = msg;
	uint8 bitmap[192];
	maple_device_t *mvmu = NULL;

	int len = strlen(str);
	int c;
	int x;
	int y = len <= 10 ? 10 : 5;
	
	if(len > 20) {
		y = 0;
	}
	
	mvmu = maple_enum_type(0, MAPLE_FUNC_LCD);

	if (mvmu) {
		
		memset(bitmap, 0, sizeof(bitmap));
		// FIXME: ugly code
		
		if(len <= 10) {

			c = (10 - len);
			x = len >= 10 ? 0 : ((c / 2) * 5) + ((c % 2) ? 3 : 0);
			vmu_draw_str(bitmap, (uint8*)str, x, y);
			
		} else {
			
			strncpy(msg, str, sizeof(msg));
			for(i = 0; i < sizeof(msg); i++) {
				
				if(msg[i] == '\0') {

					len = strlen(pmsg);
					c = (10 - len);
					x = len >= 10 ? 0 : ((c / 2) * 5) + ((c % 2) ? 3 : 0);
					vmu_draw_str(bitmap, (uint8*)pmsg, x, y);
					break;
					
				} else if(msg[i] == ' ') {

					msg[i] = '\0';
					len = strlen(pmsg);
					c = (10 - len);
					x = len >= 10 ? 0 : ((c / 2) * 5) + ((c % 2) ? 3 : 0);
					vmu_draw_str(bitmap, (uint8*)pmsg, x, y);
					y += 10;
					pmsg += i + 1;
				}
			}
		}
		
		vmu_draw_lcd(mvmu, bitmap);
	}
}
