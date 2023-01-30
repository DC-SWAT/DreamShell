/**
 * DreamShell ISO Loader
 * CDDA audio playback emulation
 * (c)2014-2022 SWAT <http://www.dc-swat.ru>
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
#define AICA_SM_16BIT    0
#define AICA_SM_8BIT     1
#define AICA_SM_ADPCM    3
#define AICA_SM_ADPCM_LS 4

/* WAV sample formats */
#define WAVE_FMT_PCM                   0x0001 /* PCM */
#define WAVE_FMT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */
#define WAVE_FMT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */

/* AICA channels for CDDA audio playback */
#define AICA_CDDA_CH_LEFT  62
#define AICA_CDDA_CH_RIGHT 63

/* SPU timer for checking playback position (not used) */
#define AICA_CDDA_TIMER_ARM 0x2894 /* Timer B */

/* AICA memory end for PCM buffer */
#define AICA_MEMORY_END 0x00A00000
#define AICA_MEMORY_END_ARM 0x00200000

#define G2BUS_FIFO (*(vuint32 *)0xa05f688c)

#define aica_dma_in_progress() AICA_DMA_ADST
#define aica_dma_wait() do { } while(AICA_DMA_ADST)

#define RAW_SECTOR_SIZE 2352

static cdda_ctx_t _cdda;
static cdda_ctx_t *cdda = &_cdda;

cdda_ctx_t *get_CDDA(void) {
	return &_cdda;
}

/* G2 Bus locking */
static void g2_lock(void) {
	AICA_DMA_ADSUSP = 1;
	cdda->g2_lock = irq_disable();
	do { } while(G2BUS_FIFO & 0x20) ;
}

static void g2_unlock(void) {
	irq_restore(cdda->g2_lock);
	AICA_DMA_ADSUSP = 0;
}

/* When writing to the SPU RAM, this is required at least every 8 32-bit
   writes that you execute */
static void g2_fifo_wait() {
	for (uint32 i = 0; i < 0x1800; ++i) {
		if (!(G2BUS_FIFO & 0x11)) {
			break;
		}
	}
}

static void aica_dma_irq_disable() {
	if (*ASIC_IRQ9_MASK & ASIC_NRM_AICA_DMA) {

		cdda->irq_index = 9;
		*ASIC_IRQ9_MASK &= ~ASIC_NRM_AICA_DMA;

	} else if (*ASIC_IRQ11_MASK & ASIC_NRM_AICA_DMA) {

		cdda->irq_index = 11;
		*ASIC_IRQ11_MASK &= ~ASIC_NRM_AICA_DMA;

	} else if (*ASIC_IRQ13_MASK & ASIC_NRM_AICA_DMA) {

		cdda->irq_index = 13;
		*ASIC_IRQ13_MASK &= ~ASIC_NRM_AICA_DMA;

	} else {
		cdda->irq_index = (uint32)-1;
	}
}

static void aica_dma_irq_restore() {

	if (cdda->irq_index == 0) {
		return;
	} else if (cdda->irq_index == 9) {
		*ASIC_IRQ9_MASK |= ASIC_NRM_AICA_DMA;
	} else if (cdda->irq_index == 11) {
		*ASIC_IRQ11_MASK |= ASIC_NRM_AICA_DMA;
	} else if (cdda->irq_index == 13) {
		*ASIC_IRQ13_MASK |= ASIC_NRM_AICA_DMA;
	}

	cdda->irq_index = 0;
}

static void aica_dma_transfer(uint8 *data, uint32 dest, uint32 size) {
	
	DBGFF("0x%08lx %ld\n", data, size);
	
	uint32 addr = (uint32)data;
	dcache_purge_range(addr, size);
	addr = addr & 0x0FFFFFFF;
	
	AICA_DMA_G2APRO = 0x4659007F;      // Protection code
	AICA_DMA_ADEN   = 0;               // Disable wave DMA
	AICA_DMA_ADDIR  = 0;               // To wave memory
	AICA_DMA_ADTRG  = 0x00000004;      // Initiate by CPU, suspend enabled
	AICA_DMA_ADSTAR = addr;            // System memory address
	AICA_DMA_ADSTAG = dest;            // Wave memory address
	AICA_DMA_ADLEN  = size|0x80000000; // Data size, disable after DMA end
	AICA_DMA_ADEN   = 1;               // Enable wave DMA
	AICA_DMA_ADST   = 1;               // Start wave DMA
}

