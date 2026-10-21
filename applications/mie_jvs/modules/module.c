/* DreamShell

   module.c - MIE JVS test and calibration app
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <settings.h>
#include <sfx.h>
#include <string.h>
#include <naomi/coins.h>
#include <naomi/input.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mie.h>
#include <tsunami/tsunami.h>

#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_mie_jvs);

#define SCREEN_WIDTH           640
#define SCREEN_HEIGHT          480
#define WHEEL_BAR_W            220.0f
#define WHEEL_BAR_H            14.0f
#define WHEEL_BAR_CENTER_Z     70.5f
#define WHEEL_BAR_CENTER_EXT   4.0f
#define PEDAL_BAR_W            40.0f
#define PEDAL_BAR_H            128.0f
#define CALIB_WHEEL_BAR_X      120.0f
#define CALIB_WHEEL_BAR_Y      300.0f
#define CALIB_PEDAL_BAR_Y      192.0f
#define CALIB_PEDAL_BAR_BOTTOM (CALIB_PEDAL_BAR_Y + PEDAL_BAR_H)
#define CALIB_PEDAL_X_BRAKE    380.0f
#define CALIB_PEDAL_X_ACCEL    500.0f
#define OUTPUT_BLINK_MS        500
#define MIE_LINE_MAX           128
#define OUTPUT_BIT(index)      MIE_JVS_OUTPUT_MASK(index)
#define MIE_RAW_MASK           0xFF7F
#define SAVE_KIND_MAP          0
#define SAVE_KIND_CALIB        1
#define SAVE_KIND_RESET        2
#define MENU_ITEM_X            148.0f
#define MENU_ITEM_Y0           168.0f
#define MENU_ITEM_STEP         40.0f
#define MENU_CURSOR_X          120.0f
#define MENU_CURSOR_W          400.0f
#define MENU_CURSOR_H          32.0f
#define MENU_CURSOR_BELOW      8.0f
#define MAP_VISIBLE            12
#define MAP_HDR_Y              68.0f
#define MAP_SRC_X              48.0f
#define MAP_DST_X              400.0f
#define MAP_POS_X              560.0f
#define MAP_ROW_Y0             96.0f
#define MAP_ROW_STEP           24.0f
#define MAP_CURSOR_X           32.0f
#define MAP_CURSOR_W           576.0f
#define MAP_CURSOR_H           22.0f
#define MAP_CURSOR_BELOW       4.0f
#define MAP_AXIS_DEADZONE      48
#define MAP_AXIS_HOLD_MS       200
#define EXIT_LABEL_Y           24.0f
#define EXIT_LABEL_MARGIN      8.0f
#define HELD_BUTTONS           0xFFFFFFFFU

typedef struct {
    Label *wheel_title;
    Label *wheel_val;
    Label *accel_title;
    Label *accel_val;
    Label *brake_title;
    Label *brake_val;
    Rectangle *wheel_bar_bg;
    Rectangle *wheel_bar_fill;
    Rectangle *wheel_bar_center;
    Rectangle *accel_bar_bg;
    Rectangle *accel_bar_fill;
    Rectangle *brake_bar_bg;
    Rectangle *brake_bar_fill;
} MieAnalogUi_t;

typedef enum {
    MIE_VIEW_MENU = 0,
    MIE_VIEW_MONITOR,
    MIE_VIEW_CALIB,
    MIE_VIEW_OUTPUTS,
    MIE_VIEW_MAP,
    MIE_VIEW_COUNT
} mie_view_t;

typedef enum {
    MIE_MENU_MONITOR = 0,
    MIE_MENU_CALIB,
    MIE_MENU_OUTPUTS,
    MIE_MENU_MAP,
    MIE_MENU_EXIT,
    MIE_MENU_COUNT
} mie_menu_t;

typedef struct {
    App_t *app;
    CardStack *pages;
    Label *title_label;
    Label *page_label;
    Label *status_label;
    Label *hint_label;
    Label *exit_label;
    Label *p1_label;
    Label *p1_svc;
    Label *p1_detail;
    Label *p2_label;
    Label *p2_svc;
    Label *p2_detail;
    Label *sys_label;
    Label *panel_label;
    Label *coin_label;
    Label *out_label;
    Label *outputs_label;
    Label *cont_title;
    Label *p1_title;
    Label *p2_title;
    Label *sys_title;
    Label *cont_label;
    Label *joy_label;
    Label *joy2_label;
    Label *analog_title;
    Label *analog_wheel;
    Label *analog_accel;
    Label *analog_brake;
    Label *monitor_calib_title;
    Label *monitor_calib;
    Label *monitor_calib2;
    Label *calib_title;
    Label *calib_step;
    Label *calib_raw;
    Label *calib_data;
    MieAnalogUi_t calib_ui;
    Rectangle *outputs_led[MIE_JVS_OUTPUT_COUNT];
    Label *outputs_num[MIE_JVS_OUTPUT_COUNT];
    Label *menu_item[MIE_MENU_COUNT];
    Rectangle *menu_cursor;
    Label *map_hdr_src;
    Label *map_hdr_dst;
    Label *map_pos;
    Label *map_src[MAP_VISIBLE];
    Label *map_dst[MAP_VISIBLE];
    Rectangle *map_cursor;
    Dialog *save_dialog;
    Event_t *input_event;
    Event_t *video_event;
    int exiting;
    mie_view_t view;
    int menu_sel;
    int map_sel;
    int map_scroll;
    int map_axis_dir;
    int map_axis_arm;
    uint64_t map_axis_ms;
    int map_dirty;
    int calib_dirty;
    int map_prompt;
    int save_kind;
    int map_after_exit;
    mie_view_t map_after;
    int panel_combo;
    uint16_t map[DS_JVS_MAP_SRC];
    int output_blink;
    uint8_t output_blink_step;
    uint64_t output_blink_last_ms;
    uint32_t output_manual;
    int output_sel;
    uint32_t old_btns;
    char board_id[MIE_ID_SIZE];
    uint16_t calib_wheel_min;
    uint16_t calib_wheel_max;
    uint16_t calib_wheel_center;
    uint16_t calib_accel_min;
    uint16_t calib_accel_max;
    uint16_t calib_brake_min;
    uint16_t calib_brake_max;
} MieJvsContext_t;

static MieJvsContext_t self;

static int prompt_map_save(mie_view_t view, int do_exit);
static int prompt_calib_save(mie_view_t view, int do_exit);
static mie_state_t *mie_dev_state(void);

static const char *port_mode_name(mie_port0_mode_t mode) {
    switch(mode) {
    case MIE_PORT0_JVS:
        return "JVS";
    case MIE_PORT0_MAPLE:
        return "Maple";
    default:
        return "unknown";
    }
}

static const char *calib_step_name(mie_analog_calib_step_t step) {
    switch(step) {
    case MIE_ANALOG_CALIB_WHEEL_CENTER:
        return "Hold wheel centered, press A or START";
    case MIE_ANALOG_CALIB_WHEEL:
        return "Turn wheel left and right, press A or START";
    case MIE_ANALOG_CALIB_ACCEL:
        return "Press accelerator fully, press A or START";
    case MIE_ANALOG_CALIB_BRAKE:
        return "Press brake fully, press A or START";
    default:
        return "Idle";
    }
}

static const char *view_name(mie_view_t view) {
    switch(view) {
    case MIE_VIEW_MENU:
        return "Menu";
    case MIE_VIEW_MONITOR:
        return "Monitor";
    case MIE_VIEW_CALIB:
        return "Calibration";
    case MIE_VIEW_OUTPUTS:
        return "Outputs";
    case MIE_VIEW_MAP:
        return "Mapping";
    default:
        return "";
    }
}

static const Color COL_TEXT = {1.0f, 1.0f, 1.0f, 1.0f};
static const Color COL_SEL = {1.0f, 0.831f, 0.945f, 0.161f};
static const Color COL_ACTIVE = {1.0f, 0.439f, 0.788f, 0.859f};
static const Color COL_EXIT = {1.0f, 1.0f, 0.533f, 0.533f};

static const char *const jvs_player_names[DS_JVS_MAP_PLAYER] = {
    "Start", "Up", "Down", "Left", "Right",
    "BTN1", "BTN2", "BTN3", "BTN4", "BTN5",
    "BTN6", "BTN7", "BTN8", "BTN9", "BTN10",
    "Service"
};

static const uint16_t pad_cycle[] = {
    0,
    CONT_A, CONT_B, CONT_X, CONT_Y, CONT_Z, CONT_C, CONT_D, CONT_START,
    CONT_DPAD_UP, CONT_DPAD_DOWN, CONT_DPAD_LEFT, CONT_DPAD_RIGHT
};

static const char *const pad_names[] = {
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

static const char *const axis_names[] = {
    "---", "X", "Y", "X2", "Y2", "L", "R"
};

#define AXIS_CYCLE_COUNT ((int)(sizeof(axis_cycle) / sizeof(axis_cycle[0])))

static const char *const jvs_analog_names[DS_JVS_MAP_ANALOG] = {
    "Wheel", "Accel", "Brake", "CH3", "CH4", "CH5", "CH6", "CH7"
};

static const char *pad_btn_name(uint16_t slot) {
    int i;

    for(i = 0; i < PAD_CYCLE_COUNT; i++) {
        if(pad_cycle[i] == slot) {
            return pad_names[i];
        }
    }

    return "---";
}

static const char *axis_name(uint16_t slot) {
    int i;

    for(i = 0; i < AXIS_CYCLE_COUNT; i++) {
        if(axis_cycle[i] == slot) {
            return axis_names[i];
        }
    }

    return "---";
}

static const char *map_slot_name(int idx, uint16_t slot) {
    if(idx >= DS_JVS_MAP_BUTTON) {
        return axis_name(slot);
    }

    return pad_btn_name(slot);
}

static void jvs_btn_label(char *dst, size_t len, int idx) {
    int extra;

    if(idx >= DS_JVS_MAP_BUTTON) {
        snprintf(dst, len, "%s", jvs_analog_names[idx - DS_JVS_MAP_BUTTON]);
        return;
    }

    if(idx < DS_JVS_MAP_PLAYER) {
        snprintf(dst, len, "P1 %s", jvs_player_names[idx]);
        return;
    }

    if(idx < DS_JVS_MAP_PANEL) {
        snprintf(dst, len, "P2 %s", jvs_player_names[idx - DS_JVS_MAP_PLAYER]);
        return;
    }

    extra = idx - DS_JVS_MAP_PANEL;
    if(extra == 0) {
        snprintf(dst, len, "Test");
        return;
    }
    if(extra == 1) {
        snprintf(dst, len, "Service");
        return;
    }

    snprintf(dst, len, "Sys Test");
}

static void place_bar(Rectangle *rect, float x, float y, float w, float h) {
    Vector pos;

    if(!rect) {
        return;
    }

    TSU_RectangleSetSize(rect, w, h);
    pos = (Vector){x, y, 60.0f, 1.0f};
    TSU_DrawableSetTranslate((Drawable *)rect, &pos);
}

static void place_label(Label *label, float x, float y) {
    Vector pos;

    if(!label) {
        return;
    }

    pos = (Vector){x, y, 80.0f, 1.0f};
    TSU_DrawableSetTranslate((Drawable *)label, &pos);
}

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

static int jvs_map_blank(const uint16_t *map) {
    int i;

    for(i = 0; i < DS_JVS_MAP_SRC; i++) {
        if(map[i] != 0) {
            return 0;
        }
    }

    return 1;
}

static void jvs_map_default(uint16_t *map) {
    memset(map, 0, DS_JVS_MAP_SRC * sizeof(*map));
    memcpy(map, jvs_player_pad, sizeof(jvs_player_pad));
    memcpy(map + DS_JVS_MAP_PLAYER, jvs_player_pad, sizeof(jvs_player_pad));
    map[DS_JVS_MAP_PANEL] = CONT_C;
    map[DS_JVS_MAP_PANEL + 1] = CONT_D;
    map[DS_JVS_MAP_BUTTON] = DS_JVS_AXIS_JOYX;
    map[DS_JVS_MAP_BUTTON + 1] = DS_JVS_AXIS_RTRIG;
    map[DS_JVS_MAP_BUTTON + 2] = DS_JVS_AXIS_LTRIG;
}

static int jvs_src_down(const mie_jvs_inputs_t *jvs, int src) {
    int extra;

    if(!jvs || src < 0 || src >= DS_JVS_MAP_BUTTON) {
        return 0;
    }

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

static int jvs_map_is_default(const uint16_t *map) {
    uint16_t def[DS_JVS_MAP_SRC];

    jvs_map_default(def);
    return memcmp(map, def, DS_JVS_MAP_SRC * sizeof(*map)) == 0;
}

static void save_btn_map(void) {
    Settings_t *settings;

    settings = GetSettings();
    if(jvs_map_is_default(self.map)) {
        memset(settings->jvs_map, 0, sizeof(settings->jvs_map));
    }
    else {
        memcpy(settings->jvs_map, self.map, sizeof(self.map));
    }
    SaveSettings();
    ApplyJvsButtonMap(settings->jvs_map);
    self.map_dirty = 0;
}

static void revert_map(void) {
    Settings_t *settings;

    settings = GetSettings();
    if(jvs_map_blank(settings->jvs_map)) {
        jvs_map_default(self.map);
    }
    else {
        memcpy(self.map, settings->jvs_map, sizeof(self.map));
    }
    ApplyJvsButtonMap(settings->jvs_map);
    self.map_dirty = 0;
}

static int prompt_map_save(mie_view_t view, int do_exit) {
    if(!self.map_dirty || self.map_prompt) {
        return self.map_prompt;
    }

    self.map_after = view;
    self.map_after_exit = do_exit;
    self.save_kind = SAVE_KIND_MAP;
    if(!self.save_dialog) {
        save_btn_map();
        return 0;
    }

    self.map_prompt = 1;
    TSU_DialogShow(self.save_dialog, "Save changed mapping to settings?");
    return 1;
}

static int calib_matches_settings(void) {
    Settings_t *settings;
    const mie_analog_calib_t *live;

    settings = GetSettings();
    live = mie_analog_calib_get();
    if(!settings || !live) {
        return 1;
    }
    return memcmp(live, &settings->analog, sizeof(settings->analog)) == 0;
}

static void save_calib(void) {
    Settings_t *settings;

    settings = GetSettings();
    if(!settings) {
        self.calib_dirty = 0;
        return;
    }
    if(mie_analog_calib_valid()) {
        settings->analog = *mie_analog_calib_get();
    }
    else {
        memset(&settings->analog, 0, sizeof(settings->analog));
    }
    SaveSettings();
    self.calib_dirty = 0;
}

static void revert_calib(void) {
    Settings_t *settings;

    settings = GetSettings();
    if(settings) {
        mie_analog_calib_set(&settings->analog);
    }
    else {
        mie_analog_calib_reset();
    }
    self.calib_dirty = 0;
}

static void mark_calib_dirty(void) {
    self.calib_dirty = !calib_matches_settings();
}

static int prompt_calib_save(mie_view_t view, int do_exit) {
    if(!self.calib_dirty || self.map_prompt) {
        return self.map_prompt;
    }

    self.map_after = view;
    self.map_after_exit = do_exit;
    self.save_kind = SAVE_KIND_CALIB;
    if(!self.save_dialog) {
        save_calib();
        return 0;
    }

    self.map_prompt = 1;
    TSU_DialogShow(self.save_dialog, "Save changed calibration to settings?");
    return 1;
}

static void map_apply(void) {
    uint16_t stored[DS_JVS_MAP_SRC];

    if(jvs_map_is_default(self.map)) {
        memset(stored, 0, sizeof(stored));
        ApplyJvsButtonMap(stored);
    }
    else {
        ApplyJvsButtonMap(self.map);
    }
    self.map_dirty = 1;
}

static void map_cycle(int dir) {
    const uint16_t *cycle;
    int count;
    int i;
    int slot;

    if(self.map_sel >= DS_JVS_MAP_BUTTON) {
        cycle = axis_cycle;
        count = AXIS_CYCLE_COUNT;
    }
    else {
        cycle = pad_cycle;
        count = PAD_CYCLE_COUNT;
    }

    slot = 0;
    for(i = 0; i < count; i++) {
        if(cycle[i] == self.map[self.map_sel]) {
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
    self.map[self.map_sel] = cycle[slot];
    map_apply();
}

static int map_axis_side(int axis) {
    if(axis <= -MAP_AXIS_DEADZONE) {
        return -1;
    }
    if(axis >= MAP_AXIS_DEADZONE) {
        return 1;
    }
    return 0;
}

static int map_pad_axis(void) {
    int axis = 0;
    mie_state_t *st;

    st = mie_dev_state();
    if(st) {
        axis = st->cont.joyx;
    }

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, c)
        if(c && abs(c->joyx) > abs(axis)) {
            axis = c->joyx;
        }
    MAPLE_FOREACH_END()

    return axis;
}

static void map_from_analog(int axis) {
    int side = map_axis_side(axis);
    uint64_t now;

    if(side == 0 || side == self.map_axis_dir) {
        self.map_axis_arm = 0;
        self.map_axis_ms = 0;
        if(side == 0) {
            self.map_axis_dir = 0;
        }
        return;
    }
    if(side != self.map_axis_arm) {
        self.map_axis_arm = side;
        self.map_axis_ms = timer_ms_gettime64();
        return;
    }

    now = timer_ms_gettime64();
    if(now - self.map_axis_ms < MAP_AXIS_HOLD_MS) {
        return;
    }

    self.map_axis_dir = side;
    self.map_axis_arm = 0;
    self.map_axis_ms = 0;
    ds_sfx_play(DS_SFX_CLICK);
    map_cycle(side);
}

static mie_state_t *mie_dev_state(void) {
    mie_state_t *st = NULL;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MIE, mie_state_t, ms)
        if(ms) {
            st = ms;
        }
    MAPLE_FOREACH_END()

    return st;
}

static uint8_t outputs_driver_count(void) {
    uint8_t count;

    count = mie_jvs_driver_outputs();
    if(count == 0 || count > MIE_JVS_OUTPUT_COUNT) {
        return MIE_JVS_OUTPUT_COUNT;
    }
    return count;
}

static uint32_t outputs_avail_mask(void) {
    return MIE_JVS_OUTPUT_MASK_COUNT(outputs_driver_count());
}

static int output_available(int index) {
    if(mie_port0_mode() != MIE_PORT0_JVS) {
        return 0;
    }

    return index >= 0 && index < (int)outputs_driver_count();
}

static int output_next_available(int index, int dir) {
    int i;
    int count = MIE_JVS_OUTPUT_COUNT;
    int next = index;

    for(i = 0; i < count; i++) {
        next += dir;
        if(next < 0) {
            next = count - 1;
        }
        if(next >= count) {
            next = 0;
        }
        if(output_available(next)) {
            return next;
        }
    }

    return index;
}

static void outputs_apply(uint32_t value) {
    if(mie_port0_mode() != MIE_PORT0_JVS) {
        return;
    }

    value &= outputs_avail_mask();
    mie_jvs_set_outputs(value, false);
    self.output_manual = value;
}

static void outputs_clear(void) {
    outputs_apply(0);
}

static void outputs_blink_step(void) {
    uint64_t now;

    if(!self.output_blink || mie_port0_mode() != MIE_PORT0_JVS) {
        return;
    }

    now = timer_ms_gettime64();
    if(now - self.output_blink_last_ms < OUTPUT_BLINK_MS) {
        return;
    }

    self.output_blink_last_ms = now;
    outputs_apply(OUTPUT_BIT(self.output_blink_step));
    self.output_blink_step = (uint8_t)output_next_available(self.output_blink_step, 1);
}

static uint32_t outputs_display_value(void) {
    if(self.view == MIE_VIEW_OUTPUTS) {
        return self.output_manual;
    }

    return mie_jvs_get_outputs();
}

static void outputs_sync_manual(void) {
    int i;

    self.output_manual = mie_jvs_get_outputs() & outputs_avail_mask();
    if(output_available(self.output_sel)) {
        return;
    }

    for(i = 0; i < MIE_JVS_OUTPUT_COUNT; i++) {
        if(output_available(i)) {
            self.output_sel = i;
            return;
        }
    }

    self.output_sel = 0;
}

static void outputs_toggle_selected(void) {
    if(!output_available(self.output_sel)) {
        return;
    }

    self.output_manual ^= OUTPUT_BIT(self.output_sel);
    outputs_apply(self.output_manual);
}

static void outputs_cycle_toggle(void) {
    if(self.output_blink) {
        self.output_blink = 0;
        outputs_clear();
        return;
    }

    self.output_blink = 1;
    self.output_blink_step = (uint8_t)output_next_available(-1, 1);
    self.output_blink_last_ms = 0;
}

static void outputs_update_mode(void) {
    if(self.output_blink) {
        outputs_blink_step();
        return;
    }

    if(self.view == MIE_VIEW_OUTPUTS) {
        outputs_apply(self.output_manual);
    }
}

static void buttons_text(char *linebuf, size_t len, uint32_t btns) {
    size_t n = 0;

    linebuf[0] = '\0';

    if(btns & CONT_START)
        n += snprintf(linebuf + n, len - n, "START ");
    if(btns & CONT_A)
        n += snprintf(linebuf + n, len - n, "A ");
    if(btns & CONT_B)
        n += snprintf(linebuf + n, len - n, "B ");
    if(btns & CONT_X)
        n += snprintf(linebuf + n, len - n, "X ");
    if(btns & CONT_Y)
        n += snprintf(linebuf + n, len - n, "Y ");
    if(btns & CONT_Z)
        n += snprintf(linebuf + n, len - n, "Z ");
    if(btns & CONT_C)
        n += snprintf(linebuf + n, len - n, "C ");
    if(btns & CONT_D)
        n += snprintf(linebuf + n, len - n, "D ");
    if(btns & CONT_DPAD_UP)
        n += snprintf(linebuf + n, len - n, "UP ");
    if(btns & CONT_DPAD_DOWN)
        n += snprintf(linebuf + n, len - n, "DOWN ");
    if(btns & CONT_DPAD_LEFT)
        n += snprintf(linebuf + n, len - n, "LEFT ");
    if(btns & CONT_DPAD_RIGHT)
        n += snprintf(linebuf + n, len - n, "RIGHT ");

    if(n == 0) {
        snprintf(linebuf, len, "(none)");
    }
}

static float wheel_bar_pos(uint16_t raw) {
    float pos;

    if(mie_analog_calib_valid() && !mie_analog_calib_active()) {
        pos = (float)mie_analog_norm_wheel(raw);
        pos = (pos + 128.0f) / 255.0f;
    }
    else {
        pos = (float)raw / 65535.0f;
    }

    if(pos < 0.0f) {
        pos = 0.0f;
    }
    if(pos > 1.0f) {
        pos = 1.0f;
    }

    return pos * WHEEL_BAR_W;
}

static float pedal_bar_height(uint16_t raw, int (*norm_fn)(uint16_t)) {
    float pos;

    if(mie_analog_calib_valid() && !mie_analog_calib_active() && norm_fn) {
        pos = (float)norm_fn(raw) / 255.0f;
    }
    else {
        pos = (float)raw / 65535.0f;
    }

    if(pos < 0.0f) {
        pos = 0.0f;
    }
    if(pos > 1.0f) {
        pos = 1.0f;
    }

    return pos * PEDAL_BAR_H;
}

static void update_wheel_bar_ui(MieAnalogUi_t *ui, uint16_t raw,
                                uint16_t range_min, uint16_t range_max, int use_range) {
    float pos;
    float ix;
    Vector vpos;
    float range;

    if(!ui || !ui->wheel_bar_fill || !ui->wheel_bar_bg) {
        return;
    }

    if(use_range && range_max > range_min) {
        range = (float)(range_max - range_min);
        pos = ((float)((raw & MIE_RAW_MASK) - range_min) / range) * WHEEL_BAR_W;
    }
    else {
        pos = wheel_bar_pos(raw);
    }

    if(pos < 0.0f) {
        pos = 0.0f;
    }
    if(pos > WHEEL_BAR_W) {
        pos = WHEEL_BAR_W;
    }

    ix = pos;
    if(ix < 4.0f) {
        ix = 4.0f;
    }
    if(ix > WHEEL_BAR_W - 4.0f) {
        ix = WHEEL_BAR_W - 4.0f;
    }

    TSU_RectangleSetSize(ui->wheel_bar_fill, 8.0f, WHEEL_BAR_H);
    vpos = (Vector){CALIB_WHEEL_BAR_X + ix - 4.0f, CALIB_WHEEL_BAR_Y, 71.0f, 1.0f};
    TSU_DrawableSetTranslate((Drawable *)ui->wheel_bar_fill, &vpos);

    if(ui->wheel_bar_center) {
        float cx = WHEEL_BAR_W * 0.5f;
        mie_analog_calib_step_t step = MIE_ANALOG_CALIB_IDLE;

        if(mie_analog_calib_active()) {
            step = mie_analog_calib_current();
        }

        if(step == MIE_ANALOG_CALIB_WHEEL &&
                use_range && range_max > range_min &&
                self.calib_wheel_center >= range_min &&
                self.calib_wheel_center <= range_max) {
            range = (float)(range_max - range_min);
            cx = ((float)(self.calib_wheel_center - range_min) / range) * WHEEL_BAR_W;
        }

        if(cx < 0.0f) {
            cx = 0.0f;
        }
        if(cx > WHEEL_BAR_W) {
            cx = WHEEL_BAR_W;
        }

        TSU_DrawableSetAlpha((Drawable *)ui->wheel_bar_center, 1.0f);
        vpos = (Vector){CALIB_WHEEL_BAR_X + cx - 1.0f,
                        CALIB_WHEEL_BAR_Y - WHEEL_BAR_CENTER_EXT,
                        WHEEL_BAR_CENTER_Z, 1.0f};
        TSU_DrawableSetTranslate((Drawable *)ui->wheel_bar_center, &vpos);
    }
}

static void update_pedal_bar_ui(Rectangle *fill, Rectangle *bg,
                                uint16_t raw, int (*norm_fn)(uint16_t), float x) {
    float h;
    Vector pos;

    if(!fill || !bg) {
        return;
    }

    h = pedal_bar_height(raw, norm_fn);
    if(h < 1.0f) {
        h = 1.0f;
    }
    if(h > PEDAL_BAR_H) {
        h = PEDAL_BAR_H;
    }

    TSU_RectangleSetSize(fill, PEDAL_BAR_W, h);
    pos = (Vector){x, CALIB_PEDAL_BAR_BOTTOM, 71.0f, 1.0f};
    TSU_DrawableSetTranslate((Drawable *)fill, &pos);
}

static void player_detail_text(char *linebuf, size_t len, const mie_jvs_player_sw_t *sw) {
    size_t n = 0;

    linebuf[0] = '\0';

    if(sw->up) {
        n += snprintf(linebuf + n, len - n, "UP ");
    }
    if(sw->down) {
        n += snprintf(linebuf + n, len - n, "DOWN ");
    }
    if(sw->left) {
        n += snprintf(linebuf + n, len - n, "LEFT ");
    }
    if(sw->right) {
        n += snprintf(linebuf + n, len - n, "RIGHT ");
    }
    if(sw->start) {
        n += snprintf(linebuf + n, len - n, "START ");
    }
    if(sw->sw2) {
        n += snprintf(linebuf + n, len - n, "BTN1 ");
    }
    if(sw->sw1) {
        n += snprintf(linebuf + n, len - n, "BTN2 ");
    }
    if(sw->sw9) {
        n += snprintf(linebuf + n, len - n, "BTN3 ");
    }
    if(sw->sw10) {
        n += snprintf(linebuf + n, len - n, "BTN4 ");
    }
    if(sw->sw11) {
        n += snprintf(linebuf + n, len - n, "BTN5 ");
    }
    if(sw->sw12) {
        n += snprintf(linebuf + n, len - n, "BTN6 ");
    }
    if(sw->sw13) {
        n += snprintf(linebuf + n, len - n, "BTN7 ");
    }
    if(sw->sw14) {
        n += snprintf(linebuf + n, len - n, "BTN8 ");
    }
    if(sw->sw15) {
        n += snprintf(linebuf + n, len - n, "BTN9 ");
    }
    if(sw->sw16) {
        n += snprintf(linebuf + n, len - n, "BTN10 ");
    }

    if(n == 0) {
        snprintf(linebuf, len, "(none)");
    }
}

static void update_monitor_calib_text(void) {
    char line[MIE_LINE_MAX];
    const mie_analog_calib_t *c;

    if(mie_analog_calib_active()) {
        if(self.monitor_calib) {
            TSU_LabelSetText(self.monitor_calib,
                             calib_step_name(mie_analog_calib_current()));
        }
        if(self.monitor_calib2) {
            TSU_LabelSetText(self.monitor_calib2, "active on calib page");
        }
        return;
    }

    if(mie_analog_calib_valid()) {
        c = mie_analog_calib_get();
        if(self.monitor_calib) {
            snprintf(line, sizeof(line),
                     "wheel %04x/%04x/%04x",
                     c->wheel.min, c->wheel.center, c->wheel.max);
            TSU_LabelSetText(self.monitor_calib, line);
        }
        if(self.monitor_calib2) {
            snprintf(line, sizeof(line),
                     "accel %04x/%04x  brake %04x/%04x",
                     c->accel.min, c->accel.max,
                     c->brake.min, c->brake.max);
            TSU_LabelSetText(self.monitor_calib2, line);
        }
    }
    else {
        if(self.monitor_calib) {
            TSU_LabelSetText(self.monitor_calib, "not calibrated");
        }
        if(self.monitor_calib2) {
            TSU_LabelSetText(self.monitor_calib2, "");
        }
    }
}

static void update_monitor_panel(const mie_state_t *st, int norm_w, int norm_a, int norm_b) {
    char line[MIE_LINE_MAX];
    char btns[64];
    char detail[64];

    if(!st) {
        return;
    }

    if(self.analog_wheel) {
        snprintf(line, sizeof(line),
                 "wheel raw=0x%04x norm=%d",
                 st->jvs.wheel & MIE_RAW_MASK, norm_w);
        TSU_LabelSetText(self.analog_wheel, line);
    }
    if(self.analog_accel) {
        snprintf(line, sizeof(line),
                 "accel raw=0x%04x norm=%d",
                 st->jvs.accel & MIE_RAW_MASK, norm_a);
        TSU_LabelSetText(self.analog_accel, line);
    }
    if(self.analog_brake) {
        snprintf(line, sizeof(line),
                 "brake raw=0x%04x norm=%d",
                 st->jvs.brake & MIE_RAW_MASK, norm_b);
        TSU_LabelSetText(self.analog_brake, line);
    }

    update_monitor_calib_text();

    buttons_text(btns, sizeof(btns), st->cont.buttons);
    if(self.cont_label) {
        TSU_LabelSetText(self.cont_label, btns);
    }
    if(self.joy_label) {
        snprintf(line, sizeof(line), "joyx=%d  joyy=%d",
                 st->cont.joyx, st->cont.joyy);
        TSU_LabelSetText(self.joy_label, line);
    }
    if(self.joy2_label) {
        snprintf(line, sizeof(line), "lt=%d  rt=%d  joy2x=%d  joy2y=%d",
                 st->cont.ltrig, st->cont.rtrig,
                 st->cont.joy2x, st->cont.joy2y);
        TSU_LabelSetText(self.joy2_label, line);
    }

    if(self.p1_label) {
        snprintf(line, sizeof(line), "buttons 0x%04x", st->jvs.p1.raw);
        TSU_LabelSetText(self.p1_label, line);
    }
    if(self.p2_label) {
        snprintf(line, sizeof(line), "buttons 0x%04x", st->jvs.p2.raw);
        TSU_LabelSetText(self.p2_label, line);
    }
    player_detail_text(detail, sizeof(detail), &st->jvs.p1);
    if(self.p1_detail) {
        TSU_LabelSetText(self.p1_detail, detail);
    }
    player_detail_text(detail, sizeof(detail), &st->jvs.p2);
    if(self.p2_detail) {
        TSU_LabelSetText(self.p2_detail, detail);
    }
    if(self.p1_svc) {
        snprintf(line, sizeof(line), "service=%d  start=%d",
                 st->jvs.p1.service, st->jvs.p1.start);
        TSU_LabelSetText(self.p1_svc, line);
    }
    if(self.p2_svc) {
        snprintf(line, sizeof(line), "service=%d  start=%d",
                 st->jvs.p2.service, st->jvs.p2.start);
        TSU_LabelSetText(self.p2_svc, line);
    }

    if(self.sys_label) {
        snprintf(line, sizeof(line), "sys=0x%02x%s%s%s%s",
                 st->jvs.system.raw,
                 st->jvs.system.test ? " TEST" : "",
                 st->jvs.system.tilt1 ? " T1" : "",
                 st->jvs.system.tilt2 ? " T2" : "",
                 st->jvs.system.tilt3 ? " T3" : "");
        TSU_LabelSetText(self.sys_label, line);
    }
    if(self.panel_label) {
        snprintf(line, sizeof(line),
                 "dip=0x%x psw=0x%02x test=%d service=%d",
                 st->jvs.panel.dip.raw,
                 st->jvs.panel.psw.raw,
                 st->jvs.panel.psw.test,
                 st->jvs.panel.psw.service);
        TSU_LabelSetText(self.panel_label, line);
    }
    if(self.coin_label) {
        snprintf(line, sizeof(line),
                 "DS credits=%lu  m0=%u  m1=%u  pulse=0x%02x fault=0x%02x",
                 (unsigned long)naomi_coins_count(),
                 mie_get_coin_meter(0), mie_get_coin_meter(1),
                 st->jvs.coin_pulse, st->jvs.coin_fault);
        TSU_LabelSetText(self.coin_label, line);
    }
    if(self.out_label) {
        snprintf(line, sizeof(line), "out=0x%08lx%s",
                 (unsigned long)outputs_display_value(),
                 self.output_blink ? " blink" : "");
        TSU_LabelSetText(self.out_label, line);
    }
}

static void calib_wheel_val_text(char *line, size_t len, const mie_state_t *st,
                                 mie_analog_calib_step_t step) {
    if(step == MIE_ANALOG_CALIB_WHEEL_CENTER) {
        snprintf(line, len, "ctr 0x%04x", st->jvs.wheel & MIE_RAW_MASK);
    }
    else if(step == MIE_ANALOG_CALIB_WHEEL) {
        snprintf(line, len,
                 "0x%04x  %04x-%04x",
                 st->jvs.wheel & MIE_RAW_MASK,
                 self.calib_wheel_min, self.calib_wheel_max);
    }
    else {
        snprintf(line, len,
                 "0x%04x  n=%d",
                 st->jvs.wheel & MIE_RAW_MASK,
                 mie_analog_norm_wheel(st->jvs.wheel));
    }
}

static void calib_pedal_val_text(char *line, size_t len, uint16_t raw,
                                 uint16_t min_val, uint16_t max_val,
                                 int (*norm_fn)(uint16_t), int tracking) {
    if(tracking) {
        snprintf(line, len,
                 "0x%04x  %04x-%04x",
                 raw & MIE_RAW_MASK, min_val, max_val);
    }
    else {
        snprintf(line, len, "0x%04x  n=%d",
                 raw & MIE_RAW_MASK, norm_fn(raw));
    }
}

static void reset_calib_ui_tracking(void) {
    self.calib_wheel_min = 0xFFFF;
    self.calib_wheel_max = 0;
    self.calib_wheel_center = 0;
    self.calib_accel_min = 0xFFFF;
    self.calib_accel_max = 0;
    self.calib_brake_min = 0xFFFF;
    self.calib_brake_max = 0;
}

static void track_calib_ui(const mie_state_t *st) {
    uint16_t wh;
    uint16_t ac;
    uint16_t br;

    if(!st || !mie_analog_calib_active()) {
        return;
    }

    wh = st->jvs.wheel & MIE_RAW_MASK;
    ac = st->jvs.accel & MIE_RAW_MASK;
    br = st->jvs.brake & MIE_RAW_MASK;

    switch(mie_analog_calib_current()) {
    case MIE_ANALOG_CALIB_WHEEL:
        if(wh < self.calib_wheel_min) {
            self.calib_wheel_min = wh;
        }
        if(wh > self.calib_wheel_max) {
            self.calib_wheel_max = wh;
        }
        break;
    case MIE_ANALOG_CALIB_ACCEL:
        if(ac < self.calib_accel_min) {
            self.calib_accel_min = ac;
        }
        if(ac > self.calib_accel_max) {
            self.calib_accel_max = ac;
        }
        break;
    case MIE_ANALOG_CALIB_BRAKE:
        if(br < self.calib_brake_min) {
            self.calib_brake_min = br;
        }
        if(br > self.calib_brake_max) {
            self.calib_brake_max = br;
        }
        break;
    default:
        break;
    }
}

static void update_calib_analog_ui(const mie_state_t *st) {
    mie_analog_calib_step_t step;
    char line[MIE_LINE_MAX];
    int wheel_range;
    int wheel_tracking;
    int accel_tracking;
    int brake_tracking;

    if(!st) {
        return;
    }

    step = mie_analog_calib_current();
    wheel_tracking = mie_analog_calib_active() &&
        (step == MIE_ANALOG_CALIB_WHEEL || step == MIE_ANALOG_CALIB_WHEEL_CENTER);
    accel_tracking = mie_analog_calib_active() && step == MIE_ANALOG_CALIB_ACCEL;
    brake_tracking = mie_analog_calib_active() && step == MIE_ANALOG_CALIB_BRAKE;
    wheel_range = wheel_tracking && self.calib_wheel_max > self.calib_wheel_min;

    if(self.calib_ui.wheel_val) {
        calib_wheel_val_text(line, sizeof(line), st, step);
        TSU_LabelSetText(self.calib_ui.wheel_val, line);
    }
    if(self.calib_ui.accel_val) {
        calib_pedal_val_text(line, sizeof(line), st->jvs.accel,
                             self.calib_accel_min, self.calib_accel_max,
                             mie_analog_norm_accel, accel_tracking);
        TSU_LabelSetText(self.calib_ui.accel_val, line);
    }
    if(self.calib_ui.brake_val) {
        calib_pedal_val_text(line, sizeof(line), st->jvs.brake,
                             self.calib_brake_min, self.calib_brake_max,
                             mie_analog_norm_brake, brake_tracking);
        TSU_LabelSetText(self.calib_ui.brake_val, line);
    }

    update_wheel_bar_ui(&self.calib_ui, st->jvs.wheel,
                          self.calib_wheel_min, self.calib_wheel_max, wheel_range);
    update_pedal_bar_ui(self.calib_ui.accel_bar_fill,
                        self.calib_ui.accel_bar_bg, st->jvs.accel,
                        accel_tracking ? NULL : mie_analog_norm_accel,
                        CALIB_PEDAL_X_ACCEL);
    update_pedal_bar_ui(self.calib_ui.brake_bar_fill,
                        self.calib_ui.brake_bar_bg, st->jvs.brake,
                        brake_tracking ? NULL : mie_analog_norm_brake,
                        CALIB_PEDAL_X_BRAKE);
}

static void update_output_leds_for(Rectangle **leds, Label **nums, uint32_t outputs) {
    int i;
    Color on;
    Color off;
    Color blink;
    Color sel_on;
    Color sel_off;
    Color disabled;
    Color num_on;
    Color num_disabled;
    Color c;

    on.r = 0.831f;
    on.g = 0.945f;
    on.b = 0.161f;
    on.a = 1.0f;

    off.r = 0.2f;
    off.g = 0.2f;
    off.b = 0.2f;
    off.a = 1.0f;

    blink.r = 1.0f;
    blink.g = 0.667f;
    blink.b = 0.0f;
    blink.a = 1.0f;

    sel_on.r = 1.0f;
    sel_on.g = 1.0f;
    sel_on.b = 1.0f;
    sel_on.a = 1.0f;

    sel_off.r = 0.45f;
    sel_off.g = 0.45f;
    sel_off.b = 0.55f;
    sel_off.a = 1.0f;

    disabled.r = 0.12f;
    disabled.g = 0.12f;
    disabled.b = 0.14f;
    disabled.a = 0.45f;

    num_on.r = 0.667f;
    num_on.g = 0.667f;
    num_on.b = 0.667f;
    num_on.a = 1.0f;

    num_disabled.r = 0.35f;
    num_disabled.g = 0.35f;
    num_disabled.b = 0.38f;
    num_disabled.a = 0.55f;

    for(i = 0; i < MIE_JVS_OUTPUT_COUNT; i++) {
        if(!leds[i]) {
            continue;
        }

        if(!output_available(i)) {
            TSU_DrawableSetTint((Drawable *)leds[i], &disabled);
            if(nums[i]) {
                TSU_LabelSetTint(nums[i], &num_disabled);
            }
            continue;
        }

        if(nums[i]) {
            TSU_LabelSetTint(nums[i], &num_on);
        }

        if(outputs & OUTPUT_BIT(i)) {
            if(self.view == MIE_VIEW_OUTPUTS && i == self.output_sel) {
                c = sel_on;
            }
            else {
                c = self.output_blink ? blink : on;
            }
        }
        else if(self.view == MIE_VIEW_OUTPUTS && i == self.output_sel) {
            c = sel_off;
        }
        else {
            c = off;
        }

        TSU_DrawableSetTint((Drawable *)leds[i], &c);
    }
}

static void update_output_leds(uint32_t outputs) {
    update_output_leds_for(self.outputs_led, self.outputs_num, outputs);
}

static void update_calib_panel(const mie_state_t *st) {
    char line[MIE_LINE_MAX];
    const mie_analog_calib_t *c;
    mie_analog_calib_step_t step;

    if(!self.calib_step) {
        return;
    }

    if(mie_analog_calib_active()) {
        step = mie_analog_calib_current();
        TSU_LabelSetText(self.calib_step, calib_step_name(step));
        if(st) {
            snprintf(line, sizeof(line),
                     "raw wh=0x%04x ac=0x%04x br=0x%04x",
                     st->jvs.wheel & MIE_RAW_MASK,
                     st->jvs.accel & MIE_RAW_MASK,
                     st->jvs.brake & MIE_RAW_MASK);
            TSU_LabelSetText(self.calib_raw, line);
        }

        switch(step) {
        case MIE_ANALOG_CALIB_WHEEL_CENTER:
            if(st) {
                snprintf(line, sizeof(line), "center raw=%04x",
                         st->jvs.wheel & MIE_RAW_MASK);
            }
            else
            {
                snprintf(line, sizeof(line), "center raw=----");
            }
            break;
        case MIE_ANALOG_CALIB_WHEEL:
            snprintf(line, sizeof(line),
                     "center=%04x  min=%04x max=%04x",
                     self.calib_wheel_center,
                     self.calib_wheel_min, self.calib_wheel_max);
            break;
        case MIE_ANALOG_CALIB_ACCEL:
            snprintf(line, sizeof(line),
                     "wheel %04x/%04x/%04x  accel tracking...",
                     self.calib_wheel_min, self.calib_wheel_center,
                     self.calib_wheel_max);
            break;
        case MIE_ANALOG_CALIB_BRAKE:
            snprintf(line, sizeof(line),
                     "wheel %04x/%04x/%04x  accel %04x/%04x",
                     self.calib_wheel_min, self.calib_wheel_center,
                     self.calib_wheel_max,
                     self.calib_accel_min, self.calib_accel_max);
            break;
        default:
            line[0] = '\0';
            break;
        }
        TSU_LabelSetText(self.calib_data, line);
    }
    else if(mie_analog_calib_valid()) {
        TSU_LabelSetText(self.calib_step, "Calibrated");
        c = mie_analog_calib_get();
        snprintf(line, sizeof(line),
                 "wheel %04x/%04x/%04x",
                 c->wheel.min, c->wheel.center, c->wheel.max);
        TSU_LabelSetText(self.calib_raw, line);
        snprintf(line, sizeof(line),
                 "accel %04x/%04x  brake %04x/%04x",
                 c->accel.min, c->accel.max,
                 c->brake.min, c->brake.max);
        TSU_LabelSetText(self.calib_data, line);
    }
    else {
        TSU_LabelSetText(self.calib_step, "Not calibrated");
        TSU_LabelSetText(self.calib_raw, "");
        TSU_LabelSetText(self.calib_data, "A or START+UP to start");
    }
}

static void sync_view_cardstack(void) {
    mie_view_t view = self.view;

    if(mie_analog_calib_active()) {
        view = MIE_VIEW_CALIB;
    }

    if(self.pages) {
        TSU_CardStackShowIndex(self.pages, (int)view);
    }
}

static void reset_calib_settings(void) {
    mie_analog_calib_reset();
    mark_calib_dirty();
}

static void prompt_reset_calib(void) {
    if(self.map_prompt) {
        return;
    }

    self.save_kind = SAVE_KIND_RESET;
    if(!self.save_dialog) {
        reset_calib_settings();
        return;
    }

    self.map_prompt = 1;
    TSU_DialogShow(self.save_dialog, "Reset calibration?");
}

static void update_hint(void) {
    char line[128];
    const char *back;

    if(mie_analog_calib_active()) {
        snprintf(line, sizeof(line), "A/START=capture  B=cancel");
        back = "Back: Start+Down or Test+Service";
    }
    else if(self.view == MIE_VIEW_MENU) {
        snprintf(line, sizeof(line), "Up/Down=select  A/START=open");
        back = "Exit: Start+Down or Test+Service";
    }
    else if(self.view == MIE_VIEW_CALIB) {
        snprintf(line, sizeof(line), "A/START+UP=start  A+START/TEST+START=reset");
        back = "Back: Start+Down or Test+Service";
    }
    else if(self.view == MIE_VIEW_OUTPUTS) {
        snprintf(line, sizeof(line), "D-pad=select  A/START=toggle  START+UP=cycle");
        back = "Back: Start+Down or Test+Service";
    }
    else if(self.view == MIE_VIEW_MAP) {
        snprintf(line, sizeof(line),
                 "Up/Down=select  Analog=pad  A=default  B=clear");
        back = "Back: Start+Down or Test+Service";
    }
    else {
        snprintf(line, sizeof(line), "START+UP=blink outputs");
        back = "Back: Start+Down or Test+Service";
    }

    if(self.hint_label) {
        TSU_LabelSetText(self.hint_label, line);
    }
    if(self.exit_label) {
        float w;
        float h;

        TSU_LabelSetText(self.exit_label, back);
        TSU_LabelGetSize(self.exit_label, &w, &h);
        place_label(self.exit_label, (float)SCREEN_WIDTH - EXIT_LABEL_MARGIN - w,
                    EXIT_LABEL_Y);
    }
}

static void update_menu_ui(void) {
    int i;
    Color col;
    float y;

    for(i = 0; i < MIE_MENU_COUNT; i++) {
        if(!self.menu_item[i]) {
            continue;
        }
        place_label(self.menu_item[i], MENU_ITEM_X,
                    MENU_ITEM_Y0 + (float)i * MENU_ITEM_STEP);
        if(i == self.menu_sel) {
            col = COL_SEL;
        }
        else if(i == MIE_MENU_EXIT) {
            col = COL_EXIT;
        }
        else {
            col = COL_TEXT;
        }
        TSU_LabelSetTint(self.menu_item[i], &col);
    }

    y = MENU_ITEM_Y0 + (float)self.menu_sel * MENU_ITEM_STEP;
    place_bar(self.menu_cursor, MENU_CURSOR_X, y + MENU_CURSOR_BELOW,
              MENU_CURSOR_W, MENU_CURSOR_H);
}

static void update_map_ui(const mie_state_t *st) {
    int row;
    int idx;
    char name[32];
    char pos[16];
    Color col;
    float y;

    if(self.map_sel < self.map_scroll) {
        self.map_scroll = self.map_sel;
    }
    if(self.map_sel >= self.map_scroll + MAP_VISIBLE) {
        self.map_scroll = self.map_sel - MAP_VISIBLE + 1;
    }

    for(row = 0; row < MAP_VISIBLE; row++) {
        idx = self.map_scroll + row;
        col = COL_TEXT;
        if(idx >= DS_JVS_MAP_SRC) {
            if(self.map_src[row]) {
                TSU_LabelSetText(self.map_src[row], "");
            }
            if(self.map_dst[row]) {
                TSU_LabelSetText(self.map_dst[row], "");
            }
            continue;
        }

        if(row == self.map_sel - self.map_scroll) {
            col = COL_SEL;
        }
        else if(st && jvs_src_down(&st->jvs, idx)) {
            col = COL_ACTIVE;
        }

        jvs_btn_label(name, sizeof(name), idx);
        y = MAP_ROW_Y0 + (float)row * MAP_ROW_STEP;
        if(self.map_src[row]) {
            place_label(self.map_src[row], MAP_SRC_X, y);
            TSU_LabelSetText(self.map_src[row], name);
            TSU_LabelSetTint(self.map_src[row], &col);
        }
        if(self.map_dst[row]) {
            place_label(self.map_dst[row], MAP_DST_X, y);
            TSU_LabelSetText(self.map_dst[row], map_slot_name(idx, self.map[idx]));
            TSU_LabelSetTint(self.map_dst[row], &col);
        }
    }

    place_label(self.map_hdr_src, MAP_SRC_X, MAP_HDR_Y);
    place_label(self.map_hdr_dst, MAP_DST_X, MAP_HDR_Y);
    place_label(self.map_pos, MAP_POS_X, MAP_HDR_Y);

    y = MAP_ROW_Y0 + (float)(self.map_sel - self.map_scroll) * MAP_ROW_STEP;
    place_bar(self.map_cursor, MAP_CURSOR_X, y + MAP_CURSOR_BELOW,
              MAP_CURSOR_W, MAP_CURSOR_H);

    if(self.map_pos) {
        snprintf(pos, sizeof(pos), "%d/%d", self.map_sel + 1, DS_JVS_MAP_SRC);
        TSU_LabelSetText(self.map_pos, pos);
    }
}

static void update_ui(const mie_state_t *st) {
    char line[128];
    int norm_w;
    int norm_a;
    int norm_b;
    mie_port0_mode_t mode;
    int calib_view;

    mode = mie_port0_mode();
    calib_view = self.view == MIE_VIEW_CALIB || mie_analog_calib_active();

    if(self.page_label) {
        TSU_LabelSetText(self.page_label,
            view_name(mie_analog_calib_active() ? MIE_VIEW_CALIB : self.view));
    }

    if(!st || mode != MIE_PORT0_JVS) {
        snprintf(line, sizeof(line), "Port A: %s MIE: %s",
                 port_mode_name(mode),
                 st ? "connected" : "waiting...");
        if(self.status_label) {
            TSU_LabelSetText(self.status_label, line);
        }
    }
    else {
        track_calib_ui(st);

        if(self.board_id[0]) {
            snprintf(line, sizeof(line), "Port A: JVS: %s", self.board_id);
        }
        else {
            snprintf(line, sizeof(line), "Port A: JVS: n/a");
        }
        if(self.status_label) {
            TSU_LabelSetText(self.status_label, line);
        }

        norm_w = mie_analog_norm_wheel(st->jvs.wheel);
        norm_a = mie_analog_norm_accel(st->jvs.accel);
        norm_b = mie_analog_norm_brake(st->jvs.brake);

        if(self.view == MIE_VIEW_MONITOR && !mie_analog_calib_active()) {
            update_monitor_panel(st, norm_w, norm_a, norm_b);
        }

        if(self.view == MIE_VIEW_OUTPUTS && !mie_analog_calib_active()) {
            if(self.outputs_label) {
                snprintf(line, sizeof(line), "out=0x%08lx  sel=%d  (%u/%u)",
                         (unsigned long)self.output_manual, self.output_sel,
                         (unsigned)outputs_driver_count(), (unsigned)MIE_JVS_OUTPUT_COUNT);
                TSU_LabelSetText(self.outputs_label, line);
            }

            update_output_leds(outputs_display_value());
        }
    }

    if(self.view == MIE_VIEW_MENU && !mie_analog_calib_active()) {
        update_menu_ui();
    }

    if(self.view == MIE_VIEW_MAP && !mie_analog_calib_active()) {
        update_map_ui(st);
    }

    if(calib_view) {
        update_calib_panel(st);
        update_calib_analog_ui(st);
    }
    else if((!st || mode != MIE_PORT0_JVS) && self.view == MIE_VIEW_MONITOR) {
        update_monitor_calib_text();
    }
    update_hint();
    sync_view_cardstack();
}

static void OnExit(void);

static void open_view(mie_view_t view) {
    mie_view_t prev;

    if(mie_analog_calib_active() && view != MIE_VIEW_CALIB) {
        mie_analog_calib_cancel();
    }

    if(self.view == MIE_VIEW_CALIB && view != MIE_VIEW_CALIB &&
            prompt_calib_save(view, 0)) {
        return;
    }

    if(self.view == MIE_VIEW_MAP && view != MIE_VIEW_MAP &&
            prompt_map_save(view, 0)) {
        return;
    }

    prev = self.view;
    self.view = view;
    if(view == MIE_VIEW_OUTPUTS && prev != MIE_VIEW_OUTPUTS) {
        self.output_blink = 0;
        outputs_sync_manual();
    }
    if(view == MIE_VIEW_MAP && prev != MIE_VIEW_MAP) {
        self.map_axis_dir = map_axis_side(map_pad_axis());
        self.map_axis_arm = 0;
        self.map_axis_ms = 0;
    }
    sync_view_cardstack();
}

static void open_menu_item(void) {
    switch(self.menu_sel) {
    case MIE_MENU_MONITOR:
        open_view(MIE_VIEW_MONITOR);
        break;
    case MIE_MENU_CALIB:
        open_view(MIE_VIEW_CALIB);
        break;
    case MIE_MENU_OUTPUTS:
        open_view(MIE_VIEW_OUTPUTS);
        break;
    case MIE_MENU_MAP:
        open_view(MIE_VIEW_MAP);
        break;
    case MIE_MENU_EXIT:
        OnExit();
        break;
    default:
        break;
    }
}

static void switch_output_sel(int dir) {
    self.output_sel = output_next_available(self.output_sel, dir);
}

static int combo_activated(uint32_t btns, uint32_t old_btns, uint32_t combo) {
    return (btns & combo) == combo && (old_btns & combo) != combo;
}

static int test_service_down(const mie_state_t *st) {
    int service;

    if(!st) {
        return 0;
    }

    if(st->jvs.panel.psw.test && st->jvs.panel.psw.service) {
        return 1;
    }

    service = st->jvs.panel.psw.service || st->jvs.p1.service || st->jvs.p2.service;
    return st->jvs.system.test && service;
}

static void OnExit(void) {

    if(self.exiting) {
        return;
    }

    if(prompt_calib_save(MIE_VIEW_MENU, 1)) {
        return;
    }

    if(prompt_map_save(MIE_VIEW_MENU, 1)) {
        return;
    }

    self.exiting = 1;

    outputs_clear();
    self.output_blink = 0;

    if(self.input_event) {
        RemoveEvent(self.input_event);
        self.input_event = NULL;
    }

    if(self.video_event) {
        RemoveEvent(self.video_event);
        self.video_event = NULL;
    }

    SDL_DC_EmulateMouse(SDL_TRUE);
    SetVirtKeyboardEnabled(1);

    OpenMainApp();
}

static void handle_capture(void) {
    mie_analog_calib_step_t step;
    mie_state_t *st;

    if(!mie_analog_calib_active()) {
        return;
    }

    step = mie_analog_calib_current();
    st = mie_dev_state();

    if(step == MIE_ANALOG_CALIB_WHEEL_CENTER && st) {
        self.calib_wheel_center = st->jvs.wheel & MIE_RAW_MASK;
    }

    if(mie_analog_calib_capture()) {
        mark_calib_dirty();
    }
}

static void finish_map_prompt(int save) {
    mie_view_t view;
    int do_exit;
    int kind;

    if(self.save_dialog) {
        TSU_DialogHide(self.save_dialog);
    }
    kind = self.save_kind;
    if(kind == SAVE_KIND_RESET) {
        self.map_prompt = 0;
        if(save) {
            reset_calib_settings();
        }
        return;
    }
    if(kind == SAVE_KIND_CALIB) {
        if(save) {
            save_calib();
        }
        else {
            revert_calib();
        }
    }
    else {
        if(save) {
            save_btn_map();
        }
        else {
            revert_map();
        }
    }

    view = self.map_after;
    do_exit = self.map_after_exit;
    self.map_prompt = 0;
    self.map_after_exit = 0;
    if(do_exit) {
        OnExit();
        return;
    }
    open_view(view);
}

void MieJvsApp_SaveConfirm(Drawable *drawable) {
    (void)drawable;
    finish_map_prompt(1);
}

void MieJvsApp_SaveCancel(Drawable *drawable) {
    (void)drawable;
    finish_map_prompt(0);
}

static int button_edge(uint32_t btns, uint32_t mask) {
    return (btns & mask) && !(self.old_btns & mask);
}

static int test_button_down(const mie_state_t *st) {
    if(!st) {
        return 0;
    }
    if(st->jvs.panel.psw.service || st->jvs.p1.service || st->jvs.p2.service) {
        return 0;
    }
    return st->jvs.panel.psw.test || st->jvs.system.test;
}

static int test_start_edge(uint32_t btns) {
    static int test_old;
    int test_now = test_button_down(mie_dev_state());
    int edge;

    edge = test_now && (btns & CONT_START) &&
            !(test_old && (self.old_btns & CONT_START));
    test_old = test_now;
    return edge;
}

static int select_edge(uint32_t btns) {
    if(btns & CONT_B) {
        return 0;
    }
    if(button_edge(btns, CONT_A)) {
        return 1;
    }
    if(button_edge(btns, CONT_START) &&
            !(btns & (CONT_DPAD_UP | CONT_DPAD_DOWN))) {
        return 1;
    }
    return 0;
}

static void handle_input_buttons(uint32_t nav_btns, uint32_t jvs_btns) {
    uint32_t btns = nav_btns | jvs_btns;
    int test_start = test_start_edge(btns);
    int calib_reset;

    if(self.save_dialog && TSU_DialogIsVisible(self.save_dialog)) {
        if(((btns & CONT_DPAD_LEFT) && !(self.old_btns & CONT_DPAD_LEFT)) ||
                ((btns & CONT_DPAD_UP) && !(self.old_btns & CONT_DPAD_UP))) {
            ds_sfx_play(DS_SFX_CLICK2);
            TSU_DialogMoveFocus(self.save_dialog, -1);
        }
        if(((btns & CONT_DPAD_RIGHT) && !(self.old_btns & CONT_DPAD_RIGHT)) ||
                ((btns & CONT_DPAD_DOWN) && !(self.old_btns & CONT_DPAD_DOWN))) {
            ds_sfx_play(DS_SFX_CLICK2);
            TSU_DialogMoveFocus(self.save_dialog, 1);
        }
        if(select_edge(btns)) {
            ds_sfx_play(DS_SFX_CLICK);
            TSU_DialogActivateFocused(self.save_dialog);
        }
        if((btns & CONT_B) && !(self.old_btns & CONT_B) && !(btns & CONT_A)) {
            ds_sfx_play(DS_SFX_CLICK);
            MieJvsApp_SaveCancel(NULL);
        }
        self.old_btns = btns;
        return;
    }

    if(mie_analog_calib_active()) {
        if((btns & CONT_B) && !(self.old_btns & CONT_B) &&
                !(btns & (CONT_START | CONT_A))) {
            mie_analog_calib_cancel();
        }
    }

    if(self.view == MIE_VIEW_MENU) {
        if((btns & CONT_DPAD_UP) && !(self.old_btns & CONT_DPAD_UP) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            self.menu_sel--;
            if(self.menu_sel < 0) {
                self.menu_sel = MIE_MENU_COUNT - 1;
            }
        }

        if((btns & CONT_DPAD_DOWN) && !(self.old_btns & CONT_DPAD_DOWN) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            self.menu_sel++;
            if(self.menu_sel >= MIE_MENU_COUNT) {
                self.menu_sel = 0;
            }
        }

        if(select_edge(btns)) {
            ds_sfx_play(DS_SFX_CLICK);
            open_menu_item();
            if(self.exiting) {
                return;
            }
        }

        self.old_btns = btns;
        return;
    }

    if(self.view == MIE_VIEW_MAP && !mie_analog_calib_active()) {
        if((btns & CONT_DPAD_UP) && !(self.old_btns & CONT_DPAD_UP) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            self.map_sel--;
            if(self.map_sel < 0) {
                self.map_sel = DS_JVS_MAP_SRC - 1;
            }
        }

        if((btns & CONT_DPAD_DOWN) && !(self.old_btns & CONT_DPAD_DOWN) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            self.map_sel++;
            if(self.map_sel >= DS_JVS_MAP_SRC) {
                self.map_sel = 0;
            }
        }

        if((btns & CONT_DPAD_LEFT) && !(self.old_btns & CONT_DPAD_LEFT)) {
            ds_sfx_play(DS_SFX_CLICK);
            map_cycle(-1);
        }

        if((btns & CONT_DPAD_RIGHT) && !(self.old_btns & CONT_DPAD_RIGHT)) {
            ds_sfx_play(DS_SFX_CLICK);
            map_cycle(1);
        }

        map_from_analog(map_pad_axis());

        if((btns & CONT_B) && !(self.old_btns & CONT_B) && !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK);
            self.map[self.map_sel] = 0;
            map_apply();
        }

        if((btns & CONT_A) && !(self.old_btns & CONT_A) && !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK);
            jvs_map_default(self.map);
            map_apply();
        }

        self.old_btns = btns;
        return;
    }

    calib_reset = self.view == MIE_VIEW_CALIB &&
            (combo_activated(btns, self.old_btns, CONT_A | CONT_START) ||
             test_start);
    if(calib_reset) {
        prompt_reset_calib();
    }
    else if(mie_analog_calib_active()) {
        if(select_edge(btns)) {
            handle_capture();
        }
    }
    else if(self.view == MIE_VIEW_CALIB &&
            ((button_edge(btns, CONT_A) && !(btns & (CONT_START | CONT_B))) ||
             combo_activated(btns, self.old_btns, CONT_START | CONT_DPAD_UP))) {
        reset_calib_ui_tracking();
        mie_analog_calib_start();
        self.view = MIE_VIEW_CALIB;
        sync_view_cardstack();
    }

    if(self.view == MIE_VIEW_OUTPUTS && !mie_analog_calib_active() &&
            select_edge(btns)) {
        self.output_blink = 0;
        ds_sfx_play(DS_SFX_CLICK);
        outputs_toggle_selected();
    }

    if(!mie_analog_calib_active() &&
            (self.view == MIE_VIEW_OUTPUTS || self.view == MIE_VIEW_MONITOR) &&
            combo_activated(btns, self.old_btns, CONT_START | CONT_DPAD_UP)) {
        outputs_cycle_toggle();
    }

    if(self.view == MIE_VIEW_OUTPUTS && !mie_analog_calib_active()) {
        if((btns & CONT_DPAD_LEFT) && !(self.old_btns & CONT_DPAD_LEFT)) {
            ds_sfx_play(DS_SFX_CLICK2);
            switch_output_sel(-1);
        }

        if((btns & CONT_DPAD_RIGHT) && !(self.old_btns & CONT_DPAD_RIGHT)) {
            ds_sfx_play(DS_SFX_CLICK2);
            switch_output_sel(1);
        }

        if((btns & CONT_DPAD_UP) && !(self.old_btns & CONT_DPAD_UP) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            switch_output_sel(-1);
        }

        if((btns & CONT_DPAD_DOWN) && !(self.old_btns & CONT_DPAD_DOWN) &&
                !(btns & CONT_START)) {
            ds_sfx_play(DS_SFX_CLICK2);
            switch_output_sel(1);
        }
    }

    self.old_btns = btns;
}

static uint32_t poll_aux_buttons(void) {
    uint32_t btns = 0;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, c)
        if(c) {
            btns |= c->buttons;
        }
    MAPLE_FOREACH_END()

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_KEYBOARD, kbd_state_t, kbd)
        if(kbd) {
            if(kbd->key_states[KBD_KEY_A].is_down) {
                btns |= CONT_A;
            }
            if(kbd->key_states[KBD_KEY_B].is_down) {
                btns |= CONT_B;
            }
            if(kbd->key_states[KBD_KEY_X].is_down) {
                btns |= CONT_X;
            }
            if(kbd->key_states[KBD_KEY_Y].is_down) {
                btns |= CONT_Y;
            }
            if(kbd->key_states[KBD_KEY_ESCAPE].is_down) {
                btns |= CONT_START;
            }
            if(kbd->key_states[KBD_KEY_LEFT].is_down) {
                btns |= CONT_DPAD_LEFT;
            }
            if(kbd->key_states[KBD_KEY_RIGHT].is_down) {
                btns |= CONT_DPAD_RIGHT;
            }
            if(kbd->key_states[KBD_KEY_UP].is_down) {
                btns |= CONT_DPAD_UP;
            }
            if(kbd->key_states[KBD_KEY_DOWN].is_down) {
                btns |= CONT_DPAD_DOWN;
            }
        }
    MAPLE_FOREACH_END()

    return btns;
}

static void poll_input(void) {
    mie_state_t *st;
    uint32_t nav_btns;
    uint32_t jvs_btns = 0;

    nav_btns = poll_aux_buttons();
    st = mie_dev_state();
    if(st) {
        jvs_btns = st->cont.buttons;
    }
    handle_input_buttons(nav_btns, jvs_btns);
}

static uint32_t exit_btns_old;

static void InputHandler(void *ds_event, void *param, int action) {
    uint32_t btns;
    mie_state_t *st;
    int panel_now;

    (void)ds_event;
    (void)param;

    if(action != EVENT_ACTION_UPDATE || self.exiting || !self.app) {
        return;
    }

    if(self.save_dialog && TSU_DialogIsVisible(self.save_dialog)) {
        return;
    }

    btns = poll_aux_buttons();
    st = mie_dev_state();
    if(st) {
        btns |= st->cont.buttons;
    }
    panel_now = test_service_down(st);
    if(combo_activated(btns, exit_btns_old, CONT_START | CONT_DPAD_DOWN) ||
            (panel_now && !self.panel_combo)) {
        self.old_btns = HELD_BUTTONS;
        exit_btns_old = btns;
        self.panel_combo = panel_now;
        if(self.view == MIE_VIEW_MENU) {
            ds_sfx_play(DS_SFX_CH_PAGE);
            OnExit();
        }
        else {
            ds_sfx_play(DS_SFX_CH_PAGE);
            open_view(MIE_VIEW_MENU);
        }
        return;
    }
    exit_btns_old = btns;
    self.panel_combo = panel_now;
}

static void VideoHandler(void *ds_event, void *param, int action) {
    mie_state_t *st;

    (void)ds_event;
    (void)param;

    if(self.exiting || !self.app) {
        return;
    }
    if(action != EVENT_ACTION_RENDER) {
        return;
    }

    outputs_update_mode();
    poll_input();
    if(self.exiting) {
        return;
    }
    st = mie_dev_state();
    update_ui(st);
}

static Label *get_label(const char *name) {
    return (Label *)APP_GET_TSU_DRAWABLE(name);
}

static Rectangle *get_rect(const char *name) {
    return (Rectangle *)APP_GET_TSU_DRAWABLE(name);
}

static void bind_analog_ui(MieAnalogUi_t *ui, const char *prefix) {
    char name[32];

    snprintf(name, sizeof(name), "%swheel_title", prefix);
    ui->wheel_title = get_label(name);
    snprintf(name, sizeof(name), "%swheel_val", prefix);
    ui->wheel_val = get_label(name);
    snprintf(name, sizeof(name), "%saccel_title", prefix);
    ui->accel_title = get_label(name);
    snprintf(name, sizeof(name), "%saccel_val", prefix);
    ui->accel_val = get_label(name);
    snprintf(name, sizeof(name), "%sbrake_title", prefix);
    ui->brake_title = get_label(name);
    snprintf(name, sizeof(name), "%sbrake_val", prefix);
    ui->brake_val = get_label(name);
    snprintf(name, sizeof(name), "%swheel_bar_bg", prefix);
    ui->wheel_bar_bg = get_rect(name);
    snprintf(name, sizeof(name), "%swheel_bar_fill", prefix);
    ui->wheel_bar_fill = get_rect(name);
    snprintf(name, sizeof(name), "%swheel_bar_center", prefix);
    ui->wheel_bar_center = get_rect(name);
    snprintf(name, sizeof(name), "%saccel_bar_bg", prefix);
    ui->accel_bar_bg = get_rect(name);
    snprintf(name, sizeof(name), "%saccel_bar_fill", prefix);
    ui->accel_bar_fill = get_rect(name);
    snprintf(name, sizeof(name), "%sbrake_bar_bg", prefix);
    ui->brake_bar_bg = get_rect(name);
    snprintf(name, sizeof(name), "%sbrake_bar_fill", prefix);
    ui->brake_bar_fill = get_rect(name);
}

static void bind_drawables(void) {
    int i;
    char name[16];

    self.pages = (CardStack *)APP_GET_TSU_DRAWABLE("pages");
    self.title_label = get_label("title_label");
    self.page_label = get_label("page_label");
    self.status_label = get_label("status_label");
    self.hint_label = get_label("hint_label");
    self.exit_label = get_label("exit_label");
    self.p1_title = get_label("p1_title");
    self.p1_label = get_label("p1_label");
    self.p1_svc = get_label("p1_svc");
    self.p1_detail = get_label("p1_detail");
    self.p2_title = get_label("p2_title");
    self.p2_label = get_label("p2_label");
    self.p2_svc = get_label("p2_svc");
    self.p2_detail = get_label("p2_detail");
    self.sys_title = get_label("sys_title");
    self.sys_label = get_label("sys_label");
    self.panel_label = get_label("panel_label");
    self.coin_label = get_label("coin_label");
    self.out_label = get_label("out_label");
    self.cont_title = get_label("cont_title");
    self.cont_label = get_label("cont_label");
    self.joy_label = get_label("joy_label");
    self.joy2_label = get_label("joy2_label");
    self.analog_title = get_label("analog_title");
    self.analog_wheel = get_label("analog_wheel");
    self.analog_accel = get_label("analog_accel");
    self.analog_brake = get_label("analog_brake");
    self.monitor_calib_title = get_label("monitor_calib_title");
    self.monitor_calib = get_label("monitor_calib");
    self.monitor_calib2 = get_label("monitor_calib2");
    self.outputs_label = get_label("outputs_label");
    self.calib_title = get_label("calib_title");
    self.calib_step = get_label("calib_step");
    self.calib_raw = get_label("calib_raw");
    self.calib_data = get_label("calib_data");

    bind_analog_ui(&self.calib_ui, "calib_");

    for(i = 0; i < MIE_JVS_OUTPUT_COUNT; i++) {
        snprintf(name, sizeof(name), "outputs_led_%d", i);
        self.outputs_led[i] = get_rect(name);
        snprintf(name, sizeof(name), "outputs_num_%d", i);
        self.outputs_num[i] = get_label(name);
    }

    for(i = 0; i < MIE_MENU_COUNT; i++) {
        snprintf(name, sizeof(name), "menu_item_%d", i);
        self.menu_item[i] = get_label(name);
    }
    self.menu_cursor = get_rect("menu_cursor");
    self.map_hdr_src = get_label("map_hdr_src");
    self.map_hdr_dst = get_label("map_hdr_dst");
    self.map_pos = get_label("map_pos");
    self.map_cursor = get_rect("map_cursor");
    self.save_dialog = (Dialog *)APP_GET_TSU_DRAWABLE("save_dialog");
    for(i = 0; i < MAP_VISIBLE; i++) {
        snprintf(name, sizeof(name), "map_src_%d", i);
        self.map_src[i] = get_label(name);
        snprintf(name, sizeof(name), "map_dst_%d", i);
        self.map_dst[i] = get_label(name);
    }
}

void MieJvsApp_Init(App_t *app) {
    Settings_t *settings;

    memset(&self, 0, sizeof(self));
    exit_btns_old = 0;
    self.app = app;
    self.view = MIE_VIEW_MENU;

    if(!app || !app->tsunami) {
        return;
    }

    bind_drawables();

    settings = GetSettings();
    mie_analog_calib_set(&settings->analog);
    if(jvs_map_blank(settings->jvs_map)) {
        jvs_map_default(self.map);
    }
    else {
        memcpy(self.map, settings->jvs_map, sizeof(self.map));
    }
    ApplyJvsButtonMap(settings->jvs_map);

    if(mie_port0_mode() == MIE_PORT0_JVS) {
        mie_get_id(self.board_id);
    }

    update_hint();
    sync_view_cardstack();
}

void MieJvsApp_Open(App_t *app) {
    (void)app;

    if(!self.app || !self.app->tsunami) {
        return;
    }

    SDL_DC_EmulateMouse(SDL_FALSE);
    SetVirtKeyboardEnabled(0);

    self.input_event = AddEvent("MieJvsInput", EVENT_TYPE_INPUT,
                                EVENT_PRIO_DEFAULT, InputHandler, NULL);
    self.video_event = AddEvent("MieJvsVideo", EVENT_TYPE_VIDEO,
                                EVENT_PRIO_DEFAULT, VideoHandler, NULL);
}

void MieJvsApp_Shutdown(App_t *app) {
    (void)app;

    self.exiting = 1;
    outputs_clear();
    self.output_blink = 0;

    if(self.input_event) {
        RemoveEvent(self.input_event);
        self.input_event = NULL;
    }

    if(self.video_event) {
        RemoveEvent(self.video_event);
        self.video_event = NULL;
    }

    SDL_DC_EmulateMouse(SDL_TRUE);
    SetVirtKeyboardEnabled(1);
}
