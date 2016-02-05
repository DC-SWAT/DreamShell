/* KallistiOS ##version##

   dc/biosfont.h
   Copyright (C)2000-2001,2004 Dan Potter

   $Id: biosfont.h,v 1.3 2002/06/27 23:24:43 bardtx Exp $

*/


#ifndef __DC_BIOSFONT_H
#define __DC_BIOSFONT_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

uint8 *bfont_find_char(int ch);

void bfont_draw(uint16 *buffer, uint16 fg, uint16 bg, int c);
void bfont_draw_str(uint16 *buffer, uint16 fg, uint16 bg, const char *str);

__END_DECLS

#endif  /* __DC_BIOSFONT_H */
