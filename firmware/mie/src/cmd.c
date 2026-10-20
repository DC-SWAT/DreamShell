/**
 * DreamShell MIE firmware
 * Maple and I/O command dispatch
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "maple.h"
#include "gpio.h"
#include "eeprom.h"
#include "jvs.h"
#include "serial.h"
#include "pad.h"

static const uint8_t version_pkt0[VERSION_FRAME] = {
    0x83, 0x00, 0x20, 0x07,
    '3', '1', '5', '-', '6', '1', '4', '9',
    ' ', 'D', 'R', 'E', 'A', 'M', 'S', 'H',
    'E', 'L', 'L', ' ', 'F', 'W', ' ', 'C',
    'O', 'P', 'Y', 'R'
};

static const uint8_t version_pkt1[VERSION_FRAME] = {
    0x83, 0x00, 0x20, 0x07,
    'I', 'G', 'H', 'T', ' ', 'S', 'W', 'A',
    'T', ' ', 'h', 't', 't', 'p', ':', '/',
    '/', 'd', 'c', '-', 's', 'w', 'a', 't',
    '.', 'r', 'u', 0x00
};

static const uint8_t gpio_reply_hdr[5] = {
    MAPLE_RESP_IO, 0x00, MAPLE_ADDR, 0x05, 0x32
};

static void cmd_reset(void) {
    maple_send_ok();
    if (pad_active()) {
        return;
    }
    mie_rom();
}

static void cmd_self_test(void) {
    uint8_t sample;
    uint8_t i;

    sample = selftest_byte;
    MAPLE_BUF[0] = MAPLE_RESP_SELFTEST;
    MAPLE_BUF[1] = 0;
    MAPLE_BUF[2] = MAPLE_ADDR;
    MAPLE_BUF[3] = 1;
    for (i = 0; i < SELFTEST_COPIES; i++) {
        MAPLE_PAYLOAD[i] = sample;
    }
    MAPLE_LREG = 1;
    maple_reply_tbuf();
}

static void cmd_version(void) {
    mie_copy((uint8_t *)MAPLE_BUF, version_pkt0, VERSION_FRAME);
    MAPLE_LREG = 7;
    maple_tx();
    mie_copy((uint8_t *)MAPLE_BUF, version_pkt1, VERSION_FRAME);
    MAPLE_LREG = 7;
    maple_tx_end();
}

static void cmd_upload(uint8_t *payload) {
    uint8_t *dest;
    uint8_t sum;
    uint8_t i;

    mie_di();
    irq_enable = 0;
    irq_pending = 0;
    if (payload[0] == UPLOAD_EXEC && payload[1] == UPLOAD_EXEC) {
        maple_send_ok();
        mie_reboot();
    }

    dest = (uint8_t *)(((uint16_t)payload[0] << 8) | payload[1]);
    mie_copy(dest, payload + 2, UPLOAD_COPY);
    sum = 0;
    for (i = 0; i < UPLOAD_SUM; i++) {
        sum = (uint8_t)(sum + payload[i]);
    }
    MAPLE_BUF[0] = MAPLE_RESP_UPLOAD;
    MAPLE_BUF[1] = 0;
    MAPLE_BUF[2] = MAPLE_ADDR;
    MAPLE_BUF[3] = 1;
    MAPLE_PAYLOAD[0] = sum;
    MAPLE_LREG = 1;
    maple_reply_tbuf();
    maple_send_ok();
    mie_reboot();
}

void ior_unknown(void) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = MAPLE_RESP_BADCMD;
    reply_buf[5] = 0;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
}

void ior_gpio(uint8_t *payload) {
    gpio_apply(payload[1], payload[2], payload[3], payload[4]);
    gpio_sample((uint8_t *)MAPLE_BUF + 5);
    mie_copy((uint8_t *)MAPLE_BUF, gpio_reply_hdr, sizeof(gpio_reply_hdr));
    MAPLE_LREG = 5;
    maple_reply_tbuf();
}

void ior_peek(uint8_t *payload) {
    uint8_t *src;
    uint8_t len;

    len = payload[1];
    src = (uint8_t *)(((uint16_t)payload[2] << 8) | payload[3]);
    maple_reply_long(src, len, MAPLE_RESP_IO, 0, MAPLE_ADDR);
}

static void cmd_io(uint8_t *payload) {
    switch (payload[0]) {
        case 0x01:
            ior_eeprom_req();
            return;
        case 0x03:
            ior_eeprom_fetch();
            return;
        case 0x05:
            ior_eeprom_05();
            return;
        case 0x07:
            ior_eeprom_07(payload);
            return;
        case 0x09:
            ior_eeprom_09();
            return;
        case 0x0B:
            ior_eeprom_write(payload);
            return;
#ifdef MIE_JVS_EXTRA
        case 0x11:
            ior_jvs_11(payload);
            return;
        case 0x13:
            ior_jvs_13(payload);
            return;
#endif
        case 0x15:
            ior_jvs_fetch(payload);
            return;
        case 0x17:
            ior_jvs_req(payload);
            return;
#ifdef MIE_JVS_EXTRA
        case 0x19:
            ior_jvs_19(payload);
            return;
        case 0x21:
            ior_jvs_21(payload);
            return;
        case 0x23:
            ior_jvs_23(payload);
            return;
        case 0x25:
            ior_jvs_25(payload);
            return;
        case 0x27:
            ior_jvs_27(payload);
            return;
#endif
        case 0x31:
            ior_gpio(payload);
            return;
#ifdef MIE_JVS_EXTRA
        case 0x33:
            ior_jvs_33(payload);
            return;
        case 0x35:
            ior_jvs_35(payload);
            return;
        case 0x37:
            ior_jvs_37(payload);
            return;
#endif
#ifdef MIE_SERIAL
        case 0x41:
            ior_serial_41(payload);
            return;
        case 0x43:
            ior_serial_43(payload);
            return;
        case 0x45:
            ior_serial_45(payload);
            return;
        case 0x47:
            ior_serial_47(payload);
            return;
        case 0x49:
            ior_serial_49(payload);
            return;
        case 0x4B:
            ior_serial_4b(payload);
            return;
        case 0x4D:
            ior_serial_4d(payload);
            return;
        case 0x4F:
            ior_serial_4f(payload);
            return;
#endif
        case MAPLE_IO_PAD:
            ior_pad(payload);
            return;
        case 0xFF:
            ior_peek(payload);
            return;
        default:
            ior_unknown();
            return;
    }
}

void maple_dispatch(uint8_t cmd) {
    if (pad_maple(cmd)) {
        return;
    }
    switch (cmd) {
        case MAPLE_CMD_DEVINFO:
            pad_devinfo();
            return;
        case MAPLE_CMD_GETCOND:
            pad_getcond();
            return;
        case MAPLE_CMD_RESET:
            cmd_reset();
            return;
        case MAPLE_CMD_UPLOAD:
            cmd_upload(MAPLE_PAYLOAD);
            return;
        case MAPLE_CMD_VERSION:
            cmd_version();
            return;
        case MAPLE_CMD_SELFTEST:
            cmd_self_test();
            return;
        case MAPLE_CMD_IO:
            cmd_io(MAPLE_PAYLOAD);
            return;
        default:
            maple_reply_badcmd();
            return;
    }
}
