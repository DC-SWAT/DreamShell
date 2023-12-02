/**
 * DreamShell ISO Loader
 * Maple sniffing and VMU emulation
 * (c)2022-2023 SWAT <http://www.dc-swat.ru>
 */

#ifndef _MAPLE_H
#define _MAPLE_H

#include <arch/types.h>

int maple_init_irq();
int maple_init_vmu(int num);

#endif /* _MAPLE_H */
