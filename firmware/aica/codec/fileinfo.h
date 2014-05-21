/*
  Copyright (C) 2006 Andreas Schwarz <andreas@andreas-s.net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _FILEINFO_H_
#define _FILEINFO_H_

#define MAX_SONGS 50

enum filetypes {WAV, MP2, MP3, MP4, AAC, UNKNOWN=0};

typedef struct _SONGFILE {
  char  filename[12]; // 8.3 format, not null terminated!
} SONGFILE;

typedef struct _SONGLIST {
  SONGFILE list[MAX_SONGS];
  unsigned int size;
} SONGLIST;

typedef struct _SONGINFO {
	char	title[40];
	char	artist[40];
	char	album[40];
	unsigned int	data_start;
  enum filetypes  type;
} SONGINFO;

void songlist_build(SONGLIST *songlist);
char* skip_artist_prefix(char* s);
void songlist_sort(SONGLIST *songlist);
int compar_song(SONGFILE *a, SONGFILE *b);
enum filetypes get_filetype(char * filename);
char * get_full_filename(char * filename);
int read_song_info_for_song(SONGFILE *song, SONGINFO *songinfo);
int read_song_info_mp3(FIL *file, SONGINFO *songinfo);
int read_song_info_mp4(FIL *file, SONGINFO *songinfo);

#endif /* _FILEINFO_H_ */
