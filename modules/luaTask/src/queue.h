/*
** $Id: queue.h 10 2007-09-15 19:37:27Z danielq $
** Queue Management: Declarations
** SoongSoft, Argentina
** http://www.soongsoft.com mailto:dq@soongsoft.com
** Copyright (C) 2003-2006 Daniel Quintela.  All rights reserved.
*/

#ifndef QUE_H_INCLUDED
#define QUE_H_INCLUDED

#if defined(_arch_dreamcast)
#   include <kos/sem.h>
#endif

#define     QUE_NO_LIMIT    -1

typedef struct  _qmsg   QMSG;

struct _qmsg
{
    void    *pMsg;
    QMSG    *next;
};

typedef struct _queue
{
     int        qMax;
     void *     qMutex;
#ifdef _WIN32
     void *     qNotEmpty;
     void *     qNotFull;
#elif defined(_arch_dreamcast)
     semaphore_t *     qNotEmpty;
     semaphore_t *     qNotFull;
#else
     int        qNotEmpty[2];
     int        qNotFull;
#endif
     QMSG       *qMsgHead;
     QMSG       *qMsgTail;
     long        msgcount;
} QUEUE;

int     _QueGet(QUEUE *pQueue, void **ppMsg);
int     QueGet(QUEUE *pQueue, void **ppMsg);
int     QuePut(QUEUE *pQueue, void *pMsg);
int     QueCreate(QUEUE *pQueue, int maxMsgs);
int     QueDestroy(QUEUE *pQueue);
long    GetQueNotEmptyHandle( QUEUE *pQueue);

#endif
