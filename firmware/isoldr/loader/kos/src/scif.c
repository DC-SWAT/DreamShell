/* KallistiOS ##version##

   hardware/scif.c
   Copyright (C)2000,2001,2004 Dan Potter
*/

#include <dc/scif.h>

/*

This module handles very basic serial I/O using the SH4's SCIF port. FIFO
mode is used by default; you can turn this off to avoid forcing a wait
when there is no serial device attached.

Unlike in KOS 1.x, this is not designed to be used as the normal I/O, but
simply as an early debugging device in case something goes wrong in the
kernel or for debugging it.

*/

/* SCIF registers */
#define SCIFREG08(x) *((volatile uint8 *)(x))
#define SCIFREG16(x) *((volatile uint16 *)(x))
#define SCSMR2	SCIFREG16(0xffeb0000)
#define SCBRR2	SCIFREG08(0xffe80004)
#define SCSCR2	SCIFREG16(0xffe80008)
#define SCFTDR2	SCIFREG08(0xffe8000C)
#define SCFSR2	SCIFREG16(0xffe80010)
#define SCFRDR2	SCIFREG08(0xffe80014)
#define SCFCR2	SCIFREG16(0xffe80018)
#define SCFDR2	SCIFREG16(0xffe8001C)
#define SCSPTR2	SCIFREG16(0xffe80020)
#define SCLSR2	SCIFREG16(0xffe80024)

//#define PTR2_RTSIO  (1 << 7)
//#define PTR2_RTSDT  (1 << 6)

/* Default serial parameters */
//static int serial_baud = 57600;
//static int serial_fifo = 1;

/* Set serial parameters; this is not platform independent like I want
   it to be, but it should be generic enough to be useful. */
//void scif_set_parameters(int baud, int fifo) {
//	serial_baud = baud;
//	serial_fifo = fifo;
//}

/* Initialize the SCIF port; baud_rate must be at least 9600 and
   no more than 57600. 115200 does NOT work for most PCs. */
// recv trigger to 1 byte
int scif_init() {
	int i;
	/* int fifo = 1; */

	/* Disable interrupts, transmit/receive, and use internal clock */
	SCSCR2 = 0;

	/* Enter reset mode */
	SCFCR2 = 0x06;
	
	/* 8N1, use P0 clock */
	SCSMR2 = 0;
	
	/* If baudrate unset, set baudrate, N = P0/(32*B)-1 */
//	if (SCBRR2 == 0xff)
		SCBRR2 = (uint8)(50000000 / (32 * 115200)) - 1;

	/* Wait a bit for it to stabilize */
	for (i=0; i<10000; i++)
		__asm__("nop");

	/* Unreset, enable hardware flow control, triggers on 8 bytes */
	SCFCR2 = 0x48;
	
	/* Disable manual pin control */
	SCSPTR2 = 0;
	
	/* Disable SD */
//	SCSPTR2 = PTR2_RTSIO | PTR2_RTSDT;
	
	/* Clear status */
	(void)SCFSR2;
	SCFSR2 = 0x60;
	(void)SCLSR2;
	SCLSR2 = 0;
	
	/* Enable transmit/receive */
	SCSCR2 = 0x30;

	/* Wait a bit for it to stabilize */
	for (i=0; i<10000; i++)
		__asm__("nop");

	return 0;
}

/* Read one char from the serial port (-1 if nothing to read) */
int scif_read() {
	int c;

	if (!(SCFDR2 & 0x1f))
		return -1;

	// Get the input char
	c = SCFRDR2;

	// Ack
	SCFSR2 &= ~0x92;

	return c;
}

/* Write one char to the serial port (call serial_flush()!) */
int scif_write(int c) {
	int timeout = 100000;

	/* Wait until the transmit buffer has space. Too long of a failure
	   is indicative of no serial cable. */
	while (!(SCFSR2 & 0x20) && timeout > 0)
		timeout--;
	if (timeout <= 0)
		return -1;
	
	/* Send the char */
	SCFTDR2 = c;
	
	/* Clear status */
	SCFSR2 &= 0xff9f;

	return 1;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
int scif_flush() {
	int timeout = 100000;

	SCFSR2 &= 0xbf;

	while (!(SCFSR2 & 0x40) && timeout > 0)
		timeout--;
	if (timeout <= 0)
		return -1;

	SCFSR2 &= 0xbf;

	return 0;
}

/* Send an entire buffer */
int scif_write_buffer(const uint8 *data, int len, int xlat) {
	int rv, i = 0, c;
	while (len-- > 0) {
		c = *data++;
		if (xlat) {
			if (c == '\n') {
				if (scif_write('\r') < 0)
					return -1;
				i++;
			}
		}
		rv = scif_write(c);
		if (rv < 0)
			return -1;
		i += rv;
	}
	if (scif_flush() < 0)
		return -1;

	return i;
}

/* Read an entire buffer (block) */
int scif_read_buffer(uint8 *data, int len) {
	int c, i = 0;
	while (len-- > 0) {
		while ( (c = scif_read()) == -1)
			;
		*data++ = c;
		i++;
	}

	return i;
}
