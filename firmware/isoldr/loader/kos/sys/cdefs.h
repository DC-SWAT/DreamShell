/* KallistiOS ##version##

   sys/cdefs.h
   (c)2002 Dan Potter

   $Id: cdefs.h,v 1.2 2002/10/26 08:01:34 bardtx Exp $

   Based loosely around some stuff in BSD's sys/cdefs.h
*/

#ifndef __SYS_CDEFS_H
#define __SYS_CDEFS_H

/* Check GCC version */
#ifndef _arch_ps2
#	if __GNUC__ < 2
#		warning Your GCC is too old. This will probably not work right.
#	endif

#	if __GNUC__ == 2 && __GNUC_MINOR__ < 97
#		warning Your GCC is too old. This will probably not work right.
#	endif
#endif	/* _arch_ps2 */

/* Special function/variable attributes */
#define __noreturn	__attribute__((__noreturn__))
#define __pure		__attribute__((__const__))
#define __unused	__attribute__((__unused__))

#define __dead2		__noreturn	/* BSD compat */
#define __pure2		__pure		/* ditto */

/* Printf/Scanf-like declaration */
#define __printflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

#define __scanflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

/* C++ compatability support */
#ifdef __cplusplus
#	define __BEGIN_DECLS	extern "C" {
#	define __END_DECLS	}
#else
#	define __BEGIN_DECLS
#	define __END_DECLS
#endif

/* GCC macros for special cases */
/* #if __GNUC__ ==  */

/* Optional CVS ID tags, without warnings */
#if defined(__GNUC__) && defined(__ELF__)
#	define __IDSTRING(name, string) __asm__(".ident\t\"" string "\"")
#else
#	define __IDSTRING(name, string) static const char name[] __unused = string;
#endif

#ifndef NO_CVS_ID
#	define IDSTRING(name, s) __IDSTRING(name, s)
#	define CVSID(s) __IDSTRING(cvsid, "KOS " s)
#else
#	define IDSTRING(s)
#endif

#endif	/* __SYS_CDEFS_H */


