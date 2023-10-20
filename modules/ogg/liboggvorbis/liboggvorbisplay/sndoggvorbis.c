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
/* #include <sndserver.h> */
#include <assert.h>
#include <vorbis/vorbisfile.h>
#include "misc.h"

/* Enable this #define to do timing testing */
/* #define TIMING_TESTS */

#define BUF_SIZE 65536			/* Size of buffer */

static uint8 pcm_buffer[BUF_SIZE+16384] __attribute__((aligned(32)));	/* complete buffer + 16KB safety */
static uint8 *pcm_ptr=pcm_buffer;		/* place we write to */

static int32 pcm_count=0;			/* bytes in buffer */
static int32 last_read=0;			/* number of bytes the sndstream driver grabbed at last callback */

static int tempcounter =0;

static snd_stream_hnd_t stream_hnd = SND_STREAM_INVALID;

/* liboggvorbis Related Variables */
static OggVorbis_File	vf;

/* Since we're only providing this library for single-file playback it should
 * make no difference if we define an info-storage here where the user can
 * get essential information about his file.
 */
static VorbisFile_info_t sndoggvorbis_info;

/* Thread-state related defines */
#define STATUS_INIT	0
#define STATUS_READY	1
#define STATUS_STARTING	2
#define STATUS_PLAYING	3
#define STATUS_STOPPING	4
#define STATUS_QUIT	5
#define STATUS_ZOMBIE	6
#define STATUS_REINIT	7
#define STATUS_RESTART	8
#define STATUS_LOADING	9
#define STATUS_QUEUEING	10
#define STATUS_QUEUED	11

#define BITRATE_MANUAL	-1

/* Library status related variables */
static int sndoggvorbis_queue_enabled;		/* wait in STATUS_QUEUED? */
static volatile int sndoggvorbis_loop;		/* current looping mode */
static volatile int sndoggvorbis_status;	/* current status of thread */
static volatile int sndoggvorbis_bitrateint;	/* bitrateinterval in calls */
static semaphore_t *sndoggvorbis_halt_sem;	/* semaphore to pause thread */
static char sndoggvorbis_lastfilename[256];	/* filename of last played file */
static int current_section;
static int sndoggvorbis_vol = 240;

/* Enable/disable queued waiting */
void sndoggvorbis_queue_enable() {
	sndoggvorbis_queue_enabled = 1;
}
void sndoggvorbis_queue_disable() {
	sndoggvorbis_queue_enabled = 0;
}

/* Wait for the song to be queued */
void sndoggvorbis_queue_wait() {
	assert(sndoggvorbis_queue_wait);

	/* Make sure we've loaded ok */
	while (sndoggvorbis_status != STATUS_QUEUED)
		thd_pass();
}

/* Queue the song to start if it's in QUEUED */
void sndoggvorbis_queue_go() {
	/* Make sure we're ready */
	sndoggvorbis_queue_wait();

	/* Tell it to go */
	sndoggvorbis_status = STATUS_STARTING;
	sem_signal(sndoggvorbis_halt_sem);
}

/* getter and setter functions for information access 
 */

int sndoggvorbis_isplaying()
{
	if((sndoggvorbis_status == STATUS_PLAYING) || 
	   (sndoggvorbis_status == STATUS_STARTING) ||
	   (sndoggvorbis_status == STATUS_QUEUEING) ||
	   (sndoggvorbis_status == STATUS_QUEUED)) 
	{
		return(1);
	}
	return(0);
}

void sndoggvorbis_setbitrateinterval(int interval)
{
	sndoggvorbis_bitrateint=interval;

	/* Just in case we're already counting above interval
	 * reset the counter
	 */
	tempcounter = 0; 
}

/* sndoggvorbis_getbitrate()
 *
 * returns the actual bitrate of the playing stream. 
 *
 * NOTE:
 * The value returned is only actualized every once in a while !
 */
long sndoggvorbis_getbitrate()
{
	return(sndoggvorbis_info.actualbitrate);
	// return(VorbisFile_getBitrateInstant());
}

long sndoggvorbis_getposition()
{
	return(sndoggvorbis_info.actualposition);
}

/* The following getters only return the contents of specific comment
 * fields. It is thinkable that these return something like "NOT SET"
 * in case the specified field has not been set !
 */
char *sndoggvorbis_getartist()
{
	return(sndoggvorbis_info.artist);
}
char *sndoggvorbis_gettitle()
{
	return(sndoggvorbis_info.title);
}
char *sndoggvorbis_getgenre()
{
	return(sndoggvorbis_info.genre);
}

char *sndoggvorbis_getcommentbyname(const char *commentfield)
{
	dbglog(DBG_ERROR, "sndoggvorbis: getcommentbyname not implemented for tremor yet\n");
	return NULL;
}


