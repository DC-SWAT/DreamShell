/* KallistiOS ##version##

   net/broadband_adapter.c

   Copyright (C)2001,2003 Dan Potter

 */

#include <stdio.h>
#include <string.h>
#include <dc/net/broadband_adapter.h>
//#include <dc/asic.h>
#include <dc/g2bus.h>
#include <arch/irq.h>
#include <net/net.h>
#include <arch/timer.h>
#include "main.h"

//CVSID("$Id: broadband_adapter.c,v 1.3 2003/07/22 03:31:58 bardtx Exp $");

/*

Contains a low-level ethernet driver for the "Broadband Adapter", which
is principally a RealTek 8193C chip attached to the G2 external bus
using a PCI bridge chip called "GAPS PCI". GAPS PCI might ought to be
in its own module, but AFAIK this is the only peripheral to use this
chip, and quite possibly will be the only peripheral to ever use it.

Thanks to Andrew Kieschnick for finishing the driver info for the rtl8193c
(mainly the transmit code, and lots of help with the error correction).
Also thanks to the NetBSD sources for some info on register names.

This driver has basically been rewritten since KOS 1.0.x.

*/

/****************************************************************************/
/* GAPS PCI stuff probably ought to be moved to another file... */

/* Detect a GAPS PCI bridge */
static int gaps_detect() {
	char str[16];

	g2_read_block_8((uint8*)str, 0xa1001400, 16);
	if (!memcmp(str, "GAPSPCI_BRIDGE_2", 16))
		return 0;
	else
		return -1;
}

/* Initialize GAPS PCI bridge */
#define GAPS_BASE 0xa1000000
static int gaps_init() {
	int i;

	/* Make sure we have one */
	if (gaps_detect() < 0) {
		DBG("bba: no ethernet card found");
		return -1;
	}

	/* Initialize the "GAPS" PCI glue controller.
	   It ain't pretty but it works. */
	g2_write_32(GAPS_BASE + 0x1418, 0x5a14a501);	/* M */
	i = 10000;
	while (!(g2_read_32(GAPS_BASE + 0x1418) & 1) && i > 0)
		i--;
	if (!(g2_read_32(GAPS_BASE + 0x1418) & 1)) {
		DBG("bba: GAPS PCI controller not responding; giving up!");
		return -1;
	}
	g2_write_32(GAPS_BASE + 0x1420, 0x01000000);
	g2_write_32(GAPS_BASE + 0x1424, 0x01000000);
	g2_write_32(GAPS_BASE + 0x1428, 0x01840000);		/* DMA Base */
	g2_write_32(GAPS_BASE + 0x142c, 0x01840000 + 32*1024);	/* DMA End */
	g2_write_32(GAPS_BASE + 0x1414, 0x00000001);		/* Interrupt enable */
	g2_write_32(GAPS_BASE + 0x1434, 0x00000001);

	/* Configure PCI bridge (very very hacky). If we wanted to be proper,
	   we ought to implement a full PCI subsystem. In this case that is
	   ridiculous for accessing a single card that will probably never
	   change. Considering that the DC is now out of production officially,
	   there is a VERY good chance it will never change. */
	g2_write_16(GAPS_BASE + 0x1606, 0xf900);
	g2_write_32(GAPS_BASE + 0x1630, 0x00000000);
	g2_write_8(GAPS_BASE + 0x163c, 0x00);
	g2_write_8(GAPS_BASE + 0x160d, 0xf0);
	g2_read_16(GAPS_BASE + 0x0004);
	g2_write_16(GAPS_BASE + 0x1604, 0x0006);
	g2_write_32(GAPS_BASE + 0x1614, 0x01000000);
	g2_read_8(GAPS_BASE + 0x1650);

	return 0;
}

/****************************************************************************/
/* RTL8193C stuff */

/* Configuration definitions */
#define RX_BUFFER_LEN		16384

/* RTL8139C Config/Status info */
struct {
	uint16	cur_rx;		/* Current Rx read ptr */
	uint16	cur_tx;		/* Current available Tx slot */
	uint8	mac[6];		/* Mac address */
} rtl;

/* 8, 16, and 32 bit access to the PCI I/O space (configured by GAPS) */
#define NIC(ADDR) (0xa1001700 + (ADDR))

/* 8 and 32 bit access to the PCI MEMMAP space (configured by GAPS) */
static uint32 const rtl_mem = 0xa1840000;

