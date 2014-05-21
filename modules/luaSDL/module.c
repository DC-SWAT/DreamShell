/* DreamShell ##version##

   module.c - luaSDL module
   Copyright (C)2007-2013 SWAT 

*/
            
#include <kos.h>
#include <kos/exports.h>
#include "ds.h"


DEFAULT_MODULE_HEADER(luaSDL);

int tolua_SDL_open(lua_State* tolua_S);


int lib_open(klibrary_t * lib) {
	lua_State *L = GetLuaState();
	
	if(L != NULL) {
		tolua_SDL_open(L);
	}
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)tolua_SDL_open);
    return nmmgr_handler_add(&ds_luaSDL_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
    return nmmgr_handler_remove(&ds_luaSDL_hnd.nmmgr); 
}





