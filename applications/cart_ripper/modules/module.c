/* DreamShell ##version##

   module.c - Cart Ripper app module
   Copyright (C) 2026 SWAT
*/

#include <ds.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <isoldr.h>
#include <naomi/cart.h>
#include "naomi_5881.h"
#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_cart_ripper);

#define RIP_BUF_SIZE 0x20000
#define UI_UPDATE_INTERVAL 1000
#define APP_PAGE_MAIN 0
#define APP_PAGE_BROWSER 1
#define DEFAULT_GAMES_DIR "games"
#define M2_STUB_RAM_MIN 0x0d000000
#define M2_STUB_MIN_SIZE 0x1000
#define M2_WIN_FLAGS0 0x00020004
#define M2_WIN_SIZE0 0x00020000
#define M2_WIN_SIZE_MIN 0x1000
#define M2_WIN_SIZE_MAX 0x80000
#define M2_WIN_MAX 16
#define M2_HOLE_CHECK 32
#define M2_ADDR_MASK 0x1fffffff

static void *cart_ripper_thread(void *arg);
static void *cart_detect_thread(void *arg);
static void start_detect(void);
static void refresh_cart_info(void);
static void reset_rip_state(const char *status);
static void sanitize_name(char *dst, size_t dst_size, const char *src);
static void trim_pad(char *dst, size_t dst_size, const char *src, size_t src_len);
static void pick_title(char *dst, size_t dst_size, const naomi_cart_header_t *hdr);
static const char *cart_type_name(naomi_cart_type_t type);
static void format_size(char *dst, size_t dst_size, size_t bytes);
static void format_mask_list(char *dst, size_t dst_size, uint8_t mask,
    const char **names, int count, const char *any_text);
static void set_rip_controls(int running);
static uint32_t m2_key_from_serial(const char serial[4]);

static struct {
    App_t *app;
    GUI_Widget *pages;
    GUI_Widget *gname;
    GUI_Widget *pbar;
    GUI_Widget *start_btn;
    GUI_Widget *cancel_btn;
    GUI_Widget *refresh_btn;
    GUI_Widget *launch_btn;
    GUI_Widget *browse_btn;
    GUI_Widget *speed_label;
    GUI_Widget *time_label;
    GUI_Widget *bytes_label;
    GUI_Widget *status_label;
    GUI_Widget *destination_path;
    GUI_Widget *file_browser;
    GUI_Widget *info_title;
    GUI_Widget *info_publisher;
    GUI_Widget *info_serial;
    GUI_Widget *info_type;
    GUI_Widget *info_size;
    GUI_Widget *info_region;
    GUI_Widget *info_video;
    GUI_Widget *info_board;
    GUI_Widget *decrypt_chk;
    volatile int rip_active;
    volatile int detect_busy;
    int cart_ok;
    int decrypt_m2;
    int can_decrypt_m2;
    naomi_cart_type_t cart_type;
    size_t dump_size;
    uint32_t dump_flags;
    uint64_t start_time;
    uint64_t processed_bytes;
    uint64_t last_ui_update;
    char selected_path[NAME_MAX];
    char dst_file[NAME_MAX];
    int dump_ready;
} self;

static void set_rip_controls(int running) {
    GUI_WidgetSetEnabled(self.start_btn, !running);
    GUI_WidgetSetEnabled(self.cancel_btn, running);
    GUI_WidgetSetEnabled(self.refresh_btn, !running);
    GUI_WidgetSetEnabled(self.browse_btn, !running);
    GUI_WidgetSetEnabled(self.launch_btn, !running && self.dump_ready);
    if(self.decrypt_chk) {
        GUI_WidgetSetEnabled(self.decrypt_chk, !running && self.can_decrypt_m2);
        if(!self.can_decrypt_m2) {
            GUI_WidgetSetState(self.decrypt_chk, 0);
        }
    }
}

static void reset_rip_state(const char *status) {
    self.rip_active = 0;
    set_rip_controls(0);
    if(!self.cart_ok) {
        GUI_WidgetSetEnabled(self.start_btn, 0);
    }
    GUI_LabelSetText(self.speed_label, "Speed: --");
    GUI_LabelSetText(self.time_label, "Time left: --");
    GUI_LabelSetText(self.bytes_label, "Done: --");
    GUI_ProgressBarSetPosition(self.pbar, 0.0);
    if(status) {
        GUI_LabelSetText(self.status_label, status);
    }
    self.start_time = 0;
    self.processed_bytes = 0;
    self.last_ui_update = 0;
}

