/* DreamShell ##version##

   module.c - Main app module
   Copyright (C)2011-2016 SWAT 

*/

#include <ds.h>
#include <drivers/rtc.h>

DEFAULT_MODULE_EXPORTS(app_main);

#define ICON_CELL_PADDING 10

static struct {

	App_t *app;
	
	GUI_Font *font;
	GUI_Widget *panel;
	SDL_Rect panel_area;

	int x;
	int y;
	int cur_x;
	int col_width;
	
	struct tm datetime;
	GUI_Widget *dateWidget;
	GUI_Widget *timeWidget;
	
} self;

typedef struct script_item {
	
	char name[64];
	char file[MAX_FN_LEN];
	
} script_item_t;


static GUI_Surface *CreateHighlight(GUI_Surface *src, int w, int h) {

	SDL_Surface *screen = GetScreen();
	GUI_Surface *s = GUI_SurfaceCreate("icon", 
										screen->flags, 
										w + 4, h + 4, 
										screen->format->BitsPerPixel, 
										screen->format->Rmask, 
										screen->format->Gmask, 
										screen->format->Bmask, 
										screen->format->Amask);
	
	GUI_SurfaceSetAlpha(s,  SDL_SRCALPHA, 100);
	GUI_SurfaceFill(s, NULL, GUI_SurfaceMapRGB(s, 0, 0, 0));
	//GUI_SurfaceBoxRouded(s, 0, 0, w, h, 10, 0xEEEEEE44);
	SDL_Rect dst;
	dst.x = dst.y = 2;
	dst.w = w;
	dst.h = h;
	GUI_SurfaceBlit(src, NULL, s, &dst);

	return s;
}


static void OpenAppCB(void *param) {
	
	App_t *app = GetAppById((int)param);
	
	if(app != NULL) {
		OpenApp(app, NULL);
	}
}


static void RunScriptCB(void *param) {
	
	script_item_t *si = (script_item_t *)param;
	int c = si->file[strlen(si->file) - 3];
	
	switch(c) {
		case 'l':
			LuaDo(LUA_DO_FILE, si->file, GetLuaState());
			break;
		case 'd':
		default:
			dsystem_script(si->file);
			break;
	}
}

static void AddToList(const char *name, const char *icon, 
						GUI_CallbackFunction *callback, 
						GUI_CallbackFunction *free_data, 
						void *callback_data) {

	SDL_Rect ts;

	if(name) {
		ts = GUI_FontGetTextSize(self.font, name);
		ts.w += 4;
	} else {
		ts.w = 0;
	}

	GUI_Surface *s = GUI_SurfaceLoad(icon);
	int w = ts.w + GUI_SurfaceGetWidth(s);
	int h = GUI_SurfaceGetHeight(s);
	int dpad = ICON_CELL_PADDING * 2;
	
	if((self.y + h + dpad) > self.panel_area.h) {

		self.x += self.col_width + ICON_CELL_PADDING;
		self.y = dpad;
		self.col_width = 0;
		
		if((self.x % self.panel_area.w) + w + ICON_CELL_PADDING > self.panel_area.w) {
			self.x += (self.panel_area.w - (self.x % self.panel_area.w)) + dpad;
		}
	}
	
	GUI_Widget *b = GUI_ButtonCreate(name ? name : "icon", self.x, self.y, w + 4, h + 4);

	if(s != NULL) {
		GUI_Surface *sh = CreateHighlight(s, w, h);

		GUI_ButtonSetNormalImage(b, s);
		GUI_ButtonSetHighlightImage(b, sh);
		GUI_ButtonSetPressedImage(b, s);
		GUI_ButtonSetDisabledImage(b, s);

		GUI_ObjectDecRef((GUI_Object *) s);
		GUI_ObjectDecRef((GUI_Object *) sh);
	}

	GUI_Callback *c = GUI_CallbackCreate(callback, free_data, callback_data);
	GUI_ButtonSetClick(b, c);
	GUI_ObjectDecRef((GUI_Object *) c);

	if(name) {
		GUI_Widget *l = GUI_LabelCreate(name, 0, 0, w, h, self.font, name);
		GUI_LabelSetTextColor(l, 0, 0, 0);
		GUI_WidgetSetAlign(l, WIDGET_HORIZ_RIGHT | WIDGET_VERT_CENTER);
		GUI_ButtonSetCaption(b, l);
		GUI_ObjectDecRef((GUI_Object *) l);
	}

	GUI_ContainerAdd(self.panel, b);
	GUI_ObjectDecRef((GUI_Object *) b);

	self.y += (h + ICON_CELL_PADDING);
	
	if(self.col_width < w) {
		self.col_width = w;
	}
}


