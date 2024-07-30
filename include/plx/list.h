/* Parallax for KallistiOS ##version##

   list.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_LIST
#define __PARALLAX_LIST

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file List specification. These are more wrappers for PVR stuff to help
  make porting easier. This is really pretty DC specific, but it will help
  a porting layer to sort out what's going on anyway, and avoid having
  to #define more names.
 */

#include <dc/pvr.h>

#define PLX_LIST_OP_POLY	PVR_LIST_OP_POLY
#define PLX_LIST_TR_POLY	PVR_LIST_TR_POLY
#define PLX_LIST_OP_MOD		PVR_LIST_OP_MOD
#define PLX_LIST_TR_MOD		PVR_LIST_TR_MOD
#define PLX_LIST_PT_POLY	PVR_LIST_PT_POLY

__END_DECLS

#endif	/* __PARALLAX_LIST */
