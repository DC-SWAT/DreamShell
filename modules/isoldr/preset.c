/* DreamShell ##version##

   DreamShell ISO Loader
   Preset file support

   Copyright (C)2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "naomi/cart.h"
#include <kos/md5.h>

static char *trim_spaces(char *txt) {
	int32_t i;
	while(txt[0] == ' ') txt++;
	int32_t len = strlen(txt);
	for(i=len; i ; i--) {
		if(txt[i] > ' ') break;
		txt[i] = '\0';
	}
	return txt;
}

enum {
	CONF_END = 0,
	CONF_INT = 1,
	CONF_STR,
	CONF_ULONG,
};

typedef struct {
	const char *name;
	int conf_type;
	void *pointer;
} isoldr_conf;

static int conf_parse(isoldr_conf *cfg, const char *filename) {
	file_t fd = fs_open(filename, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}

	size_t size = fs_total(fd);
	char *buf = (char *)malloc(size + 1);
	char *optname = NULL, *value = NULL;

	if(!buf) {
		fs_close(fd);
		return -1;
	}

	if(fs_read(fd, buf, size) != size) {
		fs_close(fd);
		free(buf);
		ds_printf("DS_ERROR: Can't read %s\n", filename);
		return -1;
	}

	buf[size] = '\0';
	fs_close(fd);

	char *cur = buf;

	while(1) {
		if(optname == NULL)
			optname = strtok(cur, "=");
		else
			optname = strtok('\0', "=");

		if(cur == buf) cur = NULL;

		value = strtok('\0', "\n");

		if(optname == NULL || value == NULL) break;

		for(int i = 0; cfg[i].conf_type; i++) {
			if(strncasecmp(cfg[i].name, trim_spaces(optname), 32)) continue;

			switch(cfg[i].conf_type) {
				case CONF_INT:
					*(int *)cfg[i].pointer = atoi(value);
					break;

				case CONF_STR:
					strcpy((char *)cfg[i].pointer, trim_spaces(value));
					break;

				case CONF_ULONG:
					*(uint32_t *)cfg[i].pointer = strtoul(value, NULL, 16);
			}
			break;
		}
	}
	free(buf);
	return 0;
}

static const char *preset_mount_points[] = {
	"/presets_cd",
	"/presets_sd",
	"/presets_ide"
};

static const char *preset_romdisk_names[] = {
	"presets_cd.romfs",
	"presets_sd.romfs",
	"presets_ide.romfs"
};

static uint8 *preset_romdisk_data[3] = {NULL, NULL, NULL};

static int get_preset_device_index(const char *dir) {
	if(!strncasecmp(dir, "/cd",  3)) return 0;
	if(!strncasecmp(dir, "/sd",  3)) return 1;
	if(!strncasecmp(dir, "/ide", 4)) return 2;
	return -1;
}

static void format_preset_filename(const char *base_path, const char *dir, uint8_t *md5, char *output) {
	char dev[8];

	memset(dev, 0, sizeof(dev));
	strncpy(dev, &dir[1], 3);
	dev[2] = (dev[2] == '/') ? '\0' : dev[2];
	dev[3] = '\0';

	snprintf(output, NAME_MAX,
		"%s/%s_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.cfg",
		base_path, dev,
		md5[0],  md5[1],  md5[2],  md5[3],
		md5[4],  md5[5],  md5[6],  md5[7],
		md5[8],  md5[9],  md5[10], md5[11],
		md5[12], md5[13], md5[14], md5[15]);
}

static char preset_filename_buf[NAME_MAX];

#define PRESET_ISO_HASH_LEN 2048
#define PRESET_MD5_LEN 16
#define PRESET_HASH_MOUNT "/isohash"
#define PRESET_ISO_SECTOR_SIZE 2048

static int hash_boot_sector_md5(const char *image_file, uint8_t *md5) {
	file_t fd;
	file_t mounted;
	uint8_t buf[PRESET_ISO_HASH_LEN];

	mounted = fs_open(PRESET_HASH_MOUNT, O_DIR | O_RDONLY);
	if(mounted != FILEHND_INVALID) {
		fs_close(mounted);
		if(fs_iso_unmount(PRESET_HASH_MOUNT) < 0) {
			return 0;
		}
	}

	if(fs_iso_mount(PRESET_HASH_MOUNT, image_file) < 0) {
		return 0;
	}

	fd = fs_iso_first_file(PRESET_HASH_MOUNT);
	if(fd == FILEHND_INVALID) {
		fs_iso_unmount(PRESET_HASH_MOUNT);
		return 0;
	}

	if(fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, buf) < 0) {
		fs_close(fd);
		fs_iso_unmount(PRESET_HASH_MOUNT);
		return 0;
	}

	fs_close(fd);
	fs_iso_unmount(PRESET_HASH_MOUNT);
	kos_md5(buf, PRESET_ISO_HASH_LEN, md5);
	return 1;
}

static void hash_image_md5(const char *image_file, uint8_t *md5) {
	file_t fd;
	const char *ext;
	uint8_t buf[PRESET_ISO_HASH_LEN];
	naomi_cart_header_t cart_hdr;
	ssize_t rd;

	memset(md5, 0, PRESET_MD5_LEN);

	ext = strrchr(image_file, '.');
	if(ext && !strcasecmp(ext, ".dni")) {
		fd = fs_open(image_file, O_RDONLY);
		if(fd == FILEHND_INVALID) {
			return;
		}
		if(isoldr_naomi_read_header(fd, &cart_hdr) < 0) {
			fs_close(fd);
			return;
		}
		kos_md5((uint8_t *)&cart_hdr, sizeof(cart_hdr), md5);
		fs_close(fd);
		return;
	}

	if(hash_boot_sector_md5(image_file, md5)) {
		return;
	}

	fd = fs_open(image_file, O_RDONLY);
	if(fd == FILEHND_INVALID) {
		return;
	}

	rd = fs_read(fd, buf, PRESET_ISO_HASH_LEN);
	fs_close(fd);
	if(rd == (ssize_t)PRESET_ISO_HASH_LEN) {
		kos_md5(buf, PRESET_ISO_HASH_LEN, md5);
	}
}

char *isoldr_make_preset_filename(const char *image_file, uint8_t *md5) {
	char base_path[NAME_MAX];

	memset(preset_filename_buf, 0, sizeof(preset_filename_buf));
	snprintf(base_path, NAME_MAX, "%s/apps/iso_loader/presets", getenv("PATH"));
	format_preset_filename(base_path, image_file, md5, preset_filename_buf);
	return preset_filename_buf;
}

static int mount_presets_romdisk(int device_idx) {
	char romdisk_path[NAME_MAX];

	if(device_idx < 0 || device_idx >= 3 || preset_romdisk_data[device_idx]) {
		return 0;
	}

	snprintf(romdisk_path, NAME_MAX, "%s/apps/iso_loader/resources/%s",
		getenv("PATH"), preset_romdisk_names[device_idx]);

	if(fs_load(romdisk_path, (void **)&preset_romdisk_data[device_idx]) <= 0) {
		ds_printf("DS_ERROR: Failed to load romdisk %s\n", romdisk_path);
		return -1;
	}

	if(fs_romdisk_mount(preset_mount_points[device_idx], preset_romdisk_data[device_idx], 0) < 0) {
		ds_printf("DS_ERROR: Failed to mount romdisk %s\n", romdisk_path);
		free(preset_romdisk_data[device_idx]);
		preset_romdisk_data[device_idx] = NULL;
		return -1;
	}

	return 0;
}

static void unmount_presets_romdisk(int device_idx) {
	if(device_idx < 0 || device_idx >= 3 || !preset_romdisk_data[device_idx]) {
		return;
	}

	fs_romdisk_unmount(preset_mount_points[device_idx]);
	free(preset_romdisk_data[device_idx]);
	preset_romdisk_data[device_idx] = NULL;
}

void isoldr_unmount_all_presets_romdisks(void) {
	for(int i = 0; i < sizeof(preset_romdisk_data) / sizeof(preset_romdisk_data[0]); i++) {
		unmount_presets_romdisk(i);
	}
}

char *isoldr_find_preset(const char *image_file, uint8_t *md5, int default_only) {
	uint8_t local_md5[PRESET_MD5_LEN];
	int device_idx;

	if(!md5) {
		hash_image_md5(image_file, local_md5);
		md5 = local_md5;
	}

	if(!default_only) {
		char *preset_file = isoldr_make_preset_filename(image_file, md5);

		if(FileSize(preset_file) > 0) {
			return preset_file;
		}
	}

	device_idx = get_preset_device_index(image_file);

	if(device_idx < 0) {
		return NULL;
	}

	if(mount_presets_romdisk(device_idx) < 0) {
		return NULL;
	}

	format_preset_filename(preset_mount_points[device_idx], image_file, md5, preset_filename_buf);

	if(FileSize(preset_filename_buf) > 0) {
		return preset_filename_buf;
	}

	return NULL;
}

static int g1_dma_device(const isoldr_info_t *info) {
	return strncasecmp(info->fs_dev, ISOLDR_DEV_G1ATA, sizeof(info->fs_dev)) == 0
		|| strncasecmp(info->fs_dev, ISOLDR_DEV_GDROM, sizeof(info->fs_dev)) == 0;
}

int isoldr_can_use_dma(const isoldr_info_t *info) {
	int iso_sectors;
	int data_image;

	if(!g1_dma_device(info)) {
		return 0;
	}

	iso_sectors = info->sector_size > 0 && info->sector_size <= PRESET_ISO_SECTOR_SIZE;
	data_image = info->image_type == IMAGE_TYPE_ISO ||
		info->image_type == IMAGE_TYPE_GDI ||
		info->image_type == IMAGE_TYPE_CDI ||
		info->image_type == IMAGE_TYPE_ROM_NAOMI;

	if(iso_sectors &&
		info->image_type != IMAGE_TYPE_CSO &&
		info->image_type != IMAGE_TYPE_ZSO) {
		return 2;
	}
	if(data_image || iso_sectors) {
		return 1;
	}
	return 0;
}

static void build_image_full_path(const isoldr_info_t *isoldr, char *out, size_t size) {
	if(isoldr->fs_dev[0] == '\0') {
		strncpy(out, isoldr->image_file, size - 1);
		out[size - 1] = '\0';
		return;
	}
	if(isoldr->fs_part) {
		snprintf(out, size, "/%s%lu%s", isoldr->fs_dev, isoldr->fs_part, isoldr->image_file);
	}
	else {
		snprintf(out, size, "/%s%s", isoldr->fs_dev, isoldr->image_file);
	}
}

static int toc_has_cdda(const cd_toc_t *toc) {
	int first = TOC_TRACK(toc->first);
	int last = TOC_TRACK(toc->last);
	int i;

	if(first < 1 || last > 99 || first > last) {
		return 0;
	}

	for(i = first; i <= last; i++) {
		uint32_t entry = toc->entry[i - 1];

		if(entry == (uint32_t)-1) {
			continue;
		}
		if(TOC_CTRL(entry) == 0) {
			return 1;
		}
	}
	return 0;
}

uintptr_t isoldr_apply_preset(isoldr_info_t *isoldr, const char *preset_file) {

	uintptr_t exec_addr = ISOLDR_DEFAULT_ADDR_LOW;
	int use_dma = 1, emu_async = 8, use_irq = 0, alt_read = 0, use_gpio = 0;
	int fastboot = 0, low = 0, emu_vmu = 0, scr_hotkey = 0, region = -1;
	int boot_mode = BOOT_MODE_DIRECT;
	int bin_type = BIN_TYPE_AUTO;
	int naomi_set = 0;
	uint32_t heap = HEAP_MODE_AUTO, emu_cdda = 0;
	char title[32] = "";
	char device[8] = "";
	char memory[12] = "";
	char heap_memory[12] = "";
	char bin_file[12] = "";
	char patch_a[2][10];
	char patch_v[2][10];

	memset(patch_a, 0, 2 * 10);
	memset(patch_v, 0, 2 * 10);

	if(preset_file) {
		isoldr_conf options[] = {
			{ "dma",      CONF_INT,   (void *) &use_dma    },
			{ "altread",  CONF_INT,   (void *) &alt_read   },
			{ "cdda",     CONF_ULONG, (void *) &emu_cdda   },
			{ "irq",      CONF_INT,   (void *) &use_irq    },
			{ "low",      CONF_INT,   (void *) &low        },
			{ "vmu",      CONF_INT,   (void *) &emu_vmu    },
			{ "scrhotkey",CONF_INT,   (void *) &scr_hotkey },
			{ "gpio",     CONF_INT,   (void *) &use_gpio   },
			{ "region",   CONF_INT,   (void *) &region     },
			{ "heap",     CONF_STR,   (void *) heap_memory },
			{ "memory",   CONF_STR,   (void *) memory      },
			{ "async",    CONF_INT,   (void *) &emu_async  },
			{ "mode",     CONF_INT,   (void *) &boot_mode  },
			{ "type",     CONF_INT,   (void *) &bin_type   },
			{ "file",     CONF_STR,   (void *) bin_file    },
			{ "title",    CONF_STR,   (void *) title       },
			{ "device",   CONF_STR,   (void *) device      },
			{ "fastboot", CONF_INT,   (void *) &fastboot   },
			{ "naomi",    CONF_INT,   (void *) &naomi_set  },
			{ "pa1",      CONF_STR,   (void *) patch_a[0]  },
			{ "pv1",      CONF_STR,   (void *) patch_v[0]  },
			{ "pa2",      CONF_STR,   (void *) patch_a[1]  },
			{ "pv2",      CONF_STR,   (void *) patch_v[1]  },
			{ NULL,       CONF_END,   NULL }
		};
		if(conf_parse(options, preset_file) < 0) {
			ds_printf("DS_ERROR: Can't parse preset\n");
			return (uintptr_t)-1;
		}
	}
	else {
		use_dma = isoldr_can_use_dma(isoldr);
		if(use_dma > 1) {
			emu_async = 0;
			use_dma = 1;
		}
	}

	if(region == -1) {
		region = flashrom_get_region_only();
		if(region <= 0) {
			region = ISOLDR_REGION_JAPAN;
		}
	}

	/* Must be computed before exec.type is overwritten */
	int actual_type = (bin_type != BIN_TYPE_AUTO) ? bin_type : (int)isoldr->exec.type;

	isoldr->use_dma = use_dma;
	isoldr->alt_read = alt_read;
	isoldr->emu_cdda = emu_cdda;
	isoldr->use_irq = use_irq;
	if(!preset_file && isoldr->image_type == IMAGE_TYPE_ROM_NAOMI) {
		isoldr->use_irq = 1;
	}
	isoldr->syscalls = low;
	isoldr->emu_async = emu_async;
	isoldr->region = region;
	isoldr->boot_mode = boot_mode;
	if(bin_type != BIN_TYPE_AUTO) {
		isoldr->exec.type = bin_type;
	}
	isoldr->fast_boot = fastboot;
	isoldr->use_gpio = use_gpio;
	isoldr->scr_hotkey = scr_hotkey;
	isoldr->emu_vmu = emu_vmu;

	if(strlen(heap_memory) > 0) {
		isoldr->heap = strtoul(heap_memory, NULL, 16);
	}
	else if(heap != HEAP_MODE_AUTO) {
		isoldr->heap = heap;
	}

	if(strlen(memory) > 0) {
		exec_addr = strtoul(memory, NULL, 16);
	}
	else if(isoldr->image_type == IMAGE_TYPE_ROM_NAOMI) {
		uint32_t phys = isoldr->exec.addr & MEM_AREA_CACHE_MASK;

		if((phys >= 0x0d000000 && phys < 0x0d010000)
			|| (isoldr->exec.addr & 0x80000000)) {
			exec_addr = ISOLDR_DEFAULT_ADDR_NAOMI_DC;
		}
		else {
			exec_addr = ISOLDR_DEFAULT_ADDR_NAOMI;
		}
	}

	/* Platform-specific address overrides (applied even when preset is loaded) */
	if(hardware_sys_mode(NULL) != HW_TYPE_RETAIL
	        && isoldr->image_type != IMAGE_TYPE_ROM_NAOMI
	        && !naomi_set) {
		/* Non-retail (NAOMI arcade) hardware with Dreamcast disc image */
		if(actual_type == BIN_TYPE_KOS) {
			exec_addr = ISOLDR_DEFAULT_ADDR_MIN;
			isoldr->boot_mode = BOOT_MODE_DIRECT;
		}
		else {
			exec_addr = ISOLDR_DEFAULT_ADDR_NAOMI_DC;
			isoldr->boot_mode = BOOT_MODE_IPBIN;
		}
	}
	else if(actual_type == BIN_TYPE_WINCE) {
		/* WinCE: use minimum address on retail hardware */
		exec_addr = ISOLDR_DEFAULT_ADDR_MIN;
	}

	if(hardware_sys_mode(NULL) != HW_TYPE_RETAIL
	        && isoldr->image_type != IMAGE_TYPE_ROM_NAOMI
	        && (actual_type != BIN_TYPE_KOS || exec_addr >= ISOLDR_DEFAULT_ADDR)) {
		isoldr->firmware = 1;
	}

	if(strlen(device) > 0 && strncmp(device, "auto", 4) != 0) {
		strncpy(isoldr->fs_dev, device, sizeof(isoldr->fs_dev));
	}

	if(strlen(bin_file) > 0) {
		char image_path[NAME_MAX];
		build_image_full_path(isoldr, image_path, sizeof(image_path));
		isoldr_set_boot_file(isoldr, image_path, bin_file);
	}

	for(int i = 0; i < 2; ++i) {
		if(strlen(patch_a[i]) > 0 && strlen(patch_v[i]) > 0) {
			isoldr->patch_addr[i] = strtoul(patch_a[i], NULL, 16);
			isoldr->patch_value[i] = strtoul(patch_v[i], NULL, 16);
		}
	}

	if(!preset_file && isoldr->image_type != IMAGE_TYPE_ROM_NAOMI && toc_has_cdda(&isoldr->toc)) {
		isoldr->emu_cdda = CDDA_MODE_EXTENDED | CDDA_MODE_SRC_DMA |
			CDDA_MODE_DST_DMA | CDDA_MODE_POS_TMU2 | CDDA_MODE_CH_ADAPT;
		isoldr->use_irq = 1;
		if(exec_addr == ISOLDR_DEFAULT_ADDR_LOW) {
			exec_addr = ISOLDR_DEFAULT_ADDR_MIN_GINSU;
		}
	}

	return exec_addr;
}

