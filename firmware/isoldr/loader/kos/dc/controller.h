/* This file is part of the libdream Dreamcast function library.
 * Please see libdream.c for further details.
 *
 * (c)2000 Jordan DeLong
 
   Thanks to Marcus Comstedt for information on the controller.
   
   Ported from KallistiOS (Dreamcast OS) for libdream by Dan Potter

 */


#include <sys/cdefs.h>
#include <arch/types.h>


#ifndef __CONTROLLER_H
#define __CONTROLLER_H

/* Buttons bitfield defines */
#define CONT_C  		   (1<<0)
#define CONT_B  		   (1<<1)
#define CONT_A  		   (1<<2)
#define CONT_START		   (1<<3)
#define CONT_DPAD_UP		   (1<<4)
#define CONT_DPAD_DOWN  	   (1<<5)
#define CONT_DPAD_LEFT  	   (1<<6)
#define CONT_DPAD_RIGHT 	   (1<<7)
#define CONT_Z  		   (1<<8)
#define CONT_Y  		   (1<<9)
#define CONT_X  		   (1<<10)
#define CONT_D  		   (1<<11)
#define CONT_DPAD2_UP		   (1<<12)
#define CONT_DPAD2_DOWN 	   (1<<13)
#define CONT_DPAD2_LEFT 	   (1<<14)
#define CONT_DPAD2_RIGHT	   (1<<15)

/* controller condition structure */
typedef struct {
	uint16 buttons;			/* buttons bitfield */
	uint8 rtrig;			/* right trigger */
	uint8 ltrig;			/* left trigger */
	uint8 joyx;			/* joystick X */
	uint8 joyy;			/* joystick Y */
	uint8 joy2x;			/* second joystick X */
	uint8 joy2y;			/* second joystick Y */
} cont_cond_t;

int cont_get_cond(uint8 addr, cont_cond_t *cond);

#endif
