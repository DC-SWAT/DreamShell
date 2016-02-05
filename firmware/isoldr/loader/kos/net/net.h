/*

   net/net.h
   Copyright (C)2004 Dan Potter

*/

#ifndef __NET_NET_H
#define __NET_NET_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/net.h>

/*

Thanks to AndrewK for these IP structures and the code which I studied to
create this (dc-load-ip).

*/

#define packed //__attribute__((packed))

// Prepended to all ethernet packets
typedef struct {
	uint8	dest[6]		packed;
	uint8	src[6]		packed;
	uint8	type[2]		packed;
} eth_hdr_t;

// Prepended to all IP packets
typedef struct {
	uint8	version_ihl	packed;
	uint8	tos		packed;
	uint16	length		packed;
	uint16	packet_id	packed;
	uint16	flags_frag_offs	packed;
	uint8	ttl		packed;
	uint8	protocol	packed;
	uint16	checksum	packed;
	uint32	src		packed;
	uint32	dest		packed;
} ip_hdr_t;

// Prepended to all ICMP packets
typedef struct {
	uint8	type		packed;
	uint8	code		packed;
	uint16	checksum	packed;
	uint32	misc		packed;
} icmp_hdr_t;

// ARP packet
typedef struct {
	uint16	hw_space	packed;
	uint16	proto_space	packed;
	uint8	hw_len		packed;
	uint8	proto_len	packed;
	uint16	opcode		packed;
	uint8	hw_src[6]	packed;
	uint8	proto_src[4]	packed;
	uint8	hw_dest[6]	packed;
	uint8	proto_dest[4]	packed;
} arp_pkt_t;

// UDP packet
typedef struct {
	uint16	src_port	packed;
	uint16	dest_port	packed;
	uint16	length		packed;
	uint16	checksum	packed;
	uint8	data[0]		packed;
} udp_pkt_t;

// UDP pseudo packet. This is used for checksum calculation.
typedef struct {
	uint32	src_ip		packed;
	uint32	dest_ip		packed;
	uint8	zero		packed;
	uint8	protocol	packed;
	uint16	udp_length	packed;
	uint16	src_port	packed;
	uint16	dest_port	packed;
	uint16	length		packed;
	uint16	checksum	packed;
	uint8	data[0]		packed;
} udp_pseudo_pkt_t;

// PC->DC and response packets
typedef struct {
	uint8	tag[4]		packed;
	uint32	addr		packed;
	uint32	size		packed;
	uint8	data[0]		packed;
} pkt_tas_t;

// DC->PC 3i packet
typedef struct {
	uint8	tag[4]		packed;
	uint32	value0		packed;
	uint32	value1		packed;
	uint32	value2		packed;
	uint32	value3		packed;		// serial for 3i
} pkt_3i_t;

// DC->PC 2is packet
typedef struct {
	uint8	tag[4]		packed;
	uint32	value0		packed;
	uint32	value1		packed;
	uint8	data[0]		packed;
} pkt_2is_t;

// DC->PC i packet
typedef struct {
	uint8	tag[4]		packed;
	uint32	value0		packed;
} pkt_i_t;

// DC->PC s packet
typedef struct {
	uint8	tag[4]		packed;
	uint8	data[0]		packed;
} pkt_s_t;

#undef packed

uint16 net_checksum(uint16 *data, int words);
uint16 ntohs(uint16 n);
uint32 ntohl(uint32 n);
#define htons ntohs
#define htonl ntohl

// Packet buffers all drivers can use
extern uint8 net_rxbuf[1514], net_txbuf[1514];

// Active network device
extern netif_t * nif;

// Our IP, if we know it
extern uint32 net_ip;

// Host PC's IP, MAC address, and port, if we know it.
extern uint32 net_host_ip;
extern uint16 net_host_port;
extern uint8 net_host_mac[6];

// When a command is initiated, this value is set to zero. When the command
// is completed, the value is set to non-zero. This is used in exclusive
// mode (to quit) or in immediate mode to exit back to the calling program.
extern volatile int net_exit_loop;

// When an RPC call is completed, this variable will hold the return
// value of the call.
extern volatile int net_rpc_ret;

// Network loop for immediate and exclusive modes. Returns when net_exit_loop
// is set to non-zero.
int net_loop();

// Packet input functions
int net_arp_input(netif_t * src, uint8 * pkt, int pktsize);
int net_icmp_input(netif_t * src, uint8 * pkt, int pktsize);
int net_udp_input(netif_t * src, uint8 * pkt, int pktsize);

// This helper function will create a response packet to the one sitting
// in net_rxbuf in net_txbuf and return a pointer to the UDP payload part
// of the packet.
uint8 * net_resp_build();

// Same as net_resp_build, but assumes there is no packet in net_rxbuf to
// model this one after. This requires that we have received at least one
// packet from the host PC.
uint8 * net_tx_build();

// This helper function completes the response by finishing the packet in
// net_txbuf (calc checksum, etc) and sending it out the ethernet driver.
void net_resp_complete(int size);

__END_DECLS

#endif	/* __NET_NET_H */

