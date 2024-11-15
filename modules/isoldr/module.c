/* DreamShell ##version##

   DreamShell ISO Loader module
   Copyright (C)2009-2024 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "fs.h"

static uint8 kos_hdr[8] = {0x2D, 0xD0, 0x02, 0x01, 0x12, 0x20, 0x2B, 0xD0};
static uint8 kos_hdr_2[8] = {0x38, 0xD0, 0x02, 0x01, 0x12, 0x20, 0x36, 0xD0};
static uint8 kos_hdr_3[8] = {0x37, 0xD0, 0x02, 0x01, 0x12, 0x20, 0x35, 0xD0};
static uint8 ron_hdr[8] = {0x1B, 0xD0, 0x1A, 0xD1, 0x1B, 0x20, 0x2B, 0x40};
static uint8 win_hdr[4] = {0x45, 0x43, 0x45, 0x43};

void isoldr_exec_at(const void *image, uint32 length, uint32 address, uint32 params_len);
void isoldr_vm2_bank_switch(const char *ipbin_info_sec);

DEFAULT_MODULE_EXPORTS_CMD(isoldr, "Runs the games images");


static void get_ipbin_info(isoldr_info_t *info, file_t fd, uint8 *sec, char *psec) {

	uint32 len;

	if(fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, sec) < 0) {

		ds_printf("DS_ERROR: Can't get boot sector data\n");

	} else {

//		kos_md5(sec, sizeof(sec), info->md5);

		if(sec[0x60] != 0 && sec[0x60] != 0x20) {

			strncpy(info->exec.file, psec + 0x60, sizeof(info->exec.file));

			for(len = 0; len < sizeof(info->exec.file); len++) {
				if(info->exec.file[len] == 0x20) {
					info->exec.file[len] = '\0';
					break;
				}
			}

		} else {
			info->exec.file[0] = 0;
		}

		isoldr_vm2_bank_switch(psec);
	}
}


static int is_homebrew(file_t fd) {

	uint8 src[sizeof(kos_hdr)];

	fs_seek(fd, 0, SEEK_SET);
	fs_read(fd, src, sizeof(kos_hdr));
	fs_seek(fd, 0, SEEK_SET);

	/* Check for unscrambled homebrew */
	if(!memcmp(src, kos_hdr, sizeof(kos_hdr))
		|| !memcmp(src, kos_hdr_2, sizeof(kos_hdr_2))
		|| !memcmp(src, kos_hdr_3, sizeof(kos_hdr_3))
		|| !memcmp(src, ron_hdr, sizeof(ron_hdr))
	) {
		return 1;
	}

	/* TODO: Check for scrambled homebrew */
	return 0;
}

static int is_wince_rom(file_t fd) {

	uint8 src[sizeof(win_hdr)];

	fs_seek(fd, 64, SEEK_SET);
	fs_read(fd, src, sizeof(win_hdr));
	fs_seek(fd, 0, SEEK_SET);

	return !memcmp(src, win_hdr, sizeof(win_hdr));
}

static int get_executable_info(isoldr_info_t *info, file_t fd) {

	if(!strncasecmp(info->exec.file, "0WINCEOS.BIN", 12)) {

		info->exec.type = BIN_TYPE_WINCE;
		info->exec.lba++;
		info->exec.size -= 2048;

	} else if(is_wince_rom(fd)) {

		info->exec.type = BIN_TYPE_WINCE;

	} else if(is_homebrew(fd)) {

		info->exec.type = BIN_TYPE_KOS;

	} else {
		// By default is KATANA
		// FIXME: detect KATANA and set scrambled homebrew by default
		info->exec.type = BIN_TYPE_KATANA;
	}

	return 0;
}


