/* DreamShell ##version##

   module.c - luaSQL module
   Copyright (C)2007-2014 SWAT 

*/
            
#include <kos.h>
#include <kos/exports.h>
#include "ds.h"

DEFAULT_MODULE_HEADER(luaSQL);

int luaopen_luasql_sqlite3(lua_State *L);


int lib_open(klibrary_t * lib) {
    luaopen_luasql_sqlite3(GetLuaState());
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)luaopen_luasql_sqlite3);
    return nmmgr_handler_add(&ds_luaSQL_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
    return nmmgr_handler_remove(&ds_luaSQL_hnd.nmmgr); 
}

