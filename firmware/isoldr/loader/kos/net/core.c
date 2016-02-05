/* KallistiOS ##version##

   kos/net/core.c
   Copyright (C)2002,2004 Dan Potter
   Copyright (C)2010-2014 SWAT
*/

#include <main.h>
#include <net/net.h>
#include <dc/net/broadband_adapter.h>
#include <dc/net/lan_adapter.h>

/*

This module, and others in this tree, handle the architecture-independent
part of the networking system of KOS. They serve the following purposes:

- Specific network card drivers may register themselves using the functions
  here, if their hardware is present
- Link-level messages will be handled here, such as ARP
- The whole networking system can be enabled or disabled from here
 
*/

// This static table is for Slinkie.
static netif_t * netifs[] = {
	&bba_if,
	&la_if,
	NULL
};

netif_t * nif = NULL;

// Static packet rx/tx buffers, to save space.
uint8 net_rxbuf[1514], net_txbuf[1514];

// Our IP address, if we know it.
uint32 net_ip = 0;

// Host PC's IP, MAC address, and port, if we know it.
uint32 net_host_ip = 0;
uint16 net_host_port = 0;
uint8 net_host_mac[6] = {0};

volatile int net_exit_loop = 0;

volatile int net_rpc_ret = 0;

int net_bcast_input(netif_t * drv, uint8 * pkt, int size) {
	eth_hdr_t	* eth;
	eth = (eth_hdr_t *)(net_rxbuf + 0);
	(void)pkt;

	DBG("bcast packet\n");

	// Is it an ARP packet?
	if (eth->type[1] == 0x06) {
		DBG("arp packet\n");
		return net_arp_input(drv, net_rxbuf, size);
	}

	return 0;
}

int net_to_me_input(netif_t * drv, uint8 * pkt, int size) {
	eth_hdr_t	* eth;
	ip_hdr_t	* ip;
	int		i;

	eth = (eth_hdr_t *)(pkt + 0);
	ip = (ip_hdr_t *)(pkt + sizeof(eth_hdr_t));

	// Ignore non-IPv4 packets
	if (eth->type[1] != 0x00)
		return 0;

	// Ignore fragments
	if (ntohs(ip->flags_frag_offs) & 0x3fff) {
		DBG("net_to_me_input: fragmented packet\n");
		return 0;
	}

	// Check the IP checksum
	i = ip->checksum;
	ip->checksum = 0;
	ip->checksum = net_checksum((uint16 *)ip, 2*(ip->version_ihl & 0x0f));
	if (i != ip->checksum) {
		DBG("net_to_me_input: ip with invalid checksum\n");
		return 0;
	}

	// What protocol are we looking at?
	switch (ip->protocol) {
	case 1:		// ICMP
		DBG("icmp packet\n");
		return net_icmp_input(drv, net_rxbuf, size);
	case 17:	// UDP
		DBG("udp packet\n");
		return net_udp_input(drv, net_rxbuf, size);
	}

	return 0;
}

static uint8 bcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int net_input(struct knetif * drv, int size) {
	eth_hdr_t	* eth;
	eth = (eth_hdr_t *)(net_rxbuf + 0);

	// Ignore non-IP packets.
	if (eth->type[0] != 0x08) {
		DBG("ignoring non-IP packet\n");
		return 0;
	}

	// Is it a broadcast packet?
	if (!memcmp(eth->dest, bcast, 6))
		return net_bcast_input(drv, net_rxbuf, size);

	return net_to_me_input(drv, net_rxbuf, size);
}

/* Perform an IP-style checksum on a block of data */
uint16 net_checksum(uint16 *data, int words) {
	uint32 sum;
	int i;

	sum = 0;
	for (i=0; i<words; i++) {
		sum += data[i];
		if (sum & 0xffff0000) {
			sum &= 0xffff;
			sum++;
		}
	}

	return ~(sum & 0xffff);
}

