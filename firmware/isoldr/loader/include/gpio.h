/**
 * DreamShell ISO Loader
 * SH4 GPIO
 * (c)2025 SWAT <http://www.dc-swat.ru>
 */
#ifndef __GPIO_H__
#define __GPIO_H__

#include <main.h>
#include <exception.h>
#include <kos/regfield.h>

/* GPIO registers */
#define PCTRA   (*(volatile unsigned int *)0xFF80002C)
#define PDTRA   (*(volatile unsigned short *)0xFF800030)
#define GPIOIC  (*(volatile unsigned int *)0xFFD8001C)

/* Bit positions in PCTRA */
#define GPIO_PIN_POS(pin)    (pin * 2)

/* Bit masks for PCTRA */
#define GPIO_PIN_MASK(pin)    GENMASK(GPIO_PIN_POS(pin) + 1, GPIO_PIN_POS(pin))  /* 2 bits */
#define GPIO_PIN_CFG_IN       0  /* Configure as input */
#define GPIO_PIN_CFG_OUT(pin) BIT(GPIO_PIN_POS(pin)) /* Configure as output (01b) */

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

#endif /* __GPIO_H__ */
