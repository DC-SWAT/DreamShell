/* DreamShell ##version##

   main.c - MIE/JVS manager
   Copyright (C) 2026 Ruslan Rostovtsev

*/

#include "ds.h"
#include <dc/maple/mie.h>

static const char *mie_cmd_port_name(mie_port0_mode_t mode) {
    switch(mode) {
    case MIE_PORT0_JVS:
        return "JVS";
    case MIE_PORT0_MAPLE:
        return "Maple";
    case MIE_PORT0_UNKNOWN:
        return "unknown";
    default:
        return "invalid";
    }
}

static const char *mie_cmd_calib_step(mie_analog_calib_step_t step) {
    switch(step) {
    case MIE_ANALOG_CALIB_WHEEL:
        return "wheel range: turn fully both ways";
    case MIE_ANALOG_CALIB_WHEEL_CENTER:
        return "wheel center: hold centered";
    case MIE_ANALOG_CALIB_ACCEL:
        return "accelerator: release then press fully";
    case MIE_ANALOG_CALIB_BRAKE:
        return "brake: release then press fully";
    default:
        return "idle";
    }
}

static void mie_cmd_hex_dump(const uint8_t *data, size_t size) {
    size_t i;

    for(i = 0; i < size; i++) {
        if(i && !(i & 15)) {
            ds_printf("\n");
        }
        ds_printf("%02x ", data[i]);
    }
    ds_printf("\n");
}

static mie_state_t *mie_cmd_dev_state(void) {
    maple_device_t *dev;

    dev = maple_enum_dev(0, 0);
    if(!dev || !(dev->info.functions & MAPLE_FUNC_MIE)) {
        return NULL;
    }

    return (mie_state_t *)maple_dev_status(dev);
}

static int mie_cmd_check_jvs(void) {
    if(mie_port0_mode() != MIE_PORT0_JVS) {
        ds_printf("DS_ERROR: Port A is not in JVS mode (%s)\n",
                  mie_cmd_port_name(mie_port0_mode()));
        return 0;
    }
    return 1;
}

static int mie_cmd_status(int argc, char *argv[]) {
    char id[MIE_ID_SIZE];
    mie_port0_mode_t mode;

    (void)argc;
    (void)argv;

    mode = mie_port0_mode();
    ds_printf("DS_INFO: Port A mode: %s\n", mie_cmd_port_name(mode));

    if(mode != MIE_PORT0_JVS) {
        return CMD_OK;
    }

    if(mie_get_id(id)) {
        ds_printf("DS_INFO: Board ID: %s\n", id);
    }
    else {
        ds_printf("DS_INFO: Board ID: unavailable\n");
    }

    ds_printf("DS_INFO: Outputs: 0x%08lx\n",
              (unsigned long)mie_jvs_get_outputs());

    if(mie_analog_calib_active()) {
        ds_printf("DS_INFO: Calibration: active (%s)\n",
                  mie_cmd_calib_step(mie_analog_calib_current()));
    }
    else if(mie_analog_calib_valid()) {
        ds_printf("DS_INFO: Calibration: valid\n");
    }
    else {
        ds_printf("DS_INFO: Calibration: not set\n");
    }

    return CMD_OK;
}

static void mie_cmd_print_inputs(const mie_state_t *st) {
    ds_printf("DS_INFO: Mapped controller:\n");
    ds_printf("DS_INFO: buttons=0x%08lx joyx=%d ltrig=%d rtrig=%d\n",
              (unsigned long)st->cont.buttons,
              st->cont.joyx, st->cont.ltrig, st->cont.rtrig);
    ds_printf("DS_INFO: JVS sys=0x%02x p1=0x%04x p2=0x%04x\n",
              st->jvs.system.raw, st->jvs.p1.raw, st->jvs.p2.raw);
    ds_printf("DS_INFO: panel dip=0x%x psw=%d%d\n",
              st->jvs.panel.dip.raw,
              st->jvs.panel.psw.test, st->jvs.panel.psw.service);
    ds_printf("DS_INFO: coin0=%u coin1=%u pulse=0x%02x\n",
              st->jvs.coin[0].count, st->jvs.coin[1].count, st->jvs.coin_pulse);
    ds_printf("DS_INFO: wheel=0x%04x accel=0x%04x brake=0x%04x\n",
              st->jvs.wheel, st->jvs.accel, st->jvs.brake);
    if(mie_analog_calib_valid()) {
        ds_printf("DS_INFO: norm wheel=%d accel=%d brake=%d\n",
                  mie_analog_norm_wheel(st->jvs.wheel),
                  mie_analog_norm_accel(st->jvs.accel),
                  mie_analog_norm_brake(st->jvs.brake));
    }
}

