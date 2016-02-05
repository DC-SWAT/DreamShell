/**
 * Copyright (c) 2011-2015 by SWAT <swat@211.ru>
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
 

#include <kos.h>
#include <dc/fs_dcload.h>
#include "drivers/spi.h"
#include "drivers/sh7750_regs.h"

#define _NO_VIDEO_H
#include "console.h"

#ifdef DEBUG
	#define SPI_DEBUG
	//#define SPI_DEBUG_RW
#endif


#define MSB     	0x80

static uint16 	pwork		=	0;
static mutex_t	spi_mutex	=	MUTEX_INITIALIZER;
static int 		spi_inited	=	0;
static int 		spi_delay	= 	SPI_DEFAULT_DELAY;

static void spi_custom_delay(void);
//static void spi_max_speed_delay(void);
#define spi_max_speed_delay() __asm__("nop\n\tnop\n\tnop\n\tnop\n\tnop")


#define RX_BIT() (reg_read_16(SCSPTR2) & SCSPTR2_SPB2DT)

#define TX_BIT()																\
	_pwork = b & MSB ? (_pwork | SCSPTR2_SPB2DT) : (_pwork & ~SCSPTR2_SPB2DT);	\
	reg_write_16(SCSPTR2, _pwork)

#define CTS_ON()												\
	reg_write_16(SCSPTR2, _pwork | SCSPTR2_CTSDT); 				\
	if(spi_delay == SPI_DEFAULT_DELAY) spi_max_speed_delay();	\
	else spi_custom_delay()
	
#define CTS_ON_FAST()	 reg_write_16(SCSPTR2, _pwork | SCSPTR2_CTSDT); spi_max_speed_delay()
#define CTS_OFF()	reg_write_16(SCSPTR2, _pwork & ~SCSPTR2_CTSDT)


#ifdef SPI_DEBUG

static void dump_regs() {

	uint32 gpio;
	char gpios[512];
	char gr[32];
	int i = 0;
	
	pwork = reg_read_16(SCSPTR2);
	gpio = reg_read_32(PCTRA);
	
	dbglog(DBG_DEBUG, "SCIF registers: \n"
					" SCSCR2:  %04x\n"
					" SCSMR2:  %04x\n"
					" SCFCR2:  %04x\n"
					" SCSSR2:  %04x\n"
					" SCLSR2:  %04x\n"
					" SCSPTR2: %04x\n"
					"	       TX:  %d\n"
					"	       RX:  %02x\n"
					"	       CLK: %d\n"
					"	       CS:  %d\n\n",
					reg_read_16(SCSCR2), 
					reg_read_16(SCSMR2), 
					reg_read_16(SCFCR2), 
					reg_read_16(SCSSR2), 
					reg_read_16(SCLSR2),
					pwork, 
					(int)(pwork & SCSPTR2_SPB2DT),
					(uint8)(pwork & 0x0001),
					(int)(pwork & SCSPTR2_CTSDT),
					(int)(pwork & SCSPTR2_RTSDT));

	dbglog(DBG_DEBUG, "GPIO registers: \n"
					" GPIOIC:  %08lx\n"
					" PDTRA:   %08lx\n"
					" PCTRA:   %08lx\n\n", 
					reg_read_32(GPIOIC), 
					reg_read_16(PDTRA), 
					gpio);
				

	
	for(i = 0; i < 10; i++) {
		
		sprintf(gr, "          GPIO_%d: %d\n", i, (int)(gpio & (1 << i)));
		
		if(!i) {
			strcpy(gpios, gr);
		} else {
			strcat(gpios, gr);
		}
	}
	
	dbglog(DBG_DEBUG, "%s\n", gpios);
	
}

#endif



/**
 * Initializes the SPI interface.
 *
 */