uint8 * net_resp_build() {
	eth_hdr_t	* srceth, * dsteth;
	ip_hdr_t	* srcip, * dstip;
	udp_pkt_t	* srcudp, * dstudp;
	
	// Copy over the ethernet header, swapping the src/dest.
	srceth = (eth_hdr_t *)(net_rxbuf + 0);
	dsteth = (eth_hdr_t *)(net_txbuf + 0);
	memcpy(dsteth->dest, srceth->src, 6);
	memcpy(dsteth->src, srceth->dest, 6);
	memcpy(dsteth->type, srceth->type, 2);

	// Copy over the IP header, swapping the src/dest.
	srcip = (ip_hdr_t *)(net_rxbuf + sizeof(eth_hdr_t));
	dstip = (ip_hdr_t *)(net_txbuf + sizeof(eth_hdr_t));
	dstip->version_ihl = 0x45;
	dstip->tos = 0;
	dstip->length = 0;
	dstip->packet_id = 0;
	dstip->flags_frag_offs = htons(0x4000);
	dstip->ttl = 0x40;
	dstip->protocol = srcip->protocol;
	dstip->checksum = 0;
	dstip->src = srcip->dest;
	dstip->dest = srcip->src;

	// Copy over the UDP header, swapping the src/dest.
	srcudp = (udp_pkt_t *)(net_rxbuf + sizeof(eth_hdr_t) + 4*(srcip->version_ihl & 0x0f));
	dstudp = (udp_pkt_t *)(net_txbuf + sizeof(eth_hdr_t) + 4*(dstip->version_ihl & 0x0f));
	dstudp->src_port = srcudp->dest_port;
	dstudp->dest_port = srcudp->src_port;
	dstudp->checksum = 0;

	return dstudp->data;
}

uint8 * net_tx_build() {
	eth_hdr_t	* dsteth;
	ip_hdr_t	* dstip;
	udp_pkt_t	* dstudp;
	
	// Build the ethernet header
	dsteth = (eth_hdr_t *)(net_txbuf + 0);
	memcpy(dsteth->dest, net_host_mac, 6);
	memcpy(dsteth->src, nif->mac_addr, 6);
	dsteth->type[0] = 8;
	dsteth->type[1] = 0;

	// Build the IP header
	dstip = (ip_hdr_t *)(net_txbuf + sizeof(eth_hdr_t));
	dstip->version_ihl = 0x45;
	dstip->tos = 0;
	dstip->length = 0;
	dstip->packet_id = 0;
	dstip->flags_frag_offs = htons(0x4000);
	dstip->ttl = 0x40;
	dstip->protocol = 17;
	dstip->checksum = 0;
	dstip->src = htonl(net_ip);
	dstip->dest = htonl(net_host_ip);

	// Build the UDP header
	dstudp = (udp_pkt_t *)(net_txbuf + sizeof(eth_hdr_t) + 4*(dstip->version_ihl & 0x0f));
	dstudp->src_port = htons(31313);
	dstudp->dest_port = htons(net_host_port);
	dstudp->checksum = 0;

	return dstudp->data;
}

