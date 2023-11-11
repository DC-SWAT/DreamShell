/**
 * DreamShell ISO Loader
 * CDDA audio playback emulation
 * (c)2014-2023 SWAT <http://www.dc-swat.ru>
 */

//#define DEBUG 1

#include <main.h>
#include <mmu.h>
#include <asic.h>
#include <exception.h>
#include <drivers/aica.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <arch/timer.h>
#include <dc/sq.h>
#include <dc/fifo.h>

/*
static const uint8 logs[] = {
	0, 15, 22, 27, 31, 35, 39, 42, 45, 47, 50, 52, 55, 57, 59, 61,
	63, 65, 67, 69, 71, 73, 74, 76, 78, 79, 81, 82, 84, 85, 87, 88,
	90, 91, 92, 94, 95, 96, 98, 99, 100, 102, 103, 104, 105, 106,
	108, 109, 110, 111, 112, 113, 114, 116, 117, 118, 119, 120, 121,
	122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
	135, 136, 137, 138, 138, 139, 140, 141, 142, 143, 144, 145, 146,
	146, 147, 148, 149, 150, 151, 152, 152, 153, 154, 155, 156, 156,
	157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167,
	167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176,
	177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185,
	186, 187, 187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194,
	195, 195, 196, 197, 197, 198, 199, 199, 200, 200, 201, 202, 202,
	203, 204, 204, 205, 205, 206, 207, 207, 208, 209, 209, 210, 210,
	211, 212, 212, 213, 213, 214, 215, 215, 216, 216, 217, 217, 218,
	219, 219, 220, 220, 221, 221, 222, 223, 223, 224, 224, 225, 225,
	226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 232, 232, 233,
	233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 239, 239, 240,
	240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246,
	247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 254, 255
};
*/

#define AICA_PAN(x) ((x) == 0x80 ? (0) : ((x) < 0x80 ? (0x1f) : (0x0f)))
//#define AICA_VOL(x) (0xff - logs[128 + (((x) & 0xff) / 2)])

/* AICA sample formats */
#define AICA_SM_16BIT    0 /* Linear PCM 16-bit */
#define AICA_SM_8BIT     1 /* Linear PCM 8-bit */
#define AICA_SM_ADPCM    2 /* Yamaha ADPCM 4-bit */
#define AICA_SM_ADPCM_LS 3 /* Long stream ADPCM 4-bit */

/* WAV sample formats */
#define WAVE_FMT_PCM                   0x0001 /* PCM */
#define WAVE_FMT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */
#define WAVE_FMT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */

#define AICA_CHANNELS_COUNT 64

/* AICA channels for CDDA audio playback by default */
#define AICA_CDDA_CH_LEFT  (AICA_CHANNELS_COUNT - 2)
#define AICA_CDDA_CH_RIGHT (AICA_CHANNELS_COUNT - 1)

/* AICA memory end for PCM buffer */
#define AICA_MEMORY_END 0x00A00000
#define AICA_MEMORY_END_ARM 0x00200000

#define aica_dma_in_progress() AICA_DMA_ADST
#define aica_dma_disable() AICA_DMA_ADEN = 0

#define AICA_DMA_SUSPEND AICA_DMA_ADSUSP
#define BBA_DMA_SUSPEND  *((vuint32 *)0xa05f783c)
#define EXT2_DMA_SUSPEND *((vuint32 *)0xa05f785c)
#define DEV_DMA_SUSPEND  *((vuint32 *)0xa05f787c)

#define RAW_SECTOR_SIZE 2352

static cdda_ctx_t _cdda;
static cdda_ctx_t *cdda = &_cdda;

cdda_ctx_t *get_CDDA(void) {
	return &_cdda;
}

/* When accessing to the SPU RAM or AICA,
	this is required at least every 8 32-bit */
static inline void g2_fifo_wait() {
	do { } while(FIFO_STATUS & (FIFO_G2 | FIFO_AICA));
}

/* G2 Bus locking */
static void g2_lock(void) {
	cdda->g2_lock = irq_disable();
	AICA_DMA_SUSPEND = 1;
	BBA_DMA_SUSPEND  = 1;
	EXT2_DMA_SUSPEND = 1;
	DEV_DMA_SUSPEND  = 1;
	do { } while(FIFO_STATUS & (FIFO_SH4 | FIFO_G2 | FIFO_AICA));
}

static void g2_unlock(void) {
	AICA_DMA_SUSPEND = 0;
	BBA_DMA_SUSPEND  = 0;
	EXT2_DMA_SUSPEND = 0;
	DEV_DMA_SUSPEND  = 0;
	irq_restore(cdda->g2_lock);
}

static void aica_dma_irq_hide() {
	if (*ASIC_IRQ11_MASK & ASIC_NRM_AICA_DMA) {
		if (exception_inited()) {
			cdda->irq_code = EXP_CODE_INT9;
			*ASIC_IRQ9_MASK |= ASIC_NRM_AICA_DMA;
		} else {
			cdda->irq_code = EXP_CODE_INT11;
		}
	} else {
		if (exception_inited()) {
			cdda->irq_code = EXP_CODE_INT11;
		} else {
			cdda->irq_code = EXP_CODE_INT9;
		}
	}
}

static void aica_dma_irq_restore() {
	if (cdda->irq_code == 0) {
		return;
	} else if(exception_inited() && cdda->irq_code == EXP_CODE_INT9) {
		ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_AICA_DMA;
		*ASIC_IRQ9_MASK &= ~ASIC_NRM_AICA_DMA;
	}
	cdda->irq_code = 0;
}

