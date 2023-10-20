/* Tryptonite

   sndmp3.c
   (c)2000 Dan Potter

   An MP3 player using sndstream and MPGLIB

   DOES NOT CURRENTLY WORK.
   
   Updated 2011 by SWAT
   This is work =)
*/



/* This library is designed to be called from another program in a thread. It
   expects an input filename, and it will do all the setup and playback work.
  
   This requires a working math library for m4-single-only (such as newlib).
   
 */

#include <kos.h>
#include <mp3/sndserver.h>
#include <mp3/sndmp3.h>

/************************************************************************/
//#include "mpg123.h"		/* From mpglib */
//#include "mpglib.h"
	
#include "mpglib_config.h"
#include "mpg123.h"
#include "interface.h"
#include "decode_i386.h"

/* Bitstream buffer: this is for input data */
#define BS_SIZE (64*1024)
#define BS_WATER (16*1024)		/* 8 sectors */
static uint8 *bs_buffer = NULL, *bs_ptr;
static int bs_count;

/* PCM buffer: for storing data going out to the SPU */
#define PCM_WATER 65536			/* Amt to send to the SPU */
#define PCM_SIZE (PCM_WATER+16384)	/* Total buffer size */
static char *pcm_buffer = NULL, *pcm_ptr;
static int pcm_count, pcm_discard;

/* MPEG file */
static uint32		mp3_fd;
//static struct mpstr	mp;
static struct mpstr_tag mp;
static int		frame_bytes;

/* Name of last played MP3 file */
static char mp3_last_fn[256];

static snd_stream_hnd_t stream_hnd = -1;

/* Checks to make sure we have some data available in the bitstream
   buffer; if there's less than a certain "water level", shift the
   data back and bring in some more. */
#if 0
static int bs_fill() {
	int n;

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

	return 0;
}

#else

static int bs_fill() {
	int n;

	/* Read in some more data */
	n = fs_read(mp3_fd, bs_buffer, BS_SIZE);
	if (n <= 0)
		return -1;

	return 0;
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


static void* mpglib_callback(snd_stream_hnd_t hnd, int size, int * actual) {
	//static	int frames = 0;
	int ret, rsize;

	/* Check for file not started or file finished */
	if (mp3_fd == 0)
		return NULL;

	/* Dump the last PCM packet */
	pcm_empty(pcm_discard);

	/* Loop decoding until we have a full buffer */
	while (pcm_count < size) {
		//printf("decoding previously loaded frame into %08x, %d bytes possible\n", pcm_ptr, PCM_WATER - pcm_count);
		
		ret = decodeMP3(&mp, NULL, 0, pcm_ptr, PCM_WATER - pcm_count, &rsize);
		//ret = decodeMP3_clipchoice(&mp, NULL, 0, pcm_ptr, &rsize, synth_1to1_mono, synth_1to1);
		//printf("ret was %s, size is %d\n", ret == MP3_OK ? "OK" : "ERROR", rsize);
		
		if (ret != MP3_OK) {
			//printf("Refilling the buffer\n");
			// Pull in some more data (and check for EOF) 
			if (bs_fill() < 0) {
				printf("Decode completed\r\n");
				goto errorout;
			}
			//printf("trying decode again...\n");
			ret = decodeMP3(&mp, bs_buffer, BS_SIZE, pcm_ptr, PCM_WATER - pcm_count, &rsize);
			//ret = decodeMP3_clipchoice(&mp, bs_ptr, BS_SIZE, pcm_ptr, &size, synth_1to1_mono, synth_1to1);
			//printf("ret was %s, size is %d\n", ret == MP3_OK ? "OK" : "ERROR", rsize);
			//bs_ptr += (8*1024); bs_count -= (8*1024);
		}

		pcm_ptr += rsize; pcm_count += rsize;
		//frames++;
		
		/*if (!(frames % 64)) {
			printf("Decoded %d frames    \r", frames);
		}*/
	}

	pcm_discard = *actual = size;

	/* Got it successfully */
	return pcm_buffer;

errorout:
	fs_close(mp3_fd); mp3_fd = 0;
	return NULL;
}



/* Open an MPEG stream and prepare for decode */
static int mpglib_init(const char *fn) {
	uint32	fd;
	int size;

	/* Open the file */
	mp3_fd = fd = fs_open(fn, O_RDONLY);
	if (fd < 0) {
		printf("Can't open input file %s\r\n", fn);
		printf("getwd() returns '%s'\r\n", fs_getwd());
		return -1;
	}
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
	if (bs_buffer == NULL)
		bs_buffer = memalign(32, BS_SIZE);
	bs_ptr = bs_buffer; bs_count = 0;
	if (pcm_buffer == NULL)
		pcm_buffer = memalign(32, PCM_SIZE);
	pcm_ptr = pcm_buffer; pcm_count = pcm_discard = 0;

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

	/* Initialize MPEG engines */
	InitMP3(&mp);

	/* Decode the first frame */
	printf("decoding first frame:\n");
	decodeMP3(&mp, bs_buffer, BS_SIZE, pcm_ptr, PCM_WATER - pcm_count, &size);
	printf("decoded size was %d\n", size);
	pcm_ptr += size; pcm_count += size;

	printf("mpglib initialized successfully\r\n");
	return 0;

errorout:
	printf("Exiting on error\r\n");
	if (bs_buffer) {
		free(bs_buffer);
		bs_buffer = NULL;
	}
	if (pcm_buffer) {
		free(pcm_buffer);
		pcm_buffer = NULL;
	}
	fs_close(fd);
	mp3_fd = 0;
	return -1;
}

static void mpglib_shutdown() {
	if (bs_buffer) {
		free(bs_buffer);
		bs_buffer = NULL;
	}
	if (pcm_buffer) {
		free(pcm_buffer);
		pcm_buffer = NULL;
	}
	if (mp3_fd) {
		fs_close(mp3_fd);
		mp3_fd = 0;
	}
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
	//assert( stream_hnd != -1 );
	
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
				if (snd_stream_reinit(stream_hnd, mpglib_callback) < 0) {
					sndmp3_status = STATUS_READY;
				} else {
					//snd_stream_start(stream_hnd, decinfo.samprate, decinfo.channels - 1);
					//snd_stream_start(stream_hnd, 44100, 1);
					snd_stream_start(stream_hnd, freqs[mp.fr.sampling_frequency], mp.fr.stereo);
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
						if (mpglib_init(mp3_last_fn) < 0) {
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
	mpglib_shutdown();
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
		if (mpglib_init(fn) < 0)
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
	mpglib_shutdown();
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

	/* Setup an ABI for other programs */
	/* sndmp3_svc_init(); */

	/* Initialize sound program for apps that don't need music */
	// snd_stream_init(NULL);

	/* Go into the main thread wait loop */
	sndmp3_status=STATUS_INIT;
	sndmp3_thread();

	/* Free the semaphore */
	sem_destroy(sndmp3_halt_sem);

	/* Thread exited, so we were requested to quit */
	/* svcmpx->remove_handler("sndsrv"); */
}

