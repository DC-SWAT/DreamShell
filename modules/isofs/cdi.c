/**
 * Copyright (c) 2013-2014 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */
#ifdef __DREAMCAST__
#include <kos.h>
#include "console.h"
#endif
#include "internal.h"
#include "isofs/cdi.h"

//#define DEBUG 1

static const uint8 TRACK_START_MARKER[20] = { 
    0x00,0x00,0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF
};
// static const uint8 EXT_MARKER[9] = {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

struct CDI_track_data {

    uint32 pregap_length;
    uint32 length;
    char unknown2[6];
    uint32 mode;
    char unknown3[0x0c];
    uint32 start_lba;
    uint32 total_length;
    char unknown4[0x10];
    uint32 sector_size;
    char unknown5[0x1D];

} __attribute__((packed));


static int check_cdi_image(file_t fd, CDI_trailer_t *trail) {
    
    uint32 len;
    
    len = fs_seek(fd, -8, SEEK_END) + 8;
    fs_read(fd, trail, sizeof(CDI_trailer_t));

    if(trail->header_offset >= len || trail->header_offset == 0) {
#ifdef DEBUG
        dbglog(DBG_DEBUG, "%s: Invalid CDI image: %ld >= %ld, version: %08lx\n", 
                    __func__, trail->header_offset, len, trail->version);
#endif
        return -1;
    }

    if(trail->version != CDI_V2_ID && 
        trail->version != CDI_V3_ID &&
        trail->version != CDI_V35_ID) {
#ifdef DEBUG
        dbglog(DBG_DEBUG, "%s: Invalid CDI image version: %08lx\n", __func__, trail->version);
#endif
        return -1;
    }

#ifdef DEBUG
    dbglog(DBG_DEBUG, "%s: CDI image version %08lx\n", __func__, trail->version);
#endif
    return 0;
}