static void sndoggvorbis_clear_comments() {
	sndoggvorbis_info.artist=NULL;
	sndoggvorbis_info.title=NULL;
	sndoggvorbis_info.album=NULL;
	sndoggvorbis_info.tracknumber=NULL;
	sndoggvorbis_info.organization=NULL;
	sndoggvorbis_info.description=NULL;
	sndoggvorbis_info.genre=NULL;
	sndoggvorbis_info.date=NULL;
	sndoggvorbis_info.location=NULL;
	sndoggvorbis_info.copyright=NULL;
	sndoggvorbis_info.isrc=NULL;
	sndoggvorbis_info.filename=NULL;

	sndoggvorbis_info.nominalbitrate=0;
	sndoggvorbis_info.actualbitrate=0;

	sndoggvorbis_info.actualposition=0;
}
	
/* main functions controlling the thread
 */


/* sndoggvorbis_wait_start()
 *
 * let's the caller wait until the vorbis thread is signalling that it is ready
 * to decode data
 */
void sndoggvorbis_wait_start()
{
	while(sndoggvorbis_status != STATUS_READY)
		thd_pass();
}

/* *callback(...)
 *
 * this procedure is called by the streaming server when it needs more PCM data
 * for internal buffering
 */
static void *callback(snd_stream_hnd_t hnd, int size, int * size_out)
{
#ifdef TIMING_TESTS
	int decoded_bytes = 0;
	uint32 s_s, s_ms, e_s, e_ms;
#endif
	
	// printf("sndoggvorbis: callback requested %d bytes\n",size);

	/* Check if the callback requests more data than our buffer can hold */
	if (size > BUF_SIZE)
		size = BUF_SIZE;

#ifdef TIMING_TESTS
	timer_ms_gettime(&s_s, &s_ms);
#endif

	/* Shift the last data the AICA driver took out of the PCM Buffer */
	if (last_read > 0) {
		pcm_count -= last_read;
		if (pcm_count < 0)
			pcm_count=0; /* Don't underrun */
		/* printf("memcpy(%08x, %08x, %d (%d - %d))\n",
			pcm_buffer, pcm_buffer+last_read, pcm_count-last_read, pcm_count, last_read); */
		memcpy(pcm_buffer, pcm_buffer+last_read, pcm_count);
		pcm_ptr = pcm_buffer + pcm_count;
	}
	
	/* If our buffer has not enough data to fulfill the callback decode 4 KB
	 * chunks until we have enough PCM samples buffered
	 */
	// printf("sndoggvorbis: fill buffer if not enough data\n");
	long pcm_decoded = 0;
	while(pcm_count < size) {
		//int pcm_decoded =  VorbisFile_decodePCMint8(v_headers, pcm_ptr, 4096);
		pcm_decoded = ov_read(&vf, pcm_ptr, 4096, 0, 2, 1, &current_section);

		/* Are we at the end of the stream? If so and looping is
		   enabled, then go ahead and seek back to the first. */
		if (pcm_decoded == 0 && sndoggvorbis_loop) {
			/* Seek back */
			if (ov_raw_seek(&vf, 0) < 0) {
				dbglog(DBG_ERROR, "sndoggvorbis: can't ov_raw_seek to the beginning!\n");
			} else {
				/* Try again */
				pcm_decoded = ov_read(&vf, pcm_ptr, 4096, 0, 2, 1, &current_section);
			}
		}
		
#ifdef TIMING_TESTS
		decoded_bytes += pcm_decoded;
#endif
		// printf("pcm_decoded is %d\n", pcm_decoded);
		pcm_count += pcm_decoded;
		pcm_ptr += pcm_decoded;

		//if (pcm_decoded == 0 && check_for_reopen() < 0)
		//	break;
		if (pcm_decoded == 0)
			break;
	}
	if (pcm_count > size)
		*size_out = size;
	else
		*size_out = pcm_count;

	/* Let the next callback know how many bytes the last callback grabbed
	 */
	last_read = *size_out;

#ifdef TIMING_TESTS
	if (decoded_bytes != 0) {
		int t;
		timer_ms_gettime(&e_s, &e_ms);
		t = ((int)e_ms - (int)s_ms) + ((int)e_s - (int)s_s) * 1000;
		printf("%dms for %d bytes (%.2fms / byte)\n", t, decoded_bytes, t*1.0f/decoded_bytes);
	}
#endif

	// printf("sndoggvorbis: callback: returning\n");

	if (pcm_decoded == 0)
		return NULL;
	else
		return pcm_buffer;
}

/* sndoggvorbis_thread()
 *
 * this function is called by sndoggvorbis_mainloop and handles all the threads
 * status handling and playing functionality.
 */
