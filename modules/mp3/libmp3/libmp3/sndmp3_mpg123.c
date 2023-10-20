/*
   sndmp3_mpg123.c
   (c)2011 SWAT

   An MP3 player using sndstream and mpg123
*/

/* This library is designed to be called from another program in a thread. It
   expects an input filename, and it will do all the setup and playback work.
   This requires a working math library for m4-single-only (such as newlib).
 */


#include <kos.h>
#include <mp3/sndserver.h>
#include <mp3/sndmp3.h>


#include "config.h"
#include "compat.h"
#include "mpg123.h"


/* Bitstream buffer: this is for input data */
//#define BS_SIZE (64*1024)

#ifdef BS_SIZE

#define BS_WATER (16*1024)		/* 8 sectors */
static uint8 *bs_buffer = NULL, *bs_ptr;
static int bs_count;
static uint32 mp3_fd;

#endif

/* PCM buffer: for storing data going out to the SPU */
#define PCM_WATER 65536			/* Amt to send to the SPU */
#define PCM_SIZE (PCM_WATER+16384)	/* Total buffer size */
static uint8 *pcm_buffer = NULL, *pcm_ptr;
static int pcm_count, pcm_discard;

static snd_stream_hnd_t stream_hnd = -1;

/* MPEG file */
static mpg123_handle *mh;
struct mpg123_frameinfo decinfo;
static long rate;
static int channels;

/* Name of last played MP3 file */
static char mp3_last_fn[256];

#ifdef BS_SIZE
/* Checks to make sure we have some data available in the bitstream
   buffer; if there's less than a certain "water level", shift the
   data back and bring in some more. */
static int bs_fill() {
	int n = 0;
	
	/* Get the current bitstream offset */
    //off_t mp3pos = mpg123_tell_stream(mh);
   
	/* Make sure we don't underflow */
	if (bs_count < 0) bs_count = 0;

	/* Pull in some more data if we need it */
	if (bs_count < BS_WATER) {
		/* Shift everything back */
		memcpy(bs_buffer, bs_ptr, bs_count);

		/* Read in some more data */
		// printf("fs_read(%d,%x,%d)\r\n", mp3_fd, bs_buffer+bs_count, BS_SIZE - bs_count);
		n = fs_read(mp3_fd, bs_buffer+bs_count, BS_SIZE - bs_count);
		// printf("==%d\r\n", n);
		if (n <= 0)
			return -1;

		/* Shift pointers back */
		bs_count += n; bs_ptr = bs_buffer;
	}

	/* Get the bitstream read size */
    return n;//mpg123_tell_stream(mh) - mp3pos;
}
#endif

/* Empties out the last (now-used) frame of data from the PCM buffer */
static int pcm_empty(int size) {
	if (pcm_count >= size) {
		/* Shift everything back */
		memcpy(pcm_buffer, pcm_buffer + size, pcm_count - size);

		/* Shift pointers back */
		pcm_count -= size;
		pcm_ptr = pcm_buffer + pcm_count;
	}

	return 0;
}

/* This callback is called each time the sndstream driver needs
   some more data. It will tell us how much it needs in bytes. */
static void* mpg123_callback(snd_stream_hnd_t hnd, int size, int * actual) {
	//static	int frames = 0;
	size_t done = 0;
	int err = 0;

#ifdef BS_SIZE
	/* Check for file not started or file finished */
	if (mp3_fd == 0)
		return NULL;
#endif

	/* Dump the last PCM packet */
	pcm_empty(pcm_discard);

	/* Loop decoding until we have a full buffer */
	while (pcm_count < size) {
		
#ifdef BS_SIZE		
		/* Pull in some more data (and check for EOF) */
		
		if (bs_fill() < 0 || bs_count < decinfo.framesize) {
			printf("snd_mp3_server: Decode completed\r\n");
			goto errorout;
		}
		err = mpg123_decode(mh, bs_ptr, decinfo.framesize, pcm_ptr, size, &done); 
#else
		err = mpg123_read(mh, pcm_ptr, size, &done);
#endif

		switch(err) {
			case MPG123_DONE:
				printf("snd_mp3_server: Decode completed\r\n");
				goto errorout;
			case MPG123_NEED_MORE:
				printf("snd_mp3_server: MPG123_NEED_MORE\n");
				break;
			case MPG123_ERR:
				printf("snd_mp3_server: %s\n", (char*)mpg123_strerror(mh));
				goto errorout;
				break;
			default:
				break;
		}
		
#ifdef BS_SIZE
		bs_ptr += decinfo.framesize; bs_count -= decinfo.framesize;
#endif
		pcm_ptr += done; pcm_count += done;
		
		//frames++;
		//if (!(frames % 64)) {
			//printf("Decoded %d frames    \r", frames);
		//}
	}

	pcm_discard = *actual = size;

	/* Got it successfully */
	return pcm_buffer;

errorout:
#ifdef BS_SIZE
	fs_close(mp3_fd); mp3_fd = 0;
#endif
	return NULL;
}

