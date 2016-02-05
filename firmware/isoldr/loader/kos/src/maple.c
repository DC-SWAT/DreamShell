/* This file is part of the Dreamcast function library.
 * Please see libdream.c for further details.
 *
 * (c)2000 Jordan DeLong
 */

#include <string.h>
#include <dc/maple.h>

/*
 * This module handles low-level communication/initialization of the maple 
 * bus.  Specific devices aren't handled by this module, rather, the modules
 * implementing specific devices can use this module to access them.
 * 
 * Thanks to Marcus Comstedt for information on the maple bus.
 *
 * Ported from KallistiOS for libdream by Dan Potter
 */

/* macro for poking maple mem */
#define MAPLE(x) (*(unsigned long *)((0xa05f6c00)+(x)))

/* dma transfer buffer */
uint32 *dmabuffer, dmabuffer_real[1024+1024+4+4+32];

/* a few small internal functions to make code slightly more readable */
void maple_enable_bus() { MAPLE(0x14) = 1; }
void maple_disable_bus() { MAPLE(0x14) = 0; }
void maple_start_dma() { MAPLE(0x18) = 1; }
int maple_dma_in_progress() { return MAPLE(0x18) & 1; }
/* set timeout to timeout, and bitrate to 2Mbps (what it must be) */
void maple_set_timeout(int timeout) { MAPLE(0x80) = (timeout << 16) | 0; }

/* set the DMA ptr */
void maple_set_dma_ptr(unsigned long *ptr) { 
	MAPLE(0x04) = ((uint32) ptr) & 0xfffffff;
}

/* initialize the maple bus. */
void maple_init(int quiet) {
	/* reset hardware */
	MAPLE(0x8c) = 0x6155404f;
	MAPLE(0x10) = 0;

	maple_set_timeout(50000);

	/* buffer for dma transfers; room for a send and recv buffer */
	dmabuffer = (uint32*)( (((uint32)dmabuffer_real) + 31) & ~31 );

	maple_enable_bus();
	
	maple_rescan_bus(quiet);
}

/* turn off the maple bus, free mem */
void maple_shutdown() {
	maple_disable_bus();
}

/* use the information in tdesc to place a new transfer descriptor, followed by
   a num of frames onto buffer, and return the new ptr into buff */
uint32 *maple_add_trans(maple_tdesc_t tdesc, uint32 *buffer) {
	int i;
	
	/* build the transfer descriptor's first word */
	*buffer++ = tdesc.length | (tdesc.port << 16) | (tdesc.lastdesc << 31);
	/* transfer descriptor second word: address to receive buffer */
	*buffer++ = ((uint32) tdesc.recvaddr) & 0xfffffff;
	
	/* add each of the frames in this transfer */
	for (i = 0; i < tdesc.numframes; i++) {
		/* frame header */
		*buffer++ = (tdesc.frames[i].cmd & 0xff) | (tdesc.frames[i].to << 8)
			| (tdesc.frames[i].from << 16) | (tdesc.frames[i].datalen << 24);
		/* parameter data, if any exists */
		if (tdesc.frames[i].datalen > 0) {
			memcpy(buffer, tdesc.frames[i].data, tdesc.frames[i].datalen * 4);
			buffer += tdesc.frames[i].datalen;
		}
	}
	
	return buffer;
}

/* read information at buffer and turn it into a maple_frame_t */
void maple_read_frame(uint32 *buffer, maple_frame_t *frame) {
	uint8 *b = (uint8 *) buffer;

	frame->cmd = b[0];
	frame->to = b[1];
	frame->from = b[2];
	frame->datalen = b[3];
	frame->data = &b[4];
}

/* create a maple address, -1 on error */
uint8 maple_create_addr(uint8 port, uint8 unit) {
	uint8 addr;
	
	if (port > 3 || unit > 5) return -1;
	
	addr = port << 6;
	if (unit != 0)
		addr |= (1 << (unit - 1)) & 0x1f;
	else
		addr |= 0x20;
		
	return addr;
}

/* initiate a dma transfer and block until it's completed,
   return -1 if error (currently only if DMA is already in
   progress) */
