/*

   slinkie/commands.h
   Copyright (C)2004 Dan Potter

*/

#ifndef __SLINKIE_COMMANDS_H
#define __SLINKIE_COMMANDS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <net/net.h>

// Command packet types
#define CMD_EXEC "EXEC"
#define CMD_LBIN "LBIN"
#define CMD_PBIN "PBIN"
#define CMD_DBIN "DBIN"
#define CMD_SBIN "SBIN"
#define CMD_SREG "SREG"
#define CMD_VERS "VERS"
#define CMD_RETV "RETV"
#define CMD_RBOT "RBOT"
#define CMD_PAUS "PAUS"
#define CMD_RSUM "RSUM"
#define CMD_TERM "TERM"
#define CMD_CDTO "CDTO"

// Command handlers
typedef void (*cmd_handler_t)(ip_hdr_t * ip, udp_pkt_t * udp);

#define CMD_HANDLER(TYPE) \
	void cmd_##TYPE(ip_hdr_t * ip, udp_pkt_t * udp)

#define CMD_HANDLER_NAME(TYPE) cmd_##TYPE

CMD_HANDLER(EXEC);
CMD_HANDLER(LBIN);
CMD_HANDLER(PBIN);
CMD_HANDLER(DBIN);
CMD_HANDLER(SBIN);
CMD_HANDLER(SREG);
CMD_HANDLER(VERS);
CMD_HANDLER(RETV);
CMD_HANDLER(RBOT);
CMD_HANDLER(PAUS);
CMD_HANDLER(RSUM);
CMD_HANDLER(TERM);
CMD_HANDLER(CDTO);



__END_DECLS

#endif	/* __SLINKIE_COMMANDS_H */

