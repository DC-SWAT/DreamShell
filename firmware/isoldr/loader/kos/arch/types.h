/* KallistiOS ##version##

   arch/dreamcast/include/types.h
   (c)2000-2001 Dan Potter
   
   $Id: types.h,v 1.2 2002/01/05 07:33:50 bardtx Exp $
*/

#ifndef __ARCH_TYPES_H
#define __ARCH_TYPES_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stddef.h>

/* Generic types */
typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long int64;
typedef long int32;
typedef short int16;
typedef char int8;

/* Volatile types */
typedef volatile uint64 vuint64;
typedef volatile uint32 vuint32;
typedef volatile uint16 vuint16;
typedef volatile uint8 vuint8;
typedef volatile int64 vint64;
typedef volatile int32 vint32;
typedef volatile int16 vint16;
typedef volatile int8 vint8;

/* Pointer arithmetic types */
typedef uint32 ptr_t;

/* another format for type names */
#define __u_char_defined
#define __u_short_defined
#define __u_int_defined
#define __u_long_defined
#define __ushort_defined
#define __uint_defined
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
typedef unsigned short	ushort;
typedef unsigned int	uint;

/* File-specific types */
//typedef size_t ssize_t;
//typedef size_t off_t;

/* This type may be used for any generic handle type that is allowed
   to be negative (for errors) and has no specific bit count
   restraints. */
typedef int handle_t;
typedef handle_t file_t;

/* Thread and priority types */
typedef handle_t tid_t;
typedef handle_t prio_t;

__END_DECLS

#endif	/* __ARCH_TYPES_H */

