/* DreamShell ##version##
	
	module.c - Speedtest app module
	Copyright (C)2014 megavolt85
	Copyright (C)2014-2020, 2024, 2026 SWAT
	
	http://www.dc-swat.ru
*/

#include "ds.h"
#include <dc/sd.h>
#include <dc/sci.h>
#include <dc/g1ata.h>
#include <kos/blockdev.h>

DEFAULT_MODULE_EXPORTS(app_speedtest);

typedef struct {
	GUI_Widget *card;
	GUI_Widget *size_label;
	GUI_Widget *lba_label;
	GUI_Widget *iface_label;
	GUI_Widget *test_btn;
} device_ui_t;

static struct {
	App_t *app;
	device_ui_t sd;
	device_ui_t ide;
	device_ui_t cd;
	GUI_Widget *dialog;
	char result_body[2048];
	char test_wname[16];
	char test_device[32];
	char test_file[64];
	int testing;
	int test_read_only;
	int test_is_ide;
	int test_is_sd;
	int test_skip_write;
	int test_size;
} self;

static void format_size(uint64_t bytes, char *buf, size_t len) {

	if(!bytes || bytes == (uint64_t)-1) {
		snprintf(buf, len, "N/A");
	}
	else if(bytes >= 1024ULL * 1024 * 1024) {
		snprintf(buf, len, "%.2f GB", bytes / (1024.0 * 1024 * 1024));
	}
	else if(bytes >= 1024 * 1024) {
		snprintf(buf, len, "%.1f MB", bytes / (1024.0 * 1024));
	}
	else if(bytes >= 1024) {
		snprintf(buf, len, "%.1f KB", bytes / 1024.0);
	}
	else {
		snprintf(buf, len, "%llu B", (unsigned long long)bytes);
	}
}

static const char *lba_mode_str(int mode) {

	switch(mode) {
		case 28:
			return "LBA28";
		case 48:
			return "LBA48";
		case 0:
			return "CHS";
		default:
			return "N/A";
	}
}

static int get_blockdev_size(kos_blockdev_t *bdev, uint64_t *size) {

	uint64_t blocks;

	if(!bdev || !bdev->count_blocks) {
		return -1;
	}

	blocks = bdev->count_blocks(bdev);

	if(!blocks) {
		return -1;
	}

	*size = blocks * (1ULL << bdev->l_block_size);
	return 0;
}

static int device_ui_ready(const device_ui_t *dev) {

	return dev != NULL
		&& dev->size_label != NULL
		&& dev->lba_label != NULL
		&& dev->iface_label != NULL
		&& dev->test_btn != NULL;
}

static void set_device_labels(device_ui_t *dev, const char *size, const char *lba, const char *iface) {

	char buf[128];

	if(!device_ui_ready(dev)) {
		return;
	}

	snprintf(buf, sizeof(buf), "Size: %s", size);
	GUI_LabelSetText(dev->size_label, buf);

	snprintf(buf, sizeof(buf), "Mode: %s", lba);
	GUI_LabelSetText(dev->lba_label, buf);

	snprintf(buf, sizeof(buf), "Interface: %s", iface);
	GUI_LabelSetText(dev->iface_label, buf);
}

static void update_test_btn(device_ui_t *dev, int hide, int enabled);

static void set_device_unavailable(device_ui_t *dev) {

	set_device_labels(dev, "N/A", "N/A", "N/A");
	update_test_btn(dev, 0, 0);
}

static void get_sd_interface(char *iface, size_t len) {

	if(sci_spi_rw_byte(0, NULL) != SCI_ERR_NOT_INITIALIZED) {
		snprintf(iface, len, "SCI-SPI");
	}
	else {
		snprintf(iface, len, "SCIF-SPI");
	}
}

static void update_sd_info(void) {

	char size_str[32];
	char iface[32];
	uint64_t size = 0;
	kos_blockdev_t bdev;

	if(!DirExists("/sd")) {
		set_device_unavailable(&self.sd);
		return;
	}

	if(sd_blockdev_for_device(&bdev) == 0) {
		get_blockdev_size(&bdev, &size);
		bdev.shutdown(&bdev);
	}

	format_size(size, size_str, sizeof(size_str));
	get_sd_interface(iface, sizeof(iface));
	set_device_labels(&self.sd, size_str, "Block", iface);
	update_test_btn(&self.sd, 0, 1);
}

