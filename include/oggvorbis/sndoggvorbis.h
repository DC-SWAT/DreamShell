/* KallistiOS ##version##

   sndoggvorbis.h
   (c)2001 Thorsten Titze

   An Ogg/Vorbis player library using sndstream and the official Xiphophorus
   libogg and libvorbis libraries.
*/

#ifndef __SNDOGGVORBIS_H
#define __SNDOGGVORBIS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdio.h>

int sndoggvorbis_init(void);
int sndoggvorbis_start(const char *filename, int loop);
int sndoggvorbis_start_fd(FILE *f, int loop);
void sndoggvorbis_stop(void);
void sndoggvorbis_shutdown(void);

int sndoggvorbis_isplaying(void);

void sndoggvorbis_volume(int vol);

void sndoggvorbis_mainloop(void);
void sndoggvorbis_wait_start(void);

void sndoggvorbis_setbitrateinterval(int interval);
long sndoggvorbis_getbitrate(void);
long sndoggvorbis_getposition(void);

char *sndoggvorbis_getcommentbyname(const char *commentfield);
char *sndoggvorbis_getartist(void);
char *sndoggvorbis_gettitle(void);
char *sndoggvorbis_getgenre(void);

void sndoggvorbis_queue_enable(void);
void sndoggvorbis_queue_disable(void);
void sndoggvorbis_queue_wait(void);
void sndoggvorbis_queue_go(void);

__END_DECLS

#endif	/* __SNDOGGVORBIS_H */
