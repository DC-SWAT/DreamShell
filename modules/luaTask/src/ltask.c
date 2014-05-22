/*
** $Id: ltask.c 22 2007-11-24 17:29:24Z danielq $
** "Multitasking" support ( see README )
** SoongSoft, Argentina
** http://www.soongsoft.com mailto:dq@soongsoft.com
** Copyright (C) 2003-2007 Daniel Quintela.  All rights reserved.
*/

#include <lauxlib.h>
#include <lua.h>
#include "lualib.h"

#ifdef _WIN32
#   include <windows.h>
#   include <process.h>
#   include <stddef.h>
    #ifndef NATV_WIN32
        #include <pthread.h>
    #endif
#else
#   define _strdup strdup
#   define _strnicmp strncasecmp
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
//#   include <poll.h>
#   include <sys/types.h>
#   include <string.h>
#   include <pthread.h>
#endif

#define LT_NAMESPACE    "task"

#include <time.h>

#include <signal.h>
#include <poll.h>

#include "syncos.h"
#include "queue.h"

#ifdef _WIN32
static long ( __stdcall *LRT_LIB_OVERRIDE)( lua_State *L) = NULL;
static long ( __stdcall *LRT_DOFILE_OVERRIDE)( lua_State *L, const char *filename) = NULL;
#else
long ( *LRT_LIB_OVERRIDE)( lua_State *L) = NULL;
long ( *LRT_DOFILE_OVERRIDE)( lua_State *L, const char *filename) = NULL;
#endif

#define TASK_SLOTS_STEP 256

typedef struct S_TASK_ENTRY {
    QUEUE       queue;
    char        *fname;
    long		flon;
    lua_State   *L;
    OS_THREAD_T th;
    long        running;
    char        *id;
    long        slot;
} TASK_ENTRY;

typedef struct S_MSG_ENTRY {
    long        len;
    long        flags;
    char        data[1];
} MSG_ENTRY;

typedef struct S_LOPN_LIB {
    char            *name;
    unsigned long   value;
    int             (*fnc)(lua_State *);
} LOPN_LIB;

static void * tlMutex = NULL;
static TASK_ENTRY * * volatile aTask;
static long countTask = 0;
static long threadDataKey;

/* Internal functions */
static OS_THREAD_FUNC taskthread( void *vp);

