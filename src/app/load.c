/*****************************
 * DreamShell ##version##    *
 * load.c                    *
 * DreamShell App loader     *
 * (c)2007-2014, 2024 SWAT   *
 * http://www.dc-swat.ru     *
 ****************************/


#include "ds.h"

//#define DEBUG 1

#ifdef DEBUG
#define APP_LOAD_DEBUG
#endif

typedef struct lua_callback_data {

	lua_State *state;
	const char *script;

} lua_callback_data_t;

typedef struct export_callback_data {

	uint32 addr;
	GUI_Widget *widget;

} export_callback_data_t;


typedef struct scrollbar_callback_data {

	GUI_Widget *scroll;
	GUI_Widget *panel;

} scrollbar_callback_data_t;

static void parseNodeSize(mxml_node_t *node, SDL_Rect *parent, int *w, int *h);
static void parseNodePosition(mxml_node_t *node, SDL_Rect *parent, int *x, int *y);

static int parseAppResource(App_t *app, mxml_node_t *node);
static int parseAppSurfaceRes(App_t *app, mxml_node_t *node, char *name);
static int parseAppFontRes(App_t *app, mxml_node_t *node, char *name, char *src);
static int parseAppImageRes(App_t *app, mxml_node_t *node, char *name, char *src);
static int parseAppFileRes(App_t *app, mxml_node_t *node, char *name, char *src);
static int parseAppTheme(App_t *app, mxml_node_t *node);

static uint32 GetAppExportFuncAddr(const char *fstr);
static GUI_Callback *CreateAppElementCallback(App_t *app, const char *event, GUI_Widget *widget);

static GUI_Widget *parseAppElement(App_t *app, mxml_node_t *node, SDL_Rect *parent);
static GUI_Widget *parseAppImageElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppLabelElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppButtonElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppToggleButtonElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppTextEntryElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppScrollBarElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppProgressBarElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppPanelElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_ListBox *parseAppListBoxElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppCardStackElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppRTFElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);
static GUI_Widget *parseAppFileManagerElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h);

static GUI_Surface *getElementSurface(App_t *app, char *name);
static GUI_Surface *getElementSurfaceTheme(App_t *app, mxml_node_t *node, char *attr);
static GUI_Font *getElementFont(App_t *app, mxml_node_t *node, char *name);


static void SetupAppLua(App_t *app) {
	if(app->lua == NULL) {
		//if(!GetCurApp())
		//ResetLua();
		app->lua = NewLuaThread();
		lua_pushnumber(app->lua, app->id);
		lua_setglobal(app->lua, "THIS_APP_ID");
	}
}

static uint32 GetAppExportFuncAddr(const char *fstr) {
	char evt[128];
	if(sscanf(fstr, "export:%[0-9a-zA-Z_]", evt)) {
		return GET_EXPORT_ADDR(evt);
	}
	return -1;
}

int CallAppExportFunc(App_t *app, const char *fstr) {
	uint32 fa = GetAppExportFuncAddr(fstr);
	int r = 0;

	if(fa > 0 && fa != 0xffffffff) {
		EXPT_GUARD_BEGIN;
		void (*cb_func)(App_t *) = (void (*)(App_t *))fa;
		cb_func(app);
		r = 1;
		EXPT_GUARD_CATCH;
		r = 0;
		EXPT_GUARD_END;
	}

	return r;
}


int LoadApp(App_t *app, int build) {

	mxml_node_t *node;
	file_t fd;

	ds_printf("DS_PROCESS: Loading app %s ...\n", app->name);
	app->state |= APP_STATE_PROCESS;

	if(app->xml == NULL) {

		fd = fs_open(app->fn, O_RDONLY);

		if(fd == FILEHND_INVALID) {
			app->state &= ~APP_STATE_PROCESS;
			ds_printf("DS_ERROR: Can't load app %s\n", app->fn);
			return 0;
		}

		if((app->xml = mxmlLoadFd(NULL, fd, NULL)) == NULL) {
			app->state &= ~APP_STATE_PROCESS;
			ds_printf("DS_ERROR: Can't parse xml %s\n", app->fn);
			fs_close(fd);
			return 0;
		}

		fs_close(fd);
	}

	if((node = mxmlFindElement(app->xml, app->xml, "body", NULL, NULL, MXML_DESCEND)) != NULL) {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Creating empty app body...\n");
#endif

		int x, y, w, h;

		parseNodeSize(node, NULL, &w, &h);
		parseNodePosition(node, NULL, &x, &y);
		
		app->body = GUI_PanelCreate(app->name, x, y, w, h);

	} else {
		ds_printf("DS_ERROR: <body> element not found\n");
		app->state &= ~APP_STATE_PROCESS;
		return 0;
	}
	
#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Loading app resources...\n");
#endif

	app->resources = listMake();
	app->elements = listMake();

	if((node = mxmlFindElement(app->xml, app->xml, "resources", NULL, NULL, MXML_DESCEND)) != NULL) {

		node = node->child->next;

		while(node) {

			if(node->type == MXML_ELEMENT) {
				if(!parseAppResource(app, node)) {
					app->state &= ~APP_STATE_PROCESS;
					UnLoadApp(app);
					return 0;
				}
			}
			node = node->next;
		}
	}

	app->state |= APP_STATE_LOADED;

	if(build) {
		return BuildAppBody(app);
	} else {
		app->state &= ~APP_STATE_PROCESS;
		return 1;
	}
}



int BuildAppBody(App_t *app) {

	mxml_node_t *node;
	int x, y, w, h;
	GUI_Widget *el;
	GUI_Surface *s = NULL;
	SDL_Rect body_area;

	if((node = mxmlFindElement(app->xml, app->xml, "body", NULL, NULL, MXML_DESCEND)) != NULL) {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Build app body...\n");
#endif

		if(!app->body) {
			parseNodeSize(node, NULL, &w, &h);
			parseNodePosition(node, NULL, &x, &y);
			app->body = GUI_PanelCreate(app->name, x, y, w, h);
		}

		if(!app->body) return 0;
		
		char *onload = FindXmlAttr("onload", node, NULL);
		char *bg = FindXmlAttr("background", node, NULL);

		if(bg != NULL) {

			if(*bg == '#') {
				SDL_Color c;
				unsigned int r = 0, g = 0, b = 0;
				sscanf(bg, "#%02x%02x%02x", &r, &g, &b);
				c.r = r;
				c.g = g;
				c.b = b;
				c.unused = 0;
				GUI_PanelSetBackgroundColor(app->body, c);

			} else if((s = getElementSurface(app, bg)) != NULL) {
				GUI_PanelSetBackground(app->body, s);
			} else {
				ds_printf("DS_ERROR: Can't find resource '%s' or resource is not surface\n", bg);
			}
		}

		node = node->child->next;
		body_area = GUI_WidgetGetArea(app->body);

		while(node) {

			if(node->type == MXML_ELEMENT && node->value.element.name[0] != '!') {

				if((el = parseAppElement(app, node, &body_area)) != NULL) {

					GUI_ContainerAdd(app->body, el);
					const char *n = GUI_ObjectGetName((GUI_Object *) el);

					if(n == NULL) {
						GUI_ObjectDecRef((GUI_Object *) el);
					} else {
						listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, n, (void *) el, 0);
					}
				}
			}
			node = node->next;
		}

		app->state &= ~APP_STATE_PROCESS;
		app->state |= APP_STATE_READY;

		if(onload != NULL) {

#ifdef APP_LOAD_DEBUG
			ds_printf("DS_DEBUG: Call to onload event...\n");
#endif

			if(!strncmp(onload, "export:", 7)) {
				CallAppExportFunc(app, onload);
			} else if(!strncmp(onload, "console:", 8)) {
				dsystem_buff(onload + 8);
			} else {

				SetupAppLua(app);
				LuaDo(LUA_DO_STRING, onload, app->lua);
			}
		}
	} else {
		app->state &= ~APP_STATE_PROCESS;
		app->state |= APP_STATE_READY;
		ds_printf("DS_WARNING: Can't find <body> in app.xml, GUI screen will not be changed.\n");
	}

	return 1;
}



