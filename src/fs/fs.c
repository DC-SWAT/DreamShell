/** 
 * \file      fs.c
 * \brief     Filesystem
 * \date      2013-2015
 * \author    SWAT
 * \copyright	http://www.dc-swat.ru
 */

#include "ds.h"
#include "fs.h"
#include "drivers/sd.h"
#include "drivers/g1_ide.h"

typedef struct romdisk_hdr {
    char    magic[8];           /* Should be "-rom1fs-" */
    uint32  full_size;          /* Full size of the file system */
    uint32  checksum;           /* Checksum */
    char    volume_name[16];    /* Volume name (zero-terminated) */
} romdisk_hdr_t;

typedef struct blockdev_devdata {
    uint64_t block_count;
    uint64_t start_block;
} sd_devdata_t;

typedef struct ata_devdata {
    uint64_t block_count;
    uint64_t start_block;
    uint64_t end_block;
} ata_devdata_t;

#define MAX_PARTITIONS 4

static kos_blockdev_t sd_dev[MAX_PARTITIONS];
static kos_blockdev_t g1_dev[MAX_PARTITIONS];

static uint32 ntohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}


static int check_partition(uint8 *buf, int partition) {
	int pval;
	
	if(buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
//		dbglog(DBG_DEBUG, "Device doesn't appear to have a MBR\n");
		return -1;
	}
	
	pval = 16 * partition + 0x01BE;

	if(buf[pval + 4] == 0) {
//		dbglog(DBG_DEBUG, "Partition empty: 0x%02x\n", buf[pval + 4]);
		return -1;
	}
	
	return 0;
}


int InitSDCard() {
	
	dbglog(DBG_INFO, "Checking for SD card...\n");
	uint8 partition_type;
	int part = 0, fat_part = 0;
	char path[8];
	uint32 sd_block_count = 0;
	uint64 sd_capacity = 0;
	uint8 buf[512];
	
	if(sdc_init()) {
		dbglog(DBG_INFO, "SD card not found.\n");
		return -1;
	}
	
	sd_capacity = sdc_get_size();
	sd_block_count = (uint32)(sd_capacity / 512);
	
	dbglog(DBG_INFO, "SD card initialized, capacity %" PRIu32 " MB\n", (uint32)(sd_capacity / 1024 / 1024));
	
//	if(sdc_print_ident()) {
//		dbglog(DBG_INFO, "SD card read CID error\n");
//		return -1;
//	}
	
	if(sdc_read_blocks(0, 1, buf)) {
		dbglog(DBG_ERROR, "Can't read MBR from SD card\n");
		return -1;
	}
	
	for(part = 0; part < MAX_PARTITIONS; part++) {
		
		if(!check_partition(buf, part) && !sdc_blockdev_for_partition(part, &sd_dev[part], &partition_type)) {
			
			if(!part) {
				strcpy(path, "/sd");
				path[3] = '\0';
			} else {
				sprintf(path, "sd%d", part);
				path[strlen(path)] = '\0';
			}
			
			/* Check to see if the MBR says that we have a Linux partition. */
			if(is_ext2_partition(partition_type)) {
				
				dbglog(DBG_INFO, "Detected EXT2 filesystem on partition %d\n", part);
				
				if(fs_ext2_init()) {
					dbglog(DBG_INFO, "Could not initialize fs_ext2!\n");
					sd_dev[part].shutdown(&sd_dev[part]);
				} else {
					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_ext2_mount(path, &sd_dev[part], FS_EXT2_MOUNT_READWRITE)) {
						dbglog(DBG_INFO, "Could not mount device as ext2fs.\n");
						sd_dev[part].shutdown(&sd_dev[part]);
					}
				}
				
			} else if((fat_part = is_fat_partition(partition_type))) {
			
				dbglog(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);
				
				sd_devdata_t *ddata = (sd_devdata_t *)sd_dev[part].dev_data;
//				ddata->block_count += ddata->start_block;
				ddata->block_count = sd_block_count;
				ddata->start_block = 0;
				
				if(fs_fat_init()) {
					dbglog(DBG_INFO, "Could not initialize fs_fat!\n");
					sd_dev[part].shutdown(&sd_dev[part]);
				} else {
				
					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_fat_mount(path, &sd_dev[part], 0, part)) {
						dbglog(DBG_INFO, "Could not mount device as fatfs.\n");
						sd_dev[part].shutdown(&sd_dev[part]);
					}
				}
				
			} else {
				dbglog(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
				sd_dev[part].shutdown(&sd_dev[part]);
			}
			
		} else {
//			dbglog(DBG_ERROR, "Could not make blockdev for partition %d, error: %d\n", part, errno);
		}
	}

	return 0;
}


