/**
 * DreamShell MIE firmware
 * Second UART interface
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_SERIAL_H
#define MIE_SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_on_irq(void);
void ior_serial_41(uint8_t *payload);
void ior_serial_43(uint8_t *payload);
void ior_serial_45(uint8_t *payload);
void ior_serial_47(uint8_t *payload);
void ior_serial_49(uint8_t *payload);
void ior_serial_4b(uint8_t *payload);
void ior_serial_4d(uint8_t *payload);
void ior_serial_4f(uint8_t *payload);

#endif
