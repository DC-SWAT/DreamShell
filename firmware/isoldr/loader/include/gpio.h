/**
 * DreamShell ISO Loader
 * SH4 GPIO
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 */
#ifndef __GPIO_H__
#define __GPIO_H__

#include <main.h>
#include <exception.h>
#include <kos/regfield.h>

/* GPIO registers */
#define PCTRA_ADDR 0xFF80002C
#define PDTRA_ADDR 0xFF800030

#define PCTRA   (*(volatile unsigned int *)PCTRA_ADDR)
#define PDTRA   (*(volatile unsigned short *)PDTRA_ADDR)
#define GPIOIC  (*(volatile unsigned int *)0xFFD8001C)

/* Bit positions in PCTRA */
#define GPIO_PIN_POS(pin)    (pin * 2)

/* Bit masks for PCTRA */
#define GPIO_PIN_MASK(pin)    GENMASK(GPIO_PIN_POS(pin) + 1, GPIO_PIN_POS(pin))  /* 2 bits */
#define GPIO_PIN_CFG_IN       0  /* Configure as input */
#define GPIO_PIN_CFG_OUT(pin) BIT(GPIO_PIN_POS(pin)) /* Configure as output (01b) */

/* Cable detection pins */
#define GPIO_CABLE_DETECT_PIN_A    8
#define GPIO_CABLE_DETECT_PIN_B    9
#define GPIO_CABLE_VGA             0
#define GPIO_CABLE_NONE            1
#define GPIO_CABLE_RGB             2
#define GPIO_CABLE_COMPOSITE       3

static inline void gpio_set_as_input(int pin) {
    PCTRA = (PCTRA & ~GPIO_PIN_MASK(pin)) | GPIO_PIN_CFG_IN;
}

static inline void gpio_set_as_output(int pin) {
    PCTRA = (PCTRA & ~GPIO_PIN_MASK(pin)) | GPIO_PIN_CFG_OUT(pin);
}

static inline int gpio_read_pin(int pin) {
    return (PDTRA >> pin) & 1;
}

static inline void gpio_write_pin(int pin, int value) {
    if (value)
        PDTRA |= BIT(pin);
    else
        PDTRA &= ~BIT(pin);
}

static inline void gpio_enable_interrupt(int pin) {
    GPIOIC |= BIT(pin);
}

static inline void gpio_disable_interrupt(int pin) {
    GPIOIC &= ~BIT(pin);
}

static inline void gpio_force_cable_type(int cable_type) {

    cable_type &= 3;

    gpio_set_as_output(GPIO_CABLE_DETECT_PIN_A);
    gpio_set_as_output(GPIO_CABLE_DETECT_PIN_B);
    gpio_write_pin(GPIO_CABLE_DETECT_PIN_A, (cable_type >> 1) & 1);
    gpio_write_pin(GPIO_CABLE_DETECT_PIN_B, cable_type & 1);
}

#endif /* __GPIO_H__ */
