/* DreamShell ##version##

   module.c - ISO Loader App module
   Copyright (C) 2011 Superdefault
   Copyright (C) 2011-2014 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include <stdbool.h>

DEFAULT_MODULE_EXPORTS(app_iso_loader);

/* Resource helpers */
static void *getElement(char *name, ListItemType type);
#define APP_GET_WIDGET(name)  ((GUI_Widget *)  getElement(name, LIST_ITEM_GUI_WIDGET))
#define APP_GET_SURFACE(name) ((GUI_Surface *) getElement(name, LIST_ITEM_GUI_SURFACE))
#define APP_GET_FONT(name)    ((GUI_Font *)    getElement(name, LIST_ITEM_GUI_FONT))

/* Indexes of devices  */
#define APP_DEVICE_CD    0
#define APP_DEVICE_SD    1
#define APP_DEVICE_IDE   2
#define APP_DEVICE_PC    3
//#define APP_DEVICE_NET   4
#define APP_DEVICE_COUNT 4

static struct {

	App_t *app;
	isoldr_info_t *isoldr;
	char filename[MAX_FN_LEN];
	bool have_args;
	bool used_preset;
	
	GUI_Widget *pages;
	
	GUI_Widget *filebrowser;
	int current_item;
	GUI_Surface *item_norm;
	GUI_Surface *item_focus;
	GUI_Surface *item_selected;

	GUI_Widget *settings;
	GUI_Widget *games;
	GUI_Widget *btn_run;

	GUI_Widget *async;
	GUI_Widget *async_sectors;
	GUI_Widget *dma;
	GUI_Widget *device;
	GUI_Widget *os_chk[3];
	GUI_Widget *boot_mode_chk[3];
	GUI_Widget *boot_method_chk[2];
	GUI_Widget *memory_chk[10];
	GUI_Widget *memory_text;

	int current_dev;
	GUI_Widget  *btn_dev[APP_DEVICE_COUNT];
	GUI_Surface *btn_dev_norm[APP_DEVICE_COUNT];
	GUI_Surface *btn_dev_over[APP_DEVICE_COUNT];

	GUI_Surface *default_cover;
	GUI_Surface *current_cover;
	GUI_Widget *cover_widget;

} self;


void isoLoader_DefaultPreset();
int isoLoader_LoadPreset();
int isoLoader_SavePreset();


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


void isoLoader_ShowSettings(GUI_Widget *widget) {
	
	(void)widget;
	
	ScreenFadeOut();
	thd_sleep(200);
	
	GUI_CardStackShowIndex(self.pages,  1);
	GUI_WidgetSetEnabled(self.games,    1);
	GUI_WidgetSetEnabled(self.settings, 0);
	
	ScreenFadeIn();
}


void isoLoader_ShowGames(GUI_Widget *widget) {
	
	(void)widget;
	
	ScreenFadeOut();
	thd_sleep(200);
	
	GUI_CardStackShowIndex(self.pages,  0);
	GUI_WidgetSetEnabled(self.settings, 1);
	GUI_WidgetSetEnabled(self.games,    0);
	
	isoLoader_SavePreset();
	ScreenFadeIn();
}


static int getDeviceType() {
	
	const char *dir = GUI_FileManagerGetPath(self.filebrowser);

	if(!strncasecmp(dir, "/cd", 3)) {
		return APP_DEVICE_CD;
	} else if(!strncasecmp(dir, "/sd",   3)) {
		return APP_DEVICE_SD;
	} else if(!strncasecmp(dir, "/ide",  4)) {
		return APP_DEVICE_IDE;
	} else if(!strncasecmp(dir, "/pc",   3)) {
		return APP_DEVICE_PC;
//	} else if(!strncasecmp(dir, "/???", 5)) {
//		return APP_DEVICE_NET;
	} else {
		return -1;
	}
}

/* Set the active volume in the menu */
static void highliteDevice() {
	
	int dev = getDeviceType();
	
	if(dev > -1 && dev != self.current_dev) {
		
		if(self.current_dev > -1) {
			GUI_ButtonSetNormalImage(self.btn_dev[self.current_dev], self.btn_dev_norm[self.current_dev]);
		}
		
		GUI_ButtonSetNormalImage(self.btn_dev[dev], self.btn_dev_over[dev]);
		self.current_dev = dev;
		UpdateActiveMouseCursor();
	}
}