static void aica_dma_transfer(uint8 *data, uint32 dest, uint32 size) {

	DBGFF("0x%08lx %ld\n", data, size);
	uint32 addr = (uint32)data;

	AICA_DMA_G2APRO = 0x4659007f;      // Protection code
	AICA_DMA_ADEN   = 0;               // Disable wave DMA
	AICA_DMA_ADDIR  = 0;               // To wave memory
	AICA_DMA_ADTRG  = 0x00000004;      // Initiate by CPU, suspend enabled
	AICA_DMA_ADSTAR = PHYS_ADDR(addr); // System memory address
	AICA_DMA_ADSTAG = dest;            // Wave memory address
	AICA_DMA_ADLEN  = size|0x80000000; // Data size, disable after DMA end
	AICA_DMA_ADEN   = 1;               // Enable wave DMA
	AICA_DMA_ADST   = 1;               // Start wave DMA
}

#ifdef DEV_TYPE_SD
static void aica_sq_transfer(uint8 *data, uint32 dest, uint32 size) {

	uint32 *d = (uint32 *)(void *)(0xe0000000 | (dest & 0x03ffffe0));
	uint32 *s = (uint32 *)data;
	size >>= 5;

	g2_lock();

	/* Set store queue memory area as desired */
	QACR0 = (dest >> 24) & 0x1c;
	QACR1 = (dest >> 24) & 0x1c;

	/* fill/write queues as many times necessary */
	while(size--) {
		/* Prefetch 32 bytes for next loop */
		dcache_pref_block(s + 8);
		d[0] = s[0];
		d[1] = s[1];
		d[2] = s[2];
		d[3] = s[3];
		d[4] = s[4];
		d[5] = s[5];
		d[6] = s[6];
		d[7] = s[7];
		/* Write back memory */
		dcache_pref_block(d);
		d += 8;
		s += 8;
	}

	/* Wait for both store queues to complete */
	d = (uint32 *)0xe0000000;
	d[0] = d[8] = 0;

	g2_unlock();
}
#endif

static void aica_transfer(uint8 *data, uint32 dest, uint32 size) {
	if (cdda->trans_method == PCM_TRANS_DMA) {
		dcache_purge_range((uint32)data, size);
		int old = irq_disable();
		aica_dma_irq_hide();
		aica_dma_transfer(data, dest, size);
		irq_restore(old);
	} 
#ifdef DEV_TYPE_SD
	else {
		aica_sq_transfer(data, dest, size);
	}
#endif
}

static void setup_pcm_buffer() {

	/* 
	 * SH4 timer counter value for polling playback position 
	 * 
	 * Some values for PCM 16 bit 44100 Hz:
	 * 
	 * 32 KB = 72374 / chn
	 * 24 KB = 54278 / chn
	 * 16 KB = 36176 / chn
	 */
	cdda->end_tm = 72374 / cdda->chn;
	size_t old_size = cdda->size;
//	cdda->size = (cdda->bitsize << 10) * cdda->chn;
	
	switch(cdda->bitsize) {
		case 4:
			cdda->size = 0x4000;
			cdda->end_tm *= 2;   /* 4-bit decoded to 16-bit */
			cdda->end_tm += 10;  /* Fixup timer value for ADPCM */
			break;
		case 8:
			cdda->size = 0x4000;
			break;
		case 16:
		default:
			cdda->size = 0x8000;
			if(exception_inited()) {
				cdda->size = 0x4000;
				cdda->end_tm = 36176 / cdda->chn;
#ifdef LOG
				if (malloc_heap_pos() < APP_ADDR) {
					// Need some memory for logging in some cases
					cdda->size >>= 1;
					cdda->end_tm >>= 1;
				}
#endif
			}
			break;
	}

	if (cdda->alloc_buff && old_size != cdda->size) {
		free(cdda->alloc_buff);
		cdda->alloc_buff = NULL;
	}

	if (cdda->alloc_buff == NULL) {
		if (cdda->trans_method == PCM_TRANS_SQ_SPLIT) {
			/* DMA buffer not is used in this case */
			cdda->alloc_buff = malloc((cdda->size >> 1) + 32);
			cdda->buff[PCM_DMA_BUFF] = NULL;
		} else {
			cdda->alloc_buff = malloc(cdda->size + 32);
		}
	}

	if (cdda->alloc_buff == NULL) {
		LOGFF("Failed malloc");
		return;
	}

	cdda->buff[PCM_TMP_BUFF] = (uint8 *)ALIGN32_ADDR((uint32)cdda->alloc_buff);
	if (cdda->trans_method != PCM_TRANS_SQ_SPLIT) {
		cdda->buff[PCM_DMA_BUFF] = cdda->buff[0] + (cdda->size >> 1);
	}

	/* Setup buffer at end of sound memory */
	cdda->aica_left[0] = AICA_MEMORY_END - cdda->size;
	cdda->aica_left[1] = cdda->aica_left[0] + (cdda->size >> 2);

	cdda->aica_right[0] = AICA_MEMORY_END - (cdda->size >> 1);
	cdda->aica_right[1] = cdda->aica_right[0] + (cdda->size >> 2);

	/* Setup end position for sound buffer */
	if(cdda->aica_format == AICA_SM_ADPCM_LS) {
		cdda->end_pos = cdda->size - 1; /* (((cdda->size / cdda->chn) * 2) - 1; */
	} else {
		cdda->end_pos = ((cdda->size / cdda->chn) / (cdda->bitsize >> 3)) - 1;
	}

	LOGFF("0x%08lx 0x%08lx %d\n",
		(uint32)cdda->buff[0], (uint32)cdda->aica_left[0], cdda->size);
}

static void aica_set_volume(int volume) {

	uint32 val;
	cdda->volume = volume;

	g2_lock();

	val = 0x24 | (volume << 8); //(AICA_VOL(vol) << 8);
	CHNREG32(cdda->left_channel,  40) = val;

//	val = 0x24 | (right_chn << 8); //(AICA_VOL(right_chn) << 8);
	CHNREG32(cdda->right_channel, 40) = val;
	CHNREG32(cdda->left_channel, 36) = AICA_PAN(0)   | (0xf << 8);
	CHNREG32(cdda->right_channel, 36) = AICA_PAN(255) | (0xf << 8);

	g2_unlock();
}

