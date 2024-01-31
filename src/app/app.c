/*****************************
 * DreamShell ##version##    *
 * app.c                     *
 * DreamShell App            *
 * (c)2007-2024 SWAT         *
 * http://www.dc-swat.ru     *
 ****************************/

#include "ds.h"
#include "vmu.h"
#include <stdlib.h>

static Item_list_t *apps;
static int curOpenedApp;
static int first_open = 1;
static int prev_width = 0;
static int prev_height = 0;


static void FreeApp(void *app) {

	if(app == NULL)
		return;

	free(app);
	app = NULL;
}


int InitApps() {

	curOpenedApp = 0;

	if((apps = listMake()) == NULL) {
		return -1;
	}

	return 0;
}


void ShutdownApps() {
	listDestroy(apps, (listFreeItemFunc *) FreeApp);
	curOpenedApp = 0;
}


Item_list_t *GetAppList() {
	return apps;
}


App_t *GetAppById(int id) {

	Item_t *i = listGetItemById(apps, id);

	if(i != NULL)
		return (App_t *) i->data;
	else
		return NULL;
}


App_t *GetAppByFileName(const char *fn) {

	App_t *a;
	Item_t *i;

	SLIST_FOREACH(i, apps, list) {
		a = (App_t *) i->data;
		if (!strncmp(fn, a->fn, NAME_MAX))
			return a;
	}

	return NULL;
}


App_t *GetAppByName(const char *name) {

	Item_t *i = listGetItemByName(apps, name);

	if(i != NULL)
		return (App_t *) i->data;
	else
		return NULL;
}


App_t *GetAppByExtension(const char *ext) {

	App_t *a;
	Item_t *i;

	SLIST_FOREACH(i, apps, list) {
		a = (App_t *) i->data;
		if (strcasestr(a->ext, ext) != NULL)
			return a;
	}

	return NULL;
}

App_t *GetAppsByExtension(const char *ext, App_t **app_list, size_t count) {

	App_t *a;
	Item_t *i;
	size_t cnt = 0;

	SLIST_FOREACH(i, apps, list) {

		a = (App_t *) i->data;

		if (strcasestr(a->ext, ext) != NULL && app_list != NULL) {
			app_list[cnt++] = a;
		}

		if(cnt >= count) {
			goto result;
		}
	}

	goto result;

result:
	return cnt ? app_list[cnt - 1] : NULL;
}


App_t *GetCurApp() {
	return GetAppById(curOpenedApp);
}


int IsFileSupportedByApp(App_t *app, const char *filename) {
	char *ext = strrchr(filename, '.');
	if(!ext) return 0;

	if (strcasestr(app->ext, ext) != NULL) {
		return 1;
	}

	return 0;
}


void UnLoadOldApps() {
	App_t *a;
	Item_t *i;

	SLIST_FOREACH(i, apps, list) {

		a = (App_t *) i->data;
		if(a && a->state & APP_STATE_WAIT_UNLOAD) {
			UnLoadApp(a);
		}
	}
}