static int get_image_info(isoldr_info_t *info, const char *iso_file, int use_gdtex) {

	file_t fd;
	char fn[NAME_MAX];
	char mount[8] = "/isoldr";
	uint8 sec[2048];
	char *psec = (char *)sec;
	mount[7] = '\0';
	int len = 0;
	SDL_Surface *gd_tex = NULL;

	fd = fs_open(mount, O_DIR | O_RDONLY);

	if(fd != FILEHND_INVALID) {
		fs_close(fd);
		if(fs_iso_unmount(mount) < 0) {
			ds_printf("DS_ERROR: Can't unmount %s\n", mount);
			return -1;
		}
	}

	if(fs_iso_mount(mount, iso_file) < 0) {
		ds_printf("DS_ERROR: Can't mount %s to %s\n", iso_file, mount);
		return -1;
	}

	if(use_gdtex) {

		snprintf(fn, NAME_MAX, "%s/0GDTEX.PVR", mount);
		fd = fs_open(fn, O_RDONLY);

		if(fd != FILEHND_INVALID) {

			get_ipbin_info(info, fd, sec, psec);
			fs_close(fd);

			gd_tex = IMG_Load(fn);

			if(gd_tex != NULL) {
				info->gdtex = (uint32)gd_tex->pixels;
			}

		} else {

			fd = fs_iso_first_file(mount);

			if(fd != FILEHND_INVALID) {
				get_ipbin_info(info, fd, sec, psec);
				fs_close(fd);
			}
		}

	} else {

		fd = fs_iso_first_file(mount);

		if(fd != FILEHND_INVALID) {
			get_ipbin_info(info, fd, sec, psec);
			fs_close(fd);
		}
	}

	memset_sh4(&sec, 0, sizeof(sec));

	if(info->exec.file[0] == 0) {
		strncpy(info->exec.file, "1ST_READ.BIN", 12);
		info->exec.file[12] = '\0';
	}

	snprintf(fn, NAME_MAX, "%s/%s", mount, info->exec.file);
	fd = fs_open(fn, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s\n", fn);
		goto image_error;
	}

	/* TODO check errors */
	fs_ioctl(fd, ISOFS_IOCTL_GET_FD_LBA, &info->exec.lba);
	fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, &info->image_type);
	fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_LBA, &info->track_lba[0]);
	fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE, &info->sector_size);
	fs_ioctl(fd, ISOFS_IOCTL_GET_TOC_DATA, &info->toc);

	if(info->image_type == ISOFS_IMAGE_TYPE_CDI) {

		uint32 *offset = (uint32 *)sec;
		fs_ioctl(fd, ISOFS_IOCTL_GET_CDDA_OFFSET, offset);
		memcpy_sh4(&info->cdda_offset, offset, sizeof(info->cdda_offset));
		memset_sh4(&sec, 0, sizeof(sec));

		fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_OFFSET, &info->track_offset);
	}

	if(info->image_type == ISOFS_IMAGE_TYPE_CSO ||
	        info->image_type == ISOFS_IMAGE_TYPE_ZSO) {

		uint32 ptr = 0;

		if(!fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_HEADER_PTR, &ptr) && ptr != 0) {
			memcpy_sh4(&info->ciso, (void*)ptr, sizeof(CISO_header_t));
		}
	}

	if(info->image_type == ISOFS_IMAGE_TYPE_GDI) {

		fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_FILENAME, sec);
		fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_FILENAME2, info->image_second);
		fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_LBA2, &info->track_lba[1]);

		if(info->track_lba[1] == 50150 &&
			!strcasecmp(info->image_second, "track04.bin")) {
			info->bleem = 1;
		}

		psec = strchr(psec + 1, '/');

	} else {
		psec = strchr(iso_file + 1, '/');
	}

	if(psec == NULL) {
		goto image_error;
	}

	len = strlen(psec);

	if(len > NAME_MAX) {
		len = NAME_MAX - 1;
	}

	strncpy(info->image_file, psec, len);
	info->image_file[len] = '\0';

	info->exec.lba += 150;
	info->exec.size = fs_total(fd);

	if(get_executable_info(info, fd) < 0) {
		ds_printf("DS_ERROR: Can't get executable info\n");
		goto image_error;
	}

	fs_close(fd);
	fs_iso_unmount(mount);
	return 0;

image_error:

	if(fd != FILEHND_INVALID) {
		fs_close(fd);
	}
	fs_iso_unmount(mount);
	if(gd_tex) SDL_FreeSurface(gd_tex);
	return -1;
}


