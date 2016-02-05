/* KallistiOS ##version##

 conio.h

 (c)2002 Dan Potter

 Adapted from Kosh, (c)2000 Jordan DeLong, (c)2014 SWAT

*/

#ifndef __CONIO_H
#define __CONIO_H

#include <sys/cdefs.h>
__BEGIN_DECLS


/* functions */
void conio_scroll();
void conio_putch(int ch);
void conio_putstr(char *str);
int conio_printf(const char *fmt, ...);
void conio_clear();


__END_DECLS

#endif /* __CONIO_H */