/* Try to get cover image from ISO */
static void showCover() {
	
	GUI_Surface *s;
	char path[MAX_FN_LEN];
	char noext[MAX_FN_LEN];
	
	strncpy(noext, self.filename, MAX_FN_LEN);
	snprintf(path, MAX_FN_LEN, "%s/apps/iso_loader/covers/%s.jpg", getenv("PATH"), strtok(noext, "."));

	/* Check for jpeg cover */
	if(FileExists(path)) {
		
		s = GUI_SurfaceLoad(path);
		
		if(s != NULL) {
			GUI_PanelSetBackground(self.cover_widget, s);
			GUI_ObjectDecRef((GUI_Object *) s);
			self.current_cover = s;
		}

	} else {

		snprintf(path, MAX_FN_LEN, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), self.filename);
		LockVideo();

		/* Try to mount ISO and get 0GDTEXT.PVR */
		if(!fs_iso_mount("/isocover", path)) {
			
			if(FileExists("/isocover/0GDTEX.PVR")) {
				
				s = GUI_SurfaceLoad("/isocover/0GDTEX.PVR");
				fs_iso_unmount("/isocover");
				UnlockVideo();
				
				if(s != NULL) {
					GUI_PanelSetBackground(self.cover_widget, s);
					GUI_ObjectDecRef((GUI_Object *) s);
					self.current_cover = s;
				} else {
					goto check_default;
				}
				
			} else {
				fs_iso_unmount("/isocover");
				UnlockVideo();
			}
			
		} else {
			UnlockVideo();
			goto check_default;
		}
	}
	
	return;
	
check_default:
	if(self.current_cover != self.default_cover) {
		GUI_PanelSetBackground(self.cover_widget, self.default_cover);
		self.current_cover = self.default_cover;
	}
}

/* Switch to the selected volume */
void isoLoader_SwitchVolume(void *dir) {
	GUI_FileManagerSetPath(self.filebrowser, (char *)dir);
	highliteDevice();
}

void isoLoader_toggleOS(GUI_Widget *widget) {
	for(int i = 0; i < 3; i++) {
		if(widget != self.os_chk[i]) {
			GUI_WidgetSetState(self.os_chk[i], 0);
		}
	}
}

void isoLoader_toggleBootMode(GUI_Widget *widget) {
	
	for(int i = 0; i < 3; i++) {
		if(widget != self.boot_mode_chk[i]) {
			GUI_WidgetSetState(self.boot_mode_chk[i], 0);
		}
	}
	
	if(widget != self.boot_mode_chk[BOOT_MODE_DIRECT]) {
		
		if(GUI_WidgetGetState(self.memory_chk[0])) {
			
			int i;
			for(i = 0; self.memory_chk[i]; i++) {
				GUI_WidgetSetState(self.memory_chk[i], 0);
			}
			
			GUI_WidgetSetState(self.memory_chk[i-4], 1);
		}
		
	} else {
		
		GUI_WidgetSetState(self.memory_chk[0], 1);
		
		for(int i = 1; self.memory_chk[i]; i++) {
			GUI_WidgetSetState(self.memory_chk[i], 0);
		}
	}
}

void isoLoader_toggleBootMethod(GUI_Widget *widget) {
	for(int i = 0; i < 2; i++) {
		if(widget != self.boot_method_chk[i]) {
			GUI_WidgetSetState(self.boot_method_chk[i], 0);
		}
	}
}

void isoLoader_toggleMemory(GUI_Widget *widget) {
	
	int i;
	
	for(i = 0; self.memory_chk[i]; i++) {
		if(widget != self.memory_chk[i]) {
			GUI_WidgetSetState(self.memory_chk[i], 0);
		}
	}
	
	if(!strncasecmp(GUI_ObjectGetName((GUI_Object *) widget), "memory_text", 12)) {
		GUI_WidgetSetState(self.memory_chk[i-1], 1);
	}
}


