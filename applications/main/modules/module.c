/* DreamShell ##version##

   module.c - Main app module
   Copyright (C)2011-2014 SWAT 

*/

#include "ds.h"

DEFAULT_MODULE_EXPORTS(app_main);

static struct {
	
	GUI_Font *font;
	GUI_Widget *panel;
	int x;
	int y;
	int cur_x;
	
} self;

typedef struct script_item {
	
	char name[32];
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

	SDL_Rect ts = GUI_FontGetTextSize(self.font, name);
	int w = 53 + ts.w;
	int h = 48;

	GUI_Widget *b = GUI_ButtonCreate(name, self.x, self.y, w + 4, h + 4);
	GUI_Surface *s = GUI_SurfaceLoad(icon);

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

	GUI_Widget *l = GUI_LabelCreate(name, 0, 0, w, h, self.font, name);
	GUI_LabelSetTextColor(l, 0, 0, 0);
	GUI_WidgetSetAlign(l, WIDGET_HORIZ_RIGHT | WIDGET_VERT_CENTER);

	GUI_ButtonSetCaption(b, l);
	GUI_ObjectDecRef((GUI_Object *) l);
	GUI_ContainerAdd(self.panel, b);
	GUI_ObjectDecRef((GUI_Object *) b);

	self.y += 58;

	if(self.y >= 405) {
		self.x += 213;
		self.y = 20;
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
		
		if(ent->size > 1 && (type == 'l' || type == 'd')) {
		
			script_item_t *si = (script_item_t *) calloc(1, (sizeof(script_item_t)));
			
			if(si == NULL)
				break;
			
			snprintf(si->file, MAX_FN_LEN, "%s/apps/%s/scripts/%s", getenv("PATH"), app_name, ent->name);
			si->file[strlen(si->file)] = '\0';
			
			snprintf(path, MAX_FN_LEN, "%s/apps/%s/images/%s", getenv("PATH"), app_name, ent->name);
			plen = strlen(path);
			
			path[plen - 3] = 'p';
			path[plen - 2] = 'n';
			path[plen - 1] = 'g';
			path[plen] = '\0';
			
			if(!FileExists(path)) {
				snprintf(path, MAX_FN_LEN, "%s/gui/icons/normal/%s.png", 
							getenv("PATH"), (type == 'l' ? "lua" : "script"));
			}

			elen -= 4;
			
			if(elen > sizeof(si->name))
				elen = sizeof(si->name);
			
			strncpy(si->name, ent->name, elen);
			si->name[elen] = '\0';
			
			AddToList(si->name, path, RunScriptCB, free, (void *)si);
		}
	}
	
	fs_close(fd);
}


static void ShowVersion(GUI_Widget *widget) {
	char vers[32];	
	snprintf(vers, sizeof(vers), "%s %s", getenv("OS"), getenv("VERSION"));
	GUI_LabelSetText(widget, vers);
}


/**
 * Global functions
 */

void MainApp_SlideLeft() {
	if(self.cur_x > 0) {
		self.cur_x -= 640;
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_SlideRight() {
	if(self.x > self.cur_x) {
		self.cur_x += 640;
		GUI_PanelSetXOffset(self.panel, self.cur_x);
	}
}

void MainApp_Init(App_t *app) {
	
	Item_t *i;
	
	if(app != NULL) {
		
		memset(&self, 0, sizeof(self));
		self.x = self.y = 20;
		
		i = listGetItemByName(app->resources, "arial");

		if(i != NULL) {
			self.font = (GUI_Font *) i->data;
		}

		i = listGetItemByName(app->elements, "applist");

		if(i != NULL && self.font != NULL) {
			self.panel = (GUI_Widget*) i->data;
			BuildAppList();
		}
		
		i = listGetItemByName(app->elements, "version");

		if(i != NULL) {
			ShowVersion((GUI_Widget*) i->data);
		}
	}
}
