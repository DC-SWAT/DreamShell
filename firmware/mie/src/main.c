/**
 * DreamShell MIE firmware
 * Reset, hardware init and main loop
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "maple.h"
#include "gpio.h"
#include "jvs.h"
#include "serial.h"

static const uint8_t wave_gpio[] = {
    0x1c,
    0x00, 0x00, 0x30, 0x70, 0x20, 0x60, 0x20, 0x60,
    0x30, 0x70, 0x30, 0x70, 0x20, 0x60, 0x20, 0x60,
    0x20, 0x60, 0x20, 0x60, 0x20, 0x60, 0x20, 0x60,
    0x20, 0x60, 0x1c, 0x00
};

static void hw_bringup(void) {
    uint8_t i;

    active_flag = 1;
    mie_zero(reply_buf, MAPLE_FRAME);
    reply_buf[2] = MAPLE_ADDR;
    gpio3 = 0;
    gpio4 = 0;
    gpio5 = 0;
    gpio_dir = GPIO_DIR_RUN;
    port_40 = PORT40_OFF;
    port_40 = PORT40_ON;
    pkt_flag = 0;
    jvs_uart_init();
    gpio_blast(wave_gpio);
    port_80 = 0;
    port_81 = 0;
    irq_pending = 0;
    maple_irq_level = IRQ_LEVEL_MAPLE;
    irq_enable = IRQ_ENABLE_MAPLE;
    for (i = 0; i < MAPLE_FRAME; i++) {
        jvs_frame[i] = 0;
    }
}

void mie_start(void) {
    hw_bringup();
    mie_ei();
    bus_init();
    for (;;) {
        pkt_flag = 0;
        for (;;) {
            if (MAPLE_STAT & MAPLE_STAT_HRES) {
                maple_dispatch(MAPLE_CMD_RESET);
            }
            if (pkt_flag == 1) {
                break;
            }
        }
        pkt_flag = 2;
        if (MAPLE_STAT & MAPLE_STAT_BFOV) {
            maple_reply_again();
            continue;
        }
        if (MAPLE_STAT & MAPLE_STAT_PERR) {
            maple_reply_again();
            continue;
        }
        maple_dispatch(MAPLE_BUF[0]);
    }
}
