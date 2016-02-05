/* KallistiOS ##version##

   slinkie/commands.c

   Copyright (C)2004 Dan Potter

 */

#include "main.h"
#include <kos/net.h>
#include <net/net.h>
#include "include/commands.h"

extern int sl_mode;


CMD_HANDLER(EXEC) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;
	//uint32 addr = ntohl(req->addr);

	// Send an identical EXEC packet to ACK.
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp, req, sizeof(pkt_tas_t));
	net_resp_complete(sizeof(pkt_tas_t));

	//sl_exec_addr = addr;
	sl_mode = SLMODE_COOP;
	net_exit_loop = 1;
}

// We have to keep track of this info across multiple packets so we can
// successfully complete an LBIN/PBIN^x/DBIN set.
struct {
	uint8	* dst;		// Address we're loading to
	uint32	size;		// Amount of data we're reciving
	uint8	map[2048];	// Bit mask of received blocks
} lbin_info;

// Clear / init everything to defaults
static void lbin_init(uint8 * ptr, uint32 size) {
	// Set our pointers
	lbin_info.dst = ptr;
	lbin_info.size = size;

	// Clear out the relevant part of the map
	size = (size + 8191) & ~8191;
	memset(lbin_info.map, 0, size / (8*1024));
}

// Mark the block at ptr as received
static void lbin_compl(uint8 * ptr) {
	// Where in the map is the current block?
	uint32 blid = ((uint32)(ptr - lbin_info.dst)) / 1024;

	// Set it received
	lbin_info.map[blid / 8] |= 1 << (blid % 8);
}

// Returns non-zero if the block at ptr is marked as received
static int lbin_is_compl(uint8 * ptr) {
	// Where in the map is the current block?
	uint32 blid = ((uint32)(ptr - lbin_info.dst)) / 1024;

	return lbin_info.map[blid / 8] & (1 << (blid % 8));
}

// Starts at the load pointer origin and searches for the next
// block which still needs to be loaded. If we find one, we return
// non-zero and set the values in *ptr and *size.
static int lbin_find_hole(uint8 ** ptr, uint32 * size) {
	uint8 * dst;
	uint8 * dstend = lbin_info.dst + lbin_info.size;
	uint32 left;

	for (dst = lbin_info.dst; dst < dstend; dst += 1024) {
		if (!lbin_is_compl(dst)) {
			// Found a hole.. figure out how big it is.
			left = dstend - dst;

			// Send back the bad news.
			*ptr = dst;
			*size = left > 1024 ? 1024 : left;
			return 1;
		}
	}

	// No holes!
	return 0;
}

CMD_HANDLER(LBIN) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;

	// Get all our pointers setup.
	uint8	* dst = (uint8 *)(ntohl(req->addr));
	uint32	cnt = ntohl(req->size);

	//printf("LBIN: dst %p, cnt %d\n", dst, cnt);

	// Init our receive map
	lbin_init(dst, cnt);

	// Send back a copy of the LBIN as an ACK.
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp, req, sizeof(pkt_tas_t));
	net_resp_complete(sizeof(pkt_tas_t));
}

CMD_HANDLER(PBIN) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;

	// Get all our pointers setup.
	uint8	* dst = (uint8 *)(ntohl(req->addr));
	uint32	cnt = ntohl(req->size);

	//printf("PBIN: dst %p, cnt %d\n", dst, cnt);

	// Copy the block out.
	memcpy(dst, req->data, cnt);

	// Set this block as received.
	lbin_compl(dst);
}

CMD_HANDLER(DBIN) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;

	uint8	* hole;
	uint32	hole_size;

	// Build the response packet.
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp, req, sizeof(pkt_tas_t));

	// Did we miss anything?
	if (lbin_find_hole(&hole, &hole_size)) {
		//printf("DBIN: need %p/%u\n", hole, hole_size);

		// Yes. Re-request it.
		rsp->addr = htonl((uint32)hole);
		rsp->size = htonl(hole_size);
	} else {
		//printf("DBIN: all fine\n");

		// Nope. ACK it all.
		rsp->addr = rsp->size = 0;
	}

	// Send the ACK/NACK.
	net_resp_complete(sizeof(pkt_tas_t));
}

CMD_HANDLER(SBIN) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;

	// Get all our pointers setup.
	uint8	* src = (uint8 *)(ntohl(req->addr));
	uint32	cnt = ntohl(req->size), now;

	//printf("SBIN: src %p, cnt %d\n", src, cnt);
	if ((ptr_t)src < 0x8c010000 || (ptr_t)src > 0x8d000000 || cnt > 16*1024*1024) {
		//printf("invalid request ignored!\n");
		return;
	}

	// Build a skeleton response packet
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp->tag, CMD_SBIN, 4);

	// Send everything one packet at a time...
	while (cnt > 0) {
		// Update counters
		now = cnt > 1024 ? 1024 : cnt;

		rsp->addr = htonl((uint32)src);
		rsp->size = htonl(now);
		memcpy(rsp->data, src, now);
		net_resp_complete(sizeof(pkt_tas_t) + now);

		src += now;		
		cnt -= now;
	}

	// Send a DBIN packet to signal completion
	memcpy(rsp->tag, CMD_DBIN, 4);
	rsp->addr = 0;
	rsp->size = 0;
	net_resp_complete(sizeof(pkt_tas_t));
}

CMD_HANDLER(SREG) {
}

CMD_HANDLER(VERS) {
	//pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;

	// Send back a VERS command with the appropriate info.
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp->tag, CMD_VERS, 4);
	rsp->addr = 0;
	rsp->size = 0;
	strcpy(rsp->data, VERSION);
	net_resp_complete(sizeof(pkt_tas_t) + strlen(VERSION) + 1);
}

CMD_HANDLER(RETV) {
	pkt_tas_t * req = (pkt_tas_t *)udp->data;
	pkt_tas_t * rsp;

	// Get the ret value
	net_rpc_ret = ntohl(req->addr);

	// Send an identical RETV packet to ACK.
	rsp = (pkt_tas_t *)net_resp_build();
	memcpy(rsp, req, sizeof(pkt_tas_t));
	net_resp_complete(sizeof(pkt_tas_t));

	// We're done with the RPC.
	if (sl_mode == SLMODE_IMME)
		net_exit_loop = 1;
}

CMD_HANDLER(RBOT) {

	//int needquit = 0;

	//printf("exit requested\n");
	net_exit_loop = 1;
	//if (sl_mode == SLMODE_COOP)
		//needquit = 1;
	sl_mode = SLMODE_NONE;
	//if (needquit)
		//exec_exit();

}

CMD_HANDLER(PAUS) {
}

CMD_HANDLER(RSUM) {
}

CMD_HANDLER(TERM) {
}

CMD_HANDLER(CDTO) {
}