void isoLoader_Run(GUI_Widget *widget) {
	
	char filepath[MAX_FN_LEN];
	const char *tmpval;
	uint32 addr = ISOLDR_DEFAULT_ADDR_LOW;
	(void)widget;
	
	memset(filepath, 0, MAX_FN_LEN);
	snprintf(filepath, MAX_FN_LEN, "%s/%s", 
				GUI_FileManagerGetPath(self.filebrowser), 
				self.filename);
				
	ScreenFadeOut();
	thd_sleep(400);

	self.isoldr = isoldr_get_info(filepath, 0);
	
	if(self.isoldr == NULL) {
		ShowConsole();
		ScreenFadeIn();
		return;
	}
	
	if(GUI_WidgetGetState(self.async)) {
		self.isoldr->emu_async = atoi(GUI_TextEntryGetText(self.async_sectors));
	}
	
	if(GUI_WidgetGetState(self.dma)) {
		self.isoldr->use_dma = 1;
	}
	
	tmpval = GUI_TextEntryGetText(self.device);
	
	if(strncasecmp(tmpval, "auto", 4)) {
		strncpy(self.isoldr->fs_dev, tmpval, sizeof(self.isoldr->fs_dev));
	}
	
	if(self.current_cover && self.current_cover != self.default_cover) {
		
		SDL_Surface *surf = GUI_SurfaceGet(self.current_cover);
		
		if(surf) {
			self.isoldr->gdtex = (uint32)surf->pixels;
		}
	}
	
	for(int i = 0; i < 4; i++) {
		if(i && GUI_WidgetGetState(self.os_chk[i])) {
			self.isoldr->exec.type = i;
			break;
		}
	}
	
	for(int i = 0; i < 3; i++) {
		if(i && GUI_WidgetGetState(self.boot_mode_chk[i])) {
			self.isoldr->boot_mode = i;
			break;
		}
	}
	
	for(int i = 0; i < 2; i++) {
		if(i && GUI_WidgetGetState(self.boot_method_chk[i])) {
			self.isoldr->boot_method = i;
			break;
		}
	}
	
	for(int i = 0; self.memory_chk[i]; i++) {
		
		if(GUI_WidgetGetState(self.memory_chk[i])) {
			
			tmpval = GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);
			
			if(strlen(tmpval) < 8) {
				char text[24];
				memset(text, 0, sizeof(text));
				strncpy(text, tmpval, 10);
				tmpval = strncat(text, GUI_TextEntryGetText(self.memory_text), 10);
			}

			addr = strtoul(tmpval, NULL, 16);
			break;
		}
	}
	
	if(!strncasecmp(self.isoldr->fs_dev, ISOLDR_DEV_DCLOAD, 4) && addr < 0x8c010000) {
		addr = ISOLDR_DEFAULT_ADDR_HIGH;
		ds_printf("DS_WARNING: Using dc-load as file system, forced loader address: 0x%08lx\n", addr);
	}
	
	if(!strncasecmp(self.isoldr->fs_dev, ISOLDR_DEV_DCIO, 4)) {
		isoldr_exec_dcio(self.isoldr, filepath);
	} else {
		isoldr_exec(self.isoldr, addr);
	}
	
	/* If we there, then something wrong... */
	ShowConsole();
	ScreenFadeIn();
	free(self.isoldr);
}


void isoLoader_ItemClick(dirent_fm_t *fm_ent) {

	dirent_t *ent = &fm_ent->ent;

	if(ent->attr == O_DIR) {
		
		self.current_item = -1;
		memset(self.filename, 0, MAX_FN_LEN);
		GUI_FileManagerChangeDir(self.filebrowser, ent->name, ent->size);
		highliteDevice();
		GUI_WidgetSetEnabled(self.btn_run, 0);
		
	} else if(fm_ent && self.current_item == fm_ent->index) {
		
		isoLoader_Run(NULL);
		
	} else if(IsFileSupportedByApp(self.app, ent->name)) {
		
		GUI_Widget *w;
		GUI_WidgetSetEnabled(self.btn_run, 1);
		
		w = GUI_FileManagerGetItem(self.filebrowser, fm_ent->index);
		GUI_ButtonSetNormalImage(w, self.item_selected);
		GUI_ButtonSetHighlightImage(w, self.item_selected);
		GUI_ButtonSetPressedImage(w, self.item_selected);
		
		if(self.current_item > -1) {
			w = GUI_FileManagerGetItem(self.filebrowser, self.current_item);
			GUI_ButtonSetNormalImage(w, self.item_norm);
			GUI_ButtonSetHighlightImage(w, self.item_focus);
			GUI_ButtonSetPressedImage(w, self.item_focus);
		}
	
		self.current_item = fm_ent->index;
		strncpy(self.filename, ent->name, MAX_FN_LEN);
		highliteDevice();
		showCover();
		isoLoader_LoadPreset();
	}
}