int UnLoadApp(App_t *app) {

	if(!(app->state & APP_STATE_LOADED)) {
		return 1;
	}

	CallAppBodyEvent(app, "onunload");

	app->state |= APP_STATE_PROCESS;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Free app body...\n");
#endif

	if(app->body != NULL) {
		GUI_ObjectDecRef((GUI_Object *) app->body);
		app->body = NULL;
	}

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Unloading app elements...\n");
#endif
	if(app->elements) {
		UnloadAppResources(app->elements);
		app->elements = NULL;
	}


#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Unloading app resources...\n");
#endif
	if(app->resources) {
		UnloadAppResources(app->resources);
		app->resources = NULL;
	}


#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Closing app lua state...\n");
#endif

	if(app->lua != NULL) {
		//lua_close(app->lua);
		app->lua = NULL;
	}


#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Destroy app thread...\n");
#endif
	if(app->thd != NULL) {
		thd_join(app->thd, NULL);
		app->thd = NULL;
	}

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Deleting XML node tree...\n");
#endif
	if(app->xml) {
		mxmlDelete(app->xml);
		app->xml = NULL;
	}

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Clear flags...\n");
#endif
	app->state = 0;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: App unload complete.\n");
#endif

	return 1;
}



void UnloadAppResources(Item_list_t *lst) {

	Item_t *c, *n;

	c = SLIST_FIRST(lst);

	while(c) {

		n = SLIST_NEXT(c, list);

		if(c->type == LIST_ITEM_MODULE) {

			if(c->data)
				CloseModule((Module_t *) c->data);

		} else if(c->type == LIST_ITEM_SDL_RWOPS) {

			if(c->data)
				SDL_FreeRW(c->data);

		} else if(c->type == LIST_ITEM_GUI_SURFACE || c->type == LIST_ITEM_GUI_WIDGET || c->type == LIST_ITEM_GUI_FONT) {

			if(c->data) {
#ifdef APP_LOAD_DEBUG
				ds_printf("DS_DEBUG: Unloading: %s\n", GUI_ObjectGetName((GUI_Object *) c->data));
#endif
				GUI_ObjectDecRef((GUI_Object *) c->data);
			}

		} else {

			if(c->data && c->size)
				free(c->data);
		}

		free(c);
		c = n;
	}

	SLIST_INIT(lst);
	free(lst);
}


char *FindXmlAttr(char *name, mxml_node_t *node, char *defValue) {
	char *attr = (char*)mxmlElementGetAttr(node, name);
	return attr != NULL ? attr : defValue;
}


static void parseNodeSize(mxml_node_t *node, SDL_Rect *parent, int *w, int *h) {

	int parent_w, parent_h;
	float tmp;
	char *width = FindXmlAttr("width", node, "100%");
	char *height = FindXmlAttr("height", node, "100%");
	
	if(parent) {
		parent_w = parent->w;
		parent_h = parent->h;
	} else {
		parent_w = GetScreenWidth();
		parent_h = GetScreenHeight();
	}

	if(width[strlen(width)-1] == '%') {
		
		if(!strncmp(width, "100%", 4)) {
			
			*w = parent_w;
			
		} else {

			tmp = ((float)parent_w / 100);
			*w = tmp * atof(width);
		}

	} else {
		*w = atoi(width);
	}

	if(height[strlen(height)-1] == '%') {
		
		if(!strncmp(height, "100%", 4)) {
			
			*h = parent_h;
			
		} else {
			
			tmp = ((float)parent_h / 100);
			*h = tmp * atof(height);
		}

	} else {
		*h = atoi(height);
	}
}


static void parseNodePosition(mxml_node_t *node, SDL_Rect *parent, int *x, int *y) {

	int parent_w, parent_h;
	float tmp;
	char *xp = FindXmlAttr("x", node, "0");
	char *yp = FindXmlAttr("y", node, "0");
	
	if(parent) {
		parent_w = parent->w;
		parent_h = parent->h;
	} else {
		parent_w = GetScreenWidth();
		parent_h = GetScreenHeight();
	}
	
	if(*xp == '0') {

		*x = 0;
		
	} else if(xp[strlen(xp)-1] == '%') {
		
		tmp = ((float)parent_w / 100);
		*x = tmp * atof(xp);

	} else {

		*x = atoi(xp);
	}

	if(*yp == '0') {

		*y = 0;
		
	} else if(yp[strlen(yp)-1] == '%') {

		tmp = ((float)parent_h / 100);
		*y = tmp * atof(yp);

	} else {

		*y = atoi(yp);
	}
}

static int alignStrToFlag(char *align) {

	if(!strncmp(align, "center", 6)) return WIDGET_HORIZ_CENTER;
	else if(!strncmp(align, "right", 5)) return WIDGET_HORIZ_RIGHT;
	else if(!strncmp(align, "left", 4)) return WIDGET_HORIZ_LEFT;
	else return 0;
}


static int valignStrToFlag(char *valign) {

	if(!strncmp(valign, "center", 6)) return WIDGET_VERT_CENTER;
	else if(!strncmp(valign, "top", 3)) return WIDGET_VERT_TOP;
	else if(!strncmp(valign, "bottom", 6)) return WIDGET_VERT_BOTTOM;
	else return 0;
}


void *getAppElement(App_t *app, const char *name, ListItemType type) {
	
	Item_t *item;
	
	switch(type) {
		case LIST_ITEM_GUI_WIDGET:
			item = listGetItemByName(app->elements, name);
			break;
		case LIST_ITEM_GUI_SURFACE:
		case LIST_ITEM_GUI_FONT:
		default:
			item = listGetItemByNameAndType(app->resources, name, type);
			break;
	}

	if(item != NULL && item->type == type) {
		return item->data;
	}

	ds_printf("DS_ERROR: %s: Couldn't find or wrong type '%s'\n", app->name, name);
	return NULL;
}


static GUI_Surface *getElementSurface(App_t *app, char *name) {

	GUI_Surface *s = NULL;
	Item_t *res;

	if(name != NULL) {

		if((res = listGetItemByName(app->resources, name)) != NULL && res->type == LIST_ITEM_GUI_SURFACE) {

			s = (GUI_Surface *) res->data;

		} else {

			char file[NAME_MAX];
			relativeFilePath_wb(file, app->fn, name);
			s = GUI_SurfaceLoad(file);

			if(s != NULL)
				listAddItem(app->resources, LIST_ITEM_GUI_SURFACE, name, (void *) s, 0);
		}
	}

	return s;
}

static GUI_Surface *getElementSurfaceTheme(App_t *app, mxml_node_t *node, char *attr) {
	
	Item_t *item;
	mxml_node_t *n;
	char *value = FindXmlAttr(attr, node, NULL);
	
#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Get surface %s for %s\n", attr, node->value.element.name);
#endif
	
	if(value) {
		return getElementSurface(app, value);
	}
		
	value = FindXmlAttr("theme", node, NULL);
	
	if(value) {
		
#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Search theme %s\n", value);
#endif
		
		item = listGetItemByNameAndType(app->resources, value, LIST_ITEM_XML_NODE);
		
		if(item) {
			
#ifdef APP_LOAD_DEBUG
			ds_printf("DS_PROCESS: Found theme %s\n", value);
#endif
			
			n = (mxml_node_t *)item->data;
			value = FindXmlAttr(attr, n, NULL);
			
			if(value) {
				return getElementSurface(app, value);
			}
		}
	}
	
	if(!strncmp(node->value.element.name, "input", 5)) {
		value = FindXmlAttr("type", node, node->value.element.name);
	} else {
		value = node->value.element.name;
	}
	
#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Search theme %s\n", value);
#endif
	
	item = listGetItemByNameAndType(app->resources, value, LIST_ITEM_XML_NODE);
		
	if(item) {
		
#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Found theme %s\n", value);
#endif
		
		n = (mxml_node_t *)item->data;
		value = FindXmlAttr(attr, n, NULL);
		
		if(value) {
			return getElementSurface(app, value);
		}
	}
	
	return NULL;
}


static GUI_Font *getElementFont(App_t *app, mxml_node_t *node, char *name) {

	GUI_Font *fnt = NULL;
	Item_t *res;

	if(name != NULL) {

		if((res = listGetItemByName(app->resources, name)) != NULL && res->type == LIST_ITEM_GUI_FONT) {

			fnt = (GUI_Font *) res->data;

		} else {

			char file[NAME_MAX];
			relativeFilePath_wb(file, app->fn, name);
			fnt = GUI_FontLoadTrueType(file, atoi(FindXmlAttr("fontsize", node, "12")));

			if(fnt != NULL)
				listAddItem(app->resources, LIST_ITEM_GUI_FONT, name, (void *) fnt, 0);
		}
	}

	return fnt;
}


static Uint32 parseAppSurfColor(GUI_Surface *surface, char *color) {

	Uint32 c = 0;
	uint r = 0, g = 0, b = 0, a = 0, cnt = 0;

	cnt = sscanf(color, "#%02x%02x%02x%02x", &r, &g, &b, &a);

	if(cnt == 4) {
		c = GUI_SurfaceMapRGBA(surface, r, g, b, a);
	} else {
		c = GUI_SurfaceMapRGB(surface, r, g, b);
	}
	return c;
}


