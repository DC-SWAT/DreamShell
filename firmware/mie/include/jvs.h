/**
 * DreamShell MIE firmware
 * JVS UART interface
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_JVS_H
#define MIE_JVS_H

#include <stdint.h>

void jvs_uart_init(void);
void jvs_on_irq(void);
void ior_jvs_req(uint8_t *payload);
void ior_jvs_fetch(uint8_t *payload);
void ior_jvs_11(uint8_t *payload);
void ior_jvs_13(uint8_t *payload);
void ior_jvs_19(uint8_t *payload);
void ior_jvs_21(uint8_t *payload);
void ior_jvs_23(uint8_t *payload);
void ior_jvs_25(uint8_t *payload);
void ior_jvs_27(uint8_t *payload);
void ior_jvs_33(uint8_t *payload);
void ior_jvs_35(uint8_t *payload);
void ior_jvs_37(uint8_t *payload);

#endif