void sndoggvorbis_thread() 
{	
	int stat;

	stream_hnd = snd_stream_alloc(NULL, SND_STREAM_BUFFER_MAX);
	assert( stream_hnd != SND_STREAM_INVALID );
	
	while(sndoggvorbis_status != STATUS_QUIT)
	{
		switch(sndoggvorbis_status)
		{
			case STATUS_INIT:
				sndoggvorbis_status= STATUS_READY;
				break;
				
			case STATUS_READY:
				printf("oggthread: waiting on semaphore\n");
				sem_wait(sndoggvorbis_halt_sem);
				printf("oggthread: released from semaphore (status=%d)\n", sndoggvorbis_status);
				break;
				
			case STATUS_QUEUEING: {
				vorbis_info * vi = ov_info(&vf, -1);

				snd_stream_reinit(stream_hnd, callback);
				snd_stream_queue_enable(stream_hnd);
				printf("oggthread: stream_init called\n");
				snd_stream_start(stream_hnd, vi->rate, vi->channels - 1);
				snd_stream_volume(stream_hnd, sndoggvorbis_vol);
				printf("oggthread: stream_start called\n");
				if (sndoggvorbis_status != STATUS_STOPPING)
					sndoggvorbis_status = STATUS_QUEUED;
				break;
			}
				
			case STATUS_QUEUED:
				printf("oggthread: queue waiting on semaphore\n");
				sem_wait(sndoggvorbis_halt_sem);
				printf("oggthread: queue released from semaphore\n");
				break;

			case STATUS_STARTING: {
				vorbis_info * vi = ov_info(&vf, -1);
				
				if (sndoggvorbis_queue_enabled) {
					snd_stream_queue_go(stream_hnd);
				} else {
					snd_stream_reinit(stream_hnd, callback);
					printf("oggthread: stream_init called\n");
					snd_stream_start(stream_hnd, vi->rate, vi->channels - 1);
					printf("oggthread: stream_start called\n");
				}
				snd_stream_volume(stream_hnd, sndoggvorbis_vol);
				sndoggvorbis_status=STATUS_PLAYING;
				break;
			}

			case STATUS_PLAYING:
				/* Preliminary Bitrate Code 
				 * For our tests the bitrate is being set in the struct once in a
				 * while so that the user can just read out this value from the struct
				 * and use it for whatever purpose
				 */
				/* if (tempcounter==sndoggvorbis_bitrateint)
				{
					long test;
					if((test=VorbisFile_getBitrateInstant()) != -1)
					{
						sndoggvorbis_info.actualbitrate=test;
					}
					tempcounter = 0;
				}
				tempcounter++; */

				/* Stream Polling and end-of-stream detection */
				if ( (stat = snd_stream_poll(stream_hnd)) < 0)
				{
					printf("oggthread: stream ended (status %d)\n", stat);
					printf("oggthread: not restarting\n");
					snd_stream_stop(stream_hnd);

					/* Reset our PCM buffer */
					pcm_count = 0;
					last_read = 0;
					pcm_ptr = pcm_buffer;

					/* This can also happen sometimes when you stop the stream
					   manually, so we'll call this here to make sure */
					ov_clear(&vf);
					sndoggvorbis_lastfilename[0] = 0;
					sndoggvorbis_status = STATUS_READY;
					sndoggvorbis_clear_comments();
				} else {
					/* Sleep some until the next buffer is needed */
					thd_sleep(50);
				}
				break;

			case STATUS_STOPPING:
				snd_stream_stop(stream_hnd);
				ov_clear(&vf);
				/* Reset our PCM buffer */
				pcm_count = 0;
				last_read = 0;
				pcm_ptr = pcm_buffer;
				
				sndoggvorbis_lastfilename[0] = 0;
				sndoggvorbis_status = STATUS_READY;
				sndoggvorbis_clear_comments();
				break;
		}
	}

	snd_stream_stop(stream_hnd);
	snd_stream_destroy(stream_hnd);
	sndoggvorbis_status=STATUS_ZOMBIE;

	printf("oggthread: thread released\n");
} 

/* sndoggvorbis_stop()
 *
 * function to stop the current playback and set the thread back to
 * STATUS_READY mode.
 */
void sndoggvorbis_stop()
{
	if (sndoggvorbis_status != STATUS_PLAYING
		&& sndoggvorbis_status != STATUS_STARTING
		&& sndoggvorbis_status != STATUS_QUEUEING
		&& sndoggvorbis_status != STATUS_QUEUED)
	{
		return;
	}

	/* Wait for the thread to finish starting */
	while (sndoggvorbis_status == STATUS_STARTING)
		thd_pass();

	sndoggvorbis_status = STATUS_STOPPING;

	/* Wait until thread is at STATUS_READY before giving control
	 * back to the MAIN
	 */
	while(sndoggvorbis_status != STATUS_READY)
	{
		thd_pass();
	}
}

