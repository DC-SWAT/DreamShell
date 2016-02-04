/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_audiomem.c,v 1.5 2004/01/04 16:49:11 slouken Exp $";
#endif

/* Functions to allocate audio buffer memory, shareable across threads
	(necessary because SDL audio emulates threads with fork()
 */

#include <stdlib.h>
#ifdef FORK_HACK
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stddef.h>
#endif

#include "SDL_audiomem.h"

/* Allocate memory that will be shared between threads (freed on exit) */
void *SDL_AllocAudioMem(int size)
{
	void *chunk;

#ifdef FORK_HACK
	int   semid;
	
	/* Create and get the address of a shared memory segment */
	semid = shmget(IPC_PRIVATE, size, (IPC_CREAT|0600));
	if ( semid < 0 ) {
		return(NULL);
	}
	chunk = shmat(semid, NULL, 0);
	
	/* Set the segment for deletion when it is detatched */
	shmctl(semid, IPC_RMID, NULL);	/* Delets semid if shmat() fails */
#else
	chunk = malloc(size);
#endif
	return((void *)chunk);
}

void SDL_FreeAudioMem(void *chunk)
{
#ifdef FORK_HACK
	shmdt(chunk);
#else
	free(chunk);
#endif
}
