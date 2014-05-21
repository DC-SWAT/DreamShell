/* 
	P A T C H . H
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	5 july 2006
	
	This file is here to patch the ripped libedc from cdrtools. 
	The goal of this patch is to allow the compilation in stand alone mode (without cdrtools libs).
*/
	
#ifndef __PATCH_H__
#define __PATCH_H__

#include <string.h>
#include <stdint.h>

#define __PR(a) a
#define	xaligned(a, s)		((((UIntptr_t)(a)) & s) == 0 )

#define UInt32_t uint32_t

typedef	unsigned long UIntptr_t;
typedef	unsigned char Uchar;

#endif //__PATCH_H__