void net_resp_complete(int length) {
	ip_hdr_t	* ip;
	udp_pkt_t	* udp;
	udp_pseudo_pkt_t * pseudo;

	// Get pointers
	ip = (ip_hdr_t *)(net_txbuf + sizeof(eth_hdr_t));
	udp = (udp_pkt_t *)(net_txbuf + sizeof(eth_hdr_t) + 4*(ip->version_ihl & 0x0f));

	// Finish up length fields
	ip->length = htons(sizeof(ip_hdr_t) + sizeof(udp_pkt_t) + length);
	udp->length = htons(sizeof(udp_pkt_t) + length);

	// RX should be free now, so we'll work in there. Copy stuff over
	// for the UDP pseudo packet.
	pseudo = (udp_pseudo_pkt_t *)net_rxbuf;
	pseudo->src_ip = ip->src;
	pseudo->dest_ip = ip->dest;
	pseudo->zero = 0;
	pseudo->protocol = ip->protocol;
	pseudo->udp_length = udp->length;
	pseudo->src_port = udp->src_port;
	pseudo->dest_port = udp->dest_port;
	pseudo->length = udp->length;
	pseudo->checksum = 0;
	memcpy(pseudo->data, udp->data, length);
	pseudo->data[length] = 0;

	// Do the UDP checksum.
	udp->checksum = net_checksum((uint16 *)pseudo,
		(sizeof(udp_pseudo_pkt_t) + length + 1) / 2);
	if (udp->checksum == 0)
		udp->checksum = 0xffff;

	// Now do the IP checksum as well.
	ip->checksum = 0;
	ip->checksum = net_checksum((uint16 *)ip,
		2 * (ip->version_ihl & 0x0f));

	// Transmit the full packet.
	nif->if_tx(nif, net_txbuf,
		sizeof(eth_hdr_t) +
		sizeof(ip_hdr_t) +
		sizeof(udp_pkt_t) +
		length);
}

uint16 ntohs(uint16 n) {
	return ((n & 0xff) << 8) | ((n >> 8) & 0xff);
}

uint32 ntohl(uint32 n) {
	return (((n >> 0) & 0xff) << 24) |
		(((n >> 8) & 0xff) << 16) |
		(((n >> 16) & 0xff) << 8) |
		(((n >> 24) & 0xff) << 0);
}

int net_loop() {
	net_exit_loop = 0;
	while (!net_exit_loop) {
		nif->if_rx_poll(nif);
	}

	return 0;
}

// Thanks to dcload-ip for this super-simple parse algorithm.
/*
void net_setup_ip() {
	int i, c;
	uint8 * ip = (uint8 *)&net_ip;

	for (net_ip=0,i=3,c=0; DREAMCAST_IP[c]; c++) {
		if (DREAMCAST_IP[c] != '.') {
			ip[i] *= 10;
			ip[i] += DREAMCAST_IP[c] - '0';
		} else
			i--;
	}

	//DBG("our IP is (%s) %08lx\n", DREAMCAST_IP, net_ip);
}*/

#include "main.h"


static void _setup_ip(const char *cip, uint8 * ip) {
	int i, c;

	for (net_ip=0,i=3,c=0; cip[c]; c++) {
		if (cip[c] != '.') {
			ip[i] *= 10;
			ip[i] += cip[c] - '0';
		} else
			i--;
	}
}


void net_setup_ip() {
	/*
	int i, c;
	uint8 * ip = (uint8 *)&net_ip;

	for (net_ip=0,i=3,c=0; IsoInfo.ip[c]; c++) {
		if (IsoInfo.ip[c] != '.') {
			ip[i] *= 10;
			ip[i] += IsoInfo.ip[c] - '0';
		} else
			i--;
	}*/
	
	uint8 * ip = (uint8 *)&net_ip;
	uint8 * host = (uint8 *)&net_host_ip;
	
	_setup_ip("192.168.1.110", ip);
	_setup_ip("192.168.1.100", host);
	net_host_port = 31313;

	DBG("Our IP is %08lx", net_ip);
	DBG("Host IP is %08lx", net_host_ip);
}


/* Device detect / init */
int net_init() {
	int i;

	// If we know our IP then set it up.
	net_setup_ip();

	bba_init();
	la_init();

	for (i=0; netifs[i]; i++) {
		nif = netifs[i];

		/* Make sure we have one */
		if (nif->if_detect(nif) < 0) {
			DBG("'%s' failed detect\n", nif->descr);
			continue;
		}

		/* Ok, we do -- initialize it */
		if (nif->if_init(nif) < 0) {
			DBG("'%s' failed init\n", nif->descr);
			continue;
		}

		/* It's initialized, so now enable it */
		if (nif->if_start(nif) < 0) {
			DBG("'%s' failed start\n", nif->descr);
			nif->if_shutdown(nif);
			continue;
		}

		return 0;
	}

	nif = NULL;
	return -1;
}
