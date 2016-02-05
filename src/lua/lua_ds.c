/****************************
 * DreamShell ##version##   *
 * lua_ds.h                 *
 * DreamShell LUA functions *
 * Created by SWAT          *
 ****************************/     
 

#include <kos.h>
#include <dirent.h>
#include "lua.h"
#include "list.h"
#include "module.h"
#include "app.h"
#include "console.h"
#include "utils.h"


static int l_ShowConsole(lua_State *L) {
	ShowConsole();
	return 0;
}

static int l_HideConsole(lua_State *L) {
	HideConsole();
	return 0;
}

static int l_PeriphExists(lua_State *L) {
	
	const char *s = luaL_checkstring(L, 1);
	
	if(PeriphExists(s)) {
		lua_pushnumber(L, 1);
	} else {
		lua_pushnil(L);
	}
	
	return 1;
}


static int l_OpenModule(lua_State *L) {
       
	const char *s = NULL;
	Module_t *m;

	s = luaL_checkstring(L, 1);
	m = OpenModule((char*)s);

	if(m != NULL) {
		lua_pushnumber(L, m->libid);
	} else {
		lua_pushnil(L);
	}
	
	return 1;
}


static int l_CloseModule(lua_State *L) {
	
	libid_t id = luaL_checkinteger(L, 1);
	Module_t *m = GetModuleById(id);
	
	if(m != NULL) {
		lua_pushnumber(L, CloseModule(m));
	} else {
		lua_pushnil(L);
	}
    
	return 1;
}


static int l_GetModuleByName(lua_State *L) {
	const char *s = NULL;
	Module_t *m;

	s = luaL_checkstring(L, 1);
	m = GetModuleByName(s);

	if(m != NULL) {
		lua_pushnumber(L, m->libid);
	} else {
		lua_pushnil(L);
	}
	return 1;
}


static int l_AddApp(lua_State *L) {
	const char *fn = NULL;
	App_t *app;

	fn = luaL_checkstring(L, 1);
	app = AddApp(fn);

	if(app != NULL) {
		lua_pushstring(L, app->name);
	} else {
		lua_pushnil(L);
	}
	
	return 1;
}


static int l_OpenApp(lua_State *L) {
	const char *s = NULL, *a = NULL;
	App_t *app;

	s = luaL_checkstring(L, 1);
	a = luaL_optstring(L, 2, NULL);
	
	app = GetAppByName(s);

	if(app != NULL && OpenApp(app, a)) {
		lua_pushnumber(L, app->id);
	} else {
		lua_pushnil(L);
	}
	
	return 1;
}


static int l_CloseApp(lua_State *L) {
	const char *s = NULL;
	int u = 1;
	App_t *app;

	s = luaL_checkstring(L, 1);
	u = luaL_optint(L, 2, 1);
	
	app = GetAppByName(s);

	if(app != NULL && CloseApp(app, u)) {
		lua_pushnumber(L, app->id);
	} else {
		lua_pushnil(L);
	}
	
	return 1;
}


static int l_set_dbgio(lua_State *L) {
    const char *s = NULL;
	
    s = luaL_checkstring(L, 1);
	
	if(!strncasecmp(s, "scif", 4)) {
		scif_init();
		dbgio_set_dev_scif();
		SetConsoleDebug(1);
	} else if(!strncasecmp(s, "dclsocket", 9)) {
		dbgio_dev_select("fs_dclsocket");
	} else if(!strncasecmp(s, "fb", 2)) {
		dbgio_set_dev_fb();
		SetConsoleDebug(1);
	} else if(!strncasecmp(s, "ds", 2)) {
		dbgio_set_dev_ds();
		SetConsoleDebug(0);
	} else if(!strncasecmp(s, "sd", 2)) {
		dbgio_set_dev_sd();
		SetConsoleDebug(1);
	} else {
		lua_pushnil(L);
		return 1;
	}
	
	lua_pushnumber(L, 1);
    return 1;
}


static int l_Sleep(lua_State *L) {
	int ms = luaL_checkinteger(L, 1);
	thd_sleep(ms);
	lua_pushnil(L);
	return 0;
}


static int l_bit_or(lua_State *L) {
	
	int a, b;
	
	a = luaL_checkinteger(L, 1);
	b = luaL_checkinteger(L, 2);
	
    lua_pushnumber(L, (a | b));
    return 1;
}

static int l_bit_and(lua_State *L) {
	
	int a, b;
	
	a = luaL_checkinteger(L, 1);
	b = luaL_checkinteger(L, 2);
	
    lua_pushnumber(L, (a & b));
    return 1;
}


static int l_bit_not(lua_State *L) {
	
	int a;
	
	a = luaL_checkinteger(L, 1);
	
    lua_pushnumber(L, ~a);
    return 1;
}

static int l_bit_xor(lua_State *L) {
	
	int a, b;
	
	a = luaL_checkinteger(L, 1);
	b = luaL_checkinteger(L, 2);
	
    lua_pushnumber(L, (a ^ b));
    return 1;
}