static char *makePresetFilename() {
	
	if(!self.filename[0]) {
		return NULL;
	}
	
	char fn[MAX_FN_LEN];
	static char filename[MAX_FN_LEN * 2];
	memset(filename, 0, sizeof(filename));
	
	snprintf(filename, sizeof(filename), "%s/%s", 
				GUI_FileManagerGetPath(self.filebrowser), 
				self.filename);
	
	strncpy(fn, filename + 1, sizeof(fn));
	int len = strlen(fn);
	
	for(int i = 0; i < len; i++) {
		if(fn[i] == '/' || fn[i] == '.' || fn[i] == ' ') {
			fn[i] = '_';
		}
	}
	
	memset(filename, 0, MAX_FN_LEN);
	snprintf(filename, sizeof(filename), "%s/apps/%s/presets/%s.pst", getenv("PATH"), lib_get_name() + 4, fn);
	return filename;
}


void isoLoader_DefaultPreset() {
	
	if(self.used_preset == true) {
		GUI_WidgetSetState(self.async, 1);
		GUI_TextEntrySetText(self.async_sectors, "8");
		GUI_WidgetSetState(self.dma, 0);
		GUI_TextEntrySetText(self.device, "auto");
		
		GUI_WidgetSetState(self.os_chk[0], 1);
		isoLoader_toggleOS(self.os_chk[0]);
		
		GUI_WidgetSetState(self.boot_mode_chk[0], 1);
		isoLoader_toggleBootMode(self.boot_mode_chk[0]);
		
		GUI_WidgetSetState(self.boot_method_chk[0], 1);
		isoLoader_toggleBootMethod(self.boot_method_chk[0]);
		
		GUI_WidgetSetState(self.memory_chk[0], 1);
		isoLoader_toggleMemory(self.memory_chk[0]);
		self.used_preset = false;
	}
}


int isoLoader_SavePreset() {

	if(!GUI_WidgetGetState(APP_GET_WIDGET("preset-checkbox"))) {
		return 0;
	}

	char *filename;
	FILE *fp;
	
	filename = makePresetFilename();
	
	if(filename == NULL) {
		return -1;
	}
	
	fp = fopen(filename, "w");
	
	if(fp == NULL) {
		return -1;
	}
	
	fprintf(fp, "dma = %d\n", GUI_WidgetGetState(self.dma));
	
	if(GUI_WidgetGetState(self.async)) {
		fprintf(fp, "async = %s\n", GUI_TextEntryGetText(self.async_sectors));
	} else {
		fputs("async = 0\n", fp);
	}
	
	fprintf(fp, "device = %s\n", GUI_TextEntryGetText(self.device));
	
	for(int i = 0; i < 4; i++) {
		if(GUI_WidgetGetState(self.os_chk[i])) {
			fprintf(fp, "type = %d\n", i);
			break;
		}
	}
	
	for(int i = 0; i < 3; i++) {
		if(GUI_WidgetGetState(self.boot_mode_chk[i])) {
			fprintf(fp, "mode = %d\n", i);
			break;
		}
	}
	
	for(int i = 0; i < 2; i++) {
		if(GUI_WidgetGetState(self.boot_method_chk[i])) {
			fprintf(fp, "method = %d\n", i);
			break;
		}
	}
	
	for(int i = 0; self.memory_chk[i]; i++) {
		
		if(GUI_WidgetGetState(self.memory_chk[i])) {
			
			char *tmpval = (char* )GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);
			
			if(strlen(tmpval) < 8) {
				char text[24];
				memset(text, 0, sizeof(text));
				strncpy(text, tmpval, 10);
				tmpval = strncat(text, GUI_TextEntryGetText(self.memory_text), 10);
			}

			fprintf(fp, "memory = %s\n", tmpval);
			break;
		}
	}
	
	fclose(fp);
	self.used_preset = true;
	return 0;
}