static int get_device_info(isoldr_info_t *info, const char *iso_file) {

	if(!strncasecmp(iso_file, "/pc/", 4)) {

		strncpy(info->fs_dev, ISOLDR_DEV_DCLOAD, 3);
		info->fs_dev[3] = '\0';

	} else if(!strncasecmp(iso_file, "/cd/", 4)) {

		strncpy(info->fs_dev, ISOLDR_DEV_GDROM, 2);
		info->fs_dev[2] = '\0';

	} else if(!strncasecmp(iso_file, "/sd", 3)) {

		strncpy(info->fs_dev, ISOLDR_DEV_SDCARD, 2);
		info->fs_dev[2] = '\0';

		if(iso_file[3] != '/') {
			info->fs_part = (iso_file[3] - '0');
		}

	} else if(!strncasecmp(iso_file, "/ide", 4)) {

		strncpy(info->fs_dev, ISOLDR_DEV_G1ATA, 3);
		info->fs_dev[3] = '\0';

		if(iso_file[4] != '/') {
			info->fs_part = (iso_file[4] - '0');
		}

	} else {
		ds_printf("DS_ERROR: isoldr doesn't support this device\n");
		return -1;
	}

	info->fs_type[0] = '\0';
	return 0;
}


isoldr_info_t *isoldr_get_info(const char *file, int use_gdtex) {

	isoldr_info_t *info = NULL;

	if(!FileExists(file)) {
		goto error;
	}

	info = (isoldr_info_t *) malloc(sizeof(*info));

	if(info == NULL) {
		ds_printf("DS_ERROR: No free memory\n");
		goto error;
	}

	memset_sh4(info, 0, sizeof(*info));

	info->track_lba[0] = 150;
	info->track_lba[1] = info->track_lba[0];
	info->sector_size = 2048;

	if(get_image_info(info, file, use_gdtex) < 0) {
		goto error;
	}

	if(get_device_info(info, file) < 0) {
		goto error;
	}

	// Keep interface version 0.6.x up to 0.8.x loaders
	if (VER_MAJOR == 0 && VER_MINOR <= 8 && VER_MINOR >= 6) {
		snprintf(info->magic, 12, "DSISOLDR%d%d%d", VER_MAJOR, 6, VER_MICRO);
	} else {
		snprintf(info->magic, 12, "DSISOLDR%d%d%d", VER_MAJOR, VER_MINOR, VER_MICRO);
	}
	info->magic[11] = '\0';
	info->exec.addr = 0xac010000;

	return info;

error:

	if(info)
		free(info);

	return NULL;
}


int isoldr_set_boot_file(isoldr_info_t *info, const char *iso_file, const char *boot_file) {

	file_t fd;
	char fn[NAME_MAX];
	char *mount = "/isoldr";

	if(fs_iso_mount(mount, iso_file) < 0) {
		ds_printf("DS_ERROR: Can't mount %s to %s\n", iso_file, mount);
		return -1;
	}

	snprintf(fn, NAME_MAX, "%s/%s", mount, boot_file);
	fd = fs_open(fn, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		return -1;
	}

	fs_ioctl(fd, ISOFS_IOCTL_GET_FD_LBA, &info->exec.lba);

	info->exec.lba += 150;
	info->exec.size = fs_total(fd);
	strncpy(info->exec.file, boot_file, 12);
	info->exec.file[12] = '\0';

	fs_close(fd);
	fs_iso_unmount(mount);
	return 0;
}


static int patch_loader_addr(uint8 *loader, uint32 size, uint32 addr) {

	uint32 i = 0, a = 0;
	int skip = 0;

	EXPT_GUARD_BEGIN;

	for(i = 3; i < size - 1; i++) {

		if(loader[i] == 0xE0 && loader[i + 1] == 0x8C/* && loader[i - 1] < 0x10*/) {
			memcpy(&a, loader + i - 2, sizeof(uint32));
//				printf("0x%08lx -> ", a);
			a -= ISOLDR_DEFAULT_ADDR;

			if(a == 0 && skip++) {
//					printf("skip\n");
				continue;
			}

//				printf("0x%04lx -> ", a);
			a += addr;
//				printf("0x%08lx at offset %ld\n", a, i);
			memcpy(loader + i - 2, &a, sizeof(uint32));
		}
	}

	EXPT_GUARD_CATCH;

	ds_printf("DS_ERROR: Loader memory patch failed\n");
	EXPT_GUARD_RETURN -1;

	EXPT_GUARD_END;

	return 0;
}