/* TX buffer pointers */
static uint32 const txdesc[4] = {	0xa1846000,
					0xa1846800,
					0xa1847000,
					0xa1847800 };

/* Is the link stabilized? */
static volatile int link_stable, link_initial;

/* Forward-declaration for IRQ handler */
static void bba_irq_hnd(uint32 code);

/* Reads the MAC address of the BBA into the specified array */
void bba_get_mac(uint8 *arr) {
	memcpy(arr, rtl.mac, 6);
}

/*
  Initializes the BBA
  
  Returns 0 for success or -1 for failure.
 */
static int bba_hw_init() {
	int i;
	uint32 tmp;

	link_stable = 0;
	link_initial = 0;

	/* Initialize GAPS */
	if (gaps_init() < 0)
		return -1;

	/* Soft-reset the chip */
	g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RESET);
	
	/* Wait for it to come back */
	i = 100;
	while ((g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET) && i > 0) {
		i--;
		timer_spin_sleep(10);
	}
	if (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET) {
		DBG("bba: timed out on reset #1");
		return -1;
	}

	/* Reset CONFIG1 */
	g2_write_8(NIC(RT_CONFIG1), 0);
	
	/* Enable auto-negotiation and restart that process */
	/* NIC16(RT_MII_BMCR) = 0x1200; */
	g2_write_16(NIC(RT_MII_BMCR), 0x9200);
	
	/* Do another reset */
	g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RESET);
	
	/* Wait for it to come back */
	i = 100;
	while ((g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET) && i > 0) {
		i--;
		timer_spin_sleep(10);
	}
	if (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET) {
		DBG("bba: timed out on reset #2");
		return -1;
	}

	/* Enable writing to the config registers */
	g2_write_8(NIC(RT_CFG9346), 0xc0);
	
	/* Read MAC address */
	tmp = g2_read_32(NIC(RT_IDR0));
	rtl.mac[0] = tmp & 0xff;
	rtl.mac[1] = (tmp >> 8) & 0xff;
	rtl.mac[2] = (tmp >> 16) & 0xff;
	rtl.mac[3] = (tmp >> 24) & 0xff;
	tmp = g2_read_32(NIC(RT_IDR0+4));
	rtl.mac[4] = tmp & 0xff;
	rtl.mac[5] = (tmp >> 8) & 0xff;
	DBG("bba: MAC Address is %02x:%02x:%02x:%02x:%02x:%02x",
		rtl.mac[0], rtl.mac[1], rtl.mac[2],
		rtl.mac[3], rtl.mac[4], rtl.mac[5]);

	/* Enable receive and transmit functions */
	g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);

	/* Set Rx FIFO threashold to 1K, Rx size to 16k+16, 1024 byte DMA burst */
	/* nic32[RT_RXCONFIG/4] = 0x0000c600; */

	/* Set Rx FIFO threashold to 1K, Rx size to 8k, 1024 byte DMA burst */
	g2_write_32(NIC(RT_RXCONFIG), 0x00000e00);
	
	/* Set Tx 1024 byte DMA burst */
	g2_write_32(NIC(RT_TXCONFIG), 0x00000600);

	/* Turn off lan-wake and set the driver-loaded bit */
	g2_write_8(NIC(RT_CONFIG1), (g2_read_8(NIC(RT_CONFIG1)) & ~0x30) | 0x20);
	
	/* Enable FIFO auto-clear */
	g2_write_8(NIC(RT_CONFIG4), g2_read_8(NIC(RT_CONFIG4)) | 0x80);
	
	/* Switch back to normal operation mode */
	g2_write_8(NIC(RT_CFG9346), 0);
	
	/* Setup RX/TX buffers */
	g2_write_32(NIC(RT_RXBUF), 0x01840000);
	g2_write_32(NIC(RT_TXADDR0 + 0), 0x01846000);
	g2_write_32(NIC(RT_TXADDR0 + 4), 0x01846800);
	g2_write_32(NIC(RT_TXADDR0 + 8), 0x01847000);
	g2_write_32(NIC(RT_TXADDR0 + 12), 0x01847800);

	/* Reset RXMISSED counter */
	g2_write_32(NIC(RT_RXMISSED), 0);
	
	/* Enable receiving broadcast and physical match packets */
	g2_write_32(NIC(RT_RXCONFIG), g2_read_32(NIC(RT_RXCONFIG)) | 0x0000000a);
	
	/* Filter out all multicast packets */
	g2_write_32(NIC(RT_MAR0 + 0), 0);
	g2_write_32(NIC(RT_MAR0 + 4), 0);
	
	/* Disable all multi-interrupts */
	g2_write_16(NIC(RT_MULTIINTR), 0);

	/* Enable G2 interrupts */
	// XXX
	/* asic_evt_set_handler(ASIC_EVT_EXP_PCI, bba_irq_hnd);
	asic_evt_enable(ASIC_EVT_EXP_PCI, ASIC_IRQB); */

	/* Enable receive interrupts */
	/* XXX need to handle more! */
	g2_write_16(NIC(RT_INTRSTATUS), 0xffff);
	g2_write_16(NIC(RT_INTRMASK), RT_INT_PCIERR |
		RT_INT_TIMEOUT |
		RT_INT_RXFIFO_OVERFLOW |
		RT_INT_RXFIFO_UNDERRUN |	// +link change
		RT_INT_RXBUF_OVERFLOW |
		RT_INT_TX_ERR |
		RT_INT_TX_OK |
		RT_INT_RX_ERR |
		RT_INT_RX_OK);

	/* Enable RX/TX once more */
	g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);

	/* Initialize status vars */
	rtl.cur_tx = 0;
	rtl.cur_rx = 0;

	return 0;
}

