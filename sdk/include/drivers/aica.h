/** 
 * \file    aica.h
 * \brief   Definitions for AICA sound system
 * \date    2013-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_AICA_H
#define _DS_AICA_H

/**
 * Sound memory start address (0x00800000 - 0x009FFFE0) 
 */
#define AICA_DMA_ADSTAG *((vuint32 *)0xA05F7800)

/**
 * System memory start address (0x0C000000 - 0x0FFFFFE0) 
 */
#define AICA_DMA_ADSTAR *((vuint32 *)0xA05F7804)

/**
 * Transfer size (32-byte aligned)
 * The setting of bit 31 enables DMA initiation (AICA_DMA_ADEN) when a DMA transfer ends.
 * 0x00000000 - Do not set DMA initiation setting to 0
 * 0x80000000 - Set DMA initiation setting to 0
 */
#define AICA_DMA_ADLEN *((vuint32 *)0xA05F7808)

/**
 * Transfer direction
 * 0x00000000 - System memory to sound memory
 * 0x00000001 - Sound memory to system memory
 */
#define AICA_DMA_ADDIR *((vuint32 *)0xA05F780C)

/**
 * DMA initiation method
 * 0x00000000 - Initiation by CPU
 * 0x00000002 - Initiation by IRQ
 * 
 * The setting of bit 3 used for enabling/disabling suspend register
 */
#define AICA_DMA_ADTRG *((vuint32 *)0xA05F7810)

/**
 * DMA operation
 * 0x00000000 - DMA enabled
 * 0x00000001 - DMA disabled
 */
#define AICA_DMA_ADEN *((vuint32 *)0xA05F7814)

/**
 * DMA initiation by CPU
 * 0x00000000 - Nothing to do
 * 0x00000001 - Initiation of DMA 
 */
#define AICA_DMA_ADST *((vuint32 *)0xA05F7818)

/**
 * DMA suspend
 * 0x00000000 - Resume
 * 0x00000001 - Suspend
 */
#define AICA_DMA_ADSUSP *((vuint32 *)0xA05F781C)

/**
 * DMA address counter on sound memory
 */
#define AICA_DMA_ADSTAGD *((vuint32 *)0xA05F78C0)

/**
 * DMA address counter on system memory
 */
#define AICA_DMA_ADSTARD *((vuint32 *)0xA05F78C4)

/**
 * DMA transfer counter
 */
#define AICA_DMA_ADLEND *((vuint32 *)0xA05F78C8)

/**
 * System memory protection
 * (shared to other G2 devices too)
 */
#define AICA_DMA_G2APRO *((vuint32 *)0xA05F78BC)

/**
 * System memory protection codes
 */
#define AICA_DMA_G2APRO_LOCK   0x46597F00
#define AICA_DMA_G2APRO_UNLOCK 0x4659007F

/**
 * AICA macros
 */
#define SPU_RAM_BASE      0xA0800000
#define SNDREGADDR(x)     (0xA0700000 + (x))
#define CHNREGADDR(ch, x) SNDREGADDR(0x80 * (ch) + (x))
#define SNDREG32(x)       (*(vuint32 *)SNDREGADDR(x))
#define CHNREG32(ch, x)   (*(vuint32 *)CHNREGADDR(ch, x))


/* Initialize AICA with custom firmware */
int snd_init_firmware(const char *filename);

/* Initialize AICA with KOS firmware */
int snd_init();

/* Shutdown AICA */
int snd_shutdown();

#endif /* _DS_AICA_H */
