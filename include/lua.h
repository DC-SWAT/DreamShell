/** 
 * \file    lua.h
 * \brief   DreamShell LUA
 * \date    2006-2014, 2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_LUA_H
#define _DS_LUA_H

#include <stddef.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

void luaB_set_fputs(void (*f)(const char *));

#define LUA_DO_FILE 0
#define LUA_DO_STRING 1
#define LUA_DO_LIBRARY 2

int lua_packlibopen(lua_State *L);


int InitLua(); 
void ShutdownLua(); 
void ResetLua();

lua_State *GetLuaState(); 
void SetLuaState(lua_State *L);

lua_State *NewLuaThread();
void ReleaseLuaThread(lua_State *L);

typedef struct {
	lua_State *L;
	int idx;
} LuaConfig_t;

int LuaOpenConfigFile(const char *path, LuaConfig_t *cfg);
void LuaCloseConfigFile(LuaConfig_t *cfg);
int LuaConfigGetInt(const LuaConfig_t *cfg, const char *key, int current);
float LuaConfigGetFloat(const LuaConfig_t *cfg, const char *key, float current);
int LuaConfigGetString(const LuaConfig_t *cfg, const char *key, char *dst, size_t size);
int LuaConfigGetStringArray(const LuaConfig_t *cfg, const char *key,
		char *buf, size_t elem_size, int max_count);

typedef void LuaRegLibOpen(lua_State *);
int RegisterLuaLib(const char *name, LuaRegLibOpen *func);
int UnregisterLuaLib(const char *name);

void LuaOpenlibs(lua_State *L); 
void RegisterLuaFunctions(lua_State *L);
void LuaPushArgs(lua_State *L, char *argv[]); 
void LuaAddArgs(lua_State *L, char *argv[]);

int lua_traceback(lua_State *L);
int lua_docall(lua_State *L, int narg, int clear);
int lua_report(lua_State *L, int status);

int LuaDo(int type, const char *str_or_file, lua_State *lu);
int RunLuaScript(char *fn, char *argv[]);


#endif
