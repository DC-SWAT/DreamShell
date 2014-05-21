/* DreamShell ##version##

   netstat.c
   Copyright (C) 2011-2013 SWAT
*/

#include "ds.h"


int main(int argc, char *argv[]) {
	net_ipv4_stats_t ip;
	net_udp_stats_t udp;

	/* Print out some statistics about the connection. */
	ip = net_ipv4_get_stats();
	udp = net_udp_get_stats();

	ds_printf("IPv4 Stats:\n"
	       "Packets sent successfully:       %6d\n"
	       "Packets that failed to send:     %6d\n"
	       "Packets received successfully:   %6d\n"
	       "Packets rejected (bad size):     %6d\n"
	       "                 (bad checksum): %6d\n"
	       "                 (bad protocol): %6d\n\n",
	       ip.pkt_sent, ip.pkt_send_failed, ip.pkt_recv, ip.pkt_recv_bad_size,
	       ip.pkt_recv_bad_chksum, ip.pkt_recv_bad_proto);

	ds_printf("UDP Stats:\n"
	       "Packets sent successfully:       %6d\n"
	       "Packets that failed to send:     %6d\n"
	       "Packets received successfully:   %6d\n"
	       "Packets rejected (bad size):     %6d\n"
	       "                 (bad checksum): %6d\n"
	       "                 (no socket):    %6d\n\n",
	       udp.pkt_sent, udp.pkt_send_failed, udp.pkt_recv,
	       udp.pkt_recv_bad_size, udp.pkt_recv_bad_chksum,
	       udp.pkt_recv_no_sock);

	return CMD_OK;
}