static void parseAppSurfaceAlign(mxml_node_t *node, SDL_Rect *rect, SDL_Rect *surf_area) {
	
	char *align = FindXmlAttr("align", node, NULL);
	char *valign = FindXmlAttr("valign", node, NULL);
	
	if(align) {
		switch (alignStrToFlag(align)) {
			case WIDGET_HORIZ_CENTER:
				rect->x = (surf_area->w - rect->w) / 2;
				break;
			case WIDGET_HORIZ_LEFT:
				rect->x = 0;
				break;
			case WIDGET_HORIZ_RIGHT:
				rect->x = surf_area->w - rect->w;
				break;
			default:
				break;
		}
	}
	
	if(valign) {
		switch (valignStrToFlag(valign)) {
			case WIDGET_VERT_CENTER:
				rect->y = (surf_area->h - rect->h) / 2;
				break;
			case WIDGET_VERT_TOP:
				rect->y = 0;
				break;
			case WIDGET_VERT_BOTTOM:
				rect->y = surf_area->h - rect->h;
				break;
			default:
				break;
		}
	}
}


static void parseAppSurfaceFill(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	SDL_Rect rect, surf_area;
	int xr, yr, wr, hr;
	
	surf_area.w = GUI_SurfaceGetWidth(surface);
	surf_area.h = GUI_SurfaceGetHeight(surface);

	parseNodeSize(node, &surf_area, &wr, &hr);
	parseNodePosition(node, &surf_area, &xr, &yr);
	
	if(wr == surf_area.w && xr) {
		wr -= xr;
	}
	
	if(hr == surf_area.h && yr) {
		hr -= yr;
	}
	
	rect.x = xr;
	rect.y = yr;
	rect.w = wr;
	rect.h = hr;
	
	parseAppSurfaceAlign(node, &rect, &surf_area);

#ifdef APP_LOAD_DEBUG
	ds_printf("Fill surface: x=%d y=%d w=%d h=%d color=%08lx\n", rect.x, rect.y, rect.w, rect.h, c);
#endif
	GUI_SurfaceFill(surface, &rect, c);
}


static void parseAppSurfaceBlit(App_t *app, mxml_node_t *node, GUI_Surface *surface) {

	char *src, *rgba;
	SDL_Rect area, rect, surf_area;
	int xr, yr, wr, hr;

	area.x = area.y = 0;

	surf_area.w = GUI_SurfaceGetWidth(surface);
	surf_area.h = GUI_SurfaceGetHeight(surface);

	parseNodeSize(node, &surf_area, &wr, &hr);
	parseNodePosition(node, &surf_area, &xr, &yr);

	if(wr == surf_area.w && xr) {
		wr -= xr;
	}

	if(hr == surf_area.h && yr) {
		hr -= yr;
	}

	rect.x = xr;
	rect.y = yr;
	rect.w = wr;
	rect.h = hr;
	
	src = FindXmlAttr("surface", node, NULL);
	rgba = FindXmlAttr("rgba", node, "false");

	if(src != NULL) {

		Item_t *res;
		GUI_Surface *surf = NULL;
		int decref = 0;
		char file[NAME_MAX];

		if((res = listGetItemByName(app->resources, src)) != NULL) {

			if(res->type == LIST_ITEM_GUI_SURFACE) {
				surf = (GUI_Surface *) res->data;
			}

		} else {
			relativeFilePath_wb(file, app->fn, src);
			surf = GUI_SurfaceLoad(file);
			decref = 1;
		}

		if(surf != NULL) {

			wr = GUI_SurfaceGetWidth(surf);
			hr = GUI_SurfaceGetHeight(surf);

			if(rect.w > wr) {
				rect.w = wr;
			}

			if(rect.h > hr) {
				rect.h = hr;
			}

			parseAppSurfaceAlign(node, &rect, &surf_area);

#ifdef APP_LOAD_DEBUG
			ds_printf("Blit surface: x=%d y=%d w=%d h=%d surface=%s\n", rect.x, rect.y, rect.w, rect.h, decref ? file : src);
#endif
	
			if(wr > surf_area.w) {
				area.x = (wr - surf_area.w) / 2;
			}

			if(hr > surf_area.h) {
				area.y = (hr - surf_area.h) / 2;
			}

			area.w = rect.w;
			area.h = rect.h;

			if(!strncmp(rgba, "true", 4)) {
				GUI_SurfaceBlitRGBA(surf, &area, surface, &rect);
			}
			else {
				GUI_SurfaceBlit(surf, &area, surface, &rect);
			}

			if(decref)
				GUI_ObjectDecRef((GUI_Object*)surf);
		}
	}
}


static void parseAppSurfaceRect(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	Sint16 x1, y1, x2, y2;

	x1 = atoi(FindXmlAttr("x1", node, "0"));
	y1 = atoi(FindXmlAttr("y1", node, "0"));
	x2 = atoi(FindXmlAttr("x2", node, "0"));
	y2 = atoi(FindXmlAttr("y2", node, "0"));

#ifdef APP_LOAD_DEBUG
	ds_printf("Rect surface: x1=%d y1=%d x2=%d y2=%d color=%08x\n", x1, y1, x2, y2, c);
#endif
	GUI_SurfaceRectagle(surface, x1, y1, x2, y2, c);
}


static void parseAppSurfaceLine(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	Sint16 x1, y1, x2, y2;
	char *aa;
	int width;

	x1 = atoi(FindXmlAttr("x1", node, "0"));
	y1 = atoi(FindXmlAttr("y1", node, "0"));
	x2 = atoi(FindXmlAttr("x2", node, "0"));
	y2 = atoi(FindXmlAttr("y2", node, "0"));
	width = atoi(FindXmlAttr("w", node, "0"));
	aa = FindXmlAttr("aa", node, "false");

#ifdef APP_LOAD_DEBUG
	ds_printf("Line surface: x1=%d y1=%d x2=%d y2=%d color=%08x\n", x1, y1, x2, y2, c);
#endif

	if(!strncmp(aa, "true", 4)) {
		GUI_SurfaceLineAA(surface, x1, y1, x2, y2, c);
	} else if(width > 0) {
		GUI_SurfaceThickLine(surface, x1, y1, x2, y2, (Uint8)width, c);
	} else {
		GUI_SurfaceLine(surface, x1, y1, x2, y2, c);
	}
}

static void parseAppSurfaceCircle(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	Sint16 x, y, rad;
	char *aa, *fill;

	x = atoi(FindXmlAttr("x1", node, "0"));
	y = atoi(FindXmlAttr("y1", node, "0"));
	rad = atoi(FindXmlAttr("rad", node, "0"));
	aa = FindXmlAttr("aa", node, "false");
	fill = FindXmlAttr("fill", node, "false");

#ifdef APP_LOAD_DEBUG
	ds_printf("Circle surface: x=%d y=%d rad=%d color=%08x\n", x, y, rad, c);
#endif

	if(!strcmp(aa, "true")) {
		GUI_SurfaceCircleAA(surface, x, y, rad, c);
	} else if(!strcmp(fill, "true")) {
		GUI_SurfaceCircleFill(surface, x, y, rad, c);
	} else {
		GUI_SurfaceCircle(surface, x, y, rad, c);
	}
}

static void parseAppSurfaceTrigon(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	Sint16 x1, y1, x2, y2, x3, y3;
	char *aa, *fill;

	x1 = atoi(FindXmlAttr("x1", node, "0"));
	y1 = atoi(FindXmlAttr("y1", node, "0"));
	x2 = atoi(FindXmlAttr("x2", node, "0"));
	y2 = atoi(FindXmlAttr("y2", node, "0"));
	x3 = atoi(FindXmlAttr("x3", node, "0"));
	y3 = atoi(FindXmlAttr("y3", node, "0"));
	aa = FindXmlAttr("aa", node, "false");
	fill = FindXmlAttr("fill", node, "false");

#ifdef APP_LOAD_DEBUG
	ds_printf("Trigon surface: x1=%d y1=%d x2=%d y2=%d x3=%d y3=%d color=%08x\n", x1, y1, x2, y2, x3, y3, c);
#endif

	if(!strncmp(aa, "true", 4)) {
		GUI_SurfaceTrigonAA(surface, x1, y1, x2, y2, x3, y3, c);
	} else if(!strncmp(fill, "true", 4)) {
		GUI_SurfaceTrigonFill(surface, x1, y1, x2, y2, x3, y3, c);
	} else {
		GUI_SurfaceTrigon(surface, x1, y1, x2, y2, x3, y3, c);
	}
}

