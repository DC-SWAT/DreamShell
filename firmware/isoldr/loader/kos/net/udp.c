/* KallistiOS ##version##

   net/udp.c

   Copyright (C)2004 Dan Potter

 */

#include <main.h>
#include "../../fs/net/include/commands.h"
#include <kos/net.h>
#include <net/net.h>


// Table of command handler functions
#define D(TYPE) { CMD_##TYPE, CMD_HANDLER_NAME(TYPE) }
struct cmd_handler_item {
	const char * tag;
	cmd_handler_t hnd;
} cmd_handlers[] = {
	D(EXEC),
	D(LBIN),
	D(PBIN),
	D(SBIN),
	{ "SBIQ", CMD_HANDLER_NAME(SBIN) },	// deprecated
	D(DBIN),
	D(SREG),
	D(VERS),
	D(RETV),
	D(RBOT),
	D(PAUS),
	D(RSUM),
	D(TERM),
	D(CDTO),
	{ NULL, NULL }
};

void net_exec(ip_hdr_t * ip, udp_pkt_t * udp) {
	int i;

	for (i=0; cmd_handlers[i].tag; i++) {
		if (!memcmp(udp->data, cmd_handlers[i].tag, 4)) {
			cmd_handlers[i].hnd(ip, udp);
			return;
		}
	}
}


int net_udp_input(netif_t *src, uint8 *pkt, int pktsize) {
	// Find our packet pieces.
	ip_hdr_t * ip = (ip_hdr_t *)(pkt + sizeof(eth_hdr_t));
	udp_pkt_t * udp = (udp_pkt_t *)(pkt + sizeof(eth_hdr_t)
		+ 4 * (ip->version_ihl & 0x0f));
	(void)src;
	(void)pktsize;

	// If the checksum is zero, it means there's no checksum.
	if (udp->checksum != 0) {
		// We have to build this "pseudo packet" to check the UDP checksum.
		int i;
		udp_pseudo_pkt_t * pseudo = (udp_pseudo_pkt_t *)(net_txbuf);
		pseudo->src_ip = ip->src;
		pseudo->dest_ip = ip->dest;
		pseudo->zero = 0;
		pseudo->protocol = ip->protocol;
		pseudo->udp_length = udp->length;
		pseudo->src_port = udp->src_port;
		pseudo->dest_port = udp->dest_port;
		pseudo->length = udp->length;
		pseudo->checksum = 0;
		memset(pseudo->data, 0, (ntohs(udp->length) - sizeof(udp_pkt_t))
			+ (ntohs(udp->length) % 2));
		memcpy(pseudo->data, udp->data, ntohs(udp->length) - sizeof(udp_pkt_t));

		i = net_checksum((uint16 *)pseudo,
			( sizeof(udp_pseudo_pkt_t)
			+ ntohs(udp->length) - sizeof(udp_pkt_t) ) / 2);

		// A stored checksum of 0xffff means it was really zero.
		if (udp->checksum == 0xffff)
			udp->checksum = 0;

		// Check it.
		if (i != udp->checksum) {
			//printf("bad udp checksum: %02x vs %02\n", i, udp->checksum);
			return 0;
		}
	}

	// Does it belong to us?
	if (ntohs(udp->dest_port) != 31313)
		return 0;

	// Note the host info
	net_host_ip = ntohl(ip->src);
	net_host_port = ntohs(udp->src_port);
	memcpy(net_host_mac, pkt + 6, 6);

	/*printf("received good udp packet: %c%c%c%c\n",
		udp->data[0], udp->data[1], udp->data[2], udp->data[3]);*/

	// Find the handler and pass it off.
	net_exec(ip, udp);

	return 0;
}


