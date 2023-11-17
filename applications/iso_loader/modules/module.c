/* DreamShell ##version##

   module.c - ISO Loader app module
   Copyright (C) 2011 Superdefault
   Copyright (C) 2011-2023 SWAT

*/

#include <ds.h>
#include <isoldr.h>
#include <vmu.h>

#include <stdbool.h>
#include <kos/md5.h>

#include "app_utils.h"
#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_iso_loader);

#define SCREENSHOT_HOTKEY (CONT_START | CONT_A | CONT_B)

static struct {

	App_t *app;
	isoldr_info_t *isoldr;
	char filename[NAME_MAX];
	uint8 md5[16];
	uint8 boot_sector[2048];
	int image_type;
	int sector_size;

	bool have_args;

	GUI_Widget *pages;

	GUI_Widget *filebrowser;
	int current_item;
	int current_item_dir;
	GUI_Surface *item_norm;
	GUI_Surface *item_focus;
	GUI_Surface *item_selected;

	GUI_Widget *extensions;
	GUI_Widget *link;
	GUI_Widget *settings;
	GUI_Widget *games;

	GUI_Widget *btn_run;
	GUI_Widget *run_pane;

	GUI_Widget *preset;

	GUI_Widget *dma;
	GUI_Widget *cdda;
	GUI_Widget *cdda_mode[6];
	GUI_Widget *cdda_mode_src[2];
	GUI_Widget *cdda_mode_dst[2];
	GUI_Widget *cdda_mode_pos[2];
	GUI_Widget *cdda_mode_ch[2];
	GUI_Widget *irq;
	GUI_Widget *low;
	GUI_Widget *heap[20];
	GUI_Widget *heap_memory_text;
	GUI_Widget *fastboot;
	GUI_Widget *async[10];
	GUI_Widget *async_label;
	GUI_Widget *vmu;
	GUI_Widget *vmu_number;
	GUI_Widget *vmu_create;
	GUI_Widget *screenshot;

	GUI_Widget *device;
	GUI_Widget *os_chk[4];

	GUI_Widget *options_switch[4];
	GUI_Widget *boot_mode_chk[3];
	GUI_Widget *memory_chk[16];
	GUI_Widget *memory_text;

	int current_dev;
	GUI_Widget  *btn_dev[APP_DEVICE_COUNT];
	GUI_Surface *btn_dev_norm[APP_DEVICE_COUNT];
	GUI_Surface *btn_dev_over[APP_DEVICE_COUNT];

	GUI_Surface *default_cover;
	GUI_Surface *current_cover;
	GUI_Widget *cover_widget;
	GUI_Widget *title;
	GUI_Widget *options_panel;

	GUI_Widget *icosizebtn[5];
	GUI_Widget *wlnkico;
	GUI_Surface *slnkico;
	GUI_Surface *stdico;

	GUI_Widget *linktext;
	GUI_Widget  *btn_hidetext;
	GUI_Widget  *save_link_btn;
	GUI_Widget  *save_link_txt;
	GUI_Widget  *rotate180;

	GUI_Widget  *wpa[2];
	GUI_Widget  *wpv[2];

	uint32_t pa[2];
	uint32_t pv[2];

} self;

void isoLoader_DefaultPreset();
int isoLoader_LoadPreset();
int isoLoader_SavePreset();
void isoLoader_ResizeUI();
void isoLoader_toggleMemory(GUI_Widget *widget);
void isoLoader_toggleBootMode(GUI_Widget *widget);
void isoLoader_toggleIconSize(GUI_Widget *widget);
static void setIcon(int size);

static int canUseTrueAsyncDMA(void) {
	return (self.sector_size == 2048 && 
			(self.current_dev == APP_DEVICE_IDE || self.current_dev == APP_DEVICE_CD) &&
			(self.image_type == ISOFS_IMAGE_TYPE_ISO || self.image_type == ISOFS_IMAGE_TYPE_GDI));
}

static char *relativeFilename(char *filename) {
	static char filepath[NAME_MAX];
	char path[NAME_MAX];
	int len;

	if (strchr(self.filename, '/')) {
		len = strlen(strchr(self.filename, '/'));
	} else {
		len = strlen(self.filename);
	}

	memset(filepath, 0, sizeof(filepath));
	memset(path, 0, sizeof(path));
	strncpy(path, self.filename, strlen(self.filename) - len);

	snprintf(filepath, NAME_MAX, "%s/%s/%s",
		GUI_FileManagerGetPath(self.filebrowser), path, filename);
	return filepath;
}

void isoLoader_Rotate_Image(GUI_Widget *widget) {
	(void)widget;
	setIcon(GUI_SurfaceGetWidth(self.slnkico));
}

void isoLoader_ShowPage(GUI_Widget *widget) {

	GUI_WidgetSetEnabled(self.link, 1);
	GUI_WidgetSetEnabled(self.extensions, 1);
	GUI_WidgetSetEnabled(self.settings, 1);
	GUI_WidgetSetEnabled(self.games, 1);

	if (widget == self.games) {
		GUI_CardStackShowIndex(self.pages, 0);
		GUI_WidgetSetEnabled(self.games, 0);
	} else if(widget == self.settings) {
		GUI_CardStackShowIndex(self.pages, 1);
		GUI_WidgetSetEnabled(self.settings, 0);
	} else if(widget == self.extensions) {
		GUI_CardStackShowIndex(self.pages, 2);
		GUI_WidgetSetEnabled(self.extensions, 0);
	} else if(widget == self.link) {
		GUI_CardStackShowIndex(self.pages, 3);
		GUI_WidgetSetEnabled(self.link, 0);
	}

	GUI_WidgetMarkChanged(self.run_pane);
}

void isoLoader_ShowSettings(GUI_Widget *widget) {
	ScreenFadeOut();
	thd_sleep(200);
	isoLoader_ShowPage(widget);
	ScreenFadeIn();
}

void isoLoader_ShowExtensions(GUI_Widget *widget) {
	ScreenFadeOut();
	thd_sleep(200);
	isoLoader_ShowPage(widget);
	ScreenFadeIn();
}

void isoLoader_ShowGames(GUI_Widget *widget) {
	ScreenFadeOut();
	thd_sleep(200);
	isoLoader_SavePreset();
	isoLoader_ShowPage(widget);
	ScreenFadeIn();
}

static void check_link_file(void) {
	char save_file[NAME_MAX];
	snprintf(save_file, NAME_MAX, "%s/apps/main/scripts/%s.dsc",
		getenv("PATH"), GUI_TextEntryGetText(self.linktext));

	if(FileExists(save_file)) {
		GUI_LabelSetText(self.save_link_txt, "rewrite shortcut");
	} else {
		GUI_LabelSetText(self.save_link_txt, "create shortcut");
	}

	GUI_WidgetMarkChanged(self.save_link_btn);
}

void isoLoader_ShowLink(GUI_Widget *widget) {

	ScreenFadeOut();
	thd_sleep(200);

	GUI_WidgetSetState(self.rotate180, 0);
	GUI_WidgetSetState(self.btn_hidetext, 0);

	ipbin_meta_t *ipbin = (ipbin_meta_t *)self.boot_sector;
	ipbin->title[sizeof(ipbin->title)-1] = '\0';
	GUI_TextEntrySetText(self.linktext, trim_spaces2(ipbin->title));
	check_link_file();

	isoLoader_ShowPage(widget);
	isoLoader_toggleIconSize(self.icosizebtn[0]);
	ScreenFadeIn();
}


/* Set the active volume in the menu */
static void highliteDevice() {
	
	int dev = getDeviceType(GUI_FileManagerGetPath(self.filebrowser));
	
	if(dev > -1 && dev != self.current_dev) {
		
		if(self.current_dev > -1) {
			GUI_ButtonSetNormalImage(self.btn_dev[self.current_dev], self.btn_dev_norm[self.current_dev]);
		}
		
		GUI_ButtonSetNormalImage(self.btn_dev[dev], self.btn_dev_over[dev]);
		self.current_dev = dev;
		UpdateActiveMouseCursor();
	}
}


