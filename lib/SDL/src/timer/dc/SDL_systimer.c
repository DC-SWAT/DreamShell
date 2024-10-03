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

    BERO
    bero@geocities.co.jp

    based on win32/SDL_systimer.c

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_systimer.c,v 1.2 2004/01/04 16:49:19 slouken Exp $";
#endif

#include <kos.h>

#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_error.h"
#include "SDL_timer_c.h"
#include "SDL_systhread.h"

static uint64 start;

/* 
	jif =  ms * HZ /1000
	ms  = jif * 1000/HZ
*/

void SDL_StartTicks(void)
{
	/* Set first ticks value */
	start = timer_ms_gettime64();
}

Uint32 SDL_GetTicks(void)
{
//	uint32 s, ms;
//	uint64 msec;

//	timer_ms_gettime(&s, &ms);
//	msec = (((uint64)s) * ((uint64)1000)) + ((uint64)ms);

	return (Uint32)timer_ms_gettime64();

//	return (Uint32)((((unsigned long long)timer_us_gettime64())>>10));
//	return (Uint32)(((unsigned long long)timer_us_gettime64())/1000LL);
/*
//	return ((timer_us_gettime64()>>10));
	unsigned long long l=timer_us_gettime64();
	l/=1000;
	return (unsigned)l;
//	return (timer_ms_gettime64());
//	return((jiffies-start)*1000/HZ);
*/
}

void SDL_Delay(Uint32 ms)
{
//	timer_spin_sleep(ms);
	thd_sleep(ms);
}

/* Data to handle a single periodic alarm */
int ___sdl_dc_timer_alive = 0;
int ___sdl_dc_timer_no_alive = 1;
static SDL_Thread *timer = NULL;

static int RunTimer(void *unused)
{
	___sdl_dc_timer_no_alive = 0;
	while ( ___sdl_dc_timer_alive ) {
		if ( SDL_timer_running ) {
			SDL_ThreadedTimerCheck();
		}
		SDL_Delay(10);
	}
	___sdl_dc_timer_no_alive = 1;
	return(0);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	___sdl_dc_timer_alive = 1;
	timer = SDL_CreateThread(RunTimer, NULL);
	if ( timer == NULL )
		return(-1);
	irq_enable();
	return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
	___sdl_dc_timer_alive = 0;
	if ( timer ) {
		Uint32 w=timer_ms_gettime64();
		while(!___sdl_dc_timer_no_alive)
			if (timer_ms_gettime64()-w > 1000)
			{
				SDL_SYS_KillThread(timer);
				break;
			}
//		SDL_WaitThread(timer, NULL);
		timer = NULL;
	}
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: DC uses threaded timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}