static void aica_stop_cdda(void) {

	uint32 val;
	DBGFF(NULL);

	g2_lock();

	val = CHNREG32(cdda->left_channel,  0);
	CHNREG32(cdda->left_channel, 0) = (val & ~0x4000) | 0x8000;

	val = CHNREG32(cdda->right_channel, 0);
	CHNREG32(cdda->right_channel, 0) = (val & ~0x4000) | 0x8000;

	g2_unlock();

	timer_stop(cdda->timer);
	timer_clear(cdda->timer);
}

static void aica_stop_clean_cdda() {

	DBGFF(NULL);
	aica_stop_cdda();

#ifdef _FS_ASYNC
	while(cdda->stat == CDDA_STAT_WAIT) {
		if (poll(cdda->fd) < 0) {
			break;
		}
	}
#endif

	if (cdda->trans_method == PCM_TRANS_DMA && aica_dma_in_progress()) {
		aica_dma_disable();
	}
	cdda->stat = CDDA_STAT_IDLE;
}

static void aica_setup_cdda(int clean) {

	int freq_hi = 7;
	uint32 val, freq_lo, freq_base = 5644800;
	const uint32 smp_ptr = AICA_MEMORY_END_ARM - cdda->size;
	const int smp_size = cdda->size / cdda->chn;

	/* Need to convert frequency to floating point format
	   (freq_hi is exponent, freq_lo is mantissa)
	   Formula is freq = 44100*2^freq_hi*(1+freq_lo/1024) */
	while (cdda->freq < freq_base && freq_hi > -8) {
		freq_base >>= 1;
		--freq_hi;
	}

	freq_lo = (cdda->freq << 10) / freq_base;
	freq_base = (freq_hi << 11) | (freq_lo & 1023);

	/* Stop AICA channels */
	aica_stop_cdda();

	if(clean) {
		LOGFF("0x%08lx 0x%08lx %d %d %d %d\n",
				smp_ptr, (smp_ptr + smp_size), smp_size, 
				cdda->freq, cdda->aica_format, cdda->end_tm);
		cdda->restore_count = 0;
		cdda->cur_buff = 0;
	}

	timer_prime_cdda(cdda->timer, cdda->end_tm, 0);

	/* Setup AICA channels */
	g2_lock();

	CHNREG32(cdda->left_channel, 8) = 0;
	CHNREG32(cdda->left_channel, 12) = cdda->end_pos & 0xffff;
	CHNREG32(cdda->left_channel, 24) = freq_base;

	CHNREG32(cdda->right_channel, 8) = 0;
	CHNREG32(cdda->right_channel, 12) = cdda->end_pos & 0xffff;
	CHNREG32(cdda->right_channel, 24) = freq_base;

	cdda->check_freq = freq_base;

	g2_unlock();
	aica_set_volume(clean ? 255 : 0);

	g2_lock();

	CHNREG32(cdda->left_channel, 16) = 0x1f;
	CHNREG32(cdda->left_channel, 4)  = smp_ptr & 0xffff;

	CHNREG32(cdda->right_channel, 16) = 0x1f;
	CHNREG32(cdda->right_channel, 4)  = (smp_ptr + smp_size) & 0xffff;

	/* Channels check value (for both the same) */
	cdda->check_status = 0x4000 | (cdda->aica_format << 7) | 0x200 | (smp_ptr >> 16);

	/* start play | format | use loop */
	val = 0xc000 | (cdda->aica_format << 7) | 0x200;

	CHNREG32(cdda->left_channel, 0) = val | (smp_ptr >> 16);
	CHNREG32(cdda->right_channel, 0) = val | ((smp_ptr + smp_size) >> 16);

	g2_fifo_wait();

	/* Start SH4 timer */
	timer_start(cdda->timer);

	g2_unlock();
}

static uint32 aica_change_cdda_channel(uint32 channel, uint32 exclude) {
	uint32 val = 0, try_count = 1;
	g2_lock();
	do {
		if (++channel >= AICA_CHANNELS_COUNT) {
			channel = 0;
		}
		if (channel == cdda->left_channel
			|| channel == cdda->right_channel
			|| channel == exclude
		) {
			continue;
		}
		if (++try_count >= AICA_CHANNELS_COUNT) {
			break;
		}
		val = CHNREG32(channel, 0);
	} while ((val & 0x4000) != 0);
	g2_unlock();
#ifdef LOG
	if (try_count >= AICA_CHANNELS_COUNT) {
		LOGF("CDDA: No free channel\n");
	}
#endif
	return channel;
}

