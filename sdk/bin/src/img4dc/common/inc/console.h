#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// couleurs pour l'affichage
#define BLACK 			0
#define DARK_BLUE 		1
#define DARK_GREEN 		2
#define DARK_CYAN 		3
#define DARK_RED 		4
#define DARK_MAGENTA 	5
#define BROWN 			6
#define LIGHT_GRAY		7
#define DARK_GRAY 		8
#define LIGHT_BLUE 		9
#define LIGHT_GREEN 	10
#define LIGHT_CYAN 		11
#define LIGHT_RED 		12
#define LIGHT_MAGENTA 	13
#define YELLOW 			14
#define WHITE 			15

char * get_program_name();
void set_program_name(char *argv0);

void text_color(int color);

int printf_colored(int color, const char *format, ...);
int printf_stderr_colored(int color, const char *format, ...);

void gotoXY(int x, int y);
int whereX();
int whereY();

void get_full_filename(char *in, char *result);

#endif /* __CONSOLE_H__ */
