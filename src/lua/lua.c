/****************************
 * DreamShell ##version##   *
 * lua.c                    *
 * DreamShell LUA           *
 * Created by SWAT          *
 ****************************/     
 
 
#include "lua.h"
#include "list.h"
#include "module.h"
#include "utils.h"
#include "console.h"
#include "exceptions.h"


static lua_State *DSLua = NULL;
static Item_list_t *lua_libs;


void LuaOpenlibs(lua_State *L) {

	/* Open stock libs */
	luaL_openlibs(L);

	/* Open additional libs */
	lua_packlibopen(L); 

	/* Custom functions */  
	RegisterLuaFunctions(L);
  
	Item_t *i;
	LuaRegLibOpen *openFunc;
	
	SLIST_FOREACH(i, lua_libs, list) {

		openFunc = (LuaRegLibOpen *) i->data;

		if(openFunc != NULL) {
			//ds_printf("DS_PROCESS: Opening lua library: %s\n", i->name);
			EXPT_GUARD_BEGIN;
				openFunc(L);
			EXPT_GUARD_CATCH;
				ds_printf("DS_ERROR: Can't open lua library: %s\n", i->name);
			EXPT_GUARD_END;
		}
	}
}


int InitLua() {
	
	//ds_printf("DS_PROCESS: Init lua library...\n");  
	luaB_set_fputs((void (*)(const char *))ds_printf);

	DSLua = luaL_newstate();//lua_open(); 

	if (DSLua == NULL) {
		ds_printf("DS_ERROR_LUA: Invalid state.. giving up\n");
		return -1; 
	}
	
	if((lua_libs = listMake()) == NULL) {
		return -1;
	}
	LuaOpenlibs(DSLua);  
	//ds_printf("DS_OK: Lua library inited.\n");
	return 0;
}


void ShutdownLua() {
	listDestroy(lua_libs, (listFreeItemFunc *) free); 
	lua_close(DSLua); 
}


void ResetLua() {
	ds_printf("DS_PROCESS: Reset lua state...\n");
	lua_close(DSLua); 
	DSLua = luaL_newstate(); 
	LuaOpenlibs(DSLua);  
}


int RegisterLuaLib(const char *name, LuaRegLibOpen *func) {
	
	if(listAddItem(lua_libs, LIST_ITEM_LUA_LIB, name, func, sizeof(LuaRegLibOpen)) != NULL){
		return 1;
	}
	return 0;
}


int UnregisterLuaLib(const char *name) {
	
	Item_t *i = listGetItemByName(lua_libs, name);
	
	if(i == NULL) {
		return 0;
	}
	
	listRemoveItem(lua_libs, i, NULL);
	return 1;
}


int lua_report (lua_State *L, int status) {
	if (status && !lua_isnil(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error object is not a string)";
		ds_printf(msg);
		lua_pop(L, 1);
	}
	return status;
}


int lua_traceback(lua_State *L) {
	if (!lua_isstring(L, 1))  /* 'message' not a string? */
		return 1;  /* keep it intact */
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}


int lua_docall (lua_State *L, int narg, int clear) {
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, lua_traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */
	status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
	lua_remove(L, base);  /* remove traceback function */
	/* force a complete garbage collection in case of errors */
	if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
	return status;
}



static int dofile (lua_State *L, const char *name) {
	int status = luaL_loadfile(L, name) || lua_docall(L, 0, 1);
	return lua_report(L, status);
}


static int dostring (lua_State *L, const char *s, const char *name) {
	int status = luaL_loadbuffer(L, s, strlen(s), name) || lua_docall(L, 0, 1);
	return lua_report(L, status);
}

static int dolibrary (lua_State *L, const char *name) {
	lua_getglobal(L, "require");
	lua_pushstring(L, name);
	return lua_report(L, lua_docall(L, 1, 1));
}



int LuaDo(int type, const char *str_or_file, lua_State *lu) {
	int res = LUA_ERRRUN;

	EXPT_GUARD_BEGIN;

		if(type == LUA_DO_FILE) {
			res = dofile(lu, str_or_file);
		} else if(type == LUA_DO_STRING) {
			res = dostring(lu, str_or_file, "=(DreamShell)");
		} else if(type == LUA_DO_LIBRARY) {
			res = dolibrary(lu, str_or_file);
		} else  {
			ds_printf("DS_ERROR: LuaDo type error: %d\n", type);
		}

	EXPT_GUARD_CATCH;
	
		res = LUA_ERRRUN;
		ds_printf("DS_ERROR: Exception in lua script: %s\n", str_or_file);

	EXPT_GUARD_END;
  
	/* Lua gives no message in such cases, so lua.c provides one */
	if (res == LUA_ERRMEM) {
		ds_printf("LUA: Memory allocation error!\n");
	} else if (res == LUA_ERRFILE)
		ds_printf("LUA: Can't open lua file %s\n", str_or_file);
	else if (res == LUA_ERRSYNTAX)
		ds_printf("LUA: Syntax error in lua script %s\n", str_or_file);
	else if (res == LUA_ERRRUN)
		ds_printf("LUA: Error at management chunk!\n");
	else if (res == LUA_ERRERR)
		ds_printf("LUA: Error in error message!\n");
		
	return res;
}



void LuaPushArgs(lua_State *L, char *argv[]) {
	int i;
	lua_newtable(L);
	for (i=0; argv[i]; i++) {
		/* arg[i] = argv[i] */
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i]);
		lua_settable(L, -3);
	}
	/* argv.n = maximum index in table `argv' */
	lua_pushstring(L, "n");
	lua_pushnumber(L, i-1);
	lua_settable(L, -3);
}


void LuaAddArgs(lua_State *L, char *argv[]) {

     LuaPushArgs(L, argv);
     lua_setglobal(L, "argv");
}


int RunLuaScript(char *fn, char *argv[]) {
    
    lua_State *L = NewLuaThread(); 
    
    if(L == NULL) {
        ds_printf("DS_ERROR: LUA: Invalid state.. giving up\n");
        return 0; 
    }
    
    if(argv != NULL) {
        LuaPushArgs(L, argv);
        lua_setglobal(L, "argv");
    }
    
    
    LuaDo(LUA_DO_FILE, fn, L);
    lua_close(L); 
    return 1;
}



lua_State *GetLuaState() {
	return DSLua; 
}

void SetLuaState(lua_State *l) {
	DSLua = l; 
}

lua_State *NewLuaThread()  {
      return lua_newthread(DSLua);
	  //lua_State *L = lua_open(); 
	  //LuaOpenlibs(L);
	  //return L;
}
