/** 
 * \file    isoldr.h
 * \brief   DreamShell ISO loader
 * \date    2009-2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_ISOLDR_H_
#define _DS_ISOLDR_H_

#include <arch/types.h>
#include <dc/cdrom.h>
#include "isofs/isofs.h"
#include "isofs/ciso.h"


/**
 * Loader params size
 */
#define ISOLDR_PARAMS_SIZE 1024


/**
 * Desired maximum memory usage by loader and params
 */
#define ISOLDR_MAX_MEM_USAGE 32768


/**
 * It's a default loader addresses
 *
 * You can find more suitable by own experience.
 */
/* All loaders compiled for this address */
#define ISOLDR_DEFAULT_ADDR              0x8ce00000
/* Default address for base loader */
#define ISOLDR_DEFAULT_ADDR_LOW          0x8c004000
/* Default address at end of memory */
#define ISOLDR_DEFAULT_ADDR_HIGH         0x8cfe8000
/* Minimum possible address */
#define ISOLDR_DEFAULT_ADDR_MIN          0x8c000100
/* Minimum possible address if the GINSU are used */
#define ISOLDR_DEFAULT_ADDR_MIN_GINSU    0x8c001100


/**
 * Supported devices
 */
#define ISOLDR_DEV_GDROM  "cd"
#define ISOLDR_DEV_SDCARD "sd"
#define ISOLDR_DEV_G1ATA  "ide"
#define ISOLDR_DEV_DCLOAD "dcl"


/**
 * Supported types
 */
#define ISOLDR_TYPE_DEFAULT  ""
#define ISOLDR_TYPE_CDDA     "cdda"
#define ISOLDR_TYPE_VMU      "vmu"
#define ISOLDR_TYPE_FEAT     "feat"
#define ISOLDR_TYPE_FULL     "full"


/**
 * Boot mode
 */
typedef enum isoldr_boot_mode {
	BOOT_MODE_DIRECT = 0,
	BOOT_MODE_IPBIN = 1,  /* Bootstrap 1 */
	BOOT_MODE_IPBIN_TRUNC /* Bootstrap 2 */
} isoldr_boot_mode_t;


/**
 * Executable types
 */
typedef enum isoldr_exec_type {
	BIN_TYPE_AUTO = 0,
	BIN_TYPE_KOS = 1,
	BIN_TYPE_KATANA,
	BIN_TYPE_WINCE,
	BIN_TYPE_NAOMI
} isoldr_exec_type_t;


/**
 * Executable info
 */
typedef struct isoldr_exec_info {
	
	uint32 lba;               /* File LBA */
	uint32 size;              /* Size in bytes */
	uint32 addr;              /* Memory address */
	char   file[16];          /* File name */
	uint32 type;              /* See isoldr_exec_type_t */
	
} isoldr_exec_info_t;


/**
 * Heap memory modes
 */
typedef enum isoldr_heap_mode {
	HEAP_MODE_AUTO = 0,
	HEAP_MODE_BEHIND = 1,
	HEAP_MODE_INGAME,
	HEAP_MODE_MAPLE,
	HEAP_MODE_SPECIFY = 0x8c000000 // +offset
} isoldr_heap_mode_t;


/**
 * CDDA modes
 */
typedef enum isoldr_cdda_mode {
	CDDA_MODE_DISABLED = 0,

	/* Simple mode selection */
	CDDA_MODE_DMA_TMU2 = 1,
	CDDA_MODE_DMA_TMU1 = 2,
	CDDA_MODE_SQ_TMU2 = 3,
	CDDA_MODE_SQ_TMU1 = 4,

	/* Extended mode selection */
	CDDA_MODE_EXTENDED = 5,
	CDDA_MODE_SRC_DMA = 0x00000010,
	CDDA_MODE_SRC_PIO = 0x00000020,
	CDDA_MODE_DST_DMA = 0x00000100,
	CDDA_MODE_DST_SQ  = 0x00000200,
	CDDA_MODE_DST_PIO = 0x00000400,
	CDDA_MODE_POS_TMU1 = 0x00001000,
	CDDA_MODE_POS_TMU2 = 0x00002000,
	CDDA_MODE_CH_ADAPT = 0x00010000,
	CDDA_MODE_CH_FIXED = 0x00020000
} isoldr_cdda_mode_t;

