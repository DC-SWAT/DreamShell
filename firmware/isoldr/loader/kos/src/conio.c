/* KallistiOS ##version##

 conio.c

 (c)2002 Dan Potter

 Adapted from Kosh, (c)2000 Jordan DeLong, (c)2014 SWAT

*/

#include "main.h"

/* some defines */
#define CONIO_NUM_ROWS	7
#define CONIO_NUM_COLS	48

/* our cursor */
typedef struct { int row, col; } conio_cursor_t;

/* the cursor */
static conio_cursor_t conio_cursor;

/* the virtual screen */
static char conio_virtscr[CONIO_NUM_ROWS][CONIO_NUM_COLS];


void draw_virtscr() {
	int row, col, y = 1, x = 1;

	for (row = 0; row <= conio_cursor.row; row++) {
		for (col = 0; col < CONIO_NUM_COLS; col++) {
			bfont_draw(vram_s + (x*12) + ((y*24+4) * 640), vid_pixel(224,224,224), vid_pixel(0,0,0), conio_virtscr[row][col]);
			x++;
		}
		x = 1;
		y++;
	}
}


/* scroll everything up a line */
void conio_scroll() {
	int i;

	memcpy(conio_virtscr, conio_virtscr[1], (CONIO_NUM_ROWS - 1) * CONIO_NUM_COLS);
	for (i = 0; i < CONIO_NUM_COLS; i++)
		conio_virtscr[CONIO_NUM_ROWS - 1][i] = ' ';
	conio_cursor.row--;
}



/* put a character at the cursor and move the cursor */
void conio_putch(int ch) {

	switch (ch) {
		case '\r':
			conio_cursor.col = 0;
			for (int col = 0; col < CONIO_NUM_COLS; col++)
				conio_virtscr[conio_cursor.row][col] = ' ';
			break;
		case '\n':
			conio_cursor.row++;
			conio_cursor.col = 0;
			if (conio_cursor.row >= CONIO_NUM_ROWS)
				conio_scroll();
			break;
		default:
			conio_virtscr[conio_cursor.row][conio_cursor.col++] = ch;
			if(conio_cursor.col >= CONIO_NUM_COLS)
				conio_putch('\n');
	}

}

/* put a string of characters */
void conio_putstr(char *str) {
	while (*str != '\0') {
		conio_putch(*str++);
	}
}

/* a printfish function */
int conio_printf(const char *fmt, ...) {
	char buff[128];
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buff, sizeof(buff), fmt, args);
	conio_putstr(buff);
	va_end(args);
	
#ifdef LOG
	WriteLog(buff);
#endif

	if(IsoInfo->video_mode > 0) {
		draw_virtscr();
	}

	return i;
}

/* clear the screen */
void conio_clear() {
	int row, col;

	for (row = 0; row < CONIO_NUM_ROWS; row++)
		for (col = 0; col < CONIO_NUM_COLS; col++)
			conio_virtscr[row][col] = ' ';
}

