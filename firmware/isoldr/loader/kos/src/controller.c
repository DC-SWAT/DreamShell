/* This file is part of the libdream Dreamcast function library.
   Please see libdream.c for further details.
  
   controller.c
   (C)2000 Jordan DeLong
*/

#include <dc/maple.h>
#include <dc/controller.h>

/*

Ported from KallistiOS (Dreamcast OS) for libdream by Dan Potter

*/


/* get the complete condition structure for controller on port
   and fill in cond with it, ret -1 on error */
int cont_get_cond(uint8 addr, cont_cond_t *cond) {
	maple_frame_t frame;
	uint32 param[1];

	param[0] = MAPLE_FUNC_CONTROLLER;

	do {
		if (maple_docmd_block(MAPLE_COMMAND_GETCOND, addr, 1, param, &frame) == -1)
			return -1;
	} while (frame.cmd == MAPLE_RESPONSE_AGAIN);

	/* response comes as func, data. */
	if (frame.cmd == MAPLE_RESPONSE_DATATRF
		&& (frame.datalen - 1) == sizeof(cont_cond_t) / 4
		&& *((uint32 *) frame.data) == MAPLE_FUNC_CONTROLLER) {
		memcpy(cond, frame.data + 4, (frame.datalen - 1) * 4);
	} else {
		return -1;
	}

	return 0;
}