static void aica_check_cdda(void) {

	const uint32 check_vol = 0x24 | (cdda->volume << 8);
	uint32 invalid_level = 0;
	uint32 val;

	g2_lock();

	val = CHNREG32(cdda->left_channel, 36) & 0xffff;
	if (val != (AICA_PAN(0) | (0xf << 8))) {
		invalid_level |= 0x11;
		// LOGF("CDDA: L PAN %04lx != %04lx\n", val, (AICA_PAN(0) | (0xf << 8)));
	}
	val = CHNREG32(cdda->right_channel, 36) & 0xffff;
	if (val != (AICA_PAN(255) | (0xf << 8))) {
		invalid_level |= 0x12;
		// LOGF("CDDA: R PAN %04lx != %04lx\n", val, (AICA_PAN(255) | (0xf << 8)));
	}
	val = CHNREG32(cdda->left_channel, 40) & 0xffff;
	if (val != check_vol) {
		invalid_level |= 0x21;
		// LOGF("CDDA: L VOL %04lx != %04lx\n", val, check_vol);
	}
	val = CHNREG32(cdda->right_channel, 40) & 0xffff;
	if (val != check_vol) {
		invalid_level |= 0x22;
		// LOGF("CDDA: R VOL %04lx != %04lx\n", val, check_vol);
	}
	val = CHNREG32(cdda->left_channel, 24) & 0xffff;
	if (val != cdda->check_freq) {
		invalid_level |= 0x41;
		// LOGF("CDDA: L FRQ %04lx != %04lx\n", val, cdda->check_freq);
	}
	val = CHNREG32(cdda->right_channel, 24) & 0xffff;
	if (val != cdda->check_freq) {
		invalid_level |= 0x42;
		// LOGF("CDDA: R FRQ %04lx != %04lx\n", val, cdda->check_freq);
	}
	val = CHNREG32(cdda->left_channel, 12) & 0xffff;
	if (val != (cdda->end_pos & 0xffff)) {
		invalid_level |= 0x41;
		// LOGF("CDDA: L POS %04lx != %04lx\n", val, cdda->end_pos & 0xffff);
	}
	val = CHNREG32(cdda->right_channel, 12) & 0xffff;
	if (val != (cdda->end_pos & 0xffff)) {
		invalid_level |= 0x42;
		// LOGF("CDDA: R POS %04lx != %04lx\n", val, cdda->end_pos & 0xffff);
	}

	g2_fifo_wait();

	val = CHNREG32(cdda->left_channel, 0) & 0xffff;
	if (val != cdda->check_status) {
		invalid_level |= 0x81;
		// LOGF("CDDA: L CON %04lx != %04lx\n", val, cdda->check_status);
	}
	val = CHNREG32(cdda->right_channel, 0) & 0xffff;
	if (val != cdda->check_status) {
		invalid_level |= 0x82;
		// LOGF("CDDA: R CON %04lx != %04lx\n", val, cdda->check_status);
	}

	g2_unlock();

	if ((invalid_level >> 4) == 0) {
		return;
	}

	LOGF("CDDA: Invalid 0x%02lx\n", invalid_level);

	if (cdda->adapt_channels == 0) {
		if ((invalid_level >> 4) <= 3) {
			aica_set_volume(0);
		} else {
			aica_setup_cdda(0);
		}
		return;
	}

	if (cdda->restore_count++ < 10) {
		if ((invalid_level >> 4) <= 3) {
			aica_set_volume(0);
			return;
		}
		if ((invalid_level >> 4) < 8) {
			aica_setup_cdda(0);
			return;
		}
	}
	aica_stop_cdda();

	if (cdda->restore_count >= 10 || (invalid_level >> 4) == 8) {

		uint32 left_channel = cdda->left_channel;

		if (invalid_level & 0x01) {
			left_channel = aica_change_cdda_channel(cdda->left_channel, left_channel);
		}
		if (invalid_level & 0x02) {
			cdda->right_channel = aica_change_cdda_channel(cdda->right_channel, left_channel);
			LOGF("CDDA: Right channel: %d\n", cdda->right_channel);
		}
		if (left_channel != cdda->left_channel) {
			cdda->left_channel = left_channel;
			LOGF("CDDA: Left channel: %d\n", cdda->left_channel);
		}
		cdda->restore_count = 0;
	}
	aica_setup_cdda(0);
}

#if 0 // For debugging

/* Get channel position */
static uint32 aica_get_pos(void) {
	uint32 p;

	/* Observe channel ch */
	g2_lock();
	SNDREG32(0x280c) = (SNDREG32(0x280c) & 0xffff00ff) | (cdda->left_channel << 8);
	g2_unlock();

	g2_fifo_wait();

	/* Update position counters */
	g2_lock();
	p = SNDREG32(0x2814) & 0xffff;
	g2_unlock();

	return p;
}

static void aica_init(void) {

	g2_lock();

	SNDREG32(0x2800) = 0x0000;

	g2_fifo_wait();

	for(int i = 0; i < 64; ++i) {
		if(!(i % 4)) g2_fifo_wait();

		CHNREG32(i, 0) = 0x8000;
		CHNREG32(i, 20) = 0x1f;
	}

	g2_fifo_wait();
	SNDREG32(0x2800) = 0x000f;
	*((vuint32*)SPU_RAM_BASE) = 0xeafffff8;

	g2_fifo_wait();
	SNDREG32(0x2c00) = SNDREG32(0x2c00) & ~1;

	g2_unlock();
}

static void aica_dma_init(void) {

	uint32 main_addr = ((uint32)cdda->buff[0]) & 0x0FFFFFFF;
	uint32 sound_addr = AICA_MEMORY_END - cdda->size;

	AICA_DMA_G2APRO = 0x4659007F;    // Protection code
	AICA_DMA_ADEN   = 0;             // Disable wave DMA
	AICA_DMA_ADDIR  = 0;             // To wave memory
	AICA_DMA_ADTRG  = 0;             // Initiate by CPU
	AICA_DMA_ADSTAR = main_addr;     // System memory address
	AICA_DMA_ADSTAG = sound_addr;    // Wave memory address
	AICA_DMA_ADLEN  = cdda->size;    // Data size
	AICA_DMA_ADEN   = 1;             // Enable wave DMA
}

#endif

static int aica_suitable_pos() {
	uint32 tm = timer_count(cdda->timer);
	uint32 ta = cdda->end_tm >> 1;
	// uint32 pos = aica_get_pos();
	// LOGF("CDDA: POS %ld %ld\n", pos, tm);
	// if((cdda->cur_buff == 0 && pos >= (cdda->end_pos >> 1)) || 
	// 	(cdda->cur_buff == 1 && pos < (cdda->end_pos >> 1))) {
	if((cdda->cur_buff == 0 && tm < ta) || (cdda->cur_buff == 1 && tm > ta)) {
		return 1;
	}
	return 0;
}

static void aica_pcm_split(uint8 *src, uint8 *dst, uint32 size) {

	DBGFF("0x%08lx 0x%08lx %ld\n", src, dst, size);
	uint32 count = size >> 1;

	switch(cdda->bitsize) {
#ifdef DEV_TYPE_SD
		case 4:
			adpcm_split(src, dst, dst + count, size);
			break;
#endif
		case 16:
			pcm16_split((int16 *)src, (int16 *)dst, (int16 *)(dst + count), size);
			break;
		default:
			break;
	}
}