static void BuildAppList() {

	file_t fd;
	dirent_t *ent;
	char path[MAX_FN_LEN];
	int plen, elen, type;
	App_t *app;
	Item_list_t *applist = GetAppList();
	const char *app_name = lib_get_name() + 4;
	
	if(applist != NULL) {
	
		Item_t *item = listGetItemFirst(applist);

		while(item != NULL) {
			
			app = (App_t*)item->data;
			
			if(strncasecmp(app->name, app_name, sizeof(app->name))) {
				AddToList(app->name, app->icon, (GUI_CallbackFunction *)OpenAppCB, NULL, (void*)app->id);
			}
			
			item = listGetItemNext(item);
		}
	}
	
	snprintf(path, MAX_FN_LEN, "%s/apps/%s/scripts", getenv("PATH"), app_name);
	fd = fs_open(path, O_RDONLY | O_DIR);
	
	if(fd == FILEHND_INVALID)
		return;
		
	while((ent = fs_readdir(fd)) != NULL) {
		
		elen = strlen(ent->name);
		type = elen > 3 ? ent->name[elen - 3] : 'd';
		
		if(!ent->attr && (type == 'l' || type == 'd')) {
		
			script_item_t *si = (script_item_t *) calloc(1, (sizeof(script_item_t)));
			
			if(si == NULL)
				break;
			
			snprintf(si->file, MAX_FN_LEN, "%s/apps/%s/scripts/%s", getenv("PATH"), app_name, ent->name);
			snprintf(path, MAX_FN_LEN, "%s/apps/%s/images/%s", getenv("PATH"), app_name, ent->name);
			plen = strlen(path);

			path[plen - 3] = 'p';
			path[plen - 2] = 'n';
			path[plen - 1] = 'g';
			
			if(!FileExists(path)) {
				
				path[plen - 3] = 'p';
				path[plen - 2] = 'v';
				path[plen - 1] = 'r';
				
				if(!FileExists(path)) {
				
					path[plen - 3] = 'b';
					path[plen - 2] = 'm';
					path[plen - 1] = 'p';
				
					if(!FileExists(path)) {
						snprintf(path, MAX_FN_LEN, "%s/gui/icons/normal/%s.png", 
									getenv("PATH"), (type == 'l' ? "lua" : "script"));
					}
				}
			}

			elen -= 4;
			
			if(elen > sizeof(si->name))
				elen = sizeof(si->name);
			
			strncpy(si->name, ent->name, elen);
			si->name[elen] = '\0';
			
			AddToList((si->name[0] != '_' ? si->name : NULL), path, RunScriptCB, free, (void *)si);
		}
	}
	
	fs_close(fd);
}


static void ShowVersion(GUI_Widget *widget) {
	
	if(!widget) {
		return;
	}
	
	char vers[32];	
	snprintf(vers, sizeof(vers), "%s %s", getenv("OS"), getenv("VERSION"));
	GUI_LabelSetText(widget, vers);
}


static void ShowDateTime(int force) {

	char str[32];
	struct timeval tv;
	struct tm *datetime;

	rtc_gettimeofday(&tv);
	datetime = localtime(&tv.tv_sec);
	
	if(force || datetime->tm_mday != self.datetime.tm_mday) {
		
		switch(flashrom_get_region_only()) {
			case FLASHROM_REGION_JAPAN:
				snprintf(str, sizeof(str), "%04d-%02d-%02d", datetime->tm_year + 1900, datetime->tm_mon + 1, datetime->tm_mday);
				break;
			case FLASHROM_REGION_US:
				snprintf(str, sizeof(str), "%02d/%02d/%04d", datetime->tm_mon + 1, datetime->tm_mday, datetime->tm_year + 1900);
				break;
			case FLASHROM_REGION_EUROPE:
			default:
				snprintf(str, sizeof(str), "%02d.%02d.%04d", datetime->tm_mday, datetime->tm_mon + 1, datetime->tm_year + 1900);
				break;
		}
		
		GUI_LabelSetText(self.dateWidget, str);
	}

	if(force || datetime->tm_min != self.datetime.tm_min) {
		snprintf(str, sizeof(str), "%02d:%02d", datetime->tm_hour, datetime->tm_min);
		GUI_LabelSetText(self.timeWidget, str);
	}
	
	memcpy(&self.datetime, datetime, sizeof(*datetime));
}

static void *ClockThread() {
	
	while(self.app->state & APP_STATE_OPENED) {
		ShowDateTime(0);
		thd_sleep(250);
	}

	return NULL;
}

/**
 * Global functions
 */

void MainApp_SlideLeft() {
	if(self.cur_x > 0) {

		for(int x = 0; x < self.panel_area.w; x += 10) {
			self.cur_x -= 10;
			GUI_PanelSetXOffset(self.panel, self.cur_x);
			thd_sleep(5);
		}
		
		thd_sleep(50);
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_SlideRight() {
	if(self.x > self.cur_x) {

		for(int x = 0; x < self.panel_area.w; x += 10) {
			self.cur_x += 10;
			GUI_PanelSetXOffset(self.panel, self.cur_x);
			thd_sleep(5);
		}
		
		thd_sleep(50);
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_Init(App_t *app) {
	
	if(app != NULL) {
		
		memset(&self, 0, sizeof(self));
		self.x = self.y = (ICON_CELL_PADDING * 2);
		self.app = app;

		self.font = APP_GET_FONT("arial");
		self.panel = APP_GET_WIDGET("app-list");
		
		if(self.font && self.panel) {
			self.panel_area = GUI_WidgetGetArea(self.panel);
			BuildAppList();
		} else {
			ds_printf("DS_ERROR: Couldt'n find font and app-list panel\n");
			return;
		}
		
		ShowVersion(APP_GET_WIDGET("version"));
		
		self.dateWidget = APP_GET_WIDGET("date");
		self.timeWidget = APP_GET_WIDGET("time");
		
		if(self.dateWidget && self.timeWidget) {
			ShowDateTime(1);
			self.app->thd = thd_create(0, ClockThread, NULL);
		}
	}
}
