/**
 * DreamShell MIE firmware
 * Maple reply interface
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_MAPLE_H
#define MIE_MAPLE_H

#include <stdint.h>

typedef void (*maple_fn)(uint8_t *payload);

void maple_reply(uint8_t response, uint8_t dest, uint8_t src, uint8_t words);
void maple_reply_iy(void);
void maple_reply_tbuf(void);
void maple_reply_long(const uint8_t *data, uint8_t len, uint8_t response,
                      uint8_t dest, uint8_t src);
void maple_tx(void);
void maple_tx_cont(void);
void maple_tx_end(void);
void maple_send_ok(void);
void maple_reply_again(void);
void maple_reply_badcmd(void);
void maple_on_irq(void);
void maple_dispatch(uint8_t cmd);

#endif
