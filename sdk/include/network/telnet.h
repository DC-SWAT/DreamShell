/** 
 * \file    telnet.h
 * \brief   Telnet server
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */ 

#ifndef _DS_TELNET_H
#define _DS_TELNET_H

#ifdef USE_LWIP
#include "lwip.h"
#include "sockets.h"
#else
#include <kos/net.h>
#include <sys/socket.h>
#endif


/* telnetd server */
int telnetd_init(int port);
void telnetd_shutdown();


#endif /* _DS_TELNET_H */