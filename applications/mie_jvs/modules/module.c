/* DreamShell

   module.c - MIE JVS test and calibration app
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <settings.h>
#include <naomi/coins.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mie.h>
#include <tsunami/tsunami.h>

#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_mie_jvs);

#define SCREEN_WIDTH           640
#define SCREEN_HEIGHT          480
#define WHEEL_BAR_W            220.0f
#define WHEEL_BAR_H            14.0f
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
    MIE_VIEW_MONITOR = 0,
    MIE_VIEW_CALIB,
    MIE_VIEW_OUTPUTS,
    MIE_VIEW_COUNT
} mie_view_t;

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
    Event_t *input_event;
    Event_t *video_event;
    int exiting;
    mie_view_t view;
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
    case MIE_ANALOG_CALIB_WHEEL:
        return "Turn wheel fully both ways, press A";
    case MIE_ANALOG_CALIB_WHEEL_CENTER:
        return "Hold wheel centered, press A";
    case MIE_ANALOG_CALIB_ACCEL:
        return "Press accelerator fully, press A";
    case MIE_ANALOG_CALIB_BRAKE:
        return "Press brake fully, press A";
    default:
        return "Idle";
    }
}

static const char *view_name(mie_view_t view) {
    switch(view) {
    case MIE_VIEW_MONITOR:
        return "Monitor";
    case MIE_VIEW_CALIB:
        return "Calibration";
    case MIE_VIEW_OUTPUTS:
        return "Outputs";
    default:
        return "";
    }
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

static void outputs_update_mode(void) {
    if(self.output_blink && self.view != MIE_VIEW_OUTPUTS) {
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
        float show_center = 1.0f;

        if(use_range && range_max > range_min) {
            range = (float)(range_max - range_min);
            if(mie_analog_calib_active()) {
                mie_analog_calib_step_t step = mie_analog_calib_current();

                if(step == MIE_ANALOG_CALIB_WHEEL) {
                    show_center = 0.0f;
                }
                else if(step == MIE_ANALOG_CALIB_WHEEL_CENTER) {
                    cx = ((float)((raw & MIE_RAW_MASK) - range_min) / range) * WHEEL_BAR_W;
                }
            }
            else if(self.calib_wheel_center >= range_min && self.calib_wheel_center <= range_max) {
                cx = ((float)(self.calib_wheel_center - range_min) / range) * WHEEL_BAR_W;
            }
        }
        else if(mie_analog_calib_valid()) {
            const mie_analog_calib_t *c = mie_analog_calib_get();
            float calib_range = (float)(c->wheel.max - c->wheel.min);

            if(calib_range > 0.0f) {
                cx = ((float)(c->wheel.center - c->wheel.min) / calib_range) * WHEEL_BAR_W;
            }
        }

        if(cx < 0.0f) {
            cx = 0.0f;
        }
        if(cx > WHEEL_BAR_W) {
            cx = WHEEL_BAR_W;
        }

        TSU_DrawableSetAlpha((Drawable *)ui->wheel_bar_center, show_center);
        vpos = (Vector){CALIB_WHEEL_BAR_X + cx - 1.0f, CALIB_WHEEL_BAR_Y - 4.0f, 72.0f, 1.0f};
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
            TSU_LabelSetText(self.monitor_calib2, "START+A on calib page");
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
    if(step == MIE_ANALOG_CALIB_WHEEL) {
        snprintf(line, len,
                 "0x%04x  %04x-%04x",
                 st->jvs.wheel & MIE_RAW_MASK,
                 self.calib_wheel_min, self.calib_wheel_max);
    }
    else if(step == MIE_ANALOG_CALIB_WHEEL_CENTER) {
        snprintf(line, len,
                 "ctr 0x%04x  %04x-%04x",
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
        case MIE_ANALOG_CALIB_WHEEL:
            snprintf(line, sizeof(line),
                     "tracking min=%04x max=%04x",
                     self.calib_wheel_min, self.calib_wheel_max);
            break;
        case MIE_ANALOG_CALIB_WHEEL_CENTER:
            snprintf(line, sizeof(line),
                     "captured min=%04x max=%04x center=----",
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
        TSU_LabelSetText(self.calib_step, "Calibration saved");
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
        TSU_LabelSetText(self.calib_data, "START+A to start");
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
    Settings_t *settings;

    mie_analog_calib_reset();
    settings = GetSettings();
    memset(&settings->analog, 0, sizeof(settings->analog));
    SaveSettings();
}

static void update_hint(void) {
    char line[128];

    if(mie_analog_calib_active()) {
        snprintf(line, sizeof(line),
                 "A=capture  B=cancel");
    }
    else if(self.view == MIE_VIEW_CALIB) {
        snprintf(line, sizeof(line),
                 "START+A=calibrate  Y=reset  X+D-pad=Mon/Calib/Out");
    }
    else if(self.view == MIE_VIEW_OUTPUTS) {
        snprintf(line, sizeof(line),
                 "D-pad=select  A=toggle  START+B=all  X+D-pad=Mon/Calib/Out");
    }
    else {
        snprintf(line, sizeof(line),
                 "X+D-pad=Mon/Calib/Out  START+A=calibrate  START=blink outputs");
    }

    if(self.hint_label) {
        TSU_LabelSetText(self.hint_label, line);
    }
    if(self.exit_label) {
        TSU_LabelSetText(self.exit_label, "Exit: A + B");
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
        snprintf(line, sizeof(line), "Port A: %s  MIE: %s",
                 port_mode_name(mode),
                 st ? "connected" : "waiting...");
        if(self.status_label) {
            TSU_LabelSetText(self.status_label, line);
        }
    }
    else {
        track_calib_ui(st);

        if(self.board_id[0]) {
            snprintf(line, sizeof(line), "Port A: JVS  ID: %s", self.board_id);
        }
        else {
            snprintf(line, sizeof(line), "Port A: JVS  ID: n/a");
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

static void switch_view(int dir) {
    mie_view_t prev;

    if(mie_analog_calib_active()) {
        self.view = MIE_VIEW_CALIB;
        sync_view_cardstack();
        return;
    }

    prev = self.view;
    self.view = (mie_view_t)((self.view + dir + MIE_VIEW_COUNT) % MIE_VIEW_COUNT);
    sync_view_cardstack();

    if(self.view == MIE_VIEW_OUTPUTS && prev != MIE_VIEW_OUTPUTS) {
        self.output_blink = 0;
        outputs_sync_manual();
    }
}

static void switch_output_sel(int dir) {
    self.output_sel = output_next_available(self.output_sel, dir);
}

static int combo_activated(uint32_t btns, uint32_t old_btns, uint32_t combo) {
    return (btns & combo) == combo && (old_btns & combo) != combo;
}

static void try_switch_view(int dir, uint32_t btns, uint32_t old_btns) {
    uint32_t combo;

    if(mie_analog_calib_active()) {
        return;
    }

    if(dir < 0) {
        combo = CONT_X | CONT_DPAD_LEFT;
        if(combo_activated(btns, old_btns, combo)) {
            switch_view(dir);
            return;
        }
        combo = CONT_X | CONT_DPAD_UP;
    }
    else {
        combo = CONT_X | CONT_DPAD_RIGHT;
        if(combo_activated(btns, old_btns, combo)) {
            switch_view(dir);
            return;
        }
        combo = CONT_X | CONT_DPAD_DOWN;
    }

    if(combo_activated(btns, old_btns, combo)) {
        switch_view(dir);
    }
}

static void OnExit(void) {

    if(self.exiting) {
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

    OpenMainApp();
}

static void handle_capture(void) {
    Settings_t *settings;
    const mie_analog_calib_t *calib;
    mie_analog_calib_step_t step;
    mie_state_t *st;

    if(!mie_analog_calib_active()) {
        return;
    }

    step = mie_analog_calib_current();
    st = mie_dev_state();

    if(mie_analog_calib_capture()) {
        if(st) {
            switch(step) {
            case MIE_ANALOG_CALIB_WHEEL_CENTER:
                self.calib_wheel_center = st->jvs.wheel & MIE_RAW_MASK;
                break;
            default:
                break;
            }
        }

        if(mie_analog_calib_valid()) {
            settings = GetSettings();
            calib = mie_analog_calib_get();
            settings->analog = *calib;
            SaveSettings();
        }
    }
}

static void handle_input_buttons(uint32_t nav_btns, uint32_t jvs_btns) {
    uint32_t btns = nav_btns | jvs_btns;

    if(mie_analog_calib_active()) {
        if((btns & CONT_B) && !(self.old_btns & CONT_B) && !(btns & CONT_START)) {
            mie_analog_calib_cancel();
        }
    }

    if(!mie_analog_calib_active() &&
            combo_activated(btns, self.old_btns, CONT_START | CONT_A)) {
        reset_calib_ui_tracking();
        mie_analog_calib_start();
        self.view = MIE_VIEW_CALIB;
        sync_view_cardstack();
    }

    if((btns & CONT_A) && !(self.old_btns & CONT_A) && !(btns & CONT_START)) {
        if(mie_analog_calib_active()) {
            handle_capture();
        }
        else if(self.view == MIE_VIEW_OUTPUTS &&
                !(btns & (CONT_DPAD_LEFT | CONT_DPAD_RIGHT |
                          CONT_DPAD_UP | CONT_DPAD_DOWN | CONT_X | CONT_B))) {
            outputs_toggle_selected();
        }
    }

    if(self.view == MIE_VIEW_OUTPUTS && !mie_analog_calib_active() &&
            combo_activated(btns, self.old_btns, CONT_START | CONT_B)) {
        uint32_t avail = outputs_avail_mask();

        if((self.output_manual & avail) == avail) {
            self.output_manual &= ~avail;
        }
        else {
            self.output_manual |= avail;
        }
        outputs_apply(self.output_manual);
    }

    if((btns & CONT_Y) && !(self.old_btns & CONT_Y) && !mie_analog_calib_active() &&
            !(btns & CONT_START)) {
        if(self.view != MIE_VIEW_OUTPUTS) {
            reset_calib_settings();
        }
    }

    if((btns & CONT_START) && !(self.old_btns & CONT_START) &&
            !mie_analog_calib_active() && self.view == MIE_VIEW_MONITOR &&
            !(btns & (CONT_DPAD_LEFT | CONT_DPAD_RIGHT | CONT_X | CONT_Y))) {
        self.output_blink = !self.output_blink;
        if(self.output_blink) {
            self.output_blink_step = (uint8_t)output_next_available(-1, 1);
            self.output_blink_last_ms = 0;
        }
        else {
            outputs_clear();
        }
    }

    if(self.view == MIE_VIEW_OUTPUTS && !mie_analog_calib_active()) {
        if((btns & CONT_DPAD_LEFT) && !(self.old_btns & CONT_DPAD_LEFT) &&
                !(btns & CONT_X)) {
            switch_output_sel(-1);
        }

        if((btns & CONT_DPAD_RIGHT) && !(self.old_btns & CONT_DPAD_RIGHT) &&
                !(btns & CONT_X)) {
            switch_output_sel(1);
        }

        if((btns & CONT_DPAD_UP) && !(self.old_btns & CONT_DPAD_UP) &&
                !(btns & CONT_X)) {
            switch_output_sel(-1);
        }

        if((btns & CONT_DPAD_DOWN) && !(self.old_btns & CONT_DPAD_DOWN) &&
                !(btns & CONT_X)) {
            switch_output_sel(1);
        }
    }

    try_switch_view(-1, btns, self.old_btns);
    try_switch_view(1, btns, self.old_btns);

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

    (void)ds_event;
    (void)param;

    if(action != EVENT_ACTION_UPDATE || self.exiting || !self.app) {
        return;
    }

    btns = poll_aux_buttons();
    if(combo_activated(btns, exit_btns_old, CONT_A | CONT_B)) {
        OnExit();
        return;
    }
    exit_btns_old = btns;
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
}

void MieJvsApp_Init(App_t *app) {
    Settings_t *settings;

    memset(&self, 0, sizeof(self));
    exit_btns_old = 0;
    self.app = app;
    self.view = MIE_VIEW_MONITOR;

    if(!app || !app->tsunami) {
        return;
    }

    bind_drawables();

    settings = GetSettings();
    mie_analog_calib_set(&settings->analog);

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
}
