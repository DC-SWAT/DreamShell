/**
 * DreamShell MIE firmware
 * Second UART buffers and IRQ
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "serial.h"
#include "maple.h"

#ifdef MIE_SERIAL

#define SER_IDLE        0
#define SER_TX          1
#define SER_ARM         2
#define SER_RX          3
#define SER_BUSY        0x03
#define SER_GO          0x80
#define SER_ARM_BYTE    0x81
#define SER_FLAG        0x40
#define SER_FILL        0x81
#define SER_HDR         0xB7FC
#define SER_SNAP        0xB400

static uint8_t ser_ready(void) {
    uint8_t n;

    n = 0;
    for (;;) {
        n++;
        if (n == MAPLE_SPIN) {
            return 0;
        }
        if ((port_0d & SER_BUSY) == 0) {
            return 1;
        }
    }
}

static void ser_kick(uint8_t mark) {
    port_0a = mark;
    port_09 = mark;
    if (ser_ready()) {
        port_0d = SER_GO;
    }
}

static void serial_arm_rx(void) {
    (void)port_0b;
    (void)port_0c;
    serial_state = SER_ARM;
    mie_di();
    ser_kick(SER_GO);
    mie_ei();
}

static void serial_tx_next(void) {
    mie_di();
    serial_state = SER_TX;
    if (serial_pos != 0) {
        uint8_t *p;

        serial_pos--;
        p = (uint8_t *)serial_ptr;
        serial_latch = *p;
        serial_ptr++;
    }
    port_0a = serial_latch;
    port_09 = 1;
    if (!ser_ready()) {
        serial_pos = 0;
        port_0d = (uint8_t)(port_0d & (uint8_t)~SER_GO);
    }
    else {
        port_0d = SER_GO;
    }
    mie_ei();
}

void serial_init(void) {
    (void)port_0b;
    (void)port_0c;
    serial_state = SER_IDLE;
    mie_zero(serial_buf_c, SERIAL_BUF);
    mie_zero(serial_buf_b, SERIAL_BUF);
    serial_pos = 0;
    serial_limit = 0;
    mie_di();
    irq_enable = (uint8_t)(irq_enable | IRQ_ENABLE_SERIAL);
    mie_ei();
}

void serial_on_irq(void) {
    uint8_t seen;

    if (serial_state == SER_TX) {
        (void)port_0b;
        seen = port_0c;
        if (seen == serial_latch) {
            if (serial_pos == 0) {
                serial_pos = 0;
                serial_state = SER_IDLE;
                return;
            }
            serial_pos--;
            serial_latch = *(uint8_t *)serial_ptr;
            serial_ptr++;
        }
        port_0a = serial_latch;
        port_09 = 1;
        if (!ser_ready()) {
            serial_pos = 0;
            serial_state = SER_IDLE;
            return;
        }
        serial_state = SER_TX;
        port_0d = SER_GO;
        return;
    }
    if (serial_state == SER_ARM) {
        uint8_t flag;

        (void)port_0b;
        flag = port_0c;
        (void)port_0d;
        if ((flag & SER_FLAG) == 0) {
            serial_state = SER_IDLE;
            return;
        }
        port_0a = SER_ARM_BYTE;
        port_09 = SER_ARM_BYTE;
        if (!ser_ready()) {
            serial_state = SER_IDLE;
            return;
        }
        serial_state = (uint8_t)(serial_state + 1);
        port_0d = SER_GO;
        return;
    }
    if (serial_state == SER_RX) {
        uint8_t byte;

        (void)port_0b;
        byte = port_0c;
        serial_buf_b[serial_limit] = byte;
        serial_limit++;
        port_0a = SER_GO;
        port_09 = SER_GO;
        if (!ser_ready()) {
            serial_state = SER_IDLE;
            return;
        }
        serial_state = SER_ARM;
        port_0d = SER_GO;
        return;
    }
    (void)port_0b;
    (void)port_0c;
}

static void serial_fill(uint8_t *buf, uint8_t value) {
    uint16_t i;

    for (i = 0; i < SER_FILL; i++) {
        buf[i] = value;
    }
}

static void serial_ack(uint8_t *payload) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = (uint8_t)(payload[0] + 1);
    reply_buf[5] = 0;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
}

void ior_serial_41(uint8_t *payload) {
    (void)port_0b;
    (void)port_0c;
    serial_state = SER_IDLE;
    serial_limit = 0;
    serial_pos = 0;
    serial_ack(payload);
}

void ior_serial_43(uint8_t *payload) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 2;
    reply_buf[4] = (uint8_t)(payload[0] + 1);
    reply_buf[5] = port_0d;
    reply_buf[6] = (uint8_t)(serial_limit >> 8);
    reply_buf[7] = (uint8_t)serial_limit;
    reply_buf[8] = (uint8_t)(serial_pos >> 8);
    reply_buf[9] = (uint8_t)serial_pos;
    reply_buf[10] = 0;
    reply_buf[11] = 0;
    maple_reply_iy();
}

void ior_serial_45(uint8_t *payload) {
    serial_fill(serial_buf_c, payload[1]);
    serial_ack(payload);
}

void ior_serial_47(uint8_t *payload) {
    uint16_t offset;
    uint8_t count;

    offset = ((uint16_t)payload[1] << 8) | payload[2];
    count = payload[3];
    mie_copy(serial_buf_c + offset, payload + 4, count);
    serial_ack(payload);
}

void ior_serial_49(uint8_t *payload) {
    uint16_t offset;
    uint16_t count;

    serial_mark = payload[1];
    offset = ((uint16_t)payload[2] << 8) | payload[3];
    count = ((uint16_t)payload[4] << 8) | payload[5];
    serial_ack(payload);
    if (serial_pos != 0) {
        return;
    }
    serial_ptr = (uint16_t)(serial_buf_c + offset);
    serial_pos = count;
    serial_tx_next();
}

void ior_serial_4b(uint8_t *payload) {
    serial_fill(serial_buf_b, payload[1]);
    serial_limit = 0;
    serial_ack(payload);
}

void ior_serial_4d(uint8_t *payload) {
    uint16_t offset;
    uint8_t len;
    uint8_t *hdr;
    uint8_t saved[4];

    offset = ((uint16_t)payload[1] << 8) | payload[2];
    len = payload[3];
    if (len == 0xFF) {
        len = (uint8_t)serial_limit;
    }
    hdr = (uint8_t *)(SER_HDR + offset);
    mie_copy(saved, hdr, 4);
    hdr[0] = 0x4E;
    hdr[1] = len;
    hdr[2] = (uint8_t)(serial_limit >> 8);
    hdr[3] = (uint8_t)serial_limit;
    maple_reply_long(hdr, (uint8_t)(len + 4), MAPLE_RESP_IO, 0, MAPLE_ADDR);
    mie_copy(hdr, saved, 4);
}

void ior_serial_4f(uint8_t *payload) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = (uint8_t)(payload[0] + 1);
    reply_buf[5] = port_0d;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
    if (serial_state != SER_IDLE) {
        return;
    }
    if (serial_pos != 0) {
        return;
    }
    serial_arm_rx();
}

#else

void serial_on_irq(void) {
}

#endif
