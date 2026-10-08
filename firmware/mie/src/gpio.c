/**
 * DreamShell MIE firmware
 * GPIO direction, sample and waveform output
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "gpio.h"

#pragma save
#pragma disable_warning 85
void gpio_blast(const uint8_t *wave) __naked {
    __asm
        ld b, (hl)
        inc hl
        ld c, #0x03
        otir
        ret
    __endasm;
}
#pragma restore

static uint8_t gpio_read(uint8_t index) {
    switch (index) {
        case 0:
            return gpio0;
        case 1:
            return gpio1;
        case 2:
            return gpio2;
        case 3:
            return gpio3;
        case 4:
            return gpio4;
        case 5:
            return gpio5;
        default:
            return gpio6;
    }
}

void gpio_apply(uint8_t dir, uint8_t g3, uint8_t g4, uint8_t g5) {
    gpio_dir = dir;
    gpio3 = (uint8_t)(g3 & GPIO_OUT_MASK);
    gpio4 = g4;
    gpio5 = g5;
    gpio_dir = GPIO_DIR_RUN;
}

void gpio_sample(uint8_t *dest) {
    uint8_t i;

    for (i = 0; i < GPIO_SAMPLE_PORTS; i++) {
        if (i == 3) {
            *dest = 0;
        }
        else {
            *dest = gpio_read(i);
        }
        dest++;
    }
    *dest = 0;
    dest++;
    adc = 0;
    for (i = 0; i < GPIO_SAMPLE_ADC; i++) {
        *dest = adc;
        dest++;
    }
}
