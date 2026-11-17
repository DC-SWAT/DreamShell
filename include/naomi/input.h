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
#include <naomi/jvs_map.h>

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

typedef enum {
	DS_JVS_MAP_KIND_PAD = 0,
	DS_JVS_MAP_KIND_JVS
} ds_jvs_map_kind_t;

void ApplyJvsButtonMap(const uint16_t *map);

int JvsMapSrcDown(const mie_jvs_inputs_t *jvs, int src);

const char *JvsMapSlotName(int idx);

const char *JvsMapValueName(ds_jvs_map_kind_t kind, int src, uint16_t value);

uint16_t JvsMapCycle(ds_jvs_map_kind_t kind, int src, uint16_t value, int dir);

void JvsMapDefault(ds_jvs_map_kind_t kind, uint16_t *map);

int LoadJvsMapFile(const char *fn, ds_jvs_map_kind_t *kind, uint16_t *map);

int SaveJvsMapFile(const char *fn, ds_jvs_map_kind_t kind, const uint16_t *map,
    const char *game);

#endif
