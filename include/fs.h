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
 * Initialize G1-ATA device
 * and mount all partitions with FAT filesystems
 */
int InitIDE();

/**
 * Search romdisk images in BIOS ROM and mount it
 */
int InitRomdisk();

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
