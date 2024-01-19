/* DreamShell ##version##

   module.c - Main app module
   Copyright (C)2011-2016, 2024 SWAT 

*/

#include <ds.h>
#include <drivers/rtc.h>

DEFAULT_MODULE_EXPORTS(app_main);

#define ICON_CELL_PADDING_X 30
#define ICON_CELL_PADDING_Y 20
#define ICON_HIGHLIGHT_PADDING 8

static struct {

	App_t *app;
	
	GUI_Font *font;
	GUI_Widget *panel;
	SDL_Rect panel_area;

	int x;
	int y;
	int cur_x;
	int col_width;
	int pages;

	struct tm datetime;
	GUI_Widget *dateWidget;
	GUI_Widget *timeWidget;
	
} self;

typedef struct script_item {
	
	char name[64];
	char file[NAME_MAX];
	
} script_item_t;


static GUI_Surface *CreateHighlight(GUI_Surface *src, int w, int h) {

	SDL_Rect dst;
	SDL_Surface *sdl = GUI_SurfaceGet(src);
	GUI_Surface *s = GUI_SurfaceCreate("app-icon-hl",
		sdl->flags,
		w,
		h,
		sdl->format->BitsPerPixel,
		sdl->format->Rmask,
		sdl->format->Gmask,
		sdl->format->Bmask,
		sdl->format->Amask);

	GUI_SurfaceBoxRouded(s, 0, 0, w - 1, h - 1, 10, GUI_SurfaceMapRGBA(s, 212, 241, 21, 220));
	GUI_SurfaceRectagleRouded(s, 1, 1, w - 2, h - 2, 10, GUI_SurfaceMapRGBA(s, 212, 241, 41, 255));

	dst.x = dst.y = ICON_HIGHLIGHT_PADDING / 2;
	dst.w = GUI_SurfaceGetWidth(src);
	dst.h = GUI_SurfaceGetHeight(src);
	GUI_SurfaceBlitRGBA(src, NULL, s, &dst);

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
	int pad_x;
	int pad_y = ICON_CELL_PADDING_Y;

	GUI_Surface *s = GUI_SurfaceLoad(icon);
	int h = GUI_SurfaceGetHeight(s);

	if(name) {
		ts = GUI_FontGetTextSize(self.font, name);
		ts.w += 6;
		pad_x = ICON_CELL_PADDING_X;
	}
	else {
		ts.w = 0;
		pad_x -= 10;

		if(h >= 96) {
			pad_y -= 5;
		}
	}

	int w = ts.w + GUI_SurfaceGetWidth(s);

	if((self.y + h) > self.panel_area.h) {

		self.x += self.col_width + pad_x;
		self.y = pad_y;
		self.col_width = w;

		if(self.x + w + (pad_x / 2) > (self.pages * self.panel_area.w)) {
			self.x = (self.pages * self.panel_area.w) + (ICON_CELL_PADDING_X / 2);
			self.pages++;
		}
	}

	GUI_Widget *b = GUI_ButtonCreate(name ? name : "icon",
		self.x,
		self.y,
		w + ICON_HIGHLIGHT_PADDING,
		h + ICON_HIGHLIGHT_PADDING);

	if(s != NULL) {
		GUI_Surface *sh = CreateHighlight(s, w + ICON_HIGHLIGHT_PADDING, h + ICON_HIGHLIGHT_PADDING);

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
		GUI_LabelSetTextColor(l, 51, 41, 90);
		GUI_WidgetSetAlign(l, WIDGET_HORIZ_RIGHT | WIDGET_VERT_CENTER);
		GUI_ButtonSetCaption(b, l);
		GUI_ObjectDecRef((GUI_Object *) l);
	}

	GUI_ContainerAdd(self.panel, b);
	GUI_ObjectDecRef((GUI_Object *) b);

	self.y += (h + pad_y);

	if(self.col_width < w) {
		self.col_width = w;
	}
}


static void BuildAppList() {

	file_t fd;
	dirent_t *ent;
	char path[NAME_MAX];
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

	if(self.pages == 1) {
		self.x += (self.panel_area.w - (self.x % self.panel_area.w)) + (ICON_CELL_PADDING_X / 2);
		self.y = ICON_CELL_PADDING_Y;
		self.col_width = 0;
		self.pages++;
	}
	
	snprintf(path, NAME_MAX, "%s/apps/%s/scripts", getenv("PATH"), app_name);
	fd = fs_open(path, O_RDONLY | O_DIR);
	
	if(fd == FILEHND_INVALID)
		return;
		
	while((ent = fs_readdir(fd)) != NULL) {
		
		if(ent->name[0] == '.') continue;
		elen = strlen(ent->name);
		type = elen > 3 ? ent->name[elen - 3] : 'd';
		
		if(!ent->attr && (type == 'l' || type == 'd')) {
		
			script_item_t *si = (script_item_t *) calloc(1, (sizeof(script_item_t)));
			
			if(si == NULL)
				break;
			
			snprintf(si->file, NAME_MAX, "%s/apps/%s/scripts/%s", getenv("PATH"), app_name, ent->name);
			snprintf(path, NAME_MAX, "%s/apps/%s/images/%s", getenv("PATH"), app_name, ent->name);
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
						snprintf(path, NAME_MAX, "%s/gui/icons/normal/%s.png", 
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
		self.cur_x -= self.panel_area.w;
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_SlideRight() {
	if(self.x > self.cur_x) {
		self.cur_x += self.panel_area.w;
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_Init(App_t *app) {
	
	if(app != NULL) {
		
		memset(&self, 0, sizeof(self));
		self.app = app;

		self.x = ICON_CELL_PADDING_X / 2;
		self.y = ICON_CELL_PADDING_Y;
		self.pages = 1;

		self.font = APP_GET_FONT("comic");
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
