/**
 * DreamShell MIE firmware
 * Memory copy and clear
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"

void mie_zero(uint8_t *dst, uint16_t n) {
    while (n != 0) {
        *dst = 0;
        dst++;
        n--;
    }
}

void mie_copy(uint8_t *dst, const uint8_t *src, uint16_t n) {
    while (n != 0) {
        *dst = *src;
        dst++;
        src++;
        n--;
    }
}