static void aica_pcm_split_sq(uint32 data, uint32 aica_left, uint32 aica_right, uint32 size) {

	uint32 masked_left = (0xe0000000 | (aica_left & 0x03ffffe0));
	uint32 masked_right = (0xe0000000 | (aica_right & 0x03ffffe0));

	g2_lock();

	/* Set store queue memory area as desired */
	QACR0 = (aica_left >> 24) & 0x1c;
	QACR1 = (aica_right >> 24) & 0x1c;

	/* Separating channels and do fill/write queues as many times necessary. */
	pcm16_split_sq(data, masked_left, masked_right, size);

	/* Wait for both store queues to complete */
	uint32 *d = (uint32 *)0xe0000000;
	d[0] = d[8] = 0;

	g2_unlock();
}

static void switch_cdda_track(uint8 track) {

	gd_state_t *GDS = get_GDS();

	if(GDS->cdda_track != track) {

		cdda->filename[cdda->fn_len - 6] = (track / 10) + '0';
		cdda->filename[cdda->fn_len - 5] = (track % 10) + '0';
		cdda->filename[cdda->fn_len - 3] = 'r';
		cdda->filename[cdda->fn_len - 1] = 'w';
		cdda->filename[cdda->fn_len - 0] = '\0';

		LOGF("Opening track%02d: %s\n", track, cdda->filename);

		if(cdda->fd > FILEHND_INVALID) {
			close(cdda->fd);
		}

		cdda->fd = open(cdda->filename, O_RDONLY);

		if(cdda->fd < 0) {

			cdda->filename[cdda->fn_len - 3] = 'w';
			cdda->filename[cdda->fn_len - 1] = 'v';

			LOGF("Not found, opening: %s\n", cdda->filename);
			cdda->fd = open(cdda->filename, O_RDONLY);

			if(cdda->fd < 0) {

				cdda->filename[cdda->fn_len - 3] = 'r';
				cdda->filename[cdda->fn_len - 1] = 'w';
				cdda->filename[cdda->fn_len - 0] = '.';
				cdda->filename[cdda->fn_len + 1] = 'w';
				cdda->filename[cdda->fn_len + 2] = 'a';
				cdda->filename[cdda->fn_len + 3] = 'v';

				LOGF("Not found, opening: %s\n", cdda->filename);

				cdda->fd = open(cdda->filename, O_RDONLY);
#ifdef LOG
				if(cdda->fd < 0) {
					LOGF("CDDA track not found\n");
				}
#endif
			}
		}
	}
}


static uint32 sector_align(uint32 offset) {
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_SD)
	if (offset % 512) {
		offset = ((offset / 512) + 1) * 512;
	}
#elif defined(DEV_TYPE_GD)
	if (offset % 2048) {
		offset = ((offset / 2048) + 1) * 2048;
	}
#endif
	return offset;
}


#ifdef HAVE_EXPT
/*
static void* tmu_handle_exception(register_stack *stack, void *current_vector) {
	uint32 code = *REG_EXPEVT;
	(void)stack;
	if ((code == EXC_CODE_TMU2 && cdda->timer == TMU2) ||
		(code == EXC_CODE_TMU1 && cdda->timer == TMU1)
	) {
		timer_clear(cdda->timer);
	}
	return current_vector;
}
*/
void *aica_dma_handler(void *passer, register_stack *stack, void *current_vector) {

	(void)passer;
	(void)stack;

	if (cdda->irq_code == *REG_INTEVT) {
		aica_dma_irq_restore();
		CDDA_MainLoop();

		uint32 st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] & ~ASIC_NRM_AICA_DMA;

		if (cdda->irq_code == EXP_CODE_INT9) {
			st = ((*ASIC_IRQ9_MASK) & st);
		} else {
			st = ((*ASIC_IRQ11_MASK) & st);
		}
		if (st == 0) {
			return my_exception_finish;
		}
	}
	return current_vector;
}

void *aica_vsync_handler(void *passer, register_stack *stack, void *current_vector) {

	(void)passer;
	(void)stack;

	CDDA_MainLoop();
	return current_vector;
}

# ifndef NO_ASIC_LT
static asic_handler_f old_vsync_handler;
static void *vsync_handler(void *passer, register_stack *stack, void *current_vector) {

	if(old_vsync_handler) {
		current_vector = old_vsync_handler(passer, stack, current_vector);
	}

	return aica_vsync_handler(passer, stack, current_vector);
}

static asic_handler_f old_dma_handler;
static void *dma_handler(void *passer, register_stack *stack, void *current_vector) {

	if(old_dma_handler) {
		current_vector = old_dma_handler(passer, stack, current_vector);
	}

	return aica_dma_handler(passer, stack, current_vector);
}
# endif
#endif

int CDDA_Init() {

	unlock_cdda();
	memset(cdda, 0, sizeof(cdda_ctx_t));

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI) {
		cdda->fd = iso_fd;
	} else {
		cdda->filename = relative_filename("track04.raw");
		cdda->fn_len = strlen(cdda->filename);
		cdda->fd = FILEHND_INVALID;
	}

	cdda->left_channel = AICA_CDDA_CH_LEFT;
	cdda->right_channel = AICA_CDDA_CH_RIGHT;

	/* AICA DMA */
	if(IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1
		|| (IsoInfo->emu_cdda & CDDA_MODE_DST_DMA)) {
		cdda->trans_method = PCM_TRANS_DMA;
	} else {
		cdda->trans_method = PCM_TRANS_SQ;
	}

	/* SH4 timer */
	if(IsoInfo->emu_cdda == CDDA_MODE_DMA_TMU1
		|| IsoInfo->emu_cdda == CDDA_MODE_SQ_TMU1
		|| (IsoInfo->emu_cdda & CDDA_MODE_POS_TMU1)) {
		cdda->timer = TMU1;
	} else {
		cdda->timer = TMU2;
	}

	if(IsoInfo->emu_cdda & CDDA_MODE_CH_FIXED) {
		cdda->adapt_channels = 0;
	} else {
		cdda->adapt_channels = 1;
	}

