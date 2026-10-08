/**
 * DreamShell MIE firmware
 * GPIO helpers
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_GPIO_H
#define MIE_GPIO_H

#include <stdint.h>

void gpio_blast(const uint8_t *wave);
void gpio_sample(uint8_t *dest);
void gpio_apply(uint8_t dir, uint8_t g3, uint8_t g4, uint8_t g5);

#endif
