/* DreamShell ##version##

   module.c - Settings app module
   Copyright (C)2016 SWAT 
*/

#include "ds.h"

DEFAULT_MODULE_EXPORTS(app_settings);

/* The first child of panel with checkboxes it's a label */
#define WIDGET_VALUE_OFFSET 1

enum {
	NATIVE_MODE_AUTO = 0,
	NATIVE_MOVE_PAL,
	NATIVE_MOVE_NTSC,
	NATIVE_MOVE_VGA
};

enum {
	SCREEN_MODE_4_3 = 0,
	SCREEN_MODE_3_2,
	SCREEN_MODE_16_10,
	SCREEN_MOVE_16_9
};

enum {
	SCREEN_FILTER_AUTO = 0,
	SCREEN_FILTER_NEAREST,
	SCREEN_FILTER_BILINEAR,
	SCREEN_FILTER_TRILINEAR
};

static struct {
	
	App_t *app;
	Settings_t *settings;
	GUI_Widget *pages;
	GUI_Widget *sysdate[6];
	
} self;

static void SetupVideoSettings();
static void SetupBootSettings();

static int UncheckBesides(GUI_Widget *widget, char *label) {

	int index = 0;
	GUI_Widget *panel, *child;
	
	if(label) {

		panel = widget;
		
		for(int i = 0; i < GUI_ContainerGetCount(panel); ++i) {
			
			widget = GUI_ContainerGetChild(panel, i);
			GUI_Widget *l = GUI_ToggleButtonGetCaption(widget);
			
			if(!strcasecmp(label, GUI_LabelGetText(l))) {
				break;
			}
		}
		
	} else {
		panel = GUI_WidgetGetParent(widget);
	}
	
	if(!GUI_WidgetGetState(widget)) {
		GUI_WidgetSetState(widget, 1);
	}
	
	for(int i = 0; i < GUI_ContainerGetCount(panel); ++i) {
		
		child = GUI_ContainerGetChild(panel, i);
		
		if(widget != child) {
			GUI_WidgetSetState(child, 0);
		} else {
			index = i;
		}
	}

	return index - WIDGET_VALUE_OFFSET;
}


void SettingsApp_Init(App_t *app) {

	if(app != NULL) {

		memset(&self, 0, sizeof(self));
		self.app = app;
		self.settings = GetSettings();
		self.pages = APP_GET_WIDGET("pages");
		self.sysdate[0] = APP_GET_WIDGET("year");
		self.sysdate[1] = APP_GET_WIDGET("month");
		self.sysdate[2] = APP_GET_WIDGET("day");
		self.sysdate[3] = APP_GET_WIDGET("hours");
		self.sysdate[4] = APP_GET_WIDGET("minute");
		
		SetupVideoSettings();
		SetupBootSettings();
	}
}


void SettingsApp_ShowPage(GUI_Widget *widget) {
	
	const char *object_name, *name_ptr;
	char name[64];
	size_t size;
	
	ScreenFadeOut();
	object_name = GUI_ObjectGetName((GUI_Object *)widget);
	name_ptr = strchr(object_name, '-');
	size = (name_ptr - object_name);
	
	if(size < sizeof(name) - 5) {

		strncpy(name, object_name, size);
		strncpy(name + size, "-page", 6);

		thd_sleep(200);
		GUI_CardStackShow(self.pages, name);
	}
	
	ScreenFadeIn();
}


void SettingsApp_ResetSettings(GUI_Widget *widget) {

	ResetSettings();
	SetupBootSettings();
	SetupVideoSettings();
	
	SetScreenMode(self.settings->video.virt_width, self.settings->video.virt_height, 0.0f, 0.0f, 1.0f);
	SetScreenFilter(self.settings->video.tex_filter);
}


void SettingsApp_ToggleNativeMode(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case NATIVE_MODE_AUTO:
			memset(&self.settings->video.mode, 0, sizeof(self.settings->video.mode));
//			SetVideoMode(-1);
			break;
		case NATIVE_MOVE_PAL:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_PAL_IL], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_PAL_IL);
			break;
		case NATIVE_MOVE_NTSC:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_NTSC_IL], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_NTSC_IL);
			break;
		case NATIVE_MOVE_VGA:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_VGA], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_VGA);
			break;
		default:
			return;
	}
}


void SettingsApp_ToggleScreenMode(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case SCREEN_MODE_4_3:
			self.settings->video.virt_width = 640;
			break;
		case SCREEN_MODE_3_2:
			self.settings->video.virt_width = 720;
			break;
		case SCREEN_MODE_16_10:
			self.settings->video.virt_width = 768;
			break;
		case SCREEN_MOVE_16_9:
			self.settings->video.virt_width = 854;
			break;
		default:
			return;
	}
	
	self.settings->video.virt_height = 480;
	SetScreenMode(self.settings->video.virt_width, self.settings->video.virt_height, 0.0f, 0.0f, 1.0f);
}


