/**
 * DreamShell ISO Loader
 * CDDA audio playback
 * (c)2014-2025 SWAT <http://www.dc-swat.ru>
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
	CDDA_STAT_WAIT
} CDDA_status_t;

typedef enum PCM_buff {
	PCM_TMP_BUFF = 0,
	PCM_DMA_BUFF = 1
} PCM_buff_t;

typedef enum PCM_transfer {
	PCM_TRANS_DMA = 0,
	PCM_TRANS_SQ,
	PCM_TRANS_PIO,
	PCM_TRANS_SQ_SPLIT,
	PCM_TRANS_PIO_SPLIT
} PCM_transfer_t;

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
	uint32 trans_method;   /* Transfer method to AICA memory */

	/* Format info */
	uint32 aica_freq;
	uint16 aica_format;
	uint16 bitsize;
	uint32 channels;

	/* End pos for channel */
	uint32 end_pos;
	uint32 end_tm;

	/* Check status value for normalize playback */
	uint32 check_status;
	uint32 restore_count;
	uint32 restore;

	/* AICA channels index */
	uint32 right_channel;
	uint32 left_channel;

	/* AICA channels restore mode */
	uint32 adapt_channels;

	/* Mutex for G2 bus */
	uint32 g2_lock;

	/* SH4 timer for checking playback position */
	uint32 timer;

	/* Exception code for internal AICA DMA IRQ */
	uint32 int_irq_code;

	/* Exception code for game AICA DMA IRQ */
	uint32 game_irq_code;

	/* Volume for both channels */
	uint32 volume;

	/* CDDA status (internal) */
	uint32 stat;

	/* CDDA syscall request data */
	uint32 loop;
	uint32 first_lba;
	uint32 last_lba;

} cdda_ctx_t;

/* PCM and ADPCM stereo splitters, optimized for SH4 */
void pcm16_split(uint32 *data, uint32 *left, uint32 *right, uint32 size);
void adpcm_split(uint32 *data, uint32 *left, uint32 *right, uint32 size);

int lock_cdda(void);
void unlock_cdda(void);

cdda_ctx_t *get_CDDA(void);

int CDDA_Init(void);
void CDDA_MainLoop(void);

int CDDA_PlayTracks(uint32 first, uint32 last, uint32 loop);
int CDDA_PlaySectors(uint32 first, uint32 last, uint32 loop);
int CDDA_Pause(void);
int CDDA_Release();
int CDDA_Stop(void);
int CDDA_Seek(uint32 offset);

void CDDA_Test();

#endif /* _CDDA_H */
