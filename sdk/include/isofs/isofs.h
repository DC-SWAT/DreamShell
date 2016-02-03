/** 
 * \file    isofs.h
 * \brief   ISO9660 filesystem for optical drive images
 * \date    2011-2016
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _ISOFS_H
#define _ISOFS_H

#include <arch/types.h>
#include <kos/blockdev.h>

/**
 * fs_ioctl commands
 */
typedef enum isofs_ioctl {

	ISOFS_IOCTL_RESET = 0,                    /* No data */
	
	ISOFS_IOCTL_GET_FD_LBA,                   /* 4 byte unsigned */
	
	ISOFS_IOCTL_GET_DATA_TRACK_FILENAME,      /* 256 bytes */
	ISOFS_IOCTL_GET_DATA_TRACK_FILENAME2,     /* 12  bytes          (second data track of GDI) */
	ISOFS_IOCTL_GET_DATA_TRACK_LBA,           /* 4 byte unsigned */
	ISOFS_IOCTL_GET_DATA_TRACK_LBA2,          /* 4 byte unsigned    (second data track of GDI) */
	ISOFS_IOCTL_GET_DATA_TRACK_OFFSET,        /* 4 byte unsigned */
	ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE,   /* 4 byte unsigned */
	
	ISOFS_IOCTL_GET_IMAGE_TYPE,               /* 4 byte unsigned */
	ISOFS_IOCTL_GET_IMAGE_HEADER_PTR,         /* 4 byte unsigned    (pointer to memory) */
	
	ISOFS_IOCTL_GET_TOC_DATA,                 /* CDROM_TOC */
	ISOFS_IOCTL_GET_BOOT_SECTOR_DATA,         /* 2048 bytes */
	ISOFS_IOCTL_GET_CDDA_OFFSET,              /* 97*4 byte unsigned (CDDA tracks offset of CDI) */
	
	ISOFS_IOCTL_GET_TRACK_SECTOR_COUNT,       /* 4 byte unsigned */
	ISOFS_IOCTL_GET_IMAGE_FD                  /* 4 byte unsigned */

} isofs_ioctl_t;

/**
 * ISO image type
 */
typedef enum isofs_image_type {

	ISOFS_IMAGE_TYPE_ISO = 0,
	ISOFS_IMAGE_TYPE_CSO,
	ISOFS_IMAGE_TYPE_ZSO,
	ISOFS_IMAGE_TYPE_CDI,
	ISOFS_IMAGE_TYPE_GDI

} isofs_image_type_t;

/** 
 * IP.BIN (boot sector) meta info
 */
typedef struct ipbin_meta {
	char hardware_ID[16];
	char maker_ID[16];
	char device_info[16];
	char country_codes[8];
	char ctrl[4];
	char dev[1];
	char VGA[1];
	char WinCE[1];
	char unk[1];
	char product_ID[10];
	char product_version[6];
	char release_date[16];
	char boot_file[16];
	char software_maker_info[16];
	char title[32];
} ipbin_meta_t;


int fs_iso_init();
int fs_iso_shutdown();

int fs_iso_mount(const char *mountpoint, const char *filename);
int fs_iso_unmount(const char *mountpoint);

file_t fs_iso_first_file(const char *mountpoint);

int fs_iso_map2dev(const char *filename, kos_blockdev_t *dev);

#endif /* _ISOFS_H */