static int mie_cmd_input(int argc, char *argv[]) {
    mie_state_t *st;

    (void)argc;
    (void)argv;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    st = mie_cmd_dev_state();
    if(!st) {
        ds_printf("DS_ERROR: MIE device not found\n");
        return CMD_ERROR;
    }

    mie_cmd_print_inputs(st);
    return CMD_OK;
}

static int mie_cmd_output(int argc, char *argv[]) {
    uint32_t mask;
    int idx;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    if(argc < 3) {
        ds_printf("DS_INFO: Outputs: 0x%08lx\n",
                  (unsigned long)mie_jvs_get_outputs());
        return CMD_OK;
    }

    if(!strcmp(argv[2], "set") && argc > 3) {
        mask = (uint32_t)strtoul(argv[3], NULL, 0);
        if(!mie_jvs_set_outputs(mask, true)) {
            ds_printf("DS_ERROR: Failed to set outputs\n");
            return CMD_ERROR;
        }
        ds_printf("DS_OK: Outputs set to 0x%08lx\n",
                  (unsigned long)mie_jvs_get_outputs());
        return CMD_OK;
    }

    if(!strcmp(argv[2], "clear")) {
        if(!mie_jvs_set_outputs(0, true)) {
            ds_printf("DS_ERROR: Failed to clear outputs\n");
            return CMD_ERROR;
        }
        ds_printf("DS_OK: Outputs cleared\n");
        return CMD_OK;
    }

    if(!strcmp(argv[2], "on") && argc > 3) {
        idx = atoi(argv[3]);
        if(!mie_jvs_set_output((uint8_t)idx, true, true)) {
            ds_printf("DS_ERROR: Failed to turn output %d on\n", idx);
            return CMD_ERROR;
        }
        ds_printf("DS_OK: Output %d on (0x%08lx)\n", idx,
                  (unsigned long)mie_jvs_get_outputs());
        return CMD_OK;
    }

    if(!strcmp(argv[2], "off") && argc > 3) {
        idx = atoi(argv[3]);
        if(!mie_jvs_set_output((uint8_t)idx, false, true)) {
            ds_printf("DS_ERROR: Failed to turn output %d off\n", idx);
            return CMD_ERROR;
        }
        ds_printf("DS_OK: Output %d off (0x%08lx)\n", idx,
                  (unsigned long)mie_jvs_get_outputs());
        return CMD_OK;
    }

    ds_printf("DS_ERROR: Unknown output action '%s'\n", argv[2]);
    return CMD_ERROR;
}

static void mie_cmd_print_calib(const mie_analog_calib_t *c) {
    ds_printf("DS_INFO: wheel min=0x%04x center=0x%04x max=0x%04x\n",
              c->wheel.min, c->wheel.center, c->wheel.max);
    ds_printf("DS_INFO: accel min=0x%04x max=0x%04x\n",
              c->accel.min, c->accel.max);
    ds_printf("DS_INFO: brake min=0x%04x max=0x%04x\n",
              c->brake.min, c->brake.max);
}

