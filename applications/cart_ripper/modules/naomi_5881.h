#ifndef _NAOMI_5881_H
#define _NAOMI_5881_H

#include <stdint.h>

#define NAOMI_M2_KEY_BAL1     0x0008ad01
#define NAOMI_M2_ROM_SIZE     0x400000
#define NAOMI_M2_OVERLAY_OFF  0x400000

void naomi_5881_set_src(const uint8_t *rom, uint32_t size);
void naomi_5881_decrypt(void *dst, uint32_t size, uint32_t word_addr,
	uint16_t subkey, uint32_t key);

#endif