int isoLoader_LoadPreset() {

	char *filename = makePresetFilename();
	
	if(filename == NULL || !FileExists(filename)) {
		isoLoader_DefaultPreset();
		return -1;
	}
	
	uint32 lex = 0;
	char *device = NULL;
	int use_dma = 0, emu_async = 0;
	int boot_mode = BOOT_MODE_DIRECT;
	int boot_method = BOOT_METHOD_04X;
	int bin_type = BIN_TYPE_AUTO;
	CFG_CONTEXT con;
	
	struct cfg_option options[] = {
		{NULL, '\0', "dma",    CFG_INT,   (void *) &use_dma,     0},
		{NULL, '\0', "device", CFG_STR,   (void *) &device,      0},
		{NULL, '\0', "memory", CFG_ULONG, (void *) &lex,         0},
		{NULL, '\0', "async",  CFG_INT,   (void *) &emu_async,   0},
		{NULL, '\0', "mode",   CFG_INT,   (void *) &boot_mode,   0},
		{NULL, '\0', "method", CFG_INT,   (void *) &boot_method, 0},
		{NULL, '\0', "type",   CFG_INT,   (void *) &bin_type,    0},
		CFG_END_OF_LIST
	};
	
	con = cfg_get_context(options);
	cfg_set_context_flag(con, CFG_IGNORE_UNKNOWN);

	if(con == NULL) {
		ds_printf("DS_ERROR: Not enough memory\n");
		isoLoader_DefaultPreset();
		return -1;	
	}

	cfg_set_cfgfile_context(con, 0, -1, (char* )filename);

	if(cfg_parse(con) != CFG_OK) {
		ds_printf("DS_ERROR: %s\n", cfg_get_error_str(con));	
		isoLoader_DefaultPreset();
		return -1;	
	}

	cfg_free_context(con);
	
	if(emu_async) {
		char sv[8]; memset(sv, 0, sizeof(sv));
		snprintf(sv, sizeof(sv), "%d", emu_async);
		GUI_WidgetSetState(self.async, 1);
		GUI_TextEntrySetText(self.async_sectors, sv);
	}
	
	GUI_WidgetSetState(self.dma, use_dma);
	GUI_TextEntrySetText(self.device, device);
	
	GUI_WidgetSetState(self.os_chk[bin_type], 1);
	isoLoader_toggleOS(self.os_chk[bin_type]);
	
	GUI_WidgetSetState(self.boot_mode_chk[boot_mode], 1);
	isoLoader_toggleBootMode(self.boot_mode_chk[boot_mode]);
	
	GUI_WidgetSetState(self.boot_method_chk[boot_method], 1);
	isoLoader_toggleBootMethod(self.boot_method_chk[boot_method]);
	
	if(lex) {
		
		int i;
		char *name, memory[12];
		memset(memory, 0, sizeof(memory));
		snprintf(memory, sizeof(memory), "0x%08lx", lex);
		
		for(i = 0; self.memory_chk[i]; i++) {
			
			name = (char* )GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);
			
			if(!strncasecmp(name, memory, sizeof(memory))) {
				GUI_WidgetSetState(self.memory_chk[i], 1);
				isoLoader_toggleMemory(self.memory_chk[i]);
				break;
			}
		}
		
		if(i > 8) {
			GUI_TextEntrySetText(self.memory_text, memory + 4);
			GUI_WidgetSetState(self.memory_chk[8], 1);
			isoLoader_toggleMemory(self.memory_chk[8]);
		}
	}

	self.used_preset = true;
	return 0;
}


//static void isoLoader_InputEvent(void *ds_event, void *param, int action) {
//	SDL_Event *event = (SDL_Event *) param;
//}


