/* KallistiOS ##version##

   include/kos/net.h
   Copyright (C)2002,2004 Dan Potter

   $Id: net.h,v 1.8 2003/06/19 04:30:23 bardtx Exp $

*/

#ifndef __KOS_NET_H
#define __KOS_NET_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* All functions in this header return < 0 on failure, and 0 on success. */

/* Structure describing one usable network device; this is pretty ethernet
   centric, though I suppose you could stuff other things into this system
   like PPP. */
typedef struct knetif {
	/* Long description of the device */
	const char			*descr;

	/* Interface flags */
	uint32				flags;

	/* The device's MAC address */
	uint8				mac_addr[6];

	/* The device's IP address (if any) */
	uint8				ip_addr[4];

	/* All of the following callback functions should return a negative
	   value on failure, and a zero or positive value on success. Some
	   functions have special values, as noted. */

	/* Attempt to detect the device */
	int (*if_detect)(struct knetif * self);

	/* Initialize the device */
	int (*if_init)(struct knetif * self);

	/* Shutdown the device */
	int (*if_shutdown)(struct knetif * self);

	/* Start the device (after init or stop) */
	int (*if_start)(struct knetif * self);

	/* Stop (hibernate) the device */
	int (*if_stop)(struct knetif * self);

	/* Queue a packet for transmission; see special return values
	   below the structure */
	int (*if_tx)(struct knetif * self, const uint8 * data, int len);

	/* Poll for queued receive packets, if neccessary. Returns non-zero
	   if we need to exit the current polling loop (if any). */
	int (*if_rx_poll)(struct knetif * self);

	/* Set flags; you should generally manipulate flags through here so that
	   the driver gets a chance to act on the info. */
	int (*if_set_flags)(struct knetif * self, uint32 flags_and, uint32 flags_or);
} netif_t;

/* Flags for netif_t */
#define NETIF_NO_FLAGS		0x00000000
#define NETIF_REGISTERED	0x00000001		/* Is it registered? */
#define NETIF_DETECTED		0x00000002		/* Is it detected? */
#define NETIF_INITIALIZED	0x00000004		/* Has it been initialized? */
#define NETIF_RUNNING		0x00000008		/* Has start() been called? */
#define NETIF_PROMISC		0x00010000		/* Promiscuous mode */
#define NETIF_NEEDSPOLL		0x01000000		/* Needs to be polled for input */

/* Return types for if_tx */
#define NETIF_TX_OK		0
#define NETIF_TX_ERROR		-1
#define NETIF_TX_AGAIN		-2

/***** net_core.c *********************************************************/

// Net input callback: returns non-zero if we need to exit the current net
// polling loop (if any).
int net_input(struct knetif * dev, int pkt_size);

/* Init */
int net_init();

/* Shutdown */
void net_shutdown();

__END_DECLS

#endif	/* __KOS_NET_H */

