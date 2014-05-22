/*
** $Id: syncos.c 10 2007-09-15 19:37:27Z danielq $
** Os Abstraction: Implementation
** SoongSoft, Argentina
** http://www.soongsoft.com mailto:dq@soongsoft.com
** Copyright (C) 2003-2006 Daniel Quintela.  All rights reserved.
*/

#ifdef _WIN32
    #include <winsock2.h>
    #include <process.h>
#else
    #define TRUE    1
    #define FALSE   0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
    #include <sys/types.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
    #ifndef NATV_WIN32
        #include <pthread.h>
    #endif
#else
    #include <errno.h>
    #include <pthread.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <signal.h>
    //#include <sys/un.h>
    #include <netinet/in.h>
#endif

#include "syncos.h"

#include <fcntl.h>

long OsCreateThread( OS_THREAD_T *th, OS_THREAD threadfunc, void *param) {
#ifdef NATV_WIN32
    unsigned        tid;

    *th = _beginthreadex( NULL, 0, threadfunc, param, 0, &tid);

    return( th == NULL ? -1 : 0);
#else
	pthread_attr_t  attr;
    int             status;

	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
#ifdef LUATASK_PTHREAD_STACK_SIZE
    pthread_attr_setstacksize( &attr, LUATASK_PTHREAD_STACK_SIZE );
#endif
    status = pthread_create( th, &attr, threadfunc, param);
	pthread_attr_destroy( &attr);

    return( status);
#endif
}

long OsCancelThread( OS_THREAD_T th) {
#ifdef NATV_WIN32
    /* Not supported */
	return( -1);
#else
    return( pthread_cancel( th));
#endif
}

//long OsCreateEvent( char *name ) {
//#ifdef _WIN32
//    HANDLE eh;
//
//    eh = CreateEvent( NULL, TRUE, FALSE, name);
//
//    return( ( long) eh);
//#else
//    int *pfd;
//    pfd = calloc( sizeof( int), 2);
//    if( pfd != NULL) {
//        if( pipe( pfd)) {
//            free( pfd);
//            pfd = NULL;
//        }
//    }
//    return( ( long) pfd);
//#endif
//}
//
//long OsSetEvent( long evt) {
//#ifdef _WIN32
//    return( SetEvent( ( HANDLE) evt) ? 0 : -1);
//#else
//    return( write( ( ( int *) evt)[1], "", 1));
//#endif
//}
//
//void OsCloseEvent( long evt) {
//#ifdef _WIN32
//    CloseHandle( ( HANDLE) evt);
//#else
//    close( ( ( int *) evt)[0]);
//    close( ( ( int *) evt)[1]);
//    free( ( ( int *) evt));
//#endif
//}

void * OsCreateMutex( char *name) {
#ifdef _WIN32
    HANDLE mh;

    mh = CreateMutex( NULL, FALSE, name);

    return( ( void *) mh);
#else
    pthread_mutex_t * mtx = malloc( sizeof( pthread_mutex_t));

    if( mtx == NULL) {
        return( NULL);
    }

    pthread_mutex_init( mtx, NULL);

    return( mtx);
#endif
}

long OsLockMutex( void *mtx, long timeout) {
#ifdef _WIN32
    return( WaitForSingleObject( ( HANDLE) mtx, timeout) == WAIT_OBJECT_0 ? 0 : -1);
#else
    int status;

    pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, NULL);

    status = pthread_mutex_lock( ( pthread_mutex_t *) mtx);

    return( status == 0 ? 0 : -1);
#endif
}

long OsUnlockMutex( void * mtx) {
#ifdef _WIN32
    return( ReleaseMutex( ( HANDLE) mtx) ? 0 : -1);
#else
    int status;

    status = pthread_mutex_unlock( ( pthread_mutex_t *) mtx);

    pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL);

    return( status == 0 ? 0 : -1);
#endif
}

void OsCloseMutex( void * mtx) {
#ifdef _WIN32
    CloseHandle( ( HANDLE) mtx);
#else
    int status;

    status = pthread_mutex_destroy( ( pthread_mutex_t *) mtx);

    if ( status != 0) {
        return;
    }

    free( mtx);
#endif
}

long OsCreateThreadDataKey() {
#ifdef _WIN32
    long tdk = ( long) TlsAlloc();
    return( tdk == TLS_OUT_OF_INDEXES ? -1 : tdk);
#else
    pthread_key_t tdk;
    return( pthread_key_create( &tdk, NULL) ? -1 : ( long) tdk);
#endif
}

void OsSetThreadData( long key, const void *tdata) {
#ifdef _WIN32
    TlsSetValue( ( DWORD) key, ( void *) tdata);
#else
    pthread_setspecific( ( pthread_key_t) key, tdata);
#endif
}

void *OsGetThreadData( long key) {
#ifdef _WIN32
    return( TlsGetValue( ( DWORD) key));
#else
    return( pthread_getspecific( ( pthread_key_t) key));
#endif
}
void OsSleep( long ms) {
	thd_sleep(ms);
/*
#ifdef _WIN32
    Sleep( ms);
#else
    struct timespec req, rem;
    req.tv_sec = ms / 1000L;
    req.tv_nsec = ms % 1000L;
    req.tv_nsec *= 1000000L;
    nanosleep( &req, &rem);
#endif
*/
}

