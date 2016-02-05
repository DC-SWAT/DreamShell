/* KallistiOS ##version##

   biosfont.c

   Copyright (C)2000-2002,2004 Dan Potter
 */

#include <main.h>
#include <dc/biosfont.h>

/*

This module handles interfacing to the bios font. It supports the standard
European encodings via ISO8859-1.

Thanks to Marcus Comstedt for the bios font information.

*/

/* A little assembly that grabs the font address */
//extern uint8* get_font_address();
//__asm__("	.text\n"
//		"	.align 2\n"
//		"_get_font_address:\n"
//		"	mov.l	syscall_b4,r0\n"
//		"	mov.l	@r0,r0\n"
//		"	jmp	@r0\n"
//		"	mov	#0,r1\n"
//		"\n"  
//		"	.align 4\n"
//		"syscall_b4:\n"
//		"	.long	0x8c0000b4\n"
//);

/* Given an ASCII character, find it in the BIOS font if possible */
uint8 *bfont_find_char(int ch) {
	int	index = -1;
	uint8	*fa = (uint8*)bfont_saved_addr; //get_font_address();
	
	/* 33-126 in ASCII are 1-94 in the font */
	if (ch >= 33 && ch <= 126)
		index = ch - 32;
	
	/* 160-255 in ASCII are 96-161 in the font */
	if (ch >= 160 && ch <= 255)
		index = ch - (160 - 96);
	
	/* Map anything else to a space */
	if (index == -1)
		index = 72 << 2;

	return fa + index*36;
}

/* Draw char */
void bfont_draw(uint16 *buffer, uint16 fg, uint16 bg, int c) {
	uint8	*ch;
	uint16	word;
	int	x, y;

	const int bufwidth = 640;

	ch = bfont_find_char(c);

	for (y=0; y<24; ) {
		/* Do the first row */
		word = (((uint16)ch[0]) << 4) | ((ch[1] >> 4) & 0x0f);
		for (x=0; x<12; x++) {
			if (word & (0x0800 >> x)) {
				*buffer = fg;
				*(buffer - 1) = fg;
			} else {
				*buffer = bg;
			}
			buffer++;
		}
		buffer += bufwidth - 12;
		y++;

		/* Do the second row */
		word = ( (((uint16)ch[1]) << 8) & 0xf00 ) | ch[2];
		for (x=0; x<12; x++) {
			if (word & (0x0800 >> x)) {
				*buffer = fg;
				*(buffer - 1) = fg;
			} else {
				*buffer = bg;
			}
			buffer++;
		}
		buffer += bufwidth - 12;
		y++;

		ch += 3;
	}
}

void bfont_draw_str(uint16 *buffer, uint16 fg, uint16 bg, const char *str) {
	while (*str) {
		bfont_draw(buffer, fg, bg, *str);

		str++;
		buffer += 12;
	}
}

