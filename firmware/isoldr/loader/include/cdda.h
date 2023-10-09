/**
 * DreamShell ISO Loader
 * CDDA audio playback
 * (c)2014-2023 SWAT <http://www.dc-swat.ru>
 */

#ifndef _CDDA_H
#define _CDDA_H

#include <arch/types.h>

typedef enum CDDA_status {
	CDDA_STAT_IDLE = 0,
	CDDA_STAT_FILL,
	CDDA_STAT_PREP,
	CDDA_STAT_POS,
	CDDA_STAT_SNDL,
	CDDA_STAT_SNDR,
	CDDA_STAT_WAIT,
	CDDA_STAT_END
} CDDA_status_t;

typedef enum PCM_buff {
	PCM_TMP_BUFF = 0,
	PCM_DMA_BUFF = 1
} PCM_buff_t;


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
	uint32 dma;            /* Use DMA for transfer to AICA memory */

	/* Format info */
	uint16 wav_format;
	uint16 aica_format;
	uint16 chn;
	uint16 bitsize;
	uint32 freq;

	/* End pos for channel */
	uint32 end_pos;
	uint32 end_tm;

	/* Check status value for normalize playback */
	uint32 check_status;
	uint32 check_freq;
	uint32 restore_count;

	/* AICA channels index */
	uint32 right_channel;
	uint32 left_channel;

	/* AICA channels restore mode */
	uint32 adapt_channels;

	/* Mutex for G2 bus */
	uint32 g2_lock;

	/* SH4 timer for checking playback position */
	uint32 timer;

	/* Exception code for AICA DMA IRQ */
	uint32 irq_code;

	/* Volume for both channels */
	uint32 volume;

	/* CDDA status (internal) */
	uint32 stat;

	/* CDDA syscall request data */
	uint32 loop;
	uint32 first_track;
	uint32 last_track;
	uint32 first_lba;
	uint32 last_lba;

} cdda_ctx_t;

/* PCM stereo splitters, optimized for SH4 */
void pcm16_split(int16 *all, int16 *left, int16 *right, uint32 size);
void pcm8_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);
void adpcm_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);

int lock_cdda(void);
void unlock_cdda(void);

cdda_ctx_t *get_CDDA(void);

int CDDA_Init(void);
void CDDA_MainLoop(void);

int CDDA_Play(uint32 first, uint32 last, uint32 loop);
int CDDA_Play2(uint32 first, uint32 last, uint32 loop);
int CDDA_Pause(void);
int CDDA_Release();
int CDDA_Stop(void);
int CDDA_Seek(uint32 offset);

void CDDA_Test();

#endif /* _CDDA_H */
