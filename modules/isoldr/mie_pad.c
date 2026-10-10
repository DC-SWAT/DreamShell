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
#define MIE_PAD_DATA     4
#define MIE_PAD_WORDS    6
#define MIE_PAD_TIMEOUT  1000

static uint8_t mie_pad_tx[32] __attribute__((aligned(32)));

_Static_assert(sizeof(mie_analog_calib_t) == 18, "mie calib size");

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

static int mie_pad_cmd(int on) {
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

void isoldr_naomi_mie_pad(uint32_t image_type) {
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

	if(mie_pad_cmd(on)) {
		ds_printf("DS_PROCESS: MIE pad bridge %s\n", on ? "on" : "off");
	}
	else {
		ds_printf("DS_ERROR: MIE pad bridge switch failed\n");
	}
}
