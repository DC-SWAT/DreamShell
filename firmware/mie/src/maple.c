/**
 * DreamShell MIE firmware
 * Maple transmit, replies and IRQ
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "maple.h"

static uint8_t spin_set(volatile uint8_t *reg, uint8_t mask) {
    uint8_t n = 0;

    for (;;) {
        n++;
        if (n == MAPLE_SPIN) {
            return 0;
        }
        if (*reg & mask) {
            return 1;
        }
    }
}

static uint8_t spin_clear(volatile uint8_t *reg, uint8_t mask) {
    uint8_t n = 0;

    for (;;) {
        n++;
        if (n == 0) {
            return 0;
        }
        if ((*reg & mask) == 0) {
            return 1;
        }
    }
}

static void wait_clear(volatile uint8_t *reg, uint8_t mask) {
    while (*reg & mask) {
    }
}

static void wait_set(volatile uint8_t *reg, uint8_t mask) {
    while ((*reg & mask) == 0) {
    }
}

void maple_tx(void) {
    MAPLE_CTRL = (uint8_t)((MAPLE_CTRL & (uint8_t)~MAPLE_CTRL_ENDP) | MAPLE_CTRL_TXB);
    if (!spin_set(&MAPLE_STAT, MAPLE_STAT_EMP)) {
        MAPLE_STAT = (uint8_t)(MAPLE_STAT | MAPLE_STAT_EMP);
    }
}

void maple_tx_cont(void) {
    MAPLE_CTRL = (uint8_t)((MAPLE_CTRL & (uint8_t)~MAPLE_CTRL_ENDP) | MAPLE_CTRL_CTXB);
    if (!spin_set(&MAPLE_STAT, MAPLE_STAT_EMP)) {
        MAPLE_STAT = (uint8_t)(MAPLE_STAT | MAPLE_STAT_EMP);
    }
}

void maple_tx_end(void) {
    MAPLE_CTRL = (uint8_t)(MAPLE_CTRL | MAPLE_CTRL_ENDP | MAPLE_CTRL_CTXB);
    if (!spin_set(&MAPLE_STAT, MAPLE_STAT_EMP)) {
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL & (uint8_t)~MAPLE_CTRL_TXB);
        MAPLE_STAT = (uint8_t)(MAPLE_STAT | MAPLE_STAT_EMP | MAPLE_STAT_TFB);
        return;
    }
    if (!spin_clear(&MAPLE_CTRL, MAPLE_CTRL_TXB)) {
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL & (uint8_t)~MAPLE_CTRL_TXB);
        MAPLE_STAT = (uint8_t)(MAPLE_STAT | MAPLE_STAT_EMP | MAPLE_STAT_TFB);
        return;
    }
    if (!spin_set(&MAPLE_STAT, MAPLE_STAT_TFB)) {
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL & (uint8_t)~MAPLE_CTRL_TXB);
        MAPLE_STAT = (uint8_t)(MAPLE_STAT | MAPLE_STAT_EMP | MAPLE_STAT_TFB);
    }
}

void maple_reply(uint8_t response, uint8_t dest, uint8_t src, uint8_t words) {
    MAPLE_BUF[0] = response;
    MAPLE_BUF[1] = dest;
    MAPLE_BUF[2] = src;
    MAPLE_BUF[3] = words;
    MAPLE_LREG = words;
    MAPLE_CTRL = (uint8_t)(MAPLE_CTRL | MAPLE_CTRL_ENDP | MAPLE_CTRL_TXB);
    wait_clear(&MAPLE_CTRL, MAPLE_CTRL_TXB);
    wait_set(&MAPLE_STAT, MAPLE_STAT_TFB);
}

void maple_reply_iy(void) {
    uint8_t words;
    uint8_t bytes;
    uint8_t i;

    words = reply_buf[3];
    MAPLE_LREG = words;
    bytes = (uint8_t)((words + 1) << 2);
    for (i = 0; i < bytes; i++) {
        MAPLE_BUF[i] = reply_buf[i];
    }
    MAPLE_CTRL = (uint8_t)(MAPLE_CTRL | MAPLE_CTRL_ENDP | MAPLE_CTRL_TXB);
    if (!spin_clear(&MAPLE_CTRL, MAPLE_CTRL_TXB) ||
            !spin_set(&MAPLE_STAT, MAPLE_STAT_TFB)) {
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL & (uint8_t)~(MAPLE_CTRL_TXB | MAPLE_CTRL_ENDP));
        MAPLE_STAT = (uint8_t)(MAPLE_STAT & (uint8_t)~MAPLE_STAT_TFB);
    }
}

void maple_reply_tbuf(void) {
    MAPLE_CTRL = (uint8_t)(MAPLE_CTRL | MAPLE_CTRL_ENDP | MAPLE_CTRL_TXB);
    if (!spin_clear(&MAPLE_CTRL, MAPLE_CTRL_TXB) ||
            !spin_set(&MAPLE_STAT, MAPLE_STAT_TFB)) {
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL & (uint8_t)~(MAPLE_CTRL_TXB | MAPLE_CTRL_ENDP));
        MAPLE_STAT = (uint8_t)(MAPLE_STAT & (uint8_t)~MAPLE_STAT_TFB);
    }
}

void maple_reply_long(uint8_t *data, uint8_t len, uint8_t response,
                      uint8_t dest, uint8_t src) {
    uint8_t words;
    uint8_t *srcp;

    mie_zero((uint8_t *)MAPLE_BUF, MAPLE_FRAME);
    if (len == 0) {
        maple_reply(response, dest, src, 0);
        return;
    }

    words = (uint8_t)(((len - 1) >> 2) + 1);
    srcp = data;
    if (len < REPLY_ONE_CHUNK) {
        MAPLE_BUF[0] = response;
        MAPLE_BUF[1] = dest;
        MAPLE_BUF[2] = src;
        MAPLE_BUF[3] = words;
        MAPLE_LREG = words;
        mie_copy((uint8_t *)MAPLE_PAYLOAD, srcp, len);
        MAPLE_CTRL = (uint8_t)(MAPLE_CTRL | MAPLE_CTRL_ENDP | MAPLE_CTRL_TXB);
        wait_clear(&MAPLE_CTRL, MAPLE_CTRL_TXB);
        wait_set(&MAPLE_STAT, MAPLE_STAT_TFB);
        return;
    }

    MAPLE_BUF[0] = response;
    MAPLE_BUF[1] = dest;
    MAPLE_BUF[2] = src;
    MAPLE_BUF[3] = words;
    MAPLE_LREG = words;
    mie_copy((uint8_t *)MAPLE_PAYLOAD, srcp, REPLY_FIRST);
    srcp += REPLY_FIRST;
    len = (uint8_t)(len - REPLY_FIRST);
    maple_tx();
    while (len >= REPLY_NEXT_LIMIT) {
        mie_copy((uint8_t *)MAPLE_BUF, srcp, REPLY_NEXT);
        srcp += REPLY_NEXT;
        len = (uint8_t)(len - REPLY_NEXT);
        maple_tx_cont();
    }
    mie_zero((uint8_t *)MAPLE_BUF, MAPLE_FRAME);
    mie_copy((uint8_t *)MAPLE_BUF, srcp, len);
    maple_tx_end();
}

void maple_send_ok(void) {
    MAPLE_BUF[0] = MAPLE_RESP_OK;
    MAPLE_BUF[1] = 0;
    MAPLE_BUF[2] = MAPLE_ADDR;
    MAPLE_BUF[3] = 0;
    MAPLE_LREG = 0;
    maple_reply_tbuf();
}

void maple_reply_again(void) {
    reply_buf[0] = MAPLE_RESP_AGAIN;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 0;
    maple_reply_iy();
}

void maple_reply_badcmd(void) {
    reply_buf[0] = MAPLE_RESP_BADCMD;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 0;
    maple_reply_iy();
}

void maple_on_irq(void) {
    uint8_t st;

    st = MAPLE_STAT;
    if (st & MAPLE_STAT_HRES) {
        mie_rom();
    }
    if ((st & MAPLE_STAT_RFB) == 0) {
        return;
    }
    if (st & MAPLE_STAT_RXB) {
        return;
    }
    if (pkt_flag == 2) {
        maple_reply(MAPLE_RESP_AGAIN, 0, MAPLE_ADDR, 0);
        return;
    }
    pkt_flag = 1;
}
