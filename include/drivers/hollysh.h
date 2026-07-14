
/** 
 * \file    hollysh.h
 * \brief   DreamShell HollySH BIOS firmware utilities
 * \date    2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef __DS_HOLLYSH_BIOS_H
#define __DS_HOLLYSH_BIOS_H

#include <stdint.h>

#define HOLLYSH_BIOS_MAGIC              "HOLLYSH"
#define HOLLYSH_BIOS_MAGIC_LEN          (sizeof(HOLLYSH_BIOS_MAGIC) - 1)

#define HOLLYSH_BIOS_MAGIC_ROM_OFFSET   0x400
#define HOLLYSH_BIOS_MAGIC_ROM_ADDR     ((const volatile uint8_t *)0xA0000400)

#define HOLLYSH_LOADER_ROM_OFFSET       0x00010000
#define HOLLYSH_BOOTLOADER_ROM_OFFSET   0x00020000
#define HOLLYSH_BIOS_ROMDISK_ROM_OFFSET 0x000A0000
#define HOLLYSH_BIOS_ROM_FONT_OFFSET    0x00100020

#define HOLLYSH_LOADER_ROM_SIZE         (HOLLYSH_BOOTLOADER_ROM_OFFSET - HOLLYSH_LOADER_ROM_OFFSET)
#define HOLLYSH_BOOTLOADER_ROM_SIZE     (HOLLYSH_BIOS_ROMDISK_ROM_OFFSET - HOLLYSH_BOOTLOADER_ROM_OFFSET)

#define HOLLYSH_LOADER_BIN_ADDR         0x8c004000
#define HOLLYSH_LOADER_BIN_END          (HOLLYSH_LOADER_BIN_ADDR + HOLLYSH_LOADER_ROM_SIZE)

/**
 * Detect if the BIOS is HollySH
 * \return BIOS version or 0 if not HollySH
 */
static inline uint32_t hollysh_bios_detect(void) {
	const volatile uint8_t *rom = HOLLYSH_BIOS_MAGIC_ROM_ADDR;
	uint32_t i;

	for(i = 0; i < HOLLYSH_BIOS_MAGIC_LEN; i++) {
		if(rom[i] != (uint8_t)HOLLYSH_BIOS_MAGIC[i]) {
			return 0;
		}
	}
	return *(volatile uint32_t *)(rom + HOLLYSH_BIOS_MAGIC_LEN + 1);
}

#endif