static void rx_reset() {
	rtl.cur_rx = g2_read_16(NIC(RT_RXBUFHEAD));
	g2_write_16(NIC(RT_RXBUFTAIL), rtl.cur_rx - 16);

	rtl.cur_rx = 0;
	g2_write_8(NIC(RT_CHIPCMD), RT_CMD_TX_ENABLE);

	g2_write_32(NIC(RT_RXCONFIG), 0x00000e0a);

	while (!(g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RX_ENABLE))
		g2_write_8(NIC(RT_CHIPCMD), RT_CMD_TX_ENABLE | RT_CMD_RX_ENABLE);

	g2_write_32(NIC(RT_RXCONFIG), 0x00000e0a);
	g2_write_16(NIC(RT_INTRSTATUS), 0xffff);
}

static void bba_hw_shutdown() {
	/* Disable receiver */
	g2_write_32(NIC(RT_RXCONFIG), 0);

	/* Disable G2 interrupts */
	// XXX
	/* asic_evt_disable(ASIC_EVT_EXP_PCI, ASIC_IRQB);
	asic_evt_set_handler(ASIC_EVT_EXP_PCI, NULL); */
}
#if 0
/* Utility function to copy out a packet from the ring buffer into an SH-4
   buffer. This is done to make sure the buffers don't overflow. */
/* XXX Could probably use a memcpy8 here, even */
static void bba_copy_packet(uint8 *dst, uint32 src, int len) {
	if ( (src+len) < RX_BUFFER_LEN ) {
		/* Straight copy is ok */
		g2_read_block_8(dst, rtl_mem+src, len);
	} else {
		/* Have to copy around the ring end */
		g2_read_block_8(dst, rtl_mem+src, RX_BUFFER_LEN - src);
		g2_read_block_8(dst+(RX_BUFFER_LEN - src),
			rtl_mem, len - (RX_BUFFER_LEN - src));
	}
}


/* Copy one received packet out of the RX DMA space and into the RX buffer */
static void rx_enq(int ring_offset, int pkt_size) {
	// int i;

	/* Copy it out */
	bba_copy_packet(net_rxbuf, ring_offset, pkt_size);

	/* //DBG("Received packet:\r\n");
	for (i=0; i<pkt_size; i++) {
		//DBG("%02x ", net_rxbuf[i]);
		if (i && !(i % 16))
			//DBG("\r\n");
	}
	//DBG("\r\n"); */

	/* Call the callback to process it */
	net_input(&bba_if, pkt_size);
}