static void get_md5_hash(const char *mountpoint) {
	
	file_t fd;
	fd = fs_iso_first_file(mountpoint);
	
	if(fd != FILEHND_INVALID) {

		if(fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, (int) self.boot_sector) < 0) {
			memset(self.md5, 0, sizeof(self.md5));
			memset(self.boot_sector, 0, sizeof(self.boot_sector));
		} else {
			kos_md5(self.boot_sector, sizeof(self.boot_sector), self.md5);
		}
		
		/* Also get image type and sector size */
		if(fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, (int) &self.image_type) < 0) {
			ds_printf("%s: Can't get image type\n", lib_get_name());
		}
		
		if(fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE, (int) &self.sector_size) < 0) {
			ds_printf("%s: Can't get sector size\n", lib_get_name());
		}
		
		fs_close(fd);
	}
}

/* Try to get cover image from ISO */
static void showCover() {

	GUI_Surface *s = NULL;
	char path[NAME_MAX];
	char noext[128];
	ipbin_meta_t *ipbin;
	int use_cover = 0;

	snprintf(path, NAME_MAX, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), self.filename);

	if(fs_iso_mount("/isocover", path)) {
		GUI_LabelSetText(self.title, "None");
		GUI_PanelSetBackground(self.cover_widget, self.default_cover);
		self.current_cover = self.default_cover;
		return;
	}

	memset(noext, 0, sizeof(noext));
	strncpy(noext, (!strchr(self.filename, '/')) ? self.filename : (strchr(self.filename, '/')+1), sizeof(noext));
	strcpy(noext, strtok(noext, "."));

	get_md5_hash("/isocover");
	ipbin = (ipbin_meta_t *)self.boot_sector;
	trim_spaces(ipbin->title, noext, sizeof(ipbin->title));
	GUI_LabelSetText(self.title, noext);

	snprintf(path, NAME_MAX, "%s/apps/iso_loader/covers/%s.png", getenv("PATH"), noext);
	if(FileExists(path)) {
		use_cover = 1;
	} else {
		snprintf(path, NAME_MAX, "%s/apps/iso_loader/covers/%s.jpg", getenv("PATH"), noext);
		if(FileExists(path)) {
			use_cover = 1;
		} else {
			memset(path, 0, sizeof(path));
			strcpy(path, "/isocover/0GDTEX.PVR");
			if (FileExists(path)) {
				use_cover = 1;
			}
		}
	} 

	if (use_cover) {
		s = GUI_SurfaceLoad(path);
	}

	fs_iso_unmount("/isocover");

	if(s != NULL) {
		GUI_PanelSetBackground(self.cover_widget, s);
		GUI_ObjectDecRef((GUI_Object *) s);
		self.current_cover = s;
	} else if(self.current_cover != self.default_cover) {
		GUI_PanelSetBackground(self.cover_widget, self.default_cover);
		self.current_cover = self.default_cover;
	}

	vmu_draw_string(noext);
}

void isoLoader_MakeShortcut(GUI_Widget *widget) 
{
	(void)widget;
	FILE *fd;
	char *env = getenv("PATH");
	char save_file[NAME_MAX];
	char cmd[NAME_MAX * 2];
	const char *tmpval;
	int i;

	StopCDDATrack();
	snprintf(save_file, NAME_MAX, "%s/apps/main/scripts/%s.dsc", env, GUI_TextEntryGetText(self.linktext));

	fd = fopen(save_file, "w");

	if(!fd)
	{
		ds_printf("DS_ERROR: Can't save shortcut\n");
		return;
	}

	fprintf(fd, "module -o -f %s/modules/minilzo.klf\n", env);
	fprintf(fd, "module -o -f %s/modules/isofs.klf\n", env);
	fprintf(fd, "module -o -f %s/modules/isoldr.klf\n", env);

	strcpy(cmd, "isoldr");
	strcat(cmd, GUI_WidgetGetState(self.fastboot) ? " -s" : " -i" );

	if(GUI_WidgetGetState(self.dma)) 
	{
		strcat(cmd, " -a");
	}

	if(GUI_WidgetGetState(self.irq)) 
	{
		strcat(cmd, " -q");
	}

	if(GUI_WidgetGetState(self.low)) 
	{
		strcat(cmd, " -l");
	}

	for(i = 0; i < sizeof(self.async) >> 2; i++) 
	{
		if(GUI_WidgetGetState(self.async[i])) 
		{
			if(i)
			{
				char async[8];
				int val = atoi(GUI_LabelGetText(GUI_ButtonGetCaption(self.async[i])));
				snprintf(async, sizeof(async), " -e %d", val);
				strcat(cmd, async);
			}
			break;
		}
	}

	tmpval = GUI_TextEntryGetText(self.device);

	if(strncmp(tmpval, "auto", 4) != 0)
	{
		strcat(cmd, " -d ");
		strcat(cmd, tmpval);
	}

	for(i = 0; self.memory_chk[i]; i++)
	{
		if(GUI_WidgetGetState(self.memory_chk[i]))
		{
			tmpval = GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);
			
			if(strlen(tmpval) < 8) 
			{
				char text[24];
				memset(text, 0, sizeof(text));
				strncpy(text, tmpval, 10);
				tmpval = strncat(text, GUI_TextEntryGetText(self.memory_text), 10);
			}
			strcat(cmd, " -x ");
			strcat(cmd, tmpval);
			break;
		}
	}

	char fpath[NAME_MAX];
	sprintf(fpath, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), self.filename);

	strcat(cmd, " -f ");
	strcat(cmd, fix_spaces(fpath));

	for(i = 0; i < sizeof(self.boot_mode_chk) >> 2; i++) 
	{
		if(i && GUI_WidgetGetState(self.boot_mode_chk[i])) 
		{
			char boot_mode[8];
			sprintf(boot_mode, "%d", i);
			strcat(cmd, " -j ");
			strcat(cmd, boot_mode);
			break;
		}
	}

	for(i = 0; i < sizeof(self.os_chk) >> 2; i++) 
	{
		if(i && GUI_WidgetGetState(self.os_chk[i])) 
		{
			char os[8];
			sprintf(os, "%d", i);
			strcat(cmd, " -o ");
			strcat(cmd, os);
			break;
		}
	}

	char patchstr[24];
	for(i = 0; i < sizeof(self.pa) >> 2; ++i)
	{
		if(self.pa[i] & 0xffffff)
		{
			sprintf(patchstr," --pa%d 0x%s", i + 1, GUI_TextEntryGetText(self.wpa[i]));
			strcat(cmd, patchstr);
			sprintf(patchstr," --pv%d 0x%s", i + 1, GUI_TextEntryGetText(self.wpv[i]));
			strcat(cmd, patchstr);
		}
	}

	for(i = 1; i < sizeof(self.cdda_mode) >> 2; i++)
	{
		if(GUI_WidgetGetState(self.cdda_mode[i]))
		{
			char cdda_mode[12];
			if (i == CDDA_MODE_EXTENDED) {
				sprintf(cdda_mode, "0x%08lx", getExtModeCDDA());
			} else {
				sprintf(cdda_mode, "%d", i);
			}
			strcat(cmd, " -g ");
			strcat(cmd, cdda_mode);
			break;
		}
	}

	for(int i = 0; i < sizeof(self.heap) >> 2; i++) {
		if(self.heap[i] && GUI_WidgetGetState(self.heap[i])) {
			if (i <= HEAP_MODE_MAPLE) {
				char mode[24];
				sprintf(mode, " -h %d", i);
				strcat(cmd, mode);
				break;
			} else {
				char *addr = (char* )GUI_ObjectGetName((GUI_Object *)self.heap[i]);
				if(strlen(addr) < 8) {
					char text[24];
					memset(text, 0, sizeof(text));
					strncpy(text, addr, 10);
					addr = strncat(text, GUI_TextEntryGetText(self.heap_memory_text), 10);
				}
				strcat(cmd, " -h ");
				strcat(cmd, addr);
			}
			break;
		}
	}

	if(GUI_WidgetGetState(self.vmu)) {
		char number[12];
		sprintf(number, " -v %d", atoi(GUI_TextEntryGetText(self.vmu_number)));
		strcat(cmd, number);
	}

	if(GUI_WidgetGetState(self.screenshot)) {
		char hotkey[24];
		sprintf(hotkey, " -k 0x%lx", (uint32)SCREENSHOT_HOTKEY);
		strcat(cmd, hotkey);
	}

	fprintf(fd, "%s\n", cmd);
	fprintf(fd, "console --show\n");
	fclose(fd);

	snprintf(save_file, NAME_MAX, "%s/apps/main/images/%s.png", env, GUI_TextEntryGetText(self.linktext));
	if(FileExists(save_file)) {
		fs_unlink(save_file);
	}
	GUI_SurfaceSavePNG(self.slnkico, save_file);

	isoLoader_ShowGames(self.settings);
}

