/** 
 * \file    internal.h
 * \brief   isofs utils
 * \date    2014-2016, 2025
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _ISOFS_INTERNAL_H
#define _ISOFS_INTERNAL_H

#ifdef __DREAMCAST__
#include <arch/types.h>
#include <isofs/ciso.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

typedef int			file_t;
typedef long int	uint32;
typedef int			int32;
typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef int			mutex_t;

#define FILEHND_INVALID -1
#define MAX_FN_LEN 256

/** \brief  Get the FAD address of a TOC entry.
    \param  n               The actual entry from the TOC to look at.
    \return                 The FAD of the entry.
*/
#define TOC_LBA(n) ((n) & 0x00ffffff)

/** \brief  Get the address of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's address.
*/
#define TOC_ADR(n) ( ((n) & 0x0f000000) >> 24 )

/** \brief  Get the control data of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's control value.
*/
#define TOC_CTRL(n) ( ((n) & 0xf0000000) >> 28 )

/** \brief  Get the track number of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's track.
*/
#define TOC_TRACK(n) ( ((n) & 0x00ff0000) >> 16 )

#define TRACK_FLAG_PREEMPH   0x10 /* Pre-emphasis (audio only) */
#define TRACK_FLAG_COPYPERM  0x20 /* Copy permitted */
#define TRACK_FLAG_DATA      0x40 /* Data track */
#define TRACK_FLAG_FOURCHAN  0x80 /* 4-channel audio */

typedef struct {
    uint32_t  entry[99];          /**< \brief TOC space for 99 tracks */
    uint32_t  first;              /**< \brief Point A0 information (1st track) */
    uint32_t  last;               /**< \brief Point A1 information (last track) */
    uint32_t  leadout_sector;     /**< \brief Point A2 information (leadout) */
} CDROM_TOC;

#define mutex_lock(m)
#define mutex_unlock(m)
#define ds_printf printf
#define fs_seek			lseek
#define fs_write		write
#define fs_read			read
#define fs_close		close
#define memcpy_sh4		memcpy
#define memset_sh4		memset
#define fs_total(fd)	lseek(fd, 0, SEEK_CUR)

char *getFilePath(const char *file);
uint32_t FileSize(const char *fn);
int write_sectors_data(file_t fd, uint32 sector_count, 
						uint16 sector_size, uint8 *buff);
#endif

#define TRACK_FLAG_PREEMPH   0x10 /* Pre-emphasis (audio only) */
#define TRACK_FLAG_COPYPERM  0x20 /* Copy permitted */
#define TRACK_FLAG_DATA      0x40 /* Data track */
#define TRACK_FLAG_FOURCHAN  0x80 /* 4-channel audio */

int read_sectors_data(file_t fd, uint32 sector_count, 
						uint16 sector_size, uint8 *buff);

void spoof_toc_gd_low_density_area(CDROM_TOC *toc);
void spoof_toc_gd_high_density_area(CDROM_TOC *toc);

void spoof_multi_toc_3track_gd(CDROM_TOC *toc);
void spoof_multi_toc_iso(CDROM_TOC *toc, file_t fd, uint32 lba);
#ifdef __DREAMCAST__
void spoof_multi_toc_cso(CDROM_TOC *toc, CISO_header_t *hdr, uint32 lba);
#endif
#endif /* _ISOFS_INTERNAL_H */