/* Open an MPEG stream and prepare for decode */
static int libmpg123_init(const char *fn) {
	int err;
	uint32	fd;
	
	/* Open the file */
#ifdef BS_SIZE
	mp3_fd = fd = fs_open(fn, O_RDONLY);
#else
	fd = fs_open(fn, O_RDONLY);
#endif
	
	if (fd < 0) {
		printf("Can't open input file %s\r\n", fn);
		printf("getwd() returns '%s'\r\n", fs_getwd());
		return -1;
	}
	
#ifndef BS_SIZE	
	fs_close(fd);
#endif
	
	if (fn != mp3_last_fn) {
		if (fn[0] != '/') {
			strcpy(mp3_last_fn, fs_getwd());
			strcat(mp3_last_fn, "/");
			strcat(mp3_last_fn, fn);
		} else {
			strcpy(mp3_last_fn, fn);
		}
	}

	/* Allocate buffers */
	if (pcm_buffer == NULL)
		pcm_buffer = memalign(32, PCM_SIZE);
	pcm_ptr = pcm_buffer; pcm_count = pcm_discard = 0;

	
#ifdef BS_SIZE

	if (bs_buffer == NULL)
		bs_buffer = memalign(32, BS_SIZE);
	bs_ptr = bs_buffer; bs_count = 0;

	/* Fill bitstream buffer */
	if (bs_fill() < 0) {
		printf("Can't read file header\r\n");
		goto errorout;
	}

	/* Are we looking at a RIFF file? (stupid Windows encoders) */
	if (bs_ptr[0] == 'R' && bs_ptr[1] == 'I' && bs_ptr[2] == 'F' && bs_ptr[3] == 'F') {
		/* Found a RIFF header, scan through it until we find the data section */
		printf("Skipping stupid RIFF header\r\n");
		while (bs_ptr[0] != 'd' || bs_ptr[1] != 'a' || bs_ptr[2] != 't'	|| bs_ptr[3] != 'a') {
			bs_ptr++;
			if (bs_ptr >= (bs_buffer + BS_SIZE)) {
				printf("Indeterminately long RIFF header\r\n");
				goto errorout;
			}
		}

		/* Skip 'data' and length */
		bs_ptr += 8;
		bs_count -= (bs_ptr - bs_buffer);
		printf("Final index is %d\r\n", (bs_ptr - bs_buffer));
	}

	if (((uint8)bs_ptr[0] != 0xff) && (!((uint8)bs_ptr[1] & 0xe0))) {
		printf("Definitely not an MPEG file\r\n");
		goto errorout;
	}
#endif
	
	mpg123_init();
    mh = mpg123_new(NULL, &err);

	if(mh == NULL) {
		printf("Can't init mpg123: %s\n", mpg123_strerror(mh));
		goto errorout;
	}
   
    /* Open the MP3 context in open_fd mode */
#ifdef BS_SIZE
    err = mpg123_open_fd(mh, mp3_fd);
#else
	err = mpg123_open(mh, fn);
#endif
 
	if(err != MPG123_OK) {
		printf("Can't open mpg123\n");
		mpg123_exit();
		goto errorout;
	}

	int enc;

	mpg123_getformat(mh, &rate, &channels, &enc);
	mpg123_info(mh, &decinfo);
	
	printf("Output Sampling rate = %ld\r\n", decinfo.rate);
	printf("Output Bitrate       = %d\r\n", decinfo.bitrate);
	printf("Output Frame size    = %d\r\n", decinfo.framesize);

	printf("mpg123 initialized successfully\r\n");
	return 0;

errorout:
	printf("Exiting on error\r\n");
#ifdef BS_SIZE
	if (bs_buffer) {
		free(bs_buffer);
		bs_buffer = NULL;
	}
#endif
	if (pcm_buffer) {
		free(pcm_buffer);
		pcm_buffer = NULL;
	}
#ifdef BS_SIZE
	fs_close(fd);
	mp3_fd = 0;
#endif
	return -1;
}

static void libmpg123_shutdown() {
	
	if(mh != NULL) {
		mpg123_close(mh);
	}
	
    mpg123_exit();

#ifdef BS_SIZE
	if (bs_buffer) {
		free(bs_buffer);
		bs_buffer = NULL;
	}
#endif
	if (pcm_buffer) {
		free(pcm_buffer);
		pcm_buffer = NULL;
	}
#ifdef BS_SIZE
	if (mp3_fd) {
		fs_close(mp3_fd);
		mp3_fd = 0;
	}
#endif
}


/************************************************************************/

#include <dc/sound/stream.h>