static void parseAppSurfaceBezier(App_t *app, mxml_node_t *node, GUI_Surface *surface, Uint32 c) {

	Sint16 vx;
	Sint16 vy;
	int n;
	int s;

	vx = atoi(FindXmlAttr("x", node, "0"));
	vy = atoi(FindXmlAttr("y", node, "0"));
	n = atoi(FindXmlAttr("n", node, "0"));
	s = atoi(FindXmlAttr("s", node, "0"));


#ifdef APP_LOAD_DEBUG
	ds_printf("Bezier surface: vx=%d vy=%d n=%d s=%d color=%08x\n", vx, vy, n, s, c);
#endif

	GUI_SurfaceBezier(surface, &vx, &vy, n, s, c);
}


static int parseAppResource(App_t *app, mxml_node_t *node) {

	if(node->value.element.name[0] == '!')
		return 1;

	char *s = FindXmlAttr("src", node, NULL);
	char src[NAME_MAX];

	if(s == NULL && strncmp(node->value.element.name, "surface", 7) && strncmp(node->value.element.name, "theme", 5)) {
		ds_printf("DS_ERROR: Empty src attribute in %s\n", node->value.element.name);
		return 0;
	} else {
		relativeFilePath_wb(src, app->fn, s);
	}

	char *name = FindXmlAttr("name", node, NULL);

	if(!strncmp(node->value.element.name, "font", 4)) {

		return parseAppFontRes(app, node, name, src);

	} else if(!strncmp(node->value.element.name, "image", 5)) {

		return parseAppImageRes(app, node, name, src);

	} else if(!strncmp(node->value.element.name, "surface", 7)) {

		return parseAppSurfaceRes(app, node, name);

	} else if(!strncmp(node->value.element.name, "script", 6)) {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Loading %s %s type %s\n", node->value.element.name, src, FindXmlAttr("type", node, "text/lua"));
#endif

		if(!strncmp(FindXmlAttr("type", node, "text/lua"), "text/lua", 8)) {
			SetupAppLua(app);
			if(app->lua != NULL) {
				LuaDo(LUA_DO_FILE, src, app->lua);
			} else {
				ds_printf("DS_ERROR: Can't initialize lua thread for %s\n", src);
				return 0;
			}
		}

	} else if(!strncmp(node->value.element.name, "module", 6)) {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_PROCESS: Loading %s %s\n", node->value.element.name, src);
#endif

		Module_t *m = OpenModule(src);

		if(m == NULL) {
			ds_printf("DS_ERROR: Can't load module %s\n", src);
			return 0;
		} else {
			listAddItem(app->resources, LIST_ITEM_MODULE, m->lib_get_name(), (void *) m, sizeof(Module_t));
		}

	} else if(!strncmp(node->value.element.name, "file", 4)) {

		return parseAppFileRes(app, node, name, src);

	} else if(!strncmp(node->value.element.name, "theme", 5)) {

		return parseAppTheme(app, node);

	} else {
		ds_printf("DS_ERROR: Uknown resurce - %s\n", node->value.element.name);
	}

	return 1;
}


static int parseAppSurfaceRes(App_t *app, mxml_node_t *node, char *name) {

	GUI_Surface *surface;
	mxml_node_t *n;
	int w, h;
	Uint32 c = 0xFFFFFFFF;
	char *color, *bpp;
	
#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Creating %s \"%s\"\n", node->value.element.name, name);
#endif

	SDL_Rect parent_area = GUI_WidgetGetArea(app->body);
	parseNodeSize(node, &parent_area, &w, &h);

	bpp = FindXmlAttr("bpp", node, NULL);

	if(bpp && atoi(bpp) == 32) {
		surface = GUI_SurfaceCreate(name,
			SDL_SWSURFACE | SDL_SRCALPHA,
			w, h,
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000);
	}
	else {
		SDL_Surface *screen = GetScreen();
		surface = GUI_SurfaceCreate(name,
			SDL_SWSURFACE | SDL_SRCALPHA,
			w, h,
			screen->format->BitsPerPixel,
			screen->format->Rmask,
			screen->format->Gmask,
			screen->format->Bmask,
			screen->format->Amask);
	}

	if(surface == NULL) {
		ds_printf("DS_ERROR: Can't create surface \"%s\"\n", name);
		return 0;
	}

	n = node->child->next;

	while(n) {

		if(n->type == MXML_ELEMENT) {

			color = FindXmlAttr("color", n, NULL);

			if(color != NULL) {
				c = parseAppSurfColor(surface, color);
			}

			if(!strncmp(n->value.element.name, "fill", 4)) {
				parseAppSurfaceFill(app, n, surface, c);
			} else if(!strncmp(n->value.element.name, "blit", 4)) {
				parseAppSurfaceBlit(app, n, surface);
			} else if(!strncmp(n->value.element.name, "rect", 4)) {
				parseAppSurfaceRect(app, n, surface, c);
			} else if(!strncmp(n->value.element.name, "line", 4)) {
				parseAppSurfaceLine(app, n, surface, c);
			} else if(!strncmp(n->value.element.name, "circle", 6)) {
				parseAppSurfaceCircle(app, n, surface, c);
			} else if(!strncmp(n->value.element.name, "trigon", 6)) {
				parseAppSurfaceTrigon(app, n, surface, c);
			} else if(!strncmp(n->value.element.name, "bezier", 6)) {
				parseAppSurfaceBezier(app, n, surface, c);
			}

		}
		n = n->next;
	}
		
	listAddItem(app->resources, LIST_ITEM_GUI_SURFACE, name, (void *)surface, 0);
	return 1;
}


static int parseAppFontRes(App_t *app, mxml_node_t *node, char *name, char *src) {

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Loading %s %s size %d\n", node->value.element.name, src, atoi(FindXmlAttr("size", node, "12")));
#endif

	char *type = FindXmlAttr("type", node, "ttf");
	GUI_Font *f = NULL;

	if(!strncmp(type, "bitmap", 6)) {
		f = GUI_FontLoadBitmap(src);
	} else if(!strcmp(type, "ttf")) {
		f = GUI_FontLoadTrueType(src, atoi(FindXmlAttr("size", node, "12")));
	}

	if(f == NULL) {
		ds_printf("DS_ERROR: Can't load font %s\n", src);
		return 0;
	}

	listAddItem(app->resources, LIST_ITEM_GUI_FONT, name, (void *) f, 0);
	return 1;
}


static int parseAppImageRes(App_t *app, mxml_node_t *node, char *name, char *src) {

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Loading %s %s\n", node->value.element.name, src);
#endif

	GUI_Surface *s;
	SDL_Rect r;
	char *x = FindXmlAttr("x", node, NULL);
	char *y = FindXmlAttr("y", node, NULL);
	char *w = FindXmlAttr("width", node, NULL);
	char *h = FindXmlAttr("height", node, NULL);

	if(x || y || w || h) {

		r.x = x ? atoi(x) : 0;
		r.y = y ? atoi(y) : 0;
		r.w = w ? atoi(w) : 0;
		r.h = h ? atoi(h) : 0;
		s = GUI_SurfaceLoad_Rect(src, &r);

	} else {
		s = GUI_SurfaceLoad(src);
	}

	if(s == NULL) {
		ds_printf("DS_ERROR: Can't load image %s\n", src);
		return 0;
	}

	listAddItem(app->resources, LIST_ITEM_GUI_SURFACE, name, (void *) s, 0);
	return 1;
}


static int parseAppFileRes(App_t *app, mxml_node_t *node, char *name, char *src) {

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_PROCESS: Loading %s %s\n", node->value.element.name, src);
#endif

	char *ftype = FindXmlAttr("type", node, "basic");

	if(!strncmp(ftype, "rwops", 5)) {

		SDL_RWops *rw = SDL_RWFromFile(src, "rb");

		if(rw == NULL) {
			ds_printf("DS_ERROR: Can't load file %s\n", src);
			return 0;
		} else {
			listAddItem(app->resources, LIST_ITEM_SDL_RWOPS, name, (void *) rw, FileSize(src));
		}

	} else {

		file_t f = fs_open(src, O_RDONLY);

		if(f == FILEHND_INVALID) {
			ds_printf("DS_ERROR: Can't open file: %s\n", src);
			return 0;
		}

		uint32 size = fs_total(f);
		uint8 *buff = (uint8*) malloc(size);
		
		if(!buff) {
			fs_close(f);
			return 0;
		}
		
		fs_read(f, buff, size);
		fs_close(f);
		listAddItem(app->resources, LIST_ITEM_USERDATA, name, (void *) buff, size);
	}

	return 1;
}


