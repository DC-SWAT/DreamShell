/* KallistiOS ##version##

   net/arp.c

   Copyright (C)2004 Dan Potter

 */

#include <stdio.h>
#include <string.h>
#include <kos/net.h>
#include <net/net.h>

int net_arp_input(netif_t *src, uint8 *pkt, int pktsize) {
	eth_hdr_t	*eth;
	arp_pkt_t	*arp;
	uint8		tmp[10];
	uint32		ip;
	
	// Do we know our IP?
	if (net_ip == 0)
		return 0;
	
	(void)pktsize;

	// Get pointers
	eth = (eth_hdr_t*)(pkt + 0);
	arp = (arp_pkt_t *)(pkt + sizeof(eth_hdr_t));

	// Make sure the hardware addr space = ethernet
	if (arp->hw_space != 0x0100) {
		printf("net_arp: bad hw space\n");
		return 0;
	}

	// Make sure the protocol addr space = IP
	if (arp->proto_space != 0x0008) {
		printf("net_arp: bad proto space\n");
		return 0;
	}

	// Is it a request?
	if (arp->opcode != 0x0100) {
		printf("net_arp: ARP not request\n");
		return 0;
	}

	// Is it for our IP?
	ip = htonl(net_ip);
	if (memcmp(arp->proto_dest, &ip, 4)) {
		printf("net_arp: request for someone else\n");
		return 0;
	}

	// Swap the MAC addresses
	memcpy(eth->dest, eth->src, 6);
	memcpy(eth->src, nif->mac_addr, 6);

	// ARP reply
	arp->opcode = 0x0200;

	// Swap the IP addresses
	memcpy(tmp, arp->hw_src, 10);
	memcpy(arp->hw_src, arp->hw_dest, 10);
	memcpy(arp->hw_dest, tmp, 10);

	// Put our MAC in the src address field
	memcpy(arp->hw_src, nif->mac_addr, 6);

	// Send it
	src->if_tx(src, net_rxbuf, sizeof(eth_hdr_t) + sizeof(arp_pkt_t));

	return 0;
}


