/**
 * DreamShell ISO Loader
 * CDDA audio playback
 * (c)2014-2020 SWAT <http://www.dc-swat.ru>
 */

#ifndef _CDDA_H
#define _CDDA_H

#include <arch/types.h>

enum {
	CDDA_STAT_IDLE = 0,
	CDDA_STAT_FILL = 1,
	CDDA_STAT_FILR = 3,
	CDDA_STAT_PREP = 4,
	CDDA_STAT_SNDL = 5,
	CDDA_STAT_SNDR = 6,
	CDDA_STAT_WAIT = 7
} CDDA_status;

enum {
	PCM_TMP_BUFF = 0,
	PCM_DMA_BUFF = 1
} PCM_buff;


typedef struct cdda_ctx {
	
	char *filename;        /* Track file name */
	size_t fn_len;         /* Track file name length */
	file_t fd;             /* Track file FD */
	uint32 offset;         /* Track file offset */
	uint32 cur_offset;     /* Track current offset */
	uint32 track_size;     /* Track size */
	uint32 lba;            /* Track LBA */
	
	uint8 *alloc_buff;     /* Dynamic PCM buffer from malloc */
	uint8 *buff[2];        /* PCM buffer in main RAM */
	uint32 aica_left[2];   /* First/second buffer for channel in sound RAM */
	uint32 aica_right[2];  /* First/second buffer for channel in sound RAM */
	uint32 cur_buff;       /* AICA channel buffer */
	size_t size;           /* Full buffer size */
	
	/* Format info */
	uint16 wav_format;
	uint16 aica_format;
	uint16 chn;
	uint16 bitsize;
	uint32 freq;
	
	/* End pos for channel */
	int end_pos;
	uint32 end_tm;
	
	/* Check status value for normalize playback */
	uint32 check_val;

	/* Mutex for G2 bus */
	uint32 g2_lock;

	/* SH4 timer for checking playback position */
	uint32 timer;

	/* IRQ index for AICA DMA */
	uint32 irq_index;
	
	/* Volume for both channels */
	uint32 volume;
	
	/* CDDA status (internal) */
	uint8 stat;
	uint8 next_stat;
	
	/* CDDA syscall request data */
	uint8 first;
	uint8 last;
	uint8 loop;
	
} cdda_ctx_t;

/* PCM stereo splitters, optimized for SH4 */
void pcm16_split(int16 *all, int16 *left, int16 *right, uint32 size);
void pcm8_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);
void adpcm_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);

cdda_ctx_t *get_CDDA(void);

int CDDA_Init(void);
void CDDA_MainLoop(void);

int CDDA_Play(uint8 first, uint8 last, uint16 loop);
int CDDA_Play2(uint32 first, uint32 last, uint16 loop);
int CDDA_Pause(void);
int CDDA_Release(void);
int CDDA_Stop(void);
int CDDA_Seek(uint32 offset);

void CDDA_Test();

#endif /* _CDDA_H */