#if 0
	/* It's not need for games (they do it), only for local test */
	aica_init();
	aica_dma_init();
#endif

#if defined(HAVE_EXPT)
# if !defined(NO_ASIC_LT)
	asic_lookup_table_entry vsync_entry, dma_entry;

	memset(&vsync_entry, 0, sizeof(vsync_entry));
	memset(&dma_entry, 0, sizeof(dma_entry));

	dma_entry.irq = EXP_CODE_ALL;
	dma_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_AICA_DMA;
	dma_entry.handler = dma_handler;

	asic_add_handler(&dma_entry, NULL, 0);

	vsync_entry.irq = EXP_CODE_ALL;
	vsync_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_VSYNC;
	vsync_entry.handler = vsync_handler;

	asic_add_handler(&vsync_entry, &old_vsync_handler, 0);
# endif
/*
	exception_table_entry tmu_entry;

	tmu_entry.type = EXP_TYPE_GEN;
	tmu_entry.code = (cdda->timer == TMU2 ? EXC_CODE_TMU2 : EXC_CODE_TMU1);
	tmu_entry.handler = tmu_handle_exception;
	exception_add_handler(&tmu_entry, NULL);
*/
#endif
	return 0;
}


static void play_track(uint32 track) {

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI) {

		if(IsoInfo->cdda_offset[track - 3] > 0) {
			cdda->offset = IsoInfo->cdda_offset[track - 3];
		} else {
			return;
		}
	} else {

		switch_cdda_track(track);

		if(cdda->fd < 0) {
			return;
		}
		cdda->offset = 0;
	}

	uint32 len = 0;

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	fs_enable_dma(FS_DMA_DISABLED);
#endif

	/* Check file magic */
	lseek(cdda->fd, cdda->offset + 8, SEEK_SET);
	read(cdda->fd, &len, 4);

	if(!memcmp(&len, "WAVE", 4)) {

		/* Read WAV header info */
		lseek(cdda->fd, cdda->offset + 0x14, SEEK_SET);
		read(cdda->fd, &cdda->wav_format, 2);
		read(cdda->fd, &cdda->chn, 2);
		read(cdda->fd, &cdda->freq, 4);
		lseek(cdda->fd, cdda->offset + 0x22, SEEK_SET);
		read(cdda->fd, &cdda->bitsize, 2);
		read(cdda->fd, &len, 4);
		
		if(len != 0x61746164) {
			
			cdda->offset += 0x32;
			lseek(cdda->fd, cdda->offset, SEEK_SET);
			read(cdda->fd, &len, 4);
			
			if(len != 0x61746164) {
				cdda->offset += 0x14;
				lseek(cdda->fd, cdda->offset, SEEK_SET);
				read(cdda->fd, &len, 4);
			}
			
		} else {
			cdda->offset += 0x2C;
		}

		read(cdda->fd, &len, 4);

		switch(cdda->wav_format) {
			case WAVE_FMT_YAMAHA_ADPCM:
			case WAVE_FMT_YAMAHA_ADPCM_ITU_G723:
				cdda->aica_format = AICA_SM_ADPCM_LS;
				break;
			default:
				cdda->aica_format = AICA_SM_16BIT;
				break;
		}
	} else {
		cdda->freq = 44100;
		cdda->chn = 2;
		cdda->bitsize = 16;
		cdda->aica_format = AICA_SM_16BIT;
		cdda->wav_format = WAVE_FMT_PCM;
	}

	/* Make alignment by sector */
	if(cdda->offset > 0) {
		cdda->offset = sector_align(cdda->offset);
	}

	lseek(cdda->fd, cdda->offset, SEEK_SET);

	cdda->cur_offset = 0;
	cdda->lba = TOC_LBA(IsoInfo->toc.entry[track - 1]);

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI) {

		if(IsoInfo->toc.entry[track] != (uint32)-1) {
			cdda->track_size = (TOC_LBA(IsoInfo->toc.entry[track]) - cdda->lba - 2) * RAW_SECTOR_SIZE;
		} else {
			cdda->track_size = (((total(cdda->fd) - cdda->offset) / RAW_SECTOR_SIZE) - cdda->lba - 2) * RAW_SECTOR_SIZE;
		}
	} else {
		cdda->track_size = total(cdda->fd) - cdda->offset - (cdda->size >> 1);
	}

	LOGFF("Track #%lu, %s %luHZ %d bits/sample, %lu bytes total,"
			  " format %d, LBA %ld\n", track, cdda->chn == 1 ? "mono" : "stereo", 
			  cdda->freq, cdda->bitsize, cdda->track_size, cdda->wav_format, cdda->lba);

#ifdef DEV_TYPE_SD
	if(cdda->bitsize == 4) {
		fs_enable_dma(cdda->size >> 13);
	} else {
		fs_enable_dma(cdda->size >> 12);
	}
#else
	if(IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1
		|| (IsoInfo->emu_cdda & CDDA_MODE_SRC_DMA)) {
		fs_enable_dma(FS_DMA_HIDDEN);
	}
#endif

	if(cdda->trans_method != PCM_TRANS_DMA) {
		if(cdda->bitsize == 16) {
			cdda->trans_method = PCM_TRANS_SQ_SPLIT;
		} else {
			cdda->trans_method = PCM_TRANS_SQ;
		}
	}

	cdda->stat = CDDA_STAT_FILL;

	gd_state_t *GDS = get_GDS();
	GDS->cdda_track = track;
	GDS->drv_stat = CD_STATUS_PLAYING;
	GDS->cdda_stat = SCD_AUDIO_STATUS_PLAYING;

	setup_pcm_buffer();
	aica_setup_cdda(1);
}

