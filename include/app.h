/** 
 * \file    app.h
 * \brief   DreamShell applications
 * \date    2007-2014, 2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_APP_H
#define _DS_APP_H

#include <kos.h>
#include "list.h"
#include "gui.h"
#include "mxml.h"
#include "tsunami/dsapp.h"
#include "console.h"
#include "lua.h"

/**
 * App state flags
 */
#define APP_STATE_OPENED        0x00000001
#define APP_STATE_LOADED        0x00000002
#define APP_STATE_READY         0x00000004
#define APP_STATE_PROCESS       0x00000008
#define APP_STATE_SLEEP         0x00000020
#define APP_STATE_WAIT_UNLOAD   0x00000040

/* Default main app name for settings */
#define DS_DEFAULT_APP_NAME	"Launch App"

/**
 * App info structure
 */
typedef struct App {

	const char fn[NAME_MAX];
	const char icon[NAME_MAX];
	const char name[64];
	const char ver[32];
	const char ext[64];
	const char *args;

	uint32 id;
	uint32 state;

	Item_list_t *resources;
	Item_list_t *elements;
	mxml_node_t *xml;

	kthread_t *thd;
	lua_State *lua;
	GUI_Widget *body;
	DSApp *tsunami;

} App_t;

/**
 * Initialize and shutdown application system
 */
int InitApps();
void ShutdownApps();

/**
 * Apps list utils
 */
Item_list_t *GetAppList();
App_t *GetAppById(int id);
App_t *GetAppByFileName(const char *fn);
App_t *GetAppByName(const char *name);
App_t *GetAppByExtension(const char *ext);
App_t *GetAppsByExtension(const char *ext, App_t **app_list, size_t count);

/**
 * Get current opened application
 */
App_t *GetCurApp();

/**
 * Get cached current app name without walking the app list.
 * Safe to call from exception handlers. Returns NULL if none.
 */
const char *GetCurAppName(void);

/**
 * Parse app XML file and add to list
 * 
 * return NULL on error
 */
App_t *AddApp(const char *fn);

/**
 * Remove app from list
 */
int RemoveApp(App_t *app);

/**
 * Open app with arguments (if needed)
 * Application can be loaded and builded automatically
 * 
 * return 0 on error and 1 on success
 */
int OpenApp(App_t *app, const char *args);

/**
 * Open main app from settings
 *
 * return 0 on error and 1 on success
 */
int OpenMainApp(void);

/**
 * Get application directory path from app.xml file path
 */
void GetAppPath(char *buffer, size_t size, const char *fn);

/**
 * Get subdirectory path inside main app from settings
 */
void GetMainAppSubdir(char *buffer, size_t size, const char *subdir);

/**
 * Close app and unload (if unload > 0)
 * 
 * return 0 on error and 1 on success
 */
int CloseApp(App_t *app, int unload);

/**
 * Loading and unloading app resources
 * 
 * return 0 on error and 1 on success
 */
int LoadApp(App_t *app, int build);
int UnLoadApp(App_t *app);

/**
 * Building app body from XML tree
 * 
 * return 0 on error and 1 on success
 */
int BuildAppBody(App_t *app);

/**
 * Add and remove GUI widget to/from app body Panel widget
 * 
 * return 0 on error and 1 on success
 */
int AddToAppBody(App_t *app, GUI_Widget *widget);
int RemoveFromAppBody(App_t *app, GUI_Widget *widget);

/**
 * Calling app exported function
 * 
 * return 0 on error and 1 on success
 */
int CallAppExportFunc(App_t *app, const char *fstr);

/**
 * Calling app onload, onunload, onopen and onclose events
 * 
 * return 0 on error and 1 on success
 */
int CallAppBodyEvent(App_t *app, char *event);

/**
 * Waiting for clear flag APP_STATE_PROCESS
 */
void WaitApp(App_t *app);

/**
 * Setup APP_STATE_SLEEP flag manualy
 * 
 * return 0 on error and 1 on success
 */
int SetAppSleep(App_t *app, int sleep);

/**
 * Check extension supported
 * 
 * return 0 is NOT and 1 on success
 */
int IsFileSupportedByApp(App_t *app, const char *filename);

/* Utils */
char *FindXmlAttr(char *name, mxml_node_t *node, char *defValue);
void parseNodeSize(mxml_node_t *node, SDL_Rect *parent, int *w, int *h);
void parseNodePosition(mxml_node_t *node, SDL_Rect *parent, int *x, int *y);
void UnLoadOldApps();
void ProcessPendingAppOps(void);
void AppGuiCallbackEnter(void);
void AppGuiCallbackLeave(void);
int AppCallbackAllowed(App_t *app);
void UnloadAppResources(App_t *app, Item_list_t *lst);

/* Resource helpers */
void *getAppElement(App_t *app, const char *name, ListItemType type);
#define APP_GET_WIDGET(name)  ((GUI_Widget *)  getAppElement(self.app, name, LIST_ITEM_GUI_WIDGET))
#define APP_GET_SURFACE(name) ((GUI_Surface *) getAppElement(self.app, name, LIST_ITEM_GUI_SURFACE))
#define APP_GET_FONT(name)    ((GUI_Font *)    getAppElement(self.app, name, LIST_ITEM_GUI_FONT))
#define APP_GET_TSU_DRAWABLE(name) ((Drawable *) getAppElement(self.app, name, LIST_ITEM_TSU_DRAWABLE))
#define APP_GET_TSU_IMAGE(name)    ((Texture *)  getAppElement(self.app, name, LIST_ITEM_TSU_IMAGE))
#define APP_GET_TSU_FONT(name)     ((Font *)     getAppElement(self.app, name, LIST_ITEM_TSU_FONT))

#endif
