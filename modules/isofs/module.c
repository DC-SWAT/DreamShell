/* DreamShell ##version##

   module.c - isofs module
   Copyright (C)2009-2016 SWAT
*/
            
#include "ds.h"
#include "isofs/isofs.h"
#include "isofs/ciso.h"

DEFAULT_MODULE_HEADER(isofs);

int builtin_isofs_cmd(int argc, char *argv[]) {

    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
					"Options: \n"
					" -m, --mount    -Mounting CD image as filesystem\n"
					" -u, --unmount  -Unmounting CD image\n\n"
					"Arguments: \n"
					" -f, --file     -CD image file\n"
					" -d, --dir      -VFS Directory for access to files of CD image\n\n"
					"Example: %s -m -f /sd/image.iso -d /iso\n\n", argv[0], argv[0]);
        return CMD_NO_ARG;
    }
    
	int mount = 0, unmount = 0;
	char *file, *dir;

	struct cfg_option options[] = {
		{"mount",   'm', NULL, CFG_BOOL, (void *) &mount,   0},
		{"unmount", 'u', NULL, CFG_BOOL, (void *) &unmount, 0},
		{"file",    'f', NULL, CFG_STR,  (void *) &file,    0},
		{"dir",     'd', NULL, CFG_STR,  (void *) &dir,     0},
		CFG_END_OF_LIST
	};
	
	CMD_DEFAULT_ARGS_PARSER(options);
			
	if(dir == NULL) {
		ds_printf("DS_ERROR: Too few arguments (fs directory) \n");
		return CMD_ERROR;
	}
	
	if(mount) {
		
		if(file == NULL || dir == NULL) {
			ds_printf("DS_ERROR: Too few arguments (file and VFS path)\n");
			return CMD_NO_ARG;
		}
		
		if(fs_iso_mount(dir, file) < 0) {
			ds_printf("DS_ERROR: Can't mount %s\n", file);
			return CMD_ERROR;
		}
		
		ds_printf("DS_OK: Mounted %s to %s\n", file, dir);
		
	} else if(unmount) {
		
		if(dir == NULL) {
			ds_printf("DS_ERROR: Too few arguments (VFS path)\n");
			return CMD_NO_ARG;
		}
		
		if(fs_iso_unmount(dir) < 0) {
			ds_printf("DS_ERROR: Can't unmount %s\n", dir);
			return CMD_ERROR;
		}
		
		ds_printf("DS_OK: Unmounted %s\n", dir);
		
	} else {
		ds_printf("DS_ERROR: Too few arguments (mount or unmount)\n");
		return CMD_NO_ARG;
	}
	
	return CMD_OK;
}


int lib_open(klibrary_t *lib) {
	fs_iso_init();
	AddCmd(lib_get_name(), "Mount/unmount CD image file as filesystem", (CmdHandler *) builtin_isofs_cmd); 
	return nmmgr_handler_add(&ds_isofs_hnd.nmmgr);
}


int lib_close(klibrary_t *lib) {
	RemoveCmd( GetCmdByName( lib_get_name() ) );
	fs_iso_shutdown();
	return nmmgr_handler_remove(&ds_isofs_hnd.nmmgr); 
}


int read_sectors_data(file_t fd, uint32 sector_count, 
						uint16 sector_size, uint8 *buff) {
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: %ld at %ld mode %d\n", 
						__func__, sector_count, fs_tell(fd), sector_size);
#endif

	const uint16 sec_size = 2048;
	size_t tmps = sec_size * sector_count;
	
	/* Reading sectors bigger than 2048 */
	if(sector_size > sec_size) {
		
		uint16 b_seek, a_seek;
		uint8 *tmpb;
		
		switch(sector_size) {
			case 2324: /* MODE2_FORM2 */
				b_seek = 16;
				a_seek = 260;
				break;
			case 2336: /* SEMIRAW_MODE2 */
				b_seek = 8;
				a_seek = 280;
				break;
			case 2352: /* RAW_XA */
				b_seek = 16;
				a_seek = 288;
				break;
			default:
				return -1;
		}
		
		while(sector_count > 2) {
			
			tmpb = buff;
			tmps = (tmps / sector_size);
		
			if(fs_read(fd, tmpb, tmps * sector_size) != tmps * sector_size) {
				return -1;
			}

#ifdef DEBUG
			dbglog(DBG_DEBUG, "%s: tmps=%d b_seek=%d a_seek=%d\n", 
								__func__, tmps, b_seek, a_seek);
#endif
			while(tmps--) {
				memmove_sh4(buff, tmpb + b_seek, sec_size);
				tmpb += sector_size;
				buff += sec_size;
				sector_count--;
			}
			
			tmps = sec_size * sector_count;
		}
		
		while(sector_count--) {
			
			fs_seek(fd, b_seek, SEEK_CUR);
			
			if(fs_read(fd, buff, sec_size) != sec_size) {
				return -1;
			}
			
			fs_seek(fd, a_seek, SEEK_CUR);
			buff += sec_size;
		}
		
	/* Reading normal data sectors (2048) */
	} else {
		
		if(fs_read(fd, buff, tmps) != tmps) {
			return -1;
		}
	}
	
	return 0;
}