App_t *AddApp(const char *fn) {

	App_t *a, *at;
	Item_t *i;
	file_t fd = FILEHND_INVALID;
	mxml_node_t *tree = NULL, *node = NULL;
	char *name = NULL, *icon = NULL;
	char file[NAME_MAX];

	a = (App_t *) calloc(1, sizeof(App_t));

	if(a == NULL) {
		ds_printf("DS_ERROR: Memory allocation error\n");
		return NULL;
	}

	realpath(fn, file);
	fd = fs_open(file, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open file %s\n", file);
		goto error;
	}

	if((tree = mxmlLoadFd(NULL, fd, NULL)) == NULL) {
		ds_printf("DS_ERROR: XML file %s parsing error\n", file);
		goto error;
	}

	fs_close(fd);
	fd = FILEHND_INVALID;

	if((node = mxmlFindElement(tree, tree, "app", NULL, NULL, MXML_DESCEND)) == NULL) {
		ds_printf("DS_ERROR: XML file %s is not app\n", file);
		goto error;
	}

	name = FindXmlAttr("name", node, NULL);

	if(name == NULL) {
		ds_printf("DS_ERROR: App without name\n");
		goto error;
	}

	if((at = GetAppByName(name)) != NULL) {
		mxmlDelete(tree);
		FreeApp(a);
		ds_printf("DS: App '%s' already exists\n", name);
		return at;
	}

	strncpy((char*)a->fn, file, sizeof(a->fn));
	strncpy((char*)a->name, name, sizeof(a->name));
	strncpy((char*)a->ver, FindXmlAttr("version", node, "1.0.0"), sizeof(a->ver));
	strncpy((char*)a->ext, FindXmlAttr("extensions", node, ""), sizeof(a->ext));

	icon = FindXmlAttr("icon", node, NULL);

	if(icon == NULL) {
		sprintf((char*)a->icon, "%s/gui/icons/normal/default_app.png", getenv("PATH"));
	} else {
		memset_sh4(file, 0, NAME_MAX);
		relativeFilePath_wb(file, a->fn, icon);
		strncpy((char*)a->icon, file, sizeof(a->icon));
	}

	a->args = NULL;
	a->thd = NULL;
	a->lua = NULL;
	a->body = NULL;
	a->xml = NULL;
	a->resources = NULL;
	a->elements = NULL;

	if(tree)
		mxmlDelete(tree);

	// ds_printf("App: %s %s %s %s", a->fn, a->name, a->ver, a->icon);

	if((i = listAddItem(apps, LIST_ITEM_APP, a->name, a, sizeof(App_t))) == NULL) {
		goto error;
	}

	a->id = i->id;
	return a;

error:
	if(tree)
		mxmlDelete(tree);
	if(a)
		FreeApp(a);
	if(fd != FILEHND_INVALID)
		fs_close(fd);

	return NULL;
}


int RemoveApp(App_t *app) {

	WaitApp(app);

	if(app->state & APP_STATE_OPENED) CloseApp(app, 1);
	if(app->state & APP_STATE_LOADED) UnLoadApp(app);

	listRemoveItem(apps, listGetItemById(apps, app->id), (listFreeItemFunc *) FreeApp);
	return 1;
}


int OpenApp(App_t *app, const char *args) {

	ds_printf("DS_PROCESS: Opening app %s\n", app->name);
	int onopen_called = 0;

	if(app == NULL) {
		ds_printf("DS_ERROR: %s: Bad app pointer - %p\n", __func__, app);
		return 0;
	}

//	ds_printf("State: %d %d %d %d",
//				app->state & APP_STATE_OPENED,
//				app->state & APP_STATE_LOADED,
//				app->state & APP_STATE_READY,
//				app->state & APP_STATE_SLEEP);

	if(app->state & APP_STATE_OPENED) {
		ds_printf("DS_WARNING: App %s already opened\n", app->name);
		return 1;
	}

	if(!ScreenIsHidden() && !ConsoleIsVisible() && !first_open) {
		const char *str = "Loading...";
		vmu_draw_string(str);
		ScreenFadeOutEx(str, 1);
	}

	if(args != NULL) {
		app->args = args;
	}

	// ds_printf("DS_DEBUG: Cur opened app: %d\n", curOpenedApp);

	if(curOpenedApp) {

		App_t *cur = GetCurApp();

		if(cur != NULL && cur->state & APP_STATE_OPENED) {
			CloseApp(cur, (cur->state & APP_STATE_LOADED) ? 1 : 0);
		}

		curOpenedApp = 0;
	}

	if(app->state & APP_STATE_LOADED && app->state & APP_STATE_READY) {
		CallAppBodyEvent(app, "onopen");
		onopen_called = 1;
	}

	if(!(app->state & APP_STATE_LOADED)) {
		if(!LoadApp(app, 1)) {
			goto error;
		}
	}

	if(first_open && !ConsoleIsVisible()) {
		ScreenFadeOutEx(NULL, 0);
	}

	if((app->state & APP_STATE_LOADED) && !(app->state & APP_STATE_READY)) {
		if(!BuildAppBody(app)) {
			goto error;
		}
	}

	if(app->state & APP_STATE_READY) {

		if(app->state & APP_STATE_SLEEP) {
			SetAppSleep(app, 0);
		}

		if(app->body != NULL) {
			SDL_Rect rect = GUI_WidgetGetArea(app->body);

			if(first_open && !ConsoleIsVisible()) {
				first_open = 0;
				while(GetScreenOpacity() > 0.0f) {
					thd_pass();
				}
			}

			if(rect.w != GetScreenWidth() || rect.h != GetScreenHeight()) {
				
				prev_width = GetScreenWidth();
				prev_height = GetScreenHeight();
				
				SetScreenMode(rect.w, rect.h, 0.0f, 0.0f, 1.0f);
			}

			LockVideo();
			GUI_ScreenSetContents(GUI_GetScreen(), app->body);
			UnlockVideo();
		}

		vmu_draw_string(app->name);
		app->state |= APP_STATE_OPENED;
		curOpenedApp = app->id;

		if(!onopen_called) {
			CallAppBodyEvent(app, "onopen");
		}

	} else {
		ShowConsole();
	}

	if(args != NULL) {
		app->args = NULL;
	}

	ds_printf("DS_OK: App %s opened\n", app->name);

	thd_sleep(50);
	ScreenFadeIn();
	return 1;

error:
	if(args != NULL) {
		app->args = NULL;
	}
	if(ScreenIsHidden()) {
		ScreenFadeIn();
	}
	vmu_draw_string("Error");
	ShowConsole();
	return 0;
}