static int traceback (lua_State *L) {
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

static int docall (lua_State *L, int narg, int clear) {
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

static int dofile (lua_State *L, const char *name) {
  int status = luaL_loadfile(L, name) || docall(L, 0, 1);
  return status;
}


static int dostring (lua_State *L, const char *s, long l, const char *name) {
  int status = luaL_loadbuffer(L, s, l, name) || docall(L, 0, 1);
  return status;
}

static void taskCleanup( void *vp) {
    TASK_ENTRY *te = ( TASK_ENTRY *) vp;
    
    lua_close( te->L);
    
    te->L = NULL;
    free( te->fname);
    if( te->id != NULL)
        free( te->id);
    QueDestroy( &( te->queue));
    te->running = 0;
}

static long int_taskcreate( const char *fname, long flon, lua_State *TL) {
    if( tlMutex != NULL) {
        long i;

        OsLockMutex( tlMutex, INFINITE);

        for( i = 0; i < countTask; i++)
            if( !( aTask[i]->running))
                break;
        if( i == countTask) {
            TASK_ENTRY **te;
            long j;

            te = ( TASK_ENTRY * *) realloc( aTask, sizeof( TASK_ENTRY *) * ( countTask + TASK_SLOTS_STEP));
            if( te == NULL) {
                OsUnlockMutex( tlMutex);
                return( -1);
            }
            aTask = te;
            
            aTask[i] = ( TASK_ENTRY *) malloc( sizeof( TASK_ENTRY) * ( TASK_SLOTS_STEP));
            if( aTask[i] == NULL) {
                OsUnlockMutex( tlMutex);
                return( -1);
            }
                    
            countTask += TASK_SLOTS_STEP;
            for( j = i; j < countTask; j++) {
                aTask[j] = ( TASK_ENTRY *) ( ( ( char * ) aTask[i] ) + ( sizeof( TASK_ENTRY) * ( j - i ) ) );
                aTask[j]->running = 0;
#ifdef NATV_WIN32
                aTask[j]->th = 0;
#endif
                aTask[j]->slot = j;
            }
        }

#ifdef NATV_WIN32
        if( aTask[i]->th)
            CloseHandle( ( HANDLE) aTask[i]->th);
#endif
    
        if( ( aTask[i]->fname = calloc( flon + 1, 1 ) ) == NULL) {
            OsUnlockMutex( tlMutex);
            return( -2);
        } else {
        	memcpy( aTask[i]->fname, fname, flon );
        	aTask[i]->flon = flon;
            if( QueCreate( &( aTask[i]->queue), QUE_NO_LIMIT)) {
                free( aTask[i]->fname);
                OsUnlockMutex( tlMutex);
                return( -3);
            }
        }

        aTask[i]->id = NULL;

        aTask[i]->L = TL;

		if( OsCreateThread( &( aTask[i]->th), taskthread, ( void *) ( aTask[i] ))) {
            free( aTask[i]->fname);
            QueDestroy( &( aTask[i]->queue));
            OsUnlockMutex( tlMutex);
            return( -4);
        }

        aTask[i]->running = 1;

        OsUnlockMutex( tlMutex);
        return( i + 1);
    }

    return( -11);
}

static long int_taskregister( const char *id ) {
    long lrc = -1;
    TASK_ENTRY * te = ( TASK_ENTRY * ) OsGetThreadData( threadDataKey );

    if( te != NULL ) {
        OsLockMutex( tlMutex, INFINITE);
        if( te->id != NULL )
            free( te->id);
        te->id = _strdup( id );
        lrc = 0;
        OsUnlockMutex( tlMutex );
    }

    return( lrc);
}

static void int_tasklibinit(lua_State *L) {
    if( tlMutex == NULL) {
        threadDataKey = OsCreateThreadDataKey();
        aTask = ( TASK_ENTRY * *) malloc( sizeof( TASK_ENTRY *) * TASK_SLOTS_STEP);
        if( aTask != NULL) {
            long i;
            
            aTask[0] = ( TASK_ENTRY *) malloc( sizeof( TASK_ENTRY) * TASK_SLOTS_STEP);
            
            if( aTask[0] != NULL) {

                countTask = TASK_SLOTS_STEP;
                for( i = 0; i < TASK_SLOTS_STEP; i++) {
                    aTask[i] = ( TASK_ENTRY *) ( ( ( char * ) aTask[0] ) + ( sizeof( TASK_ENTRY) * i ) );
                    aTask[i]->running = 0;
#ifdef NATV_WIN32
                    aTask[i]->th = 0;
#endif
                    aTask[i]->slot = i;
                }
                tlMutex = OsCreateMutex( NULL);
                QueCreate( &( aTask[0]->queue), QUE_NO_LIMIT);
                aTask[0]->L = L;
                aTask[0]->running = 1;
                aTask[0]->id = NULL;
                aTask[0]->fname = "_main_";
                aTask[0]->flon = 7;
                aTask[0]->slot = 0;
                OsSetThreadData( threadDataKey, aTask[0] );
            }
        }
#ifdef _WIN32
        LRT_LIB_OVERRIDE = ( long ( __stdcall *)( lua_State *L))
            GetProcAddress( GetModuleHandle( NULL), "_LRT_LIB_OVERRIDE@4");
        LRT_DOFILE_OVERRIDE = ( long ( __stdcall *)( lua_State *L, const char *filename))
            GetProcAddress( GetModuleHandle( NULL), "_LRT_DOFILE_OVERRIDE@8");
#endif
    }
}


/* Registered functions */

static int reg_taskcreate(lua_State *L) {
    long lrc;
    long ti = 0;
    size_t flon;
    const char *fname = luaL_checklstring(L, 1, &flon);
    lua_State *TL = lua_open();
        
    lua_newtable(TL);
    lua_pushnumber(TL, 0);
    lua_pushlstring(TL, fname, flon);
    lua_settable( TL, -3);

    if( lua_istable(L, 2)) {
        lua_pushnil(L);
        while( lua_next(L, 2) != 0) {
            if( lua_isnumber(L, -1)) {
                lua_pushnumber( TL, ++ti);
                lua_pushnumber( TL, lua_tonumber( L, -1));
                lua_settable( TL, -3);
            } else if( lua_isstring(L, -1)) {
                lua_pushnumber( TL, ++ti);
                lua_pushstring( TL, lua_tostring( L, -1));
                lua_settable( TL, -3);
            } else {
                lua_pop(L, 1);
                break;
            }
            lua_pop(L, 1);
        }
    }
    
    lua_setglobal(TL, "arg");
    
    lrc = int_taskcreate( fname, flon, TL);

    if( lrc < 0)
        lua_close( TL);

    lua_pushnumber( L, lrc);

    return 1;
}

static int reg_taskregister( lua_State *L) {
    long lrc;
    const char *id = luaL_checkstring(L, 1);

    lrc = int_taskregister( id);

    lua_pushnumber( L, lrc);

    return( 1);
}

static int reg_taskfind( lua_State *L) {
    long i;
    long lrc = -1;
    const char *id = luaL_checkstring(L, 1);

    OsLockMutex( tlMutex, INFINITE);
    
    for( i = 0; i < countTask; i++)
        if( aTask[i]->id != NULL)
            if( !strcmp( aTask[i]->id, id)) {
                lrc = i + 1;
                break;
            }

    OsUnlockMutex( tlMutex);

    lua_pushnumber( L, lrc);

    return( 1);
}

static int reg_taskunregister(lua_State *L) {
    long lrc;

    lrc = int_taskregister( "");

    lua_pushnumber( L, lrc);

    return( 1);
}

static int reg_taskpost(lua_State *L) {
    const char *buffer;
    MSG_ENTRY *me;
    TASK_ENTRY * volatile te;
    size_t len;
    long idx = ( long) luaL_checknumber(L, 1);
    long flags = ( long) luaL_optinteger(L, 3, 0);
    long lrc = -1;
    buffer   =         luaL_checklstring(L, 2, &len);

    idx--;

    if( ( idx > -1) && ( idx < countTask)) {

        me = malloc( sizeof( MSG_ENTRY) + len - 1);
        if( me == NULL) {
            lrc = -2;
        } else {
            me->len = len;
            me->flags = flags;
            memcpy( me->data, buffer, len);
            OsLockMutex( tlMutex, INFINITE);
            te = aTask[idx];
            OsUnlockMutex( tlMutex);
            QuePut( &( te->queue), me);
            lrc = 0;
        }
    }

    lua_pushnumber( L, lrc);

    return 1;
}

static int reg_tasklist(lua_State *L) {
    long i;
    
    lua_newtable( L);
    
    OsLockMutex( tlMutex, INFINITE);
    for( i = 0; i < countTask; i++)
        if( aTask[i]->running == 1 ) {
            lua_pushnumber( L, i + 1);
            lua_newtable( L);
            lua_pushstring( L, "script");
			if( aTask[i]->fname[0] == '=' )
				lua_pushstring( L, "STRING_TASK");
			else
				lua_pushlstring( L, aTask[i]->fname, aTask[i]->flon);
            lua_settable( L, -3);
            lua_pushstring( L, "msgcount");
            lua_pushnumber( L, aTask[i]->queue.msgcount);
            lua_settable( L, -3);
            if( aTask[i]->id != NULL) {
                lua_pushstring( L, "id");
                lua_pushstring( L, aTask[i]->id);
                lua_settable( L, -3);
            }
            lua_settable( L, -3);
        }
    OsUnlockMutex( tlMutex);
    
    return 1;
}

static int reg_taskreceive(lua_State *L) {
    MSG_ENTRY *me;
    TASK_ENTRY *te = ( TASK_ENTRY * ) OsGetThreadData( threadDataKey );
    long lrc = -1;
    long tout = ( long) luaL_optinteger(L, 1, INFINITE);

    if( te != NULL ) {
#ifdef _WIN32
        HANDLE qwh = ( HANDLE) GetQueNotEmptyHandle( &( te->queue));
        DWORD dwWait = WaitForSingleObjectEx( qwh, tout, TRUE);
        if( ( te->running == 1) && ( dwWait == WAIT_OBJECT_0)) {
#else
        int psw;
        struct pollfd pfd;
        pfd.fd = GetQueNotEmptyHandle( &( te->queue));
        pfd.events = POLLIN;
        psw = poll( &pfd, 1, tout);
        if( ( te->running == 1) && ( psw == 1)) {
#endif
            _QueGet( &( te->queue), ( void **) &me);
            lua_pushlstring( L, me->data, me->len);
            lua_pushnumber( L, me->flags);
            free( me);
            lrc = 0;
        } else {
            lua_pushnil( L);
            lua_pushnil( L);
            lrc = -2;
        }
    } else {
        lua_pushnil( L);
        lua_pushnil( L);
    }
    
    lua_pushnumber( L, lrc);

    return 3;
}

static int reg_taskid( lua_State *L) {
    TASK_ENTRY * te = ( TASK_ENTRY * ) OsGetThreadData( threadDataKey );

    if(te != NULL )
        lua_pushnumber( L, te->slot + 1);
    else
        lua_pushnumber( L, -1);

    return( 1);
}


static int reg_getqhandle( lua_State *L) {
    long qwh = 0;
    TASK_ENTRY * te = ( TASK_ENTRY * ) OsGetThreadData( threadDataKey);

    if(te != NULL ) {
        qwh = GetQueNotEmptyHandle( &( te->queue));
    }

    lua_pushnumber( L, ( long) qwh);

    return( 1);
}

static int reg_cancel( lua_State *L) {
    long running;
    TASK_ENTRY * te = ( TASK_ENTRY * ) OsGetThreadData( threadDataKey);
    long lrc = -1;
    long i = ( long) luaL_checknumber(L, 1);

    OsLockMutex( tlMutex, INFINITE);

    if( ( i > 1) && ( i <= countTask))
        if( --i != te->slot ) {
            lrc = 0;
            running = aTask[i]->running;
            if( running == 1) {
                aTask[i]->running = 2;
                lrc = OsCancelThread( aTask[i]->th);
#ifdef NATV_WIN32
                if( aTask[i]->running == 2)
                    aTask[i]->running = 0;
#endif
            }
        }

    OsUnlockMutex( tlMutex);

    lua_pushnumber( L, lrc);

    return( 1);
}

static int reg_isrunning( lua_State *L) {
    long running = 0;
    long i = ( long) luaL_checknumber(L, 1);

    OsLockMutex( tlMutex, INFINITE);

    if( ( i > 1) && ( i <= countTask))
        running = aTask[--i]->running == 1 ? 1 : 0;

    OsUnlockMutex( tlMutex);

    lua_pushboolean( L, running);

    return( 1);
}

static int reg_sleep( lua_State *L) {
    long t = ( long) luaL_checknumber(L, 1);

    OsSleep( t);

    return( 0);
}

/* Module exported function */

static const struct luaL_reg lt_lib[] = {
    { "create",     reg_taskcreate},
    { "register",   reg_taskregister},
    { "find",       reg_taskfind},
    { "receive",    reg_taskreceive},
    { "post",       reg_taskpost},
    { "unregister", reg_taskunregister},
    { "list",       reg_tasklist},
    { "id",         reg_taskid},
    { "getqhandle", reg_getqhandle},
    { "cancel",     reg_cancel},
    { "isrunning",  reg_isrunning},
    { "sleep",      reg_sleep},
    { NULL,         NULL}
};

int luaopen_task(lua_State *L) {
	int_tasklibinit(L);
    luaL_openlib (L, LT_NAMESPACE, lt_lib, 0);
    lua_pop (L, 1);
    return 0;
}

static OS_THREAD_FUNC taskthread( void *vp) {
    TASK_ENTRY *te;
	int status = 0;
#if (_MSC_VER >= 1400)
	size_t l_init;
	char *init;
	_dupenv_s(&init,&l_init,"LUA_INIT");
#else
	const char *init = getenv("LUA_INIT");
#endif

    OsLockMutex( tlMutex, INFINITE);
    
    OsSetThreadData( threadDataKey, vp );
    te = ( TASK_ENTRY * ) vp;

#ifndef NATV_WIN32
    pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_cleanup_push( taskCleanup, te);
#endif

	lua_gc(te->L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(te->L);  /* open libraries */
    luaopen_task(te->L);
	lua_gc(te->L, LUA_GCRESTART, 0);
    if( LRT_LIB_OVERRIDE != NULL)
        LRT_LIB_OVERRIDE( te->L);

    OsUnlockMutex( tlMutex);

	if (init != NULL) {
		if (init[0] == '@')
			status = dofile( te->L, init+1);
		else
			status = dostring( te->L, init, strlen( init), "=LUA_INIT");
#if (_MSC_VER >= 1400)
		free(init);
#endif
	}

	if (status == 0) {
		if( te->fname[0] == '=' )
			dostring( te->L, te->fname + 1, te->flon - 1, "=STRING_TASK");
		else {
			if( LRT_DOFILE_OVERRIDE != NULL)
				LRT_DOFILE_OVERRIDE( te->L, te->fname);
			else
				dofile( te->L, te->fname);
		}
	}
    
    OsLockMutex( tlMutex, INFINITE);
    
#ifndef NATV_WIN32
    pthread_cleanup_pop( 0);
#endif

    taskCleanup( te);
    
    OsUnlockMutex( tlMutex);

    return( 0);
}

