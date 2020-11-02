#ifndef __IMGWRITE__H__
#define __IMGWRITE__H__

#include "console.h"

typedef struct AUDIOTRACKINFOS {
	int lba;
	int sectors_count;
} AUDIOTRACKINFOS;

void write_audio_data_image(FILE* mds, FILE* mdf, FILE* iso, int cdda_tracks_count, char* audio_files_array[]);
void write_data_data_image(FILE* mds, FILE* mdf, FILE* iso);

#endif // __IMGWRITE__H__
