/**
 * DreamShell ISO Loader
 * Maple sniffing and VMU emulation
 * (c)2022-2023 SWAT <http://www.dc-swat.ru>
 */

#ifndef _MAPLE_H
#define _MAPLE_H

#include <arch/types.h>

#define MAPLE_REG(x) (*(vuint32 *)(x))
#define MAPLE_BASE 0xa05f6c00
#define MAPLE_DMA_ADDR (MAPLE_BASE + 0x04)
#define MAPLE_DMA_STATUS (MAPLE_BASE + 0x18)

int maple_init_irq();
int maple_init_vmu(int num);

#endif /* _MAPLE_H */
