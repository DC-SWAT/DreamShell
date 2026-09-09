/**
 * \file    arch/types.h
 * \brief   Compatibility aliases for old KOS integer types
 * \author  SWAT www.dc-swat.ru
 */

#ifndef __ARCH_TYPES_H
#define __ARCH_TYPES_H

#ifndef __ASSEMBLER__

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#include <stddef.h>
#include <stdint.h>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;
typedef int64_t  int64;
typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;

typedef volatile uint64 vuint64;
typedef volatile uint32 vuint32;
typedef volatile uint16 vuint16;
typedef volatile uint8  vuint8;
typedef volatile int64  vint64;
typedef volatile int32  vint32;
typedef volatile int16  vint16;
typedef volatile int8   vint8;

#endif /* !__ASSEMBLER__ */

#endif /* __ARCH_TYPES_H */

