/**
 * Copyright (c) 2011-2016 SWAT <http://www.dc-swat.ru>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 *	Dreamcast SCIF <=> SPI interface
 *
 *	SCIF          	SD Card
 *	-----------------------
 *	RTS (SPI CS)   --> /CS
 *	CTS (SPI CLK)  --> CLK
 *	TX  (SPI DOUT) --> DIN
 *	RX  (SPI DIN)  <-- DOUT
 */
 

#include "spi.h"
#include "drivers/sh7750_regs.h"

//#define SPI_DEBUG
//#define SPI_DEBUG_RW


#define MSB     	0x80
static uint16 	pwork		=	0;

#define spi_max_speed_delay() /*__asm__("nop\n\tnop\n\tnop\n\tnop\n\tnop")*/

#define RX_BIT() 	(uint8)(reg_read_16(SCSPTR2) & SCSPTR2_SPB2DT)
#define TX_BIT()	_pwork = b & MSB ? (_pwork | SCSPTR2_SPB2DT) : (_pwork & ~SCSPTR2_SPB2DT); reg_write_16(SCSPTR2, _pwork)
#define CTS_ON() reg_write_16(SCSPTR2, _pwork | SCSPTR2_CTSDT); spi_max_speed_delay()
#define CTS_ON_FAST CTS_ON
#define CTS_OFF()	reg_write_16(SCSPTR2, _pwork & ~SCSPTR2_CTSDT)


/**
 * Initializes the SPI interface.
 *
 */
int spi_init() {

	// TX:1 CTS:0 RTS:1 (spiout:1 spiclk:0 spics:1)
	pwork = SCSPTR2_SPB2IO | SCSPTR2_CTSIO | SCSPTR2_RTSIO | SCSPTR2_SPB2DT | SCSPTR2_RTSDT;	
	
	reg_write_16(SCSCR2, 0);
	reg_write_16(SCSMR2, 0);
	reg_write_16(SCFCR2, 0);
	reg_write_16(SCSPTR2, pwork);
	reg_write_16(SCSSR2, 0);
	reg_write_16(SCLSR2, 0);
	reg_write_16(SCSCR2, 0);
	
	spi_cs_off();
	return 0;
}

//int spi_shutdown() {
//	return 0;
//}

#ifndef NO_SD_INIT

// 1.5uSEC delay
static void spi_low_speed_delay(void) {
//	timer_prime(TMU2, 2000000, 0);
//	timer_clear(TMU2);
//	timer_start(TMU2);
//
//	while(!timer_clear(TMU2));
//	while(!timer_clear(TMU2));
//	while(!timer_clear(TMU2));
//	timer_stop(TMU2);

  for (int i = 0; i < 100; ++i) 
	  __asm__("nop\n");
}

#endif

void spi_cs_on() {
	pwork &= ~SCSPTR2_RTSDT;
	reg_write_16(SCSPTR2, pwork);
}

void spi_cs_off() {
	pwork |= SCSPTR2_RTSDT;
	reg_write_16(SCSPTR2, pwork);
}



/**
 * Sends a byte over the SPI bus.
 *
 * \param[in] b The byte to send.
 */
void spi_send_byte(register uint8 b) {

	register uint16 _pwork;
	_pwork = pwork;
	
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
	TX_BIT();
	CTS_ON();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();		/* SPI clock OFF */
}


/**
 * Receives a byte from the SPI bus.
 *
 * \returns The received byte.
 */
uint8 spi_rec_byte() {

	register uint8 b;
	register uint16 _pwork;
  
	_pwork = pwork;
	b = 0xff;
	
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */

	return b;
}


/**
 * Send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_sr_byte(register uint8 b) {

	register uint16 _pwork;
	_pwork = pwork;

	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */
	TX_BIT();
	CTS_ON();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();			/* SPI clock OFF */

	return b;
}


#ifndef NO_SD_INIT

/**
 * Slow send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_slow_sr_byte(register uint8 b) {
	
	register int cnt;

	for (cnt = 0; cnt < 8; cnt++) {
		
		pwork = b & MSB ? (pwork | SCSPTR2_SPB2DT) : (pwork & ~SCSPTR2_SPB2DT);
		reg_write_16(SCSPTR2, pwork);
		
		spi_low_speed_delay();
		pwork |= SCSPTR2_CTSDT;
		reg_write_16(SCSPTR2, pwork);
		b = (b << 1) | RX_BIT();
		spi_low_speed_delay();
		
		pwork &= ~SCSPTR2_CTSDT;
		reg_write_16(SCSPTR2, pwork);
	}
	
	return b;
}

#endif

#if _FS_READONLY == 0

/**
 * Sends data contained in a buffer over the SPI bus.
 *
 * \param[in] data A pointer to the buffer which contains the data to send.
 * \param[in] len The number of bytes to send.
 */

void spi_send_data(const uint8* data, uint16 len) {
#if 0
	register uint8 b;
	register uint16 _pwork;
	_pwork = pwork;
	
	/* Used send/receive byte because only receive works unstable */
	while(len) {
		
		b = data[0];
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
	
		b = data[1];
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		b = data[2];
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		b = data[3];
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		data += 4;
		len -= 4;
	}
#else
	while(len--)
		spi_send_byte(*data++);
#endif
}

#endif

/**
 * Receives multiple bytes from the SPI bus and writes them to a buffer.
 *
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] len The number of bytes to read.
 */

void spi_rec_data(uint8* buffer, uint16 len) {
	uint8 b = 0xff;
	uint16 cts_off = (pwork & ~SCSPTR2_CTSDT) | SCSPTR2_SPB2DT;
	uint16 cts_on = (pwork | SCSPTR2_CTSDT | SCSPTR2_SPB2DT);
	uint32 *ptr = (uint32 *)buffer;
	uint32 data;

	reg_write_16(SCSPTR2, cts_off);

	for(; len > 0; len -= 4) {
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */

		data = b;

		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */

		data |= b << 8;

		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */

		data |= b << 16;

		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */
		reg_write_16(SCSPTR2, cts_on);	/* SPI clock ON */
		b = (b << 1) | RX_BIT();		/* SPI data input */
		reg_write_16(SCSPTR2, cts_off);	/* SPI clock OFF */

		data |= b << 24;
		*ptr++ = data;
	}
}
