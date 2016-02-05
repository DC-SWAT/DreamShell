/* KallistiOS ##version##

   net/icmp.c

   Copyright (C)2002,2004 Dan Potter

 */

#include <stdio.h>
#include <string.h>
#include <kos/net.h>
#include <net/net.h>

int net_icmp_input(netif_t *src, uint8 *pkt, int pktsize) {
	eth_hdr_t	*eth;
	ip_hdr_t	*ip;
	icmp_hdr_t	*icmp;
	int		i;
	uint8		tmp[6];
	
	(void)pktsize;
	
	/* Get pointers */
	eth = (eth_hdr_t*)(pkt + 0);
	ip = (ip_hdr_t*)(pkt + sizeof(eth_hdr_t));
	icmp = (icmp_hdr_t*)(pkt + sizeof(eth_hdr_t) + 4*(ip->version_ihl & 0x0f));
	
	/* Check icmp checksum */
	memset(net_txbuf, 0, ntohs(ip->length) + (ntohs(ip->length) % 2)
		- 4*(ip->version_ihl & 0x0f));
	i = icmp->checksum;
	icmp->checksum = 0;
	memcpy(net_txbuf, icmp, ntohs(ip->length) - 4*(ip->version_ihl & 0x0f));
	icmp->checksum = net_checksum((uint16*)net_txbuf,
		(ntohs(ip->length) + 1) / 2 - 2*(ip->version_ihl & 0x0f));
	if (i != icmp->checksum) {
		printf("net_icmp: icmp with invalid checksum\n");
		return 0;
	}

	if (icmp->type == 8) {	/* echo request */
		icmp->type = 0;	/* echo reply */

		/* Swap source and dest addresses */
		memcpy(tmp, eth->dest, 6);
		memcpy(eth->dest, eth->src, 6);
		memcpy(eth->src, tmp, 6);

		/* Swap source and dest ip addresses */
		memcpy(&i, &ip->src, 4);
		memcpy(&ip->src, &ip->dest, 4);
		memcpy(&ip->dest, &i, 4);

		/* Recompute the IP header checksum */
		ip->checksum = 0;
		ip->checksum = net_checksum((uint16*)ip, 2*(ip->version_ihl & 0x0f));

		/* Recompute the ICMP header checksum */
		icmp->checksum = 0;
		icmp->checksum = net_checksum((uint16*)icmp, ntohs(ip->length)/2 - 2*(ip->version_ihl & 0x0f));

		/* Send it */
		src->if_tx(src, pkt, sizeof(eth_hdr_t) + ntohs(ip->length));
	}

	return 0;
}


