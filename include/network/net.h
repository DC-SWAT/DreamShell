/** 
 * \file    net.h
 * \brief   Network utils
 * \date    2007-2023
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_NET_H
#define _DS_NET_H

#include <sys/cdefs.h>
#include <arch/types.h>

/**
 * Initialize and shutdown DreamShell network
 */
int InitNet(uint32 ipl);
void ShutdownNet();


/**
 * Utils
 */
#define READIP(dst, src) \
        dst = ((src[0]) << 24) | \
                ((src[1]) << 16) | \
                ((src[2]) << 8) | \
                ((src[3]) << 0)

#define SETIP(target, src) \
	IP4_ADDR(target, \
		(src >> 24) & 0xff, \
		(src >> 16) & 0xff, \
		(src >> 8) & 0xff, \
		(src >> 0) & 0xff)

#endif /* _DS_NET_H */
