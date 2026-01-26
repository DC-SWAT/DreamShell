/** 
 * \file      fs.c
 * \brief     Filesystem
 * \date      2013-2025
 * \author    SWAT
 * \copyright	http://www.dc-swat.ru
 */

#include <fatfs.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <fcntl.h>
#include <malloc.h>
#include "ds.h"

typedef struct romdisk_hdr {
    char    magic[8];           /* Should be "-rom1fs-" */
    uint32  full_size;          /* Full size of the file system */
    uint32  checksum;           /* Checksum */
    char    volume_name[16];    /* Volume name (zero-terminated) */
} romdisk_hdr_t;

static uint32 ntohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}

int InitSDCard() {
	return fs_fat_mount_sd();
}

int InitIDE() {
	return fs_fat_mount_ide();
}

int InitRomdisk() {
	
	int cnt = -1;
	uint32 size, addr;
	char path[32];
	uint8 *tmpb = (uint8 *)0x00100000;
	
	dbglog(DBG_INFO, "Checking for romdisk in the bios...\n");
	
	for(addr = 0x00100000; addr < 0x00200000; addr++) {

		if(tmpb[0] == 0x2d && tmpb[1] == 0x72 && tmpb[2] == 0x6f) {
			
			romdisk_hdr_t *romfs = (romdisk_hdr_t *)tmpb;

			if(strncmp(romfs->magic, "-rom1fs-", 8) || strncmp(romfs->volume_name, getenv("HOST"), 10)) {
				continue;
			}

			size = ntohl_32((const void *)&romfs->full_size);

			if(!size || size > 0x1F8000) {
				continue;
			}

			dbglog(DBG_INFO, "Detected romdisk at 0x%08lx, mounting...\n", (uint32)tmpb);

			if(cnt) {
				snprintf(path, sizeof(path), "/brd%d", cnt+1);
			} else {
				strncpy(path, "/brd", sizeof(path));
			}

			if(fs_romdisk_mount(path, (const uint8 *)tmpb, 0) < 0) {
				dbglog(DBG_INFO, "Error mounting romdisk at 0x%08lx\n", (uint32)tmpb);
			} else {
				dbglog(DBG_INFO, "Romdisk mounted as %s\n", path);
			}

			cnt++;
			tmpb += sizeof(romdisk_hdr_t) + size;
		}

		tmpb++;
	}

	return (cnt > -1 ? 0 : -1);
}


int RootDeviceIsSupported(const char *name) {
	if(!strncmp(name, "sd", 2) ||
		!strncmp(name, "ide", 3) ||
		!strncmp(name, "cd", 2) ||
		!strncmp(name, "pc", 2) ||
		!strncmp(name, "brd", 3)) {
		return 1;
	}
	return 0;
}

static int SearchRootCheck(const char *device, const char *path, const char *file) {

	char check[NAME_MAX];

	if(file == NULL) {
		sprintf(check, "/%s%s", device, path);
	} else {
		sprintf(check, "/%s%s/%s", device, path, file);
	}

	if((file == NULL && DirExists(check)) || (file != NULL && FileExists(check))) {
		sprintf(check, "/%s%s", device, path);
		setenv("PATH", check, 1);
		return 1;
	}

	return 0;
}


int SearchRoot() {

	const dirent_t *ent;
	file_t hnd;
	int detected = 0;

	hnd = fs_open("/", O_RDONLY | O_DIR);

	if(hnd < 0) {
		dbglog(DBG_ERROR, "Can't open root directory!\n");
		return -1;
	}

	while ((ent = fs_readdir(hnd)) != NULL) {

		if(!RootDeviceIsSupported(ent->name)) {
			continue;
		}

		dbglog(DBG_INFO, "Checking for root directory on /%s\n", ent->name);

		if(SearchRootCheck(ent->name, "/DS", "/lua/startup.lua") ||
			SearchRootCheck(ent->name, "", "/lua/startup.lua")) {
			detected = 1;
			break;
		}
	}

	fs_close(hnd);

	if (!detected) {
		dbglog(DBG_ERROR, "Can't find root directory.\n");
		setenv("PATH", "/ram", 1);
		setenv("TEMP", "/ram", 1);
		return -1;
	}

	if(strncmp(getenv("PATH"), "/pc", 3) && DirExists("/pc")) {

		dbglog(DBG_INFO, "Checking for root directory on /pc\n");

		if(!SearchRootCheck("pc", "", "/lua/startup.lua")) {
			SearchRootCheck("pc", "/DS", "/lua/startup.lua");
		}
	}

	if(	!strncmp(getenv("PATH"), "/sd", 3) || 
		!strncmp(getenv("PATH"), "/ide", 4) || 
		!strncmp(getenv("PATH"), "/pc", 3)) {
		setenv("TEMP", getenv("PATH"), 1);
	} else {
		setenv("TEMP", "/ram", 1);
	}

	dbglog(DBG_INFO, "Root directory is %s\n", getenv("PATH"));
//	dbglog(DBG_INFO, "Temp directory is %s\n", getenv("TEMP"));
	return 0;
}