static void aica_sq_transfer(uint8 *data, uint32 dest, uint32 size) {

	uint32 *d = (uint32 *)(void *)(0xe0000000 | (dest & 0x03ffffe0));
	uint32 *s = (uint32 *)data;

	/* Set store queue memory area as desired */
	QACR0 = ((dest >> 26) << 2) & 0x1c;
	QACR1 = ((dest >> 26) << 2) & 0x1c;
	
	/* fill/write queues as many times necessary */
	size >>= 5;
	g2_fifo_wait();
	
	while(size--) {
//		g2_fifo_wait();
		/* prefetch 32 bytes for next loop */
		__asm__("pref @%0" : : "r"(s + 8));
		d[0] = s[0];
		d[1] = s[1];
		d[2] = s[2];
		d[3] = s[3];
		d[4] = s[4];
		d[5] = s[5];
		d[6] = s[6];
		d[7] = s[7];
		__asm__("pref @%0" : : "r"(d));
		d += 8;
		s += 8;
	}
}

static void aica_transfer(uint8 *data, uint32 dest, uint32 size) {
	if (IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1) {
		aica_dma_irq_disable();
		aica_dma_transfer(data, dest, size);
	} else {
		aica_sq_transfer(data, dest, size);
	}
}

static int aica_transfer_in_progress() {

	if (IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1) {
		return aica_dma_in_progress();
	}

	// TODO: SQ
	return 0;
}

static void aica_transfer_wait() {
	if (IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1) {
		aica_dma_wait();
	} else {
		/* Wait for both store queues to complete */
		vuint32 *d = (vuint32 *)0xe0000000;
		d[0] = d[8] = 0;
	}
}

static void aica_transfer_stop() {
	if (IsoInfo->emu_cdda <= CDDA_MODE_DMA_TMU1) {
		AICA_DMA_ADEN = 0;
	} else {
		// TODO: SQ
	}
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
			cdda->end_tm *= 2;   /* 4 bits decoded to 16 bits */
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
				cdda->size >>= 1;
				cdda->end_tm >>= 1;
#endif
			}
			break;
	}

	if (cdda->alloc_buff && old_size != cdda->size) {
		free(cdda->alloc_buff);
		cdda->alloc_buff = NULL;
	}

	if (cdda->alloc_buff == NULL) {
		cdda->alloc_buff = malloc(cdda->size + 32);
	}

	if (cdda->alloc_buff == NULL) {
		LOGFF("Failed malloc");
		return;
	}

	cdda->buff[0] = (uint8 *)ALIGN32_ADDR((uint32)cdda->alloc_buff);
	cdda->buff[1] = cdda->buff[0] + (cdda->size >> 1);

	/* Setup buffer at end of sound memory */
	cdda->aica_left[0] = AICA_MEMORY_END - cdda->size;
	cdda->aica_left[1] = cdda->aica_left[0] + (cdda->size >> 2);

	cdda->aica_right[0] = AICA_MEMORY_END - (cdda->size >> 1);
	cdda->aica_right[1] = cdda->aica_right[0] + (cdda->size >> 2);

	/* Setup end position for sound buffer */
	if(cdda->aica_format == AICA_SM_ADPCM) {
		cdda->end_pos = cdda->size - 1;
	} else {
		cdda->end_pos = ((cdda->size / cdda->chn) / (cdda->bitsize >> 3)) - (cdda->bitsize >> 3);
	}

	LOGFF("0x%08lx 0x%08lx %d\n",
		(uint32)cdda->buff[0], (uint32)cdda->aica_left[0], cdda->size);
}

static void aica_set_volume(int volume) {

	uint32 val;
	cdda->volume = volume;

	g2_fifo_wait();
	g2_lock();
	
	/* Get original CDDA volume (doesn't work) */
//	SNDREG32(0x2040) & 0xff00; // Left
//	SNDREG32(0x2044) & 0xff00; // Right
	
	val = 0x24 | (volume << 8); //(AICA_VOL(vol) << 8);
	
	CHNREG32(AICA_CDDA_CH_LEFT,  40) = val;
	
	if(cdda->chn > 1) {

//		val = 0x24 | (right_chn << 8); //(AICA_VOL(right_chn) << 8);
		CHNREG32(AICA_CDDA_CH_RIGHT, 40) = val;
	}

	g2_unlock();
}