int InitIDE() {
	
	dbglog(DBG_INFO, "Checking for G1 ATA devices...\n");
	uint8 partition_type;
	int part = 0, fat_part = 0;
	char path[8];
	uint64 ide_block_count = 0;
	uint8 buf[512];
	int use_dma = 1;
	
	if(g1_ata_init()) {
		//dbglog(DBG_INFO, "G1 ATA device not found.\n");
		return -1;
	}
	
//	dbglog(DBG_INFO, "G1 ATA device initialized\n");
	
	/* Read the MBR from the disk */
	if(g1_ata_max_lba() > 0) {
		if(g1_ata_read_lba(0, 1, (uint16_t *)buf) < 0) {
			dbglog(DBG_ERROR, "Can't read MBR from IDE by LBA\n");
			return -1;
		}
	} else {
		
		use_dma = 0;
		
		if(g1_ata_read_chs(0, 0, 1, 1, (uint16_t *)buf) < 0) {
			dbglog(DBG_ERROR, "Can't read MBR from IDE by CHS\n");
			return -1;
		}
	}
	
	for(part = 0; part < MAX_PARTITIONS; part++) {
		
		if(!check_partition(buf, part) && !g1_ata_blockdev_for_partition(part, use_dma, &g1_dev[part], &partition_type)) {
			
			if(!part) {
				strcpy(path, "/ide");
				path[4] = '\0';
			} else {
				sprintf(path, "/ide%d", part);
				path[strlen(path)] = '\0';
			}
			
			/* Check to see if the MBR says that we have a EXT2 or FAT partition. */
			if(is_ext2_partition(partition_type)) {
				
				dbglog(DBG_INFO, "Detected EXT2 filesystem on partition %d\n", part);
				
				if(fs_ext2_init()) {
					dbglog(DBG_INFO, "Could not initialize fs_ext2!\n");
					g1_dev[part].shutdown(&g1_dev[part]);
				} else {
					
					/* Only PIO for EXT2 */
					g1_dev[part].shutdown(&g1_dev[part]);
					if(g1_ata_blockdev_for_partition(part, 0, &g1_dev[part], &partition_type)) {
						continue;
					}
					
					dbglog(DBG_INFO, "Mounting filesystem...\n");
					if(fs_ext2_mount(path, &g1_dev[part], FS_EXT2_MOUNT_READWRITE)) {
						dbglog(DBG_INFO, "Could not mount device as ext2fs.\n");
					}
				}
				
			}  else if((fat_part = is_fat_partition(partition_type))) {
			
				dbglog(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);
				
				if(!ide_block_count) {
					ide_block_count = g1_ata_max_lba();
				}
				
				ata_devdata_t *ddata = (ata_devdata_t *)g1_dev[part].dev_data;
				
				if(ide_block_count > 0) {
					ddata->block_count = ide_block_count;
					ddata->end_block = ide_block_count - 1;
				} else {
					ddata->block_count += ddata->start_block;
					ddata->end_block += ddata->start_block;
				}
				
				ddata->start_block = 0;
				
				if(fs_fat_init()) {
					dbglog(DBG_INFO, "Could not initialize fs_fat!\n");
					g1_dev[part].shutdown(&g1_dev[part]);
				} else {
				
					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_fat_mount(path, &g1_dev[part], use_dma, part)) {
						dbglog(DBG_INFO, "Could not mount device as fatfs.\n");
						g1_dev[part].shutdown(&g1_dev[part]);
					}
				}
				
			} else {
				dbglog(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
				g1_dev[part].shutdown(&g1_dev[part]);
			}
		}
	}

	return 0;
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

			if(strncasecmp(romfs->magic, "-rom1fs-", 8) || strncasecmp(romfs->volume_name, getenv("HOST"), 10)) {
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



static int SearchRootCheck(char *device, char *path, char *file) {
	
	char check[MAX_FN_LEN];
	
	if(file == NULL) {
		sprintf(check, "/%s%s", device, path);
	} else {
		sprintf(check, "/%s%s/%s", device, path, file);
	}
	
	if((file == NULL && DirExists(check)) || (file != NULL && FileExists(check))) {
		sprintf(check, "/%s%s", device, path);
		setenv("PATH", check, 1);
		return 0;
	}
	
	return -1;
}


int SearchRoot(int pass_cnt) {
	
	dirent_t *ent;
	file_t hnd;
	
	hnd = fs_open("/", O_RDONLY | O_DIR);
	
	if(hnd < 0) {
		dbglog(DBG_ERROR, "Can't open root directory!\n");
		return -1;
	}
	
	while ((ent = fs_readdir(hnd)) != NULL) {
		
		if(!strncasecmp(ent->name, "pty", 3) || 
			!strncasecmp(ent->name, "sock", 4) || 
			!strncasecmp(ent->name, "vmu", 3) || 
			!strncasecmp(ent->name, "rd", 2)
			/* || !strncasecmp(ent->name, "sd", 2)*/) {
			continue;
		}
		
		dbglog(DBG_INFO, "Checking for root directory on /%s\n", ent->name);
		
		if(!SearchRootCheck(ent->name, "/DS", "/update/version.dat") || !SearchRootCheck(ent->name, "", "/update/version.dat")) {
			if(!pass_cnt--) goto success;
		}
		
		/*if(!SearchRootCheck(ent->name, "/DS", "/lua/startup.lua") || !SearchRootCheck(ent->name, "", "/lua/startup.lua")) {
			if(!pass_cnt--) goto success;
		} else if(!SearchRootCheck(ent->name, "/DS/apps", NULL) && !SearchRootCheck(ent->name, "/apps", NULL)) {
			if(!pass_cnt--) goto success;
		} else if(!SearchRootCheck(ent->name, "/DS/modules", NULL) && !SearchRootCheck(ent->name, "/modules", NULL)) {
			if(!pass_cnt--) goto success;
		}*/
	}
	
	dbglog(DBG_ERROR, "Can't find root directory.\n");
	setenv("PATH", "/ram", 1);
	setenv("TEMP", "/ram", 1);
	fs_close(hnd);
	return -1;
	
success:
	fs_close(hnd);
	
	if(strncasecmp(getenv("PATH"), "/pc", 3) && DirExists("/pc")) {
		
		dbglog(DBG_INFO, "Checking for root directory on /pc\n");

		if(SearchRootCheck("pc", "", "/lua/startup.lua")) {
			SearchRootCheck("pc", "/DS", "/lua/startup.lua");
		}
	}
	
	if(	!strncasecmp(getenv("PATH"), "/sd", 3) || 
		!strncasecmp(getenv("PATH"), "/ide", 4) || 
		!strncasecmp(getenv("PATH"), "/pc", 3)) {
		setenv("TEMP", getenv("PATH"), 1);
	} else {
		setenv("TEMP", "/ram", 1);
	}
	
	dbglog(DBG_INFO, "Root directory is %s\n", getenv("PATH"));
//	dbglog(DBG_INFO, "Temp directory is %s\n", getenv("TEMP"));
	return 0;
}


#if 0 // TODO

static void *blkdev_open(vfs_handler_t * vfs, const char *fn, int flags) {
	return -1;
}

static int blkdev_close(void *hnd) {
	return -1;
}

static ssize_t blkdev_read(void *hnd, void *buffer, size_t size) {
	return -1;
}

static ssize_t blkdev_write(void * hnd, const void *buffer, size_t cnt) {
	return -1;
}

static off_t blkdev_tell(void * hnd) {
	return 0;
}

static off_t blkdev_seek(void * hnd, off_t offset, int whence) {
	return 0;
}

static size_t blkdev_total(void * hnd) {
	return 0;
}

static int blkdev_complete(void * hnd, ssize_t * rv) {
	return -1; 
}


static vfs_handler_t vh = {
    /* Name Handler */
    {
        { 0 },                  /* name */
        0,                      /* in-kernel */
        0x00010000,             /* Version 1.0 */
        NMMGR_FLAGS_NEEDSFREE,  /* We malloc each VFS struct */
        NMMGR_TYPE_VFS,         /* VFS handler */
        NMMGR_LIST_INIT         /* list */
    },
    0, NULL,               /* no cacheing, privdata */
    blkdev_open,           /* open */
    blkdev_close,          /* close */
    blkdev_read,           /* read */
    blkdev_write,          /* write */
    blkdev_seek,           /* seek */
    blkdev_tell,           /* tell */
    blkdev_total,          /* total */
    NULL,                  /* readdir */
    NULL,                  /* ioctl */
    NULL,                  /* rename */
    NULL,                  /* unlink */
    NULL,                  /* mmap */
    blkdev_complete,       /* complete */
    NULL,                  /* stat */
    NULL,                  /* mkdir */
    NULL,                  /* rmdir */
    NULL,                  /* fcntl */
    NULL,                  /* poll */
    NULL,                  /* link */
    NULL,                  /* symlink */
    NULL,                  /* seek64 */
    NULL,                  /* tell64 */
    NULL,                  /* total64 */
    NULL                   /* readlink */
};

#endif