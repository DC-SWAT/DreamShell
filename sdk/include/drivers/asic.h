/** 
 * \file    asic.h
 * \brief   Additional definitions for Dreamcast HOLLY ASIC
 * \date    2014-2015
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_ASIC_H
#define _DS_ASIC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

#define ASIC_BIOS_PROT *((vuint32*)0xa05F74E4)
#define ASIC_SYS_RESET *((vuint32*)0x005F6890)

/**
 * Holly reset
 */
void asic_sys_reset(void);

/**
 * Disabling IDE interface
 */
void asic_ide_disable(void);

/**
 * Enabling IDE interface (BIOS check)
 */
void asic_ide_enable(void);


__END_DECLS
#endif /* _DS_ASIC_H */