static int parseAppTheme(App_t *app, mxml_node_t *node) {
	
	mxml_node_t *n;
	char *name;
	
	if(node->child && node->child->next) {

		n = node->child->next;

		while(n) {

			if(n->type == MXML_ELEMENT && n->value.element.name[0] != '!') {
				
				name = FindXmlAttr("name", n, NULL);

				if(!name) {
					if(!strncmp(n->value.element.name, "input", 5)) {
						name = FindXmlAttr("type", n, n->value.element.name);
					} else {
						name = n->value.element.name;
					}
				}

#ifdef APP_LOAD_DEBUG
				ds_printf("DS_DEBUG: Creating theme - %s\n", name);
#endif
				listAddItem(app->resources, LIST_ITEM_XML_NODE, name, (void *)n, 0);
			}
			n = n->next;
		}
	}
	
	return 1;
}


static GUI_Widget *parseAppElement(App_t *app, mxml_node_t *node, SDL_Rect *parent) {

	int x, y, w, h;
	GUI_Widget *widget;
	char *attr, *name;

	parseNodeSize(node, parent, &w, &h);
	parseNodePosition(node, parent, &x, &y);
	
	if(w == parent->w && x) {
		w -= x;
	}
	
	if(h == parent->h && y) {
		h -= y;
	}
	
	name = FindXmlAttr("name", node, NULL);

	if(!strncmp(node->value.element.name, "image", 5)) {

		widget = parseAppImageElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "label", 5)) {

		widget = parseAppLabelElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "input", 5)) {

		char *type = FindXmlAttr("type", node, "unknown");

		if(!strncmp(type, "button", 6)) {

			widget = parseAppButtonElement(app, node, name, x, y, w, h);

		} else if(!strncmp(type, "checkbox", 8)) {

			widget = parseAppToggleButtonElement(app, node, name, x, y, w, h);

		} else if(!strncmp(type, "text", 4)) {

			widget = parseAppTextEntryElement(app, node, name, x, y, w, h);

		} else {
			return NULL;
		}

	} else if(!strncmp(node->value.element.name, "scrollbar", 9)) {

		widget = parseAppScrollBarElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "progressbar", 11)) {

		widget = parseAppProgressBarElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "panel", 5)) {

		widget = parseAppPanelElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "listbox", 7)) {

		widget = (GUI_Widget *) parseAppListBoxElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "cardstack", 9)) {

		widget = parseAppCardStackElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "rtf", 3)) {

		widget = parseAppRTFElement(app, node, name, x, y, w, h);

	} else if(!strncmp(node->value.element.name, "filemanager", 11)) {

		widget = parseAppFileManagerElement(app, node, name, x, y, w, h);
		/*
		} else if(!strcmp(node->value.element.name, "nanox")) {

			widget = parseAppNanoXElement(app, node, name, x, y, w, h);
		*/
	} else {
		return NULL;
	}


	if(widget == NULL) {
		return NULL;
	}

	attr = FindXmlAttr("align", node, NULL);
	char *valign = FindXmlAttr("valign", node, NULL);

	if(attr || valign) {
		
		int align_flags = 0;
		
		if(attr) {
			align_flags |= alignStrToFlag(attr);
		}
		
		if(valign) {
			align_flags |= valignStrToFlag(valign);
		}
		
		GUI_WidgetSetAlign(widget, align_flags);
	}

	attr = FindXmlAttr("tile", node, NULL);

	if(attr) {
		GUI_Surface *surface = getElementSurface(app, attr);
		if(surface != NULL) {
			GUI_WidgetTileImage(widget, surface, NULL, 0, 0);
		}
	}

	attr = FindXmlAttr("visibility", node, NULL);

	if(attr) {
		if(!strncmp(attr, "false", 5)) {
			GUI_WidgetSetFlags(widget, WIDGET_HIDDEN);
		} else {
			GUI_WidgetClearFlags(widget, WIDGET_HIDDEN);
		}
		GUI_WidgetMarkChanged(widget);
	}

	attr = FindXmlAttr("transparent", node, NULL);

	if(attr)
		GUI_WidgetSetTransparent(widget, atoi(attr));

	attr = FindXmlAttr("inactive", node, NULL);

	if(attr)
		GUI_WidgetSetEnabled(widget, 0);
		

	return widget;
}




static GUI_Widget *parseAppImageElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Surface *s;
	GUI_Widget *img;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing Image: %s\n", name);
#endif

	if((s = getElementSurface(app, FindXmlAttr("src", node, NULL))) == NULL) return NULL;

	img = GUI_PictureCreate(name, x, y, w, h, s);

	if(node->child && node->child->next && node->child->next->value.element.name[0] != '!') {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_DEBUG: Parsing Image Child: %s\n", node->child->next->value.element.name);
#endif

		SDL_Rect parent_area = GUI_WidgetGetArea(img);
		GUI_Widget *el = parseAppElement(app, node->child->next, &parent_area);

		if(el != NULL) {

			const char *n = GUI_ObjectGetName((GUI_Object *) el);

			GUI_PictureSetCaption(img, el);

			if(n == NULL) {
				GUI_ObjectDecRef((GUI_Object *) el);
			} else {
				listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, n, (void *) el, 0);
			}
		}
	}
#if 0//def APP_LOAD_DEBUG
	ds_printf("Image: %s x=%d y=%d w=%d h=%d\n", name, x, y, w, h);
#endif
	return img;
}



static GUI_Widget *parseAppLabelElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Font *font;
	char *src;
	GUI_Widget *label;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing Label: %s\n", name);
#endif

	src = FindXmlAttr("font", node, NULL);

	if(src == NULL || (font = getElementFont(app, node, src)) == NULL) {
		ds_printf("DS_ERROR: Can't find font '%s'\n", src);
		return NULL;
	}

	label = GUI_LabelCreate(name, x, y, w, h, font, FindXmlAttr("text", node, " "));
	if(label == NULL) return NULL;

	char *color = FindXmlAttr("color", node, NULL);
	unsigned int r = 0, g = 0, b = 0;

	if(color != NULL) {
		sscanf(color, "#%02x%02x%02x", &r, &g, &b);
	}

#ifdef APP_LOAD_DEBUG
	ds_printf("Label (%s): %s  |  Color: %d.%d.%d  |  font: %s\n", name, FindXmlAttr("text", node, " "), r, g, b, src);
#endif

	GUI_LabelSetTextColor(label, r, g, b);

	return label;
}




static GUI_Widget *parseAppRTFElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *widget;
	char *src, *color, *offset, *freesrc, *font;
	unsigned int r = 0, g = 0, b = 0;
	Item_t *item;
	SDL_RWops *rwf;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing RTF: %s\n", name);
#endif

	src = FindXmlAttr("file", node, NULL);

	if(src == NULL) {
		return NULL;
	}

	font = FindXmlAttr("font", node, NULL);
	item = listGetItemByNameAndType(app->resources, src, LIST_ITEM_SDL_RWOPS);

	if(item != NULL) {
		rwf = (SDL_RWops *) item->data;
		freesrc = FindXmlAttr("freesrc", node, "0");
		widget = GUI_RTF_LoadRW(name, rwf, atoi(freesrc), font, x, y, w, h);
	} else {
		char file[NAME_MAX];
		relativeFilePath_wb(file, app->fn, src);
		widget = GUI_RTF_Load(name, file, font, x, y, w, h);
	}

	color = FindXmlAttr("bgcolor", node, NULL);

	if(color != NULL) {
		sscanf(color, "#%02x%02x%02x", &r, &g, &b);
		GUI_RTF_SetBgColor(widget, r, g, b);
	}

	offset = FindXmlAttr("offset", node, NULL);

	if(offset != NULL) {
		GUI_RTF_SetOffset(widget, atoi(offset));
	}

	return widget;
}



static void lua_callback_function(void *data) {

	lua_callback_data_t *d = (lua_callback_data_t *) data;
	LuaDo(LUA_DO_STRING, d->script, (lua_State *) d->state);
}

static void export_callback_function(void *data) {

	export_callback_data_t *d = (export_callback_data_t *) data;

	EXPT_GUARD_BEGIN;
	void (*func)(GUI_Widget *widget) = (void (*)(GUI_Widget *widget))d->addr;
	func(d->widget);
	EXPT_GUARD_CATCH;
	EXPT_GUARD_END;
}

static void cmd_callback_function(void *data) {
	dsystem_buff((char *)data);
}


