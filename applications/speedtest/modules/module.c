/* DreamShell ##version##
	
	module.c - Speedtest app module
	Copyright (C)2014 megavolt85
	Copyright (C)2014-2020, 2024 SWAT
	
	http://www.dc-swat.ru
*/

#include "ds.h"
#include "drivers/sd.h"

DEFAULT_MODULE_EXPORTS(app_speedtest);

static struct {
	App_t *app;
	GUI_Widget *cd_c;
	GUI_Widget *sd_c;
	GUI_Widget *hdd_c;
	GUI_Widget *pc_c;
	GUI_Widget *fs_write_text;
	GUI_Widget *fs_read_text;
	GUI_Widget *io_read_text;
	GUI_Widget *status;
} self;

static uint8 rd_buff[0x10000] __attribute__((aligned(32)));

static void show_status_ok(char *msg) {
	GUI_LabelSetTextColor(self.status, 28, 227, 70);
	GUI_LabelSetText(self.status, msg);
	ScreenWaitUpdate();
}

static void show_status_error(char *msg) {
	GUI_LabelSetTextColor(self.status, 255, 20, 20);
	GUI_LabelSetText(self.status, msg);
}

int test_ide_io_io(void) {
	kos_blockdev_t bdev;
	uint64 st, et;
	uint32_t tm;
	uint8_t pt;
	uint8_t *buf;
	const size_t blocks = 4096;
	const size_t buf_size = blocks * 512;
	double speed;
	char result[256];

	buf = memalign(32, buf_size);

	if(!buf) {
		return -1;
	}
	if(g1_ata_blockdev_for_partition(0, 1, &bdev, &pt)) {
		dbglog(DBG_DEBUG, "Couldn't get blockdev for partition!\n");
		free(buf);
		return -1;
	}
	show_status_ok("Testing IO read speed...");
	GUI_LabelSetText(self.io_read_text, "...");

	ShutdownVideoThread();
	st = timer_ns_gettime64();

	if(bdev.read_blocks(&bdev, 0, blocks, buf)) {
		dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
		free(buf);
		return -1;
	}
	et = timer_ns_gettime64();
	tm = (et - st) / 1000000;
	free(buf);

	InitVideoThread();
	speed = buf_size / ((float)tm / 1000);

	snprintf(result, sizeof(result), 
		"IO read: %.2f Kbytes/s or %.2f Mbit/s",
		speed / 1024, ((speed / 1024) / 1024) * 8);
	GUI_LabelSetText(self.io_read_text, result);
	show_status_ok("Complete!"); 

	ds_printf("DS_OK: Complete!\n"
		" Test: IO read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n",
		tm, speed / 1024, 
		((speed / 1024) / 1024) * 8, 
		buf_size / 1024);

	return 0;
}

int test_sd_io(void) {
	kos_blockdev_t bdev;
	uint64 st, et;
	uint32_t tm;
	uint8_t pt;
	uint8_t *buf;
	const size_t blocks = 1024;
	const size_t buf_size = blocks * 512;
	double speed;
	char result[256];

	buf = memalign(32, buf_size);

	if(!buf) {
		return -1;
	}
	if(sdc_blockdev_for_partition(0, &bdev, &pt)) {
		dbglog(DBG_DEBUG, "Couldn't get blockdev for partition!\n");
		free(buf);
		return -1;
	}
	show_status_ok("Testing SD IO read speed...");
	GUI_LabelSetText(self.io_read_text, "...");

	ShutdownVideoThread();
	st = timer_ns_gettime64();

	if(bdev.read_blocks(&bdev, 0, blocks, buf)) {
		dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
		free(buf);
		return -1;
	}
	et = timer_ns_gettime64();
	tm = (et - st) / 1000000;
	free(buf);

	InitVideoThread();
	speed = buf_size / ((float)tm / 1000);

	snprintf(result, sizeof(result), 
		"SD IO read: %.2f Kbytes/s or %.2f Mbit/s",
		speed / 1024, ((speed / 1024) / 1024) * 8);
	GUI_LabelSetText(self.io_read_text, result);
	show_status_ok("Complete!"); 

	ds_printf("DS_OK: Complete!\n"
		" Test: SD IO read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n",
		tm, speed / 1024, 
		((speed / 1024) / 1024) * 8, 
		buf_size / 1024);

	return 0;
}

