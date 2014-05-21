/* DreamShell ##version##

   module.c - luaGUI module
   Copyright (C)2007-2013 SWAT 

*/
            
#include <kos.h>
#include <kos/exports.h>
#include "ds.h"
#include "lua/tolua.h"


DEFAULT_MODULE_HEADER(luaGUI);

int tolua_GUI_open(lua_State* tolua_S);

typedef struct lua_callback_data {

	lua_State *state;
	const char *script; 

} lua_callback_data_t;

static void luaCallbackf(void *data) {
	lua_callback_data_t *d = (lua_callback_data_t *) data;
	//ds_printf("Callback: %s %p\n", d->script, d->state);
	LuaDo(LUA_DO_STRING, d->script, (lua_State *) d->state);
	//ds_printf("Callback called\n");
}

static void free_lua_callack(void *data) {
	if(data == NULL) {
		return;
	}

	lua_callback_data_t *d = (lua_callback_data_t*) data;
	
	free((void*)d->script);
	free(data);
}


GUI_Callback *GUI_LuaCallbackCreate(int appId, const char *luastring) {
	
	App_t *app = GetAppById(appId);
	
	if(app == NULL) {
		ds_printf("DS_ERROR: Can't find app: %d\n", appId);
		return NULL;
	}

	lua_callback_data_t *d = (lua_callback_data_t*) malloc(sizeof(lua_callback_data_t));

	d->script = strdup(luastring);
	d->state = app->lua;
	//ds_printf("Created callback: str=%s state=%p app=%s lua=%p\n", d->script, d->state, app->name, app->lua);

	return GUI_CallbackCreate((GUI_CallbackFunction *) luaCallbackf, (GUI_CallbackFunction *) free_lua_callack, (void *) d);
}


int lib_open(klibrary_t * lib) {
    
	lua_State *L = GetLuaState();
	
	if(L != NULL) {
		tolua_GUI_open(GetLuaState());
	}
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)tolua_GUI_open);
    return nmmgr_handler_add(&ds_luaGUI_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
    return nmmgr_handler_remove(&ds_luaGUI_hnd.nmmgr); 
}

