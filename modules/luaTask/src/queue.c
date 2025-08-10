/*
** $Id: queue.c 10 2007-09-15 19:37:27Z danielq $
** Queue Management: Implementation
** SoongSoft, Argentina
** http://www.soongsoft.com mailto:dq@soongsoft.com
** Copyright (C) 2003-2006 Daniel Quintela.  All rights reserved.
*/

#ifdef _WIN32
#   include <windows.h>
    #ifndef NATV_WIN32
        #include <pthread.h>
    #endif
#else
#   include <unistd.h>
#   include <strings.h>
#   include <pthread.h>
#endif
#include <stdlib.h>

#include "syncos.h"
#include "queue.h"

#if !defined(_WIN32) && !defined(_arch_dreamcast)
#   define     QUEUE_PIPE_IN   0
#   define     QUEUE_PIPE_OUT  1
#endif


static int InsertMsgAtQueueTail( QUEUE *pQueue, void *pMsg) {
    QMSG    *pQMsg;

    pQMsg       = ( QMSG *) calloc( 1, sizeof( QMSG));
    pQMsg->pMsg = pMsg;

    if( pQueue->qMsgHead == NULL) {
        pQueue->qMsgHead = pQMsg;
        pQueue->qMsgTail = pQMsg;
    } else {
        pQueue->qMsgTail->next = pQMsg;
        pQueue->qMsgTail = pQMsg;
    }
    
    pQueue->msgcount++;
    
    return( 0);
}

static QMSG * RemoveMsgFromQueueHead( QUEUE *pQueue) {
    QMSG    *pQMsg;

    pQMsg = pQueue->qMsgHead;
    if( pQueue->qMsgHead == pQueue->qMsgTail) {
        pQueue->qMsgTail = NULL;
    }

    pQueue->qMsgHead = pQueue->qMsgHead->next;

    pQueue->msgcount--;
    
    return( pQMsg);
}

int QueDestroy( QUEUE *pQueue) {
    while( pQueue->qMsgHead != NULL) {
        QMSG    *pQMsg;
        pQMsg = RemoveMsgFromQueueHead( pQueue);
        free( pQMsg->pMsg);
        free( pQMsg);
    }
#ifdef _WIN32
    CloseHandle( pQueue->qNotEmpty);
    if( pQueue->qNotFull != NULL)
        CloseHandle( pQueue->qNotFull);
    CloseHandle( pQueue->qMutex);

#elif defined(_arch_dreamcast)
    sem_destroy( &pQueue->qNotEmpty);
    if( pQueue->qNotFull.initialized ) {
        sem_destroy( &pQueue->qNotFull);
    }
#else
    close( pQueue->qNotEmpty[QUEUE_PIPE_IN]);
    close( pQueue->qNotEmpty[QUEUE_PIPE_OUT]);
#endif
    return( 0);
}

int _QueGet( QUEUE *pQueue, void **ppMsg) {
    QMSG    *pQMsg;

    OsLockMutex( ( void *) pQueue->qMutex, INFINITE);

    pQMsg = RemoveMsgFromQueueHead( pQueue);

    OsUnlockMutex( ( void *) pQueue->qMutex);

    if( pQueue->qMax != QUE_NO_LIMIT) {
#ifdef _WIN32
        ReleaseSemaphore( pQueue->qNotFull, 1, NULL);
#elif defined(_arch_dreamcast)
		sem_signal(&pQueue->qNotFull);
#endif
    }
#if !defined(_WIN32) && !defined(_arch_dreamcast)
    {
        char    b;
        read( pQueue->qNotEmpty[QUEUE_PIPE_IN], &b, 1);
    }
#endif

    *ppMsg = pQMsg->pMsg;

    free( pQMsg);

    return( 0);
}

int QueGet( QUEUE *pQueue, void **ppMsg) {
#ifdef _WIN32
    WaitForSingleObject( pQueue->qNotEmpty, INFINITE);
#elif defined(_arch_dreamcast)
	sem_wait(&pQueue->qNotEmpty);
#endif

    return( _QueGet( pQueue, ppMsg));
}

int QuePut( QUEUE *pQueue, void *pMsg) {
    if( pQueue->qMax != QUE_NO_LIMIT) { /* bounded queue */
#ifdef _WIN32
        WaitForSingleObject( pQueue->qNotFull, INFINITE);
#elif defined(_arch_dreamcast)
		sem_wait(&pQueue->qNotFull);
#endif
    }

    OsLockMutex( ( void *) pQueue->qMutex, INFINITE);

    InsertMsgAtQueueTail( pQueue, pMsg);

    OsUnlockMutex( ( void *) pQueue->qMutex);

#ifdef _WIN32
    ReleaseSemaphore( pQueue->qNotEmpty, 1, NULL);
#elif defined(_arch_dreamcast)
	sem_signal(&pQueue->qNotEmpty);
#else
    {
        char    b;
        write( pQueue->qNotEmpty[QUEUE_PIPE_OUT], &b, 1);
    }
#endif

    return( 0);
}

int QueCreate( QUEUE *pQueue, int qLimit ) {
    pQueue->qMutex = OsCreateMutex( NULL);

#ifdef _WIN32
    pQueue->qNotEmpty =  CreateSemaphore( NULL, 0, 0x7fffffff, NULL);
#elif defined(_arch_dreamcast)
    	if (sem_init(&pQueue->qNotEmpty, 0) < 0) {
    		return (-1);
    	}
#else
    if( pipe(pQueue->qNotEmpty)) {
        return( -1);
    }
#endif
    if( qLimit != QUE_NO_LIMIT) {
#ifdef _WIN32
        pQueue->qNotFull = CreateSemaphore( NULL, qLimit, 0x7fffffff, NULL);
    } else
        pQueue->qNotFull = NULL;

#elif defined(_arch_dreamcast)
        if (sem_init(&pQueue->qNotFull, qLimit) < 0) {
            sem_destroy(&pQueue->qNotEmpty);
            return (-1);
        }
    } else {
        pQueue->qNotFull.initialized = 0;
    }
#else
    } else
        pQueue->qNotFull = 0;
#endif

    pQueue->qMax     = qLimit;
    pQueue->qMsgHead = NULL;
    pQueue->qMsgTail = NULL;
    pQueue->msgcount = 0;

    return( 0);
}

long GetQueNotEmptyHandle( QUEUE *pQueue) {
#if defined(_WIN32)
    return( ( long) ( pQueue->qNotEmpty));
#elif defined(_arch_dreamcast)
    return( ( long) ( &pQueue->qNotEmpty));
#else
    return( ( long) ( pQueue->qNotEmpty[QUEUE_PIPE_IN]));
#endif
}