/**
* Type of dumped image
*/
typedef enum isoldr_image_type {

	IMAGE_TYPE_ISO = ISOFS_IMAGE_TYPE_ISO,
	IMAGE_TYPE_CSO = ISOFS_IMAGE_TYPE_CSO,
	IMAGE_TYPE_ZSO = ISOFS_IMAGE_TYPE_ZSO,
	IMAGE_TYPE_CDI = ISOFS_IMAGE_TYPE_CDI,
	IMAGE_TYPE_GDI = ISOFS_IMAGE_TYPE_GDI,

	IMAGE_TYPE_ROM_NAOMI = 10

} isoldr_image_type_t;


typedef struct isoldr_info {

	char magic[12];                     /* isoldr magic code - 'DSISOLDRXXX' where XXX is version */

	uint32 image_type;                  /* See isoldr_image_type_t */
	char image_file[256];               /* Full path to image */
	char image_second[12];              /* Second data track file for the multitrack GDI image */

	char fs_dev[8];                     /* Device name, see supported devices */
	char fs_type[8];                    /* Extend device name */
	uint32 fs_part;                     /* Partition on device (0-3), only for SD and IDE devices */

	CISO_header_t ciso;                 /* CISO header for CSO/ZSO images */
	cd_toc_t toc;                       /* Table of content */
	uint32 track_offset;                /* Data track offset, for the CDI images only */
	uint32 track_lba[2];                /* Data track LBA, second value for the multitrack GDI image */
	uint32 sector_size;                 /* Data track sector size */

	uint32 boot_mode;                   /* See isoldr_boot_mode_t */
	uint32 emu_cdda;                    /* Emulate CDDA audio. See isoldr_cdda_mode_t */
	uint32 emu_async;                   /* Emulate async data transfer (value is sectors count per frame) */
	uint32 use_dma;                     /* Use DMA data transfer for G1-bus devices (GD drive and IDE) */
	uint32 fast_boot;                   /* Don't show any info on screen */

	isoldr_exec_info_t exec;            /* Executable info */

	uint32 gdtex;                       /* Memory address for GD texture (unused) */
	uint32 patch_addr[2];               /* Memory addresses for patching every frame or interrupt */
	uint32 patch_value[2];              /* Values for patching */
	uint32 heap;                        /* Memory address or mode for heap. See isoldr_heap_mode_t */
	uint32 use_irq;                     /* Use IRQ hooking */
	uint32 emu_vmu;                     /* Emulate VMU on port A1. Set number for VMU dump or zero for disabled. */
	uint32 syscalls;                    /* Memory address for syscalls binary or 1 for auto load. */
	uint32 scr_hotkey;                  /* Creating screenshots by hotkey (zero for disabled). */
	uint32 bleem;                       /* Memory address for Bleem! binary or 1 for auto load. */
	uint32 alt_read;                    /* Use alternative reading without aborting. */
	uint32 use_gpio;                    /* Use GPIO-0 as button for IGR. */
	uint32 firmware;                    /* Memory address for flashrom dump or IRQ table. Set 1 for auto load. */
	uint32 region;                      /* Hardware region. 1 = Japan, 2 = USA, 3 = Europe, 4 = Korea, 5 = Australia */

	uint32 cdda_offset[40];             /* CDDA tracks offset, only for CDI images */

} isoldr_info_t;


/**
 * Get some info from CD image or NAOMI ROM dump and fill info structure
 */
isoldr_info_t *isoldr_get_info(const char *file, int test_mode);

/**
 * Set alternative boot file
 */
int isoldr_set_boot_file(isoldr_info_t *info, const char *iso_file, const char *boot_file);

/**
 * Apply preset file (or default if not specified) to isoldr info
 * and return execution address. Returns -1 on error.
 */
uintptr_t isoldr_apply_preset(isoldr_info_t *info, const char *preset_file);

/**
 * Execute loader for specified device at any valid memory address
 */
void isoldr_exec(isoldr_info_t *info, uintptr_t addr);

#endif /* ifndef _DS_ISOLDR_H_*/