int CDDA_Play(uint32 first, uint32 last, uint32 loop) {

	gd_state_t *GDS = get_GDS();

	if(IsoInfo->emu_cdda == CDDA_MODE_DISABLED) {
		GDS->cdda_track = first;
		GDS->drv_stat = CD_STATUS_PLAYING;
		GDS->cdda_stat = SCD_AUDIO_STATUS_PLAYING;
		return COMPLETED;
	}

	if(cdda->stat || (first != cdda->first_track && cdda->fd > FILEHND_INVALID)) {
		CDDA_Stop();
	}

	cdda->first_track = first;
	cdda->first_lba = 0;
	cdda->last_track = last;
	cdda->last_lba = 0;
	cdda->loop = loop;

	play_track(first);

	return COMPLETED;
}


int CDDA_Play2(uint32 first_lba, uint32 last_lba, uint32 loop) {

	uint8 track = 0;

	for(int i = 3; i < 99; ++i) {
		if(IsoInfo->toc.entry[i] == (uint32)-1) {
			break;
		} else if(TOC_LBA(IsoInfo->toc.entry[i]) == TOC_LBA(first_lba)) {
			track = i + 1;
			break;
		} else if(TOC_LBA(IsoInfo->toc.entry[i]) > TOC_LBA(first_lba)) {
			track = i;
			break;
		}
	}
	if (track == 0) {
		return FAILED;
	}

	CDDA_Play(track, track, loop);

	if(cdda->fd > FILEHND_INVALID) {

		cdda->first_lba = TOC_LBA(first_lba);
		cdda->last_lba = TOC_LBA(last_lba);

		gd_state_t *GDS = get_GDS();
		GDS->lba = first_lba;

		uint32 offset = cdda->offset + ((first_lba - cdda->lba) * RAW_SECTOR_SIZE);
		lseek(cdda->fd, sector_align(offset), SEEK_SET);
	}
	return COMPLETED;
}


int CDDA_Seek(uint32 offset) {

	gd_state_t *GDS = get_GDS();

	if(IsoInfo->emu_cdda && cdda->fd > FILEHND_INVALID) {
		aica_stop_clean_cdda();
		uint32 value = cdda->offset + ((offset - cdda->lba) * RAW_SECTOR_SIZE);
		lseek(cdda->fd, sector_align(value), SEEK_SET);
		cdda->stat = CDDA_STAT_FILL;
		aica_setup_cdda(1);
	}
	GDS->lba = offset;
	return COMPLETED;
}


int CDDA_Pause(void) {

	gd_state_t *GDS = get_GDS();

	if(cdda->stat) {
		aica_stop_clean_cdda();
		cdda->cur_offset = tell(cdda->fd) - cdda->offset;
		GDS->cdda_stat = SCD_AUDIO_STATUS_PAUSED;
	}

	GDS->drv_stat = CD_STATUS_PAUSED;
	return COMPLETED;
}


int CDDA_Release() {

	gd_state_t *GDS = get_GDS();

	if(IsoInfo->emu_cdda && cdda->fd > FILEHND_INVALID) {
		
		lseek(cdda->fd, cdda->cur_offset + cdda->offset, SEEK_SET);
		cdda->stat = CDDA_STAT_FILL;
		aica_setup_cdda(1);
#ifdef _FS_ASYNC
#	ifdef DEV_TYPE_SD
		if(cdda->bitsize == 4)
			fs_enable_dma(cdda->size >> 13);
		else
			fs_enable_dma(cdda->size >> 12);
#	else
		fs_enable_dma(FS_DMA_HIDDEN);
#	endif
#endif /* _FS_ASYNC */
	}

	GDS->drv_stat = CD_STATUS_PLAYING;
	GDS->cdda_stat = SCD_AUDIO_STATUS_PLAYING;
	return COMPLETED;
}


int CDDA_Stop(void) {

	do {} while(lock_cdda());

	LOGFF(NULL);
	gd_state_t *GDS = get_GDS();

	if(cdda->stat) {
		aica_stop_clean_cdda();
	}

	if(cdda->fd > FILEHND_INVALID && IsoInfo->image_type != ISOFS_IMAGE_TYPE_CDI) {
		close(cdda->fd);
		cdda->fd = FILEHND_INVALID;
	}

	if(cdda->alloc_buff) {
		free(cdda->alloc_buff);
		cdda->alloc_buff = NULL;
	}

	GDS->cdda_track = 0;
	GDS->drv_stat = CD_STATUS_STANDBY;
	GDS->cdda_stat = SCD_AUDIO_STATUS_NO_INFO;
	unlock_cdda();

	return COMPLETED;
}

static void end_playback() {
	LOGFF(NULL);
	aica_stop_clean_cdda();
	gd_state_t *GDS = get_GDS();
	GDS->drv_stat = CD_STATUS_PAUSED;
	GDS->cdda_stat = SCD_AUDIO_STATUS_ENDED;
}

static void play_next_track() {

	if (cdda->stat != CDDA_STAT_END) {
		end_playback();
	}

	if (exception_inside_int() && IsoInfo->exec.type == BIN_TYPE_WINCE) {
		cdda->stat = CDDA_STAT_END;
		return;
	}

	LOGFF(NULL);

	if(cdda->first_track < cdda->last_track) {

		gd_state_t *GDS = get_GDS();
		uint32 track = GDS->cdda_track;

		if(GDS->cdda_track < cdda->last_track) {
			track++;
		} else {

			if(cdda->loop == 0) {
				return;
			} else if(cdda->loop < 0xf) {
				cdda->loop--;
			}
			track = cdda->first_track;
		}

		play_track(track);

	} else if(cdda->loop != 0) {

		if (cdda->loop < 0xf) {
			cdda->loop--;
		}

		if(cdda->first_lba == 0) {
			CDDA_Play(cdda->first_track, cdda->last_track, cdda->loop);
		} else {
			CDDA_Play2(cdda->first_lba, cdda->last_lba, cdda->loop);
		}
	}
}

