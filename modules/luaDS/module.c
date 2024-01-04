/* DreamShell ##version##

   module.c - luaDS module
   Copyright (C)2007-2013 SWAT 

*/
            
#include <kos.h>
#include <kos/exports.h>
#include "ds.h"

DEFAULT_MODULE_HEADER(luaDS);

int tolua_DS_open(lua_State* tolua_S);

int lib_open(klibrary_t * lib) {
	lua_State *L = GetLuaState();
	
	if(L != NULL) {
		tolua_DS_open(L);
	}
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)tolua_DS_open);
    return nmmgr_handler_add(&ds_luaDS_hnd.nmmgr);
}

typedef struct lua_hnd {
	
	const char *lua;
	const char *cmd;
	
} lua_hnd_t;

static lua_hnd_t hnds[32];
static int cur_hnds_size = 0;


int lib_close(klibrary_t * lib) {
	
	UnregisterLuaLib(lib_get_name());
	
	if(cur_hnds_size) {
		
		int i;
		for(i = 0; i < cur_hnds_size; i++) {
			RemoveCmd(GetCmdByName(hnds[i].cmd));
		}
		//free(hnds);
		memset(&hnds, 0, sizeof(hnds));
		cur_hnds_size = 0;
	}
	
    return nmmgr_handler_remove(&ds_luaDS_hnd.nmmgr); 
}



static int LuaCmdHandler(int argc, char *argv[]) {
	
	int i, j;
	lua_State *L = GetLuaState();
	//ds_printf("CALL: %s\n", argv[0]);
	
	for(i = 0; i < cur_hnds_size; i++) {
		//ds_printf("FIND: %s\n", hnds[i].cmd);
		if(!strcmp(hnds[i].cmd, argv[0])) {
			lua_getfield(L, LUA_GLOBALSINDEX, hnds[i].lua); 

			if(lua_type(L, -1) != LUA_TFUNCTION) {
				ds_printf("DS_ERROR: Can't find function: \"%s\" Command handlers support only global functions.\n", hnds[i].lua);
				return CMD_NOT_EXISTS;
			}
			
			lua_pushnumber(L, argc);
			lua_newtable(L);
			
			for (j = 0; j < argc; j++) {
				lua_pushnumber(L, j);
				lua_pushstring(L, argv[j]);
				lua_settable(L, -3);
			}
			EXPT_GUARD_BEGIN;
				lua_report(L, lua_docall(L, 2, 1));
			EXPT_GUARD_CATCH;
				ds_printf("DS_ERROR: lua command failed: %s\n", argv[0]);
			EXPT_GUARD_END;
			//return luaL_checkinteger(L, 1);
			return CMD_OK;
		}
	}
	
	return CMD_NOT_EXISTS;
}



Cmd_t *AddCmdLua(const char *cmd, const char *helpmsg, const char *handler) {

	cur_hnds_size++;
	/*
	if(cur_hnds_size == 1) {
		hnds = (lua_hnd_t**)malloc(sizeof(lua_hnd_t) * cur_hnds_size);
	} else {
		hnds = (lua_hnd_t**)realloc(hnds, sizeof(lua_hnd_t) * cur_hnds_size);
	}*/
	
	hnds[cur_hnds_size-1].cmd = cmd;
	hnds[cur_hnds_size-1].lua = handler;
	
	//strcpy((char*)hnds[cur_hnds_size-1]->cmd, (char*)cmd);
	//strcpy((char*)hnds[cur_hnds_size-1]->lua, (char*)handler);
	
	//ds_printf("ADD: %s\n", hnds[cur_hnds_size-1].cmd);
	
	return AddCmd(cmd, helpmsg, LuaCmdHandler);
}