/* "Receive" any available packets and send them through the callback. */
static int bba_rx() {
	int	processed;
	uint32	rx_status;
	int	rx_size, pkt_size, ring_offset, intr;
	
	processed = 0;
	DBG("bba_rx");
	/* While we have frames left to process... */	
	while (!(g2_read_8(NIC(RT_CHIPCMD)) & 1)) {
		
		/* Get frame size and status */
		ring_offset = rtl.cur_rx % RX_BUFFER_LEN;
		rx_status = g2_read_32(rtl_mem + ring_offset);
		rx_size = (rx_status >> 16) & 0xffff;
		pkt_size = rx_size - 4;

		if (rx_size == 0xfff0) {
			DBG("bba: early receive triggered");
			break;
		}
		
		if (!(rx_status & 1)) {
			DBG("bba: frame receive error, status is %08lx; skipping", rx_status);
			rx_reset();
			break;
		}

		if ((rx_status & 1) && (pkt_size <= 1514)) {
			/* Add it to the rx queue */
			rx_enq(ring_offset + 4, pkt_size);
		} else {
			DBG("bba: bogus packet receive detected; skipping packet");
			rx_reset();
			break;
		}

		/* Tell the chip where we are for overflow checking */
		rtl.cur_rx = (rtl.cur_rx + rx_size + 4 + 3) & ~3;
		g2_write_16(NIC(RT_RXBUFTAIL), rtl.cur_rx - 16);

		/* If the chip is expecting an ACK, give it an ACK */
		intr = g2_read_16(NIC(RT_INTRSTATUS));
		if (intr & RT_INT_RX_ACK)
			g2_write_16(NIC(RT_INTRSTATUS), RT_INT_RX_ACK);

		/* Increase our "processed" count */
		processed++;
	}
	
	return processed;
}
#endif

/* Transmit a single packet */
int bba_tx(const uint8 *pkt, int len) {
	/* int i;

	//DBG("Transmitting packet:\r\n");
	for (i=0; i<len; i++) {
		//DBG("%02x ", pkt[i]);
		if (i && !(i % 16))
			//DBG("\r\n");
	}
	//DBG("\r\n"); */

	if (!link_stable) {
		while (!link_stable) {
			bba_irq_hnd(0);
		}
	}

	/* Wait till it's clear to transmit */
	while ( !(g2_read_32(NIC(RT_TXSTATUS0 + 4*rtl.cur_tx)) & 0x2000) ) {
		if (g2_read_32(NIC(RT_TXSTATUS0 + 4*rtl.cur_tx)) & 0x40000000)
			g2_write_32(NIC(RT_TXSTATUS0 + 4*rtl.cur_tx),
				g2_read_32(NIC(RT_TXSTATUS0 + 4*rtl.cur_tx)) | 1);
	}
	
	/* Copy the packet out to RTL memory */
	/* XXX could use store queues or memcpy8 here */
	/*if (len & 3) len = (len + 3) & ~3; */
	g2_write_block_8(pkt, txdesc[rtl.cur_tx], len);
	
	/* All packets must be at least 60 bytes */
	if (len < 60) len = 60;
	
	/* Transmit from the current TX buffer */
	g2_write_32(NIC(RT_TXSTATUS0 + 4*rtl.cur_tx), len);
	
	/* Go to the next TX buffer */
	rtl.cur_tx = (rtl.cur_tx + 1) % 4;

	return 0;
}

/* Ethernet IRQ handler */
static void bba_irq_hnd(uint32 code) {
	int intr;//, hnd;

	(void)code;

	/* Acknowledge 8193 interrupt, except RX ACK bits. We'll handle
	   those in the RX int handler. */
	intr = g2_read_16(NIC(RT_INTRSTATUS));
	g2_write_16(NIC(RT_INTRSTATUS), intr & ~RT_INT_RX_ACK);

	/* Do processing */
//	hnd = 0;
	if (intr & RT_INT_RX_ACK) {
//		bba_rx(); hnd = 1;
	}
//	if (intr & RT_INT_TX_OK) {
//		hnd = 1;
//	}
	if (intr & RT_INT_LINK_CHANGE) {
		// Get the MII media status reg.
		uint32 bmsr = g2_read_16(NIC(RT_MII_BMSR));

		// If this is the first time, force a renegotiation.
		if (!link_initial) {
			bmsr &= ~(RT_MII_LINK | RT_MII_AN_COMPLETE);
			DBG("bba: initial link change, redoing auto-neg");
		}
		
		// This should really be a bit more complete, but this
		// should be sufficient.

		// Is our link there?
		if (bmsr & RT_MII_LINK) {
			// We must have just finished an auto-negotiation.
			DBG("bba: link stable");

			// The link is back.
			link_stable = 1;
		} else {
			if (link_initial)
				DBG("bba: link lost");

			// Do an auto-negotiation.
			g2_write_16(NIC(RT_MII_BMCR),
				RT_MII_RESET |
				RT_MII_AN_ENABLE |
				RT_MII_AN_START);

			// The link is gone.
			link_stable = 0;
		}

		// We've done our initial link interrupt now.
		link_initial = 1;
		
//		hnd = 1;
	}
	if (intr & RT_INT_RXBUF_OVERFLOW) {
		DBG("bba: RX overrun\n");
		rx_reset();
//		hnd = 1;
	}
	/*
	 if (!hnd) {
		DBG("bba: spurious interrupt, status is %04x\n", intr);
	 }*/ 
}

