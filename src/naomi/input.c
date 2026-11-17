/** \file    naomi/input.c
    \brief   NAOMI JVS input utilities.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dc/maple/mie.h>
#include <naomi/input.h>
#include "utils.h"

#define JVS_WHEEL_UNUSED_DIFF 0x500
#define JVS_ANALOG_CENTER     0x8000
#define JVS_STICK_MIN         (-128)
#define JVS_STICK_MAX         127
#define JVS_PEDAL_SHIFT       8
#define JVS_ANALOG_WHEEL      0
#define JVS_ANALOG_ACCEL      1
#define JVS_ANALOG_BRAKE      2

#define JVS_PAD_MASK (CONT_A | CONT_B | CONT_X | CONT_Y | CONT_Z | CONT_C | \
                      CONT_D | CONT_START | CONT_DPAD_UP | CONT_DPAD_DOWN | \
                      CONT_DPAD_LEFT | CONT_DPAD_RIGHT)

static const uint8_t jvs_player_bit_num[DS_JVS_MAP_PLAYER] = {
    DS_JVS_MAP_PLAYER_BITS
};

static uint16_t active_jvs_map[DS_JVS_MAP_SRC];

int JvsMapSrcDown(const mie_jvs_inputs_t *jvs, int src) {
    int extra;
    uint16_t bit;

    if(!jvs || src < 0 || src >= DS_JVS_MAP_BUTTON) {
        return 0;
    }

    if(src < DS_JVS_MAP_PLAYER) {
        bit = (uint16_t)(1u << jvs_player_bit_num[src]);
        return (jvs->p1.raw & bit) != 0;
    }

    if(src < DS_JVS_MAP_PANEL) {
        bit = (uint16_t)(1u << jvs_player_bit_num[src - DS_JVS_MAP_PLAYER]);
        return (jvs->p2.raw & bit) != 0;
    }

    extra = src - DS_JVS_MAP_PANEL;
    if(extra == 0) {
        return (jvs->panel.psw.raw & MIE_JVS_PANEL_TEST_BIT) != 0;
    }
    if(extra == 1) {
        return (jvs->panel.psw.raw & MIE_JVS_PANEL_SERVICE_BIT) != 0;
    }

    return (jvs->system.raw & MIE_JVS_SYS_TEST_BIT) != 0;
}

static int wheel_unused(const mie_jvs_inputs_t *jvs) {
    uint16_t wheel;
    uint16_t accel;
    uint16_t diff;

    wheel = (uint16_t)(jvs->wheel & MIE_JVS_ANALOG_RAW_MASK);
    accel = (uint16_t)(jvs->accel & MIE_JVS_ANALOG_RAW_MASK);
    if(wheel > accel) {
        diff = wheel - accel;
    }
    else {
        diff = accel - wheel;
    }

    return diff <= JVS_WHEEL_UNUSED_DIFF;
}

static int analog_centered(uint16_t raw) {
    int32_t centered;

    raw = (uint16_t)(raw & MIE_JVS_ANALOG_RAW_MASK);
    centered = (int32_t)raw - JVS_ANALOG_CENTER;
    centered = centered * JVS_STICK_MAX / JVS_ANALOG_CENTER;
    if(centered < JVS_STICK_MIN) {
        return JVS_STICK_MIN;
    }
    if(centered > JVS_STICK_MAX) {
        return JVS_STICK_MAX;
    }

    return (int)centered;
}

static int axis_is_trigger(uint16_t axis) {
    return axis == DS_JVS_AXIS_LTRIG || axis == DS_JVS_AXIS_RTRIG;
}

static int analog_channel_value(const mie_jvs_inputs_t *jvs, int ch, int as_trigger) {
    uint16_t raw;

    raw = jvs->analog[ch];
    if(ch <= JVS_ANALOG_BRAKE && !mie_analog_calib_valid()) {
        return 0;
    }
    if(mie_analog_calib_active() && ch > JVS_ANALOG_BRAKE) {
        return (int)(raw & MIE_JVS_ANALOG_RAW_MASK);
    }
    if(ch == JVS_ANALOG_WHEEL) {
        return mie_analog_norm_wheel(raw);
    }
    if(ch == JVS_ANALOG_ACCEL) {
        return mie_analog_norm_accel(raw);
    }
    if(ch == JVS_ANALOG_BRAKE) {
        return mie_analog_norm_brake(raw);
    }
    if(as_trigger) {
        return (int)((raw & MIE_JVS_ANALOG_RAW_MASK) >> JVS_PEDAL_SHIFT);
    }

    return analog_centered(raw);
}

static void axis_write(cont_state_t *cont, uint16_t axis, int value) {
    switch(axis) {
    case DS_JVS_AXIS_JOYX:
        cont->joyx = value;
        break;
    case DS_JVS_AXIS_JOYY:
        cont->joyy = value;
        break;
    case DS_JVS_AXIS_JOY2X:
        cont->joy2x = value;
        break;
    case DS_JVS_AXIS_JOY2Y:
        cont->joy2y = value;
        break;
    case DS_JVS_AXIS_LTRIG:
        cont->ltrig = value;
        break;
    case DS_JVS_AXIS_RTRIG:
        cont->rtrig = value;
        break;
    default:
        break;
    }
}

static void jvs_cont_map(const mie_jvs_inputs_t *jvs, cont_state_t *cont) {
    uint32_t buttons = 0;
    int i;
    int ch;
    uint16_t slot;

    for(i = 0; i < DS_JVS_MAP_BUTTON; i++) {
        slot = active_jvs_map[i];
        if(slot == 0 || (slot & ~JVS_PAD_MASK) != 0) {
            continue;
        }
        if(JvsMapSrcDown(jvs, i)) {
            buttons |= slot;
        }
    }

    cont->buttons = buttons;
    cont->joyx = 0;
    cont->joyy = 0;
    cont->joy2x = 0;
    cont->joy2y = 0;
    cont->ltrig = 0;
    cont->rtrig = 0;

    for(ch = 0; ch < DS_JVS_MAP_ANALOG; ch++) {
        slot = active_jvs_map[DS_JVS_MAP_BUTTON + ch];
        if(slot == DS_JVS_AXIS_NONE || slot >= DS_JVS_AXIS_COUNT) {
            continue;
        }
        if(ch <= JVS_ANALOG_BRAKE && !mie_analog_calib_valid()) {
            continue;
        }
        if(ch == JVS_ANALOG_WHEEL && wheel_unused(jvs)) {
            continue;
        }
        axis_write(cont, slot, analog_channel_value(jvs, ch, axis_is_trigger(slot)));
    }
}

void ApplyJvsButtonMap(const uint16_t *map) {
    int i;
    int custom = 0;

    if(!map) {
        mie_set_cont_map(NULL);
        return;
    }

    for(i = 0; i < DS_JVS_MAP_BUTTON; i++) {
        if((map[i] & ~JVS_PAD_MASK) != 0) {
            mie_set_cont_map(NULL);
            return;
        }
        if(map[i] != 0) {
            custom = 1;
        }
    }

    for(; i < DS_JVS_MAP_SRC; i++) {
        if(map[i] >= DS_JVS_AXIS_COUNT) {
            mie_set_cont_map(NULL);
            return;
        }
        if(map[i] != DS_JVS_AXIS_NONE) {
            custom = 1;
        }
    }

    if(!custom) {
        mie_set_cont_map(NULL);
        return;
    }

    memcpy(active_jvs_map, map, sizeof(active_jvs_map));
    mie_set_cont_map(jvs_cont_map);
}

#define JVS_MAP_FILE_CAP     2048
#define JVS_MAP_FILE_ALIGN   32
#define JVS_MAP_GAME_MAX     128
#define JVS_REMAP_INDEX_BASE 1

static const char *const jvs_slot_token[DS_JVS_MAP_SRC] = {
    "P1_START", "P1_UP", "P1_DOWN", "P1_LEFT", "P1_RIGHT",
    "P1_BTN1", "P1_BTN2", "P1_BTN3", "P1_BTN4", "P1_BTN5",
    "P1_BTN6", "P1_BTN7", "P1_BTN8", "P1_BTN9", "P1_BTN10",
    "P1_SERVICE",
    "P2_START", "P2_UP", "P2_DOWN", "P2_LEFT", "P2_RIGHT",
    "P2_BTN1", "P2_BTN2", "P2_BTN3", "P2_BTN4", "P2_BTN5",
    "P2_BTN6", "P2_BTN7", "P2_BTN8", "P2_BTN9", "P2_BTN10",
    "P2_SERVICE",
    "TEST", "SERVICE", "SYS_TEST",
    "WHEEL", "ACCEL", "BRAKE", "CH3", "CH4", "CH5", "CH6", "CH7"
};

static const char *const jvs_slot_name[DS_JVS_MAP_SRC] = {
    "P1 Start", "P1 Up", "P1 Down", "P1 Left", "P1 Right",
    "P1 BTN1", "P1 BTN2", "P1 BTN3", "P1 BTN4", "P1 BTN5",
    "P1 BTN6", "P1 BTN7", "P1 BTN8", "P1 BTN9", "P1 BTN10",
    "P1 Service",
    "P2 Start", "P2 Up", "P2 Down", "P2 Left", "P2 Right",
    "P2 BTN1", "P2 BTN2", "P2 BTN3", "P2 BTN4", "P2 BTN5",
    "P2 BTN6", "P2 BTN7", "P2 BTN8", "P2 BTN9", "P2 BTN10",
    "P2 Service",
    "Test", "Service", "Sys Test",
    "Wheel", "Accel", "Brake", "CH3", "CH4", "CH5", "CH6", "CH7"
};

static const uint16_t pad_cycle[] = {
    0,
    CONT_A, CONT_B, CONT_X, CONT_Y, CONT_Z, CONT_C, CONT_D, CONT_START,
    CONT_DPAD_UP, CONT_DPAD_DOWN, CONT_DPAD_LEFT, CONT_DPAD_RIGHT
};

static const char *const pad_token[] = {
    "NONE",
    "A", "B", "X", "Y", "Z", "C", "D", "START",
    "UP", "DOWN", "LEFT", "RIGHT"
};

static const char *const pad_name[] = {
    "---",
    "A", "B", "X", "Y", "Z", "C", "D", "Start",
    "Up", "Down", "Left", "Right"
};

#define PAD_CYCLE_COUNT ((int)(sizeof(pad_cycle) / sizeof(pad_cycle[0])))

static const uint16_t axis_cycle[] = {
    DS_JVS_AXIS_NONE,
    DS_JVS_AXIS_JOYX,
    DS_JVS_AXIS_JOYY,
    DS_JVS_AXIS_JOY2X,
    DS_JVS_AXIS_JOY2Y,
    DS_JVS_AXIS_LTRIG,
    DS_JVS_AXIS_RTRIG
};

static const char *const axis_token[] = {
    "NONE", "X", "Y", "X2", "Y2", "L", "R"
};

static const char *const axis_name[] = {
    "---", "X", "Y", "X2", "Y2", "L", "R"
};

#define AXIS_CYCLE_COUNT ((int)(sizeof(axis_cycle) / sizeof(axis_cycle[0])))

static const uint16_t jvs_player_pad[DS_JVS_MAP_PLAYER] = {
    CONT_START,
    CONT_DPAD_UP,
    CONT_DPAD_DOWN,
    CONT_DPAD_LEFT,
    CONT_DPAD_RIGHT,
    CONT_B,
    CONT_X,
    0, 0, 0, 0, 0,
    CONT_Y,
    0,
    CONT_A,
    CONT_Z
};

static int token_eq(const char *a, const char *b) {
    unsigned char ca;
    unsigned char cb;

    if(!a || !b) {
        return 0;
    }

    while(*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if(ca >= 'A' && ca <= 'Z') {
            ca = (unsigned char)(ca - 'A' + 'a');
        }
        if(cb >= 'A' && cb <= 'Z') {
            cb = (unsigned char)(cb - 'A' + 'a');
        }
        if(ca != cb) {
            return 0;
        }
    }

    return *a == *b;
}

static int parse_ch(const char *token, int *ch) {
    if(!token || !ch) {
        return 0;
    }
    if((token[0] != 'c' && token[0] != 'C') ||
            (token[1] != 'h' && token[1] != 'H')) {
        return 0;
    }
    if(token[2] < '0' || token[2] > '7' || token[3] != '\0') {
        return 0;
    }

    *ch = token[2] - '0';
    return 1;
}

static int slot_by_token(const char *token) {
    int i;
    int ch;

    if(parse_ch(token, &ch)) {
        return DS_JVS_MAP_BUTTON + ch;
    }

    for(i = 0; i < DS_JVS_MAP_SRC; i++) {
        if(token_eq(token, jvs_slot_token[i])) {
            return i;
        }
    }

    return -1;
}

static char *trim_token(char *s) {
    char *end;

    while(*s == ' ' || *s == '\t' || *s == '\r') {
        s++;
    }

    end = s + strlen(s);
    while(end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
        *--end = '\0';
    }

    return s;
}

const char *JvsMapSlotName(int idx) {
    if(idx < 0 || idx >= DS_JVS_MAP_SRC) {
        return "---";
    }

    return jvs_slot_name[idx];
}

static int jvs_remap_index(uint16_t code) {
    int dst;

    if(code == DS_JVS_REMAP_KEEP || code == DS_JVS_REMAP_NONE) {
        return -1;
    }

    dst = (int)code - JVS_REMAP_INDEX_BASE;
    if(dst < 0 || dst >= DS_JVS_MAP_SRC) {
        return -1;
    }

    return dst;
}

static uint16_t jvs_remap_code(int dst) {
    return (uint16_t)(dst + JVS_REMAP_INDEX_BASE);
}

const char *JvsMapValueName(ds_jvs_map_kind_t kind, int src, uint16_t value) {
    int i;
    int dst;

    if(kind == DS_JVS_MAP_KIND_JVS) {
        if(value == DS_JVS_REMAP_KEEP) {
            return "Keep";
        }
        if(value == DS_JVS_REMAP_NONE) {
            return "None";
        }
        dst = jvs_remap_index(value);
        return JvsMapSlotName(dst);
    }

    if(src >= DS_JVS_MAP_BUTTON) {
        for(i = 0; i < AXIS_CYCLE_COUNT; i++) {
            if(axis_cycle[i] == value) {
                return axis_name[i];
            }
        }
        return "---";
    }

    for(i = 0; i < PAD_CYCLE_COUNT; i++) {
        if(pad_cycle[i] == value) {
            return pad_name[i];
        }
    }

    return "---";
}

static uint16_t cycle_value(const uint16_t *cycle, int count, uint16_t value, int dir) {
    int slot = 0;
    int i;

    for(i = 0; i < count; i++) {
        if(cycle[i] == value) {
            slot = i;
            break;
        }
    }

    slot += dir;
    if(slot < 0) {
        slot = count - 1;
    }
    if(slot >= count) {
        slot = 0;
    }

    return cycle[slot];
}

static uint16_t cycle_jvs(int src, uint16_t value, int dir) {
    int count;
    int base;
    int slot = 0;
    int i;
    uint16_t item;

    if(src >= DS_JVS_MAP_BUTTON) {
        count = DS_JVS_MAP_ANALOG;
        base = DS_JVS_MAP_BUTTON;
    }
    else {
        count = DS_JVS_MAP_BUTTON;
        base = 0;
    }
    count += 2;

    for(i = 0; i < count; i++) {
        if(i == 0) {
            item = DS_JVS_REMAP_KEEP;
        }
        else if(i == 1) {
            item = DS_JVS_REMAP_NONE;
        }
        else {
            item = jvs_remap_code(base + (i - 2));
        }
        if(item == value) {
            slot = i;
            break;
        }
    }

    slot += dir;
    if(slot < 0) {
        slot = count - 1;
    }
    if(slot >= count) {
        slot = 0;
    }

    if(slot == 0) {
        return DS_JVS_REMAP_KEEP;
    }
    if(slot == 1) {
        return DS_JVS_REMAP_NONE;
    }

    return jvs_remap_code(base + (slot - 2));
}

uint16_t JvsMapCycle(ds_jvs_map_kind_t kind, int src, uint16_t value, int dir) {
    if(src < 0 || src >= DS_JVS_MAP_SRC) {
        return value;
    }

    if(kind == DS_JVS_MAP_KIND_JVS) {
        return cycle_jvs(src, value, dir);
    }

    if(src >= DS_JVS_MAP_BUTTON) {
        return cycle_value(axis_cycle, AXIS_CYCLE_COUNT, value, dir);
    }

    return cycle_value(pad_cycle, PAD_CYCLE_COUNT, value, dir);
}

void JvsMapDefault(ds_jvs_map_kind_t kind, uint16_t *map) {
    if(!map) {
        return;
    }

    memset(map, 0, DS_JVS_MAP_SRC * sizeof(*map));
    if(kind != DS_JVS_MAP_KIND_PAD) {
        return;
    }

    memcpy(map, jvs_player_pad, sizeof(jvs_player_pad));
    memcpy(map + DS_JVS_MAP_PLAYER, jvs_player_pad, sizeof(jvs_player_pad));
    map[DS_JVS_MAP_PANEL] = CONT_C;
    map[DS_JVS_MAP_PANEL + 1] = CONT_D;
    map[DS_JVS_MAP_BUTTON] = DS_JVS_AXIS_JOYX;
    map[DS_JVS_MAP_BUTTON + 1] = DS_JVS_AXIS_RTRIG;
    map[DS_JVS_MAP_BUTTON + 2] = DS_JVS_AXIS_LTRIG;
}

static int pad_value_by_token(int src, const char *token, uint16_t *out) {
    int i;

    if(src >= DS_JVS_MAP_BUTTON) {
        for(i = 0; i < AXIS_CYCLE_COUNT; i++) {
            if(token_eq(token, axis_token[i])) {
                *out = axis_cycle[i];
                return 1;
            }
        }
        return 0;
    }

    for(i = 0; i < PAD_CYCLE_COUNT; i++) {
        if(token_eq(token, pad_token[i])) {
            *out = pad_cycle[i];
            return 1;
        }
    }

    return 0;
}

static int jvs_value_by_token(int src, const char *token, uint16_t *out) {
    int dst;

    if(token_eq(token, "KEEP")) {
        *out = DS_JVS_REMAP_KEEP;
        return 1;
    }
    if(token_eq(token, "NONE")) {
        *out = DS_JVS_REMAP_NONE;
        return 1;
    }

    dst = slot_by_token(token);
    if(dst < 0) {
        return 0;
    }
    if(src >= DS_JVS_MAP_BUTTON) {
        if(dst < DS_JVS_MAP_BUTTON) {
            return 0;
        }
    }
    else if(dst >= DS_JVS_MAP_BUTTON) {
        return 0;
    }

    *out = jvs_remap_code(dst);
    return 1;
}

static int parse_kind(const char *value, ds_jvs_map_kind_t *kind) {
    if(token_eq(value, "pad")) {
        *kind = DS_JVS_MAP_KIND_PAD;
        return 1;
    }
    if(token_eq(value, "jvs")) {
        *kind = DS_JVS_MAP_KIND_JVS;
        return 1;
    }

    return 0;
}

static void apply_map_line(ds_jvs_map_kind_t kind, uint16_t *map, char *line) {
    char *eq;
    char *key;
    char *val;
    int src;
    uint16_t value;

    eq = strchr(line, '=');
    if(!eq) {
        return;
    }

    *eq = '\0';
    key = trim_token(line);
    val = trim_token(eq + 1);
    if(token_eq(key, "type")) {
        return;
    }

    src = slot_by_token(key);
    if(src < 0) {
        return;
    }

    if(kind == DS_JVS_MAP_KIND_JVS) {
        if(!jvs_value_by_token(src, val, &value)) {
            return;
        }
    }
    else if(!pad_value_by_token(src, val, &value)) {
        return;
    }

    map[src] = value;
}

static int kind_from_text(char *text, ds_jvs_map_kind_t *kind) {
    char *cur = text;
    char *line;
    char *eq;
    char *key;
    char *val;

    while(*cur) {
        line = cur;
        eq = strchr(cur, '\n');
        if(eq) {
            *eq = '\0';
            cur = eq + 1;
        }
        else {
            cur += strlen(cur);
        }

        eq = strchr(line, '=');
        if(!eq) {
            continue;
        }
        *eq = '\0';
        key = trim_token(line);
        val = trim_token(eq + 1);
        if(token_eq(key, "type") && parse_kind(val, kind)) {
            return 1;
        }
    }

    return 0;
}

int LoadJvsMapFile(const char *fn, ds_jvs_map_kind_t *kind, uint16_t *map) {
    file_t fd;
    char *buf;
    char *scan;
    char *cur;
    char *line;
    char *nl;
    size_t size;
    ds_jvs_map_kind_t parsed;

    if(!fn || !kind || !map) {
        return -1;
    }

    fd = fs_open(fn, O_RDONLY);
    if(fd == FILEHND_INVALID) {
        return -1;
    }

    size = fs_total(fd);
    {
        size_t bytes = size + 1;
        size_t rem = bytes % JVS_MAP_FILE_ALIGN;

        if(rem) {
            bytes += JVS_MAP_FILE_ALIGN - rem;
        }
        buf = (char *)aligned_alloc(JVS_MAP_FILE_ALIGN, bytes);
    }
    if(!buf) {
        fs_close(fd);
        return -1;
    }
    if(fs_read(fd, buf, size) != (ssize_t)size) {
        fs_close(fd);
        free(buf);
        return -1;
    }
    fs_close(fd);
    buf[size] = '\0';

    scan = strdup(buf);
    if(!scan) {
        free(buf);
        return -1;
    }
    if(!kind_from_text(scan, &parsed)) {
        free(scan);
        free(buf);
        return -1;
    }
    free(scan);

    JvsMapDefault(parsed, map);
    cur = buf;
    while(*cur) {
        line = cur;
        nl = strchr(cur, '\n');
        if(nl) {
            *nl = '\0';
            cur = nl + 1;
        }
        else {
            cur += strlen(cur);
        }
        apply_map_line(parsed, map, line);
    }

    free(buf);
    *kind = parsed;
    return 0;
}

static const char *pad_value_token(int src, uint16_t value) {
    int i;

    if(src >= DS_JVS_MAP_BUTTON) {
        for(i = 0; i < AXIS_CYCLE_COUNT; i++) {
            if(axis_cycle[i] == value) {
                return axis_token[i];
            }
        }
        return NULL;
    }

    for(i = 0; i < PAD_CYCLE_COUNT; i++) {
        if(pad_cycle[i] == value) {
            return pad_token[i];
        }
    }

    return NULL;
}

static const char *jvs_value_token(uint16_t value) {
    int dst;

    if(value == DS_JVS_REMAP_KEEP) {
        return "KEEP";
    }
    if(value == DS_JVS_REMAP_NONE) {
        return "NONE";
    }

    dst = jvs_remap_index(value);
    if(dst < 0) {
        return NULL;
    }

    return jvs_slot_token[dst];
}

static int game_label(const char *game, char *dst, int cap) {
    const char *end;
    int n;

    if(!game || cap < 2) {
        return 0;
    }
    while(*game == ' ' || *game == '\t') {
        game++;
    }
    end = game;
    while(*end && *end != '\n' && *end != '\r') {
        end++;
    }
    while(end > game && (end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    n = (int)(end - game);
    if(n <= 0 || n >= cap) {
        return 0;
    }
    memcpy(dst, game, (size_t)n);
    dst[n] = '\0';
    return 1;
}

static int buf_add_pair(char *buf, int cap, int used, const char *key, const char *val) {
    int n;

    if(used < 0 || used >= cap) {
        return -1;
    }

    n = snprintf(buf + used, (size_t)(cap - used), "%s = %s\n", key, val);
    if(n < 0 || used + n >= cap) {
        return -1;
    }

    return used + n;
}

static int map_parent(const char *fn, char *dir, size_t len) {
    const char *slash;
    size_t n;

    slash = strrchr(fn, '/');
    if(!slash || slash == fn) {
        return -1;
    }

    n = (size_t)(slash - fn);
    if(n >= len) {
        return -1;
    }

    memcpy(dir, fn, n);
    dir[n] = '\0';
    return 0;
}

int SaveJvsMapFile(const char *fn, ds_jvs_map_kind_t kind, const uint16_t *map,
        const char *game) {
    uint16_t def[DS_JVS_MAP_SRC];
    char buf[JVS_MAP_FILE_CAP];
    char label[JVS_MAP_GAME_MAX];
    char dir[NAME_MAX];
    const char *value;
    file_t fd;
    int used;
    int i;

    if(!fn || !map) {
        return -1;
    }
    if(kind != DS_JVS_MAP_KIND_PAD && kind != DS_JVS_MAP_KIND_JVS) {
        return -1;
    }

    JvsMapDefault(kind, def);
    used = buf_add_pair(buf, JVS_MAP_FILE_CAP, 0, "type",
        kind == DS_JVS_MAP_KIND_JVS ? "jvs" : "pad");
    if(used < 0) {
        return -1;
    }
    if(game_label(game, label, sizeof(label))) {
        used = buf_add_pair(buf, JVS_MAP_FILE_CAP, used, "game", label);
        if(used < 0) {
            return -1;
        }
    }

    for(i = 0; i < DS_JVS_MAP_SRC; i++) {
        if(map[i] == def[i]) {
            continue;
        }
        if(kind == DS_JVS_MAP_KIND_JVS) {
            value = jvs_value_token(map[i]);
        }
        else {
            value = pad_value_token(i, map[i]);
        }
        if(!value) {
            continue;
        }
        used = buf_add_pair(buf, JVS_MAP_FILE_CAP, used, jvs_slot_token[i], value);
        if(used < 0) {
            return -1;
        }
    }

    if(map_parent(fn, dir, sizeof(dir)) == 0) {
        mkpath(dir);
    }

    fd = fs_open(fn, O_CREAT | O_TRUNC | O_WRONLY);
    if(fd == FILEHND_INVALID) {
        return -1;
    }
    if(fs_write(fd, buf, (size_t)used) != (ssize_t)used) {
        fs_close(fd);
        return -1;
    }
    fs_close(fd);
    return 0;
}
