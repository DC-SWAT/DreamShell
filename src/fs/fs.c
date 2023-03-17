/** 
 * \file      fs.c
 * \brief     Filesystem
 * \date      2013-2023
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
	uint8 buf[512];
	kos_blockdev_t *dev;

	if(sdc_init()) {
		dbglog(DBG_INFO, "SD card not found.\n");
		return -1;
	}

	dbglog(DBG_INFO, "SD card initialized, capacity %" PRIu32 " MB\n",
		(uint32)(sdc_get_size() / 1024 / 1024));

//	if(sdc_print_ident()) {
//		dbglog(DBG_INFO, "SD card read CID error\n");
//		return -1;
//	}

	if(sdc_read_blocks(0, 1, buf)) {
		dbglog(DBG_ERROR, "Can't read MBR from SD card\n");
		return -1;
	}

	for(part = 0; part < MAX_PARTITIONS; part++) {

		dev = &sd_dev[part];

		if(!check_partition(buf, part) && !sdc_blockdev_for_partition(part, dev, &partition_type)) {

			if(!part) {
				strcpy(path, "/sd");
				path[3] = '\0';
			} else {
				sprintf(path, "sd%d", part);
			}

			/* Check to see if the MBR says that we have a Linux partition. */
			if(is_ext2_partition(partition_type)) {

				dbglog(DBG_INFO, "Detected EXT2 filesystem on partition %d\n", part);

				if(fs_ext2_init()) {

					dbglog(DBG_INFO, "Could not initialize fs_ext2!\n");
					sd_dev[part].shutdown(dev);

				} else {

					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_ext2_mount(path, dev, FS_EXT2_MOUNT_READWRITE)) {
						dbglog(DBG_INFO, "Could not mount device as ext2fs.\n");
						sd_dev[part].shutdown(dev);
					}
				}

			} else if((fat_part = is_fat_partition(partition_type))) {

				dbglog(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);

				if(fs_fat_init()) {

					dbglog(DBG_INFO, "Could not initialize fs_fat!\n");
					sd_dev[part].shutdown(dev);

				} else {

					/* Need full disk block device for FAT */
					sd_dev[part].shutdown(dev);
					if(sdc_blockdev_for_device(dev)) {
						continue;
					}

					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_fat_mount(path, dev, 0, part)) {
						dbglog(DBG_INFO, "Could not mount device as fatfs.\n");
						sd_dev[part].shutdown(dev);
					}
				}

			} else {
				dbglog(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
				sd_dev[part].shutdown(dev);
			}
		}
	}
	return 0;
}


int InitIDE() {

	dbglog(DBG_INFO, "Checking for G1 ATA devices...\n");
	uint8 partition_type;
	int part = 0, fat_part = 0;
	char path[8];
	uint8 buf[512];
	 /* FIXME: G1 DMA has conflicts with other stuff like G2 DMA and so on */
	int use_dma = 0;
	kos_blockdev_t *dev;

	if(g1_ata_init()) {
		return -1;
	}

	/* Read the MBR from the disk */
	if(g1_ata_lba_mode()) {
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

		dev = &g1_dev[part];

		if(!check_partition(buf, part) && !g1_ata_blockdev_for_partition(part, use_dma, dev, &partition_type)) {

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
					g1_dev[part].shutdown(dev);

				} else {

					if (use_dma) {
						/* Only PIO for EXT2 */
						g1_dev[part].shutdown(dev);
						if(g1_ata_blockdev_for_partition(part, 0, dev, &partition_type)) {
							continue;
						}
					}

					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_ext2_mount(path, dev, FS_EXT2_MOUNT_READWRITE)) {
						dbglog(DBG_INFO, "Could not mount device as ext2fs.\n");
						g1_dev[part].shutdown(dev);
					}
				}

			}  else if((fat_part = is_fat_partition(partition_type))) {

				dbglog(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);

				if(fs_fat_init()) {

					dbglog(DBG_INFO, "Could not initialize fs_fat!\n");
					g1_dev[part].shutdown(dev);

				} else {

					/* Need full disk block device for FAT */
					g1_dev[part].shutdown(dev);
					if(g1_ata_blockdev_for_device(use_dma, dev)) {
						continue;
					}

					dbglog(DBG_INFO, "Mounting filesystem...\n");

					if(fs_fat_mount(path, dev, use_dma, part)) {
						dbglog(DBG_INFO, "Could not mount device as fatfs.\n");
						g1_dev[part].shutdown(dev);
					}
				}

			} else {
				dbglog(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
				g1_dev[part].shutdown(dev);
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
		return 1;
	}

	return 0;
}


int SearchRoot() {

	dirent_t *ent;
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