/****************************************************************************/
/* Netcore interface */

netif_t bba_if;

/* They only ever made one GAPS peripheral, so this should suffice */
static int bba_if_detect(netif_t *self) {
	(void)self;
	
	if (bba_if.flags & NETIF_DETECTED)
		return 0;

	if (gaps_detect() < 0)
		return -1;

	bba_if.flags |= NETIF_DETECTED;
	return 0;
}

static int bba_if_init(netif_t *self) {
	(void)self;
	
	if (bba_if.flags & NETIF_INITIALIZED)
		return 0;

	if (bba_hw_init() < 0)
		return -1;

	memcpy(bba_if.mac_addr, rtl.mac, 6);
	bba_if.flags |= NETIF_INITIALIZED;
	return 0;
}

static int bba_if_shutdown(netif_t *self) {
	(void)self;
	
	if (!(bba_if.flags & NETIF_INITIALIZED))
		return 0;

	bba_hw_shutdown();
	
	bba_if.flags &= ~(NETIF_INITIALIZED | NETIF_RUNNING);
	return 0;
}

static int bba_if_start(netif_t *self) {
	int i;
	
	(void)self;

	if (!(bba_if.flags & NETIF_INITIALIZED))
		return -1;

	/* We need something like this to get DHCP to work (since it doesn't
	   know anything about an activated and yet not-yet-receiving network
	   adapter =) */
	/* Spin until the link is stabilized */
	i = 1000;
	while (!link_stable && i>0) {
		bba_irq_hnd(0);
		i--;
		timer_spin_sleep(10);
	}
	if (!link_stable) {
		DBG("bba: timed out waiting for link to stabilize");
		return -1;
	}

	bba_if.flags |= NETIF_RUNNING;
	return 0;
}

static int bba_if_stop(netif_t *self) {
	(void)self;
	
	if (!(bba_if.flags & NETIF_RUNNING))
		return -1;

	bba_if.flags &= ~NETIF_RUNNING;
	return 0;
}

static int bba_if_tx(netif_t *self, const uint8 *data, int len) {
	(void)self;
	
	if (!(bba_if.flags & NETIF_RUNNING))
		return -1;
	
	//DBG("bba_if_tx");
	
	if (bba_tx(data, len) != 0)
		return -1;

	return 0;
}

/* All RX is done via the interrupt */
static int bba_if_rx_poll(netif_t *self) {
	(void)self;
	
	bba_irq_hnd(0);
	//DBG("bba_if_rx_poll");
	return 0;
}

/* Don't need to hook anything here yet */
static int bba_if_set_flags(netif_t *self, uint32 flags_and, uint32 flags_or) {

	(void)self;
	
	bba_if.flags = (bba_if.flags & flags_and) | flags_or;
	return 0;
}

/* Initialize */
int bba_init() {
	/* Setup the structure */
	bba_if.descr = "Broadband Adapter (HIT-0400)";
	bba_if.flags = NETIF_NO_FLAGS;
	bba_get_mac(bba_if.mac_addr);
	memset(bba_if.ip_addr, 0, sizeof(bba_if.ip_addr));
	bba_if.if_detect = bba_if_detect;
	bba_if.if_init = bba_if_init;
	bba_if.if_shutdown = bba_if_shutdown;
	bba_if.if_start = bba_if_start;
	bba_if.if_stop = bba_if_stop;
	bba_if.if_tx = bba_if_tx;
	bba_if.if_rx_poll = bba_if_rx_poll;
	bba_if.if_set_flags = bba_if_set_flags;

	return 0;
}

/* Shutdown */
int bba_shutdown() {
	/* Shutdown hardware */
	bba_if.if_stop(&bba_if);
	bba_if.if_shutdown(&bba_if);
	
	return 0;
}
