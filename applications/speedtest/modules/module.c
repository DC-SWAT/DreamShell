/* DreamShell ##version##
	
	speedtest.c
	Copyright (C) 2011-2014 SWAT
	
	module.c - Speedtest app module
	Copyright (C)2014 megavolt85
	
	http://www.dc-swat.ru
*/

#include "ds.h"

DEFAULT_MODULE_EXPORTS(app_speedtest);

static void *getElement(char *name, ListItemType type);
#define APP_GET_WIDGET(name)  ((GUI_Widget *)  getElement(name, LIST_ITEM_GUI_WIDGET))

static struct self {
		App_t *app;
		GUI_Widget *cd_c;
		GUI_Widget *sd_c;
		GUI_Widget *hdd_c;
		GUI_Widget *pc_c;
		GUI_Widget *speedww;
		GUI_Widget *speedrw;
		GUI_Widget *status_text;
} self;

void Speedtest_Run(GUI_Widget *widget) {

	uint8 *buff;
	size_t buff_size = 32*1024;
	int size, cnt, rs; 
	int64 time_before, time_after;
	uint32 t;
	double speed;
	file_t fd;
	char name[MAX_FN_LEN / 2];
	char readresult[MAX_FN_LEN / 2];
	char writeresult[MAX_FN_LEN / 2];
	
	if(!strncasecmp(GUI_ObjectGetName( (GUI_Object *)widget), "/cd/", 4)){
		cdrom_reinit();
		if(FileExists("/cd/1DS_CORE.BIN")) {
			snprintf(name, MAX_FN_LEN/2,"%s1DS_CORE.BIN", GUI_ObjectGetName( (GUI_Object *)widget));
			goto readtest;
		} else if(FileExists("/cd/1ST_READ.BIN")) {
			snprintf(name, MAX_FN_LEN/2,"%s1ST_READ.BIN", GUI_ObjectGetName( (GUI_Object *)widget));
			goto readtest;
		} else {
			ds_printf("DS_ERROR: Can't open file for read");
			GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
			GUI_LabelSetText(self.status_text, "Can't open file for read"); 
			return;
		}
	}
	
	GUI_LabelSetTextColor(self.status_text, 28, 227, 70);
	GUI_LabelSetText(self.status_text, "Testing WRITE speed...");
	
	snprintf(name, MAX_FN_LEN/2,"%sspeedtest.testfile", GUI_ObjectGetName( (GUI_Object *)widget));
	if(FileExists(name)) fs_unlink(name);
	
					/* WRITE TEST */
	
	if ((fd = fs_open(name, O_CREAT | O_WRONLY)) == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for write: %d\n", name, errno);
		GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
		GUI_LabelSetText(self.status_text, "Can't open file for write"); 
		return;
	}

	buff = (uint8*)0x8c000000;
	size = 16*1024*1024;
	cnt = 0;

	ShutdownVideoThread(); 
	time_before = timer_ms_gettime64();
	
	while(cnt < size) {
		
		rs = fs_write(fd, buff, buff_size);
		
		if(rs < 0) {
			fs_close(fd);
			InitVideoThread(); 
			ds_printf("DS_ERROR: Can't write to file: %d\n", errno);
			GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
			GUI_LabelSetText(self.status_text, "Can't write to file"); 
			return;
		}
		buff += rs;
		cnt += rs;
	}
	time_after = timer_ms_gettime64();
	InitVideoThread();

	t = (uint32)(time_after - time_before);
	speed = size / ((float)t / 1000);
	fs_close(fd);
	
	snprintf(writeresult,MAX_FN_LEN/2,"Write speed %.2f Kbytes/s (%.2f Mbit/s) Time: %ld ms",speed / 1024,((speed / 1024) / 1024) * 8,t);
	ds_printf("DS_OK: Complete!\n"
				" Test: write\n Time: %ld ms\n"
				" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
				" Size: %d Kb\n Buff: %d Kb\n", 
				t, speed / 1024, 
				((speed / 1024) / 1024) * 8, 
				size / 1024, buff_size / 1024); 
				
	GUI_LabelSetText(self.speedww, writeresult);
	
					/* READ TEST */
readtest:	
	if(!strncasecmp(GUI_ObjectGetName( (GUI_Object *)widget), "/cd/", 4)) {
		
		if ((fd = fs_open(name, O_RDONLY) == FILEHND_INVALID)) {
			ds_printf("DS_ERROR: Can't open %s for read: %d\n", name, errno);
			GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
			GUI_LabelSetText(self.status_text, "Can't open file for read"); 
			return;
		}
		fs_ioctl(fd,NULL,0);
		fs_close(fd);
	}
	
	thd_sleep(500);
	GUI_LabelSetText(self.status_text, "Testing READ speed...");
	
	time_before = time_after = t = 0;
	speed = 0.0f;
	
	if ((fd = fs_open(name, O_RDONLY) == FILEHND_INVALID)) {
		ds_printf("DS_ERROR: Can't open %s for read: %d\n", name, errno);
		GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
		GUI_LabelSetText(self.status_text, "Can't open file for read"); 
		return;
	}
	
	cnt = 0;
	size = fs_total(fd);
	buff = (uint8 *) malloc(buff_size);

	if(buff == NULL) {
		fs_close(fd);
		ds_printf("DS_ERROR: No free memory\n");
		GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
		GUI_LabelSetText(self.status_text, "No free memory");
		if(strncasecmp(GUI_ObjectGetName( (GUI_Object *)widget), "/cd/", 4)) { 
			fs_unlink(name);
		}
		return;
	}

	ShutdownVideoThread();
	time_before = timer_ms_gettime64();

	while(cnt < size) {
		
		rs = fs_read(fd, buff, buff_size);
		
		if(rs < 0) {
			fs_close(fd);
			free(buff);
			InitVideoThread(); 
			ds_printf("DS_ERROR: Can't read file: %d\n", errno);
			GUI_LabelSetTextColor(self.status_text, 255, 0, 0);
			GUI_LabelSetText(self.status_text, "Can't read file"); 
			return;
		}
		cnt += rs;
	}
	
	time_after = timer_ms_gettime64();
	InitVideoThread();

	t = (uint32)(time_after - time_before);
	speed = size / ((float)t / 1000);

	free(buff);
	fs_close(fd);
	if(strncasecmp(GUI_ObjectGetName( (GUI_Object *)widget), "/cd/", 4)){ 
		fs_unlink(name);
	} else cdrom_spin_down();
	
	snprintf(readresult,MAX_FN_LEN/2,"Read speed %.2f Kbytes/s (%.2f Mbit/s) Time: %ld ms",speed / 1024,((speed / 1024) / 1024) * 8,t);
	
	ds_printf("DS_OK: Complete!\n"
				" Test: read\n Time: %ld ms\n"
				" Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
				" Size: %d Kb\n Buff: %d Kb\n", 
				t, speed / 1024, 
				((speed / 1024) / 1024) * 8, 
				size / 1024, buff_size / 1024);
    
    GUI_LabelSetTextColor(self.status_text, 28, 227, 70);
    GUI_LabelSetText(self.status_text, "Complete!");
    GUI_LabelSetText(self.speedrw, readresult);
}


