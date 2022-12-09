/**
 * DreamShell ISO Loader
 * Generic stdio-style functions
 * (c)2009-2014 SWAT <http://www.dc-swat.ru>
 */

#ifndef __STDIO_H
#define __STDIO_H

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdarg.h>

/* Flags which can be passed to number () */
#define N_ZEROPAD   1       /* pad with zero */
#define N_SIGN      2       /* unsigned/signed long */
#define N_PLUS      4       /* show plus */
#define N_SPACE     8       /* space if plus */
#define N_LEFT      16      /* left justified */
#define N_SPECIAL   32      /* 0x */
#define N_LARGE     64      /* use 'ABCDEF' instead of 'abcdef' */

char *printf_number(char *str, long num, int32 base, int32 size, int32 precision, int32 type);
int vsnprintf(char *buf, int size, const char *fmt, va_list args);
int snprintf(char *buf, int size, const char *fmt, ...);
int vsprintf(char *, const char *, va_list);
int sprintf(char *, const char *, ...);
int printf(const char *, ...);


#endif	/* __STDIO_H */
