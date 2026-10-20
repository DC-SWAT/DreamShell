/** \file    naomi/input.h
    \brief   NAOMI JVS input utilities.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#ifndef __NAOMI_INPUT_H
#define __NAOMI_INPUT_H

#include <stdint.h>
#include <dc/maple/mie.h>

#define DS_JVS_MAP_PLAYER 16
#define DS_JVS_MAP_PANEL  (DS_JVS_MAP_PLAYER * 2)
#define DS_JVS_MAP_BUTTON (DS_JVS_MAP_PANEL + 3)
#define DS_JVS_MAP_ANALOG MIE_JVS_ANALOG_CHANNELS
#define DS_JVS_MAP_SRC    (DS_JVS_MAP_BUTTON + DS_JVS_MAP_ANALOG)

typedef enum {
	DS_JVS_AXIS_NONE = 0,
	DS_JVS_AXIS_JOYX,
	DS_JVS_AXIS_JOYY,
	DS_JVS_AXIS_JOY2X,
	DS_JVS_AXIS_JOY2Y,
	DS_JVS_AXIS_LTRIG,
	DS_JVS_AXIS_RTRIG,
	DS_JVS_AXIS_COUNT
} ds_jvs_axis_t;

void ApplyJvsButtonMap(const uint16_t *map);

#endif
