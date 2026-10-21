/* DreamShell ##version##

   MIE pad bridge switch for ISO Loader
   Copyright (C) 2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "settings.h"

#include <dc/maple.h>
#include <kos/genwait.h>
#include <string.h>

#define MIE_MAPLE_PORT   0
#define MIE_MAPLE_UNIT   0
#define MIE_CMD_IO       0x86
#define MIE_RESP_IO      0x87
#define MIE_IOR_PAD      0x51
#define MIE_IOR_PAD_ACK  0x52
#define MIE_PAD_OFF      0
#define MIE_PAD_ON       1
#define MIE_PAD_CAL      2
#define MIE_PAD_FEAT     3
#define MIE_PAD_FEAT_VMU 1
#define MIE_PAD_DATA     4
#define MIE_PAD_WORDS    6
#define MIE_PAD_TIMEOUT  1000
#define MIE_IO_PAD_MAP    0x53
#define MIE_PAD_MAP_OFF   1
#define MIE_PAD_MAP_DATA  2
#define MIE_PAD_MAP_CHUNK 26
#define MIE_PAD_MAP_BYTES 86

static uint8_t mie_pad_tx[32] __attribute__((aligned(32)));

_Static_assert(sizeof(mie_analog_calib_t) == 18, "mie calib size");
_Static_assert(sizeof(((Settings_t *)0)->jvs_map) == MIE_PAD_MAP_BYTES, "mie pad map size");

static void mie_pad_cb(maple_state_t *st, maple_frame_t *frm) {
	(void)st;
	maple_frame_unlock(frm);
	genwait_wake_all(frm);
}

static int mie_pad_cal_ok(const mie_analog_calib_t *c) {
	if(c->wheel.min >= c->wheel.center || c->wheel.center >= c->wheel.max) {
		return 0;
	}
	if(c->accel.min >= c->accel.max || c->brake.min >= c->brake.max) {
		return 0;
	}
	return 1;
}

static int mie_pad_cmd(int on, uint32_t emu_vmu) {
	maple_device_t *dev;
	maple_frame_t *frm;
	maple_response_t *resp;
	Settings_t *settings;
	int words;

	dev = maple_enum_dev(MIE_MAPLE_PORT, MIE_MAPLE_UNIT);
	if(!dev) {
		return 0;
	}

	frm = &dev->frame;
	maple_frame_lock(frm);
	memset(mie_pad_tx, 0, sizeof(mie_pad_tx));
	mie_pad_tx[0] = MIE_IOR_PAD;
	words = 1;
	if(on) {
		mie_pad_tx[1] = MIE_PAD_ON;
		if(emu_vmu) {
			mie_pad_tx[MIE_PAD_FEAT] = MIE_PAD_FEAT_VMU;
		}
		settings = GetSettings();
		if(settings && mie_pad_cal_ok(&settings->analog)) {
			mie_pad_tx[MIE_PAD_CAL] = 1;
			memcpy(mie_pad_tx + MIE_PAD_DATA, &settings->analog,
			       sizeof(settings->analog));
			words = MIE_PAD_WORDS;
		}
	}
	else {
		mie_pad_tx[1] = MIE_PAD_OFF;
	}

	maple_frame_init(frm);
	frm->cmd = MIE_CMD_IO;
	frm->dst_port = MIE_MAPLE_PORT;
	frm->dst_unit = MIE_MAPLE_UNIT;
	frm->length = words;
	frm->send_buf = (uint32_t *)mie_pad_tx;
	frm->callback = mie_pad_cb;
	maple_queue_frame(frm);

	if(genwait_wait(frm, "mie_pad", MIE_PAD_TIMEOUT) < 0) {
		return 0;
	}

	resp = (maple_response_t *)frm->recv_buf;
	if((uint8_t)resp->response != MIE_RESP_IO || resp->data_len < 1) {
		return 0;
	}
	return resp->data[0] == MIE_IOR_PAD_ACK;
}

static int mie_pad_map_custom(const uint16_t *map, int slots) {
	int i;

	for(i = 0; i < slots; i++) {
		if(map[i] != 0) {
			return 1;
		}
	}
	return 0;
}

static int mie_pad_map_cmd(uint8_t offset, const uint8_t *src, int n) {
	maple_device_t *dev;
	maple_frame_t *frm;
	maple_response_t *resp;
	int words;

	dev = maple_enum_dev(MIE_MAPLE_PORT, MIE_MAPLE_UNIT);
	if(!dev) {
		return 0;
	}

	words = (MIE_PAD_MAP_DATA + n + 3) / 4;
	frm = &dev->frame;
	maple_frame_lock(frm);
	memset(mie_pad_tx, 0, sizeof(mie_pad_tx));
	mie_pad_tx[0] = MIE_IO_PAD_MAP;
	mie_pad_tx[MIE_PAD_MAP_OFF] = offset;
	memcpy(mie_pad_tx + MIE_PAD_MAP_DATA, src, (size_t)n);

	maple_frame_init(frm);
	frm->cmd = MIE_CMD_IO;
	frm->dst_port = MIE_MAPLE_PORT;
	frm->dst_unit = MIE_MAPLE_UNIT;
	frm->length = words;
	frm->send_buf = (uint32_t *)mie_pad_tx;
	frm->callback = mie_pad_cb;
	maple_queue_frame(frm);

	if(genwait_wait(frm, "mie_pad_map", MIE_PAD_TIMEOUT) < 0) {
		return 0;
	}

	resp = (maple_response_t *)frm->recv_buf;
	if((uint8_t)resp->response != MIE_RESP_IO || resp->data_len < 1) {
		return 0;
	}
	return resp->data[0] == MIE_IOR_PAD_ACK;
}

static int mie_pad_send_map(void) {
	Settings_t *settings;
	const uint8_t *raw;
	int slots;
	int bytes;
	int off;
	int n;

	settings = GetSettings();
	if(!settings) {
		return 1;
	}

	slots = (int)(sizeof(settings->jvs_map) / sizeof(settings->jvs_map[0]));
	if(!mie_pad_map_custom(settings->jvs_map, slots)) {
		return 1;
	}

	raw = (const uint8_t *)settings->jvs_map;
	bytes = (int)sizeof(settings->jvs_map);
	for(off = 0; off < bytes; off += MIE_PAD_MAP_CHUNK) {
		n = bytes - off;
		if(n > MIE_PAD_MAP_CHUNK) {
			n = MIE_PAD_MAP_CHUNK;
		}
		if(!mie_pad_map_cmd((uint8_t)off, raw + off, n)) {
			return 0;
		}
	}
	return 1;
}

void isoldr_naomi_mie_pad(uint32_t image_type, uint32_t emu_vmu) {
	int on;

	if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
		return;
	}

	if(image_type == IMAGE_TYPE_ROM_NAOMI) {
		on = 0;
	}
	else {
		on = 1;
	}

	if(mie_pad_cmd(on, emu_vmu)) {
		if(on && !mie_pad_send_map()) {
			ds_printf("DS_ERROR: MIE pad map send failed\n");
		}
		ds_printf("DS_PROCESS: MIE pad bridge %s\n", on ? "on" : "off");
	}
	else {
		ds_printf("DS_ERROR: MIE pad bridge switch failed\n");
	}
}
