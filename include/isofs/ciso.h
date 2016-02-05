/**
 * Copyright (c) 2011-2014 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ISOFS_CISO_H
#define _ISOFS_CISO_H

#include <arch/types.h>

/**
 * \file
 * CISO/ZISO support for isofs
 *
 * \author SWAT
 */


/* zlib compress */
#define CISO_MAGIC_CISO	0x4F534943 

/* lzo compress */
#define CISO_MAGIC_ZISO 0x4F53495A


typedef struct CISO_header {
	uint8 magic[4]; /* +00 : C’,'I’,'S’,'O’ */
	uint32 header_size;
	uint64 total_bytes;
	uint32 block_size;
	uint8 ver;
	uint8 align;
	uint8 rsv_06[2];
} CISO_header_t;


CISO_header_t *ciso_open(file_t fd);
int ciso_close(CISO_header_t *hdr);

int ciso_get_blocks(CISO_header_t *hdr, file_t fd, uint *blocks, uint32 sector, uint32 count);
int ciso_read_sectors(CISO_header_t *hdr, file_t fd, uint8 *buff, uint32 start, uint32 count);


#endif /* _ISOFS_CISO_H */