#ifdef _FS_ASYNC
static void read_callback(size_t size) {
	DBGF("CDDA: FILL END %d\n", size);
	if(cdda->stat > CDDA_STAT_IDLE) {
		if(size == (size_t)-1) {
			lseek(cdda->fd, cdda->cur_offset + cdda->offset, SEEK_SET);
			cdda->stat = CDDA_STAT_FILL;
		} else if(size < cdda->size >> 1) {
			cdda->stat = CDDA_STAT_FILL;
		} else {
			cdda->stat = CDDA_STAT_PREP;
		}
	}
}
#endif

static void fill_pcm_buff() {

	/* On first loading the channels is muted */
	if(cdda->volume) {
		/* FIXME: Strange issue in rare cases */
		if ((tell(cdda->fd) - cdda->offset) >= cdda->track_size && cdda->loop == 0) {
			cdda->loop = 1;
		}
		if(cdda->cur_offset) {
			aica_set_volume(0);
		}
	}

	cdda->cur_offset = tell(cdda->fd) - cdda->offset;

	if(cdda->last_lba > 0) {

		uint32 offset = (cdda->cur_offset / RAW_SECTOR_SIZE) + cdda->lba;

		if (offset > cdda->last_lba) {
			play_next_track();
			return;
		}
	}
	if(cdda->cur_offset >= cdda->track_size) {
		play_next_track();
		return;
	}

	DBGF("CDDA: FILL %ld %ld\n", cdda->cur_offset, cdda->track_size - cdda->cur_offset);

#ifdef _FS_ASYNC
	cdda->stat = CDDA_STAT_WAIT;
	int rc = read_async(cdda->fd, cdda->buff[PCM_TMP_BUFF], cdda->size >> 1, read_callback);
#else
	int rc = read(cdda->fd, cdda->buff[PCM_TMP_BUFF], cdda->size >> 1);
	cdda->stat = CDDA_STAT_PREP;
#endif

	if(rc < 0) {
		LOGFF("Unable to read data\n");
		end_playback();
	}
}

void CDDA_MainLoop(void) {

	if(lock_cdda()) {
		return;
	}

	if(cdda->stat == CDDA_STAT_IDLE) {
		unlock_cdda();
		return;
	}

	DBGFF("0x%08lx %d\n", IsoInfo->emu_cdda, cdda->stat);

	if(cdda->stat == CDDA_STAT_END && !exception_inside_int()) {
		play_next_track();
	}

#ifdef _FS_ASYNC
	if(cdda->stat == CDDA_STAT_WAIT) {
		/* Polling async data transfer */
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		if(!exception_inited() || !pre_read_xfer_busy())
#endif
		{
			if(poll(cdda->fd) < 0) {
				lseek(cdda->fd, cdda->cur_offset + cdda->offset, SEEK_SET);
				cdda->stat = CDDA_STAT_FILL;
			}
		}
		unlock_cdda();
		return;
	}
#endif

	/* Reading data for left or all channels */
	if(cdda->stat == CDDA_STAT_FILL) {
		aica_check_cdda();
		fill_pcm_buff();
		/* Update LBA for SCD command */
		gd_state_t *GDS = get_GDS();
		GDS->lba = cdda->lba + (cdda->cur_offset / RAW_SECTOR_SIZE);
		unlock_cdda();
		return;
	}

	/* Split PCM data to left and right channels */
	if(cdda->stat == CDDA_STAT_PREP) {
		if (cdda->trans_method == PCM_TRANS_DMA) {
			if (aica_dma_in_progress()) {
				unlock_cdda();
				return;
			}
			int old = irq_disable();
			aica_dma_irq_restore();
			irq_restore(old);
		}
		if (cdda->trans_method != PCM_TRANS_SQ_SPLIT) {
			aica_pcm_split(cdda->buff[PCM_TMP_BUFF], cdda->buff[PCM_DMA_BUFF], cdda->size >> 1);
		}
		cdda->stat = CDDA_STAT_POS;
	}

	/* Wait suitable channels position */
	if(cdda->stat == CDDA_STAT_POS && aica_suitable_pos()) {
		cdda->stat = CDDA_STAT_SNDL;
	}

	/* Send data to AICA */
	if(cdda->stat == CDDA_STAT_SNDL && aica_dma_in_progress() == 0) {
		if (cdda->trans_method == PCM_TRANS_SQ_SPLIT) {
			aica_pcm_split_sq((uint32)cdda->buff[PCM_TMP_BUFF],
					cdda->aica_left[cdda->cur_buff],
					cdda->aica_right[cdda->cur_buff],
					cdda->size >> 1);
			cdda->cur_buff = !cdda->cur_buff;
			cdda->stat = CDDA_STAT_FILL;
		} else {
			aica_transfer(cdda->buff[PCM_DMA_BUFF], cdda->aica_left[cdda->cur_buff], cdda->size >> 2);
			cdda->stat = CDDA_STAT_SNDR;
		}
	}
	/* If transfer of left channel is done, start for right channel */
	else if(cdda->stat == CDDA_STAT_SNDR && aica_dma_in_progress() == 0) {
		uint32 size = cdda->size >> 2;
		aica_transfer(cdda->buff[PCM_DMA_BUFF] + size, cdda->aica_right[cdda->cur_buff], size);
		cdda->cur_buff = !cdda->cur_buff;
		cdda->stat = CDDA_STAT_FILL;
	}

	unlock_cdda();
}

#ifdef DEBUG
void CDDA_Test() {
	/* Internal CDDA test */
//	uint32 next = loader_addr;
	int i = 0, track = 4;
	
	while(1) {
		
		i = 0;
//		next = next * 1103515245 + 12345;
//		track = (((uint)(next / 65536) % 32768) % 32) + 4;
		track++;
		CDDA_Play(track, track, 15);
		
		while(i++ < 400000) {
			CDDA_MainLoop();
			timer_spin_sleep(15);
		}
	}
}
#endif
