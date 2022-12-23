/**
 * DreamShell ISO Loader 
 * disk I/O for FatFs
 * (c)2009-2017 SWAT <http://www.dc-swat.ru>
 */
#include <main.h>
#include "diskio.h"

#ifdef DEV_TYPE_SD
#include "../dev/sd/sd.h"
#endif

#ifdef DEV_TYPE_IDE
#include <ide/ide.h>
#endif


/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

DWORD get_fattime ()
{
#if 0
    ulong now;
    struct _time_block *tm;
    DWORD tmr;
    
    now = rtc_secs();
    tm = conv_gmtime(now);
    tmr =  (((DWORD)tm->year - 60) << 25)
		| ((DWORD)tm->mon << 21)
		| ((DWORD)tm->day << 16)
		| (WORD)(tm->hour << 11)
		| (WORD)(tm->min << 5)
		| (WORD)(tm->sec >> 1);
    return tmr;
#else
	return 0;
#endif
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{

	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_init() ? STA_NOINIT : 0;
#endif


#ifdef DEV_TYPE_IDE
	return g1_bus_init() ? STA_NOINIT : 0;
#endif

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{

	(void)drv;

#ifdef DEV_TYPE_SD
	return 0;
#endif

#ifdef DEV_TYPE_IDE
	return 0;
#endif

}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	DWORD count		/* Number of sectors to read */
)
{
	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_read_blocks(sector, count, buff, 1) ? RES_ERROR : RES_OK;
#endif

#ifdef DEV_TYPE_IDE
	return g1_ata_read_blocks(sector, count, buff, 1) ? RES_ERROR : RES_OK;
#endif
}


DRESULT disk_read_async (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	DWORD count		/* Number of sectors to read */
)
{
	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_read_blocks(sector, count, buff, 0) ? RES_ERROR : RES_OK;
#endif

#ifdef DEV_TYPE_IDE
	return g1_ata_read_blocks(sector, count, buff, 0) ? RES_ERROR : RES_OK;
#endif
}

#ifdef DEV_TYPE_IDE
DRESULT disk_read_part (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	DWORD bytes		/* Bytes to read */
)
{
	(void)drv;
	return g1_ata_read_lba_dma_part((uint64_t)sector, bytes, buff) ? RES_ERROR : RES_OK;
}
#endif

DRESULT disk_pre_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	DWORD sector,	/* Sector address (LBA) */
	DWORD count		/* Number of sectors to read */
)
{
	(void)drv;

#ifdef DEV_TYPE_SD
	(void)sector;
	(void)count;
	return RES_OK;
#endif

#ifdef DEV_TYPE_IDE
	return g1_ata_pre_read_lba(sector, count) ? RES_ERROR : RES_OK;
#endif
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _FS_READONLY == 0

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	DWORD count			/* Number of sectors to write */
)
{

	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_write_blocks(sector, count, buff, 1) ? RES_ERROR : RES_OK;
#endif

#ifdef DEV_TYPE_IDE
	return g1_ata_write_blocks(sector, count, buff, fs_dma_enabled()) ? RES_ERROR : RES_OK;
#endif

}

#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{

	(void)drv;

	switch (ctrl) {
		
#if _USE_MKFS && !_FS_READONLY

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (ulong) */
		
#ifdef DEV_TYPE_SD
			*(ulong*)buff = (ulong)(sd_get_size() / 512);
#endif

#ifdef DEV_TYPE_IDE
			*(ulong*)buff = (ulong)g1_ata_max_lba();
#endif
			return RES_OK;
#endif /* _USE_MKFS && !_FS_READONLY */

		case GET_SECTOR_SIZE :	/* Get sectors on the disk (ushort) */
		
			*(ushort*)buff = 512;
			return RES_OK;

#if _FS_READONLY == 0
		case CTRL_SYNC :	/* Make sure that data has been written */
#ifdef DEV_TYPE_IDE
			return g1_ata_flush() ? RES_ERROR : RES_OK;
#else
			return RES_OK;
#endif
#endif
		default:
			return RES_PARERR;
	}
}


/*-----------------------------------------------------------------------*/
/* Disk Polling                                                          */

int disk_poll (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{

	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_poll(fs_dma_enabled());
#endif

#ifdef DEV_TYPE_IDE
	return g1_ata_poll();
#endif

}

/*-----------------------------------------------------------------------*/
/* Disk Aborting                                                         */

DRESULT disk_abort (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{

	(void)drv;

#ifdef DEV_TYPE_SD
	return sd_abort();
#endif

#ifdef DEV_TYPE_IDE
	g1_ata_abort();
	return 0;
#endif
}