static void sanitize_name(char *dst, size_t dst_size, const char *src) {
    size_t o = 0;
    const char *p = src;

    if(!dst || !dst_size) {
        return;
    }

    if(!p) {
        p = "";
    }

    while(*p == ' ') {
        p++;
    }

    while(*p && o + 1 < dst_size) {
        char c = *p++;
        if(c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' ||
                c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
        if((unsigned char)c < 32) {
            continue;
        }
        dst[o++] = c;
    }

    while(o > 0 && (dst[o - 1] == '_' || dst[o - 1] == '.')) {
        o--;
    }
    dst[o] = '\0';

    if(o >= 4) {
        char *ext = dst + o - 4;
        if(!strcasecmp(ext, ".dni")) {
            *ext = '\0';
            o -= 4;
            while(o > 0 && (dst[o - 1] == '_' || dst[o - 1] == '.')) {
                dst[--o] = '\0';
            }
        }
    }

    if(!dst[0]) {
        strncpy(dst, "naomi_cart", dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

static void trim_pad(char *dst, size_t dst_size, const char *src, size_t src_len) {
    size_t i;
    size_t start = 0;
    size_t end;
    size_t n;

    if(!dst || !dst_size) {
        return;
    }
    dst[0] = '\0';
    if(!src || !src_len) {
        return;
    }

    while(start < src_len && src[start] == ' ') {
        start++;
    }
    end = src_len;
    while(end > start && src[end - 1] == ' ') {
        end--;
    }
    n = end - start;
    if(n >= dst_size) {
        n = dst_size - 1;
    }
    for(i = 0; i < n; i++) {
        dst[i] = src[start + i];
    }
    dst[n] = '\0';
}

static void pick_title(char *dst, size_t dst_size, const naomi_cart_header_t *hdr) {
    int i;

    trim_pad(dst, dst_size, hdr->regional_name[NAOMI_REGION_USA], 32);
    if(dst[0]) {
        return;
    }
    trim_pad(dst, dst_size, hdr->regional_name[NAOMI_REGION_JAPAN], 32);
    if(dst[0]) {
        return;
    }
    for(i = 0; i < 8; i++) {
        trim_pad(dst, dst_size, hdr->regional_name[i], 32);
        if(dst[0]) {
            return;
        }
    }
    strncpy(dst, "naomi_cart", dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static const char *cart_type_name(naomi_cart_type_t type) {
    switch(type) {
        case NAOMI_CART_M1:
            return "M1";
        case NAOMI_CART_M2:
            return "M2";
        case NAOMI_CART_M4:
            return "M4";
        case NAOMI_CART_DIMM:
            return "DIMM";
        default:
            return "None";
    }
}

static void format_size(char *dst, size_t dst_size, size_t bytes) {
    if(bytes >= 1024 * 1024 && (bytes % (1024 * 1024)) == 0) {
        snprintf(dst, dst_size, "%u MB", (unsigned)(bytes / (1024 * 1024)));
    }
    else if(bytes >= 1024 * 1024) {
        snprintf(dst, dst_size, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    }
    else if(bytes >= 1024) {
        snprintf(dst, dst_size, "%u KB", (unsigned)(bytes / 1024));
    }
    else {
        snprintf(dst, dst_size, "%u B", (unsigned)bytes);
    }
}

static void format_mask_list(char *dst, size_t dst_size, uint8_t mask,
        const char **names, int count, const char *any_text) {
    int i;
    int first = 1;
    int w;
    size_t used = 0;

    dst[0] = '\0';
    if(!mask) {
        snprintf(dst, dst_size, "%s", any_text);
        return;
    }

    for(i = 0; i < count; i++) {
        if(!(mask & (1u << i))) {
            continue;
        }
        if(used >= dst_size - 1) {
            break;
        }
        if(!first) {
            w = snprintf(dst + used, dst_size - used, ", ");
            if(w < 0) {
                break;
            }
            used += (size_t)w;
            if(used >= dst_size - 1) {
                break;
            }
        }
        w = snprintf(dst + used, dst_size - used, "%s", names[i]);
        if(w < 0) {
            break;
        }
        used += (size_t)w;
        first = 0;
    }
    if(first) {
        snprintf(dst, dst_size, "%s", any_text);
    }
}

static void update_ui_display(void) {
    uint64_t current_time = timer_ms_gettime64();
    uint64_t elapsed_time;
    char speed_text[64];
    char time_text[64];
    char bytes_text[64];
    char size_done[32];
    char size_total[32];
    double progress;
    double speed_kb;
    uint64_t remain_sec;

    if(!self.dump_size) {
        return;
    }

    if(current_time - self.last_ui_update < UI_UPDATE_INTERVAL) {
        return;
    }

    elapsed_time = current_time - self.start_time;
    if(elapsed_time < UI_UPDATE_INTERVAL || !self.processed_bytes) {
        return;
    }

    progress = (double)self.processed_bytes / (double)self.dump_size;
    if(progress > 1.0) {
        progress = 1.0;
    }
    GUI_ProgressBarSetPosition(self.pbar, progress);

    speed_kb = ((double)self.processed_bytes / (elapsed_time / 1000.0)) / 1024.0;
    snprintf(speed_text, sizeof(speed_text), "Speed: %.1f KB/s", speed_kb);

    format_size(size_done, sizeof(size_done), (size_t)self.processed_bytes);
    format_size(size_total, sizeof(size_total), self.dump_size);
    snprintf(bytes_text, sizeof(bytes_text), "Done: %s / %s", size_done, size_total);

    if(speed_kb > 0.0 && self.processed_bytes < self.dump_size) {
        remain_sec = (uint64_t)(((double)(self.dump_size - self.processed_bytes) / 1024.0) / speed_kb);
        if(remain_sec >= 3600) {
            snprintf(time_text, sizeof(time_text), "Time left: %uh %um",
                (unsigned)(remain_sec / 3600),
                (unsigned)((remain_sec % 3600) / 60));
        }
        else {
            snprintf(time_text, sizeof(time_text), "Time left: %um %us",
                (unsigned)(remain_sec / 60),
                (unsigned)(remain_sec % 60));
        }
    }
    else {
        snprintf(time_text, sizeof(time_text), "Time left: --");
    }

    GUI_LabelSetText(self.speed_label, speed_text);
    GUI_LabelSetText(self.time_label, time_text);
    GUI_LabelSetText(self.bytes_label, bytes_text);
    self.last_ui_update = current_time;
}

static void refresh_cart_info(void) {
    alignas(32) naomi_cart_header_t hdr;
    char title[48];
    char publisher[40];
    char system[20];
    char serial[8];
    char buf[96];
    char size_txt[32];
    char region[80];
    char players[48];
    char video[48];
    char orient[48];
    static const char *region_names[] = {
        "Japan", "USA", "Export", "Korea", "Australia"
    };
    static const char *player_names[] = { "1P", "2P", "3P", "4P" };
    static const char *video_names[] = { "31kHz", "15kHz" };
    static const char *orient_names[] = { "Horizontal", "Vertical" };
    bool have_hdr;

    self.cart_ok = 0;
    self.can_decrypt_m2 = 0;
    self.cart_type = NAOMI_CART_NONE;
    self.dump_size = 0;
    self.dump_flags = NAOMI_CART_READ_DEFAULT;
    memset(&hdr, 0, sizeof(hdr));

    if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
        GUI_LabelSetText(self.info_title, "Not available");
        GUI_LabelSetText(self.info_publisher, "-");
        GUI_LabelSetText(self.info_serial, "-");
        GUI_LabelSetText(self.info_type, "Retail Dreamcast");
        GUI_LabelSetText(self.info_size, "-");
        GUI_LabelSetText(self.info_region, "-");
        GUI_LabelSetText(self.info_video, "-");
        GUI_LabelSetText(self.info_board, "-");
        GUI_LabelSetText(self.status_label, "This app requires NAOMI hardware");
        GUI_WidgetSetEnabled(self.start_btn, 0);
        GUI_WidgetSetEnabled(self.refresh_btn, 1);
        GUI_WidgetSetEnabled(self.browse_btn, 1);
        if(self.decrypt_chk) {
            GUI_WidgetSetEnabled(self.decrypt_chk, 0);
            GUI_WidgetSetState(self.decrypt_chk, 0);
        }
        return;
    }

    GUI_LabelSetText(self.status_label, "Reading cartridge header...");
    self.cart_type = naomi_cart_type();
    self.dump_flags = naomi_cart_detect_flags(self.cart_type);
    have_hdr = naomi_cart_read_header(&hdr);

    if(self.cart_type != NAOMI_CART_NONE) {
        GUI_LabelSetText(self.status_label, "Detecting cartridge size...");
        self.dump_size = naomi_cart_probe_size();
    }

    if(have_hdr) {
        pick_title(title, sizeof(title), &hdr);
        trim_pad(publisher, sizeof(publisher), hdr.publisher, 32);
        trim_pad(system, sizeof(system), hdr.system_name, 16);
        trim_pad(serial, sizeof(serial), hdr.serial, 4);
        sanitize_name(buf, sizeof(buf), title);
        GUI_TextEntrySetText(self.gname, buf);

        GUI_LabelSetText(self.info_title, title[0] ? title : "-");
        GUI_LabelSetText(self.info_publisher, publisher[0] ? publisher : "-");

        snprintf(buf, sizeof(buf), "%s  %04u-%02u-%02u",
            serial[0] ? serial : "----",
            (unsigned)hdr.year, (unsigned)hdr.month, (unsigned)hdr.day);
        GUI_LabelSetText(self.info_serial, buf);

        snprintf(buf, sizeof(buf), "%s / %s",
            system[0] ? system : "NAOMI",
            cart_type_name(self.cart_type));
        GUI_LabelSetText(self.info_type, buf);

        format_mask_list(region, sizeof(region), hdr.region, region_names, 5, "Any");
        format_mask_list(players, sizeof(players), hdr.players, player_names, 4, "Any");
        snprintf(buf, sizeof(buf), "%s / %s", region, players);
        GUI_LabelSetText(self.info_region, buf);

        format_mask_list(video, sizeof(video), hdr.vid_mode, video_names, 2, "Any");
        format_mask_list(orient, sizeof(orient), hdr.disp_orientation, orient_names, 2, "Any");
        snprintf(buf, sizeof(buf), "%s / %s", video, orient);
        GUI_LabelSetText(self.info_video, buf);

        if(self.cart_type == NAOMI_CART_M4) {
            snprintf(buf, sizeof(buf), "M4 ID 0x%04X  %s",
                (unsigned)naomi_cart_m4_id(),
                naomi_cart_header_encrypted(&hdr) ? "Encrypted" : "Plain");
        }
        else if(self.cart_type == NAOMI_CART_M1) {
            snprintf(buf, sizeof(buf), "M1 ID 0x%04X  %s",
                (unsigned)naomi_cart_m1_id(),
                naomi_cart_header_encrypted(&hdr) ? "Encrypted" : "Plain");
        }
        else if(self.cart_type == NAOMI_CART_M2 && m2_key_from_serial(hdr.serial)) {
            snprintf(buf, sizeof(buf), "M2  5881");
        }
        else {
            snprintf(buf, sizeof(buf), "%s",
                naomi_cart_header_encrypted(&hdr) ? "Encrypted header" : "Plain header");
        }
        GUI_LabelSetText(self.info_board, buf);
    }
    else {
        GUI_LabelSetText(self.info_title, "No valid header");
        GUI_LabelSetText(self.info_publisher, "-");
        GUI_LabelSetText(self.info_serial, "-");
        snprintf(buf, sizeof(buf), "- / %s", cart_type_name(self.cart_type));
        GUI_LabelSetText(self.info_type, buf);
        GUI_LabelSetText(self.info_region, "-");
        GUI_LabelSetText(self.info_video, "-");
        GUI_LabelSetText(self.info_board, "-");
        GUI_TextEntrySetText(self.gname, "naomi_cart");
    }

    if(self.dump_size) {
        format_size(size_txt, sizeof(size_txt), self.dump_size);
        GUI_LabelSetText(self.info_size, size_txt);
    }
    else {
        GUI_LabelSetText(self.info_size, "Unknown");
    }

    self.cart_ok = have_hdr && self.dump_size;
    if(self.cart_ok && !(self.dump_flags & NAOMI_CART_ADDR_8MB)) {
        ds_printf("DS_INFO: Dumping with 4MB ROM mapping\n");
    }
    self.can_decrypt_m2 = self.cart_ok && self.cart_type == NAOMI_CART_M2
        && have_hdr && m2_key_from_serial(hdr.serial);
    if(self.decrypt_chk) {
        GUI_WidgetSetEnabled(self.decrypt_chk, self.can_decrypt_m2);
        if(!self.can_decrypt_m2) {
            GUI_WidgetSetState(self.decrypt_chk, 0);
        }
    }
    if(!self.cart_ok) {
        GUI_LabelSetText(self.status_label, "Insert a NAOMI cartridge");
        GUI_WidgetSetEnabled(self.start_btn, 0);
        GUI_WidgetSetEnabled(self.refresh_btn, 1);
        GUI_WidgetSetEnabled(self.browse_btn, 1);
        return;
    }

    GUI_WidgetSetEnabled(self.start_btn, 1);
    GUI_WidgetSetEnabled(self.refresh_btn, 1);
    GUI_WidgetSetEnabled(self.browse_btn, 1);
    GUI_LabelSetText(self.status_label, "Ready");
}

static void start_detect(void) {
    if(self.detect_busy || self.rip_active) {
        return;
    }
    self.detect_busy = 1;
    GUI_LabelSetText(self.status_label, "Detecting cartridge...");
    GUI_WidgetSetEnabled(self.start_btn, 0);
    GUI_WidgetSetEnabled(self.refresh_btn, 0);
    GUI_WidgetSetEnabled(self.browse_btn, 0);
    if(!thd_create(1, cart_detect_thread, NULL)) {
        self.detect_busy = 0;
        GUI_WidgetSetEnabled(self.refresh_btn, 1);
        GUI_WidgetSetEnabled(self.browse_btn, 1);
    }
}

static void *cart_detect_thread(void *arg) {
    (void)arg;
    if(self.app && (self.app->state & APP_STATE_OPENED) && !self.rip_active) {
        refresh_cart_info();
    }
    self.detect_busy = 0;
    return NULL;
}

void CartRipperApp_Gamename(void) {
    char text[NAME_MAX];
    const char *input = GUI_TextEntryGetText(self.gname);

    sanitize_name(text, sizeof(text), input);
    GUI_TextEntrySetText(self.gname, text);
}

void CartRipperApp_Init(App_t *app) {
    const char *path;
    const char *slash;
    char games[NAME_MAX];

    memset(&self, 0, sizeof(self));
    self.app = app;

    self.pages = APP_GET_WIDGET("pages");
    self.gname = APP_GET_WIDGET("gname-text");
    self.pbar = APP_GET_WIDGET("progress-bar");
    self.start_btn = APP_GET_WIDGET("start_btn");
    self.cancel_btn = APP_GET_WIDGET("cancel_btn");
    self.refresh_btn = APP_GET_WIDGET("refresh_btn");
    self.launch_btn = APP_GET_WIDGET("launch_btn");
    self.browse_btn = APP_GET_WIDGET("browse_btn");
    self.speed_label = APP_GET_WIDGET("speed-label");
    self.time_label = APP_GET_WIDGET("time-label");
    self.bytes_label = APP_GET_WIDGET("bytes-label");
    self.status_label = APP_GET_WIDGET("status-label");
    self.destination_path = APP_GET_WIDGET("destination-path");
    self.file_browser = APP_GET_WIDGET("file-browser");
    self.info_title = APP_GET_WIDGET("info-title");
    self.info_publisher = APP_GET_WIDGET("info-publisher");
    self.info_serial = APP_GET_WIDGET("info-serial");
    self.info_type = APP_GET_WIDGET("info-type");
    self.info_size = APP_GET_WIDGET("info-size");
    self.info_region = APP_GET_WIDGET("info-region");
    self.info_video = APP_GET_WIDGET("info-video");
    self.info_board = APP_GET_WIDGET("info-board");
    self.decrypt_chk = APP_GET_WIDGET("decrypt-checkbox");

    path = getenv("PATH");
    slash = strchr(path + 1, '/');
    if(slash) {
        memcpy(self.selected_path, path, slash - path);
        self.selected_path[slash - path] = '\0';
    }
    else {
        strcpy(self.selected_path, path);
    }

    snprintf(games, sizeof(games), "%s/%s", self.selected_path, DEFAULT_GAMES_DIR);
    if(DirExists(games)) {
        strncpy(self.selected_path, games, NAME_MAX - 1);
        self.selected_path[NAME_MAX - 1] = '\0';
    }

    GUI_LabelSetText(self.destination_path, self.selected_path);
    GUI_WidgetSetEnabled(self.cancel_btn, 0);
    GUI_WidgetSetEnabled(self.start_btn, 0);
    GUI_WidgetSetEnabled(self.launch_btn, 0);
}

void CartRipperApp_Open(App_t *app) {
    (void)app;
    start_detect();
}

void CartRipperApp_Shutdown(App_t *app) {
    (void)app;
    if(self.rip_active) {
        self.rip_active = 0;
        if(self.app && self.app->thd) {
            thd_join(self.app->thd, NULL);
            self.app->thd = NULL;
        }
    }
}

void CartRipperApp_Refresh(GUI_Widget *widget) {
    (void)widget;
    start_detect();
}

void CartRipperApp_StartRip(GUI_Widget *widget) {
    (void)widget;

    if(self.detect_busy || !self.cart_ok) {
        return;
    }

    if(self.app->thd) {
        self.rip_active = 0;
        thd_join(self.app->thd, NULL);
        self.app->thd = NULL;
    }

    CartRipperApp_Gamename();
    self.dump_ready = 0;
    self.decrypt_m2 = self.can_decrypt_m2 && self.decrypt_chk
        && GUI_WidgetGetState(self.decrypt_chk);
    self.rip_active = 1;
    set_rip_controls(1);
    GUI_LabelSetText(self.status_label, "Starting...");
    GUI_LabelSetText(self.speed_label, "Preparing...");
    GUI_LabelSetText(self.time_label, "Please wait");
    GUI_ProgressBarSetPosition(self.pbar, 0.0);

    self.app->thd = thd_create(0, cart_ripper_thread, NULL);
    if(!self.app->thd) {
        self.rip_active = 0;
        reset_rip_state("Failed to start");
    }
}

void CartRipperApp_CancelRip(GUI_Widget *widget) {
    (void)widget;
    if(!self.rip_active) {
        return;
    }

    ds_printf("DS_PROCESS: Cancelling cartridge rip\n");
    self.rip_active = 0;

    if(self.app->thd) {
        thd_join(self.app->thd, NULL);
        self.app->thd = NULL;
        ds_printf("DS_INFO: Cartridge rip cancelled\n");
    }

    reset_rip_state("Cancelled");
}

void CartRipperApp_Launch(GUI_Widget *widget) {
    isoldr_info_t *isoldr;
    char *preset_file;
    uintptr_t exec_addr;
    (void)widget;

    if(self.rip_active || !self.dump_ready || !self.dst_file[0]) {
        return;
    }

    isoldr = isoldr_get_info(self.dst_file, 0);
    if(!isoldr) {
        ds_printf("DS_ERROR: Failed to get isoldr info for %s\n", self.dst_file);
        GUI_LabelSetText(self.status_label, "Failed to load dump");
        return;
    }

    preset_file = isoldr_find_preset(self.dst_file, NULL, 0);
    exec_addr = isoldr_apply_preset(isoldr, preset_file);
    if(exec_addr == (uintptr_t)-1) {
        free(isoldr);
        GUI_LabelSetText(self.status_label, "Failed to apply preset");
        return;
    }

    GUI_LabelSetText(self.status_label, "Launching...");
    isoldr_exec(isoldr, exec_addr);
    free(isoldr);
    GUI_LabelSetText(self.status_label, "Launch failed");
}

static uint32_t m2_key_from_serial(const char serial[4]) {
    if(serial[0] == 'B' && serial[1] == 'A' && serial[2] == 'L' && serial[3] == '1') {
        return NAOMI_M2_KEY_BAL1;
    }
    return 0;
}

static void dump_m2_wipe_overlay(file_t hnd) {
    uint8_t ff[M2_HOLE_CHECK];
    uint32_t i;

    for(i = 0; i < sizeof(ff); i++) {
        ff[i] = 0xff;
    }
    if(fs_seek(hnd, NAOMI_M2_OVERLAY_OFF, SEEK_SET) >= 0) {
        fs_write(hnd, ff, sizeof(ff));
    }
}

static int dump_m2_overlay(const char *path) {
    file_t hnd = FILEHND_INVALID;
    naomi_cart_header_t hdr;
    uint8_t *rom = NULL;
    uint8_t *stub = NULL;
    uint8_t *out = NULL;
    uint8_t hole[M2_HOLE_CHECK];
    uint32_t *tbl;
    uint32_t key;
    uint32_t stub_off;
    uint32_t stub_size;
    uint32_t i;
    uint32_t ram;
    int n;
    int k;
    int cnt;
    ssize_t got;
    char msg[64];

    hnd = fs_open(path, O_RDWR);
    if(hnd == FILEHND_INVALID) {
        ds_printf("DS_ERROR: Failed to reopen %s for M2 overlay\n", path);
        return 0;
    }
    if(fs_read(hnd, &hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr)) {
        ds_printf("DS_ERROR: Failed to read cart header\n");
        fs_close(hnd);
        return 0;
    }
    key = m2_key_from_serial(hdr.serial);
    if(!key) {
        ds_printf("DS_INFO: No M2 key for %.4s, leaving raw dump\n", hdr.serial);
        fs_close(hnd);
        return 1;
    }
    stub_off = 0;
    stub_size = 0;
    for(i = 0; i < 8; i++) {
        uint32_t dst = (uint32_t)hdr.game_exe[i].dst_buf & M2_ADDR_MASK;

        if(hdr.game_exe[i].offset == (uint32_t)-1) {
            break;
        }
        if(hdr.game_exe[i].size > M2_STUB_MIN_SIZE && dst >= M2_STUB_RAM_MIN) {
            stub_off = hdr.game_exe[i].offset;
            stub_size = hdr.game_exe[i].size;
            break;
        }
    }
    if(!stub_size || stub_off + stub_size > self.dump_size) {
        ds_printf("DS_ERROR: M2 stub not found in dump\n");
        fs_close(hnd);
        return 0;
    }
    if(fs_seek(hnd, NAOMI_M2_OVERLAY_OFF, SEEK_SET) < 0
        || fs_read(hnd, hole, sizeof(hole)) != (ssize_t)sizeof(hole)) {
        ds_printf("DS_ERROR: Failed to read overlay hole\n");
        fs_close(hnd);
        return 0;
    }
    for(i = 0; i < sizeof(hole); i++) {
        if(hole[i] != 0xff) {
            ds_printf("DS_INFO: Overlay hole is not empty, leaving dump\n");
            fs_close(hnd);
            return 1;
        }
    }
    stub = (uint8_t *)memalign(32, stub_size);
    if(!stub) {
        ds_printf("DS_ERROR: Out of memory for M2 stub\n");
        fs_close(hnd);
        return 0;
    }
    if(fs_seek(hnd, stub_off, SEEK_SET) < 0
        || fs_read(hnd, stub, stub_size) != (ssize_t)stub_size) {
        ds_printf("DS_ERROR: Failed to read M2 stub\n");
        free(stub);
        fs_close(hnd);
        return 0;
    }
    tbl = NULL;
    n = 0;
    ram = 0;
    for(i = 0; i + 32 <= stub_size; i += 4) {
        if(*(uint32_t *)(stub + i) != M2_WIN_FLAGS0
            || *(uint32_t *)(stub + i + 8) != M2_WIN_SIZE0) {
            continue;
        }
        cnt = 0;
        for(k = 0; k < M2_WIN_MAX && i + (uint32_t)(k * 16) + 12 <= stub_size; k++) {
            uint32_t sz = *(uint32_t *)(stub + i + k * 16 + 8);

            if(sz < M2_WIN_SIZE_MIN || sz > M2_WIN_SIZE_MAX) {
                break;
            }
            cnt++;
        }
        if(cnt >= 8) {
            tbl = (uint32_t *)(stub + i);
            n = cnt;
            break;
        }
    }
    if(!tbl) {
        ds_printf("DS_ERROR: M2 window table not found\n");
        free(stub);
        fs_close(hnd);
        return 0;
    }
    for(k = 0; k < n; k++) {
        ram += tbl[k * 4 + 2];
    }
    if(NAOMI_M2_OVERLAY_OFF + ram > self.dump_size) {
        ds_printf("DS_ERROR: M2 overlay does not fit in dump\n");
        free(stub);
        fs_close(hnd);
        return 0;
    }
    rom = (uint8_t *)memalign(32, NAOMI_M2_ROM_SIZE);
    out = (uint8_t *)memalign(32, M2_WIN_SIZE_MAX);
    if(!rom || !out) {
        ds_printf("DS_ERROR: Out of memory for M2 decrypt\n");
        free(out);
        free(rom);
        free(stub);
        fs_close(hnd);
        return 0;
    }
    GUI_LabelSetText(self.status_label, "Reading M2 ROM...");
    if(fs_seek(hnd, 0, SEEK_SET) < 0
        || fs_read(hnd, rom, NAOMI_M2_ROM_SIZE) != (ssize_t)NAOMI_M2_ROM_SIZE) {
        ds_printf("DS_ERROR: Failed to read M2 ROM\n");
        free(out);
        free(rom);
        free(stub);
        fs_close(hnd);
        return 0;
    }
    naomi_5881_set_src(rom, NAOMI_M2_ROM_SIZE);
    if(fs_seek(hnd, NAOMI_M2_OVERLAY_OFF, SEEK_SET) < 0) {
        ds_printf("DS_ERROR: Failed to seek overlay\n");
        free(out);
        free(rom);
        free(stub);
        fs_close(hnd);
        return 0;
    }
    for(k = 0; k < n; k++) {
        uint32_t rom_off = tbl[k * 4 + 1];
        uint32_t sz = tbl[k * 4 + 2];
        uint16_t sub = (uint16_t)tbl[k * 4 + 3];

        if(!self.rip_active) {
            dump_m2_wipe_overlay(hnd);
            free(out);
            free(rom);
            free(stub);
            fs_close(hnd);
            return 0;
        }
        if(sz > M2_WIN_SIZE_MAX) {
            ds_printf("DS_ERROR: M2 window %d too large\n", k);
            dump_m2_wipe_overlay(hnd);
            free(out);
            free(rom);
            free(stub);
            fs_close(hnd);
            return 0;
        }
        snprintf(msg, sizeof(msg), "Decrypting M2 %d/%d...", k + 1, n);
        GUI_LabelSetText(self.status_label, msg);
        GUI_ProgressBarSetPosition(self.pbar, (double)k / (double)n);
        naomi_5881_decrypt(out, sz, rom_off >> 1, sub, key);
        got = fs_write(hnd, out, sz);
        if(got != (ssize_t)sz) {
            ds_printf("DS_ERROR: Failed to write M2 overlay\n");
            dump_m2_wipe_overlay(hnd);
            free(out);
            free(rom);
            free(stub);
            fs_close(hnd);
            return 0;
        }
        ds_printf("DS_PROCESS: M2 window %d: %08lx %u bytes\n",
            k, (unsigned long)rom_off, (unsigned)sz);
    }
    free(out);
    free(rom);
    free(stub);
    fs_close(hnd);
    ds_printf("DS_OK: M2 overlay written at 0x%08X (%u bytes)\n",
        NAOMI_M2_OVERLAY_OFF, (unsigned)ram);
    return 1;
}

static void *cart_ripper_thread(void *arg) {
    uint8_t *buffer = NULL;
    file_t hnd = FILEHND_INVALID;
    char name[NAME_MAX];
    char dir[NAME_MAX];
    uint32_t offset;
    size_t remain;
    size_t chunk;
    size_t n;
    (void)arg;

    ds_printf("DS_PROCESS: Starting cartridge rip\n");
    self.start_time = timer_ms_gettime64();
    self.processed_bytes = 0;
    self.last_ui_update = 0;
    self.dst_file[0] = '\0';

    if(!self.dump_size) {
        ds_printf("DS_ERROR: Failed to detect cartridge size\n");
        GUI_LabelSetText(self.status_label, "Failed to detect size");
        goto out;
    }

    snprintf(name, sizeof(name), "%s", GUI_TextEntryGetText(self.gname));
    snprintf(dir, sizeof(dir), "%s/%s", self.selected_path, name);
    if(!DirExists(dir) && fs_mkdir(dir) < 0) {
        ds_printf("DS_ERROR: Failed to create %s\n", dir);
        GUI_LabelSetText(self.status_label, "Failed to create folder");
        goto out;
    }
    snprintf(self.dst_file, sizeof(self.dst_file), "%s/%s.dni", dir, name);
    ds_printf("DS_PROCESS: Dumping %u bytes to %s\n",
        (unsigned)self.dump_size, self.dst_file);

    buffer = (uint8_t *)memalign(32, RIP_BUF_SIZE);
    if(!buffer) {
        ds_printf("DS_ERROR: Failed to allocate rip buffer\n");
        GUI_LabelSetText(self.status_label, "Out of memory");
        goto out;
    }

    hnd = fs_open(self.dst_file, O_WRONLY | O_TRUNC | O_CREAT);
    if(hnd == FILEHND_INVALID) {
        ds_printf("DS_ERROR: Failed to create %s\n", self.dst_file);
        GUI_LabelSetText(self.status_label, "Failed to create file");
        free(buffer);
        goto out;
    }

    GUI_LabelSetText(self.status_label, "Ripping cartridge...");
    offset = 0;
    remain = self.dump_size;

    while(remain) {
        if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: Cartridge rip cancelled\n");
            GUI_LabelSetText(self.status_label, "Cancelled");
            free(buffer);
            fs_close(hnd);
            fs_unlink(self.dst_file);
            self.dst_file[0] = '\0';
            goto out;
        }

        chunk = remain;
        if(chunk > RIP_BUF_SIZE) {
            chunk = RIP_BUF_SIZE;
        }

        n = naomi_cart_read_ex(offset, buffer, chunk, self.dump_flags);
        if(n != chunk) {
            ds_printf("DS_ERROR: Cart read failed at 0x%08X (%u / %u)\n",
                (unsigned)offset, (unsigned)n, (unsigned)chunk);
            GUI_LabelSetText(self.status_label, "Read error");
            free(buffer);
            fs_close(hnd);
            fs_unlink(self.dst_file);
            self.dst_file[0] = '\0';
            goto out;
        }

        if(fs_write(hnd, buffer, chunk) != (ssize_t)chunk) {
            ds_printf("DS_ERROR: Write error to %s\n", self.dst_file);
            GUI_LabelSetText(self.status_label, "Write error");
            free(buffer);
            fs_close(hnd);
            fs_unlink(self.dst_file);
            self.dst_file[0] = '\0';
            goto out;
        }

        offset += chunk;
        remain -= chunk;
        self.processed_bytes += chunk;
        update_ui_display();
    }

    free(buffer);
    fs_close(hnd);
    ds_printf("DS_OK: Cartridge dumped to %s\n", self.dst_file);
    self.dump_ready = 1;
    self.last_ui_update = 0;
    self.processed_bytes = self.dump_size;
    update_ui_display();
    GUI_ProgressBarSetPosition(self.pbar, 1.0);
    if(self.decrypt_m2 && self.rip_active) {
        if(!dump_m2_overlay(self.dst_file)) {
            if(!self.rip_active) {
                GUI_LabelSetText(self.status_label, "Cancelled");
            }
            else {
                GUI_LabelSetText(self.status_label, "Dumped (decrypt failed)");
            }
        }
        else {
            GUI_ProgressBarSetPosition(self.pbar, 1.0);
            GUI_LabelSetText(self.status_label, "Done");
        }
    }
    else {
        GUI_LabelSetText(self.status_label, "Done");
    }

out:
    self.rip_active = 0;
    set_rip_controls(0);
    if(!self.cart_ok) {
        GUI_WidgetSetEnabled(self.start_btn, 0);
    }
    return NULL;
}

void CartRipperApp_ShowFileBrowser(GUI_Widget *widget) {
    (void)widget;
    if(self.rip_active || self.detect_busy) {
        return;
    }
    if(self.file_browser) {
        GUI_FileManagerSetPath(self.file_browser, self.selected_path);
    }
    if(self.pages) {
        GUI_CardStackShowIndex(self.pages, APP_PAGE_BROWSER);
    }
}

void CartRipperApp_ShowMainPage(GUI_Widget *widget) {
    (void)widget;
    if(self.pages) {
        GUI_CardStackShowIndex(self.pages, APP_PAGE_MAIN);
    }
}

void CartRipperApp_FileBrowserItemClick(dirent_fm_t *fm_ent) {
    if(!fm_ent) {
        return;
    }
    GUI_FileManagerChangeDir(self.file_browser, fm_ent->ent.name, fm_ent->ent.size);
}

void CartRipperApp_FileBrowserConfirm(GUI_Widget *widget) {
    const char *path;
    (void)widget;

    if(!self.file_browser || !self.destination_path || !self.pages) {
        return;
    }

    path = GUI_FileManagerGetPath(self.file_browser);
    if(path) {
        strncpy(self.selected_path, path, NAME_MAX - 1);
        self.selected_path[NAME_MAX - 1] = '\0';
        GUI_LabelSetText(self.destination_path, self.selected_path);
    }
    GUI_CardStackShowIndex(self.pages, APP_PAGE_MAIN);
}