int CloseApp(App_t *app, int unload) {

	if(app == NULL) {
		ds_printf("DS_ERROR: CloseApp: Bad app pointer - %p\n", app);
		return 0;
	}

	// ds_printf("DS_PROCESS: Closing app %s", app->name);

	if(!(app->state & APP_STATE_OPENED)) {
		return 1;
	}

	app->state &= ~APP_STATE_OPENED;
	WaitApp(app);

	if(prev_width && (prev_width != GetScreenWidth() || prev_height != GetScreenHeight())) {
		SetScreenMode(prev_width, prev_height, 0.0f, 0.0f, 1.0f);
	}

	if(unload && (app->state & APP_STATE_LOADED)) {
		app->state |= APP_STATE_WAIT_UNLOAD;
	}

	if(!unload && (app->state & APP_STATE_LOADED)) {
		CallAppBodyEvent(app, "onclose");
		SetAppSleep(app, 1);
	}

	if(app->id == curOpenedApp) {
		curOpenedApp = 0;
	}

	app->state &= ~APP_STATE_OPENED;

	ds_printf("DS_OK: App %s closed%s\n", app->name, (unload ? " and unloaded." : "."));
	return 1;
}


int SetAppSleep(App_t *app, int sleep) {

	if(app == NULL) {
		ds_printf("DS_ERROR: %s: Bad app pointer - %p\n", __func__, app);
		return 0;
	}

	if(sleep && (app->state & APP_STATE_SLEEP) == 0) {
		app->state |= APP_STATE_SLEEP;
	} else if(!sleep && (app->state & APP_STATE_SLEEP)) {
		app->state &= ~APP_STATE_SLEEP;
	} else  {
		return 0;
	}

	return 1;
}


int AddToAppBody(App_t *app, GUI_Widget *widget) {

	if(app == NULL) {
		ds_printf("DS_ERROR: %s: Bad app pointer - %p\n", __func__, app);
		return 0;
	}

	if(app->body && widget) {
		GUI_ContainerAdd(app->body, widget);
	}
	return 1;
}


int RemoveFromAppBody(App_t *app, GUI_Widget *widget) {

	if(app == NULL) {
		ds_printf("DS_ERROR: %s: Bad app pointer - %p\n", __func__, app);
		return 0;
	}

	if(app->body && widget) GUI_ContainerRemove(app->body, widget);
	return 1;
}


int CallAppBodyEvent(App_t *app, char *event) {

	mxml_node_t	*node;

	if(app->xml && (node = mxmlFindElement(app->xml, app->xml, "body", NULL, NULL, MXML_DESCEND)) != NULL) {

		const char *e = FindXmlAttr(event, node, NULL);

		if(e != NULL) {

			if(!strncmp(e, "export:", 7)) {

				return CallAppExportFunc(app, e);

			} else if(!strncmp(e, "console:", 8)) {

				if(dsystem_buff(e+8) == CMD_OK) return 1;

			} else {
				if(LuaDo(LUA_DO_STRING, e, app->lua) < 1) return 1;
			}
		}
	}
	return 0;
}


void WaitApp(App_t *app) {
	if(app->state & APP_STATE_PROCESS) {
		while(app->state & APP_STATE_PROCESS) {
			thd_sleep(50);
		}
	}
}