static void aica_stop_cdda(void) {
	
	uint32 l, r;
	LOGFF(NULL);
	
	g2_fifo_wait();
	g2_lock();
	
	l = CHNREG32(AICA_CDDA_CH_LEFT,  0);
	
	if(cdda->chn > 1) {
		r = CHNREG32(AICA_CDDA_CH_RIGHT, 0);
		CHNREG32(AICA_CDDA_CH_RIGHT, 0) = (r & ~0x4000) | 0x8000;
	}
	
	CHNREG32(AICA_CDDA_CH_LEFT,  0) = (l & ~0x4000) | 0x8000;
	
	g2_unlock();

	if(aica_transfer_in_progress()) {
		aica_transfer_stop();
	}

	/* Wait transfer done */
	aica_transfer_wait();
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

	/* Setup SH4 timer */
	if (IsoInfo->emu_cdda == CDDA_MODE_DMA_TMU1 || IsoInfo->emu_cdda == CDDA_MODE_SQ_TMU1) {
		cdda->timer = TMU1;
	} else {
		cdda->timer = TMU2;
	}

	timer_prime_cdda(cdda->timer, cdda->end_tm);

	LOGFF("0x%08lx 0x%08lx %d %d %d %d\n",
				smp_ptr, (smp_ptr + smp_size), smp_size, 
				cdda->freq, cdda->aica_format, cdda->end_tm);

	if(clean) {
		/* Stop AICA channels */
		aica_stop_cdda();
	}

	/* Setup AICA channels */
	g2_fifo_wait();
	g2_lock();

	/* Setup AICA timer */
//	SNDREG32(AICA_CDDA_TIMER_ARM) = 6 << 8;

	CHNREG32(AICA_CDDA_CH_LEFT, 8) = 0;
	CHNREG32(AICA_CDDA_CH_LEFT, 12) = cdda->end_pos & 0xffff;
	CHNREG32(AICA_CDDA_CH_LEFT, 24) = freq_base;

	if(cdda->chn > 1) {

		CHNREG32(AICA_CDDA_CH_RIGHT, 8) = 0;
		CHNREG32(AICA_CDDA_CH_RIGHT, 12) = cdda->end_pos & 0xffff;
		CHNREG32(AICA_CDDA_CH_RIGHT, 24) = freq_base;

		CHNREG32(AICA_CDDA_CH_LEFT, 36) = AICA_PAN(0)   | (0xf << 8);
		CHNREG32(AICA_CDDA_CH_RIGHT, 36) = AICA_PAN(255) | (0xf << 8);

	} else {
		CHNREG32(AICA_CDDA_CH_LEFT, 36) = AICA_PAN(128) | (0xf << 8);
	}

	g2_unlock();

	aica_set_volume(clean ? 255 : 0);

	g2_fifo_wait();
	g2_lock();

	CHNREG32(AICA_CDDA_CH_LEFT, 16) = 0x1f;
	CHNREG32(AICA_CDDA_CH_LEFT, 4)  = smp_ptr & 0xffff;

	if(cdda->chn > 1) {
		CHNREG32(AICA_CDDA_CH_RIGHT, 16) = 0x1f;
		CHNREG32(AICA_CDDA_CH_RIGHT, 4)  = (smp_ptr + smp_size) & 0xffff;
	}

	/* start play | format | use loop */
	val = 0xc000 | (cdda->aica_format << 7) | 0x200;

	cdda->check_val = val;
	CHNREG32(AICA_CDDA_CH_LEFT, 0) = val | (smp_ptr >> 16);

	if(cdda->chn > 1) {
		CHNREG32(AICA_CDDA_CH_RIGHT, 0) = val | ((smp_ptr + smp_size) >> 16);
	}

	/* Start AICA timer */
	// SNDREG32(AICA_CDDA_TIMER_ARM) = 6 << 8 | 0x01;

	g2_unlock();
	g2_fifo_wait();

	/* Start SH4 timer */
	timer_start(cdda->timer);
}

