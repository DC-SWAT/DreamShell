/**
 * Copyright (c) 2014 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ISOFS_GDI_H
#define _ISOFS_GDI_H

#include <kos.h>

/**
 * \file
 * nullDC GDI support for isofs
 *
 * \author SWAT
 */

#define GDI_MAX_TRACKS 99


typedef struct GDI_track {
	
	uint32 start_lba;
	uint32 flags;
	uint32 sector_size;
	uint32 offset;
	char filename[MAX_FN_LEN];
	
} GDI_track_t;


typedef struct GDI_header {

	file_t track_fd;
	uint16 track_current;
	
	uint16 track_count;
	GDI_track_t *tracks[GDI_MAX_TRACKS];

} GDI_header_t;


GDI_header_t *gdi_open(file_t fd, const char *path);
int gdi_close(GDI_header_t *hdr);

#define gdi_track_sector_size(track) track->sector_size
GDI_track_t *gdi_get_track(GDI_header_t *hdr, uint32 lba);
uint32 gdi_get_offset(GDI_header_t *hdr, uint32 lba, uint16 *sector_size);

int gdi_get_toc(GDI_header_t *hdr, CDROM_TOC *toc);
int gdi_read_sectors(GDI_header_t *hdr, uint8 *buff, uint32 start, uint32 count);


#endif /* _ISOFS_GDI_H */
