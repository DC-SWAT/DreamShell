/**
 * DreamShell ISO Loader 
 * disk I/O for FatFs
 * (c)2009-2017, 2025 SWAT <http://www.dc-swat.ru>
 */
#include <arch/rtc.h>
#include <time.h>
#include <main.h>
#include "diskio.h"

#ifdef DEV_TYPE_SD
#include "../dev/sd/sd.h"
#endif

#ifdef DEV_TYPE_IDE
#include <ide/ide.h>
#endif

#if _FS_READONLY == 0

/* gmtime() implementation */
#define YEAR0           1900                    /* the first year */
#define EPOCH_YR        1970            /* EPOCH = Jan 1 1970 00:00:00 */
#define SECS_DAY        (24L * 60L * 60L)
#define LEAPYEAR(year)  (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)  (LEAPYEAR(year) ? 366 : 365)
#define FIRSTSUNDAY(timp)       (((timp)->tm_yday - (timp)->tm_wday + 420) % 7)
#define FIRSTDAYOF(timp)        (((timp)->tm_wday - (timp)->tm_yday + 420) % 7)
#define TIME_MAX        ULONG_MAX
#define ABB_LEN         3

const int _ytab[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

struct tm *gmtime(const time_t *timer)
{
        static struct tm br_time;
        struct tm *timep = &br_time;
        time_t time = *timer;
        unsigned long dayclock, dayno;
        int year = EPOCH_YR;

        dayclock = (unsigned long)time % SECS_DAY;
        dayno = (unsigned long)time / SECS_DAY;

        timep->tm_sec = dayclock % 60;
        timep->tm_min = (dayclock % 3600) / 60;
        timep->tm_hour = dayclock / 3600;
        timep->tm_wday = (dayno + 4) % 7;       /* day 0 was a thursday */
        while (dayno >= (unsigned long)YEARSIZE(year)) {
                dayno -= YEARSIZE(year);
                year++;
        }
        timep->tm_year = year - YEAR0;
        timep->tm_yday = dayno;
        timep->tm_mon = 0;
        while (dayno >= (unsigned long)_ytab[LEAPYEAR(year)][timep->tm_mon]) {
                dayno -= _ytab[LEAPYEAR(year)][timep->tm_mon];
                timep->tm_mon++;
        }
        timep->tm_mday = dayno + 1;
        timep->tm_isdst = 0;

        return timep;
}

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

DWORD get_fattime ()
{
    struct tm *time;
    time_t unix_time;
    DWORD tmr = 0;

    unix_time = rtc_unix_secs();
    time = gmtime(&unix_time);

    if (time != NULL) {
        tmr = (((DWORD)(time->tm_year - 80)) << 25)   /* tm_year is years since 1900; FAT starts from 1980 */
             | ((DWORD)(time->tm_mon + 1) << 21)      /* tm_mon ranges from 0 to 11; add 1 for FAT */
             | ((DWORD)(time->tm_mday) << 16)         /* tm_mday ranges from 1 to 31 */
             | ((DWORD)(time->tm_hour) << 11)         /* tm_hour ranges from 0 to 23 */
             | ((DWORD)(time->tm_min) << 5)           /* tm_min ranges from 0 to 59 */
             | ((DWORD)(time->tm_sec / 2));           /* tm_sec ranges from 0 to 59; FAT stores seconds in 2-second steps */
    }
    return tmr;
}

#endif /* _FS_READONLY == 0 */

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{

#ifdef DEV_TYPE_SD
	sd_init_params_t params = {
		.interface = (drv == 0 ? SD_IF_SCIF : SD_IF_SCI),
		.check_crc = false
	};
	return sd_init_ex(&params) ? STA_NOINIT : 0;
#endif

#ifdef DEV_TYPE_IDE
	(void)drv;
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
	/* Start async read for SD card */
	return sd_pre_read(sector, count) ? RES_ERROR : RES_OK;
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
