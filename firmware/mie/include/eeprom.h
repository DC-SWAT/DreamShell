/**
 * DreamShell MIE firmware
 * EEPROM bitbang interface
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_EEPROM_H
#define MIE_EEPROM_H

#include <stdint.h>

void eeprom_load(void);
void eeprom_store(uint8_t offset, uint8_t count);
void eeprom_op05(void);
void eeprom_op07(uint8_t data);
void eeprom_op09(void);

void ior_eeprom_req(void);
void ior_eeprom_fetch(void);
void ior_eeprom_write(uint8_t *payload);
void ior_eeprom_05(void);
void ior_eeprom_07(uint8_t *payload);
void ior_eeprom_09(void);

#endif