CDI_header_t *cdi_open(file_t fd) {

    CDI_trailer_t trail;

    if(check_cdi_image(fd, &trail) < 0) {
        return NULL;
    }
    
    int i, j;
    uint16 total_tracks = 0;
    uint32 posn = 0;
    uint32 sector_size = 0, offset = 0;
    uint8 marker[20];
    CDI_header_t *hdr;
    
    hdr = (CDI_header_t*)malloc(sizeof(CDI_header_t));
    memset_sh4(hdr, 0, sizeof(CDI_header_t));
    memcpy_sh4(&hdr->trail, &trail, sizeof(CDI_trailer_t));
    
#ifdef DEBUG
    dbglog(DBG_DEBUG, "%s: Seek to header at %ld\n", __func__, 
        (hdr->trail.version == CDI_V35_ID ? 
        fs_total(fd) - hdr->trail.header_offset : hdr->trail.header_offset)
    );
#endif

    if(hdr->trail.version == CDI_V35_ID) {
        fs_seek(fd, -(off_t)hdr->trail.header_offset, SEEK_END);
    } else {
        fs_seek(fd, (off_t)hdr->trail.header_offset, SEEK_SET);
    }

    fs_read(fd, &hdr->session_count, sizeof(hdr->session_count));
    
    if(!hdr->session_count || hdr->session_count > CDI_MAX_SESSIONS) {
        ds_printf("DS_ERROR: Bad sessions count in CDI image: %d\n", hdr->session_count);
        free(hdr);
        return NULL;
    }
    
#ifdef DEBUG
    dbglog(DBG_DEBUG, "%s: Sessions count: %d\n", __func__, hdr->session_count);
#endif
    
    for(i = 0; i < hdr->session_count; i++) {
        
        hdr->sessions[i] = (CDI_session_t *)malloc(sizeof(CDI_session_t));
        
        if(hdr->sessions[i] == NULL) {
            goto error;
        }

        fs_read(fd, &hdr->sessions[i]->track_count, sizeof(hdr->sessions[i]->track_count));
        
#ifdef DEBUG
        dbglog(DBG_DEBUG, "%s: Session: %d, tracks count: %d\n", __func__, i + 1, hdr->sessions[i]->track_count);
#endif

        if((i != hdr->session_count-1 && hdr->sessions[i]->track_count < 1) 
            || hdr->sessions[i]->track_count > 99 ) {
            ds_printf("DS_ERROR: Invalid number of tracks (%d), bad cdi image\n", hdr->sessions[i]->track_count);
            goto error;
        }

        if(hdr->sessions[i]->track_count + total_tracks > 99) {
            ds_printf("DS_ERROR: Invalid number of tracks in disc, bad cdi image\n");
            goto error;
        }

        for(j = 0; j < hdr->sessions[i]->track_count; j++) {
            
            hdr->sessions[i]->tracks[j] = (CDI_track_t *)malloc(sizeof(CDI_track_t));

            if(hdr->sessions[i]->tracks[j] == NULL) {
                goto error;
            }

            uint32 new_fmt = 0;
            uint8 fnamelen = 0;

            fs_read(fd, &new_fmt, sizeof(new_fmt));

            if(new_fmt != 0) { /* Additional data 3.00.780+ ?? */
                fs_seek(fd, 8, SEEK_CUR); /* Skip */
            }

            fs_read(fd, marker, 20);

            if(memcmp(marker, TRACK_START_MARKER, 20) != 0) {
                ds_printf("DS_ERROR: Track start marker not found, error reading cdi image\n");
                goto error;
            }

            fs_seek(fd, 4, SEEK_CUR);
            fs_read(fd, &fnamelen, 1);
            fs_seek(fd, (off_t)fnamelen, SEEK_CUR); /* skip over the filename */
            fs_seek(fd, 19, SEEK_CUR);
            fs_read(fd, &new_fmt, sizeof(new_fmt));

            if(new_fmt == 0x80000000) {
                fs_seek(fd, 10, SEEK_CUR);
            } else {
                fs_seek(fd, 2, SEEK_CUR);
            }

            struct CDI_track_data trk;
            memset(&trk, 0, sizeof(trk));
            fs_read(fd, &trk, sizeof(trk));

            hdr->sessions[i]->tracks[j]->length = trk.length;
            hdr->sessions[i]->tracks[j]->mode = trk.mode;
            hdr->sessions[i]->tracks[j]->pregap_length = trk.pregap_length;
            hdr->sessions[i]->tracks[j]->sector_size = trk.sector_size;
            hdr->sessions[i]->tracks[j]->start_lba = trk.start_lba;
            hdr->sessions[i]->tracks[j]->total_length = trk.total_length;

            if(hdr->trail.version != CDI_V2_ID) {

                uint32 extmarker = 0;
                fs_seek(fd, 5, SEEK_CUR);
                fs_read(fd, &extmarker, sizeof(extmarker));

                if(extmarker == 0xFFFFFFFF)  {
                    fs_seek(fd, 78, SEEK_CUR);
                }
            }

            sector_size = cdi_track_sector_size(hdr->sessions[i]->tracks[j]);
            offset = posn + hdr->sessions[i]->tracks[j]->pregap_length * sector_size;
            
#ifdef DEBUG
            dbglog(DBG_DEBUG, "%s: Session: %d, track: %d, lba: %ld, length: %ld, mode: %ld, secsize: %ld, offset: %ld\n", 
                        __func__, i + 1, j + 1, 
                        hdr->sessions[i]->tracks[j]->start_lba, 
                        hdr->sessions[i]->tracks[j]->length, 
                        hdr->sessions[i]->tracks[j]->mode,
                        hdr->sessions[i]->tracks[j]->sector_size,
                        offset);
#endif
            hdr->sessions[i]->tracks[j]->offset = offset;
            posn += hdr->sessions[i]->tracks[j]->total_length * sector_size;

            total_tracks++;
        }

        fs_seek(fd, 12, SEEK_CUR);

        if(hdr->trail.version != CDI_V2_ID) {
            fs_seek(fd, 1, SEEK_CUR);
        }
    }

    return hdr;

error:
    cdi_close(hdr);
    return NULL;
}


int cdi_close(CDI_header_t *hdr) {

    if(!hdr) {
        return -1;
    }

    int i, j;
    
    for(i = 0; i < hdr->session_count; i++) {
        
        if(hdr->sessions[i] != NULL && hdr->sessions[i]->track_count > 0 
			&& hdr->sessions[i]->track_count < CDI_MAX_TRACKS) {
				
            for(j = 0; j < hdr->sessions[i]->track_count; j++) {
                
                if(hdr->sessions[i]->tracks[j] != NULL) {
                    free(hdr->sessions[i]->tracks[j]);
                }
            }
            
            free(hdr->sessions[i]);
        }
    }

    free(hdr);
    return 0;
}


