#ifndef __TOOLS__H__
#define __TOOLS__H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// couleurs pour l'affichage
#ifdef WIN32

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

#else

#define BLACK 			0
#define DARK_BLUE		4
#define DARK_GREEN		2
#define DARK_CYAN		6
#define DARK_RED		1
#define DARK_MAGENTA	5
#define BROWN			1
#define LIGHT_GRAY		0
#define DARK_GRAY		7
#define LIGHT_BLUE		4
#define LIGHT_GREEN		2
#define LIGHT_CYAN		6
#define LIGHT_RED		1
#define LIGHT_MAGENTA	5
#define YELLOW			3
#define	WHITE			8

#endif

void textColor(int color);
char* extract_proggyname(char* in);

#endif //__TOOLS__H__
