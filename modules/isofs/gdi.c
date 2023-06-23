/**
 * Copyright (c) 2014-2015 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */

#include <kos.h>
#include "console.h"
#include "utils.h"
#include "isofs/gdi.h"
#include "internal.h"

//#define DEBUG 1

static char *fs_gets(file_t fd, char *buffer, int count) {
	
	uint8 ch;
	char *cp;
	int rc;

	cp = buffer;
	
	while(count-- > 1) {
		
		rc = fs_read(fd, &ch, 1);
		
		if(rc < 0) {
			*cp = 0;
			return NULL;
		} else if(!rc) {
			break;
		}
		
		*cp++ = ch;
		
		if (ch == '\n')
			break;
	}
	
	*cp = 0;
	return buffer;
}

static int check_gdi_image(file_t fd) {
    
    char line[NAME_MAX];
    uint32 track_count;

    fs_seek(fd, 0, SEEK_SET);
	
    if(fs_gets(fd, line, NAME_MAX) == NULL) {
#ifdef DEBUG
        dbglog(DBG_DEBUG, "%s: Not a GDI image\n", __func__);
#endif
        return -1;
    }
	
    track_count = strtoul(line, NULL, 0);
	
    if(track_count == 0 || track_count > 99) {
#ifdef DEBUG
        dbglog(DBG_DEBUG, "%s: Invalid GDI image\n", __func__);
#endif
        return -1;
    }

    return (int)track_count;
}



GDI_header_t *gdi_open(file_t fd, const char *filename) {

	int track_count;

	if((track_count = check_gdi_image(fd)) < 0) {
		return NULL;
	}
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: %d tracks in image\n", __func__, track_count);
#endif

	int track_no, i, rc;
	char line[NAME_MAX], fname[NAME_MAX / 2];
	char *path = NULL;
	GDI_header_t *hdr;
	
	hdr = (GDI_header_t*)malloc(sizeof(GDI_header_t));
	
	if(hdr == NULL) {
		return NULL;
	}
	
	memset_sh4(hdr, 0, sizeof(GDI_header_t));
	hdr->track_count = (uint32)track_count;
	hdr->track_fd = FILEHND_INVALID;
	hdr->track_current = GDI_MAX_TRACKS;
	path = getFilePath(filename);

	for(i = 0; i < hdr->track_count; i++) {

		if(fs_gets(fd, line, sizeof(line)) == NULL) {
#ifdef DEBUG
			dbglog(DBG_DEBUG, "%s: Unexpected end of file\n", __func__);
#endif
			goto error;
		}

		hdr->tracks[i] = (GDI_track_t*)malloc(sizeof(GDI_track_t));
		
		if(hdr->tracks[i] == NULL) {
			goto error;
		}
		
#ifdef DEBUG
		dbglog(DBG_DEBUG, "%s: %s", __func__, line);
#endif
		
		rc = sscanf(line, "%d %ld %ld %ld %s %ld", 
					&track_no, 
					&hdr->tracks[i]->start_lba, 
					&hdr->tracks[i]->flags, 
					&hdr->tracks[i]->sector_size,
					fname, 
					&hdr->tracks[i]->offset);
					
		if(rc < 6) {
#ifdef DEBUG
			dbglog(DBG_DEBUG, "%s: Invalid line in GDI: %s\n", __func__, line);
#endif
			goto error;
		}
		
		snprintf(hdr->tracks[i]->filename, NAME_MAX, "%s/%s", path, fname);
	}

	free(path);
	return hdr;

error:
	if(path) {
		free(path);
	}
	gdi_close(hdr);
	return NULL;
}


int gdi_close(GDI_header_t *hdr) {

	if(!hdr) {
		return -1;
	}

	int i;

	for(i = 0; i < hdr->track_count; i++) {
		
		if(hdr->tracks[i] != NULL) {
			free(hdr->tracks[i]);
		}
	}
	
	if(hdr->track_fd != FILEHND_INVALID) {
		fs_close(hdr->track_fd);
	}

	free(hdr);
	return 0;
}