static int mie_cmd_calib(int argc, char *argv[]) {
    const mie_analog_calib_t *calib;
    const mie_state_t *st;
    Settings_t *settings;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    if(argc < 3) {
        ds_printf("Usage: mie calib start|cancel|capture|status|reset|show|load\n");
        return CMD_NO_ARG;
    }

    if(!strcmp(argv[2], "start")) {
        mie_analog_calib_start();
        ds_printf("DS_OK: Calibration started\n");
        ds_printf("DS_INFO: %s\n", mie_cmd_calib_step(mie_analog_calib_current()));
        ds_printf("DS_INFO: Run 'mie calib capture' to confirm step\n");
        return CMD_OK;
    }

    if(!strcmp(argv[2], "cancel")) {
        mie_analog_calib_cancel();
        ds_printf("DS_OK: Calibration cancelled\n");
        return CMD_OK;
    }

    if(!strcmp(argv[2], "reset")) {
        mie_analog_calib_reset();
        settings = GetSettings();
        memset(&settings->analog, 0, sizeof(settings->analog));
        if(!SaveSettings()) {
            ds_printf("DS_ERROR: Failed to save settings\n");
            return CMD_ERROR;
        }
        ds_printf("DS_OK: Calibration reset\n");
        return CMD_OK;
    }

    if(!strcmp(argv[2], "load")) {
        settings = GetSettings();
        mie_analog_calib_set(&settings->analog);
        if(!mie_analog_calib_valid()) {
            ds_printf("DS_INFO: No valid calibration in settings\n");
            return CMD_OK;
        }
        ds_printf("DS_OK: Calibration loaded from settings\n");
        mie_cmd_print_calib(mie_analog_calib_get());
        return CMD_OK;
    }

    if(!strcmp(argv[2], "show")) {
        if(!mie_analog_calib_valid()) {
            ds_printf("DS_INFO: No valid calibration\n");
            return CMD_OK;
        }
        mie_cmd_print_calib(mie_analog_calib_get());
        return CMD_OK;
    }

    if(!strcmp(argv[2], "status")) {
        if(mie_analog_calib_active()) {
            st = mie_cmd_dev_state();
            ds_printf("DS_INFO: Step: %s\n",
                      mie_cmd_calib_step(mie_analog_calib_current()));
            ds_printf("DS_INFO: Run 'mie calib capture' to confirm step\n");
            if(st) {
                ds_printf("DS_INFO: raw wheel=0x%04x accel=0x%04x brake=0x%04x\n",
                          st->jvs.wheel, st->jvs.accel, st->jvs.brake);
            }
        }
        else if(mie_analog_calib_valid()) {
            ds_printf("DS_INFO: Calibration valid\n");
            mie_cmd_print_calib(mie_analog_calib_get());
        }
        else {
            ds_printf("DS_INFO: Not calibrated\n");
        }
        return CMD_OK;
    }

    if(!strcmp(argv[2], "capture")) {
        if(!mie_analog_calib_active()) {
            ds_printf("DS_ERROR: No calibration session active\n");
            return CMD_ERROR;
        }

        if(mie_analog_calib_capture()) {
            if(mie_analog_calib_valid()) {
                settings = GetSettings();
                calib = mie_analog_calib_get();
                settings->analog = *calib;
                if(!SaveSettings()) {
                    ds_printf("DS_ERROR: Failed to save settings\n");
                    return CMD_ERROR;
                }
                ds_printf("DS_OK: Calibration saved\n");
                mie_cmd_print_calib(calib);
            }
            else {
                ds_printf("DS_ERROR: Calibration invalid\n");
                return CMD_ERROR;
            }
        }
        else {
            ds_printf("DS_OK: Step captured\n");
            ds_printf("DS_INFO: %s\n", mie_cmd_calib_step(mie_analog_calib_current()));
            ds_printf("DS_INFO: Run 'mie calib capture' to confirm step\n");
        }
        return CMD_OK;
    }

    ds_printf("DS_ERROR: Unknown calib action '%s'\n", argv[2]);
    return CMD_ERROR;
}

