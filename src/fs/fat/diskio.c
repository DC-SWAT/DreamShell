/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <kos/blockdev.h>
#include <dc/sd.h>
#include "fs.h"
#include "fatfs/diskio.h"		/* FatFs lower layer API */
#include "drivers/ide.h"
#include "drivers/g1_ata.h"

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

static DSTATUS dev_stat[FS_DEV_COUNT] = {0, 0, 0};


DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{

	switch (pdrv) {
		case FS_DEV_SCIF_SD:

			if(sd_init()) {
				printf("Could not initialize the SD card. Please make sure that you "
				   "have an SD card adapter plugged in and an SD card inserted.\n");
				dev_stat[pdrv] |= STA_NOINIT;
			} else {
				dev_stat[pdrv] &= ~STA_NOINIT;
			}
			
			break;

		case FS_DEV_G1_IDE:
		
			if(g1_ata_init()) {
				dev_stat[pdrv] |= STA_NOINIT;
			} else {
				dev_stat[pdrv] &= ~STA_NOINIT;
			}
			
			break;

		case FS_DEV_G2_IDE:
			
			if(ide_init() < 0) {
				dev_stat[pdrv] |= STA_NOINIT;
			} else {
				dev_stat[pdrv] &= ~STA_NOINIT;
			}
			break;
			
		default:
			return STA_NOINIT;
	}
	
	return dev_stat[pdrv];
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	if(pdrv < 0 || pdrv > FS_DEV_COUNT) {
		return STA_NOINIT;
	}
	
	return dev_stat[pdrv];
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..128) */
)
{

	switch (pdrv) {
		case FS_DEV_SCIF_SD:
			
			if(sd_read_blocks(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;

		case FS_DEV_G1_IDE:
		
			if(g1_ata_read_blocks(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;

		case FS_DEV_G2_IDE:
		
			if(ide_read(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;
			
		default:
			return RES_PARERR;
	}
	
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..128) */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
		case FS_DEV_SCIF_SD:
			
			if(sd_write_blocks(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;

		case FS_DEV_G1_IDE:
			
			if(g1_ata_write_blocks(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;

		case FS_DEV_G2_IDE:
		
			if(ide_write(sector, count, buff) < 0) {
				return RES_ERROR;
			}
			
			return RES_OK;
			
		default:
			return RES_PARERR;
	}
	
	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{

	switch(cmd) {
		
		case CTRL_SYNC:
			return RES_OK;
			
		case GET_SECTOR_COUNT:
		
			switch (pdrv) {
				case FS_DEV_SCIF_SD:
				
					*(ulong*)buff = sd_get_size() / 512;
					return RES_OK;
					
				case FS_DEV_G1_IDE:
				
					*(ulong*)buff = g1_ata_get_size() / 512;
					return RES_OK;

				case FS_DEV_G2_IDE:
					
					*(ulong*)buff = ide_num_sectors();
					return RES_OK;
			
				default:
					return RES_PARERR;
			}
			break;
			
		case GET_SECTOR_SIZE:
			*(ushort*)buff = 512;
			return RES_OK;
		case GET_BLOCK_SIZE:
			*(ushort*)buff = 512;
			return RES_OK;
		case CTRL_ERASE_SECTOR:
			return RES_NOTRDY;
		default:
			return RES_PARERR;
	}
	
	return RES_PARERR;
}
#endif