static void set_loader_type(isoldr_info_t *info) {
	if (info->syscalls != 0 || info->scr_hotkey != 0 || info->bleem != 0) {
		strncpy(info->fs_type, ISOLDR_TYPE_FULL, 4);
		info->fs_type[4] = '\0';
	} else if ((info->emu_cdda != CDDA_MODE_DISABLED || info->use_irq != 0) && info->emu_vmu == 0) {
		strncpy(info->fs_type, ISOLDR_TYPE_CDDA, 4);
		info->fs_type[4] = '\0';
	} else if (info->emu_vmu != 0 && info->emu_cdda == CDDA_MODE_DISABLED) {
		strncpy(info->fs_type, ISOLDR_TYPE_VMU, 3);
		info->fs_type[3] = '\0';
	} else if (info->emu_vmu != 0 && info->emu_cdda != CDDA_MODE_DISABLED) {
		strncpy(info->fs_type, ISOLDR_TYPE_FEAT, 4);
		info->fs_type[4] = '\0';
	} else {
		info->fs_type[0] = '\0';
	}
}

void isoldr_exec(isoldr_info_t *info, uint32 addr) {

	char fn[NAME_MAX];

	if (strcmp(info->fs_dev, ISOLDR_DEV_DCLOAD) == 0
		|| strcmp(info->fs_dev, ISOLDR_DEV_GDROM) == 0
		|| strcmp(info->fs_dev, ISOLDR_DEV_SDCARD) == 0
		|| strcmp(info->fs_dev, ISOLDR_DEV_G1ATA) == 0
	) {
		set_loader_type(info);
	}

	if(info->fs_type[0] != '\0') {
		snprintf(fn, NAME_MAX, "%s/firmware/%s/%s_%s.bin", getenv("PATH"), lib_get_name(), info->fs_dev, info->fs_type);
	} else {
		snprintf(fn, NAME_MAX, "%s/firmware/%s/%s.bin", getenv("PATH"), lib_get_name(), info->fs_dev);
	}

	file_t fd = fs_open(fn, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open file: %s\n", fn);
		return;
	}

	size_t sc_len = 0;
	size_t len = fs_total(fd) + ISOLDR_PARAMS_SIZE;
	size_t buf_size = len < 0x20000 ? 0x25000 : len + 0x5000;

	ds_printf("DS_PROCESS: Loading %s %d bytes...\n", fn, len - ISOLDR_PARAMS_SIZE);
	uint8 *loader = (uint8 *) memalign(32, buf_size);

	if(loader == NULL) {
		fs_close(fd);
		ds_printf("DS_ERROR: No free memory, needed %d bytes\n", len);
		return;
	}

	memset_sh4(loader, 0, buf_size);

	if(fs_read(fd, loader + ISOLDR_PARAMS_SIZE, len) != (len - ISOLDR_PARAMS_SIZE)) {
		fs_close(fd);
		ds_printf("DS_ERROR: Can't load %s\n", fn);
		return;
	}

	fs_close(fd);

	if (info->syscalls == 1) {

		snprintf(fn, NAME_MAX, "%s/firmware/%s/syscalls.bin", getenv("PATH"), lib_get_name());
		fd = fs_open(fn, O_RDONLY);

		if (fd < 0) {
			info->syscalls = 0;
		} else {
			sc_len = fs_total(fd);
			ds_printf("DS_PROCESS: Loading %s %d bytes...\n", fn, sc_len);
			uint8 *buff = loader + len + 0x1000; // Keep some for a loader heap

			if (fs_read(fd, buff, sc_len) != sc_len) {
				ds_printf("DS_ERROR: Can't load %s\n", fn);
				info->syscalls = 0;
			} else {
				addr = ISOLDR_DEFAULT_ADDR;

				info->syscalls = (uint32)buff;
				info->heap = HEAP_MODE_BEHIND;
				info->emu_cdda = 0;
				info->emu_vmu = 0;
				info->use_irq = 0;

				if (sc_len < 0x4000) {
					sc_len = 0x5000;
				} else {
					sc_len += 0x1000;
				}
			}
			fs_close(fd);
		}
	}

	if (info->bleem == 1) {

		snprintf(fn, NAME_MAX, "%s/firmware/emu/bleem.bin", getenv("PATH"));
		fd = fs_open(fn, O_RDONLY);

		if (fd < 0) {
			info->bleem = 0;
		} else {
			size_t blen = fs_total(fd);
			uint8 *buff = memalign(32, blen);

			if(buff == NULL) {
				fs_close(fd);
				ds_printf("DS_ERROR: No free memory, needed %d bytes\n", blen);
				return;
			}
			ds_printf("DS_PROCESS: Loading %s %d bytes...\n", fn, blen);

			if (fs_read(fd, buff, blen) != blen) {
				ds_printf("DS_ERROR: Can't load %s\n", fn);
				info->bleem = 0;
			} else {
				dcache_flush_range((uint32)buff, blen);
				addr = ISOLDR_DEFAULT_ADDR_HIGH - 8000;
				info->bleem = (uint32)buff;
				info->heap = HEAP_MODE_BEHIND;
			}
			fs_close(fd);
		}
	}

	if(addr != ISOLDR_DEFAULT_ADDR) {
		if(patch_loader_addr(loader + ISOLDR_PARAMS_SIZE, len - ISOLDR_PARAMS_SIZE, addr)) {
			free(loader);
			return;
		}
	}

	memcpy_sh4(loader, info, sizeof(*info));

	// free(info);
	ds_printf("DS_PROCESS: Executing at 0x%08lx...\n", addr);
	ShutdownDS();

	isoldr_exec_at(loader, len + sc_len, addr, ISOLDR_PARAMS_SIZE);
}


