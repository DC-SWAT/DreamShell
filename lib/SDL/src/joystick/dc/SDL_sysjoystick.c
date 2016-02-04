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

    based on win32/SDL_mmjoystick.c

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_sysjoystick.c,v 1.2 2004/01/04 16:49:17 slouken Exp $";
#endif

#include <kos.h>

#include <stdio.h>		/* For the definition of NULL */
#include <stdlib.h>
#include <string.h>

#include "SDL_error.h"
#include "SDL_sysevents.h"
#include "SDL_events_c.h"
#include "SDL_joystick.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"

#include "SDL_dreamcast.h"

#include <dc/maple.h>
#include <dc/maple/controller.h>

#define MIN_FRAME_UPDATE 16
extern unsigned __sdl_dc_mouse_shift;

#define MAX_JOYSTICKS	8	/* only 2 are supported in the multimedia API */
#define MAX_AXES	6	/* each joystick can have up to 6 axes */
#define MAX_BUTTONS	8	/* and 8 buttons                      */
#define	MAX_HATS	2

#define	JOYNAMELEN	8

static SDLKey _dc_sdl_key[MAX_JOYSTICKS][13]= {
  { SDLK_ESCAPE, SDLK_LALT, SDLK_LCTRL, SDLK_RETURN, SDLK_3, SDLK_SPACE, SDLK_LSHIFT,
	  SDLK_TAB, SDLK_BACKSPACE, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT },
  { SDLK_ESCAPE, SDLK_q,    SDLK_e,     SDLK_z,      SDLK_3, SDLK_x,     SDLK_c,
	  SDLK_1,   SDLK_2,         SDLK_w,  SDLK_s,    SDLK_a,    SDLK_d },
  { SDLK_ESCAPE, SDLK_r,    SDLK_y,     SDLK_v,      SDLK_3, SDLK_b,     SDLK_n,
	  SDLK_4,   SDLK_5,         SDLK_t,  SDLK_g,    SDLK_f,    SDLK_h },
  { SDLK_ESCAPE, SDLK_u,    SDLK_o,     SDLK_m,      SDLK_3, SDLK_COMMA, SDLK_PERIOD,
	  SDLK_8,   SDLK_9,         SDLK_i,  SDLK_k,    SDLK_j,    SDLK_l }
};

void SDL_DC_MapKey(int joy, SDL_DC_button button, SDLKey key)
{
	if ((joy<MAX_JOYSTICKS)&&(joy>=0))
		if (((int)button<13)&&((int)button>=0))
			_dc_sdl_key[joy][(int)button]=key;
}


/* array to hold devices */
static maple_device_t * SYS_Joystick_addr[MAX_JOYSTICKS];

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
	int prev_buttons;
	int ltrig;
	int rtrig;
	int joyx;
	int joyy;
	int joy2x;
	int joy2y;
};

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */

int __sdl_dc_emulate_keyboard=1, __sdl_dc_emulate_mouse=1;

void SDL_DC_EmulateKeyboard(SDL_bool value)
{
	__sdl_dc_emulate_keyboard=(int)value;
}

void SDL_DC_EmulateMouse(SDL_bool value)
{
	__sdl_dc_emulate_mouse=(int)value;
}


int SDL_SYS_JoystickInit(void)
{
	int numdevs;
	int i;

	numdevs = 0;
	//Update KOS Maple
	int n = maple_enum_count();
	for (i=0;i<n;i++){
		maple_device_t *cont = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
		if (cont){
			SYS_Joystick_addr[numdevs] = cont;
			numdevs++;
		}
		else{
			maple_device_t *keyb = maple_enum_type(i, MAPLE_FUNC_KEYBOARD); 
			if (keyb) __sdl_dc_emulate_keyboard=0;
			maple_device_t *mouse = maple_enum_type(i, MAPLE_FUNC_MOUSE); 
			if (mouse) __sdl_dc_emulate_mouse=0;
		}
	}
	return(numdevs);
}


/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	maple_device_t *dev = SYS_Joystick_addr[index];
	if (dev && dev->info.functions & MAPLE_FUNC_CONTROLLER) {
		return dev->info.product_name;
	}
	else return NULL;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	/* allocate memory for system specific hardware data */
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	if (joystick->hwdata == NULL)
	{
		SDL_OutOfMemory();
		return(-1);
	}
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));

	/* fill nbuttons, naxes, and nhats fields */
	joystick->nbuttons = MAX_BUTTONS;
	joystick->naxes = MAX_AXES;
	joystick->nhats = MAX_HATS;
	return(0);
}


