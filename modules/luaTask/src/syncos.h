/*
** $Id: syncos.h 10 2007-09-15 19:37:27Z danielq $
** Os Abstraction: Declarations
** SoongSoft, Argentina
** http://www.soongsoft.com mailto:dq@soongsoft.com
** Copyright (C) 2003-2006 Daniel Quintela.  All rights reserved.
*/

#ifndef SYNCOS_H_INCLUDED
#define SYNCOS_H_INCLUDED

#ifdef NATV_WIN32
    #define OS_THREAD_FUNC    unsigned __stdcall
	#define	OS_THREAD_T 	  long
#else
    #define OS_THREAD_FUNC    void *
	#define	OS_THREAD_T		  pthread_t
#endif

#ifdef NATV_WIN32
    typedef unsigned ( __stdcall *OS_THREAD )( void * );
#else
    typedef void * ( *OS_THREAD )( void * );
#endif

#ifndef _WIN32
    #define INFINITE    -1
#endif

long OsCreateThread( OS_THREAD_T *th, OS_THREAD threadfunc, void *param);
long OsCancelThread( OS_THREAD_T th);
long OsCreateEvent( char *name);
long OsSetEvent( long evt);
void OsCloseEvent( long evt);
void * OsCreateMutex( char *name);
long OsLockMutex( void *mtx, long timeout);
long OsUnlockMutex( void * mtx);
void OsCloseMutex( void * mtx);
long OsCreateThreadDataKey();
void OsSetThreadData( long key, const void *tdata);
void *OsGetThreadData( long key);
void OsSleep( long ms);

#endif
