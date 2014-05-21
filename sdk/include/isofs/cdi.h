/**
 * Copyright (c) 2013-2014 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ISOFS_CDI_H
#define _ISOFS_CDI_H

#include <arch/types.h>

/**
 * \file
 * DiscJuggler CDI support for isofs
 *
 * \author SWAT
 */


#define CDI_V2_ID  0x80000004
#define CDI_V3_ID  0x80000005
#define CDI_V35_ID 0x80000006

/* This limits only for DC games */
#define CDI_MAX_SESSIONS    2
#define CDI_MAX_TRACKS      99


typedef enum CDI_sector_mode {

	CDI_SECTOR_MODE_CDDA  = 0,
	CDI_SECTOR_MODE_DATA  = 1,
	CDI_SECTOR_MODE_MULTI = 2

} CDI_sector_mode_t;


typedef enum cdi_sector_size {

	CDI_SECTOR_SIZE_DATA    = 0, // 2048
	CDI_SECTOR_SIZE_SEMIRAW = 1, // 2336
	CDI_SECTOR_SIZE_CDDA    = 2  // 2352

} CDI_sector_size_t;


typedef struct CDI_trailer {

    uint32 version;
    uint32 header_offset;

} CDI_trailer_t;


typedef struct CDI_track {

    uint32 pregap_length;
    uint32 length;
    uint32 mode;
    uint32 start_lba;
    uint32 total_length;
    uint32 sector_size;
    uint32 offset;

} CDI_track_t;


typedef struct CDI_session {

	uint16 track_count;
	CDI_track_t *tracks[CDI_MAX_TRACKS];

} CDI_session_t;


typedef struct CDI_header {

	CDI_trailer_t trail;
    
	uint16 session_count;
	CDI_session_t *sessions[CDI_MAX_SESSIONS];

} CDI_header_t;


CDI_header_t *cdi_open(file_t fd);
int cdi_close(CDI_header_t *hdr);

uint16 cdi_track_sector_size(CDI_track_t *track);
CDI_track_t *cdi_get_track(CDI_header_t *hdr, uint32 lba);
uint32 cdi_get_offset(CDI_header_t *hdr, uint32 lba, uint16 *sector_size);

int cdi_get_toc(CDI_header_t *hdr, CDROM_TOC *toc);
int cdi_read_sectors(CDI_header_t *hdr, file_t fd, uint8 *buff, uint32 start, uint32 count);


#endif /* _ISOFS_CDI_H */
