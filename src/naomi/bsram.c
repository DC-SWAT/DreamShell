/** \file    naomi/bsram.c
    \brief   NAOMI battery-backed SRAM access.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#include <assert.h>
#include <arch/arch.h>
#include <dc/maple/mie.h>

#include <naomi/bsram.h>

static int naomi_bsram_available(void) {
    return hardware_sys_mode(NULL) != HW_TYPE_RETAIL;
}

static int naomi_bsram_bounds(uint32_t offset, size_t len) {
    if(len == 0 || offset >= NAOMI_BSRAM_SIZE ||
            len > NAOMI_BSRAM_SIZE - offset) {
        return 0;
    }

    return 1;
}

static uint32_t naomi_bsram_read_u32(const volatile uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off]
        | ((uint32_t)ram[off + 1] << 8)
        | ((uint32_t)ram[off + 2] << 16)
        | ((uint32_t)ram[off + 3] << 24);
}

static void naomi_bsram_write_u32(volatile uint8_t *ram, uint32_t off, uint32_t val) {
    ram[off] = (uint8_t)(val);
    ram[off + 1] = (uint8_t)(val >> 8);
    ram[off + 2] = (uint8_t)(val >> 16);
    ram[off + 3] = (uint8_t)(val >> 24);
}

int naomi_bsram_read(uint32_t offset, void *buf, size_t len) {
    volatile uint8_t *ram;
    uint8_t *dst;
    size_t i;

    assert(buf);

    if(!naomi_bsram_bounds(offset, len) || !naomi_bsram_available()) {
        return 0;
    }

    ram = (volatile uint8_t *)NAOMI_BSRAM_BASE;
    dst = (uint8_t *)buf;
    for(i = 0; i < len; i++) {
        dst[i] = ram[offset + i];
    }
    return 1;
}

int naomi_bsram_write(uint32_t offset, const void *buf, size_t len) {
    volatile uint8_t *ram;
    const uint8_t *src;
    size_t i;

    assert(buf);

    if(!naomi_bsram_bounds(offset, len) || !naomi_bsram_available()) {
        return 0;
    }

    ram = (volatile uint8_t *)NAOMI_BSRAM_BASE;
    src = (const uint8_t *)buf;
    for(i = 0; i < len; i++) {
        ram[offset + i] = src[i];
    }
    return 1;
}

uint32_t naomi_bsram_read_credits(void) {
    const volatile uint8_t *ram;

    if(!naomi_bsram_available()) {
        return 0;
    }

    ram = (volatile uint8_t *)NAOMI_BSRAM_BASE;
    return naomi_bsram_read_u32(ram, NAOMI_BSRAM_CREDITS_OFF);
}

int naomi_bsram_write_credits(uint32_t credits) {
    volatile uint8_t *ram;
    uint32_t mirror_off;

    if(!naomi_bsram_available()) {
        return 0;
    }

    ram = (volatile uint8_t *)NAOMI_BSRAM_BASE;
    naomi_bsram_write_u32(ram, NAOMI_BSRAM_CREDITS_OFF, credits);
    mirror_off = NAOMI_BSRAM_CREDITS_OFF + NAOMI_BSRAM_MIRROR_OFF;
    naomi_bsram_write_u32(ram, mirror_off, credits);
    return naomi_bsram_update_bookkeeping_crc();
}

int naomi_bsram_add_credits(uint32_t delta) {
    uint32_t credits;

    if(delta == 0) {
        return 0;
    }

    credits = naomi_bsram_read_credits() + delta;
    return naomi_bsram_write_credits(credits);
}

int naomi_bsram_update_bookkeeping_crc(void) {
    volatile uint8_t *ram;
    uint8_t block[NAOMI_BSRAM_BOOKKEEPING_CRC_LEN];
    uint16_t crc;
    uint32_t i;

    if(!naomi_bsram_available()) {
        return 0;
    }

    ram = (volatile uint8_t *)NAOMI_BSRAM_BASE;
    for(i = 0; i < NAOMI_BSRAM_BOOKKEEPING_CRC_LEN; i++) {
        block[i] = ram[NAOMI_BSRAM_CREDITS_OFF + i];
    }

    crc = mie_eeprom_crc16(block, sizeof(block));
    for(i = 0; i < 4; i++) {
        ram[NAOMI_BSRAM_BOOKKEEPING_CRC_OFF + i] = (uint8_t)((uint32_t)crc >> (i * 8));
        ram[NAOMI_BSRAM_BOOKKEEPING_CRC_OFF + NAOMI_BSRAM_MIRROR_OFF + i] =
            (uint8_t)((uint32_t)crc >> (i * 8));
    }
    return 1;
}