static void update_ide_info(void) {

	char size_str[32];
	const char *lba;
	uint64_t size = 0;
	kos_blockdev_t bdev;
	int lba_mode = -1;

	if(!DirExists("/ide")) {
		set_device_unavailable(&self.ide);
		return;
	}

	if(g1_ata_blockdev_for_device(0, &bdev) == 0) {
		get_blockdev_size(&bdev, &size);
		bdev.shutdown(&bdev);
	}

	lba_mode = g1_ata_lba_mode();
	lba = lba_mode_str(lba_mode);
	format_size(size, size_str, sizeof(size_str));
	set_device_labels(&self.ide, size_str, lba, "G1 ATA slave");
	update_test_btn(&self.ide, 0, 1);
}

static void update_cd_info(void) {

	if(is_custom_bios()) {
		set_device_unavailable(&self.cd);
		return;
	}

	set_device_labels(&self.cd, "N/A", "LBA48", "G1 ATA master");
	update_test_btn(&self.cd, 0, 1);
}

static void show_dialog(GUI_DialogMode mode, const char *title, const char *body) {

	if(self.dialog == NULL) {
		return;
	}

	GUI_DialogShow(self.dialog, mode, title, body);
}

static void hide_dialog(void) {

	if(self.dialog == NULL) {
		return;
	}

	GUI_DialogHide(self.dialog);
}

static void show_progress(const char *step) {

	char body[256];

	snprintf(body, sizeof(body),
		"[align=center][size=20][b]%s[/b][/size][/align]", step);
	show_dialog(DIALOG_MODE_INFO, "Speed Test", body);
}

static int find_cd_test_file(void) {

	static const char *candidates[] = {
		"/cd/1DS_CORE.BIN",
		"/cd/1DS_BOOT.BIN",
		"/cd/1ST_READ.BIN",
		"/cd/0WINCEOS.BIN",
		NULL
	};
	int i;

	for(i = 0; candidates[i]; i++) {
		if(FileExists(candidates[i])) {
			strncpy(self.test_file, candidates[i], sizeof(self.test_file) - 1);
			self.test_file[sizeof(self.test_file) - 1] = '\0';
			return 0;
		}
	}

	return -1;
}

static void show_error(const char *msg) {

	char body[256];

	snprintf(body, sizeof(body),
		"[align=center][size=20][color=red][b]%s[/b][/color][/size][/align]", msg);
	show_dialog(DIALOG_MODE_ALERT, "Error", body);
}

static void show_results(const char *device) {

	char title[64];

	snprintf(title, sizeof(title), "Results: %s", device);
	show_dialog(DIALOG_MODE_ALERT, title, self.result_body);
}

static void append_result(const char *name, uint32_t tm, double speed, int size_kb, int buff_kb) {

	char line[256];
	size_t used = strlen(self.result_body);

	if(used >= sizeof(self.result_body) - 1) {
		return;
	}

	if(buff_kb > 0) {
		snprintf(line, sizeof(line),
			"[size=20][b]%s[/b][/size]\n"
			"[size=20][color=green][b]%.2f KB/s[/b][/color] "
			"[color=blue](%.2f Mbit/s)[/color][/size]\n"
			"[size=16]Time: %lu ms   Size: %d KB   Buffer: %d KB[/size]\n\n",
			name, speed / 1024, ((speed / 1024) / 1024) * 8,
			(unsigned long)tm, size_kb, buff_kb);
	}
	else {
		snprintf(line, sizeof(line),
			"[size=20][b]%s[/b][/size]\n"
			"[size=20][color=green][b]%.2f KB/s[/b][/color] "
			"[color=blue](%.2f Mbit/s)[/color][/size]\n"
			"[size=16]Time: %lu ms   Size: %d KB[/size]\n\n",
			name, speed / 1024, ((speed / 1024) / 1024) * 8,
			(unsigned long)tm, size_kb);
	}

	strncat(self.result_body, line, sizeof(self.result_body) - used - 1);
}

static void update_test_btn(device_ui_t *dev, int hide, int enabled) {

	if(dev == NULL || dev->test_btn == NULL) {
		return;
	}

	if(hide) {
		GUI_WidgetSetEnabled(dev->test_btn, 0);
		GUI_WidgetSetFlags(dev->test_btn, WIDGET_HIDDEN);
	}
	else {
		GUI_WidgetClearFlags(dev->test_btn, WIDGET_HIDDEN);
		GUI_WidgetSetEnabled(dev->test_btn, enabled);
	}
}

static void set_testing(int active) {

	self.testing = active;

	update_test_btn(&self.sd, active, !active && DirExists("/sd"));
	update_test_btn(&self.ide, active, !active && DirExists("/ide"));

	if(!is_custom_bios()) {
		update_test_btn(&self.cd, active, !active);
	}
	else {
		update_test_btn(&self.cd, active, 0);
	}
}