static void setIcon(int size) {
	GUI_Surface *image, *surf;
	SDL_Surface *orig_s, *new_s, *temp_s;

	if(self.current_cover == self.default_cover) {
		surf = self.stdico;
	} else {
		surf = self.current_cover;
	}

	orig_s = GUI_SurfaceGet(surf);
	double scaling = (double)size / (double)orig_s->w;
	new_s = zoomSurface(orig_s, scaling, scaling, 1);

	if(!new_s) {
		return;
	}

	if(GUI_WidgetGetState(self.rotate180)) {
		temp_s = rotateSurface90Degrees(new_s, 2);
		if(temp_s) {
			SDL_FreeSurface(new_s);
			new_s = temp_s;
		}
	}

	image = GUI_SurfaceFrom("shortcut-icon-surface", new_s);

	if (image) {
		GUI_PictureSetImage(self.wlnkico, image);
		GUI_ObjectDecRef((GUI_Object *) image);
		self.slnkico = image;
	}
	GUI_WidgetMarkChanged(self.pages);
	GUI_WidgetMarkChanged(self.run_pane);
}

void isoLoader_toggleIconSize(GUI_Widget *widget) {
	int i, size;

	for(i = 0; i < 5; ++i) {
		GUI_WidgetSetState(self.icosizebtn[i], 0);
	}

	GUI_WidgetSetState(widget, 1);
	char *name = (char *)GUI_ObjectGetName((GUI_Object *)widget);

	switch(name[1]) {
		case '2':
			size = 128;
			break;
		case '4':
			size = 64;
			break;
		case '5':
			size = 256;
			break;
		case '6':
			size = 96;
			break;
		case '8':
		default:
			size = 48;
	}
	setIcon(size);
}

void isoLoader_toggleLinkName(GUI_Widget *widget)
{
	char curtext[33];
	
	snprintf(curtext, 33, "%s", GUI_TextEntryGetText(widget));
	
	int state = !(curtext[0] != '_');
	
	if(strlen(curtext) < 3)
	{
		ipbin_meta_t *ipbin = (ipbin_meta_t *)self.boot_sector;
		ipbin->title[sizeof(ipbin->title)-1] = '\0';
		GUI_TextEntrySetText(self.linktext, trim_spaces2(ipbin->title));
	}
	
	GUI_WidgetSetState(self.btn_hidetext, state);
	
	check_link_file();	
}

void isoLoader_toggleHideName(GUI_Widget *widget)
{
	int state = GUI_WidgetGetState(widget);
	char *curtext = (char *)GUI_TextEntryGetText(self.linktext);
	char text[NAME_MAX];
	
	if(state && curtext[0] != '_')
	{
		snprintf(text, NAME_MAX, "_%s", curtext);
		GUI_TextEntrySetText(self.linktext, text);
	}
	else if(!state && curtext[0] == '_')
	{
		snprintf(text, NAME_MAX, "%s", &curtext[1]);
		GUI_TextEntrySetText(self.linktext, text);
	}
	
	check_link_file();
}

void isoLoader_toggleOptions(GUI_Widget *widget) 
{
	for(int i = 0; i < sizeof(self.options_switch) >> 2; i++) {
		if(widget != self.options_switch[i]) {
			GUI_WidgetSetState(self.options_switch[i], 0);
		} else if(GUI_WidgetGetState(widget)) {
			GUI_CardStackShowIndex(self.options_panel, i);
		} else {
			GUI_WidgetSetState(widget, 1);
		}
	}
	GUI_WidgetMarkChanged(self.run_pane);
}

void isoLoader_togglePatchAddr(GUI_Widget *widget)
{
	const char *name = GUI_ObjectGetName((GUI_Object *)widget);
	const char *text = GUI_TextEntryGetText(widget);
	uint32_t temp = strtoul(text, NULL, 16);
	
	if( (strlen(text) != 8) || (name[1] == 'a' && (!(temp & 0xffffff) 
							|| ((temp >> 24 != 0x0c) 
							&&  (temp >> 24 != 0x8c) 
							&&  (temp >> 24 != 0xac)))) 
							|| (name[1] == 'v' && !temp))
	{
		GUI_TextEntrySetText(widget, (name[1] == 'a') ? "0c000000" : "00000000");
	}
	else
	{
		switch(name[1])
		{
			case 'a':
				self.pa[name[2]-'1'] = strtoul(text, NULL, 16);
				break;
			
			case 'v':
				self.pv[name[2]-'1'] = strtoul(text, NULL, 16);
				break;
		}
	}
}

/* Switch to the selected volume */
void isoLoader_SwitchVolume(void *dir) {
	GUI_FileManagerSetPath(self.filebrowser, (char *)dir);
	highliteDevice();
}

void isoLoader_toggleOS(GUI_Widget *widget) {
	for(int i = 0; i < sizeof(self.os_chk) >> 2; i++) {
		if(widget != self.os_chk[i]) {
			GUI_WidgetSetState(self.os_chk[i], 0);
		}
	}
}

void isoLoader_toggleAsync(GUI_Widget *widget) {
	for(int i = 0; i < sizeof(self.async) >> 2; i++) {
		if(widget != self.async[i]) {
			GUI_WidgetSetState(self.async[i], 0);
		} else {
			GUI_WidgetSetState(self.async[i], 1);
		}
	}
}

void isoLoader_toggleDMA(GUI_Widget *widget) {
	
	if (GUI_WidgetGetState(widget) && canUseTrueAsyncDMA()) {

		if (!strncmp(GUI_LabelGetText(self.async_label), "none", 4)) {
			GUI_LabelSetText(self.async_label, "true");
		}

		if (!GUI_WidgetGetState(self.async[0])) {
			isoLoader_toggleAsync(self.async[0]);
		} else {
			GUI_WidgetMarkChanged(self.async[0]);
		}

	} else {

		if (!strncmp(GUI_LabelGetText(self.async_label), "true", 4)) {
			GUI_LabelSetText(self.async_label, "none");
		}

		if (GUI_WidgetGetState(self.async[0])) {
			isoLoader_toggleAsync(self.async[8]);
		} else {
			GUI_WidgetMarkChanged(self.async[0]);
		}
	}
}

