/* DreamShell ##version##

   module.c - luaTask module
   Copyright (C)2011-2014 SWAT 

*/

#include "ds.h"
#include <kos/exports.h>

DEFAULT_MODULE_HEADER(luaTask);

int luaopen_task(lua_State *L);

int lib_open(klibrary_t * lib) {
	luaopen_task(GetLuaState());
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)luaopen_task);
	return nmmgr_handler_add(&ds_luaTask_hnd.nmmgr);
}

int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
	return nmmgr_handler_remove(&ds_luaTask_hnd.nmmgr); 
}