void Speedtest_Run(GUI_Widget *widget) {

	uint8 *buff = (uint8*)0x8c400000;
	size_t buff_size = 0x10000;
	int size = 0x800000, cnt = 0, rs; 
	int64 time_before, time_after;
	uint32 t;
	double speed;
	file_t fd;
	int read_only = 0, is_ide = 0, is_sd = 0;
	
	char name[64];
	char result[256];
	const char *wname = GUI_ObjectGetName((GUI_Object *)widget);

	GUI_LabelSetText(self.fs_write_text, " ");
	GUI_LabelSetText(self.fs_read_text, " ");
	GUI_LabelSetText(self.io_read_text, " ");

	if(!strncmp(wname, "/ide", 4)) {
		is_ide = 1;
	}
	else if(!strncmp(wname, "/sd", 3)) {
		is_sd = 1;
		size >>= 2;
	}
	else if(!strncmp(wname, "/cd", 3)) {
		read_only = 1;
		snprintf(name, sizeof(name), "%s/1DS_CORE.BIN", wname);

		if(FileExists(name)) {
			goto readtest;
		} else {
			snprintf(name, sizeof(name), "%s/1ST_READ.BIN", wname);
			goto readtest;
		}
	}

	show_status_ok("Testing FS write speed...");
	GUI_LabelSetText(self.fs_write_text, "...");

	snprintf(name, sizeof(name), "%s/%s.tst", wname, lib_get_name());

	if(FileExists(name)) {
		fs_unlink(name);
	}

	/* WRITE TEST */
	fd = fs_open(name, O_WRONLY | O_CREAT | O_TRUNC);

	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for write: %d\n", name, errno);
		show_status_error("Can't open file for write");
		return;
	}

	ShutdownVideoThread(); 
	time_before = timer_ns_gettime64();

	while(cnt < size) {

		rs = fs_write(fd, buff, buff_size);

		if(rs <= 0) {
			fs_close(fd);
			InitVideoThread(); 
			ds_printf("DS_ERROR: Can't write to file: %d\n", errno);
			show_status_error("Can't write to file");
			return;
		}
		buff += rs;
		cnt += rs;
	}

	time_after = timer_ns_gettime64();
	InitVideoThread();

	t = (time_after - time_before) / 1000000;
	speed = size / ((float)t / 1000);
	fs_close(fd);

	snprintf(result, sizeof(result), 
		"FS write: %.2f Kbytes/s or %.2f Mbit/s",
		speed / 1024, ((speed / 1024) / 1024) * 8);

	GUI_LabelSetText(self.fs_write_text, result);
	show_status_ok("Complete!");

	ds_printf("DS_OK: Complete!\n"
		" Test: FS write\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n Buff: %d Kb\n", 
		t, speed / 1024, 
		((speed / 1024) / 1024) * 8, 
		size / 1024, buff_size / 1024);

readtest:

	show_status_ok("Testing FS read speed...");
	GUI_LabelSetText(self.fs_read_text, "...");

	/* READ TEST */
	fd = fs_open(name, O_RDONLY);

	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for read: %d\n", name, errno);
		show_status_error("Can't open file for read");
		return;
	}
	if(read_only) {
		GUI_LabelSetText(self.fs_write_text, "Write test passed");
		/* Reset ISO9660 filesystem cache */
		fs_ioctl(fd, 0, NULL);
		fs_close(fd);
		fd = fs_open(name, O_RDONLY);
	}

	time_before = time_after = t = cnt = 0;
	speed = 0.0f;
	size = fs_total(fd);
	buff = rd_buff;

	ShutdownVideoThread();
	time_before = timer_ns_gettime64();

	while(cnt < size) {

		rs = fs_read(fd, buff, buff_size);

		if(rs <= 0) {
			fs_close(fd);
			InitVideoThread(); 
			ds_printf("DS_ERROR: Can't read file: %d\n", errno);
			show_status_error("Can't read file");
			return;
		}
		cnt += rs;
	}

	time_after = timer_ns_gettime64();
	t = (uint32)(time_after - time_before) / 1000000;
	speed = size / ((float)t / 1000);

	fs_close(fd);

	if(!read_only) { 
		fs_unlink(name);
	} else {
		cdrom_spin_down();
	}

	snprintf(result, sizeof(result), 
		"FS read: %.2f Kbytes/s or %.2f Mbit/s",
		speed / 1024, ((speed / 1024) / 1024) * 8);

	InitVideoThread();

	ds_printf("DS_OK: Complete!\n"
		" Test: FS read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n Buff: %d Kb\n", 
		t, speed / 1024, 
		((speed / 1024) / 1024) * 8, 
		size / 1024, buff_size / 1024);

	GUI_LabelSetText(self.fs_read_text, result);
	show_status_ok("Complete!"); 

	if(is_ide) {
		test_ide_io_io();
	}
	else if(is_sd) {
		test_sd_io();
	}
}


void Speedtest_Init(App_t *app) {

	memset(&self, 0, sizeof(self));

	if(app != NULL) {

		self.app = app;

		self.fs_read_text = APP_GET_WIDGET("fs-read-text");
		self.io_read_text = APP_GET_WIDGET("io-read-text");
		self.fs_write_text = APP_GET_WIDGET("fs-write-text");
		self.status  = APP_GET_WIDGET("status-text");

		self.cd_c  = APP_GET_WIDGET("/cd");
		self.sd_c  = APP_GET_WIDGET("/sd");
		self.hdd_c = APP_GET_WIDGET("/ide");
		self.pc_c  = APP_GET_WIDGET("/pc");

		if(!DirExists("/pc")) GUI_WidgetSetEnabled(self.pc_c, 0);
		if(!DirExists("/sd")) GUI_WidgetSetEnabled(self.sd_c, 0);
		if(is_custom_bios()/*!DirExists("/cd")*/) {
			GUI_WidgetSetEnabled(self.cd_c, 0);
		}
		if(!DirExists("/ide")) GUI_WidgetSetEnabled(self.hdd_c, 0);

	} else {
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}
