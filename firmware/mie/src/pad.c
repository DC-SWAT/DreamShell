/**
 * DreamShell MIE firmware
 * Dreamcast controller bridge
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "maple.h"
#include "jvs.h"
#include "pad.h"

static const uint8_t pad_idle[PAD_COND_BYTES] = {
    0xFF, 0xFF, 0x00, 0x00,
    PAD_AXIS_CENTER, PAD_AXIS_CENTER, PAD_AXIS_CENTER, PAD_AXIS_CENTER
};

static uint8_t pad_mode;
static uint8_t pad_cal_on;
static uint8_t pad_map_on;
static uint8_t pad_map_raw[PAD_MAP_BYTES];
static uint16_t pad_cal[PAD_CAL_COUNT];
static uint8_t pad_cond[PAD_COND_BYTES];

static const uint8_t pad_info[PAD_DEVINFO_LEN] = {
    0x00, 0x00, 0x00, 0x01, 0x00, 0x0F, 0x06, 0xFE, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x44, 0x72, 0x65, 0x61, 0x6D, 0x63,
    0x61, 0x73, 0x74, 0x20, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C,
    0x65, 0x72, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x50, 0x72, 0x6F, 0x64, 0x75, 0x63, 0x65, 0x64, 0x20, 0x42, 0x79, 0x20,
    0x6F, 0x72, 0x20, 0x55, 0x6E, 0x64, 0x65, 0x72, 0x20, 0x4C, 0x69, 0x63,
    0x65, 0x6E, 0x73, 0x65, 0x20, 0x46, 0x72, 0x6F, 0x6D, 0x20, 0x53, 0x45,
    0x47, 0x41, 0x20, 0x45, 0x4E, 0x54, 0x45, 0x52, 0x50, 0x52, 0x49, 0x53,
    0x45, 0x53, 0x2C, 0x4C, 0x54, 0x44, 0x2E, 0x20, 0x20, 0x20, 0x20, 0x20,
    0xAE, 0x00, 0xF4, 0x01
};

static const uint8_t rumble_info[PAD_DEVINFO_LEN] = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x50, 0x75, 0x72, 0x75, 0x20, 0x50,
    0x75, 0x72, 0x75, 0x20, 0x50, 0x61, 0x63, 0x6B, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x50, 0x72, 0x6F, 0x64, 0x75, 0x63, 0x65, 0x64, 0x20, 0x42, 0x79, 0x20,
    0x6F, 0x72, 0x20, 0x55, 0x6E, 0x64, 0x65, 0x72, 0x20, 0x4C, 0x69, 0x63,
    0x65, 0x6E, 0x73, 0x65, 0x20, 0x46, 0x72, 0x6F, 0x6D, 0x20, 0x53, 0x45,
    0x47, 0x41, 0x20, 0x45, 0x4E, 0x54, 0x45, 0x52, 0x50, 0x52, 0x49, 0x53,
    0x45, 0x53, 0x2C, 0x4C, 0x54, 0x44, 0x2E, 0x20, 0x20, 0x20, 0x20, 0x20,
    0xAE, 0x00, 0xF4, 0x01
};

static uint8_t pad_exp;

uint8_t pad_active(void) {
    if (pad_mode == PAD_MODE_ON) {
        return 1;
    }
    return 0;
}

static uint8_t pad_src(void) {
    if (pad_exp) {
        return MAPLE_AP_EXP;
    }
    return MAPLE_ADDR;
}

static void pad_center(void) {
    mie_copy(pad_cond, pad_idle, PAD_COND_BYTES);
}

#define PAD_MAP_LEN   30
#define PAD_MAP_STEP  3
#define PAD_MAP_JVS_LO 0x01
#define PAD_MAP_COND_HI 0x02

#define PAD_SRC_COUNT 16
#define PAD_SRC_LO    0x7F80

static const uint8_t pad_src_bit[PAD_SRC_COUNT] = {
    JVS_HI_START, JVS_HI_UP, JVS_HI_DOWN, JVS_HI_LEFT, JVS_HI_RIGHT,
    JVS_HI_B, JVS_HI_X,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    JVS_HI_SERVICE
};

static const uint8_t pad_map[PAD_MAP_LEN] = {
    JVS_HI_START, 0x08, 0x00,
    JVS_HI_UP, 0x10, 0x00,
    JVS_HI_DOWN, 0x20, 0x00,
    JVS_HI_LEFT, 0x40, 0x00,
    JVS_HI_RIGHT, 0x80, 0x00,
    JVS_HI_B, 0x02, 0x00,
    JVS_HI_X, 0x04, PAD_MAP_COND_HI,
    JVS_HI_SERVICE, 0x01, PAD_MAP_COND_HI,
    JVS_LO_A, 0x04, PAD_MAP_JVS_LO,
    JVS_LO_Y, 0x02, PAD_MAP_JVS_LO | PAD_MAP_COND_HI
};

static void pad_player(uint8_t *blo, uint8_t *bhi, uint8_t jhi, uint8_t jlo) {
    uint8_t i;
    uint8_t sw;
    uint8_t *dst;

    for (i = 0; i < PAD_MAP_LEN; i = (uint8_t)(i + PAD_MAP_STEP)) {
        if (pad_map[i + 2] & PAD_MAP_JVS_LO) {
            sw = jlo;
        }
        else {
            sw = jhi;
        }
        if ((sw & pad_map[i]) == 0) {
            continue;
        }
        if (pad_map[i + 2] & PAD_MAP_COND_HI) {
            dst = bhi;
        }
        else {
            dst = blo;
        }
        *dst = (uint8_t)(*dst & (uint8_t)~pad_map[i + 1]);
    }
}

static uint16_t pad_u16(const uint8_t *p);

static uint16_t pad_slot(uint8_t idx) {
    return pad_u16(pad_map_raw + ((uint16_t)idx << 1));
}

static void pad_mask(uint8_t *blo, uint8_t *bhi, uint16_t mask) {
    mask = (uint16_t)(mask & PAD_BTN_MASK);
    if (mask == 0) {
        return;
    }
    *blo = (uint8_t)(*blo & (uint8_t)~(uint8_t)mask);
    *bhi = (uint8_t)(*bhi & (uint8_t)~(uint8_t)(mask >> 8));
}

static void pad_axis(uint8_t *next, uint8_t id, uint8_t value) {
    if (id == 0 || id > PAD_DST_AXIS_MAX) {
        return;
    }
    if (id == PAD_AXIS_JOYX) {
        next[PAD_COND_JOYX] = value;
    }
    else if (id == PAD_AXIS_JOYY) {
        next[PAD_COND_JOYY] = value;
    }
    else if (id == PAD_AXIS_JOY2X) {
        next[PAD_COND_JOY2X] = value;
    }
    else if (id == PAD_AXIS_JOY2Y) {
        next[PAD_COND_JOY2Y] = value;
    }
    else if (id == PAD_AXIS_LTRIG) {
        next[PAD_COND_LTRIG] = value;
    }
    else if (id == PAD_AXIS_RTRIG) {
        next[PAD_COND_RTRIG] = value;
    }
}

static uint16_t pad_div(uint32_t n, uint16_t d) {
    uint32_t q;
    uint32_t r;
    uint8_t i;

    if (d == 0) {
        return 0;
    }
    q = 0;
    r = 0;
    for (i = 0; i < 32; i++) {
        if (pkt_flag != 0) {
            return 0;
        }
        r = (r << 1) | (n >> 31);
        n <<= 1;
        q <<= 1;
        if (r >= d) {
            r -= d;
            q |= 1;
        }
    }
    if (q > 0xFFFF) {
        return 0xFFFF;
    }
    return (uint16_t)q;
}

static uint16_t pad_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint8_t pad_cal_valid(void) {
    if (pad_cal[PAD_CAL_WHEEL_MIN] >= pad_cal[PAD_CAL_WHEEL_MID]) {
        return 0;
    }
    if (pad_cal[PAD_CAL_WHEEL_MID] >= pad_cal[PAD_CAL_WHEEL_MAX]) {
        return 0;
    }
    if (pad_cal[PAD_CAL_ACCEL_MIN] >= pad_cal[PAD_CAL_ACCEL_MAX]) {
        return 0;
    }
    if (pad_cal[PAD_CAL_BRAKE_MIN] >= pad_cal[PAD_CAL_BRAKE_MAX]) {
        return 0;
    }
    return 1;
}

static void pad_cal_load(const uint8_t *payload) {
    uint8_t i;
    const uint8_t *p;

    pad_cal_on = 0;
    if (MAPLE_BUF[3] < PAD_CAL_WORDS || payload[PAD_PAY_CAL] == 0) {
        return;
    }
    p = payload + PAD_PAY_DATA;
    for (i = 0; i < PAD_CAL_COUNT; i++) {
        pad_cal[i] = pad_u16(p);
        p += 2;
    }
    if (pad_cal_valid()) {
        pad_cal_on = 1;
    }
}

static uint8_t pad_pedal(uint16_t raw, uint16_t min, uint16_t max) {
    uint16_t v;

    raw = (uint16_t)(raw & PAD_ANALOG_MASK);
    if (max <= min || raw <= min) {
        return 0;
    }
    if (raw >= max) {
        return PAD_PEDAL_MAX;
    }
    v = pad_div(((uint32_t)(raw - min) << 8) - (raw - min),
                (uint16_t)(max - min));
    if (v > PAD_PEDAL_MAX) {
        return PAD_PEDAL_MAX;
    }
    return (uint8_t)v;
}

static uint8_t pad_wheel(uint16_t raw) {
    uint16_t min;
    uint16_t mid;
    uint16_t max;
    uint16_t v;

    raw = (uint16_t)(raw & PAD_ANALOG_MASK);
    min = pad_cal[PAD_CAL_WHEEL_MIN];
    mid = pad_cal[PAD_CAL_WHEEL_MID];
    max = pad_cal[PAD_CAL_WHEEL_MAX];
    if (raw <= mid) {
        if (mid <= min) {
            return 0;
        }
        v = pad_div((uint32_t)(mid - raw) << 7, (uint16_t)(mid - min));
        if (v > PAD_WHEEL_NEG) {
            v = PAD_WHEEL_NEG;
        }
        return (uint8_t)(PAD_AXIS_CENTER - v);
    }
    if (max <= mid) {
        return (uint8_t)(PAD_AXIS_CENTER + PAD_WHEEL_POS);
    }
    v = pad_div(((uint32_t)(raw - mid) << 7) - (raw - mid),
                (uint16_t)(max - mid));
    if (v > PAD_WHEEL_POS) {
        v = PAD_WHEEL_POS;
    }
    return (uint8_t)(PAD_AXIS_CENTER + v);
}

static void pad_player_mapped(uint8_t *blo, uint8_t *bhi, uint8_t jhi, uint8_t jlo, uint8_t base) {
    uint8_t i;
    uint8_t sw;

    for (i = 0; i < PAD_SRC_COUNT; i++) {
        if ((PAD_SRC_LO >> i) & 1) {
            sw = jlo;
        }
        else {
            sw = jhi;
        }
        if ((sw & pad_src_bit[i]) == 0) {
            continue;
        }
        pad_mask(blo, bhi, pad_slot((uint8_t)(base + i)));
    }
}

static uint8_t pad_chan_value(uint16_t raw, uint8_t ch) {
    if (ch > PAD_CHAN_BRAKE) {
        return (uint8_t)(raw >> 8);
    }
    if (ch == PAD_CHAN_WHEEL) {
        return pad_wheel(raw);
    }
    if (ch == PAD_CHAN_ACCEL) {
        return pad_pedal(raw, pad_cal[PAD_CAL_ACCEL_MIN], pad_cal[PAD_CAL_ACCEL_MAX]);
    }
    return pad_pedal(raw, pad_cal[PAD_CAL_BRAKE_MIN], pad_cal[PAD_CAL_BRAKE_MAX]);
}

static uint16_t pad_an_u16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static void pad_analogs(uint8_t *next, const uint8_t *an, uint8_t stored) {
    uint8_t ch;
    uint8_t need;
    uint16_t raw;
    uint16_t axis;

    for (ch = 0; ch < JVS_PAD_ANALOG; ch++) {
        need = (uint8_t)(PAD_JVS_ANALOG + ((uint8_t)(ch + 1) << 1));
        if (stored < need) {
            return;
        }
        if (ch <= PAD_CHAN_BRAKE && !pad_cal_on) {
            continue;
        }
        raw = pad_an_u16(an + (ch << 1));
        axis = pad_slot((uint8_t)(PAD_SRC_ANALOG + ch));
        if (axis == 0 || axis > PAD_DST_AXIS_MAX) {
            continue;
        }
        pad_axis(next, (uint8_t)axis, pad_chan_value(raw, ch));
    }
}

static void pad_panel_mapped(uint8_t *blo, uint8_t *bhi, uint8_t sys) {
    uint8_t panel;

    panel = gpio5;
    if ((panel & PAD_PANEL_TEST_BIT) == 0) {
        pad_mask(blo, bhi, pad_slot(PAD_SRC_PANEL_TEST));
    }
    if ((panel & PAD_PANEL_SERVICE_BIT) == 0) {
        pad_mask(blo, bhi, pad_slot(PAD_SRC_PANEL_SERVICE));
    }
    if (sys & JVS_SYS_TEST) {
        pad_mask(blo, bhi, pad_slot(PAD_SRC_SYS_TEST));
    }
}

static void pad_apply(const uint8_t *rx, uint8_t stored) {
    uint8_t blo;
    uint8_t bhi;
    uint8_t next[PAD_COND_BYTES];
    uint16_t wheel;
    uint16_t accel;
    uint16_t brake;
    const uint8_t *an;

    if (stored < PAD_JVS_AN_ACK) {
        return;
    }
    if (rx[PAD_JVS_STATUS] != JVS_STATUS_OK) {
        return;
    }
    if (rx[PAD_JVS_SW_ACK] != JVS_REPORT_OK) {
        return;
    }
    blo = (uint8_t)PAD_BTN_IDLE;
    bhi = (uint8_t)(PAD_BTN_IDLE >> 8);
    if (pad_map_on) {
        pad_player_mapped(&blo, &bhi, rx[PAD_JVS_P1], rx[PAD_JVS_P1 + 1], 0);
        pad_player_mapped(&blo, &bhi, rx[PAD_JVS_P2], rx[PAD_JVS_P2 + 1], PAD_SRC_COUNT);
        pad_panel_mapped(&blo, &bhi, rx[PAD_JVS_SYS]);
    }
    else {
        if (rx[PAD_JVS_SYS] & JVS_SYS_TEST) {
            blo = (uint8_t)(blo & (uint8_t)~PAD_BTN_C);
        }
        pad_player(&blo, &bhi,
                   (uint8_t)(rx[PAD_JVS_P1] | rx[PAD_JVS_P2]),
                   (uint8_t)(rx[PAD_JVS_P1 + 1] | rx[PAD_JVS_P2 + 1]));
    }
    next[0] = blo;
    next[1] = bhi;
    next[PAD_COND_RTRIG] = 0;
    next[PAD_COND_LTRIG] = 0;
    next[PAD_COND_JOYX] = PAD_AXIS_CENTER;
    next[PAD_COND_JOYY] = PAD_AXIS_CENTER;
    next[PAD_COND_JOY2X] = PAD_AXIS_CENTER;
    next[PAD_COND_JOY2Y] = PAD_AXIS_CENTER;
    if (stored >= PAD_JVS_MIN && rx[PAD_JVS_AN_ACK] == JVS_REPORT_OK) {
        an = rx + PAD_JVS_ANALOG;
        wheel = ((uint16_t)an[0] << 8) | an[1];
        accel = ((uint16_t)an[2] << 8) | an[3];
        brake = ((uint16_t)an[4] << 8) | an[5];
        if (pad_map_on) {
            pad_analogs(next, an, stored);
        }
        else if (pad_cal_on) {
            next[PAD_COND_JOYX] = pad_wheel(wheel);
            next[PAD_COND_RTRIG] = pad_pedal(accel,
                                             pad_cal[PAD_CAL_ACCEL_MIN],
                                             pad_cal[PAD_CAL_ACCEL_MAX]);
            next[PAD_COND_LTRIG] = pad_pedal(brake,
                                             pad_cal[PAD_CAL_BRAKE_MIN],
                                             pad_cal[PAD_CAL_BRAKE_MAX]);
        }
    }
    if (pkt_flag != 0) {
        return;
    }
    mie_copy(pad_cond, next, PAD_COND_BYTES);
}

void pad_poll(void) {
    uint8_t rx[JVS_PAD_RX];
    uint8_t stored;

    if (pad_mode != PAD_MODE_ON) {
        return;
    }
    stored = jvs_pad_read(rx, JVS_PAD_RX);
    if (stored == 0 || pkt_flag != 0) {
        return;
    }
    pad_apply(rx, stored);
}

void pad_devinfo(void) {
    if (pad_mode != PAD_MODE_ON) {
        maple_reply_badcmd();
        return;
    }
    maple_reply_long(pad_info, PAD_DEVINFO_LEN, MAPLE_RESP_DEVINFO,
                     0, pad_src());
}

static uint8_t pad_func_ok(void) {
    if (MAPLE_PAYLOAD[0] != PAD_FUNC_LO || MAPLE_PAYLOAD[1] != PAD_FUNC_LO) {
        return 0;
    }
    if (MAPLE_PAYLOAD[2] != PAD_FUNC_LO || MAPLE_PAYLOAD[3] != PAD_FUNC_HI) {
        return 0;
    }
    return 1;
}

void pad_getcond(void) {
    uint8_t i;

    if (pad_mode != PAD_MODE_ON) {
        maple_reply_badcmd();
        return;
    }
    if (!pad_func_ok()) {
        maple_reply(MAPLE_RESP_BADFUNC, 0, pad_src(), 0);
        return;
    }
    MAPLE_BUF[0] = MAPLE_RESP_DATATRF;
    MAPLE_BUF[1] = 0;
    MAPLE_BUF[2] = pad_src();
    MAPLE_BUF[3] = PAD_COND_WORDS;
    MAPLE_BUF[4] = PAD_FUNC_LO;
    MAPLE_BUF[5] = PAD_FUNC_LO;
    MAPLE_BUF[6] = PAD_FUNC_LO;
    MAPLE_BUF[7] = PAD_FUNC_HI;
    for (i = 0; i < PAD_COND_BYTES; i++) {
        MAPLE_BUF[8 + i] = pad_cond[i];
    }
    MAPLE_LREG = PAD_COND_WORDS;
    maple_reply_tbuf();
}

static void pad_sub_ok(uint8_t src) {
    maple_reply(MAPLE_RESP_OK, 0, src, 0);
}

static uint8_t rumble_func_ok(void) {
    if (MAPLE_PAYLOAD[0] != 0 || MAPLE_PAYLOAD[1] != 0) {
        return 0;
    }
    if (MAPLE_PAYLOAD[2] != MAPLE_FUNC_RUMBLE || MAPLE_PAYLOAD[3] != 0) {
        return 0;
    }
    return 1;
}

static void rumble_clear(void) {
    uint8_t i;

    for (i = 0; i < RUMBLE_BYTES; i++) {
        rumble_fx[i] = 0;
    }
}

static void pad_rumble(uint8_t cmd) {
    uint8_t i;

    if (cmd == MAPLE_CMD_DEVINFO) {
        maple_reply_long(rumble_info, PAD_DEVINFO_LEN, MAPLE_RESP_DEVINFO,
                         0, MAPLE_AP_RUMBLE);
        return;
    }
    if (cmd == MAPLE_CMD_ALLINFO) {
        maple_reply_long(rumble_info, PAD_DEVINFO_LEN, MAPLE_RESP_ALLINFO,
                         0, MAPLE_AP_RUMBLE);
        return;
    }
    if (cmd == MAPLE_CMD_RESET) {
        rumble_clear();
        pad_sub_ok(MAPLE_AP_RUMBLE);
        return;
    }
    if (cmd == MAPLE_CMD_SETCOND) {
        if (!rumble_func_ok() || MAPLE_BUF[3] < 2) {
            maple_reply(MAPLE_RESP_BADFUNC, 0, MAPLE_AP_RUMBLE, 0);
            return;
        }
        for (i = 0; i < RUMBLE_BYTES; i++) {
            rumble_fx[i] = MAPLE_PAYLOAD[4 + i];
        }
        rumble_seq++;
        pad_sub_ok(MAPLE_AP_RUMBLE);
        return;
    }
    if (cmd == MAPLE_CMD_GETCOND) {
        if (!rumble_func_ok()) {
            maple_reply(MAPLE_RESP_BADFUNC, 0, MAPLE_AP_RUMBLE, 0);
            return;
        }
        MAPLE_BUF[0] = MAPLE_RESP_DATATRF;
        MAPLE_BUF[1] = 0;
        MAPLE_BUF[2] = MAPLE_AP_RUMBLE;
        MAPLE_BUF[3] = 2;
        MAPLE_BUF[4] = 0;
        MAPLE_BUF[5] = 0;
        MAPLE_BUF[6] = MAPLE_FUNC_RUMBLE;
        MAPLE_BUF[7] = 0;
        for (i = 0; i < RUMBLE_BYTES; i++) {
            MAPLE_BUF[8 + i] = rumble_fx[i];
        }
        MAPLE_LREG = 2;
        maple_reply_tbuf();
        return;
    }
    pad_sub_ok(MAPLE_AP_RUMBLE);
}

uint8_t pad_maple(uint8_t cmd) {
    uint8_t dest;

    if (!pad_active() || !pad_exp) {
        return 0;
    }
    dest = MAPLE_BUF[1];
    if (dest == MAPLE_AP_VMU) {
        pad_sub_ok(MAPLE_AP_VMU);
        return 1;
    }
    if (dest == MAPLE_AP_RUMBLE) {
        pad_rumble(cmd);
        return 1;
    }
    return 0;
}

static void pad_reply(void) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = MAPLE_IO_PAD_ACK;
    reply_buf[5] = pad_mode;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
}

void ior_pad(uint8_t *payload) {
    pad_map_on = 0;
    if (payload[PAD_PAY_MODE] == PAD_MODE_OFF) {
        pad_mode = PAD_MODE_OFF;
        pad_cal_on = 0;
        pad_exp = 0;
        rumble_clear();
    }
    else {
        pad_mode = PAD_MODE_ON;
        pad_center();
        pad_cal_load(payload);
        pad_exp = 0;
        if (payload[PAD_PAY_FEAT] & PAD_FEAT_VMU) {
            pad_exp = 1;
        }
        rumble_clear();
    }
    pad_reply();
}

void ior_pad_map(uint8_t *payload) {
    uint8_t off;
    uint8_t n;
    uint8_t i;
    uint8_t words;

    off = payload[PAD_PAY_MAP_OFF];
    words = MAPLE_BUF[3];
    pad_map_on = 0;
    if (words == 0 || off >= PAD_MAP_BYTES) {
        pad_reply();
        return;
    }
    n = (uint8_t)((words << 2) - PAD_PAY_MAP_DATA);
    if ((uint16_t)off + n > PAD_MAP_BYTES) {
        n = (uint8_t)(PAD_MAP_BYTES - off);
    }
    for (i = 0; i < n; i++) {
        pad_map_raw[off + i] = payload[PAD_PAY_MAP_DATA + i];
    }
    if ((uint16_t)off + n >= PAD_MAP_BYTES) {
        pad_map_on = 1;
    }
    pad_reply();
}
