/**
 * DreamShell MIE firmware
 * EEPROM bitbang and I/O commands
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "gpio.h"
#include "maple.h"
#include "eeprom.h"

static const uint8_t wave_shift_in[] = {
    0x08, 0x00, 0x00, 0x30, 0x70, 0x30, 0x70, 0x20, 0x60
};

static const uint8_t wave_shift_out[] = {
    0x08, 0x00, 0x00, 0x30, 0x70, 0x20, 0x60, 0x30, 0x70
};

static const uint8_t wave_op07[] = {
    0x1c, 0x00, 0x00, 0x30, 0x70, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60,
    0x30, 0x70, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60,
    0x20, 0x60, 0x20, 0x60, 0x20, 0x60
};

static const uint8_t wave_op05[] = {
    0x1c, 0x00, 0x00, 0x30, 0x70, 0x20, 0x60, 0x20, 0x60, 0x30, 0x70,
    0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60,
    0x20, 0x60, 0x20, 0x60, 0x20, 0x60
};

static const uint8_t eeprom_ack1[4] = {
    MAPLE_RESP_IO, 0x00, MAPLE_ADDR, 0x01
};

static const uint8_t eeprom_ack2[8] = {
    MAPLE_RESP_IO, 0x00, MAPLE_ADDR, 0x02, 0x02, 0x00, 0x00, 0x00
};

static void ee_pulse(uint8_t bits) {
    gpio3 = bits;
    gpio3 = (uint8_t)(bits | EE_STROBE);
}

static void ee_clock_addr(uint8_t addr) {
    uint8_t i;

    for (i = 0; i < EE_ADDR_BITS; i++) {
        uint8_t bit;

        bit = 0;
        if (addr & EE_ADDR_BIT) {
            bit = EE_BIT;
        }
        addr = (uint8_t)(addr << 1);
        ee_pulse((uint8_t)(EE_CLK | bit));
    }
}

static void ee_clock_data(uint8_t data) {
    uint8_t i;

    for (i = 0; i < EE_DATA_BITS; i++) {
        uint8_t bit;

        bit = 0;
        if (data & 0x80) {
            bit = EE_BIT;
        }
        data = (uint8_t)(data << 1);
        ee_pulse((uint8_t)(EE_CLK | bit));
    }
}

static uint8_t ee_clock_in(void) {
    uint8_t value;
    uint8_t i;

    value = 0;
    for (i = 0; i < EE_DATA_BITS; i++) {
        value = (uint8_t)(value << 1);
        ee_pulse(EE_CLK);
        if (gpio5 & EE_DIN) {
            value = (uint8_t)(value + 1);
        }
    }
    return value;
}

static void ee_wait_ready(void) {
    for (;;) {
        ee_pulse(EE_CLK);
        if (gpio5 & EE_DIN) {
            return;
        }
    }
}

static uint8_t eeprom_shift_in(uint8_t addr) {
    uint8_t value;

    gpio_blast(wave_shift_in);
    ee_clock_addr(addr);
    value = ee_clock_in();
    ee_pulse(EE_CLK);
    gpio3 = 0;
    gpio3 = EE_STROBE;
    return value;
}

static void eeprom_shift_out(uint8_t addr, uint8_t data) {
    uint8_t i;

    gpio_blast(wave_shift_out);
    ee_clock_addr(addr);
    ee_clock_data(data);
    for (i = 0; i < EE_TAIL; i++) {
        gpio3 = 0;
    }
    ee_wait_ready();
    gpio3 = 0;
    gpio3 = 0;
}

void eeprom_load(void) {
    uint8_t i;

    gpio_dir = GPIO_DIR_RUN;
    for (i = 0; i < EEPROM_SIZE; i++) {
        eeprom_buf[i] = eeprom_shift_in(i);
    }
}

void eeprom_store(uint8_t offset, uint8_t count) {
    gpio_dir = GPIO_DIR_RUN;
    while (count != 0) {
        eeprom_shift_out(offset, eeprom_wr[offset]);
        offset++;
        count--;
    }
}

void eeprom_op05(void) {
    gpio_dir = GPIO_DIR_RUN;
    gpio_blast(wave_op05);
    gpio3 = 0;
    gpio3 = 0;
    gpio3 = 0;
    gpio3 = 0;
    ee_wait_ready();
}

void eeprom_op07(uint8_t data) {
    gpio_blast(wave_op07);
    ee_clock_data(data);
    gpio3 = 0;
    gpio3 = 0;
    ee_pulse(EE_CLK);
}

void eeprom_op09(void) {
    uint8_t i;

    gpio_dir = GPIO_DIR_RUN;
    for (i = 0; i < EEPROM_SIZE; i++) {
        eeprom_shift_out(i, eeprom_wr[i]);
    }
}

void ior_eeprom_req(void) {
    mie_copy((uint8_t *)MAPLE_BUF, eeprom_ack2, sizeof(eeprom_ack2));
    MAPLE_LREG = 1;
    maple_reply_tbuf();
    eeprom_load();
}

void ior_eeprom_fetch(void) {
    maple_reply_long(eeprom_buf, EEPROM_SIZE, MAPLE_RESP_IO, 0, MAPLE_ADDR);
}

void ior_eeprom_write(uint8_t *payload) {
    uint8_t offset;
    uint8_t count;

    offset = payload[1];
    count = payload[2];
    mie_copy(eeprom_wr + offset, payload + 4, count);
    maple_reply_long(eeprom_wr, 4, MAPLE_RESP_IO, 0, MAPLE_ADDR);
    eeprom_store(offset, count);
}

void ior_eeprom_05(void) {
    maple_reply(MAPLE_RESP_IO, 0, MAPLE_ADDR, 0);
    eeprom_op05();
}

void ior_eeprom_07(uint8_t *payload) {
    maple_reply(MAPLE_RESP_IO, 0, MAPLE_ADDR, 0);
    eeprom_op07(payload[1]);
}

void ior_eeprom_09(void) {
    mie_copy((uint8_t *)MAPLE_BUF, eeprom_ack1, sizeof(eeprom_ack1));
    MAPLE_LREG = 0;
    maple_reply_tbuf();
    eeprom_op09();
}
