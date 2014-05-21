#ifndef _INIT_H
#define _INIT_H

#include <stdint.h>

void reset(void);

uint32_t int_enable(void);
uint32_t int_disable(void);
void int_restore(uint32_t context);
int int_enabled(void);

void int_acknowledge(void);

#endif
