/**
 * Copyright (c) 2011-2014 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */

#include <kos.h>
#include "minilzo/minilzo.h"
#include "zlib/zlib.h"
#include "isofs/ciso.h"
#include "console.h"

//#define DEBUG 1

#define isCompressed(block) ((block & 0x80000000) == 0)
#define getPosition(block, align) ((block & 0x7FFFFFFF) << align)


static int check_cso_image(file_t fd) {
	uint32 magic = 0;
    
	fs_seek(fd, 0, SEEK_SET);
	fs_read(fd, &magic, sizeof(magic));
    
	if(magic != CISO_MAGIC_CISO && magic != CISO_MAGIC_ZISO) {
		return -1;
	}
    
	return 0;
}

CISO_header_t *ciso_open(file_t fd) {
    
	if(check_cso_image(fd) < 0) {
		return NULL;
	}
	
	int ret;
	uint32 *magic;
	CISO_header_t *hdr;

	hdr = (CISO_header_t*)malloc(sizeof(CISO_header_t));

	if(hdr == NULL) {
		return NULL;
	}

	memset_sh4(hdr, 0, sizeof(CISO_header_t));
	hdr->magic[0] = '\0';

	fs_seek(fd, 0, SEEK_SET);
	ret = fs_read(fd, hdr, sizeof(CISO_header_t));

	if(ret != sizeof(CISO_header_t)) {
		free(hdr);
		return NULL;
	}

	magic = (uint32*)hdr->magic;
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "Magic: %c%c%c%c\n", hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3]);
	dbglog(DBG_DEBUG, "Hdr size: %u\n", hdr->header_size);
	dbglog(DBG_DEBUG, "Total bytes: %u\n", hdr->total_bytes);
	dbglog(DBG_DEBUG, "Block size: %u\n", hdr->block_size);
	dbglog(DBG_DEBUG, "Version: %02x\n", hdr->ver);
	dbglog(DBG_DEBUG, "Align: %d\n", 1 << hdr->align);
#endif

	if(*magic == CISO_MAGIC_CISO) { 
		;
	} else if(*magic == CISO_MAGIC_ZISO) {

		if(lzo_init() != LZO_E_OK) {
			ds_printf("DS_ERROR: lzo_init() failed\n");
			free(hdr);
			hdr = NULL;
		}
		
	} else {
		free(hdr);
		hdr = NULL;
	}
	
	return hdr;
}


int ciso_close(CISO_header_t *hdr) {
	
	if(hdr != NULL) {
		free(hdr);
		return 0;
	}

	return -1;
}


int ciso_get_blocks(CISO_header_t *hdr, file_t fd, uint *blocks, uint32 sector, uint32 cnt) {
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: fd=%d seek=%u\n", __func__, fd, sizeof(CISO_header_t) + (sector * sizeof(uint)));
#endif

	int r = 0;
	//uint len = (uint)(hdr->total_bytes / hdr->block_size) + 1;
	fs_seek(fd, sizeof(CISO_header_t) + (sector * sizeof(uint)), SEEK_SET);
	r = fs_read(fd, blocks, sizeof(uint) * cnt);
	
#ifdef DEBUG
	int i;
	
	for(i = 0; i < cnt; i++) 
		dbglog(DBG_DEBUG, "   blocks[%d]=%08u\n", i, blocks[i]);
#endif
	
	return r;
}


static int ciso_read_sector(CISO_header_t *hdr, file_t fd, uint8 *buff, uint8 *buffc, uint32 sector) {
	
	uint32 *magic, dlen;
	uint blocks[2], start, len;
	z_stream zstream;
	
	zstream.zalloc = Z_NULL;
	zstream.zfree  = Z_NULL;
	zstream.opaque = Z_NULL;
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: fd=%d sector=%ld\n", __func__, fd, sector);
#endif
	
	ciso_get_blocks(hdr, fd, blocks, sector, 2);
	
	start = getPosition(blocks[0], hdr->align);
	len = getPosition(blocks[1], hdr->align) - start;
	
#ifdef DEBUG
	dbglog(DBG_DEBUG, "%s: fd=%d start=%ld len=%d\n", __func__, fd, start, len);
#endif
	
	fs_seek(fd, start, SEEK_SET);
	zstream.avail_in = fs_read(fd, buffc, len);

	if(len == zstream.avail_in && isCompressed(blocks[0])) {
		
		magic = (uint32*)hdr->magic;
		
		switch(*magic) {
			case CISO_MAGIC_CISO:
			
#ifdef DEBUG
				dbglog(DBG_DEBUG, "%s: decompress zlib\n", __func__);
#endif

				if(inflateInit2(&zstream, -15) != Z_OK) {
					ds_printf("DS_ERROR: zlib init error: %s\n", zstream.msg ? zstream.msg : "unknown");
					return -1;
				}

				zstream.next_out  = buff;
				zstream.avail_out = hdr->block_size;
				zstream.next_in   = buffc;

				if(inflate(&zstream, Z_FULL_FLUSH) != Z_STREAM_END) {
					ds_printf("DS_ERROR: zlib inflate error: %s\n", zstream.msg ? zstream.msg : "unknown");
					return -1;
				}

				inflateEnd(&zstream);
				break;
				
			case CISO_MAGIC_ZISO:
			
#ifdef DEBUG
				dbglog(DBG_DEBUG, "%s: decompress lzo\n", __func__);
#endif
				if(lzo1x_decompress(buffc, len, buff, &dlen, NULL) != LZO_E_OK) {
					ds_printf("DS_ERROR: lzo decompress error\n", __func__);
					return -1;
				}
				
				if(dlen != hdr->block_size) {
					return -1;
				}
				
				break;
			default:
				memcpy_sh4(buff, buffc, hdr->block_size);
				break;
		}
		
	} else if(len == zstream.avail_in) {
		memcpy_sh4(buff, buffc, hdr->block_size);
	} else {
		return -1;
	}
	
	return 0;
}


int ciso_read_sectors(CISO_header_t *hdr, file_t fd, uint8 *buff, uint32 start, uint32 count) {
	
	uint8 *buffc;
	buffc = (uint8*) malloc(hdr->block_size);

	if(buffc == NULL) {
		return -1;
	}
	
	while(count--) {
		if(ciso_read_sector(hdr, fd, buff, buffc, start++)) {
			return -1;
		}
		buff += hdr->block_size;
	}
	
	free(buffc);
	return 0;
}