static char *vc_get_comment(vorbis_comment * vc, const char *commentfield) {
	int i;
	int commentlength = strlen(commentfield);

	for (i=0; i<vc->comments; i++) {
		if (!(strncmp(commentfield, vc->user_comments[i], commentlength))) {
			/* Return adress of comment content but leave out the
			 * commentname= part !
			 */
			return vc->user_comments[i] + commentlength + 1;
		}
	}
	return NULL;
}


/* Same as sndoggvorbis_start, but takes an already-opened file. Once a file is
   passed into this function, we assume that _we_ own it. */
int sndoggvorbis_start_fd(FILE * fd, int loop)
{
	vorbis_comment *vc;
	
	/* If we are already playing or just loading a new file
	 * we can't start another one !
	 */
	if (sndoggvorbis_status != STATUS_READY)
	{
		fclose(fd);
		return -1;
	}
	
	/* Initialize liboggvorbis. Try to open file and grab the headers
	 * from the bitstream
	 */

	printf("oggthread: initializing and parsing ogg\n");

	if(ov_open(fd, &vf, NULL, 0) < 0) {
		printf("sndoggvorbis: input does not appear to be an Ogg bitstream\n");
		fclose(fd);
		return -1;
	}
	
	sndoggvorbis_loop = loop;
	strcpy(sndoggvorbis_lastfilename, "<stream>");

	if (sndoggvorbis_queue_enabled)
		sndoggvorbis_status = STATUS_QUEUEING;
	else
		sndoggvorbis_status = STATUS_STARTING;
	sem_signal(sndoggvorbis_halt_sem);

	/* Grab all standard comments from the file
	 * (based on v-comment.html found in OggVorbis source packages
	 */
	vc = ov_comment(&vf, -1);
	if (vc == NULL) {
		dbglog(DBG_WARNING, "sndoggvorbis: can't obtain bitstream comments\n");
		sndoggvorbis_clear_comments();
	} else {
		sndoggvorbis_info.artist = vc_get_comment(vc, "artist");
		sndoggvorbis_info.title = vc_get_comment(vc, "title");
		sndoggvorbis_info.version = vc_get_comment(vc, "version");
		sndoggvorbis_info.album = vc_get_comment(vc, "album");
		sndoggvorbis_info.tracknumber = vc_get_comment(vc, "tracknumber");
		sndoggvorbis_info.organization = vc_get_comment(vc, "organization");
		sndoggvorbis_info.description = vc_get_comment(vc, "description");
		sndoggvorbis_info.genre = vc_get_comment(vc, "genre");
		sndoggvorbis_info.date = vc_get_comment(vc, "date");
		sndoggvorbis_info.filename = sndoggvorbis_lastfilename;
	}

	return 0;
}

/* sndoggvorbis_start(...)
 *
 * function to start playback of a Ogg/Vorbis file. Expects a filename to
 * a file in the KOS filesystem.
 */
int sndoggvorbis_start(const char *filename,int loop)
{
	FILE *f;

	f = fopen(filename, "rb");
	if (!f) {
		dbglog(DBG_ERROR, "sndoggvorbis: couldn't open source file!\n");
		return -1;
	}

	return sndoggvorbis_start_fd(f, loop);
}

/* sndoggvorbis_shutdown()
 *
 * function that stops playing and shuts down the player thread.
 */
void sndoggvorbis_thd_quit()
{
	sndoggvorbis_status = STATUS_QUIT;

	/* In case player is READY -> tell it to continue */
	sem_signal(sndoggvorbis_halt_sem);
        while (sndoggvorbis_status != STATUS_ZOMBIE)
                thd_pass();
        // snd_stream_stop();
}

/* sndoggvorbis_volume(...)
 *
 * function to set the volume of the streaming channel
 */
void sndoggvorbis_volume(int vol)
{
	sndoggvorbis_vol = vol;
	snd_stream_volume(stream_hnd, vol);
}
	
/* sndoggvorbis_mainloop()
 *
 * code that runs in our decoding thread. sets up the semaphore. initializes the stream
 * driver and calls our *real* thread code
 */
void sndoggvorbis_mainloop()
{
	/* create a semaphore for thread to halt on
	 */
	sndoggvorbis_halt_sem = sem_create(0);
	
	sndoggvorbis_status = STATUS_INIT;
	sndoggvorbis_queue_enabled = 0;

	/* initialize comment field
	 */
	sndoggvorbis_clear_comments();
	/* sndoggvorbis_setbitrateinterval(1000);*/
	
	/* call the thread code
	 */
	sndoggvorbis_thread();

	/* destroy the semaphore we first created
	 */
	sem_destroy(sndoggvorbis_halt_sem);
}
