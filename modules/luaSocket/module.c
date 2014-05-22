/* DreamShell ##version##

   module.c - luaSocket module
   Copyright (C)2011-2014 SWAT 

*/

#include "ds.h"
#include <kos/exports.h>

DEFAULT_MODULE_HEADER(luaSocket);

int luaopen_socket_core(lua_State *L);

int lib_open(klibrary_t * lib) {
	luaopen_socket_core(GetLuaState());
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)luaopen_socket_core);
	return nmmgr_handler_add(&ds_luaSocket_hnd.nmmgr);
}

int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
	return nmmgr_handler_remove(&ds_luaSocket_hnd.nmmgr); 
}