/* Status flag */
#define STATUS_INIT	0
#define STATUS_READY	1
#define STATUS_STARTING	2
#define STATUS_PLAYING	3
#define STATUS_STOPPING	4
#define STATUS_QUIT	5
#define STATUS_ZOMBIE	6
#define STATUS_REINIT	7
static volatile int sndmp3_status;

/* Wait until the MP3 thread is started and ready */
void sndmp3_wait_start() {
	while (sndmp3_status != STATUS_READY)
		;
}

/* Semaphore to halt sndmp3 until a command comes in */
static semaphore_t *sndmp3_halt_sem;

/* Loop flag */
static volatile int sndmp3_loop;

/* Call this function as a thread to handle playback. Playback will 
   stop and this thread will return when you call sndmp3_shutdown(). */
static void sndmp3_thread() {
	int sj;

	stream_hnd = snd_stream_alloc(NULL, SND_STREAM_BUFFER_MAX);
	
	if(stream_hnd < 0) {
		printf("sndserver: can't alloc stream\r\n");
		return;
	}
	
	/* Main command loop */
	while(sndmp3_status != STATUS_QUIT) {
		switch(sndmp3_status) {
			case STATUS_INIT:
				sndmp3_status = STATUS_READY;
				break;
			case STATUS_READY:
				printf("sndserver: waiting on semaphore\r\n");
				sem_wait(sndmp3_halt_sem);
				printf("sndserver: released from semaphore\r\n");
				break;
			case STATUS_STARTING:
				/* Initialize streaming driver */
				if (snd_stream_reinit(stream_hnd, mpg123_callback) < 0) {
					sndmp3_status = STATUS_READY;
				} else {
					snd_stream_start(stream_hnd, rate, channels - 1);
					sndmp3_status = STATUS_PLAYING;
				}
				break;
			case STATUS_REINIT:
				/* Re-initialize streaming driver */
				snd_stream_reinit(stream_hnd, NULL);
				sndmp3_status = STATUS_READY;
				break;
			case STATUS_PLAYING: {
				sj = jiffies;
				if (snd_stream_poll(stream_hnd) < 0) {
					if (sndmp3_loop) {
						printf("sndserver: restarting '%s'\r\n", mp3_last_fn);
						if (libmpg123_init(mp3_last_fn) < 0) {
							sndmp3_status = STATUS_STOPPING;
							mp3_last_fn[0] = 0;
						}
					} else {
						printf("sndserver: not restarting\r\n");
						snd_stream_stop(stream_hnd);
						sndmp3_status = STATUS_READY;
						mp3_last_fn[0] = 0;
					}
					// stream_start();
				} else
					thd_sleep(50);
				break;
			}
			case STATUS_STOPPING:
				snd_stream_stop(stream_hnd);
				sndmp3_status = STATUS_READY;
				break;
		}
	}
	
	/* Done: clean up */
	libmpg123_shutdown();
	snd_stream_stop(stream_hnd);
	snd_stream_destroy(stream_hnd);

	sndmp3_status = STATUS_ZOMBIE;
}

/* Start playback (implies song load) */
int sndmp3_start(const char *fn, int loop) {
	/* Can't start again if already playing */
	if (sndmp3_status == STATUS_PLAYING)
		return -1;

	/* Initialize MP3 engine */
	if (fn) {
		if (libmpg123_init(fn) < 0)
			return -1;
	
		/* Set looping status */
		sndmp3_loop = loop;
	}

	/* Wait for player thread to be ready */
	while (sndmp3_status != STATUS_READY)
		thd_pass();

	/* Tell it to start */
	if (fn)
		sndmp3_status = STATUS_STARTING;
	else
		sndmp3_status = STATUS_REINIT;
	sem_signal(sndmp3_halt_sem);

	return 0;
}

/* Stop playback (implies song unload) */
void sndmp3_stop() {
	if (sndmp3_status == STATUS_READY)
		return;
	sndmp3_status = STATUS_STOPPING;
	while (sndmp3_status != STATUS_READY)
		thd_pass();
	libmpg123_shutdown();
	mp3_last_fn[0] = 0;
}

/* Shutdown the player */
void sndmp3_shutdown() {
	sndmp3_status = STATUS_QUIT;
	sem_signal(sndmp3_halt_sem);
	while (sndmp3_status != STATUS_ZOMBIE)
		thd_pass();
	spu_disable();
}

/* Adjust the MP3 volume */
void sndmp3_volume(int vol) {
	snd_stream_volume(stream_hnd, vol);
}


/* The main loop for the sound server */
void sndmp3_mainloop() {
	/* Allocate a semaphore for temporarily halting sndmp3 */
	sndmp3_halt_sem = sem_create(0);

	/* Go into the main thread wait loop */
	sndmp3_status=STATUS_INIT;
	sndmp3_thread();

	/* Free the semaphore */
	sem_destroy(sndmp3_halt_sem);
}
	