int builtin_isoldr_cmd(int argc, char *argv[]) {

	if(argc < 2) {
		ds_printf("\n  ## ISO Loader v%d.%d.%d build %d ##\n\n"
		          "Usage: %s options args\n"
		          "Options: \n", VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD, argv[0]);
		ds_printf(" -s, --fast       -Don't show loader text and disc texture on screen\n",
		          " -i, --verbose    -Show additional info\n",
		          " -a, --dma        -Use DMA transfer if available\n"
		          " -q, --irq        -Use IRQ hooking\n"
		          " -c, --cdda       -Emulate CDDA audio (cddamode=1 by default)\n"
				  " -l, --low        -Use low-level syscalls emulation (disabled by default).\n");
		ds_printf("Arguments: \n"
		          " -e, --async      -Emulate async reading, 0=none default, >0=sectors per frame\n"
		          " -d, --device     -Loader device (sd/ide/cd), default auto\n"
		          " -p, --fspart     -Device partition (0-3), default auto\n"
		          " -t, --fstype     -Device filesystem (fat, ext2, raw), default auto\n");
		ds_printf(" -x, --lmem       -Any valid address for the loader (default auto)\n"
		          " -f, --file       -ISO image file path\n"
		          " -j, --jmp        -Boot mode:\n"
		          "                      0 = from executable (default)\n"
		          "                      1 = from IP.BIN\n"
		          "                      2 = from truncated IP.BIN\n");
		ds_printf(" -o, --os         -Executable OS:\n"
		          "                      0 = auto (default)\n"
		          "                      1 = KallistiOS\n"
		          "                      2 = KATANA\n"
		          "                      3 = WINCE\n");
		ds_printf(" -r, --addr       -Executable memory address (default 0xac010000)\n"
		          " -b, --boot       -Executable file name (default from IP.BIN)\n");
		ds_printf(" -h, --heap       -Heap mode or memory address\n"
		          "                      0 = auto address selection (default)\n"
		          "                      1 = behind the loader\n"
		          "                      2 = ingame memory allocation (KATANA only)\n"
		          "                      3 = maple DMA buffer (keep some for devices)\n"
		          "             0x8cXXXXXX = address (specify valid address)\n");
		ds_printf(" -g, --cddamode   -CDDA emulation mode\n"
		          "                      0 = Disabled (default)\n"
		          "                      1 = DMA/DMA/TMU2\n"
		          "                      2 = DMA/DMA/TMU1\n"
		          "                      3 = SQ/PIO/TMU2\n"
		          "                      4 = SQ/PIO/TMU1\n"
		          "             0x000CPDS%d = Ch[1-2],Pos[1-2],Dst[1-2-4],Src[1-2],%d\n",
				  CDDA_MODE_EXTENDED, CDDA_MODE_EXTENDED);
		ds_printf(" -v, --vmu        -Emulate VMU on port A1.\n"
		          "                      0 = disabled (default)\n"
		          "                      1 = number for VMU dump /vmu/vmudump001.vmd\n"
		          "                    999 = max number\n");
		ds_printf(" -k, --scrhot     -Screenshots from video frame buffer\n"
		          "                      0 = disabled (default)\n"
		          "                   XXXX = bit mask for pad buttons to me pressed\n");
		ds_printf("     --pa1        -Patch address 1\n"
		          "     --pa2        -Patch address 2\n"
		          "     --pv1        -Patch value 1\n"
		          "     --pv2        -Patch value 2\n\n"
		          "Example: %s -f /sd/game.iso\n\n", argv[0]);
		return CMD_NO_ARG;
	}

	uint32 p_addr[2]  = {0, 0};
	uint32 p_value[2] = {0, 0};
	uint32 addr = 0, use_dma = 0, lex = 0, heap = HEAP_MODE_AUTO;
	char *file = NULL, *bin_file = NULL, *device = NULL, *fstype = NULL;
	uint32 emu_async = 0, emu_cdda = 0, boot_mode = BOOT_MODE_DIRECT;
	uint32 bin_type = BIN_TYPE_AUTO, fast_boot = 0, verbose = 0;
	uint32 cdda_mode = CDDA_MODE_DISABLED, use_irq = 0, emu_vmu = 0;
	uint32 low_level = 0, scr_hotkey = 0, bleem = 0, alt_read = 0;
	int fspart = -1;
	isoldr_info_t *info;

	struct cfg_option options[] = {
		{"verbose",   'i', NULL, CFG_BOOL,  (void *) &verbose,     0},
		{"dma",       'a', NULL, CFG_BOOL,  (void *) &use_dma,     0},
		{"device",    'd', NULL, CFG_STR,   (void *) &device,      0},
		{"fspart",    'p', NULL, CFG_INT,   (void *) &fspart,      0},
		{"fstype",    't', NULL, CFG_STR,   (void *) &fstype,      0},
		{"memory",    'x', NULL, CFG_ULONG, (void *) &lex,         0},
		{"addr",      'r', NULL, CFG_ULONG, (void *) &addr,        0},
		{"file",      'f', NULL, CFG_STR,   (void *) &file,        0},
		{"async",     'e', NULL, CFG_ULONG, (void *) &emu_async,   0},
		{"cdda",      'c', NULL, CFG_BOOL,  (void *) &emu_cdda,    0},
		{"cddamode",  'g', NULL, CFG_ULONG, (void *) &cdda_mode,   0},
		{"heap",      'h', NULL, CFG_ULONG, (void *) &heap,        0},
		{"jmp",       'j', NULL, CFG_ULONG, (void *) &boot_mode,   0},
		{"os",        'o', NULL, CFG_ULONG, (void *) &bin_type,    0},
		{"boot",      'b', NULL, CFG_STR,   (void *) &bin_file,    0},
		{"fast",      's', NULL, CFG_BOOL,  (void *) &fast_boot,   0},
		{"irq",       'q', NULL, CFG_BOOL,  (void *) &use_irq,     0},
		{"vmu",       'v', NULL, CFG_ULONG, (void *) &emu_vmu,     0},
		{"low",       'l', NULL, CFG_BOOL,  (void *) &low_level,   0},
		{"scrhotkey", 'k', NULL, CFG_ULONG, (void *) &scr_hotkey,  0},
		{"bleem",     'u', NULL, CFG_ULONG, (void *) &bleem,       0},
		{"altread",   'y', NULL, CFG_BOOL,  (void *) &alt_read,    0},
		{"pa1",      '\0', NULL, CFG_ULONG, (void *) &p_addr[0],   0},
		{"pa2",      '\0', NULL, CFG_ULONG, (void *) &p_addr[1],   0},
		{"pv1",      '\0', NULL, CFG_ULONG, (void *) &p_value[0],  0},
		{"pv2",      '\0', NULL, CFG_ULONG, (void *) &p_value[1],  0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(file == NULL) {
		ds_printf("DS_ERROR: Too few arguments (ISO file) \n");
		return CMD_ERROR;
	}

	if(!lex) {
		if(boot_mode != BOOT_MODE_DIRECT) {
			lex = ISOLDR_DEFAULT_ADDR_HIGH;
		} else {
			lex = ISOLDR_DEFAULT_ADDR_MIN;
		}
	}

	info = isoldr_get_info(file, fast_boot);

	if(info == NULL) {
		return CMD_ERROR;
	}

	if(device != NULL && strncasecmp(device, "auto", 4)) {

		strcpy(info->fs_dev, device);
		info->fs_dev[strlen(info->fs_dev)] = '\0';

	} else {
		if(!strncasecmp(file, "/pc/", 4) && lex < 0x8c010000) {
			lex = ISOLDR_DEFAULT_ADDR_HIGH;
			ds_printf("DS_WARNING: Using dc-load as file system, forced loader address: 0x%08lx\n", lex);
		}
	}

	if(fstype != NULL && strncasecmp(fstype, "auto", 4)) {
		strcpy(info->fs_type, fstype);
		info->fs_type[strlen(info->fs_type)] = '\0';
	}

	if(fspart > -1 && fspart < 4) {
		info->fs_part = fspart;
	}

	if(bin_file != NULL) {
		isoldr_set_boot_file(info, file, bin_file);
	}

	if(bin_type) {
		info->exec.type = bin_type;
	}

	if(addr) {
		info->exec.addr = addr;
	}

	info->boot_mode = boot_mode;
	info->emu_async = emu_async;
	info->use_dma = use_dma;
	info->fast_boot = fast_boot;
	info->heap = heap;
	info->use_irq = use_irq;
	info->emu_vmu = emu_vmu;
	info->syscalls = low_level;
	info->scr_hotkey = scr_hotkey;
	info->bleem = bleem;
	info->alt_read = alt_read;

	if (cdda_mode > CDDA_MODE_DISABLED) {
		info->emu_cdda  = cdda_mode;
	} else if(emu_cdda) {
		info->emu_cdda  = (CDDA_MODE_EXTENDED | CDDA_MODE_SRC_DMA |
			CDDA_MODE_DST_DMA | CDDA_MODE_POS_TMU2 | CDDA_MODE_CH_FIXED);
	} else {
		info->emu_cdda  = CDDA_MODE_DISABLED;
	}

	info->patch_addr[0]  = p_addr[0];
	info->patch_addr[1]  = p_addr[1];
	info->patch_value[0] = p_value[0];
	info->patch_value[1] = p_value[1];

	if(verbose) {

		ds_printf("Params size: %d\n", sizeof(isoldr_info_t));

		ds_printf("\n--- Executable info ---\n"
		          "Name: %s\n"
		          "OS: %d\n"
		          "Size: %d Kb\n"
		          "LBA: %d\n"
		          "Address: 0x%08lx\n"
		          "Boot mode: %d\n",
		          info->exec.file,
		          info->exec.type,
		          info->exec.size/1024,
		          info->exec.lba,
		          info->exec.addr,
		          info->boot_mode);

		ds_printf("--- ISO info ---\n"
		          "File: %s (%s)\n"
		          "Format: %d\n"
		          "LBA: %d (%d)\n"
		          "Sector size: %d\n",
		          info->image_file,
		          info->image_second,
		          info->image_type,
		          info->track_lba[0],
		          info->track_lba[1],
		          info->sector_size);

		ds_printf("--- Loader info ---\n"
		          "Device: %s\n"
		          "Type: %s\n"
				  "Partition: %d\n"
		          "Address: 0x%08lx\n"
		          "DMA: %d\n"
		          "IRQ: %d\n"
		          "Heap: %lx\n",
		          info->fs_dev,
		          info->fs_dev[0] != '\0' ? info->fs_type : "normal",
		          info->fs_part,
		          lex,
		          info->use_dma,
		          info->use_irq,
		          info->heap);
		ds_printf("Emu async: %d\n"
		          "Emu CDDA: 0x%08lx\n"
		          "Emu VMU: %d\n"
		          "Syscalls: %lx\n",
		          "Bleem: %lx\n\n",
		          info->emu_async,
		          info->emu_cdda,
		          info->emu_vmu,
		          info->syscalls,
		          info->bleem);
	}

	isoldr_exec(info, lex);

	return CMD_ERROR;
}
