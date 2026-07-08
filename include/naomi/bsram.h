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
#define NAOMI_BSRAM_CREDITS_OFF             0x0010

int naomi_bsram_read(uint32_t offset, void *buf, size_t len);
int naomi_bsram_write(uint32_t offset, const void *buf, size_t len);
uint32_t naomi_bsram_read_credits(void);
int naomi_bsram_write_credits(uint32_t credits);
int naomi_bsram_add_credits(uint32_t delta);
int naomi_bsram_update_bookkeeping_crc(void);

#endif