int spi_init(int use_gpio) {
	
	if(spi_inited) {
		return 0;
	}
	
    if(*DCLOADMAGICADDR == DCLOADMAGICVALUE && dcload_type == DCLOAD_TYPE_SER) {
        dbglog(DBG_KDEBUG, "scif_spi_init: no spi device -- using "
               "dcload-serial\n");
        return -1;
    }
	
	scif_shutdown();
	
#ifdef SPI_DEBUG
	dump_regs();
#endif
	
	int cable = CT_NONE;
	uint32 reg;

	// TX:1 CTS:0 RTS:1 (spiout:1 spiclk:0 spics:1)
	pwork = SCSPTR2_SPB2IO | SCSPTR2_CTSIO | SCSPTR2_RTSIO | SCSPTR2_SPB2DT | SCSPTR2_RTSDT;	
	
	reg_write_16(SCSCR2, 0);
	reg_write_16(SCSMR2, 0);
	reg_write_16(SCFCR2, 0);
	reg_write_16(SCSPTR2, pwork);
	reg_write_16(SCSSR2, 0);
	reg_write_16(SCLSR2, 0);
	reg_write_16(SCSCR2, 0);
	
	spi_cs_off(SPI_CS_SCIF_RTS);

	if(use_gpio) {
		
		cable = vid_check_cable();
		
		if(cable != CT_VGA && cable != CT_RGB) {
			
			reg_write_16(PDTRA, 0);
			reg_write_32(PCTRA, 0);
			
			reg_write_16(PDTRA, PDTRA_BIT(SPI_CS_GPIO_VGA) | PDTRA_BIT(SPI_CS_GPIO_RGB)); /* disable */
			
			reg = reg_read_32(PCTRA);
			reg &= ~PCTRA_PBNPUP(SPI_CS_GPIO_VGA); 
			reg |=  PCTRA_PBOUT(SPI_CS_GPIO_VGA);
			reg &= ~PCTRA_PBNPUP(SPI_CS_GPIO_RGB);
			reg |=  PCTRA_PBOUT(SPI_CS_GPIO_RGB); 

			reg_write_32(PCTRA, reg);
			
			spi_cs_off(SPI_CS_GPIO_VGA);
			spi_cs_off(SPI_CS_GPIO_RGB);
			
		} else if(cable != CT_VGA) {
			
			reg_write_16(PDTRA, 0);
			reg_write_32(PCTRA, 0);
			
			reg_write_16(PDTRA, PDTRA_BIT(SPI_CS_GPIO_VGA)); /* disable */
			
			reg = reg_read_32(PCTRA);
			reg &= ~PCTRA_PBNPUP(SPI_CS_GPIO_VGA); 
			reg |=  PCTRA_PBOUT(SPI_CS_GPIO_VGA); 
			reg_write_32(PCTRA, reg);

			spi_cs_off(SPI_CS_GPIO_VGA);
			
		} else {
			;
		}
	}

#ifdef SPI_DEBUG
	dump_regs();
#endif
	
	spi_set_delay(SPI_DEFAULT_DELAY);
	spi_inited = 1;
	return 0;
}

int spi_shutdown() {
	
	if(spi_inited) {
		mutex_destroy(&spi_mutex);
		scif_init();
		spi_inited = 0;
		return 0;
	}
	
	return -1;
}


void spi_set_delay(int delay) {
	spi_delay = delay;
#ifdef SPI_DEBUG
	dbglog(DBG_DEBUG, "spi_set_delay: %d\n", spi_delay);
#endif
}


int spi_get_delay() {
	return spi_delay;
}


// 1.5uSEC delay
static void spi_low_speed_delay(void) {
	timer_prime(TMU1, 2000000, 0);
	timer_clear(TMU1);
	timer_start(TMU1);

	while(!timer_clear(TMU1));
	while(!timer_clear(TMU1));
	while(!timer_clear(TMU1));
	timer_stop(TMU1);
}


static void spi_custom_delay(void) {
	timer_prime(TMU1, spi_delay, 0);
	timer_clear(TMU1);
	timer_start(TMU1);

	while(!timer_clear(TMU1));
	timer_stop(TMU1);
}

void spi_cs_on(int cs) {
	
//	mutex_lock(&spi_mutex);
	uint16 reg;
	
	switch(cs) {
		case SPI_CS_SCIF_RTS:

			pwork &= ~SCSPTR2_RTSDT;
			reg_write_16(SCSPTR2, pwork);
			
			break;
		case SPI_CS_GPIO_VGA:
		case SPI_CS_GPIO_RGB:
		
			reg = reg_read_16(PDTRA);
			reg &= ~(1 << cs);
			reg_write_16(PDTRA, reg);

			break;
		default:
			break;
	}
	
#ifdef SPI_DEBUG
	dbglog(DBG_DEBUG, "spi_cs_on: %d\n", cs);
#endif
}