static GUI_Callback *CreateAppElementCallback(App_t *app, const char *event, GUI_Widget *widget) {

	GUI_Callback *cb = NULL;

	if(!strncmp(event, "export:", 7)) {

		export_callback_data_t *de = (export_callback_data_t*) malloc(sizeof(export_callback_data_t));

		if(de != NULL) {

			de->addr = GetAppExportFuncAddr(event);
			de->widget = widget;

			if(de->addr > 0 && de->addr != 0xffffffff) {
				cb = GUI_CallbackCreate(export_callback_function, free, (void*)de);
				return cb;
			}

			free(de);
		}

	} else if(!strncmp(event, "console:", 8)) {

		cb = GUI_CallbackCreate(cmd_callback_function, NULL, (void*)event+8);

	} else if(app->lua != NULL) {

		lua_callback_data_t *d = (lua_callback_data_t*) malloc(sizeof(lua_callback_data_t));

		if(d != NULL) {
			d->script = event;
			d->state = app->lua;

			cb = GUI_CallbackCreate(lua_callback_function, free, (void *) d);

			if(cb == NULL) {
				free(d);
			}
		}
	}

	return cb;
}

/*

#include "nano-X.h"
#include "nanowm.h"

static void NanoXDefaultEventHandler(void *data) {
	GR_EVENT gevent;
	GrGetNextEvent(&gevent);
	//printf("NanoXDefaultEventHandler: %d\n", gevent.type);
	GUI_WidgetMarkChanged((GUI_Widget *)data);
	switch (gevent.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			GUI_WidgetMarkChanged((GUI_Widget *)data);
			break;
		case GR_EVENT_TYPE_UPDATE:
			GUI_WidgetMarkChanged((GUI_Widget *)data);
			break;
		case GR_EVENT_TYPE_MOUSE_MOTION:
			GUI_WidgetMarkChanged((GUI_Widget *)data);
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			//GrClose();
			break;
	}
}



static GUI_Widget *parseAppNanoXElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *widget;
	GUI_Callback *cb;
	char *event = NULL;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing NanoX: %s\n", name);
#endif

	widget = GUI_NANOX_Create(name, x, y, w, h);
	event = FindXmlAttr("event", node, NULL);

	if(event != NULL) {
		cb = CreateAppElementCallback(app, event, widget);
	} else {
		ds_printf("NanoXDefaultEventHandler: OK\n");
		cb = GUI_CallbackCreate((GUI_CallbackFunction *)NanoXDefaultEventHandler, NULL, widget);
	}

	if(cb != NULL) {
		GUI_NANOX_SetGEventHandler(widget, cb);
		GUI_ObjectDecRef((GUI_Object *) cb);
	}

	return widget;
}

*/


static GUI_Widget *parseAppButtonElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *button;
	GUI_Surface *sf;
	char *onclick, *oncontextclick, *onmouseover, *onmouseout;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing Button: %s\n", name);
