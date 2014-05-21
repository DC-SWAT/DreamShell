/** 
 * \file    isofs.h
 * \brief   ISO9660 filesystem for using with CD images
 * \date    2011-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _ISOFS_H
#define _ISOFS_H

#include <arch/types.h>

typedef enum isofs_ioctl {

	ISOFS_IOCTL_RESET = 0,                    /* No data */
	
	ISOFS_IOCTL_GET_FD_LBA = 1,               /* 4 byte unsigned */
	
	ISOFS_IOCTL_GET_DATA_TRACK_FILENAME,      /* 256 bytes */
	ISOFS_IOCTL_GET_DATA_TRACK_FILENAME2,     /* 12  bytes */
	ISOFS_IOCTL_GET_DATA_TRACK_LBA,           /* 4 byte unsigned */
	ISOFS_IOCTL_GET_DATA_TRACK_LBA2,          /* 4 byte unsigned */
	ISOFS_IOCTL_GET_DATA_TRACK_OFFSET,        /* 4 byte unsigned */
	ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE,   /* 4 byte unsigned */
	
	ISOFS_IOCTL_GET_IMAGE_TYPE,               /* 4 byte unsigned */
	ISOFS_IOCTL_GET_IMAGE_HEADER_PTR,         /* 4 byte unsigned  (pointer to memory) */
	
	ISOFS_IOCTL_GET_TOC_DATA,                 /* CDROM_TOC */
	ISOFS_IOCTL_GET_BOOT_SECTOR_DATA          /* 2048 bytes */

} isofs_ioctl_t;


typedef enum isofs_image_type {

	ISOFS_IMAGE_TYPE_ISO = 0,
	ISOFS_IMAGE_TYPE_CSO = 1,
	ISOFS_IMAGE_TYPE_ZSO,
	ISOFS_IMAGE_TYPE_CDI,
	ISOFS_IMAGE_TYPE_GDI

} isofs_image_type_t;


int fs_iso_init();
int fs_iso_shutdown();

int fs_iso_mount(const char *mountpoint, const char *filename);
int fs_iso_unmount(const char *mountpoint);


#endif /* _ISOFS_H */
