/* ps2-load-ip

   sprintf.c
   Copyright (C)2002 Dan Potter
   License: BSD

   $Id: sprintf.c,v 1.1 2002/10/30 05:34:13 bardtx Exp $
*/

#include <stdio.h>
#include <stdarg.h>

int sprintf(char *out, const char *fmt, ...) {
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(out, fmt, args);
	va_end(args);

	return i;
}
