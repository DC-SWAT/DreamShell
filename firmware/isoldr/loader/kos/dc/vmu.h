/* This file is part of the libdream Dreamcast function library.
 * Please see libdream.c for further details.
 *
 * (c)2000 Jordan DeLong
 
   Ported from KallistiOS (Dreamcast OS) for libdream by Dan Potter

 */

#include <sys/cdefs.h>
#include <arch/types.h>


#ifndef __VMU_H
#define __VMU_H

int vmu_draw_lcd(uint8 addr, void *bitmap);
int vmu_block_read(uint8 addr, uint16 blocknum, uint8 *buffer);
int vmu_block_write(uint8 addr, uint16 blocknum, uint8 *buffer);

#endif
