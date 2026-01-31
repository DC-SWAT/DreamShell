/* DreamShell ##version##

   DreamShell ISO Loader
   Preset file support

   Copyright (C)2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "naomi/cart.h"

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

uintptr_t isoldr_apply_preset(isoldr_info_t *isoldr, const char *preset_file) {

	uintptr_t exec_addr = ISOLDR_DEFAULT_ADDR_LOW;
	int use_dma = 1, emu_async = 8, use_irq = 0, alt_read = 0, use_gpio = 0;
	int fastboot = 0, low = 0, emu_vmu = 0, scr_hotkey = 0, region = -1;
	int boot_mode = BOOT_MODE_DIRECT;
	int bin_type = BIN_TYPE_AUTO;
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
		if(isoldr->image_type == IMAGE_TYPE_ROM_NAOMI) {
			strcpy(memory, "0x8c005000");
		}
		else if(hardware_sys_mode(NULL) != HW_TYPE_RETAIL) {
			strcpy(memory, "0x8dfe0000");
		}
	}

	if(region == -1) {
		switch(flashrom_get_region_only()) {
			case FLASHROM_REGION_US:
				region = NAOMI_REGION_USA;
				break;
			case FLASHROM_REGION_EUROPE:
				region = NAOMI_REGION_EXPORT;
				break;
			case FLASHROM_REGION_JAPAN:
			default:
				region = NAOMI_REGION_JAPAN;
				break;
		}
	}

	isoldr->use_dma = use_dma;
	isoldr->alt_read = alt_read;
	isoldr->emu_cdda = emu_cdda;
	isoldr->use_irq = use_irq;
	isoldr->syscalls = low;
	isoldr->emu_async = emu_async;
	isoldr->region = region;
	isoldr->boot_mode = boot_mode;
	isoldr->exec.type = bin_type;
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

	if(strlen(device) > 0 && strncmp(device, "auto", 4) != 0) {
		strncpy(isoldr->fs_dev, device, sizeof(isoldr->fs_dev));
	}

	if(strlen(bin_file) > 0) {
		isoldr_set_boot_file(isoldr, isoldr->image_file, bin_file);
	}

	for(int i = 0; i < 2; ++i) {
		if(strlen(patch_a[i]) > 0 && strlen(patch_v[i]) > 0) {
			isoldr->patch_addr[i] = strtoul(patch_a[i], NULL, 16);
			isoldr->patch_value[i] = strtoul(patch_v[i], NULL, 16);
		}
	}
	return exec_addr;
}
