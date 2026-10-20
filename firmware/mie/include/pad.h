/**
 * DreamShell MIE firmware
 * Dreamcast controller bridge
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_PAD_H
#define MIE_PAD_H

#include <stdint.h>

uint8_t pad_active(void);
uint8_t pad_maple(uint8_t cmd);
void pad_poll(void);
void pad_devinfo(void);
void pad_getcond(void);
void ior_pad(uint8_t *payload);

#endif