static int test_ide_io(void) {

	kos_blockdev_t bdev;
	uint64 st, et;
	uint32_t tm;
	uint8_t pt;
	uint8_t *buf;
	const size_t blocks = 4096;
	const size_t buf_size = blocks * 512;
	double speed;

	buf = memalign(32, buf_size);

	if(!buf) {
		return -1;
	}
	if(g1_ata_blockdev_for_partition(0, 1, &bdev, &pt)) {
		dbglog(DBG_DEBUG, "Couldn't get blockdev for partition!\n");
		free(buf);
		return -1;
	}

	show_progress("IO read test...");
	LockVideo();
	st = timer_ns_gettime64();

	if(bdev.read_blocks(&bdev, 0, blocks, buf)) {
		dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
		free(buf);
		UnlockVideo();
		return -1;
	}

	et = timer_ns_gettime64();
	tm = (et - st) / 1000000;
	free(buf);
	UnlockVideo();
	speed = buf_size / ((float)tm / 1000);

	append_result("IO read", tm, speed, buf_size / 1024, 0);

	ds_printf("DS_OK: Complete!\n"
		" Test: IO read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n",
		tm, speed / 1024,
		((speed / 1024) / 1024) * 8,
		buf_size / 1024);

	return 0;
}

static int test_sd_io(void) {

	kos_blockdev_t bdev;
	uint64 st, et;
	uint32_t tm;
	uint8_t pt;
	uint8_t *buf;
	const size_t blocks = 1024;
	const size_t buf_size = blocks * 512;
	double speed;

	buf = memalign(32, buf_size);

	if(!buf) {
		return -1;
	}
	if(sd_blockdev_for_partition(0, &bdev, &pt)) {
		dbglog(DBG_DEBUG, "Couldn't get blockdev for partition!\n");
		free(buf);
		return -1;
	}

	show_progress("SD IO read test...");
	thd_sleep(100);

	LockVideo();
	st = timer_ns_gettime64();

	if(bdev.read_blocks(&bdev, 0, blocks, buf)) {
		dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
		free(buf);
		UnlockVideo();
		return -1;
	}

	et = timer_ns_gettime64();
	tm = (et - st) / 1000000;
	free(buf);
	UnlockVideo();
	speed = buf_size / ((float)tm / 1000);

	append_result("SD IO read", tm, speed, buf_size / 1024, 0);

	ds_printf("DS_OK: Complete!\n"
		" Test: SD IO read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n",
		tm, speed / 1024,
		((speed / 1024) / 1024) * 8,
		buf_size / 1024);

	return 0;
}

static void run_speed_test(void) {

	uint8 *buff = NULL;
	size_t buff_size = 0x40000;
	int size = self.test_size;
	int cnt = 0, rs;
	int64 time_before, time_after;
	uint32 t;
	double speed;
	file_t fd;

	if(self.test_skip_write) {
		goto readtest;
	}

	show_progress("File system write test...");

	if(FileExists(self.test_file)) {
		fs_unlink(self.test_file);
	}

	fd = fs_open(self.test_file, O_WRONLY | O_CREAT | O_TRUNC);

	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for write: %d\n", self.test_file, errno);
		show_error("Can't open file for write");
		return;
	}

	buff = (uint8 *)0x8c400000;

	thd_sleep(100);
	LockVideo();
	time_before = timer_ns_gettime64();

	while(cnt < size) {

		rs = fs_write(fd, buff, buff_size);

		if(rs <= 0) {
			fs_close(fd);
			UnlockVideo();
			ds_printf("DS_ERROR: Can't write to file: %d\n", errno);
			show_error("Can't write to file");
			return;
		}
		buff += rs;
		cnt += rs;
	}

	time_after = timer_ns_gettime64();
	UnlockVideo();

	t = (time_after - time_before) / 1000000;
	speed = size / ((float)t / 1000);
	fs_close(fd);

	append_result("File system write", t, speed, size / 1024, buff_size / 1024);

	ds_printf("DS_OK: Complete!\n"
		" Test: File system write\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n Buff: %d Kb\n",
		t, speed / 1024,
		((speed / 1024) / 1024) * 8,
		size / 1024, buff_size / 1024);