int maple_dodma_block() {
	if (maple_dma_in_progress())
		return -1;

	/* enable maple dma transfer bit */
	maple_start_dma();
	
	/* wait for it to clear (indicating the end of the transfer */
	while (maple_dma_in_progress())
		;
	
	return 0;
}

/* little funct to send a single command on the maple bus, and block until 
   recving a response, returns -1 on error */
int maple_docmd_block(int8 cmd, uint8 addr, uint8 datalen, void *data, maple_frame_t *retframe) {
	uint32 *sendbuff, *recvbuff;
	maple_tdesc_t tdesc;
	maple_frame_t frame;

	/* setup buffer ptrs, into dmabuffer and set uncacheable */
	sendbuff = (uint32 *) dmabuffer;
	recvbuff = (uint32 *) (dmabuffer + 1024);
	sendbuff = (uint32 *) ((uint32) sendbuff | 0xa0000000);
	recvbuff = (uint32 *) ((uint32) recvbuff | 0xa0000000);

	/* setup tdesc/frame */
	tdesc.lastdesc = 1;
	tdesc.port = addr >> 6;
	tdesc.length = datalen;
	tdesc.recvaddr = recvbuff;
	tdesc.numframes = 1;
	tdesc.frames = &frame;
	frame.cmd = cmd;
	frame.data = data;
	frame.datalen = datalen;
	frame.from = tdesc.port << 6;
	frame.to = addr;

	/* make sure no DMA is in progress */
	if (maple_dma_in_progress()) {
		printf("docmd_block: dma already in progress\r\n");
		return -1;
	}
		
	maple_set_dma_ptr(sendbuff);

	maple_add_trans(tdesc, sendbuff);
					  
	/* do the dma, blocking till it finishes */	
	if (maple_dodma_block() == -1) {
		printf("docmd_block: dodma_block failed\r\n");
		return -1;
	}
	
	maple_read_frame(recvbuff, retframe);
	return 0;
}

/* Rescan the maple bus to recognize VMUs, etc */
static int func_codes[4][6] = { {0} };
int maple_rescan_bus(int quiet) {
	int port, unit, to;
	maple_frame_t frame;

	if (!quiet)
		printf("Rescanning maple bus:\r\n");	
	for (port=0; port<4; port++)
		for (unit=0; unit<6; unit++) {
			to = 1000;
			do {
				if (maple_docmd_block(MAPLE_COMMAND_DEVINFO,
						maple_create_addr(port, unit), 0, NULL, &frame)
						== -1)
					return -1;
				to--;
				if (to <= 0) {
					printf("  %c%c: timeout\r\n", 'a'+port, '0'+unit);
					break;
				}
			} while (frame.cmd == MAPLE_RESPONSE_AGAIN);
			if (frame.cmd == MAPLE_RESPONSE_DEVINFO) {
				maple_devinfo_t *di = (maple_devinfo_t*)frame.data;
				di->product_name[29] = '\0';
				func_codes[port][unit] = di->func;
				if (!quiet)
					printf("  %c%c: %s (%08x)\r\n", 'a'+port, '0'+unit, di->product_name, di->func);
			} else
				func_codes[port][unit] = 0;
		}
		
	return 0;
}


/* A couple of convienence functions to load maple peripherals */

/* First with a given function code... */
uint8 maple_device_addr(int code) {
	int port, unit;

	for (port=0; port<4; port++)
		for (unit=0; unit<6; unit++) {
			if (func_codes[port][unit] & code)
				return maple_create_addr(port, unit);
		}
	return 0;
}

/* First controller */
uint8 maple_controller_addr() {
	return maple_device_addr(MAPLE_FUNC_CONTROLLER);
}

/* First mouse */
uint8 maple_mouse_addr() {
	return maple_device_addr(MAPLE_FUNC_MOUSE);
}

/* First keyboard */
uint8 maple_kb_addr() {
	return maple_device_addr(MAPLE_FUNC_KEYBOARD);
}

/* First LCD unit */
uint8 maple_lcd_addr() {
	return maple_device_addr(MAPLE_FUNC_LCD);
}

/* First VMU */
uint8 maple_vmu_addr() {
	return maple_device_addr(MAPLE_FUNC_MEMCARD);
}

