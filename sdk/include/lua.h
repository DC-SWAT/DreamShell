/** 
 * \file    lua.h
 * \brief   DreamShell LUA
 * \date    2006-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_LUA_H
#define _DS_LUA_H

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