static void LuaEvent(void *ds_event, void *sdl_event, int action) {
	
	SDL_Event *event = NULL;
	Event_t *devent = (Event_t *) ds_event;
	const char *lua = (const char *) devent->param;
	lua_State *L = GetLuaState();
	int narg = 0;
	
	lua_getfield(L, LUA_GLOBALSINDEX, lua); 

	if(lua_type(L, -1) != LUA_TFUNCTION) {
		ds_printf("DS_ERROR: Can't find function: \"%s\" Event handlers support only global functions.\n", lua);
		return;
	}
	
	if(devent->type == EVENT_TYPE_INPUT) {
		narg = 1;
		event = (SDL_Event *) sdl_event;
		lua_createtable(L,1,8);
		lua_pushliteral(L,"type");
		lua_pushnumber(L,event->type);
		lua_settable(L,-3);
		lua_pushliteral(L,"active");
		lua_createtable(L,0,2);
		lua_pushliteral(L,"gain");
		lua_pushnumber(L,event->active.gain);
		lua_settable(L,-3);
		lua_pushliteral(L,"state");
		lua_pushnumber(L,event->active.state);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"key");
		lua_createtable(L,0,3);
		lua_pushliteral(L,"type");
		lua_pushnumber(L,event->key.type);
		lua_settable(L,-3);
		lua_pushliteral(L,"state");
		lua_pushnumber(L,event->key.state);
		lua_settable(L,-3);
		lua_pushliteral(L,"keysym");
		lua_createtable(L,0,3);
		lua_pushliteral(L,"sym");
		lua_pushnumber(L,event->key.keysym.sym);
		lua_settable(L,-3);
		lua_pushliteral(L,"mod");
		lua_pushnumber(L,event->key.keysym.mod);
		lua_settable(L,-3);
		lua_pushliteral(L,"unicode");
		lua_pushnumber(L,event->key.keysym.unicode);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"motion");
		lua_createtable(L,0,3);
		lua_pushliteral(L,"state");
		lua_pushnumber(L,event->motion.state);
		lua_settable(L,-3);
		lua_pushliteral(L,"x");
		lua_pushnumber(L,event->motion.x);
		lua_settable(L,-3);
		lua_pushliteral(L,"y");
		lua_pushnumber(L,event->motion.y);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"button");
		lua_createtable(L,0,5);
		lua_pushliteral(L,"type");
		lua_pushnumber(L,event->button.type);
		lua_settable(L,-3);
		lua_pushliteral(L,"button");
		lua_pushnumber(L,event->button.button);
		lua_settable(L,-3);
		lua_pushliteral(L,"state");
		lua_pushnumber(L,event->button.state);
		lua_settable(L,-3);
		lua_pushliteral(L,"x");
		lua_pushnumber(L,event->button.x);
		lua_settable(L,-3);
		lua_pushliteral(L,"y");
		lua_pushnumber(L,event->button.y);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"jaxis");
		lua_createtable(L,0,2);
		lua_pushliteral(L,"axis");
		lua_pushnumber(L,event->jaxis.axis);
		lua_settable(L,-3);
		lua_pushliteral(L,"value");
		lua_pushnumber(L,event->jaxis.value);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"jhat");
		lua_createtable(L,0,2);
		lua_pushliteral(L,"hat");
		lua_pushnumber(L,event->jhat.hat);
		lua_settable(L,-3);
		lua_pushliteral(L,"value");
		lua_pushnumber(L,event->jhat.value);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"jbutton");
		lua_createtable(L,0,3);
		lua_pushliteral(L,"type");
		lua_pushnumber(L,event->jbutton.type);
		lua_settable(L,-3);
		lua_pushliteral(L,"button");
		lua_pushnumber(L,event->jbutton.button);
		lua_settable(L,-3);
		lua_pushliteral(L,"state");
		lua_pushnumber(L,event->jbutton.state);
		lua_settable(L,-3);
		lua_settable(L,-3);
		lua_pushliteral(L,"user");
		lua_createtable(L,0,2);
		lua_pushliteral(L,"type");
		lua_pushnumber(L,event->user.type);
		lua_settable(L,-3);
		lua_pushliteral(L,"code");
		lua_pushnumber(L,event->user.code);
		lua_settable(L,-3);
		lua_settable(L,-3);
	}

	EXPT_GUARD_BEGIN;
		lua_report(L, lua_docall(L, narg, 1));
	EXPT_GUARD_CATCH;
		ds_printf("DS_ERROR: lua event failed: %s\n", lua);
	EXPT_GUARD_END;
}


Event_t *AddEventLua(const char *name, int type, int prio, const char *handler) {
	return AddEvent(name, type, prio, LuaEvent, (void *)handler);
}