int gdi_get_toc(GDI_header_t *hdr, CDROM_TOC *toc) {

	int i, ft_no = 0;
	uint8 ctrl = 0, adr = 1;
	GDI_track_t *first_track = hdr->tracks[0];
	
	for(i = 0; i < hdr->track_count; i++) {
		
		toc->entry[i] = ((hdr->tracks[i]->flags & 0x0F) << 28) | adr << 24 | (hdr->tracks[i]->start_lba + 150);
		
		if(hdr->tracks[i]->start_lba == 45000) {
			first_track = hdr->tracks[i];
			ft_no = i+1;
		}
#ifdef DEBUG
		dbglog(DBG_DEBUG, "%s: Track%d %08lx\n", __func__, i, toc->entry[i]);
#endif
	}
	
	for(i = hdr->track_count - 1; i > -1; i--) {
		if(hdr->tracks[i]->start_lba == 45000) {
			first_track = hdr->tracks[i];
			break;
		}
	}
	
	GDI_track_t *last_track = hdr->tracks[hdr->track_count - 1];
	uint32 track_size = FileSize(last_track->filename) / last_track->sector_size;

	ctrl = first_track->flags & 0x0F;
	toc->first = ctrl << 28 | adr << 24 | ft_no << 16;
	ctrl = last_track->flags & 0x0F;
	toc->last = ctrl << 28 | adr << 24 | hdr->track_count << 16;
	toc->leadout_sector = ctrl << 28 | adr << 24 | (last_track->start_lba + track_size + 150);

	for(i = hdr->track_count; i < 99; i++) {
		toc->entry[i] = -1;
	}
	
#ifdef DEBUG
    dbglog(DBG_DEBUG, "%s:\n First track %08lx\n Last track %08lx\n Leadout    %08lx\n", 
        __func__, toc->first, toc->last, toc->leadout_sector);
#endif

    return 0;
}


GDI_track_t *gdi_get_track(GDI_header_t *hdr, uint32 lba) {

	int i;

	for(i = hdr->track_count - 1; i > -1; i--) {
		
		if(hdr->tracks[i]->start_lba <= lba) {
			
#ifdef DEBUG
				dbglog(DBG_DEBUG, "%s: %ld found in track #%d\n", __func__, lba, i + 1);
#endif
			
			if(hdr->track_current != i) {
				
#ifdef DEBUG
				dbglog(DBG_DEBUG, "%s: Opening %s\n", __func__, hdr->tracks[i]->filename);
#endif
				
				if(hdr->track_fd != FILEHND_INVALID) {
					fs_close(hdr->track_fd);
				}

				hdr->track_fd = fs_open(hdr->tracks[i]->filename, O_RDONLY);
				
				if(hdr->track_fd < 0) {
#ifdef DEBUG
					dbglog(DBG_DEBUG, "%s: Can't open %s\n", __func__, hdr->tracks[i]->filename);
#endif
					return NULL;
				}
				
				hdr->track_current = i;
			}
			
			return hdr->tracks[i];
		}
	}

	return NULL;
}


uint32 gdi_get_offset(GDI_header_t *hdr, uint32 lba, uint16 *sector_size) {
	
    GDI_track_t *track = gdi_get_track(hdr, lba);

    if(track == NULL) {
        return -1;
    }

    uint32 offset = (lba - track->start_lba) * track->sector_size;
    *sector_size = gdi_track_sector_size(track);
	
    return offset;
}


int gdi_read_sectors(GDI_header_t *hdr, uint8 *buff, uint32 start, uint32 count) {

	uint16 sector_size;
	uint32 offset = gdi_get_offset(hdr, start, &sector_size);
	
	if(offset == (uint32)-1) {
		return -1;
	}
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: %ld %ld at %ld mode %d\n", __func__, start, count, offset, sector_size);
#endif

	fs_seek(hdr->track_fd, offset, SEEK_SET);
	return read_sectors_data(hdr->track_fd, count, sector_size, buff);
}