static int change_dir (lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	if (fs_chdir((char*)path)) {
		lua_pushnil (L);
		lua_pushfstring (L,"Unable to change working directory to '%s'\n", path);
		return 2;
	} else {
		lua_pushboolean (L, 1);
		return 1;
	}
}

static int get_dir (lua_State *L) {
	
	const char *path;
	if ((path = fs_getwd()) == NULL) {
		lua_pushnil(L);
		lua_pushstring(L, "getcwd error\n");
		return 2;
	} else {
		lua_pushstring(L, path);
		return 1;
	}
}



static int make_dir (lua_State *L) {
	const char *path = luaL_checkstring (L, 1);
	int fail;

	fail = fs_mkdir(path);

	if (fail < 0) {
		lua_pushnil (L);
		lua_pushfstring (L, "Can't make dir");
		return 2;
	}

	lua_pushboolean (L, 1);
	return 1;
}

/*
** Removes a directory.
** @param #1 Directory path.
*/
static int remove_dir (lua_State *L) {
	const char *path = luaL_checkstring (L, 1);
	int fail;

	fail = fs_rmdir(path);

	if (fail < 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "Can't remove dir");
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}


#define DIR_METATABLE "directory metatable"


typedef struct dir_data {
	int  closed;
	uint32 dir;
} dir_data;


/*
** Directory iterator
*/
static int dir_iter (lua_State *L) {

	dirent_t *entry;
	dir_data *d = (dir_data *)lua_touserdata (L, lua_upvalueindex (1));
	luaL_argcheck (L, !d->closed, 1, "closed directory");

	if ((entry = fs_readdir(d->dir)) != NULL) {
		
		lua_newtable (L);
		//ds_printf("entry = %s\n", entry->name);
		lua_pushstring(L, "name");
		lua_pushstring(L, entry->name);
		//lua_rawset(L, -3);
		lua_settable(L, -3);
		
		lua_pushstring(L, "size");
		lua_pushinteger(L, entry->size);
		//lua_rawset(L, -3);
		lua_settable(L, -3);
		
		lua_pushstring(L, "attr");
		lua_pushinteger(L, entry->attr);
		//lua_rawset(L, -3);
		lua_settable(L, -3);
		
		lua_pushstring(L, "time");
		lua_pushinteger(L, entry->time);
		//lua_rawset(L, -3);
		lua_settable(L, -3);
		return 1;

	} else {
		/* no more entries => close directory */
		fs_close(d->dir);
		d->closed = 1;
		return 0;
	}

	return 0;
}


/*
** Closes directory iterators
*/
static int dir_close (lua_State *L) {
	dir_data *d = (dir_data *)lua_touserdata (L, 1);

	if (!d->closed && d->dir) {
		fs_close(d->dir);
		d->closed = 1;
	}
	return 0;
}


/*
** Factory of directory iterators
*/
static int dir_iter_factory (lua_State *L) {
	const char *path = luaL_checkstring (L, 1);
	dir_data *d = (dir_data *) lua_newuserdata (L, sizeof(dir_data));
	d->closed = 0;

	luaL_getmetatable (L, DIR_METATABLE);
	lua_setmetatable (L, -2);
	
	d->dir = fs_open(path, O_RDONLY | O_DIR);
	if (d->dir < 0) luaL_error (L, "cannot open %s", path);
	lua_pushcclosure (L, dir_iter, 1);
	return 1;
}




/*
** Creates directory metatable.
*/
static int dir_create_meta (lua_State *L) {
	luaL_newmetatable (L, DIR_METATABLE);
	/* set its __gc field */
	lua_pushstring (L, "__gc");
	lua_pushcfunction (L, dir_close);
	lua_settable (L, -3);

	return 1;
}


static const struct luaL_reg bit_lib[] = {
	{"or", l_bit_or},
	{"and", l_bit_and},
	{"not", l_bit_not},
	{"xor", l_bit_xor},
	{NULL, NULL},
};


static const struct luaL_reg fs_lib[] = {
	{"chdir", change_dir},
	{"currentdir", get_dir},
	{"dir", dir_iter_factory},
	{"mkdir", make_dir},
	{"rmdir", remove_dir},
	{NULL, NULL},
};

int luaopen_lfs (lua_State *L) {
	dir_create_meta (L);
	luaL_register (L, "lfs", fs_lib);
	return 1;
}

int luaopen_bit (lua_State *L) {
	luaL_register (L, "bit", bit_lib);
	return 1;
}

void RegisterLuaFunctions(lua_State *L) {

	lua_register(L, "OpenModule", l_OpenModule);
	lua_register(L, "CloseModule", l_CloseModule);
	lua_register(L, "GetModuleByName", l_GetModuleByName);
	lua_register(L, "AddApp", l_AddApp);
	lua_register(L, "OpenApp", l_OpenApp);
	lua_register(L, "CloseApp", l_CloseApp);
	lua_register(L, "ShowConsole", l_ShowConsole);
	lua_register(L, "HideConsole", l_HideConsole);
	lua_register(L, "SetDebugIO", l_set_dbgio);
	lua_register(L, "Sleep", l_Sleep);
	lua_register(L, "MapleAttached", l_PeriphExists);

	luaopen_lfs(L);
	luaopen_bit(L);
}