void isoLoader_toggleCDDA(GUI_Widget *widget) {

	int mode = CDDA_MODE_DISABLED;

	for(int i = 0; i < sizeof(self.cdda_mode) >> 2; i++) {
		if(widget != self.cdda_mode[i]) {
			GUI_WidgetSetState(self.cdda_mode[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
			mode = i;
		}
	}

	if (self.cdda == widget) {
		if (GUI_WidgetGetState(widget)) {
			isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_DMA_TMU2]);
		} else {
			isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_DISABLED]);
		}
		return;
	}

	if(self.cdda_mode[CDDA_MODE_DISABLED] == widget) {
		if (GUI_WidgetGetState(widget)) {
			isoLoader_toggleCDDA_Source(NULL);
			isoLoader_toggleCDDA_Dest(NULL);
			isoLoader_toggleCDDA_Pos(NULL);
			isoLoader_toggleCDDA_Chan(NULL);
			GUI_WidgetSetState(self.cdda, 0);
			GUI_WidgetSetState(self.cdda_mode[CDDA_MODE_DISABLED], 1);
			GUI_WidgetSetState(self.cdda_mode[CDDA_MODE_EXTENDED], 0);
		}
		return;
	}

	if (!GUI_WidgetGetState(self.cdda)) {
		GUI_WidgetSetState(self.cdda, 1);
	}

	if (self.cdda_mode[CDDA_MODE_EXTENDED] != widget) {
		switch (mode) {
			case CDDA_MODE_DMA_TMU2:
				isoLoader_toggleCDDA_Source(self.cdda_mode_src[1]);
				isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[1]);
				isoLoader_toggleCDDA_Pos(self.cdda_mode_pos[1]);
				isoLoader_toggleCDDA_Chan(self.cdda_mode_ch[1]);
				break;
			case CDDA_MODE_DMA_TMU1:
				isoLoader_toggleCDDA_Source(self.cdda_mode_src[1]);
				isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[1]);
				isoLoader_toggleCDDA_Pos(self.cdda_mode_pos[0]);
				isoLoader_toggleCDDA_Chan(self.cdda_mode_ch[1]);
				break;
			case CDDA_MODE_SQ_TMU2:
				isoLoader_toggleCDDA_Source(self.cdda_mode_src[0]);
				isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[0]);
				isoLoader_toggleCDDA_Pos(self.cdda_mode_pos[1]);
				isoLoader_toggleCDDA_Chan(self.cdda_mode_ch[1]);
				break;
			case CDDA_MODE_SQ_TMU1:
				isoLoader_toggleCDDA_Source(self.cdda_mode_src[0]);
				isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[0]);
				isoLoader_toggleCDDA_Pos(self.cdda_mode_pos[0]);
				isoLoader_toggleCDDA_Chan(self.cdda_mode_ch[1]);
				break;
			default:
				break;
		}
		GUI_WidgetSetState(self.cdda_mode[mode], 1);
		GUI_WidgetSetState(self.cdda_mode[CDDA_MODE_EXTENDED], 0);
	}
}

void isoLoader_toggleCDDA_Source(GUI_Widget *widget) {

	if (!GUI_WidgetGetState(self.cdda_mode[CDDA_MODE_EXTENDED])) {
		isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_EXTENDED]);
	}

	for(int i = 0; i < sizeof(self.cdda_mode_src) >> 2; i++) {
		if(widget != self.cdda_mode_src[i]) {
			GUI_WidgetSetState(self.cdda_mode_src[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
		}
	}
}

void isoLoader_toggleCDDA_Dest(GUI_Widget *widget) {

	if (!GUI_WidgetGetState(self.cdda_mode[CDDA_MODE_EXTENDED])) {
		isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_EXTENDED]);
	}

	for(int i = 0; i < sizeof(self.cdda_mode_dst) >> 2; i++) {
		if(widget != self.cdda_mode_dst[i]) {
			GUI_WidgetSetState(self.cdda_mode_dst[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
		}
	}
}

void isoLoader_toggleCDDA_Pos(GUI_Widget *widget) {

	if (!GUI_WidgetGetState(self.cdda_mode[CDDA_MODE_EXTENDED])) {
		isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_EXTENDED]);
	}

	for(int i = 0; i < sizeof(self.cdda_mode_pos) >> 2; i++) {
		if(widget != self.cdda_mode_pos[i]) {
			GUI_WidgetSetState(self.cdda_mode_pos[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
		}
	}
}

void isoLoader_toggleCDDA_Chan(GUI_Widget *widget) {

	if (!GUI_WidgetGetState(self.cdda_mode[CDDA_MODE_EXTENDED])) {
		isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_EXTENDED]);
	}

	for(int i = 0; i < sizeof(self.cdda_mode_ch) >> 2; i++) {
		if(widget != self.cdda_mode_ch[i]) {
			GUI_WidgetSetState(self.cdda_mode_ch[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
		}
	}
}

static int getActiveWidgetIndex(GUI_Widget **widgets, size_t count) {
	for(int i = 0; i < count; i++) {
		if(GUI_WidgetGetState(widgets[i])) {
			return i;
		}
	}
	return -1;
}

uint32 getExtModeCDDA() {
	uint32 mode = CDDA_MODE_EXTENDED;
	int index;

	index = getActiveWidgetIndex(self.cdda_mode_src, sizeof(self.cdda_mode_src) >> 2);
	if (index == 0) {
		mode |= CDDA_MODE_SRC_PIO;
	} else {
		mode |= CDDA_MODE_SRC_DMA;
	}

	index = getActiveWidgetIndex(self.cdda_mode_dst, sizeof(self.cdda_mode_dst) >> 2);
	if (index == 0) {
		mode |= CDDA_MODE_DST_SQ;
	} else {
		mode |= CDDA_MODE_DST_DMA;
	}

	index = getActiveWidgetIndex(self.cdda_mode_pos, sizeof(self.cdda_mode_pos) >> 2);
	if (index == 0) {
		mode |= CDDA_MODE_POS_TMU1;
	} else {
		mode |= CDDA_MODE_POS_TMU2;
	}

	index = getActiveWidgetIndex(self.cdda_mode_ch, sizeof(self.cdda_mode_ch) >> 2);
	if (index == 0) {
		mode |= CDDA_MODE_CH_FIXED;
	} else {
		mode |= CDDA_MODE_CH_ADAPT;
	}
	return mode;
}

void setExtModeCDDA(uint32 mode) {
	isoLoader_toggleCDDA(self.cdda_mode[CDDA_MODE_EXTENDED]);

	if (mode & CDDA_MODE_SRC_PIO) {
		isoLoader_toggleCDDA_Source(self.cdda_mode_src[0]);
	} else {
		isoLoader_toggleCDDA_Source(self.cdda_mode_src[1]);
	}

	if (mode & CDDA_MODE_DST_SQ) {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[0]);
	} else {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_dst[1]);
	}

	if (mode & CDDA_MODE_POS_TMU1) {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_pos[0]);
	} else {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_pos[1]);
	}

	if (mode & CDDA_MODE_CH_FIXED) {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_ch[0]);
	} else {
		isoLoader_toggleCDDA_Dest(self.cdda_mode_ch[1]);
	}
}

void isoLoader_toggleHeap(GUI_Widget *widget) 
{
	for(int i = 0; i < sizeof(self.heap) >> 2; i++) {
		if (!self.heap[i]) {
			break;
		} if(widget != self.heap[i]) {
			GUI_WidgetSetState(self.heap[i], 0);
		} else {
			GUI_WidgetSetState(widget, 1);
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
	
	if(!strncmp(GUI_ObjectGetName((GUI_Object *) widget), "memory-text", 12)) {
		GUI_WidgetSetState(self.memory_chk[i-1], 1);
	}
	
	if(GUI_WidgetGetState(self.memory_chk[2]) && 
		GUI_WidgetGetState(self.boot_mode_chk[BOOT_MODE_IPBIN])) {
		
		GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_IPBIN], 0);
		GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_IPBIN_TRUNC], 1);
		
	} else if(GUI_WidgetGetState(self.memory_chk[3]) && 
		(GUI_WidgetGetState(self.boot_mode_chk[BOOT_MODE_IPBIN]) || 
		GUI_WidgetGetState(self.boot_mode_chk[BOOT_MODE_IPBIN_TRUNC]))) {
		
		GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_IPBIN], 0);
		GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_IPBIN_TRUNC], 0);
		GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_DIRECT], 1);
	}
}

void isoLoader_toggleBootMode(GUI_Widget *widget) {
	
	for(int i = 0; i < sizeof(self.boot_mode_chk) >> 2; i++) {
		if(widget != self.boot_mode_chk[i]) {
			GUI_WidgetSetState(self.boot_mode_chk[i], 0);
		}
	}
}

void isoLoader_toggleExtension(GUI_Widget *widget) {
	if (GUI_WidgetGetState(widget)) {
		GUI_WidgetSetState(self.irq, 1);
		if (widget == self.vmu_create) {
			GUI_WidgetSetState(self.vmu, 1);
		}
	}
}

