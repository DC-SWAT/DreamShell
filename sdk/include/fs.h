/** 
 * \file    fs.h
 * \brief   DreamShell filesystem
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_FS_H
#define _DS_FS_H

#include <arch/types.h>
#include <kos/blockdev.h>
#include <ext2/fs_ext2.h>

/**
 * Initialize SD Card 
 * and mount all partitions with FAT or EXT2 filesystems
 */
int InitSDCard();

/**
 * Initialize G1-ATA device
 * and mount all partitions with FAT or EXT2 filesystems
 */
int InitIDE();

/**
 * Search romdisk images in BIOS ROM and mount it
 */
int InitRomdisk();

/**
 * Search DreamShell root directory on all usable devices
 * pass_cnt is a number of passed devices
 */
int SearchRoot(int pass_cnt);


/**
 * FAT filesystem API
 */

typedef enum fatfs_ioctl {

	FATFS_IOCTL_CTRL_SYNC = 0,        /* Flush disk cache (for write functions) */
	FATFS_IOCTL_GET_SECTOR_COUNT,     /* Get media size (for only f_mkfs()), 4 byte unsigned */
	FATFS_IOCTL_GET_SECTOR_SIZE,      /* Get sector size (for multiple sector size (_MAX_SS >= 1024)), 2 byte unsigned */
	FATFS_IOCTL_GET_BLOCK_SIZE,       /* Get erase block size (for only f_mkfs()), 2 byte unsigned */
	FATFS_IOCTL_CTRL_ERASE_SECTOR,    /* Force erased a block of sectors (for only _USE_ERASE) */
	FATFS_IOCTL_GET_BOOT_SECTOR_DATA, /* Get first sector data, ffconf.h _MAX_SS bytes */
	FATFS_IOCTL_GET_FD_LBA,           /* Get file LBA, 4 byte unsigned */
	FATFS_IOCTL_GET_FD_LINK_MAP       /* Get file clusters linkmap, 128+ bytes */

} fatfs_ioctl_t;

/**
 * Initialize and shutdown FAT filesystem
 * return 0 on success, or < 0 if error
 */
int fs_fat_init(void);
int fs_fat_shutdown(void);

/**
 * Mount FAT filesystem on specified partition
 * For block device need reset to 0 start_block number
 * return 0 on success, or < 0 if error
 */
int fs_fat_mount(const char *mp, kos_blockdev_t *dev, int dma, int partition);

/**
 * Unmount FAT filesystem
 * return 0 on success, or < 0 if error
 */
int fs_fat_unmount(const char *mp);

/**
 * Check mount point for FAT filesystem
 * return 0 is not FAT
 */
int fs_fat_is_mounted(const char *mp);

/**
 * Check partition type for FAT
 * return 0 if not FAT or 16/32 if FAT partition
 */
int is_fat_partition(uint8 partition_type);

/**
 * Check partition type for EXT2
 */
#define is_ext2_partition(partition_type) (partition_type == 0x83)


#endif /* _DS_FS_H */