uint16 cdi_track_sector_size(CDI_track_t *track) {
	
	switch(track->mode) {
		case 0:
			return (track->sector_size == 2 ? 2352 : 0);
		case 1:
			return (track->sector_size == 0 ? 2048 : 0);
		case 2:
			switch(track->sector_size) {
				case 0:
					return 2048;
				case 1:
					return 2336;
				case 2:
				default:
					return 0;
			}
		default:
			break;
	}
	
	return 0;
}


int cdi_get_toc(CDI_header_t *hdr, CDROM_TOC *toc) {

    int i, j = 0;
    uint16 total_tracks = 0;
    uint8 ctrl = 0, adr = 1;

    for(i = 0; i < hdr->session_count; i++) {
        for(j = 0; j < hdr->sessions[i]->track_count; j++) {
            ctrl = (hdr->sessions[i]->tracks[j]->mode == 0 ? 0 : 4);
            toc->entry[total_tracks] = ctrl << 28 | adr << 24 | (hdr->sessions[i]->tracks[j]->start_lba + 150);
#ifdef DEBUG
            dbglog(DBG_DEBUG, "%s: Track%d %08lx\n", __func__, (int)total_tracks, toc->entry[total_tracks]);
#endif
            total_tracks++;
        }
    }

    CDI_session_t *last_session = hdr->sessions[ hdr->session_count - 1 ];
    CDI_track_t *last_track = last_session->tracks[ last_session->track_count - 1 ];
    CDI_track_t *first_track = hdr->sessions[0]->tracks[0];

    ctrl = (first_track->mode == 0 ? 0 : 4);
    toc->first = ctrl << 28 | adr << 24 | 1 << 16;

    ctrl = (last_track->mode == 0 ? 0 : 4);
    toc->last  = ctrl << 28 | adr << 24 | total_tracks << 16;

    toc->leadout_sector = ctrl << 28 | adr << 24 | (last_track->start_lba + last_track->length + 150);

    for(i = total_tracks; i < 99; i++) {
        toc->entry[i] = -1;
    }

#ifdef DEBUG
    dbglog(DBG_DEBUG, "%s:\n First track %08lx\n Last track %08lx\n Leadout    %08lx\n", 
        __func__, toc->first, toc->last, toc->leadout_sector);
#endif

    return 0;
}


CDI_track_t *cdi_get_track(CDI_header_t *hdr, uint32 lba) {

    int i, j;

    for(i = hdr->session_count - 1; i > -1; i--) {
        for(j = hdr->sessions[i]->track_count - 1; j > -1; j--) {
            if(hdr->sessions[i]->tracks[j]->start_lba <= lba) {
                return hdr->sessions[i]->tracks[j];
            }
        }
    }

    return NULL;
}


uint32 cdi_get_offset(CDI_header_t *hdr, uint32 lba, uint16 *sector_size) {
    CDI_track_t *track = cdi_get_track(hdr, lba);

    if(track == NULL) {
        return -1;
    }

    uint32 offset = lba - track->start_lba;
    *sector_size = cdi_track_sector_size(track);

    if(!offset) {
        offset = track->offset;
    } else {
        offset *= *sector_size;
        offset += track->offset;
    }
	
    return offset;
}


int cdi_read_sectors(CDI_header_t *hdr, file_t fd, uint8 *buff, uint32 start, uint32 count) {

	uint16 sector_size;
	uint32 offset = cdi_get_offset(hdr, start, &sector_size);
	
	if(offset == (uint32)-1) {
		return -1;
	}
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: %ld %ld at %ld mode %d\n", __func__, start, count, offset, sector_size);
#endif

	fs_seek(fd, offset, SEEK_SET);
	
#ifndef __DREAMCAST__
	CDI_track_t *track = cdi_get_track(hdr, start);
	if (track->mode != 2) {
		uint32 bytes = count * sector_size;
		if(fs_read(fd, buff, bytes) != bytes) {
			printf("%s: error read %d\n", __func__, start);
			return -1;
		}
		
		return 0;
	}
#endif
	
	return read_sectors_data(fd, count, sector_size, buff);
}

#ifndef __DREAMCAST__
int cdi_write_sectors(CDI_header_t *hdr, int fd, uint8 *buff, uint32 start, uint32 count) {

	uint16_t sector_size;
	uint32_t offset = cdi_get_offset(hdr, start, &sector_size);
	
	if(offset == (uint32_t)-1) {
		return -1;
	}
	
#ifdef DEBUG
	printf("%s: %d %d at %d mode %d\n", __func__, start, count, offset, sector_size);
#endif

	lseek(fd, offset, SEEK_SET);	
	return write_sectors_data(fd, count, sector_size, buff);
}
#endif

