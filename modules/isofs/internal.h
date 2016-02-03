/** 
 * \file    internal.h
 * \brief   isofs utils
 * \date    2014-2016
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _ISOFS_INTERNAL_H
#define _ISOFS_INTERNAL_H

#include <arch/types.h>
#include <isofs/ciso.h>

#define TRACK_FLAG_PREEMPH   0x10 /* Pre-emphasis (audio only) */
#define TRACK_FLAG_COPYPERM  0x20 /* Copy permitted */
#define TRACK_FLAG_DATA      0x40 /* Data track */
#define TRACK_FLAG_FOURCHAN  0x80 /* 4-channel audio */


int read_sectors_data(file_t fd, uint32 sector_count, 
						uint16 sector_size, uint8 *buff);


/**
 * Spoof TOC for GD session 1
 */
void spoof_toc_3track_gd_session_1(CDROM_TOC *toc);

/**
 * Spoof TOC for GD session 2
 */
void spoof_toc_3track_gd_session_2(CDROM_TOC *toc);

void spoof_multi_toc_3track_gd(CDROM_TOC *toc);
void spoof_multi_toc_iso(CDROM_TOC *toc, file_t fd, uint32 lba);
void spoof_multi_toc_cso(CDROM_TOC *toc, CISO_header_t *hdr, uint32 lba);


#endif /* _ISOFS_INTERNAL_H */
