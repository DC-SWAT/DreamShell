#ifndef __TOOLS__H__
#define __TOOLS__H__

#include "mdfwrt.h"

void fill_buffer(unsigned char *buf, int total_size, int values_array_size, const unsigned int values_array[][2]);
void write_null_block(FILE *target, int size);
void write_array_block(FILE* target, int array_size, const int array_entries, const unsigned int values_array[][2]);
int lba_2_msf(long lba, int *m, int* s, int* f);
uint32_t fsize(FILE *stream);
unsigned int int_2_inthex(int dec);
unsigned int get_total_space_used_bytes(int cdda_session_sectors_count, int data_session_sectors_count);
unsigned int get_total_space_used_blocks(int cdda_session_sectors_count, int data_session_sectors_count);
void get_volumename(FILE* iso, char* volume_name);
char* check_ext(char* in, char* ext);
char* get_friendly_unit(float* size);
int check_iso_is_bootable(FILE* iso);
char* extract_proggyname(char* in);
void warning_msg(char* msg);
void info_msg(char* msg);
void start_progressbar();
void padding_event(int sector_count);
void writing_track_event(uint32_t current, uint32_t total);
void writing_data_track_event(uint32_t current_pos, uint32_t total_iso_size);
void writing_track_event_end(uint32_t block_count, uint32_t track_size);

#endif //__TOOLS__H__
