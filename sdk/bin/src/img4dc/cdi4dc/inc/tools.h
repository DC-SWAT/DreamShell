/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	13 april 2007, v0.3b
	
	File : 	TOOLS.H
	Desc : 	Header for tools.c, it contains the colors code for Win32
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#ifndef __TOOLS__H__
#define __TOOLS__H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

void fill_buffer(unsigned char *buf, int total_size, int values_array_size, const unsigned int values_array[][2]);
uint32_t fsize(FILE *stream);
void get_volumename(FILE* iso, char* volume_name);
int check_iso_is_bootable(FILE* iso);
uint32_t get_total_cdi_space_used(uint32_t data_sectors_count);
int get_iso_msinfo_value(FILE* iso);
void write_null_block(FILE *cdi, int size);
void write_array_block(FILE* cdi, int array_size, const int array_entries, const unsigned int values_array[][2]);
void get_full_filename(char* in, char* result);
void gotoXY(int x, int y);
void textColor(int color);
int whereX();
int whereY();

#endif //__TOOLS__H__