#endif

	button = GUI_ButtonCreate(name, x, y, w, h);
	if(button == NULL) return NULL;

	onclick = FindXmlAttr("onclick", node, NULL);
	oncontextclick = FindXmlAttr("oncontextclick", node, NULL);
	onmouseover = FindXmlAttr("onmouseover", node, NULL);
	onmouseout = FindXmlAttr("onmouseout", node, NULL);

	if((sf = getElementSurfaceTheme(app, node, "normal")) != NULL) {
		GUI_ButtonSetNormalImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "highlight")) != NULL) {
		GUI_ButtonSetHighlightImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "pressed")) != NULL) {
		GUI_ButtonSetPressedImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "disabled")) != NULL) {
		GUI_ButtonSetDisabledImage(button, sf);
	}

	GUI_Callback *cb;

	if(onclick != NULL) {

		cb = CreateAppElementCallback(app, onclick, button);

		if(cb != NULL) {
			GUI_ButtonSetClick(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(oncontextclick != NULL) {

		cb = CreateAppElementCallback(app, oncontextclick, button);

		if(cb != NULL) {
			GUI_ButtonSetContextClick(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(onmouseover != NULL) {

		cb = CreateAppElementCallback(app, onmouseover, button);

		if(cb != NULL) {
			GUI_ButtonSetMouseover(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(onmouseout != NULL && app->lua != NULL) {

		cb = CreateAppElementCallback(app, onmouseout, button);

		if(cb != NULL) {
			GUI_ButtonSetMouseout(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}


	if(node->child && node->child->next && node->child->next->value.element.name[0] != '!') {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_DEBUG: Parsing Button Child = %s\n", node->child->next->value.element.name);
#endif

		SDL_Rect parent_area = GUI_WidgetGetArea(button);
		GUI_Widget *el = parseAppElement(app, node->child->next, &parent_area);

		if(el != NULL) {

			const char *n = GUI_ObjectGetName((GUI_Object *) el);

			GUI_ButtonSetCaption(button, el);

			if(n == NULL) {
				GUI_ObjectDecRef((GUI_Object *) el);
			} else {
				listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, n, (void *) el, 0);
			}
		}
	}

	return button;
}


static GUI_Widget *parseAppToggleButtonElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *button;
	GUI_Surface *sf;
	char *onclick, *oncontextclick, *checked, *onmouseover, *onmouseout;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing Toggle Button: %s\n", name);
#endif

	button = GUI_ToggleButtonCreate(name, x, y, w, h);
	if(button == NULL) return NULL;

	onclick = FindXmlAttr("onclick", node, NULL);
	oncontextclick = FindXmlAttr("oncontextclick", node, NULL);
	onmouseover = FindXmlAttr("onmouseover", node, NULL);
	onmouseout = FindXmlAttr("onmouseout", node, NULL);
	checked = FindXmlAttr("checked", node, NULL);

	if((sf = getElementSurfaceTheme(app, node, "onnormal")) != NULL) {
		GUI_ToggleButtonSetOnNormalImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "onhighlight")) != NULL) {
		GUI_ToggleButtonSetOnHighlightImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "offnormal")) != NULL) {
		GUI_ToggleButtonSetOffNormalImage(button, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "offhighlight")) != NULL) {
		GUI_ToggleButtonSetOffHighlightImage(button, sf);
	}


	if(checked != NULL && strncmp(checked, "false", 5) && strncmp(checked, "no", 2)) {
		GUI_WidgetSetState(button, 1);
	} else {
		GUI_WidgetSetState(button, 0);
	}

	GUI_Callback *cb;

	if(onclick != NULL) {

		cb = CreateAppElementCallback(app, onclick, button);

		if(cb != NULL) {
			GUI_ToggleButtonSetClick(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(oncontextclick != NULL) {

		cb = CreateAppElementCallback(app, oncontextclick, button);

		if(cb != NULL) {
			GUI_ToggleButtonSetContextClick(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(onmouseover != NULL) {

		cb = CreateAppElementCallback(app, onmouseover, button);

		if(cb != NULL) {
			GUI_ToggleButtonSetMouseover(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(onmouseout != NULL) {

		cb = CreateAppElementCallback(app, onmouseout, button);

		if(cb != NULL) {
			GUI_ToggleButtonSetMouseout(button, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}


	if(node->child && node->child->next && node->child->next->value.element.name[0] != '!') {

#ifdef APP_LOAD_DEBUG
		ds_printf("DS_DEBUG: Parsing Toggle Button Child: %s\n", node->child->next->value.element.name);
#endif

		SDL_Rect parent_area = GUI_WidgetGetArea(button);
		GUI_Widget *el = parseAppElement(app, node->child->next, &parent_area);

		if(el != NULL) {

			const char *n = GUI_ObjectGetName((GUI_Object *) el);

			GUI_ToggleButtonSetCaption(button, el);

			if(n == NULL) {
				GUI_ObjectDecRef((GUI_Object *) el);
			} else {
				listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, n, (void *) el, 0);
			}
		}
	}

	return button;
}



static GUI_Widget *parseAppTextEntryElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *text;
	GUI_Surface *sf;
	GUI_Font *font = NULL;
	char *onfocus, *onblur, *src;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing TextEntry: %s\n", name);
#endif

	onfocus = FindXmlAttr("onfocus", node, NULL);
	onblur = FindXmlAttr("onblur", node, NULL);

	src = FindXmlAttr("font", node, NULL);

	if(src == NULL || (font = getElementFont(app, node, src)) == NULL) {
		ds_printf("DS_ERROR: Can't find font '%s'\n", src);
		return NULL;
	}

	text = GUI_TextEntryCreate(name, x, y, w, h, font, atoi(FindXmlAttr("size", node, "12")));
	if(text == NULL) return NULL;

	char *color = FindXmlAttr("fontcolor", node, NULL);
	unsigned int r = 0, g = 0, b = 0;

	if(color != NULL) {
		sscanf(color, "#%02x%02x%02x", &r, &g, &b);
	}

	GUI_TextEntrySetTextColor(text, r, g, b);

	if(font) GUI_TextEntrySetFont(text, font);
	GUI_TextEntrySetText(text, FindXmlAttr("value", node, ""));


	if((sf = getElementSurfaceTheme(app, node, "normal")) != NULL) {
		GUI_TextEntrySetNormalImage(text, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "highlight")) != NULL) {
		GUI_TextEntrySetHighlightImage(text, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "focus")) != NULL) {
		GUI_TextEntrySetFocusImage(text, sf);
	}

	GUI_Callback *cb;

	if(onfocus != NULL) {

		cb = CreateAppElementCallback(app, onfocus, text);

		if(cb != NULL) {
			GUI_TextEntrySetFocusCallback(text, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	if(onblur != NULL) {

		cb = CreateAppElementCallback(app, onblur, text);

		if(cb != NULL) {
			GUI_TextEntrySetUnfocusCallback(text, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	return text;
}



static void scrollbar_move_callback(void *data) {
	scrollbar_callback_data_t *d = (scrollbar_callback_data_t *) data;
	GUI_PanelSetYOffset(d->panel, GUI_ScrollBarGetPosition(d->scroll));
}

static GUI_Widget *parseAppScrollBarElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *scroll;
	GUI_Surface *sf;
	char *onmove;
	int /*step = 0, */pos = 0;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing ScrollBar: %s\n", name);
#endif

	scroll = GUI_ScrollBarCreate(name, x, y, w, h);
	if(scroll == NULL) return NULL;

	onmove = FindXmlAttr("onmove", node, NULL);
	//step = atoi(FindXmlAttr("step", node, "10"));
	pos = atoi(FindXmlAttr("pos", node, "0"));

	if((sf = getElementSurfaceTheme(app, node, "knob")) != NULL) {
		GUI_ScrollBarSetKnobImage(scroll, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "background")) != NULL) {
		GUI_ScrollBarSetBackgroundImage(scroll, sf);
	}

	GUI_ScrollBarSetPosition(scroll, pos);
	//GUI_ScrollBarSetPageStep(scroll, step);

	if(onmove != NULL) {

		GUI_Callback *cb = CreateAppElementCallback(app, onmove, scroll);

		if(cb != NULL) {
			GUI_ScrollBarSetMovedCallback(scroll, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		}
	}

	return scroll;
}



static GUI_Widget *parseAppProgressBarElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *progress;
	GUI_Surface *sf;
	double pos = 0.0;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing ProgressBar: %s\n", name);
#endif

	progress = GUI_ProgressBarCreate(name, x, y, w, h);
	if(progress == NULL) return NULL;
	pos = atof(FindXmlAttr("pos", node, "0.0"));


	if((sf = getElementSurfaceTheme(app, node, "bimage")) != NULL) {
		GUI_ProgressBarSetImage1(progress, sf);
	}

	if((sf = getElementSurfaceTheme(app, node, "pimage")) != NULL) {
		GUI_ProgressBarSetImage2(progress, sf);
	}

	GUI_ProgressBarSetPosition(progress, pos);
	return progress;
}



static GUI_Widget *parseAppPanelElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *panel;
	GUI_Surface *s = NULL;
	Item_t *res;
	char *back = NULL, *sbar;
	int xo = 0, yo = 0;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing Panel: %s\n", name);
#endif

	panel = GUI_PanelCreate(name, x, y, w, h);

	if(panel == NULL) return NULL;

	back = FindXmlAttr("background", node, NULL);
	sbar = FindXmlAttr("scrollbar", node, NULL);
	xo = atoi(FindXmlAttr("xoffset", node, "0"));
	yo = atoi(FindXmlAttr("yoffset", node, "0"));

	if(back != NULL) {

		if(*back == '#') {

			SDL_Color c;
			unsigned int r = 0, g = 0, b = 0;
			sscanf(back, "#%02x%02x%02x", &r, &g, &b);
			c.r = r;
			c.g = g;
			c.b = b;
			c.unused = 0;
			GUI_PanelSetBackgroundColor(app->body, c);

		} else if((s = getElementSurface(app, back)) != NULL) {
			GUI_PanelSetBackground(panel, s);
		}
	}

	back = FindXmlAttr("backgroundCenter", node, NULL);

	if(back != NULL) {
		s = getElementSurface(app, back);

		if(s != NULL) {
			GUI_PanelSetBackgroundCenter(panel, s);
		}
	}

	if(sbar != NULL && (res = listGetItemByName(app->elements, sbar)) != NULL && res->type == LIST_ITEM_GUI_WIDGET) {

		scrollbar_callback_data_t *d = (scrollbar_callback_data_t*) malloc(sizeof(scrollbar_callback_data_t));

		d->scroll = (GUI_Widget *) res->data;
		d->panel = panel;

		GUI_Callback *cb = GUI_CallbackCreate(scrollbar_move_callback, free, (void *) d);

		if(cb != NULL) {
			GUI_ScrollBarSetMovedCallback(d->scroll, cb);
			GUI_ObjectDecRef((GUI_Object *) cb);
		} else {
			free(d);
		}
	}


	if(node->child && node->child->next) {

		mxml_node_t *n = node->child->next;
		GUI_Widget *el;
		SDL_Rect parent_area = GUI_WidgetGetArea(panel);

		while(n) {

			if(n->type == MXML_ELEMENT) {

#ifdef APP_LOAD_DEBUG
				ds_printf("DS_DEBUG: Parsing Panel Child: %s\n", n->value.element.name);
#endif
				if(n->value.element.name[0] != '!' && (el = parseAppElement(app, n, &parent_area)) != NULL) {

					const char *nm = GUI_ObjectGetName((GUI_Object *) el);

					GUI_ContainerAdd(panel, el);

					if(nm == NULL) {
						GUI_ObjectDecRef((GUI_Object *) el);
					} else {
						listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, nm, (void *) el, 0);
					}
				}
			}
			n = n->next;
		}
	}

	GUI_PanelSetXOffset(panel, xo);
	GUI_PanelSetYOffset(panel, yo);

	return panel;
}




static GUI_ListBox *parseAppListBoxElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_ListBox *listbox;
	GUI_Font *fnt = NULL;
	char *font, *color;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing ListBox: %s\n", name);
#endif

	font = FindXmlAttr("font", node, NULL);
	color = FindXmlAttr("fontcolor", node, NULL);

	if(font != NULL) {

		if((fnt = getElementFont(app, node, font)) == NULL) {
			ds_printf("DS_ERROR: Can't find font '%s'\n", font);
			return NULL;
		}

	} else {
		ds_printf("DS_ERROR: ListBox attr 'font' is undefined\n");
		return NULL;
	}

	listbox = GUI_CreateListBox(name, x, y, w, h, fnt);

	if(listbox == NULL) return NULL;

	if(color != NULL) {
		unsigned int r = 0, g = 0, b = 0;
		sscanf(color, "#%02x%02x%02x", &r, &g, &b);
		GUI_ListBoxSetTextColor(listbox, r, g, b);
	}

	if(node->child && node->child->next) {

		mxml_node_t *n = node->child->next;

		while(n) {

			if(n->type == MXML_ELEMENT) {

				if(!strcmp(n->value.element.name, "item")) {
					GUI_ListBoxAddItem(listbox, FindXmlAttr("value", node, NULL));
				}
			}
			n = n->next;
		}
	}

#ifdef APP_LOAD_DEBUG
	//ds_printf("ListBox: %s\n", name);
#endif
	return listbox;
}



static GUI_Widget *parseAppCardStackElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *cards;
	GUI_Surface *s = NULL;
	char *back = NULL;
	SDL_Color c;
	unsigned int r = 0, g = 0, b = 0;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing CardStack: %s\n", name);
#endif

	cards = GUI_CardStackCreate(name, x, y, w, h);
	if(cards == NULL) return NULL;
	back = FindXmlAttr("background", node, NULL);

	if(back != NULL) {

		if(back[0] == '#') {

			sscanf(back, "#%02x%02x%02x", &r, &g, &b);
			c.r = r;
			c.g = g;
			c.b = b;
			GUI_CardStackSetBackgroundColor(cards, c);

		} else if((s = getElementSurface(app, back)) != NULL) {
			GUI_CardStackSetBackground(cards, s);
		}
	}

	if(node->child && node->child->next) {

		mxml_node_t *n = node->child->next;
		GUI_Widget *el;
		SDL_Rect parent_area = GUI_WidgetGetArea(cards);

		while(n) {

			if(n->type == MXML_ELEMENT) {

#ifdef APP_LOAD_DEBUG
				ds_printf("DS_DEBUG: Parsing CardStack Child: %s\n", n->value.element.name);
#endif

				if(n->value.element.name[0] != '!' && (el = parseAppElement(app, n, &parent_area)) != NULL) {

					const char *nm = GUI_ObjectGetName((GUI_Object *) el);

					GUI_ContainerAdd(cards, el);

					if(nm == NULL) {
						GUI_ObjectDecRef((GUI_Object *) el);
					} else {
						listAddItem(app->elements, LIST_ITEM_GUI_WIDGET, nm, (void *) el, 0);
					}
				}
			}
			n = n->next;
		}
	}
	return cards;
}



#define LIST_ITEM_FM_DATA_OFFSET 1000

static void filemanager_call_lua_func(dirent_fm_t *ent, int index) {
	Item_t *item;
	App_t *app = GetCurApp();

	if(app) {

		lua_State *L = app->lua;
		const char *name = GUI_ObjectGetName(ent->obj);
		item = listGetItemByNameAndType(app->resources, name, app->id + LIST_ITEM_FM_DATA_OFFSET + index);

		if(item == NULL) {
			ds_printf("item not fount\n");
			return;
		}

		lua_getfield(L, LUA_GLOBALSINDEX, (const char *)item->data);

		if(lua_type(L, -1) != LUA_TFUNCTION) {
			ds_printf("DS_ERROR: Can't find function: \"%s\" FileManager events support only global functions.\n", (const char *)item->data);
			return;
		}

		lua_newtable(L);
		lua_pushstring(L, "name");
		lua_pushstring(L, ent->ent.name);
		lua_settable(L, -3);
		lua_pushstring(L, "size");
		lua_pushnumber(L, ent->ent.size);
		lua_settable(L, -3);
		lua_pushstring(L, "time");
		lua_pushnumber(L, ent->ent.time);
		lua_settable(L, -3);
		lua_pushstring(L, "attr");
		lua_pushnumber(L, ent->ent.attr);
		lua_settable(L, -3);
		lua_pushstring(L, "index");
		lua_pushnumber(L, ent->index);
		lua_settable(L, -3);
		lua_pushstring(L, "fmwname");
		lua_pushstring(L, name);
		lua_settable(L, -3);
		lua_report(L, lua_docall(L, 1, 1));
	}
}

static void filemanager_lua_func_click(dirent_fm_t *ent) {
	filemanager_call_lua_func(ent, 0);
}

static void filemanager_lua_func_context_click(dirent_fm_t *ent) {
	filemanager_call_lua_func(ent, 1);
}

static void filemanager_lua_func_over(dirent_fm_t *ent) {
	filemanager_call_lua_func(ent, 2);
}

static void filemanager_lua_func_out(dirent_fm_t *ent) {
	filemanager_call_lua_func(ent, 3);
}



static GUI_CallbackFunction *FileManagerCallbackFunc(App_t *app, const char *event, char *wname, int idx) {

	if(!strncmp(event, "export:", 7)) {

		uint32 fa = GetAppExportFuncAddr(event);

		if(fa > 0 && fa != 0xffffffff) {
			return (GUI_CallbackFunction *)(void (*)(void*))fa;
		}

	} else if(!strncmp(event, "console:", 8)) {

		return NULL;

	} else if(app->lua != NULL) {

		Item_t *l = listAddItem(app->resources, app->id + LIST_ITEM_FM_DATA_OFFSET + idx, wname, (void *)event, 0);

		if(l) {
			switch(idx) {
			case 0:
				return (GUI_CallbackFunction *)filemanager_lua_func_click;
			case 1:
				return (GUI_CallbackFunction *)filemanager_lua_func_context_click;
			case 2:
				return (GUI_CallbackFunction *)filemanager_lua_func_over;
			case 3:
				return (GUI_CallbackFunction *)filemanager_lua_func_out;
			default:
				return NULL;
			}
		}
	}

	return NULL;
}



static GUI_Widget *parseAppFileManagerElement(App_t *app, mxml_node_t *node, char *name, int x, int y, int w, int h) {

	GUI_Widget *fm;
	GUI_Surface *sn, *sh, *sp, *sd = NULL;
	GUI_Font *fnt;
	int r, g, b;
	char *event;

#ifdef APP_LOAD_DEBUG
	ds_printf("DS_DEBUG: Parsing FileManager: %s\n", name);
#endif

	char path[NAME_MAX];
	relativeFilePath_wb(path, app->fn, FindXmlAttr("path", node, "/"));

	fm = GUI_FileManagerCreate(name, path, x, y, w, h);

	if(fm == NULL) return NULL;

	sn = getElementSurfaceTheme(app, node, "background");

	if(sn != NULL) {
		GUI_PanelSetBackground(fm, sn);
	}

	sn = getElementSurfaceTheme(app, node, "item_normal");

	if(sn != NULL) {
		sh = getElementSurfaceTheme(app, node, "item_highlight");
		sp = getElementSurfaceTheme(app, node, "item_pressed");
		sd = getElementSurfaceTheme(app, node, "item_disabled");
		GUI_FileManagerSetItemSurfaces(fm, sn, sh, sp, sd);
	}

	fnt = getElementFont(app, node, FindXmlAttr("item_font", node, NULL));

	if(fnt != NULL) {

		char *color = FindXmlAttr("item_font_color", node, NULL);

		if(color != NULL) {
			sscanf(color, "#%02x%02x%02x", &r, &g, &b);
		} else {
			r = atoi(FindXmlAttr("item_font_r", node, "0"));
			g = atoi(FindXmlAttr("item_font_g", node, "0"));
			b = atoi(FindXmlAttr("item_font_b", node, "0"));
		}

		GUI_FileManagerSetItemLabel(fm, fnt, r, g, b);
	}

	sn = getElementSurfaceTheme(app, node, "sb_knob");

	if(sn) {
		sh = getElementSurfaceTheme(app, node, "sb_back");
		GUI_FileManagerSetScrollbar(fm, sn, sh);
	}

	sn = getElementSurfaceTheme(app, node, "sbbup_normal");

	if(sn != NULL) {
		sh = getElementSurfaceTheme(app, node, "sbbup_highlight");
		sp = getElementSurfaceTheme(app, node, "sbbup_pressed");
		sd = getElementSurfaceTheme(app, node, "sbbup_disabled");
		GUI_FileManagerSetScrollbarButtonUp(fm, sn, sh, sp, sd);
	}

	sn = getElementSurfaceTheme(app, node, "sbbdown_normal");

	if(sn != NULL) {
		sh = getElementSurfaceTheme(app, node, "sbbdown_highlight");
		sp = getElementSurfaceTheme(app, node, "sbbdown_pressed");
		sd = getElementSurfaceTheme(app, node, "sbbdown_disabled");
		GUI_FileManagerSetScrollbarButtonDown(fm, sn, sh, sp, sd);
	}


	event = FindXmlAttr("onclick", node, NULL);

	if(event != NULL) {
		GUI_FileManagerSetItemClick(fm, FileManagerCallbackFunc(app, event, name, 0));
	}

	event = FindXmlAttr("oncontextclick", node, NULL);

	if(event != NULL) {
		GUI_FileManagerSetItemContextClick(fm, FileManagerCallbackFunc(app, event, name, 1));
	}

	event = FindXmlAttr("onmouseover", node, NULL);

	if(event != NULL) {
		GUI_FileManagerSetItemMouseover(fm, FileManagerCallbackFunc(app, event, name, 2));
	}

	event = FindXmlAttr("onmouseout", node, NULL);

	if(event != NULL) {
		GUI_FileManagerSetItemMouseout(fm, FileManagerCallbackFunc(app, event, name, 3));
	}

	if(node->child && node->child->next) {

		mxml_node_t *n = node->child->next;
		char *iname = NULL;
		int size = 0, time = 0, attr = 0;

		while(n) {

			if(n->type == MXML_ELEMENT) {
#ifdef APP_LOAD_DEBUG
				ds_printf("DS_DEBUG: Parsing Panel Child: %s\n", n->value.element.name);
#endif
				iname = FindXmlAttr("name", n, "item");
				size = atoi(FindXmlAttr("size", n, "-3"));
				time = atoi(FindXmlAttr("time", n, "0"));
				attr = atoi(FindXmlAttr("attr", n, "0"));
				GUI_FileManagerAddItem(fm, iname, size, time, attr);
			}

			n = n->next;
		}
	}

	return fm;
}
