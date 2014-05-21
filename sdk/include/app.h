/** 
 * \file    app.h
 * \brief   DreamShell applications
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_APP_H
#define _DS_APP_H

#include <kos.h>
#include "list.h"
#include "gui.h"
#include "mxml.h"
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

/**
 * App info structure
 */
typedef struct App {
    
	const char fn[MAX_FN_LEN];
	const char icon[MAX_FN_LEN];
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
GUI_Surface *getElementSurface(App_t *app, char *name);
char *FindXmlAttr(char *name, mxml_node_t *node, char *defValue);

void UnLoadOldApps();
void UnloadAppResources(Item_list_t *lst);

#endif