static void aica_check_cdda(void) {

	uint32 left_val, right_val;

	g2_fifo_wait();
	g2_lock();

	left_val = CHNREG32(AICA_CDDA_CH_LEFT, 0);

	if(cdda->chn > 1) {
		right_val = CHNREG32(AICA_CDDA_CH_RIGHT, 0);
	} else {
		right_val = cdda->check_val;
	}

	g2_unlock();

	if(!(left_val & 0x4000)
		|| !(right_val & 0x4000)
		|| !(left_val & cdda->check_val)
		|| !(right_val & cdda->check_val)
	) {
		aica_transfer_wait();
		aica_setup_cdda(0);
	}
}

#if 0 // For debugging

/* Get channel position */
static int aica_get_pos(void) {

	int p;
	
	g2_fifo_wait();
	
	/* Observe channel ch */
	g2_lock();
	SNDREG32(0x280c) = (SNDREG32(0x280c) & 0xffff00ff) | (AICA_CDDA_CH_LEFT << 8);
	g2_unlock();
	
	g2_fifo_wait();
	
	/* Update position counters */
	g2_lock();
	p = SNDREG32(0x2814) & 0xffff;
	g2_unlock();
	
//	LOGFF("%d\n", p);

	return p;
}

static uint8 aica_get_tmval(void) {
	uint8 val;
	
	g2_lock();
	val = SNDREG32(AICA_CDDA_TIMER_ARM) & 0xff;
	g2_unlock();
	
	return val;
}


