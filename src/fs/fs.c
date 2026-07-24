/** 
 * \file      fs.c
 * \brief     Filesystem
 * \date      2013-2026
 * \author    SWAT
 * \copyright	http://www.dc-swat.ru
 */

#include <fatfs.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <fcntl.h>
#include <malloc.h>
#include <drivers/hollysh.h>
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

void ShutdownFS() {
	fs_fat_shutdown();
}

int InitRomdisk() {

	uint32 size;
	uint8 *tmpb = (uint8 *)HOLLYSH_BIOS_ROMDISK_ROM_OFFSET;
	romdisk_hdr_t *romfs;

	if(!hollysh_bios_detect()) {
		return -1;
	}

	dbglog(DBG_INFO, "Checking for romdisk in the HollySH BIOS...\n");

	if(tmpb + sizeof(romdisk_hdr_t) >= (uint8 *)HOLLYSH_BIOS_ROM_FONT_OFFSET) {
		return -1;
	}

	romfs = (romdisk_hdr_t *)tmpb;

	if(strncmp(romfs->magic, "-rom1fs-", 8) || strncmp(romfs->volume_name, getenv("HOST"), 10)) {
		return -1;
	}

	size = ntohl_32((const void *)&romfs->full_size);

	if(!size || (uint32)tmpb + sizeof(romdisk_hdr_t) + size > HOLLYSH_BIOS_ROM_FONT_OFFSET) {
		return -1;
	}

	dbglog(DBG_INFO, "Detected romdisk at 0x%08lx, mounting...\n", (uint32)tmpb);

	if(fs_romdisk_mount("/brd", (const uint8 *)tmpb, 0) < 0) {
		dbglog(DBG_INFO, "Error mounting romdisk at 0x%08lx\n", (uint32)tmpb);
		return -1;
	}

	dbglog(DBG_INFO, "Romdisk mounted as /brd\n");
	return 0;
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
