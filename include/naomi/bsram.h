/** \file    naomi/bsram.h
    \brief   NAOMI battery-backed SRAM access.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#ifndef __NAOMI_BSRAM_H
#define __NAOMI_BSRAM_H

#include <stddef.h>
#include <stdint.h>

#define NAOMI_BSRAM_BASE                    0xA0200000
#define NAOMI_BSRAM_SIZE                    0x8000
#define NAOMI_BSRAM_MIRROR_OFF              0x00F8
#define NAOMI_BSRAM_BOOKKEEPING_CRC_OFF     0x0008
#define NAOMI_BSRAM_BOOKKEEPING_CRC_LEN     0x00F0
#define NAOMI_BSRAM_VALID_OFF               0x000C
#define NAOMI_BSRAM_VALID                   1
#define NAOMI_BSRAM_CREDITS_OFF             0x0010
#define NAOMI_BSRAM_GAME_OFF                0x01F8

/** \brief  Read bytes from backup RAM. */
int naomi_bsram_read(uint32_t offset, void *buf, size_t len);

/** \brief  Write bytes to backup RAM. */
int naomi_bsram_write(uint32_t offset, const void *buf, size_t len);

/** \brief  Read the shared credit counter. */
uint32_t naomi_bsram_read_credits(void);

/** \brief  Store the shared credit counter and refresh its CRC. */
int naomi_bsram_write_credits(uint32_t credits);

/** \brief  Add credits to the shared counter. */
int naomi_bsram_add_credits(uint32_t delta);

/** \brief  Recompute the bookkeeping CRC and its mirror. */
int naomi_bsram_update_bookkeeping_crc(void);

/** \brief  Save the whole backup RAM window to a file. */
void naomi_bsram_save(const char *path);

/** \brief  Restore a game record from a file, leaving live coins in place. */
void naomi_bsram_restore(const char *path);

#endif