static void aica_init(void) {
	
	g2_fifo_wait();
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
	
	/* AICA Timer */
//	SNDREG32(AICA_CDDA_TIMER_ARM) = 6 << 8 | 0xd4;
	
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

static void aica_pcm_split(uint8 *src, uint8 *dst, uint32 size) {
	
	DBGFF("0x%08lx 0x%08lx %ld\n", src, dst, size);
	
	if(cdda->chn > 1/* && cdda->wav_format != WAVE_FMT_YAMAHA_ADPCM_ITU_G723*/) {
		
		uint32 count = size >> 1;
		
		switch(cdda->bitsize) {
			case 4:
				adpcm_split(src, dst, dst + count, size);
				break;
			case 8:
				pcm8_split(src, dst, dst + count, size);
				break;
			case 16:
				pcm16_split((int16 *)src, (int16 *)dst, (int16 *)(dst + count), size);
				break;
			default:
				break;
		}
		
	} else {
		/* TODO optimize it, maybe just switch the buffer? */
		memcpy(dst, src, size);
	}
}

static void switch_cdda_track(uint8 track) {

	gd_state_t *GDS = get_GDS();

	if(GDS->cdda_track != track) {

		if(!cdda->fn_len) {
			cdda->fn_len = strlen(IsoInfo->image_file);
		}

		memcpy(cdda->filename, IsoInfo->image_file, sizeof(IsoInfo->image_file));

		if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {

			cdda->filename[cdda->fn_len - 3] = 'r';
			cdda->filename[cdda->fn_len - 2] = 'a';
			cdda->filename[cdda->fn_len - 1] = 'w';

		} else {

			uint8 *val = (uint8 *)cdda->filename, *tmp = NULL;

			do {
				tmp = tmp ? val + 1 : val;
				val = memchr(tmp, '/', cdda->fn_len);
			} while(val != NULL);

			memcpy(tmp, "track04.raw\0", 12);
			cdda->fn_len = strlen(cdda->filename);
		}

		cdda->filename[cdda->fn_len - 6] = (track / 10) + '0';
		cdda->filename[cdda->fn_len - 5] = (track % 10) + '0';

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
				cdda->filename[cdda->fn_len] = '.';
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


static void stop_clean_cdda() {

	DBGFF(NULL);
	aica_set_volume(255);

#ifdef _FS_ASYNC
	while(cdda->stat == CDDA_STAT_WAIT) {
		if (poll(cdda->fd) < 0) {
			break;
		}
	}
#endif

	cdda->stat = CDDA_STAT_IDLE;
	aica_transfer_wait();
	aica_stop_cdda();
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

void *aica_dma_handler(void *passer, register_stack *stack, void *current_vector) {

	(void)passer;
	(void)stack;

	if (cdda->irq_index == 0) {
		return current_vector;
	}

	uint32 code = *REG_INTEVT;

	if (cdda->irq_index && (
		(code == EXP_CODE_INT11 && cdda->irq_index == 11)
		|| (code == EXP_CODE_INT9 && cdda->irq_index == 9)
		|| (code == EXP_CODE_INT13 && cdda->irq_index == 13)
	)) {
		ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_AICA_DMA;
		return my_exception_finish;
	}
	return current_vector;
}

# ifndef NO_ASIC_LT
static asic_handler_f old_vsync_handler;
static asic_handler_f old_dma_handler;
static void *vsync_handler(void *passer, register_stack *stack, void *current_vector) {

	if(old_vsync_handler) {
		current_vector = old_vsync_handler(passer, stack, current_vector);
	}

	CDDA_MainLoop();
	return current_vector;
}
static void *dma_handler(void *passer, register_stack *stack, void *current_vector) {

	if(old_dma_handler) {
		current_vector = old_dma_handler(passer, stack, current_vector);
	}

	return aica_dma_handler(passer, stack, current_vector);
}
# endif
#endif

int CDDA_Init() {
	static int inited = 0;

	if (inited) {
		return 0;
	}

	inited = 1;
	memset(cdda, 0, sizeof(cdda_ctx_t));

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI) {
		cdda->fd = iso_fd;
	} else {
		cdda->filename = (char *)malloc(sizeof(IsoInfo->image_file));
		cdda->fd = FILEHND_INVALID;
	}

#if 0
	/* It's not need for games (they do it), only for local test */
	aica_init();
	aica_dma_init();
#endif

#if defined(HAVE_EXPT) && !defined(NO_ASIC_LT)
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

	return asic_add_handler(&vsync_entry, &old_vsync_handler, 0);
#else
	return 0;
#endif
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
			case WAVE_FMT_PCM:
				cdda->aica_format = cdda->bitsize == 8 ? AICA_SM_8BIT : AICA_SM_16BIT;
				break;
			case WAVE_FMT_YAMAHA_ADPCM:
			case WAVE_FMT_YAMAHA_ADPCM_ITU_G723:
				cdda->aica_format = AICA_SM_ADPCM;
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

	if(!IsoInfo->emu_cdda) {
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
		} else if(TOC_LBA(IsoInfo->toc.entry[i]) == first_lba) {
			track = i + 1;
			break;
		} else if(TOC_LBA(IsoInfo->toc.entry[i]) > first_lba) {
			track = i;
			break;
		}
	}

	CDDA_Play(track, track, loop);

	if(cdda->fd != FILEHND_INVALID) {

		cdda->first_lba = first_lba;
		cdda->last_lba = last_lba;

		gd_state_t *GDS = get_GDS();
		GDS->lba = first_lba;

		uint32 offset = cdda->offset + ((first_lba - cdda->lba) * RAW_SECTOR_SIZE);
		lseek(cdda->fd, sector_align(offset), SEEK_SET);
	}
	return COMPLETED;
}


int CDDA_Seek(uint32 offset) {

	gd_state_t *GDS = get_GDS();

	if(IsoInfo->emu_cdda && cdda->fd > -1) {
		stop_clean_cdda();
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
		stop_clean_cdda();
		cdda->cur_offset = tell(cdda->fd) - cdda->offset;
	}

	GDS->drv_stat = CD_STATUS_PAUSED;
	GDS->cdda_stat = SCD_AUDIO_STATUS_PAUSED;
	return COMPLETED;
}


int CDDA_Release() {

	gd_state_t *GDS = get_GDS();

	if(IsoInfo->emu_cdda && cdda->fd > FILEHND_INVALID) {
		
		lseek(cdda->fd, cdda->cur_offset, SEEK_SET);
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

	LOGFF(NULL);
	gd_state_t *GDS = get_GDS();

	if(cdda->stat) {
		stop_clean_cdda();
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
	return COMPLETED;
}

static void end_playback() {
	LOGFF(NULL);
	stop_clean_cdda();
	gd_state_t *GDS = get_GDS();
	GDS->drv_stat = CD_STATUS_PAUSED;
	GDS->cdda_stat = SCD_AUDIO_STATUS_ENDED;
}

static void play_next_track() {

	end_playback();
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
	DBGFF("%d\n", size);
	if(cdda->stat > CDDA_STAT_IDLE) {
		if((int)size < 0) {
			end_playback();
		} else if(size < cdda->size >> 1) {
			cdda->stat = CDDA_STAT_FILL;
		} else {
			cdda->stat = CDDA_STAT_PREP;
		}
	}
}
#endif

static void fill_pcm_buff() {

	if(cdda->volume && cdda->cur_offset) {
		aica_set_volume(0);
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

	DBGFF("%ld %ld\n", cdda->cur_offset, cdda->track_size - cdda->cur_offset);

	/* Reading data for all channels */
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

	if(cdda->stat == CDDA_STAT_IDLE) {
		return;
	}

	DBGFF("%d %d\n", IsoInfo->emu_cdda, cdda->stat);

#ifdef _FS_ASYNC
	if(cdda->stat == CDDA_STAT_WAIT) {
		/* Polling async data transfer */
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		if(!exception_inited() || !pre_read_xfer_busy())
#endif
		{
			if(poll(cdda->fd) < 0) {
				/* Retry on error */
				cdda->stat = CDDA_STAT_FILL;
			}
		}
		aica_check_cdda();
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
		return;
	}

	/* Split PCM data to left and right channels */
	if(cdda->stat == CDDA_STAT_PREP) {

		aica_check_cdda();
	
		if(!aica_transfer_in_progress()) {

			if(cdda->irq_index) {
				aica_dma_irq_restore();
			}

			aica_pcm_split(cdda->buff[PCM_TMP_BUFF], cdda->buff[PCM_DMA_BUFF], cdda->size >> 1);
			cdda->stat = CDDA_STAT_SNDL;
		}
	}
	
	/* Send data to AICA */
	if(cdda->stat == CDDA_STAT_SNDL) {
		
		uint32 tm = timer_count(cdda->timer);
		uint32 ta = cdda->end_tm >> 1;
//		gd_state_t *GDS = get_GDS();
//		int pos = aica_get_pos();
//		uint8 atm = aica_get_tmval();
//		
//		LOGFF("SH4_TIMER: %d AICA_POS: %d AICA_TIMER: %d LBA: %ld\n", 
//					tm, pos, atm, GDS->lba);

		/* 
		 * If the AICA playing position leave needed memory, 
		 * start the data transfer for left channel
		 */
//		if((!cdda->cur_buff && pos >= (cdda->end_pos >> 1)) || 
//			(cdda->cur_buff && pos < (cdda->end_pos >> 1))) {
//		if((!cdda->cur_buff && atm >= 128) || 
//			(cdda->cur_buff && atm < 128)) {
		if((!cdda->cur_buff && tm < ta) || (cdda->cur_buff && tm > ta)) {

			if(!aica_transfer_in_progress()) {

				if(cdda->chn > 1) {
					aica_transfer(cdda->buff[PCM_DMA_BUFF], cdda->aica_left[cdda->cur_buff], cdda->size >> 2);
#ifdef DEV_TYPE_SD
					fill_pcm_buff();
#endif
					cdda->stat = CDDA_STAT_SNDR;
				} else {
					aica_transfer(cdda->buff[PCM_DMA_BUFF], cdda->cur_buff ? cdda->aica_right[0] : cdda->aica_left[0], cdda->size >> 1);
					cdda->cur_buff = cdda->cur_buff ? 0 : 1;
					cdda->stat = CDDA_STAT_FILL;
				}
			}
		}
	}
	
	/* If transfer of left channel is done, start for right channel */
	else if(cdda->stat == CDDA_STAT_SNDR && !aica_transfer_in_progress()) {

		uint32 size = cdda->size >> 2;

		aica_transfer(cdda->buff[PCM_DMA_BUFF] + size, cdda->aica_right[cdda->cur_buff], size);
		cdda->cur_buff = cdda->cur_buff ? 0 : 1;
		
#ifdef DEV_TYPE_SD
		if(cdda->stat == CDDA_STAT_WAIT) {
			poll(cdda->fd);
		} else {
			cdda->stat = CDDA_STAT_FILL;
		}
#else
		cdda->stat = CDDA_STAT_FILL;
#endif
	}
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
