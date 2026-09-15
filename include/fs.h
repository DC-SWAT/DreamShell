/** 
 * \file    fs.h
 * \brief   DreamShell filesystem
 * \date    2007-2025
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_FS_H
#define _DS_FS_H

#include <arch/types.h>

/**
 * Initialize SD Card 
 * and mount all partitions with FAT filesystems
 */
int InitSDCard();

/**
 * Unmount FAT partitions and shutdown SD Card
 */
void ShutdownSDCard();

/**
 * Initialize G1-ATA device
 * and mount all partitions with FAT filesystems
 */
int InitIDE();

/**
 * Unmount FAT partitions and shutdown G1-ATA device
 */
void ShutdownIDE();

/**
 * Search romdisk images in BIOS ROM and mount it
 */
int InitRomdisk();

/**
 * Unmount FAT filesystems and shutdown SD/IDE devices
 */
void ShutdownFS();

/**
 * Get FAT/exFAT filesystem type name for a mount point
 * ("FAT12", "FAT16", "FAT32", "exFAT"), or NULL if not mounted
 */
const char *fs_fat_get_type(const char *mp);

/**
 * Search DreamShell root directory on all usable devices
 */
int SearchRoot();

/**
 * Check for usable devices
 * 1 on success, 0 on fail
 */
int RootDeviceIsSupported(const char *name);


#endif /* _DS_FS_H */