static int mie_cmd_eeprom(int argc, char *argv[]) {
    uint8_t buf[MIE_EEPROM_SIZE];
    char path[NAME_MAX];
    file_t fd;
    ssize_t n;
    int fix_crc = 0;
    int i;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    if(argc < 3) {
        ds_printf("Usage: mie eeprom read [file]|write <file> [--fix-crc]|crc\n");
        return CMD_NO_ARG;
    }

    if(!strcmp(argv[2], "read")) {
        if(!mie_get_eeprom(buf)) {
            ds_printf("DS_ERROR: EEPROM read failed\n");
            return CMD_ERROR;
        }

        if(argc > 3) {
            fs_normalize_path(argv[3], path);
            fd = fs_open(path, O_WRONLY | O_CREAT | O_TRUNC);
            if(fd == FILEHND_INVALID) {
                ds_printf("DS_ERROR: Can't create %s\n", path);
                return CMD_ERROR;
            }
            n = fs_write(fd, buf, MIE_EEPROM_SIZE);
            fs_close(fd);
            if(n != MIE_EEPROM_SIZE) {
                ds_printf("DS_ERROR: Write failed\n");
                return CMD_ERROR;
            }
            ds_printf("DS_OK: EEPROM saved to %s\n", path);
        }
        else {
            ds_printf("DS_INFO: EEPROM (%d bytes):\n", MIE_EEPROM_SIZE);
            mie_cmd_hex_dump(buf, MIE_EEPROM_SIZE);
        }
        return CMD_OK;
    }

    if(!strcmp(argv[2], "write") && argc > 3) {
        for(i = 4; i < argc; i++) {
            if(!strcmp(argv[i], "--fix-crc")) {
                fix_crc = 1;
            }
        }

        fs_normalize_path(argv[3], path);
        fd = fs_open(path, O_RDONLY);
        if(fd == FILEHND_INVALID) {
            ds_printf("DS_ERROR: Can't open %s\n", path);
            return CMD_ERROR;
        }

        if((size_t)fs_total(fd) != MIE_EEPROM_SIZE) {
            fs_close(fd);
            ds_printf("DS_ERROR: File must be %d bytes\n", MIE_EEPROM_SIZE);
            return CMD_ERROR;
        }

        n = fs_read(fd, buf, MIE_EEPROM_SIZE);
        fs_close(fd);
        if(n != MIE_EEPROM_SIZE) {
            ds_printf("DS_ERROR: Read failed\n");
            return CMD_ERROR;
        }

        if(fix_crc) {
            mie_eeprom_fix_crc(buf);
        }

        if(!mie_set_eeprom(buf)) {
            ds_printf("DS_ERROR: EEPROM write failed\n");
            return CMD_ERROR;
        }

        ds_printf("DS_OK: EEPROM written from %s\n", path);
        return CMD_OK;
    }

    if(!strcmp(argv[2], "crc")) {
        if(!mie_get_eeprom(buf)) {
            ds_printf("DS_ERROR: EEPROM read failed\n");
            return CMD_ERROR;
        }
        ds_printf("DS_INFO: EEPROM CRC16: 0x%04x\n",
                  mie_eeprom_crc16(buf, MIE_EEPROM_SIZE));
        return CMD_OK;
    }

    ds_printf("DS_ERROR: Unknown eeprom action '%s'\n", argv[2]);
    return CMD_ERROR;
}

static int mie_cmd_fw(int argc, char *argv[]) {
    char path[NAME_MAX];

    if(argc < 4 || strcmp(argv[2], "load")) {
        ds_printf("Usage: mie fw load <file>\n");
        return CMD_NO_ARG;
    }

    fs_normalize_path(argv[3], path);
    ds_printf("DS_PROCESS: Loading MIE firmware from %s...\n", path);

    if(!mie_init_fw(path)) {
        ds_printf("DS_ERROR: Firmware load failed\n");
        return CMD_ERROR;
    }

    ds_printf("DS_OK: Firmware loaded, port mode: %s\n",
              mie_cmd_port_name(mie_port0_mode()));
    return CMD_OK;
}

