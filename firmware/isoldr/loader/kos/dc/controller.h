/**
 * DreamShell ISO Loader
 * Controller
 * (c)2023 SWAT <http://www.dc-swat.ru>
 */

#include <sys/cdefs.h>
#include <arch/types.h>

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

/** \defgroup controller_input_masks Inputs
    \brief    Collection of all status masks for checking input
    \ingroup  controller_inputs
  
    A set of bitmasks representing each input source on a controller, used to
    check its status.

    @{
*/
#define CONT_C              (1<<0)      /**< \brief C button Mask. */
#define CONT_B              (1<<1)      /**< \brief B button Mask. */
#define CONT_A              (1<<2)      /**< \brief A button Mask. */
#define CONT_START          (1<<3)      /**< \brief Start button Mask. */
#define CONT_DPAD_UP        (1<<4)      /**< \brief Main Dpad Up button Mask. */
#define CONT_DPAD_DOWN      (1<<5)      /**< \brief Main Dpad Down button Mask. */
#define CONT_DPAD_LEFT      (1<<6)      /**< \brief Main Dpad Left button Mask. */
#define CONT_DPAD_RIGHT     (1<<7)      /**< \brief Main Dpad right button Mask. */
#define CONT_Z              (1<<8)      /**< \brief Z button Mask. */
#define CONT_Y              (1<<9)      /**< \brief Y button Mask. */
#define CONT_X              (1<<10)     /**< \brief X button Mask. */
#define CONT_D              (1<<11)     /**< \brief D button Mask. */
#define CONT_DPAD2_UP       (1<<12)     /**< \brief Secondary Dpad Up button Mask. */
#define CONT_DPAD2_DOWN     (1<<13)     /**< \brief Secondary Dpad Down button Mask. */
#define CONT_DPAD2_LEFT     (1<<14)     /**< \brief Secondary Dpad Left button Mask. */
#define CONT_DPAD2_RIGHT    (1<<15)     /**< \brief Secondary Dpad Right button Mask. */
/** @} */

/* controller condition structure */
typedef struct {
	uint16 buttons;	/* buttons bitfield */
	uint8 rtrig;    /* right trigger */
	uint8 ltrig;    /* left trigger */
	uint8 joyx;     /* joystick X */
	uint8 joyy;     /* joystick Y */
	uint8 joy2x;    /* second joystick X */
	uint8 joy2y;    /* second joystick Y */
} cont_cond_t;

#endif