void isoLoader_Run(GUI_Widget *widget) {

	char filepath[NAME_MAX];
	const char *tmpval;
	uint32 addr = ISOLDR_DEFAULT_ADDR_LOW;
	(void)widget;

	memset(filepath, 0, NAME_MAX);
	snprintf(filepath, NAME_MAX, "%s/%s", 
				GUI_FileManagerGetPath(self.filebrowser), 
				self.filename);

	ScreenFadeOut();
	StopCDDATrack();

	if(!GUI_WidgetGetState(self.fastboot)) {
		thd_sleep(400);
	}

	if(GUI_CardStackGetIndex(self.pages) != 0) {
		isoLoader_SavePreset();
	}

	self.isoldr = isoldr_get_info(filepath, 0);

	if(self.isoldr == NULL) {
		ShowConsole();
		ScreenFadeIn();
		return;
	}

	for(int i = 0; i < sizeof(self.async) >> 2; ++i) {
		if (GUI_WidgetGetState(self.async[i])) {
			if (i) {
				self.isoldr->emu_async = atoi(GUI_LabelGetText(GUI_ButtonGetCaption(self.async[i])));
			} else {
				self.isoldr->emu_async = 0;
			}
			break;
		}
	}

	if(GUI_WidgetGetState(self.irq)) {
		self.isoldr->use_irq = 1;
	}

	if(GUI_WidgetGetState(self.low)) {
		self.isoldr->syscalls = 1;
	}

	for(int i = 0; i < sizeof(self.cdda_mode) >> 2; i++) {
		if(GUI_WidgetGetState(self.cdda_mode[i])) {
			if (i == CDDA_MODE_EXTENDED) {
				self.isoldr->emu_cdda = getExtModeCDDA();
			} else {
				self.isoldr->emu_cdda = i;
			}
			break;
		}
	}

	for(int i = 0; i < sizeof(self.heap) >> 2; i++) {
		if(GUI_WidgetGetState(self.heap[i])) {
			if (i <= HEAP_MODE_MAPLE) {
				self.isoldr->heap = i;
			} else {
				tmpval = GUI_ObjectGetName((GUI_Object *)self.heap[i]);

				if(strlen(tmpval) < 8) {
					char text[24];
					memset(text, 0, sizeof(text));
					strncpy(text, tmpval, 10);
					tmpval = strncat(text, GUI_TextEntryGetText(self.heap_memory_text), 10);
				}

				self.isoldr->heap = strtoul(tmpval, NULL, 16);
			}
			break;
		}
	}

	if(GUI_WidgetGetState(self.dma)) {
		self.isoldr->use_dma = 1;
	}

	if(GUI_WidgetGetState(self.fastboot)) {
		self.isoldr->fast_boot = 1;
	}

	tmpval = GUI_TextEntryGetText(self.device);

	if(strncmp(tmpval, "auto", 4) != 0) {
		strncpy(self.isoldr->fs_dev, tmpval, sizeof(self.isoldr->fs_dev));
	}

	if(self.current_cover && self.current_cover != self.default_cover && !self.isoldr->fast_boot) {

		SDL_Surface *surf = GUI_SurfaceGet(self.current_cover);

		if(surf) {
			self.isoldr->gdtex = (uint32)surf->pixels;
		}
	}

	for(int i = 0; i < sizeof(self.os_chk) >> 2; i++) {
		if(i && GUI_WidgetGetState(self.os_chk[i])) {
			self.isoldr->exec.type = i;
			break;
		}
	}

	for(int i = 0; i < sizeof(self.boot_mode_chk) >> 2; i++) {
		if(i && GUI_WidgetGetState(self.boot_mode_chk[i])) {
			self.isoldr->boot_mode = i;
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

	for(int i = 0; i < sizeof(self.isoldr->patch_addr) >> 2; ++i) {
		if(self.pa[i] & 0xffffff) {
			self.isoldr->patch_addr[i] = self.pa[i];
			self.isoldr->patch_value[i] = self.pv[i];
		}
	}

	if(GUI_WidgetGetState(self.vmu)) {
		self.isoldr->emu_vmu = atoi(GUI_TextEntryGetText(self.vmu_number));
	}

	if(GUI_WidgetGetState(self.vmu_create)) {

		snprintf(filepath, sizeof(filepath),
				"%s/apps/%s/resources/empty_vmu.vmd",
				getenv("PATH"), lib_get_name() + 4);

		char fn[32];
		snprintf(fn, sizeof(fn), "vmu%03ld.vmd", self.isoldr->emu_vmu);
		CopyFile(filepath, relativeFilename(fn), 0);
	}

	if(GUI_WidgetGetState(self.screenshot)) {
		self.isoldr->scr_hotkey = SCREENSHOT_HOTKEY;
	}

	isoldr_exec(self.isoldr, addr);

	/* If we there, then something wrong... */
	ShowConsole();
	ScreenFadeIn();
	free(self.isoldr);
}

static void selectFile(char *name, int index) {
	GUI_Widget *w;
	GUI_WidgetSetEnabled(self.btn_run, 1);
	
	w = GUI_FileManagerGetItem(self.filebrowser, index);
	GUI_ButtonSetNormalImage(w, self.item_selected);
	GUI_ButtonSetHighlightImage(w, self.item_selected);
	GUI_ButtonSetPressedImage(w, self.item_selected);
	
	if(self.current_item > -1) {
		w = GUI_FileManagerGetItem(self.filebrowser, self.current_item);
		GUI_ButtonSetNormalImage(w, self.item_norm);
		GUI_ButtonSetHighlightImage(w, self.item_focus);
		GUI_ButtonSetPressedImage(w, self.item_focus);
	}

	self.current_item = index;
	strncpy(self.filename, name, NAME_MAX);
	highliteDevice();
	StopCDDATrack();
	showCover();
	isoLoader_LoadPreset();

	if (GUI_WidgetGetState(self.cdda)) {
		char filepath[NAME_MAX];
		size_t track_size = GetCDDATrackFilename(5,
			GUI_FileManagerGetPath(self.filebrowser), self.filename, filepath);

		if (track_size) {
			do {
				track_size = GetCDDATrackFilename((random() % 15) + 4,
								GUI_FileManagerGetPath(self.filebrowser), self.filename, filepath);
			} while(track_size == 0);
			PlayCDDATrack(filepath, 3);
		}
	}

	if (GUI_WidgetGetFlags(self.settings) & WIDGET_DISABLED) {
		GUI_WidgetSetEnabled(self.settings, 1);
		GUI_WidgetSetEnabled(self.link, 1);
		GUI_WidgetSetEnabled(self.extensions, 1);
	}
}


static void changeDir(dirent_t *ent) {
	self.current_item = -1;
	self.current_item_dir = -1;
	memset(self.filename, 0, NAME_MAX);
	GUI_FileManagerChangeDir(self.filebrowser, ent->name, ent->size);
	highliteDevice();
	GUI_WidgetSetEnabled(self.btn_run, 0);
}

void isoLoader_ItemClick(dirent_fm_t *fm_ent) {

	if(!fm_ent) {
		return;
	}
	dirent_t *ent = &fm_ent->ent;

	if(ent->attr == O_DIR && self.current_item_dir != fm_ent->index) {

		char filepath[NAME_MAX];
		memset(filepath, 0, NAME_MAX);
		snprintf(filepath, NAME_MAX, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), ent->name);

		file_t fd = fs_open(filepath, O_RDONLY | O_DIR);

		if(fd != FILEHND_INVALID) {

			int total_count = 0;
			int dir_count = 0;
			dirent_t *dent;

			while ((dent = fs_readdir(fd)) != NULL) {

				if(++total_count > 100) {
					break;
				}
				if(dent->name[0] == '.') {
					continue;
				}
				if(dent->attr == O_DIR && ++dir_count > 1) {
					break;
				}
				int len = strlen(dent->name);

				if(len > 4 && strncasecmp(dent->name + len - 4, ".gdi", 4) == 0) {
					fs_close(fd);

					memset(filepath, 0, NAME_MAX);
					snprintf(filepath, NAME_MAX, "%s/%s", ent->name, dent->name);
					selectFile(filepath, fm_ent->index);

					self.current_item_dir = fm_ent->index;
					return;
				}
			}
			fs_close(fd);
		}
		changeDir(ent);

	} else if(self.current_item == fm_ent->index) {

		isoLoader_Run(NULL);

	} else if(IsFileSupportedByApp(self.app, ent->name)) {

		selectFile(ent->name, fm_ent->index);
	}
}


void isoLoader_ItemContextClick(dirent_fm_t *fm_ent) {

	if (self.current_item != fm_ent->index) {

		dirent_t *ent = &fm_ent->ent;

		if(ent->attr == O_DIR) {
			changeDir(ent);
		} else {
			isoLoader_ItemClick(fm_ent);
		}
	} else {
		isoLoader_ShowSettings(self.settings);
	}
}


void isoLoader_DefaultPreset() {

	if(canUseTrueAsyncDMA() && !GUI_WidgetGetState(self.dma)) {

		GUI_WidgetSetState(self.dma, 1);
		isoLoader_toggleDMA(self.dma);

	} else if(!canUseTrueAsyncDMA() && GUI_WidgetGetState(self.dma)) {

		GUI_WidgetSetState(self.dma, 0);
		isoLoader_toggleDMA(self.dma);
		isoLoader_toggleAsync(self.async[8]);
	}

	GUI_TextEntrySetText(self.device, "auto");

	GUI_WidgetSetState(self.preset, 0);

	GUI_WidgetSetState(self.cdda, 0);
	isoLoader_toggleCDDA(self.cdda);

	GUI_WidgetSetState(self.irq, 0);
	GUI_WidgetSetState(self.low, 0);

	GUI_WidgetSetState(self.vmu, 0);
	GUI_WidgetSetState(self.screenshot, 0);

	GUI_WidgetSetState(self.os_chk[BIN_TYPE_AUTO], 1);
	isoLoader_toggleOS(self.os_chk[BIN_TYPE_AUTO]);

	GUI_WidgetSetState(self.memory_chk[0], 1);
	isoLoader_toggleMemory(self.memory_chk[0]);

	GUI_WidgetSetState(self.boot_mode_chk[BOOT_MODE_DIRECT], 1);
	isoLoader_toggleBootMode(self.boot_mode_chk[BOOT_MODE_DIRECT]);

	/*
	 * Enable CDDA if present
	 */
	if (self.filename[0] != 0) {

		char filepath[NAME_MAX];
		size_t track_size = GetCDDATrackFilename(4,
			GUI_FileManagerGetPath(self.filebrowser), self.filename, filepath);

		if (track_size && track_size < 30 * 1024 * 1024) {
			track_size = GetCDDATrackFilename(6,
				GUI_FileManagerGetPath(self.filebrowser), self.filename, filepath);
		}

		if (track_size > 0) {
			GUI_WidgetSetState(self.irq, 1);
			GUI_WidgetSetState(self.cdda, 1);
			isoLoader_toggleCDDA(self.cdda);
		}
	}
}

int isoLoader_SavePreset() {

	if (!GUI_WidgetGetState(self.preset)) {
		return 0;
	}

	char *filename, *memory = NULL;
	file_t fd;
	ipbin_meta_t *ipbin = (ipbin_meta_t *)self.boot_sector;
	char result[1024];
	char text[24];
	int async = 0, type = 0, mode = 0;
	uint32 heap = HEAP_MODE_AUTO;
	uint32 cdda_mode = CDDA_MODE_DISABLED;

	if (!self.filename[0]) {
		return 0;
	}

	StopCDDATrack();
	filename = makePresetFilename(GUI_FileManagerGetPath(self.filebrowser), self.md5);

	fd = fs_open(filename, O_CREAT | O_TRUNC | O_WRONLY);

	if(fd == FILEHND_INVALID) {
		return -1;
	}

	for(int i = 1; i < sizeof(self.async) >> 2; i++) {
		if(GUI_WidgetGetState(self.async[i])) {
			async = atoi(GUI_LabelGetText(GUI_ButtonGetCaption(self.async[i])));
			break;
		}
	}

	for(int i = 0; i < sizeof(self.os_chk) >> 2; i++) {
		if(GUI_WidgetGetState(self.os_chk[i])) {
			type = i;
			break;
		}
	}

	for(int i = 0; i < sizeof(self.boot_mode_chk) >> 2; i++) {
		if(GUI_WidgetGetState(self.boot_mode_chk[i])) {
			mode = i;
			break;
		}
	}

	for(int i = 0; i < sizeof(self.heap) >> 2; i++) {
		if(self.heap[i] && GUI_WidgetGetState(self.heap[i])) {
			if (i <= HEAP_MODE_MAPLE) {
				heap = i;
			} else {
				char *tmpval = (char* )GUI_ObjectGetName((GUI_Object *)self.heap[i]);

				if(strlen(tmpval) < 8) {
					memset(text, 0, sizeof(text));
					strncpy(text, tmpval, 10);
					tmpval = strncat(text, GUI_TextEntryGetText(self.heap_memory_text), 10);
				}

				heap = strtoul(tmpval, NULL, 16);
			}
			break;
		}
	}

	for(int i = 0; i < sizeof(self.cdda_mode) >> 2; i++) {
		if(GUI_WidgetGetState(self.cdda_mode[i])) {
			if (i == CDDA_MODE_EXTENDED) {
				cdda_mode = getExtModeCDDA();
			} else {
				cdda_mode = i;
			}
			break;
		}
	}

	for(int i = 0; self.memory_chk[i]; i++) {

		if(GUI_WidgetGetState(self.memory_chk[i])) {

			memory = (char* )GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);

			if(strlen(memory) < 8) {
				memset(text, 0, sizeof(text));
				strncpy(text, memory, 10);
				memory = strncat(text, GUI_TextEntryGetText(self.memory_text), 10);
			}
			break;
		}
	}

	trim_spaces(ipbin->title, text, sizeof(ipbin->title));
	memset(result, 0, sizeof(result));

	int vmu_num = 0;

	if (GUI_WidgetGetState(self.vmu)) {
		vmu_num = atoi(GUI_TextEntryGetText(self.vmu_number));
	}

	snprintf(result, sizeof(result),
			"title = %s\ndevice = %s\ndma = %d\nasync = %d\ncdda = %08lx\n"
			"irq = %d\nlow = %d\nheap = %08lx\nfastboot = %d\ntype = %d\nmode = %d\nmemory = %s\n"
			"vmu = %d\nscrhotkey = %lx\n"
			"pa1 = %08lx\npv1 = %08lx\npa2 = %08lx\npv2 = %08lx\n",
			text, GUI_TextEntryGetText(self.device), GUI_WidgetGetState(self.dma), async,
			cdda_mode, GUI_WidgetGetState(self.irq), GUI_WidgetGetState(self.low), heap,
			GUI_WidgetGetState(self.fastboot), type, mode, memory,
			vmu_num, (uint32)(GUI_WidgetGetState(self.screenshot) ? SCREENSHOT_HOTKEY : 0),
			self.pa[0], self.pv[0], self.pa[1], self.pv[1]);

	fs_write(fd, result, strlen(result));
	fs_close(fd);

	return 0;
}

