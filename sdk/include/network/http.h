/** 
 * \file    http.h
 * \brief   HTTP protocol
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_HTTP_H
#define _DS_HTTP_H

#include <kos/net.h>
#include <sys/socket.h>

/* File systems */
int tcpfs_init();
void tcpfs_shutdown();

int httpfs_init();
void httpfs_shutdown();


/* DSN utils */
int dns_init(const uint8 *ip);
int dns(const char *name, struct in_addr* addr);
int ds_gethostbyname(const struct sockaddr_in * dnssrv, const char *name, uint8 *ipout);


/* URL utils */
void _url_split(char *proto, int proto_size,
               char *hostname, int hostname_size,
               int *port_ptr,
               char *path, int path_size,
               const char *url);
			   

/* httpd server */
int httpd_init(int port);
void httpd_shutdown();


#endif /* _DS_HTTP_H */