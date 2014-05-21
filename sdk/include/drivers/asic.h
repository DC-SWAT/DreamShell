/** 
 * \file    asic.h
 * \brief   Additional definitions for Dreamcast HOLLY ASIC
 * \date    2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_ASIC_H
#define _DS_ASIC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

#define asic_sys_reset() *(vuint32*)0x005F6890 = 0x00007611; while(1)

__END_DECLS
#endif /* _DS_ASIC_H */