int isoLoader_LoadPreset() {

	if (!self.filename[0]) {
		isoLoader_DefaultPreset();
		return -1;
	}

	char *filename = makePresetFilename(GUI_FileManagerGetPath(self.filebrowser), self.md5);

	if (FileSize(filename) < 5) {
		isoLoader_DefaultPreset();
		return -1;
	}

	int use_dma = 0, emu_async = 16, emu_cdda = 0, use_irq = 0;
	int fastboot = 0, low = 0, emu_vmu = 0, scr_hotkey = 0;
	int boot_mode = BOOT_MODE_DIRECT;
	int bin_type = BIN_TYPE_AUTO;
	uint32 heap = HEAP_MODE_AUTO;
	char title[32] = "";
	char device[8] = "";
	char memory[12] = "0x8c000100";
	char heap_memory[12] = "";
	char patch_a[2][10];
	char patch_v[2][10];
	int i, len;
	char *name;
	memset(patch_a, 0, 2 * 10);
	memset(patch_v, 0, 2 * 10);

	isoldr_conf options[] = {
		{ "dma",      CONF_INT,   (void *) &use_dma    },
		{ "cdda",     CONF_INT,   (void *) &emu_cdda   },
		{ "irq",      CONF_INT,   (void *) &use_irq    },
		{ "low",      CONF_INT,   (void *) &low        },
		{ "vmu",      CONF_INT,   (void *) &emu_vmu    },
		{ "scrhotkey",CONF_INT,   (void *) &scr_hotkey },
		{ "heap",     CONF_STR,   (void *) &heap_memory},
		{ "memory",   CONF_STR,   (void *) memory      },
		{ "async",    CONF_INT,   (void *) &emu_async  },
		{ "mode",     CONF_INT,   (void *) &boot_mode  },
		{ "type",     CONF_INT,   (void *) &bin_type   },
		{ "title",    CONF_STR,   (void *) title       },
		{ "device",   CONF_STR,   (void *) device      },
		{ "fastboot", CONF_INT,   (void *) &fastboot   },
		{ "pa1",      CONF_STR,   (void *) patch_a[0]  },
		{ "pv1",      CONF_STR,   (void *) patch_v[0]  },
		{ "pa2",      CONF_STR,   (void *) patch_a[1]  },
		{ "pv2",      CONF_STR,   (void *) patch_v[1]  },
		{ NULL,       CONF_END,   NULL				   }
	};

	if (conf_parse(options, filename)) {
		ds_printf("DS_ERROR: Can't parse preset\n");
		isoLoader_DefaultPreset();
		return -1;
	}

	GUI_WidgetSetState(self.dma, use_dma);
	isoLoader_toggleDMA(self.dma);

	if (emu_async == 0) {
		GUI_WidgetSetState(self.async[0], 1);
		isoLoader_toggleAsync(self.async[0]);
	} else {
		for(int i = 1; i < sizeof(self.async) >> 2; i++) {
			int val = atoi(GUI_LabelGetText(GUI_ButtonGetCaption(self.async[i])));
			if(emu_async == val) {
				GUI_WidgetSetState(self.async[i], 1);
				isoLoader_toggleAsync(self.async[i]);
				break;
			}
		}
	}

	GUI_WidgetSetState(self.cdda, emu_cdda ? 1 : 0);
	if (emu_cdda >= CDDA_MODE_EXTENDED) {
		setExtModeCDDA(emu_cdda);
	} else {
		isoLoader_toggleCDDA(self.cdda_mode[emu_cdda]);
	}
	GUI_WidgetSetState(self.fastboot, fastboot);
	GUI_WidgetSetState(self.irq, use_irq);
	GUI_WidgetSetState(self.vmu, emu_vmu ? 1 : 0);
	GUI_WidgetSetState(self.screenshot, scr_hotkey ? 1 : 0);
	GUI_WidgetSetState(self.low, low);

	if (emu_vmu) {
		char num[8];
		sprintf(num, "%03d", emu_vmu);
		GUI_TextEntrySetText(self.vmu_number, num);
	}

	heap = strtoul(heap_memory, NULL, 16);

	if (heap <= HEAP_MODE_MAPLE) {
		GUI_WidgetSetState(self.heap[heap], 1);
		isoLoader_toggleHeap(self.heap[heap]);
	} else {
		for (int i = HEAP_MODE_MAPLE; i < sizeof(self.heap) >> 2; i++) {
			if (!self.heap[i]) {
				break;
			}
			name = (char *)GUI_ObjectGetName((GUI_Object *)self.heap[i]);
			len = strlen(name);

			if (!strncmp(name, heap_memory, sizeof(heap_memory)) || len < 8) {
				GUI_WidgetSetState(self.heap[i], 1);
				isoLoader_toggleHeap(self.heap[i]);
				if (len < 8) {
					GUI_TextEntrySetText(self.heap_memory_text, &heap_memory[4]);
				}
				break;
			}
		}
	}
	
	GUI_WidgetSetState(self.os_chk[bin_type], 1);
	isoLoader_toggleOS(self.os_chk[bin_type]);
	
	GUI_WidgetSetState(self.boot_mode_chk[boot_mode], 1);
	isoLoader_toggleBootMode(self.boot_mode_chk[boot_mode]);

	if (strlen(title) > 0) {
		GUI_LabelSetText(self.title, title);
		vmu_draw_string(title);
	}

	if (strlen(device) > 0) {
		GUI_TextEntrySetText(self.device, device);
	} else {
		GUI_TextEntrySetText(self.device, "auto");
	}

	for (i = 0; self.memory_chk[i]; i++) {

		name = (char *)GUI_ObjectGetName((GUI_Object *)self.memory_chk[i]);
		len = strlen(name);

		if (!strncmp(name, memory, sizeof(memory)) || len < 8) {
			GUI_WidgetSetState(self.memory_chk[i], 1);
			isoLoader_toggleMemory(self.memory_chk[i]);
			if (len < 8) {
				GUI_TextEntrySetText(self.memory_text, &memory[4]);
			}
			break;
		}
	}

	for(int i = 0; i < sizeof(self.wpa) >> 2; ++i) {
		if (patch_a[i][1] != '0' && strlen(patch_a[i]) == 8 && strlen(patch_v[i])
		) {
			self.pa[i] = strtoul(patch_a[i], NULL, 16);
			self.pv[i] = strtoul(patch_v[i], NULL, 16);
			GUI_TextEntrySetText(self.wpa[i], patch_a[i]);
			GUI_TextEntrySetText(self.wpv[i], patch_v[i]);
		} else {
			self.pa[i] = 0;
			self.pv[i] = 0;
		}
	}

	return 0;
}