int isoldr_save_preset(isoldr_info_t *info, const char *filename,
                       const char *title, uintptr_t loader_addr,
                       const char *alt_file) {
	file_t fd;
	char result[1024] = "";
	size_t result_len = 0;

	if (!info || !filename) {
		return -1;
	}

	fd = fs_open(filename, O_CREAT | O_TRUNC | O_WRONLY);

	if (fd == FILEHND_INVALID) {
		return -1;
	}

	snprintf(result, sizeof(result),
			"title = %s\n"
			"device = %s\n"
			"dma = %lu\n"
			"async = %lu\n"
			"cdda = %08lx\n"
			"irq = %lu\n"
			"low = %lu\n"
			"heap = %08lx\n"
			"fastboot = %lu\n"
			"type = %lu\n"
			"mode = %lu\n"
			"memory = %08lx\n"
			"vmu = %lu\n"
			"scrhotkey = %lu\n"
			"altread = %lu\n"
			"gpio = %lu\n"
			"region = %lu\n"
			"pa1 = %08lx\n"
			"pv1 = %08lx\n"
			"pa2 = %08lx\n"
			"pv2 = %08lx\n",
			title ? title : "",
			info->fs_dev,
			info->use_dma,
			info->emu_async,
			info->emu_cdda,
			info->use_irq,
			info->syscalls,
			info->heap,
			info->fast_boot,
			info->exec.type,
			info->boot_mode,
			(uint32)loader_addr,
			info->emu_vmu,
			info->scr_hotkey,
			info->alt_read,
			info->use_gpio,
			info->region,
			info->patch_addr[0], info->patch_value[0],
			info->patch_addr[1], info->patch_value[1]
	);

	if(alt_file && alt_file[0]) {
		strncat(result, "file = ", sizeof(result) - strlen(result) - 1);
		strncat(result, alt_file, sizeof(result) - strlen(result) - 1);
		strncat(result, "\n", sizeof(result) - strlen(result) - 1);
	}

	if(hardware_sys_mode(NULL) != HW_TYPE_RETAIL) {
		strncat(result, "naomi = 1\n", sizeof(result) - strlen(result) - 1);
	}

	result_len = strlen(result);

	if(fs_write(fd, result, result_len) != result_len) {
		fs_close(fd);
		return -1;
	}
	fs_close(fd);
	return 0;
}