readtest:

	show_progress("File system read test...");

	fd = fs_open(self.test_file, O_RDONLY);

	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for read: %d\n", self.test_file, errno);
		show_error("Can't open file for read");
		return;
	}
	if(self.test_read_only) {
		fs_ioctl(fd, 0, NULL);
		fs_close(fd);
		fd = fs_open(self.test_file, O_RDONLY);
	}

	time_before = time_after = t = cnt = 0;
	speed = 0.0f;
	size = fs_total(fd);
	buff = memalign(32, buff_size);

	thd_sleep(100);
	LockVideo();
	time_before = timer_ns_gettime64();

	while(cnt < size) {

		rs = fs_read(fd, buff, buff_size);

		if(rs <= 0) {
			fs_close(fd);
			free(buff);
			UnlockVideo();
			ds_printf("DS_ERROR: Can't read file: %d\n", errno);
			show_error("Can't read file");
			return;
		}
		cnt += rs;
	}

	time_after = timer_ns_gettime64();
	t = (uint32)(time_after - time_before) / 1000000;
	speed = size / ((float)t / 1000);

	fs_close(fd);

	if(!self.test_read_only) {
		fs_unlink(self.test_file);
	}

	UnlockVideo();
	free(buff);

	append_result("File system read", t, speed, size / 1024, buff_size / 1024);

	ds_printf("DS_OK: Complete!\n"
		" Test: File system read\n Time: %ld ms\n"
		" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
		" Size: %d Kb\n Buff: %d Kb\n",
		t, speed / 1024,
		((speed / 1024) / 1024) * 8,
		size / 1024, buff_size / 1024);

	if(self.test_is_ide) {
		test_ide_io();
	}
	else if(self.test_is_sd) {
		test_sd_io();
	}

	show_results(self.test_device);
}

void Speedtest_Run(GUI_Widget *widget) {

	const char *wname = GUI_ObjectGetName((GUI_Object *)widget);

	if(self.testing) {
		return;
	}

	strncpy(self.test_wname, wname, sizeof(self.test_wname) - 1);
	self.test_wname[sizeof(self.test_wname) - 1] = '\0';
	self.test_read_only = 0;
	self.test_is_ide = 0;
	self.test_is_sd = 0;
	self.test_skip_write = 0;
	self.test_size = 0x800000;
	strncpy(self.test_device, wname, sizeof(self.test_device) - 1);
	self.test_device[sizeof(self.test_device) - 1] = '\0';
	self.test_file[0] = '\0';

	if(!strncmp(wname, "/ide", 4)) {
		self.test_is_ide = 1;
		strncpy(self.test_device, "IDE", sizeof(self.test_device) - 1);
	}
	else if(!strncmp(wname, "/sd", 3)) {
		self.test_is_sd = 1;
		self.test_size >>= 2;
		strncpy(self.test_device, "SD", sizeof(self.test_device) - 1);
	}
	else if(!strncmp(wname, "/cd", 3)) {
		self.test_read_only = 1;
		self.test_skip_write = 1;
		strncpy(self.test_device, "GD-ROM", sizeof(self.test_device) - 1);

		if(find_cd_test_file() < 0) {
			show_error("Test file not found on disc");
			return;
		}
	}

	if(!self.test_skip_write) {
		snprintf(self.test_file, sizeof(self.test_file), "%s/%s.tst", wname, lib_get_name());
	}

	self.result_body[0] = '\0';
	set_testing(1);

	run_speed_test();
}

void Speedtest_DialogConfirm(GUI_Widget *widget) {

	(void)widget;
	hide_dialog();
	set_testing(0);
}

static int load_device_ui(device_ui_t *dev, const char *prefix) {

	char name[32];

	snprintf(name, sizeof(name), "dev-%s", prefix);
	dev->card = APP_GET_WIDGET(name);

	snprintf(name, sizeof(name), "%s-size", prefix);
	dev->size_label = APP_GET_WIDGET(name);

	snprintf(name, sizeof(name), "%s-lba", prefix);
	dev->lba_label = APP_GET_WIDGET(name);

	snprintf(name, sizeof(name), "%s-iface", prefix);
	dev->iface_label = APP_GET_WIDGET(name);

	snprintf(name, sizeof(name), "/%s", prefix);
	dev->test_btn = APP_GET_WIDGET(name);

	if(!device_ui_ready(dev)) {
		ds_printf("DS_ERROR: Speedtest: Missing UI widgets for '%s'\n", prefix);
		return 0;
	}

	return 1;
}

static void refresh_devices(void) {

	update_sd_info();
	update_ide_info();
	update_cd_info();
}

void Speedtest_Init(App_t *app) {

	memset(&self, 0, sizeof(self));

	if(app == NULL) {
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n",
					lib_get_name(), __func__);
		return;
	}

	self.app = app;
	self.dialog = APP_GET_WIDGET("results-dialog");

	if(self.dialog == NULL) {
		ds_printf("DS_ERROR: Speedtest: Missing dialog widget 'results-dialog'\n");
	}

	load_device_ui(&self.sd, "sd");
	load_device_ui(&self.ide, "ide");
	load_device_ui(&self.cd, "cd");
}

void Speedtest_Open(App_t *app) {

	if(app != NULL) {
		self.app = app;
	}

	refresh_devices();
}