void isoLoader_Init(App_t *app) {

	GUI_Widget *w, *b;
	GUI_Callback *cb;

	if(app != NULL) {
		
		memset(&self, 0, sizeof(self));
		
		self.app = app;
		self.current_dev = -1;
		self.current_item = -1;
		self.current_item_dir = -1;
		self.sector_size = 2048;

		self.btn_dev[APP_DEVICE_CD]  = APP_GET_WIDGET("btn_cd");
		self.btn_dev[APP_DEVICE_SD]  = APP_GET_WIDGET("btn_sd");
		self.btn_dev[APP_DEVICE_IDE] = APP_GET_WIDGET("btn_hdd");
		self.btn_dev[APP_DEVICE_PC]  = APP_GET_WIDGET("btn_pc");

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

		self.default_cover = self.current_cover = APP_GET_SURFACE("cover");

		self.pages    = APP_GET_WIDGET("pages");
		self.settings = APP_GET_WIDGET("settings");
		self.games    = APP_GET_WIDGET("games");
		self.link     = APP_GET_WIDGET("link");
		self.extensions = APP_GET_WIDGET("extensions");

		self.filebrowser  = APP_GET_WIDGET("file_browser");
		self.cover_widget = APP_GET_WIDGET("cover_image");
		self.title        = APP_GET_WIDGET("game_title");
		self.btn_run      = APP_GET_WIDGET("run_iso");
		self.run_pane     = APP_GET_WIDGET("run-panel");
		
		self.preset        = APP_GET_WIDGET("preset-checkbox");
		self.dma           = APP_GET_WIDGET("dma-checkbox");
		self.cdda          = APP_GET_WIDGET("cdda-checkbox");
		self.irq           = APP_GET_WIDGET("irq-checkbox");
		self.low           = APP_GET_WIDGET("low-checkbox");
		self.fastboot      = APP_GET_WIDGET("fastboot-checkbox");

		self.vmu           = APP_GET_WIDGET("vmu-checkbox");
		self.vmu_number    = APP_GET_WIDGET("vmu-number");
		self.vmu_create    = APP_GET_WIDGET("vmu-create-checkbox");
		self.screenshot    = APP_GET_WIDGET("screenshot-checkbox");
		
		self.options_panel	  = APP_GET_WIDGET("options-panel");
		self.wpa[0]	          = APP_GET_WIDGET("pa1-text");
		self.wpa[1]	          = APP_GET_WIDGET("pa2-text");
		self.wpv[0]	          = APP_GET_WIDGET("pv1-text");
		self.wpv[1]	          = APP_GET_WIDGET("pv2-text");

		self.icosizebtn[0]	  = APP_GET_WIDGET("48x48");
		self.icosizebtn[1]	  = APP_GET_WIDGET("64x64");
		self.icosizebtn[2]	  = APP_GET_WIDGET("96x96");
		self.icosizebtn[3]	  = APP_GET_WIDGET("128x128");
		self.icosizebtn[4]	  = APP_GET_WIDGET("256x256");
		self.rotate180	      = APP_GET_WIDGET("rotate-link");
		self.linktext	      = APP_GET_WIDGET("link-text");
		self.btn_hidetext     = APP_GET_WIDGET("hide-name");
		self.save_link_btn    = APP_GET_WIDGET("save-link-btn");
		self.save_link_txt    = APP_GET_WIDGET("save-link-txt");
		self.wlnkico	      = APP_GET_WIDGET("link-icon");
		self.stdico           = APP_GET_SURFACE("stdico");

		w = APP_GET_WIDGET("async-panel");
		self.async[0] = GUI_ContainerGetChild(w, 1);
		int sz = (sizeof(self.async) >> 2) - 1;

		for(int i = 0; i < sz; i++) {

			b = GUI_ContainerGetChild(w, i + 2);
			int val = atoi( GUI_LabelGetText( GUI_ButtonGetCaption(b) ) );

			self.async[(val - 1) < sz ? val : sz] = b;
		}

		self.async_label   = APP_GET_WIDGET("async-label");
		self.device        = APP_GET_WIDGET("device");

		w = APP_GET_WIDGET("switch-panel");

		for(int i = 0; i < (sizeof(self.options_switch) >> 2); i++) {
			self.options_switch[i] = GUI_ContainerGetChild(w, i);
		}

		w = APP_GET_WIDGET("boot-mode-panel");
		
		for(int i = 0; i < (sizeof(self.boot_mode_chk) >> 2); i++) {
			self.boot_mode_chk[i] = GUI_ContainerGetChild(w, i);
		}

		w = APP_GET_WIDGET("heap-memory-panel");
		
		for(int i = 0, j = 0; i <= GUI_ContainerGetCount(w); i++) {

			b = GUI_ContainerGetChild(w, i);

			if(GUI_WidgetGetType(b) == WIDGET_TYPE_BUTTON) {
				self.heap[j++] = b;
				if(j == sizeof(self.heap) / 4)
					break;
			}
		}

		w = APP_GET_WIDGET("cdda-mode-panel");

		for(int i = 0; i < (sizeof(self.cdda_mode) >> 2); i++) {
			self.cdda_mode[i] = GUI_ContainerGetChild(w, i);
		}

		w = APP_GET_WIDGET("cdda-ext-mode-src-panel");

		for(int i = 0; i < (sizeof(self.cdda_mode_src) >> 2); i++) {
			self.cdda_mode_src[i] = GUI_ContainerGetChild(w, i + 1);
		}

		w = APP_GET_WIDGET("cdda-ext-mode-dst-panel");

		for(int i = 0; i < (sizeof(self.cdda_mode_dst) >> 2); i++) {
			self.cdda_mode_dst[i] = GUI_ContainerGetChild(w, i + 1);
		}

		w = APP_GET_WIDGET("cdda-ext-mode-pos-panel");

		for(int i = 0; i < (sizeof(self.cdda_mode_pos) >> 2); i++) {
			self.cdda_mode_pos[i] = GUI_ContainerGetChild(w, i + 1);
		}

		w = APP_GET_WIDGET("cdda-ext-mode-ch-panel");

		for(int i = 0; i < (sizeof(self.cdda_mode_ch) >> 2); i++) {
			self.cdda_mode_ch[i] = GUI_ContainerGetChild(w, i + 1);
		}

		w = APP_GET_WIDGET("os-panel");

		for(int i = 0; i < (sizeof(self.os_chk) >> 2); i++) {
			self.os_chk[i] = GUI_ContainerGetChild(w, i + 1);
		}

		w = APP_GET_WIDGET("boot-memory-panel");

		for(int i = 0, j = 0; i < GUI_ContainerGetCount(w); i++) {
			b = GUI_ContainerGetChild(w, i);

			if(GUI_WidgetGetType(b) == WIDGET_TYPE_BUTTON) {
				self.memory_chk[j++] = b;
				if(j == sizeof(self.memory_chk) / 4) {
					break;
				}
			}
		}

		self.memory_text = APP_GET_WIDGET("boot-memory-text");
		self.heap_memory_text = APP_GET_WIDGET("heap-memory-text");

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

		/* Checking for arguments from a executor */
		if(app->args != NULL) {
			
			char *name = getFilePath(app->args);
			
			if(name) {
				
				GUI_FileManagerSetPath(self.filebrowser, name);
				free(name);
			}
			
			self.have_args = true;
			
		} else {
			self.have_args = false;
		}
		
		isoLoader_DefaultPreset();
		w =  APP_GET_WIDGET("version");
		
		if(w) {
			char vers[32];
			snprintf(vers, sizeof(vers), "v%s", app->ver);
			GUI_LabelSetText(w, vers);
		}

		isoLoader_ResizeUI();

	} else {
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}

void isoLoader_ResizeUI()
{
	SDL_Surface *screen = GetScreen();
	GUI_Widget *filemanager_bg = APP_GET_WIDGET("filemanager_bg");
	GUI_Widget *cover_bg = APP_GET_WIDGET("cover_bg");
	GUI_Widget *title_panel = APP_GET_WIDGET("title_panel");

	int screen_width = GetScreenWidth();
	SDL_Rect filemanager_bg_rect = GUI_WidgetGetArea(filemanager_bg);
	SDL_Rect cover_bg_rect = GUI_WidgetGetArea(cover_bg);
	SDL_Rect title_rect = GUI_WidgetGetArea(title_panel);

	const int gap = 19;

	int filemanager_bg_width = screen_width - cover_bg_rect.w - gap * 3;
	int filemanager_bg_height = filemanager_bg_rect.h;

	GUI_Surface *fm_bg = GUI_SurfaceCreate("fm_bg", 
											screen->flags, 
											filemanager_bg_width, 
											filemanager_bg_height,
											screen->format->BitsPerPixel, 
											screen->format->Rmask, 
											screen->format->Gmask, 
											screen->format->Bmask, 
											screen->format->Amask);

	GUI_SurfaceFill(fm_bg, NULL, GUI_SurfaceMapRGB(fm_bg, 255, 255, 255));
	SDL_Rect grayRect;
	grayRect.x = 6;
	grayRect.y = 6;
	grayRect.w = filemanager_bg_width - grayRect.x * 2;
	grayRect.h = filemanager_bg_height - grayRect.y * 2;
	GUI_SurfaceFill(fm_bg, &grayRect, GUI_SurfaceMapRGB(fm_bg, 0xCC, 0xE4, 0xF0));

	GUI_PanelSetBackground(filemanager_bg, fm_bg);
	GUI_ObjectDecRef((GUI_Object *) fm_bg);

	int delta_x = filemanager_bg_width + gap * 2 - cover_bg_rect.x;

	GUI_WidgetSetPosition(cover_bg, cover_bg_rect.x + delta_x, cover_bg_rect.y);
	GUI_WidgetSetPosition(title_panel, title_rect.x + delta_x, title_rect.y);
	GUI_WidgetSetSize(filemanager_bg, filemanager_bg_width, filemanager_bg_height);
	GUI_FileManagerResize(self.filebrowser, 
						  filemanager_bg_width - grayRect.x * 2, 
						  filemanager_bg_height - grayRect.y * 2);
}

void isoLoader_Shutdown(App_t *app) {
	(void)app;
	StopCDDATrack();

	if(self.isoldr) {
		free(self.isoldr);
	}
}

void isoLoader_Exit(GUI_Widget *widget) {

	(void)widget;
	App_t *app = NULL;
	StopCDDATrack();

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
