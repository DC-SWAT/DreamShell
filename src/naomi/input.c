/** \file    naomi/input.c
    \brief   NAOMI JVS input utilities.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#include <string.h>
#include <dc/maple/mie.h>
#include <naomi/input.h>

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

static const uint16_t jvs_player_bit[DS_JVS_MAP_PLAYER] = {
    MIE_JVS_START_BIT,
    MIE_JVS_UP_BIT,
    MIE_JVS_DOWN_BIT,
    MIE_JVS_LEFT_BIT,
    MIE_JVS_RIGHT_BIT,
    MIE_JVS_SW2_BIT,
    MIE_JVS_SW1_BIT,
    MIE_JVS_SW9_BIT,
    MIE_JVS_SW10_BIT,
    MIE_JVS_SW11_BIT,
    MIE_JVS_SW12_BIT,
    MIE_JVS_SW13_BIT,
    MIE_JVS_SW14_BIT,
    MIE_JVS_SW15_BIT,
    MIE_JVS_SW16_BIT,
    MIE_JVS_SERVICE_BIT
};

static uint16_t active_jvs_map[DS_JVS_MAP_SRC];

static int jvs_src_down(const mie_jvs_inputs_t *jvs, int src) {
    int extra;

    if(src < DS_JVS_MAP_PLAYER) {
        return (jvs->p1.raw & jvs_player_bit[src]) != 0;
    }

    if(src < DS_JVS_MAP_PANEL) {
        return (jvs->p2.raw & jvs_player_bit[src - DS_JVS_MAP_PLAYER]) != 0;
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
    if(mie_analog_calib_active()) {
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
        if(jvs_src_down(jvs, i)) {
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