void spi_cs_off(int cs) {
	
	uint16 reg;
	
	switch(cs) {
		case SPI_CS_SCIF_RTS:

			pwork |= SCSPTR2_RTSDT;
			reg_write_16(SCSPTR2, pwork);

			break;
		case SPI_CS_GPIO_VGA:
		case SPI_CS_GPIO_RGB:
		
			reg = reg_read_16(PDTRA);
			reg |= (1 << cs);
			reg_write_16(PDTRA, reg);
			
			break;
		default:
			break;
	}
	
//	mutex_unlock(&spi_mutex);
	
#ifdef SPI_DEBUG
	dbglog(DBG_DEBUG, "spi_cs_off: %d\n", cs);
#endif
}


/**
 * Sends a byte over the SPI bus.
 *
 * \param[in] b The byte to send.
 */
void spi_cc_send_byte(register uint8 b) {

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
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_send_byte: %02x\n", b);
	//dump_regs();
#endif
}

void spi_send_byte(register uint8 b) {

	register uint16 _pwork;
	_pwork = pwork;
	
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	TX_BIT();
	CTS_ON_FAST();		/* SPI clock ON */
	b = (b << 1);
	CTS_OFF();				/* SPI clock OFF */
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_send_byte: %02x\n", b);
	//dump_regs();
#endif
}


/**
 * Receives a byte from the SPI bus.
 *
 * \returns The received byte.
 */
uint8 spi_cc_rec_byte() {

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
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_rec_byte: %02x\n", b);
	//dump_regs();
#endif

	return b;
}

uint8 spi_rec_byte() {

	register uint8 b;
	register uint16 _pwork;
  
	_pwork = pwork;
	b = 0xff;
	
	TX_BIT();
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	CTS_ON_FAST();			/* SPI clock ON */
	b = (b << 1) | RX_BIT();	/* SPI data input */
	CTS_OFF();					/* SPI clock OFF */
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_rec_byte: %02x\n", b);
	//dump_regs();
#endif

	return b;
}


/**
 * Send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_cc_sr_byte(register uint8 b) {

	register uint16 _pwork;
	_pwork = pwork;
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_sr_byte: send: %02x\n", b);
	//dump_regs();
#endif

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

#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_sr_byte: receive: %02x\n", b);
	//dump_regs();
#endif

	return b;
}

uint8 spi_sr_byte(register uint8 b) {

	register uint16 _pwork;
	_pwork = pwork;
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_sr_byte: send: %02x\n", b);
	//dump_regs();
#endif

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

#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_sr_byte: receive: %02x\n", b);
	//dump_regs();
#endif

	return b;
}


/**
 * Slow send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_slow_sr_byte(register uint8 b) {
	
	register int cnt;
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_slow_sr_byte: send: %02x\n", b);
#endif

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
	
#ifdef SPI_DEBUG_RW
	dbglog(DBG_DEBUG, "spi_slow_sr_byte: receive: %02x\n", b);
	//dump_regs();
#endif
	
	return b;
}


/**
 * Sends data contained in a buffer over the SPI bus.
 *
 * \param[in] data A pointer to the buffer which contains the data to send.
 * \param[in] len The number of bytes to send.
 */
void spi_cc_send_data(const uint8* data, uint16 len) {
    do {
		/* Used send/receive byte because only receive works unstable */
		spi_cc_sr_byte(*data++);
    } while(--len);
}

void spi_send_data(const uint8* data, uint16 len) {

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
}


/**
 * Receives multiple bytes from the SPI bus and writes them to a buffer.
 *
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] len The number of bytes to read.
 */
void spi_cc_rec_data(uint8* buffer, uint16 len) {
    do {
        *buffer++ = spi_rec_byte();
    } while(--len);
}

void spi_rec_data(uint8* buffer, uint16 len) {
	
	register uint8 b;
	register uint16 _pwork;
//	register uint32 tmp __asm__("r1");
	_pwork = pwork;
	
	while(len) {
		
//		tmp = 0;
		b = 0xff;
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		buffer[0] = b;
//		tmp |= b;
		b = 0xff;
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		buffer[1] = b;
//		tmp |= b << 8;
		b = 0xff;
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		buffer[2] = b;
//		tmp |= b << 16;
		b = 0xff;
		
		TX_BIT();
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		CTS_ON_FAST();			/* SPI clock ON */
		b = (b << 1) | RX_BIT();	/* SPI data input */
		CTS_OFF();					/* SPI clock OFF */
		
		buffer[3] = b;
//		tmp |= b << 24;
//		__asm__ __volatile__("mov r1, r0\nmovca.l r0, @%0\n" : : "r" ((uint32)buffer));
//		*(uint32*)buffer = tmp;
		
		buffer += 4;
		len -= 4;
	}
}