file_t fs_iso_first_file(const char *mountpoint) {
	
	file_t fd;
	dirent_t *ent;
	char fn[MAX_FN_LEN];

	fd = fs_open(mountpoint, O_DIR | O_RDONLY);
	
	if(fd == FILEHND_INVALID) {
		return fd;
	}
	
	do {
		ent = fs_readdir(fd);
	} while(ent && ent->attr != 0);
	
	if(ent != NULL) {
		snprintf(fn, MAX_FN_LEN, "%s/%s", mountpoint, ent->name);
		fs_close(fd);
		fd = fs_open(fn, O_RDONLY);
	} else {
		fs_close(fd);
		fd = FILEHND_INVALID;
	}
	
	return fd;
}


/**
 * Spoof TOC for GD session 1
 */
void spoof_toc_3track_gd_session_1(CDROM_TOC *toc) {

	/**
	 * Track 1
	 * CTRL = 4, ADR = 1, FAD = 0x0096 (FAD 150, so LBA 0)
	 * MSF = 00:02:00
	 */
	toc->entry[0] = 0x41000096;
	
	/**
	 * Track 2
	 * CTRL = 0, ADR = 1, FAD = 0x02EE (FAD 750, so LBA 600)
	 * MSF = 00:10:00
	 */
	toc->entry[1] = 0x010002EE;
	
	/**
	 * Ununsed / empty entries
	 */
	for(int i = 2; i < 99; i++)
		toc->entry[i] = (uint32)-1;
	
	/**
	 * First track 1 Data
	 */
	toc->first = 0x41010000;
	
	/**
	 * Last track 2 Audio
	 */
	toc->last  = 0x01020000;
	
	/**
	 * Leadout Audio
	 * FAD = 0x1A2C (FAD 6700, so LBA 6850 decimal)
	 * MSF = 01:29:00, 25 frames per second
	 */
	toc->leadout_sector = 0x01001A2C;
}


/**
 * Spoof TOC for GD session 2
 */
void spoof_toc_3track_gd_session_2(CDROM_TOC *toc) {

	/**
	 * Track 1 and 2 is empty.
	 * This is a low-density track, unused in GD session.
	 */
	toc->entry[0] = -1;
	toc->entry[1] = -1;
	
	/**
	 * Track 3 Data
	 * CTRL = 4, ADR = 1, FAD = 0xB05E (FAD 45150, so LBA 45000)
	 * MSF = 10:02:00
	 */
	toc->entry[2] = 0x4100B05E;
	
	/**
	 * Ununsed / empty entries
	 */
	for(int i = 3; i < 99; i++) {
		toc->entry[i] = (uint32)-1;
	}
	
	/**
	 * Track 3 Data
	 */
	toc->first = 0x41030000;
	toc->last  = 0x41030000;
	
	/**
	 * Data
	 * FAD = 0x0861B4 (FAD 549300, so LBA 549150 decimal)
	 * MSF = 122:04:00
	 */
	toc->leadout_sector = 0x410861B4;
}


void spoof_multi_toc_3track_gd(CDROM_TOC *toc) {

	spoof_toc_3track_gd_session_2(toc);
	
	toc->entry[0] = 0x41000096;
	toc->entry[1] = 0x010002EE;
	
	/* Need change before use */
//	toc->first = 0x41010000;
//	toc->last  = 0x01020000;
//	toc->leadout_sector = 0x01001A2C;
}


void spoof_multi_toc_iso(CDROM_TOC *toc, file_t fd, uint32 lba) {
	/**
	 * Track 1 Data
	 * CTRL = 4, ADR = 1, LBA = ???
	 */
	toc->entry[0] = 0x41000000 | lba;
	
	/**
	 * Ununsed / empty entries
	 */
	for(int i = 1; i < 99; i++)
		toc->entry[i] = (uint32)-1;
	
	/**
	 * First and last is track 1
	 */
	toc->first = 0x41010000;
	toc->last  = 0x01010000;
	
	/**
	 * Leadout sector
	 */
	toc->leadout_sector = (fs_total(fd) / 2048) + lba;
}


void spoof_multi_toc_cso(CDROM_TOC *toc, CISO_header_t *hdr, uint32 lba) {
	/**
	 * Track 1 Data
	 * CTRL = 4, ADR = 1, LBA = ???
	 */
	toc->entry[0] = 0x41000000 | lba;
	
	/**
	 * Ununsed / empty entries
	 */
	for(int i = 1; i < 99; i++)
		toc->entry[i] = (uint32)-1;
	
	/**
	 * First and last is track 1
	 */
	toc->first = 0x41010000;
	toc->last  = 0x01010000;
	
	/**
	 * Leadout sector
	 */
	toc->leadout_sector = (hdr->total_bytes / hdr->block_size) + lba;
}