/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */

static void joyUpdate(SDL_Joystick *joystick)
{
	SDL_keysym keysym;
	int count_cond;
	static int count=1;
	static int mx=2048,my=2048;
	static int escaped=0;
	const	int sdl_buttons[] = {
		CONT_C,
		CONT_B,
		CONT_A,
		CONT_START,
		CONT_Z,
		CONT_Y,
		CONT_X,
		CONT_D
	};

	cont_state_t *cond;
	int buttons,prev_buttons,i,max,changed;
	int prev_ltrig,prev_rtrig,prev_joyx,prev_joyy,prev_joy2x,prev_joy2y;
	
	//Get buttons of the Controller
	maple_device_t * dev = SYS_Joystick_addr[joystick->index];
	cond = (cont_state_t *)maple_dev_status(dev);
	
	if (!cond) return;

	//Check changes on cont_state_t->buttons
	buttons = cond->buttons;
	prev_buttons = joystick->hwdata->prev_buttons;
	changed = buttons^prev_buttons;

	//Check Directions for HAT
	if ((changed)&(CONT_DPAD_UP|CONT_DPAD_DOWN|CONT_DPAD_LEFT|CONT_DPAD_RIGHT)) {
		int hat = SDL_HAT_CENTERED;
		if (!(buttons&CONT_DPAD_UP)) hat|=SDL_HAT_UP;
		if (!(buttons&CONT_DPAD_DOWN)) hat|=SDL_HAT_DOWN;
		if (!(buttons&CONT_DPAD_LEFT)) hat|=SDL_HAT_LEFT;
		if (!(buttons&CONT_DPAD_RIGHT)) hat|=SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 0, hat);
	}
	if ((changed)&(CONT_DPAD2_UP|CONT_DPAD2_DOWN|CONT_DPAD2_LEFT|CONT_DPAD2_RIGHT)) {
		int hat = SDL_HAT_CENTERED;
		if (!(buttons&CONT_DPAD2_UP)) hat|=SDL_HAT_UP;
		if (!(buttons&CONT_DPAD2_DOWN)) hat|=SDL_HAT_DOWN;
		if (!(buttons&CONT_DPAD2_LEFT)) hat|=SDL_HAT_LEFT;
		if (!(buttons&CONT_DPAD2_RIGHT)) hat|=SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 1, hat);
	}
	
	//Check buttons
	//"buttons" is zero based: so invert the PRESSED/RELEASED way.
	for(i=0,max=0;i<sizeof(sdl_buttons)/sizeof(sdl_buttons[0]);i++) {
		if (changed & sdl_buttons[i]) {
			int act=(buttons & sdl_buttons[i]);
			SDL_PrivateJoystickButton(joystick, i, act?SDL_PRESSED:SDL_RELEASED);
			if (__sdl_dc_emulate_mouse)
				SDL_PrivateMouseButton(act?SDL_PRESSED:SDL_RELEASED,i,0,0);
			if (__sdl_dc_emulate_keyboard)
			{
				if (act)
					max++;
				if (max==4)
				{
					keysym.sym = SDLK_ESCAPE;
					SDL_PrivateKeyboard(SDL_PRESSED,&keysym);
					max=0;
					escaped=1;
				}
				keysym.sym = _dc_sdl_key[joystick->index][i];
				SDL_PrivateKeyboard(act?SDL_PRESSED:SDL_RELEASED,&keysym);
			}
		}
	}
	if (escaped)
	{
		if (escaped==2)
		{
			keysym.sym = SDLK_ESCAPE;
			SDL_PrivateKeyboard(SDL_RELEASED,&keysym);
			escaped=0;
		}
		else
			escaped=2;
	}

	//If we emulate keyboard, check for SDL_DC DPAD
	//Also here invert the states!
	if (__sdl_dc_emulate_keyboard)
	{
		if (changed & CONT_DPAD_UP)
		{
			keysym.sym=_dc_sdl_key[joystick->index][9];
			SDL_PrivateKeyboard((buttons & CONT_DPAD_UP)?SDL_PRESSED:SDL_RELEASED,&keysym);
		}
		if (changed & CONT_DPAD_DOWN)
		{
			keysym.sym=_dc_sdl_key[joystick->index][10];
			SDL_PrivateKeyboard((buttons & CONT_DPAD_DOWN)?SDL_PRESSED:SDL_RELEASED,&keysym);
		}
		if (changed & CONT_DPAD_LEFT)
		{
			keysym.sym=_dc_sdl_key[joystick->index][11];
			SDL_PrivateKeyboard((buttons & CONT_DPAD_LEFT)?SDL_PRESSED:SDL_RELEASED,&keysym);
		}
		if (changed & CONT_DPAD_RIGHT)
		{
			keysym.sym=_dc_sdl_key[joystick->index][12];
			SDL_PrivateKeyboard((buttons & CONT_DPAD_RIGHT)?SDL_PRESSED:SDL_RELEASED,&keysym);
		}
	}

	//Emulating mouse
	//"joyx", "joyy", "joy2x", and "joy2y" are all zero based
	if ((__sdl_dc_emulate_mouse) && (!joystick->index))
	{
		count_cond=!(count&0x1);
		if (cond->joyx!=0 || cond->joyy!=0 || count_cond)
		{
			{
				register unsigned s=__sdl_dc_mouse_shift+1;
				mx=(cond->joyx)>>s;
				my=(cond->joyy)>>s;
			}
			if (count_cond)
				SDL_PrivateMouseMotion(changed>>1,1,(Sint16)(mx),(Sint16)(my));
			count++;
		}
	}

	//Check changes on cont_state_t: triggers and Axis
	prev_ltrig = joystick->hwdata->ltrig;
	prev_rtrig = joystick->hwdata->rtrig;
	prev_joyx = joystick->hwdata->joyx;
	prev_joyy = joystick->hwdata->joyy;
	prev_joy2x = joystick->hwdata->joy2x;
	prev_joy2y = joystick->hwdata->joy2y;

	//Check Joystick Axis P1
	//"joyx", "joyy", "joy2x", and "joy2y" are all zero based
	if (cond->joyx!=prev_joyx)
		SDL_PrivateJoystickAxis(joystick, 0, cond->joyx);
	if (cond->joyy!=prev_joyy)
		SDL_PrivateJoystickAxis(joystick, 1, cond->joyy);
	
	//Check L and R triggers
	//In this case, do not flip the PRESSED/RELEASED!
	if (cond->rtrig!=prev_rtrig)
	{
		if (__sdl_dc_emulate_keyboard)
			if (((prev_rtrig) && (!cond->rtrig)) ||
		    	    ((!prev_rtrig) && (cond->rtrig)))
			{
				keysym.sym=_dc_sdl_key[joystick->index][7];
				SDL_PrivateKeyboard((cond->rtrig)?SDL_PRESSED:SDL_RELEASED,&keysym);
			}
		SDL_PrivateJoystickAxis(joystick, 2, cond->rtrig);
	}
	if (cond->ltrig!=prev_ltrig)
	{
		if (__sdl_dc_emulate_keyboard)
			if (((prev_ltrig) && (!cond->ltrig)) ||
		    	    ((!prev_ltrig) && (cond->ltrig)))
			{
				keysym.sym=_dc_sdl_key[joystick->index][8];
				SDL_PrivateKeyboard((cond->ltrig)?SDL_PRESSED:SDL_RELEASED,&keysym);
			}
		SDL_PrivateJoystickAxis(joystick, 3, cond->ltrig);
	}
	//Check Joystick Axis P2
	if (cond->joy2x!=prev_joy2x)
		SDL_PrivateJoystickAxis(joystick, 4, cond->joy2x);
	if (cond->joy2y!=prev_joy2y)
		SDL_PrivateJoystickAxis(joystick, 5, cond->joy2y);

	//Update previous state
	joystick->hwdata->prev_buttons = buttons;
	
	joystick->hwdata->ltrig = cond->ltrig;
	joystick->hwdata->rtrig = cond->rtrig;
	joystick->hwdata->joyx = cond->joyx;
	joystick->hwdata->joyy = cond->joyy;
	joystick->hwdata->joy2x = cond->joy2x;
	joystick->hwdata->joy2y = cond->joy2y;

}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		free(joystick->hwdata);
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	return;
}

static __inline__ Uint32 myGetTicks(void)
{
	return ((timer_us_gettime64()>>10));
}

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	static Uint32 last_time[MAX_JOYSTICKS]={0,0,0,0,0,0,0,0};
	Uint32 now=myGetTicks();
	if (now-last_time[joystick->index]>MIN_FRAME_UPDATE)
	{
		last_time[joystick->index]=now;
		joyUpdate(joystick);
	}
}

