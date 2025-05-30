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

#define MAX_JOYSTICKS	4	/* only 2 are supported in the multimedia API */
#define MAX_AXES	6	/* each joystick can have up to 6 axes */
#define MAX_BUTTONS	8	/* and 8 buttons                      */
#define	MAX_HATS	2

#define	JOYNAMELEN	8

/* array to hold devices */
//static maple_device_t * SYS_Joystick_addr[MAX_JOYSTICKS];

/* The private structure used to keep track of a joystick */
struct joystick_hwdata {
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

int __sdl_dc_emulate_mouse = 1;

void SDL_DC_EmulateMouse(SDL_bool value) {
	__sdl_dc_emulate_mouse = (int)value;
}

int SDL_SYS_JoystickInit(void) {
	return MAX_JOYSTICKS;
}


/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index) {
	maple_device_t *dev = maple_enum_type(index, MAPLE_FUNC_CONTROLLER);
	
	if (dev && dev->info.functions & MAPLE_FUNC_CONTROLLER) {
		return dev->info.product_name;
	}
	
	return NULL;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick) {
	/* allocate memory for system specific hardware data */
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	
	if (joystick->hwdata == NULL) {
		SDL_OutOfMemory();
		return -1;
	}
	
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));

	/* fill nbuttons, naxes, and nhats fields */
	joystick->nbuttons = MAX_BUTTONS;
	joystick->naxes = MAX_AXES;
	joystick->nhats = MAX_HATS;
	
	return 0;
}


/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
#define CONT_LTRIG	(1 << 16)
#define CONT_RTRIG	(1 << 17)

static void joyUpdate(SDL_Joystick *joystick) {
	SDL_keysym keysym;
	int count_cond;
	static int count=1;
	static int mx = 2048, my = 2048;
	const	int sdl_buttons[] = {
		CONT_C,
		CONT_B,
		CONT_A,
		CONT_START,
		CONT_Z,
		CONT_Y,
		CONT_X,
		CONT_D,
		CONT_LTRIG,
		CONT_RTRIG
	};

	cont_state_t *cond;
	maple_device_t * dev;
	int buttons,prev_buttons,i,max,changed;
	int prev_ltrig,prev_rtrig,prev_joyx,prev_joyy,prev_joy2x,prev_joy2y;
	
	//Get buttons of the Controller
	if(!(dev = maple_enum_type(joystick->index, MAPLE_FUNC_CONTROLLER))) {
		return;
	}
	
	
	if (!(cond = (cont_state_t *)maple_dev_status(dev))) {
		return;
	}

	//Check changes on cont_state_t->buttons
	buttons = cond->buttons;
	
	if (cond->ltrig > 192) {
		buttons |= CONT_LTRIG;
	}
	
	if (cond->rtrig > 192) {
		buttons |= CONT_RTRIG;
	}
	
	prev_buttons = joystick->hwdata->prev_buttons;
	changed = buttons ^ prev_buttons;

	//Check Directions for HAT
	if (changed & (CONT_DPAD_UP | CONT_DPAD_DOWN | CONT_DPAD_LEFT | CONT_DPAD_RIGHT)) {
		int hat = SDL_HAT_CENTERED;
		if (buttons & CONT_DPAD_UP   ) hat |= SDL_HAT_UP;
		if (buttons & CONT_DPAD_DOWN ) hat |= SDL_HAT_DOWN;
		if (buttons & CONT_DPAD_LEFT ) hat |= SDL_HAT_LEFT;
		if (buttons & CONT_DPAD_RIGHT) hat |= SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 0, hat);
	}
	
	if (changed & (CONT_DPAD2_UP | CONT_DPAD2_DOWN | CONT_DPAD2_LEFT | CONT_DPAD2_RIGHT)) {
		int hat = SDL_HAT_CENTERED;
		if (buttons & CONT_DPAD2_UP   ) hat |= SDL_HAT_UP;
		if (buttons & CONT_DPAD2_DOWN ) hat |= SDL_HAT_DOWN;
		if (buttons & CONT_DPAD2_LEFT ) hat |= SDL_HAT_LEFT;
		if (buttons & CONT_DPAD2_RIGHT) hat |= SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 1, hat);
	}
	
	//Check buttons
	//"buttons" is zero based: so invert the PRESSED/RELEASED way.
	for(i=0, max=0; i < sizeof(sdl_buttons) / sizeof(sdl_buttons[0]); i++) {
		if (changed & sdl_buttons[i]) {
			int act = (buttons & sdl_buttons[i]);
			SDL_PrivateJoystickButton(joystick, i, act ? SDL_PRESSED : SDL_RELEASED);
			
			if (__sdl_dc_emulate_mouse) {
				if (sdl_buttons[i] == CONT_A || sdl_buttons[i] == CONT_B) {
					SDL_PrivateMouseButton(act ? SDL_PRESSED : SDL_RELEASED, 
						(sdl_buttons[i] == CONT_A) ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT, 0, 0);
				}
			}
		}
	}

	//Emulating mouse
	//"joyx", "joyy", "joy2x", and "joy2y" are all zero based
	if ((__sdl_dc_emulate_mouse) && (!joystick->index)) {
		count_cond = !(count & 0x1);
		if (cond->joyx!=0 || cond->joyy!=0 || count_cond) {
			{
				register unsigned s= __sdl_dc_mouse_shift+1;
				mx = (cond->joyx) >> s;
				my = (cond->joyy) >> s;
			}
			
			if (count_cond) {
				SDL_PrivateMouseMotion(changed >> 1, 1, (Sint16)(mx), (Sint16)(my), 0);
			}
			
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
	if (cond->joyx != prev_joyx) {
		SDL_PrivateJoystickAxis(joystick, 0, cond->joyx);
	}
	
	if (cond->joyy != prev_joyy) {
		SDL_PrivateJoystickAxis(joystick, 1, cond->joyy);
	}
	
	//Check L and R triggers
	//In this case, do not flip the PRESSED/RELEASED!
	if (cond->rtrig != prev_rtrig) {
		SDL_PrivateJoystickAxis(joystick, 2, cond->rtrig);
	}
	
	if (cond->ltrig != prev_ltrig) {
		SDL_PrivateJoystickAxis(joystick, 3, cond->ltrig);
	}
	//Check Joystick Axis P2
	if (cond->joy2x != prev_joy2x) {
		SDL_PrivateJoystickAxis(joystick, 4, cond->joy2x);
	}
	
	if (cond->joy2y != prev_joy2y) {
		SDL_PrivateJoystickAxis(joystick, 5, cond->joy2y);
	}

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
void SDL_SYS_JoystickClose(SDL_Joystick *joystick) {
	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		free(joystick->hwdata);
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void) {
	return;
}

static __inline__ Uint32 myGetTicks(void)
{
	return ((timer_us_gettime64()>>10));
}

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick) {
	static Uint32 last_time[MAX_JOYSTICKS] = {0,0,0,0};
	Uint32 now = myGetTicks();
	
	if (now-last_time[joystick->index] > MIN_FRAME_UPDATE) {
		last_time[joystick->index] = now;
		joyUpdate(joystick);
	}
}

