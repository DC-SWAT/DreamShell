/* DreamShell ##version##

   DreamShell ISO Loader
   Per-game JVS input map

   Copyright (C)2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include <naomi/input.h>
#include <string.h>

#define INPUT_MAP_MD5_LEN 16

static uint16_t pad_map[DS_JVS_MAP_SRC];
static const isoldr_info_t *pad_map_owner;

static int md5_is_zero(const uint8_t *md5) {
	int i;

	for(i = 0; i < INPUT_MAP_MD5_LEN; i++) {
		if(md5[i]) {
			return 0;
		}
	}

	return 1;
}

void isoldr_input_map_prepare(isoldr_info_t *info, const char *file) {
	uint8_t md5[INPUT_MAP_MD5_LEN];
	char *fn;
	ds_jvs_map_kind_t kind;
	uint16_t map[DS_JVS_MAP_SRC];
	int i;

	pad_map_owner = NULL;

	if(!info || !file) {
		return;
	}
	if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
		return;
	}

	isoldr_image_md5(file, md5);
	if(md5_is_zero(md5)) {
		return;
	}

	fn = isoldr_make_input_map_filename(md5);
	if(!fn || LoadJvsMapFile(fn, &kind, map) < 0) {
		return;
	}

	if(info->image_type == IMAGE_TYPE_ROM_NAOMI) {
		if(kind != DS_JVS_MAP_KIND_JVS) {
			return;
		}
		for(i = 0; i < DS_JVS_MAP_SRC; i++) {
			info->jvs_map[i] = (uint8_t)map[i];
		}
		return;
	}

	if(kind != DS_JVS_MAP_KIND_PAD) {
		return;
	}

	memcpy(pad_map, map, sizeof(pad_map));
	pad_map_owner = info;
}

const uint16_t *isoldr_game_pad_map(const isoldr_info_t *info) {
	if(!info || pad_map_owner != info) {
		return NULL;
	}

	return pad_map;
}
