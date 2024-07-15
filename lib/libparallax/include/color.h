/* Parallax for KallistiOS ##version##

   color.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_COLOR
#define __PARALLAX_COLOR

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Color handling routines. This is all just wrappers for PVR stuff
  at the moment to ease portability.
 */

#include <dc/pvr.h>

static inline uint32 plx_pack_color(float a, float r, float g, float b) {
	return PVR_PACK_COLOR(a, r, g, b);
}
__END_DECLS

#endif	/* __PARALLAX_COLOR */
