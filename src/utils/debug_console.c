/* KallistiOS ##version##

   util/dsd_console.c
   Copyright (C) 2011-2014, 2026 SWAT

*/


#include "ds.h"
#include <string.h>
#include <errno.h>
#include <kos/dbgio.h>


static file_t flog = 0;

static int dsd_detected() {
	return 1;
}

static int dsd_init() {
	return 0;
}

static int dsd_shutdown() {
	return 0;
}


static int dsd_sd_init() {

	if(flog == 0) {
		flog = fs_open("/sd/ds.log", O_WRONLY);
	}
	return 0;
}

static int dsd_sd_shutdown() {
	if(flog > 0) {
		fs_close(flog);
	}
	return 0;
}


static int dsd_set_irq_usage(int mode) {
	(void)mode;

	return 0;
}

static int dsd_flush() {
	return 0;
}

static int dsd_write_buffer(const uint8_t *data, int len, int xlat) {
	static char irq_buf[1024];
	char *buf;
	int owned = 0;
	int n = len;

	(void)xlat;

	if(n <= 0) {
		return 0;
	}

	if(irq_inside_int()) {
		if(n >= (int)sizeof(irq_buf)) {
			n = (int)sizeof(irq_buf) - 1;
		}
		buf = irq_buf;
	}
	else {
		buf = (char *)malloc((size_t)n + 1);
		if(!buf) {
			return -1;
		}
		owned = 1;
	}

	memcpy(buf, data, (size_t)n);
	buf[n] = '\0';
	ds_printf("%s", buf);
	if(owned) {
		free(buf);
	}
	return len;
}


static int dsd_sd_write_buffer(const uint8_t *data, int len, int xlat) {

	if(flog == 0) {
		dsd_sd_init();
	}

	if(flog < 0) {
		return dsd_write_buffer(data, len, xlat);
	}

	return fs_write(flog, data, len);
}

static int dsd_read_buffer(uint8_t *data, int len) {
	(void)data;
	(void)len;

	errno = EAGAIN;
	return -1;
}

dbgio_handler_t dbgio_ds = {
	.name = "ds",
	.detected = dsd_detected,
	.init = dsd_init,
	.shutdown = dsd_shutdown,
	.set_irq_usage = dsd_set_irq_usage,
	.flush = dsd_flush,
	.write_buffer = dsd_write_buffer,
	.read_buffer = dsd_read_buffer
};

dbgio_handler_t dbgio_sd = {
	.name = "sd",
	.detected = dsd_detected,
	.init = dsd_sd_init,
	.shutdown = dsd_sd_shutdown,
	.set_irq_usage = dsd_set_irq_usage,
	.flush = dsd_flush,
	.write_buffer = dsd_sd_write_buffer,
	.read_buffer = dsd_read_buffer
};


void dbgio_set_dev_ds() {
	dbgio_add_handler(&dbgio_ds);
	dbgio_dev_select("ds");
}

void dbgio_set_dev_sd() {
	dbgio_add_handler(&dbgio_sd);
	dbgio_dev_select("sd");
}

void dbgio_set_dev_scif() {
	dbgio_dev_select("scif");
}

void dbgio_set_dev_fb() {
	dbgio_dev_select("fb");
}
