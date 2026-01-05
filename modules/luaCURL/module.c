/* DreamShell ##version##

   module.c - luaCURL module
   Copyright (C) 2026 SWAT
*/

#include "ds.h"

int luaopen_lcurl(lua_State *L);

char *lib_get_name() {
    return "luaCURL";
}

uint32_t lib_get_version() {
    return DS_MAKE_VER(VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
}

int lib_open(klibrary_t * lib) {
    if(luaopen_lcurl(GetLuaState())) {
        return -1;
    }
    if(!RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)luaopen_lcurl)) {
        return -1;
    }
    return 0;
}

int lib_close(klibrary_t * lib) {
    if(!UnregisterLuaLib(lib_get_name())) {
        return -1;
    }
    return 0;
}
