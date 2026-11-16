/** \file    naomi/jvs_map.h
    \brief   Shared NAOMI JVS map layout.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#ifndef __NAOMI_JVS_MAP_H
#define __NAOMI_JVS_MAP_H

#include <stdint.h>

#define DS_JVS_MAP_PLAYER 16
#define DS_JVS_MAP_PANEL  (DS_JVS_MAP_PLAYER * 2)
#define DS_JVS_MAP_BUTTON (DS_JVS_MAP_PANEL + 3)
#define DS_JVS_MAP_ANALOG 8
#define DS_JVS_MAP_SRC    (DS_JVS_MAP_BUTTON + DS_JVS_MAP_ANALOG)

#define DS_JVS_REMAP_KEEP 0
#define DS_JVS_REMAP_NONE 0xFF

#define DS_JVS_MAP_PLAYER_BITS \
	15, 13, 12, 11, 10, \
	9, 8, \
	0, 1, 2, 3, 4, 5, 6, 7, \
	14

#endif