static int mie_cmd_coin(int argc, char *argv[]) {
    mie_jvs_coin_slot_t slots[MIE_JVS_COIN_SLOTS];
    const mie_state_t *st;
    int slot, amount;
    int i;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    if(argc < 3) {
        ds_printf("Usage: mie coin show|read|add <slot> <amount>|dec <slot> <amount>\n");
        return CMD_NO_ARG;
    }

    if(!strcmp(argv[2], "show")) {
        st = mie_cmd_dev_state();
        for(i = 0; i < MIE_JVS_COIN_SLOTS; i++) {
            ds_printf("DS_INFO: slot %d count=%u meter=%u\n",
                      i,
                      st ? st->jvs.coin[i].count : 0,
                      mie_get_coin_meter((uint8_t)i));
        }
        return CMD_OK;
    }

    if(!strcmp(argv[2], "read")) {
        if(!mie_read_coin_input(slots, MIE_JVS_COIN_SLOTS)) {
            ds_printf("DS_ERROR: Coin read failed\n");
            return CMD_ERROR;
        }
        for(i = 0; i < MIE_JVS_COIN_SLOTS; i++) {
            ds_printf("DS_INFO: slot %d raw=0x%04x count=%u busy=0x%04x\n",
                      i, slots[i].raw, slots[i].count,
                      (unsigned)(slots[i].raw & MIE_JVS_COIN_BUSY));
        }
        return CMD_OK;
    }

    if((!strcmp(argv[2], "add") || !strcmp(argv[2], "dec")) && argc > 4) {
        slot = atoi(argv[3]);
        amount = atoi(argv[4]);
        if(slot < 0 || slot >= MIE_JVS_COIN_SLOTS || amount <= 0) {
            ds_printf("DS_ERROR: Invalid slot or amount\n");
            return CMD_ERROR;
        }

        if(!strcmp(argv[2], "add")) {
            if(!mie_coin_add((uint8_t)slot, (uint16_t)amount, true)) {
                ds_printf("DS_ERROR: Coin add failed\n");
                return CMD_ERROR;
            }
            ds_printf("DS_OK: Added %d credits to slot %d\n", amount, slot);
        }
        else {
            if(!mie_coin_decrease((uint8_t)slot, (uint16_t)amount, true)) {
                ds_printf("DS_ERROR: Coin decrease failed\n");
                return CMD_ERROR;
            }
            ds_printf("DS_OK: Removed %d credits from slot %d\n", amount, slot);
        }
        return CMD_OK;
    }

    ds_printf("DS_ERROR: Unknown coin action '%s'\n", argv[2]);
    return CMD_ERROR;
}

static int mie_cmd_dip(int argc, char *argv[]) {
    const mie_state_t *st;

    (void)argc;
    (void)argv;

    if(!mie_cmd_check_jvs()) {
        return CMD_ERROR;
    }

    st = mie_cmd_dev_state();
    if(!st) {
        ds_printf("DS_ERROR: MIE device not found\n");
        return CMD_ERROR;
    }

    ds_printf("DS_INFO: DIP raw=0x%02x sw1=%u sw2=%u sw3=%u sw4=%u\n",
              st->jvs.panel.dip.raw,
              st->jvs.panel.dip.sw1, st->jvs.panel.dip.sw2,
              st->jvs.panel.dip.sw3, st->jvs.panel.dip.sw4);
    ds_printf("DS_INFO: coin0=%u coin1=%u\n",
              st->jvs.coin[0].count, st->jvs.coin[1].count);
    return CMD_OK;
}

int main(int argc, char *argv[]) {

    if(argc < 2) {
        ds_printf("Usage: mie command [args...]\n\n"
                  "Commands:\n"
                  " status                     Port mode, board ID, outputs, calibration\n"
                  " input                      JVS inputs snapshot\n"
                  " output [set <mask>|on <n>|off <n>|clear]\n"
                  " calib start|cancel|capture|status|reset|show|load\n"
                  " eeprom read [file]|write <file> [--fix-crc]|crc\n"
                  " fw load <file>             Upload Z80 firmware\n"
                  " coin show|read|add <slot> <n>|dec <slot> <n>\n"
                  " dip                        DIP switches and coin counts\n");
        return CMD_NO_ARG;
    }

    if(!strcmp(argv[1], "status")) {
        return mie_cmd_status(argc, argv);
    }

    if(!strcmp(argv[1], "input")) {
        return mie_cmd_input(argc, argv);
    }

    if(!strcmp(argv[1], "output")) {
        return mie_cmd_output(argc, argv);
    }

    if(!strcmp(argv[1], "calib")) {
        return mie_cmd_calib(argc, argv);
    }

    if(!strcmp(argv[1], "eeprom")) {
        return mie_cmd_eeprom(argc, argv);
    }

    if(!strcmp(argv[1], "fw")) {
        return mie_cmd_fw(argc, argv);
    }

    if(!strcmp(argv[1], "coin")) {
        return mie_cmd_coin(argc, argv);
    }

    if(!strcmp(argv[1], "dip")) {
        return mie_cmd_dip(argc, argv);
    }

    ds_printf("DS_ERROR: Unknown mie command '%s'\n", argv[1]);
    return CMD_ERROR;
}