void isoLoader_Init(App_t *app) {

	GUI_Widget *w, *b;
	GUI_Callback *cb;

	if(app != NULL) {

		memset(&self, 0, sizeof(self));
		
		self.app = app;
		self.current_dev = -1;
		self.used_preset = false;

		self.btn_dev[APP_DEVICE_CD]  = APP_GET_WIDGET("btn_cd");
		self.btn_dev[APP_DEVICE_SD]  = APP_GET_WIDGET("btn_sd");
		self.btn_dev[APP_DEVICE_IDE] = APP_GET_WIDGET("btn_hdd");
		self.btn_dev[APP_DEVICE_PC]  = APP_GET_WIDGET("btn_pc");
//		self.btn_dev[APP_DEVICE_NET] = APP_GET_WIDGET("btn_net");

		self.item_norm     = APP_GET_SURFACE("item-normal");
		self.item_focus    = APP_GET_SURFACE("item-focus");
		self.item_selected = APP_GET_SURFACE("item-selected");

		self.btn_dev_norm[APP_DEVICE_CD]  = APP_GET_SURFACE("btn_cd_norm");
		self.btn_dev_over[APP_DEVICE_CD]  = APP_GET_SURFACE("btn_cd_over");
		
		self.btn_dev_norm[APP_DEVICE_SD]  = APP_GET_SURFACE("btn_sd_norm");
		self.btn_dev_over[APP_DEVICE_SD]  = APP_GET_SURFACE("btn_sd_over");
		
		self.btn_dev_norm[APP_DEVICE_IDE] = APP_GET_SURFACE("btn_hdd_norm");
		self.btn_dev_over[APP_DEVICE_IDE] = APP_GET_SURFACE("btn_hdd_over");
		
		self.btn_dev_norm[APP_DEVICE_PC]  = APP_GET_SURFACE("btn_pc_norm");
		self.btn_dev_over[APP_DEVICE_PC]  = APP_GET_SURFACE("btn_pc_over");
		
//		self.btn_dev_norm[APP_DEVICE_NET] = APP_GET_SURFACE("btn_net_norm");
//		self.btn_dev_over[APP_DEVICE_NET] = APP_GET_SURFACE("btn_net_over");

		self.default_cover = self.current_cover = APP_GET_SURFACE("cover");

		self.pages    = APP_GET_WIDGET("pages");
		self.settings = APP_GET_WIDGET("settings");
		self.games    = APP_GET_WIDGET("games");
		
		self.filebrowser  = APP_GET_WIDGET("file_browser");
		self.cover_widget = APP_GET_WIDGET("cover_image");
		self.btn_run      = APP_GET_WIDGET("run_iso");

		self.async         = APP_GET_WIDGET("async-checkbox");
		self.async_sectors = APP_GET_WIDGET("async-sectors");
		self.dma           = APP_GET_WIDGET("dma-checkbox");
		self.device        = APP_GET_WIDGET("device");

		self.boot_mode_chk[BOOT_MODE_DIRECT]  = APP_GET_WIDGET("direct-checkbox");
		self.boot_mode_chk[BOOT_MODE_IPBIN]   = APP_GET_WIDGET("ipbin-checkbox");
		self.boot_mode_chk[BOOT_MODE_SYSCALL] = APP_GET_WIDGET("syscall-checkbox");
		
		self.boot_method_chk[BOOT_METHOD_04X] = APP_GET_WIDGET("04x-checkbox");
		self.boot_method_chk[BOOT_METHOD_03X] = APP_GET_WIDGET("03x-checkbox");
		
		self.os_chk[BIN_TYPE_AUTO]   = APP_GET_WIDGET("os-auto-checkbox");
		self.os_chk[BIN_TYPE_KOS]    = APP_GET_WIDGET("os-kos-checkbox");
		self.os_chk[BIN_TYPE_KATANA] = APP_GET_WIDGET("os-katana-checkbox");
//		self.os_chk[BIN_TYPE_WINCE]  = APP_GET_WIDGET("os-wince-checkbox");
		
		w = APP_GET_WIDGET("memory_panel");
		
		for(int i = 0, j = 0; i < GUI_ContainerGetCount(w); i++) {
			
			b = GUI_ContainerGetChild(w, i);
			
			if(GUI_WidgetGetType(b) == WIDGET_TYPE_BUTTON) {
				
				self.memory_chk[j++] = b;
				
				if(j > 8)
					break;
			}
		}

		self.memory_text = APP_GET_WIDGET("memory_text");

		/* Disabling scrollbar on filemanager */
		for(int i = 3; i > 0; i--) {
			w = GUI_ContainerGetChild(self.filebrowser, i);
			GUI_ContainerRemove(self.filebrowser, w);
		}

		if(!is_custom_bios()/*DirExists("/cd")*/) {
			
			cb = GUI_CallbackCreate((GUI_CallbackFunction *)isoLoader_SwitchVolume, NULL, "/cd");

			if(cb) {
				GUI_ButtonSetClick(self.btn_dev[APP_DEVICE_CD], cb);
				GUI_ObjectDecRef((GUI_Object *) cb);
			}
			
			GUI_WidgetSetEnabled(self.btn_dev[APP_DEVICE_CD], 1);
		} 
		
		if(DirExists("/sd")) {
			
			cb = GUI_CallbackCreate((GUI_CallbackFunction *)isoLoader_SwitchVolume, NULL, "/sd");

			if(cb) {
				GUI_ButtonSetClick(self.btn_dev[APP_DEVICE_SD], cb);
				GUI_ObjectDecRef((GUI_Object *) cb);
			}
			
			GUI_WidgetSetEnabled(self.btn_dev[APP_DEVICE_SD], 1);
		}
		
		if(DirExists("/ide")) {
			
			cb = GUI_CallbackCreate((GUI_CallbackFunction *)isoLoader_SwitchVolume, NULL, "/ide");

			if(cb) {
				GUI_ButtonSetClick(self.btn_dev[APP_DEVICE_IDE], cb);
				GUI_ObjectDecRef((GUI_Object *) cb);
			}
			
			GUI_WidgetSetEnabled(self.btn_dev[APP_DEVICE_IDE], 1);
		}
		
		if(DirExists("/pc")) {
			
			cb = GUI_CallbackCreate((GUI_CallbackFunction *)isoLoader_SwitchVolume, NULL, "/pc");

			if(cb) {
				GUI_ButtonSetClick(self.btn_dev[APP_DEVICE_PC], cb);
				GUI_ObjectDecRef((GUI_Object *) cb);
			}
			
			GUI_WidgetSetEnabled(self.btn_dev[APP_DEVICE_PC], 1);
		}
		
//		if(DirExists("/???")) {
//			
//			cb = GUI_CallbackCreate((GUI_CallbackFunction *)isoLoader_SwitchVolume, NULL, "/???");
//
//			if(cb) {
//				GUI_ButtonSetClick(self.btn_dev[APP_DEVICE_NET], cb);
//				GUI_ObjectDecRef((GUI_Object *) cb);
//			}
//			
//			GUI_WidgetSetEnabled(self.btn_dev[APP_DEVICE_NET], 1);
//		}

		/* Checking for arguments from a executor */
		if(app->args != NULL) {
			
			char *name = getFilePath(app->args);
			
			if(name) {
				
				GUI_FileManagerSetPath(self.filebrowser, name);
				free(name);
				/* =(
				name = strrchr(app->args, '/');
				
				if(name) {

					GUI_FileManagerUpdate(self.filebrowser, 1);
					name++;
					w = GUI_FileManagerGetItemPanel(self.filebrowser);
					int count = GUI_ContainerGetCount(w);
					
					for(int i = 0; i < count; i++) {
						
						b = GUI_FileManagerGetItem(self.filebrowser, i);
						
						if(!strncasecmp(name, GUI_ObjectGetName((GUI_Object*)b), MAX_FN_LEN)) {
							GUI_WidgetClicked(b, 1, 1);
							break;
						}
					}
				}*/
			}
			
			self.have_args = true;
			
		} else {
			self.have_args = false;
		}

	} else {
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}

void isoLoader_Shutdown(App_t *app) {
	(void)app;
	
	if(self.isoldr) {
		free(self.isoldr);
	}
}

void isoLoader_Exit(GUI_Widget *widget) {
	
	(void)widget;
	App_t *app = NULL;
	
	if(self.have_args == true) {
		
		app = GetAppByName("File Manager");
		
		if(!app || !(app->state & APP_STATE_LOADED)) {
			app = NULL;
		}
	}
	
	if(!app) {
		app = GetAppByName("Main");
	}
	
	OpenApp(app, NULL);
}