void SettingsApp_ToggleScreenFilter(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case SCREEN_FILTER_AUTO:
			self.settings->video.tex_filter = -1;
			break;
		case SCREEN_FILTER_NEAREST:
			self.settings->video.tex_filter = PVR_FILTER_NEAREST;
			break;
		case SCREEN_FILTER_BILINEAR:
			self.settings->video.tex_filter = PVR_FILTER_BILINEAR;
			break;
//		case SCREEN_FILTER_TRILINEAR:
//			self.settings->video.tex_filter = PVR_FILTER_TRILINEAR1;
//			break;
		default:
			return;
	}
	
	SetScreenFilter(self.settings->video.tex_filter);
}


static void SetupVideoSettings() {
	
	GUI_Widget *panel;
	int value = NATIVE_MODE_AUTO;

	panel = APP_GET_WIDGET("display-native-mode");
	
	if(self.settings->video.mode.width > 0) {

		for(int i = 0; i < DM_SENTINEL; ++i) {

			if(!memcmp(&self.settings->video.mode, &vid_builtin[i], sizeof(vid_builtin[i]))) {
				
				switch(i) {
					case DM_640x480_PAL_IL:
						value = NATIVE_MOVE_PAL;
						break;
					case DM_640x480_NTSC_IL:
						value = NATIVE_MOVE_NTSC;
						break;
					case DM_640x480_VGA:
						value = NATIVE_MOVE_VGA;
						break;
					default:
						break;
				}
				break;
			}
		}
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
	
	panel = APP_GET_WIDGET("display-screen-mode");
	
	switch(self.settings->video.virt_width) {
		case 640:
			value = SCREEN_MODE_4_3;
			break;
		case 720:
			value = SCREEN_MODE_3_2;
			break;
		case 854:
			value = SCREEN_MOVE_16_9;
			break;
		default:
			value = SCREEN_MODE_4_3;
			break;
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
	
	panel = APP_GET_WIDGET("display-screen-filter");
	
	switch(self.settings->video.tex_filter) {
		case -1:
			value = SCREEN_FILTER_AUTO;
			break;
		case PVR_FILTER_NEAREST:
			value = SCREEN_FILTER_AUTO;
			break;
		case PVR_FILTER_BILINEAR:
			value = SCREEN_FILTER_AUTO;
			break;
//			case PVR_FILTER_TRILINEAR1:
//				value = SCREEN_FILTER_TRILINEAR;
//				break;
		default:
			value = SCREEN_FILTER_AUTO;
			break;
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
}



void SettingsApp_ToggleApp(GUI_Widget *widget) {
	UncheckBesides(widget, NULL);
	strncpy(self.settings->app, GUI_ObjectGetName((GUI_Object *)widget), sizeof(self.settings->app));
}


void SettingsApp_ToggleRoot(GUI_Widget *widget) {

	UncheckBesides(widget, NULL);
	GUI_Widget *label = GUI_ToggleButtonGetCaption(widget);
	
	if(label) {
		char *value = GUI_LabelGetText(label);
		strncpy(self.settings->root, value, sizeof(self.settings->root));
	}
}


void SettingsApp_ToggleStartup(GUI_Widget *widget) {

	UncheckBesides(widget, NULL);
	GUI_Widget *label = GUI_ToggleButtonGetCaption(widget);
	
	if(label) {
		char *value = GUI_LabelGetText(label);
		strncpy(self.settings->startup, value, sizeof(self.settings->root));
	}
}


static void SetupBootSettings() {

	Item_list_t *applist      = GetAppList();
	GUI_Widget  *panel        = APP_GET_WIDGET("boot-root");
	GUI_Font    *font         = APP_GET_FONT("arial");
	GUI_Surface *check_on     = APP_GET_SURFACE("check-on");
	GUI_Surface *check_on_hl  = APP_GET_SURFACE("check-on-hl");
	GUI_Surface *check_off    = APP_GET_SURFACE("check-off");
	GUI_Surface *check_off_hl = APP_GET_SURFACE("check-off-hl");
	
	if(self.settings->root[0] == 0) {
		UncheckBesides(GUI_ContainerGetChild(panel, WIDGET_VALUE_OFFSET), NULL);
	} else {
		UncheckBesides(panel, self.settings->root);
	}

	panel = APP_GET_WIDGET("boot-startup");
	UncheckBesides(panel, self.settings->startup);
	
	panel = APP_GET_WIDGET("boot-app");
	
	if(applist && panel) {
		
		if(GUI_ContainerGetCount(panel) > 1) {
			UncheckBesides(panel, self.settings->app);
			return;
		}
	
		Item_t *item = listGetItemFirst(applist);

		while(item != NULL) {
			
			App_t *app = (App_t *)item->data;
			item = listGetItemNext(item);

			SDL_Rect ts = GUI_FontGetTextSize(font, app->name);

			int w = ts.w + GUI_SurfaceGetWidth(check_on) + 16;
			int h = GUI_SurfaceGetHeight(check_on) + 6;

			GUI_Widget *b = GUI_ToggleButtonCreate(app->name, 0, 0, w, h);
			GUI_ToggleButtonSetOnNormalImage(b, check_on);
			GUI_ToggleButtonSetOnHighlightImage(b, check_on_hl);
			GUI_ToggleButtonSetOffNormalImage(b, check_off);
			GUI_ToggleButtonSetOffHighlightImage(b, check_off_hl);
			
			GUI_Callback *c = GUI_CallbackCreate((GUI_CallbackFunction *)SettingsApp_ToggleApp, NULL, b);
			GUI_ToggleButtonSetClick(b, c);
			GUI_ObjectDecRef((GUI_Object *) c);

			if(!strncasecmp(app->name, self.settings->app, sizeof(app->name))) {
				GUI_WidgetSetState(b, 1);
			} else {
				GUI_WidgetSetState(b, 0);
			}

			GUI_Widget *l = GUI_LabelCreate(app->name, 22, 0, w, h - 5, font, app->name);
			GUI_LabelSetTextColor(l, 0, 0, 0);
			GUI_WidgetSetAlign(l, WIDGET_HORIZ_LEFT | WIDGET_VERT_CENTER);
			GUI_ButtonSetCaption(b, l);
			GUI_ObjectDecRef((GUI_Object *) l);

			GUI_ContainerAdd(panel, b);
			GUI_ObjectDecRef((GUI_Object *) b);
		}
	}
}

void SettingsApp_TimeChange(GUI_Widget *widget)
{
	time_t t;
    struct tm tm;
	
	if(strcmp(GUI_ObjectGetName(widget),"get-time") == 0)
	{
		char buf[5];
		t = (((g2_read_32(0xa0710000) & 0xffff) << 16) | (g2_read_32(0xa0710004) & 0xffff))-631152000;
		localtime_r(&t, &tm);
		
		sprintf(buf,"%04d",tm.tm_year + 1900);
		GUI_TextEntrySetText(self.sysdate[0], buf);
		sprintf(buf,"%02d",tm.tm_mon + 1);
		GUI_TextEntrySetText(self.sysdate[1], buf);
		sprintf(buf,"%02d",tm.tm_mday);
		GUI_TextEntrySetText(self.sysdate[2], buf);
		sprintf(buf,"%02d",tm.tm_hour);
		GUI_TextEntrySetText(self.sysdate[3], buf);
		sprintf(buf,"%02d",tm.tm_min);
		GUI_TextEntrySetText(self.sysdate[4], buf);
	}
	else if(strcmp(GUI_ObjectGetName(widget),"set-time") == 0)
	{
		tm.tm_sec = 0;
		tm.tm_min = atoi(GUI_TextEntryGetText(self.sysdate[4]));
		tm.tm_hour = atoi(GUI_TextEntryGetText(self.sysdate[3]));
		tm.tm_mday = atoi(GUI_TextEntryGetText(self.sysdate[2]));
		tm.tm_mon = atoi(GUI_TextEntryGetText(self.sysdate[1])) - 1;
		tm.tm_year = atoi(GUI_TextEntryGetText(self.sysdate[0])) - 1900;
		
		t = mktime(&tm);
		t += 631152000;

		g2_write_32(0xa0710008, 1);
		g2_write_32(0xa0710004, t & 0xffff);
		g2_write_32(0xa0710000, (t >> 16) & 0xffff);

	}
}

void SettingsApp_Time(GUI_Widget *widget)
{
	char wname[32];
	sprintf(wname,"%s",GUI_ObjectGetName(widget));
	int entryvar = atoi(GUI_TextEntryGetText(widget));
	
	switch (wname[strlen(wname)-1])
	{	
		case 'r':
			if(entryvar < 1998) GUI_TextEntrySetText(widget, "1998");
			if(entryvar > 2086) GUI_TextEntrySetText(widget, "2086");
			break;
		case 'h':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "01");
			if(entryvar > 12) GUI_TextEntrySetText(widget, "12");
			break;
		case 'y':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "01");
			if(entryvar > 31) GUI_TextEntrySetText(widget, "31");
			break;
		case 's':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "00");
			if(entryvar > 23) GUI_TextEntrySetText(widget, "23");
			break;
		case 'e':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "00");
			if(entryvar > 59) GUI_TextEntrySetText(widget, "59");
			break;
	}
}

void SettingsApp_Time_Clr(GUI_Widget *widget)
{
	GUI_TextEntrySetText(widget, "");
}

