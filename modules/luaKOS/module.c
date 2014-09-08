/* DreamShell ##version##

   module.c - luaKOS module
   Copyright (C)2007-2014 SWAT 

*/
            
#include <kos.h>
#include <kos/exports.h>
#include "ds.h"

DEFAULT_MODULE_HEADER(luaKOS);

int tolua_KOS_open(lua_State* tolua_S);

int lib_open(klibrary_t * lib) {
	tolua_KOS_open(GetLuaState());
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)tolua_KOS_open);
	return nmmgr_handler_add(&ds_luaKOS_hnd.nmmgr);
}

int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
	return nmmgr_handler_remove(&ds_luaKOS_hnd.nmmgr); 
}

