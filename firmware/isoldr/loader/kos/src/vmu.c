/* This file is part of the libdream Dreamcast function library.
   Please see libdream.c for further details.
   
   vmu.c
  (C)2000 Jordan DeLong
*/

#include <string.h>
#include <dc/maple.h>
#include <dc/vmu.h>

/*
  This module deals with the VMU.  It provides functionality for
  memorycard access, and for access to the lcd screen.
 
  Thanks to Marcus Comstedt for VMU/Maple information.
  
  Ported from KallistiOS (Dreamcast OS) for libdream by Dan Potter
*/

/* draw a 1-bit bitmap on the LCD screen (48x32). return a -1 if
   an error occurs */
int vmu_draw_lcd(uint8 addr, void *bitmap) {
	uint32 param[2 + 48];
	maple_frame_t frame;

	param[0] = MAPLE_FUNC_LCD;
	/* this is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
	param[1] = 0;
	memcpy(&param[2], bitmap, 48 * 4);

	do {
		if (maple_docmd_block(MAPLE_COMMAND_BWRITE, addr, 2 + 48, param, &frame) == -1)
			return -1;
	} while (frame.cmd == MAPLE_RESPONSE_AGAIN);

	if (frame.cmd != MAPLE_RESPONSE_OK)
		return -1;

	return 0;
}

/* read the data in block blocknum into buffer, return a -1
   if an error occurs, for now we ignore MAPLE_RESPONSE_FILEERR,
   which will be changed shortly */
int vmu_block_read(uint8 addr, uint16 blocknum, uint8 *buffer) {
	uint32 param[2];
	maple_frame_t frame;

	param[0] = MAPLE_FUNC_MEMCARD;
	/* this is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
	param[1] = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16);

	do {
		if (maple_docmd_block(MAPLE_COMMAND_BREAD, addr, 2, param, &frame) == -1)
			return -1;
	} while (frame.cmd == MAPLE_RESPONSE_AGAIN);

	if (frame.cmd == MAPLE_RESPONSE_DATATRF
		&& *((uint32 *) frame.data) == MAPLE_FUNC_MEMCARD 
		&& *(((uint32 *) frame.data) + 1) == param[1]) {

		memcpy(buffer, ((uint32 *) frame.data) + 2, (frame.datalen-2) * 4);
	} else {
		printf("vmu_block_read failed: %d/%d\r\n",
			frame.cmd, *((uint32 *)frame.data));
			
		return -1;
	}

	return 0;
}

/* writes buffer into block blocknum.  ret a -1 on error.  We don't do anything about the
   maple bus returning file errors, etc, right now, but that will change soon. */
int vmu_block_write(uint8 addr, uint16 blocknum, uint8 *buffer) {
	uint32 param[2 + (128 / 4)];
	maple_frame_t frame;
	int to, phase;

	/* Writes have to occur in four phases per block -- this is the
	   way of flash memory, which you must erase an entire block 
	   at once to write; the blocks in this case are 128 bytes long. */
	for (phase=0; phase<4; phase++) {
		/* Memory card function */
		param[0] = MAPLE_FUNC_MEMCARD;
		/* this is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
		param[1] = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16) | (phase << 8);
		/* data to write to the block */
		memcpy(param + 2, buffer + 128*phase, 128);

		to = 1000;
		do {
			if (maple_docmd_block(MAPLE_COMMAND_BWRITE, addr, 2 + (128 / 4), param, &frame) == -1)
				return -1;
			to--;
			if (to < 0) {
				printf("Timed out executing BWRITE\r\n");
				return -2;
			}
		} while (frame.cmd == MAPLE_RESPONSE_AGAIN);

		if (frame.cmd != MAPLE_RESPONSE_OK) {
			printf("vmu_block_write failed: %d/%d\r\n",
				frame.cmd, *((uint32 *)frame.data));
			return -1;
		}
		
		/* Gotta wait a bit to let the flash catch up */
		sleep(20);
	}

	return 0;
}
