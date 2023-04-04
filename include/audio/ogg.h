/* KallistiOS Ogg/Vorbis Decoder Library
 * for KOS ##version##
 *
 * sndoggvorbis.c
 * Copyright (C)2001,2002 Thorsten Titze
 * Copyright (C)2002,2003,2004 Dan Potter
 *
 * An Ogg/Vorbis player library using sndstream and functions provided by
 * ivorbisfile (Tremor).
 */

#include <kos.h>
#include <vorbis/vorbisfile.h>

int sndoggvorbis_init();
void sndoggvorbis_shutdown();
void sndoggvorbis_thd_quit();

/* Enable/disable queued waiting */
void sndoggvorbis_queue_enable();
void sndoggvorbis_queue_disable();

/* Wait for the song to be queued */
void sndoggvorbis_queue_wait();

/* Queue the song to start if it's in QUEUED */
void sndoggvorbis_queue_go();

/* getter and setter functions for information access 
 */

int sndoggvorbis_isplaying();

void sndoggvorbis_setbitrateinterval(int interval);

/* sndoggvorbis_getbitrate()
 *
 * returns the actual bitrate of the playing stream. 
 *
 * NOTE:
 * The value returned is only actualized every once in a while !
 */
long sndoggvorbis_getbitrate();

long sndoggvorbis_getposition();

/* The following getters only return the contents of specific comment
 * fields. It is thinkable that these return something like "NOT SET"
 * in case the specified field has not been set !
 */
char *sndoggvorbis_getartist();
char *sndoggvorbis_gettitle();
char *sndoggvorbis_getgenre();

char *sndoggvorbis_getcommentbyname(const char *commentfield);

/* sndoggvorbis_wait_start()
 *
 * let's the caller wait until the vorbis thread is signalling that it is ready
 * to decode data
 */
void sndoggvorbis_wait_start();

/* sndoggvorbis_thread()
 *
 * this function is called by sndoggvorbis_mainloop and handles all the threads
 * status handling and playing functionality.
 */
void sndoggvorbis_thread();

/* sndoggvorbis_stop()
 *
 * function to stop the current playback and set the thread back to
 * STATUS_READY mode.
 */
void sndoggvorbis_stop();

/* Same as sndoggvorbis_start, but takes an already-opened file. Once a file is
   passed into this function, we assume that _we_ own it. */
int sndoggvorbis_start_fd(FILE * fd, int loop);

/* sndoggvorbis_start(...)
 *
 * function to start playback of a Ogg/Vorbis file. Expects a filename to
 * a file in the KOS filesystem.
 */
int sndoggvorbis_start(const char *filename,int loop);

/* sndoggvorbis_shutdown()
 *
 * function that stops playing and shuts down the player thread.
 */
void sndoggvorbis_thd_quit();

/* sndoggvorbis_volume(...)
 *
 * function to set the volume of the streaming channel
 */
void sndoggvorbis_volume(int vol);
	
/* sndoggvorbis_mainloop()
 *
 * code that runs in our decoding thread. sets up the semaphore. initializes the stream
 * driver and calls our *real* thread code
 */
void sndoggvorbis_mainloop();