void Speedtest_Init(App_t *app) {
	
	self.app = app;

	if(self.app != NULL) {

		self.speedrw = APP_GET_WIDGET("speedr_text");
		self.speedww = APP_GET_WIDGET("speedw_text");
		self.status_text = APP_GET_WIDGET("status_text");
		self.cd_c = APP_GET_WIDGET("/cd/");
		self.sd_c = APP_GET_WIDGET("/sd/");
		self.hdd_c = APP_GET_WIDGET("/ide/");
		self.pc_c = APP_GET_WIDGET("/pc/");
		
		cdrom_reinit();
		
		if(!DirExists("/pc")) GUI_WidgetSetEnabled(self.pc_c, 0);
		if(!DirExists("/sd")) GUI_WidgetSetEnabled(self.sd_c, 0);
		if(!DirExists("/cd")) GUI_WidgetSetEnabled(self.cd_c, 0);
		if(!DirExists("/ide")) GUI_WidgetSetEnabled(self.hdd_c, 0);
	} else ds_printf("DS_ERROR: %s: Can't find app named:\n", lib_get_name());
}

static void *getElement(char *name, ListItemType type) {
	
	Item_t *item;
	
	switch(type) {
		case LIST_ITEM_GUI_WIDGET:
			item = listGetItemByName(self.app->elements, name);
			break;
		case LIST_ITEM_GUI_SURFACE:
		case LIST_ITEM_GUI_FONT:
		default:
			item = listGetItemByName(self.app->resources, name);
			break;
	}

	if(item != NULL && item->type == type) {
		return item->data;
	}

	ds_printf("DS_ERROR: %s: Couldn't find or wrong type '%s'\n", 
				lib_get_name(), name, self.app->name);
	return NULL;
}
