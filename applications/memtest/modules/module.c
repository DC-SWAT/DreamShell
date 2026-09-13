/* DreamShell ##version##

   module.c - Memtest app module
   Copyright (C) 2026 SWAT
*/

#include <ds.h>
#include <dc/memory.h>
#include <dc/pvr.h>
#include <dc/spu.h>
#include <stdio.h>
#include "app_module.h"
#include "memtest_core.h"

DEFAULT_MODULE_EXPORTS(app_memtest);

#define RESULT_BODY_SIZE 2048

typedef struct {
    GUI_Widget *row;
    GUI_Widget *chk;
    GUI_Widget *name;
    GUI_Widget *size;
    GUI_Widget *addr;
    GUI_Widget *result;
} region_ui_t;

static struct {
    App_t *app;
    GUI_Widget *sys_label;
    GUI_Widget *pbar;
    GUI_Widget *status;
    GUI_Widget *start_btn;
    GUI_Widget *stop_btn;
    GUI_Widget *quick_chk;
    GUI_Widget *led_chk;
    GUI_Widget *dialog;
    region_ui_t rows[MEMTEST_REGIONS_MAX];
    memtest_plan_t plan;
    char result_body[RESULT_BODY_SIZE];
    volatile int testing;
} self;

static const char *sub_tag(int id) {
    switch(id) {
        case MEMTEST_SUB_DATABUS:
            return "Data";
        case MEMTEST_SUB_ADDRBUS:
            return "Addr";
        case MEMTEST_SUB_DEVICE:
            return "Dev";
        default:
            return "Test";
    }
}

static const char *status_text(int status) {
    switch(status) {
        case MEMTEST_ST_PASS:
            return "PASS";
        case MEMTEST_ST_FAIL:
            return "FAIL";
        case MEMTEST_ST_SKIPPED:
            return "Skip";
        case MEMTEST_ST_RUNNING:
            return "Running";
        default:
            return "--";
    }
}

static void format_size(size_t bytes, char *buf, size_t len) {
    if(bytes >= 1024 * 1024) {
        snprintf(buf, len, "%u MB", (unsigned)(bytes / (1024 * 1024)));
        return;
    }
    if(bytes >= 1024) {
        snprintf(buf, len, "%u KB", (unsigned)(bytes / 1024));
        return;
    }
    snprintf(buf, len, "%u B", (unsigned)bytes);
}

static void format_addr(uintptr_t addr, char *buf, size_t len) {
    snprintf(buf, len, "%08lX", (unsigned long)(addr & MEM_AREA_CACHE_MASK));
}

static void set_controls(int running) {
    int i;

    GUI_WidgetSetEnabled(self.start_btn, !running);
    GUI_WidgetSetEnabled(self.stop_btn, running);
    if(self.quick_chk) {
        GUI_WidgetSetEnabled(self.quick_chk, !running);
    }
    if(self.led_chk) {
        GUI_WidgetSetEnabled(self.led_chk, !running);
    }
    for(i = 0; i < self.plan.region_count; i++) {
        if(self.rows[i].chk) {
            GUI_WidgetSetEnabled(self.rows[i].chk, !running);
        }
    }
}

static void refresh_row(int index) {
    memtest_region_t *reg;
    char buf[64];

    if(index < 0 || index >= self.plan.region_count) {
        return;
    }

    reg = &self.plan.regions[index];
    if(!self.rows[index].result) {
        return;
    }

    if(reg->status == MEMTEST_ST_FAIL) {
        snprintf(buf, sizeof(buf), "FAIL @%08lX",
            (unsigned long)(reg->fail_addr & MEM_AREA_CACHE_MASK));
        GUI_LabelSetText(self.rows[index].result, buf);
        return;
    }
    if(reg->status == MEMTEST_ST_PASS) {
        snprintf(buf, sizeof(buf), "PASS  %u ms", (unsigned)reg->msec);
        GUI_LabelSetText(self.rows[index].result, buf);
        return;
    }
    GUI_LabelSetText(self.rows[index].result, status_text(reg->status));
}

static void format_mb(size_t bytes, char *buf, size_t len) {
    snprintf(buf, len, "%u", (unsigned)(bytes / (1024 * 1024)));
}

static void update_sys_label(void) {
    char buf[160];
    char ram[8];
    char vram[8];
    char aica[8];

    format_mb(self.plan.ram_size, ram, sizeof(ram));
    format_mb(self.plan.aica_size, aica, sizeof(aica));
    format_mb(self.plan.vram_size, vram, sizeof(vram));

    if(self.plan.vram_b_size) {
        char elan[8];
        char vram_b[8];
        format_mb(self.plan.elan_size, elan, sizeof(elan));
        format_mb(self.plan.vram_b_size, vram_b, sizeof(vram_b));
        snprintf(buf, sizeof(buf), "Memtest  %s  RAM %sMB  VRAM %s+%sMB  AICA %sMB  Elan %sMB",
            self.plan.hw_name, ram, vram, vram_b, aica, elan);
    }
    else {
        snprintf(buf, sizeof(buf), "Memtest  %s  RAM %sMB  VRAM %sMB  AICA %sMB",
            self.plan.hw_name, ram, vram, aica);
    }

    if(self.sys_label) {
        GUI_LabelSetText(self.sys_label, buf);
        GUI_WidgetSetAlign(self.sys_label, WIDGET_HORIZ_CENTER | WIDGET_VERT_CENTER);
    }
}

static void bind_rows(void) {
    int i;
    char name[32];
    char buf[32];

    for(i = 0; i < MEMTEST_REGIONS_MAX; i++) {
        snprintf(name, sizeof(name), "row-%d", i);
        self.rows[i].row = APP_GET_WIDGET(name);
        snprintf(name, sizeof(name), "chk-%d", i);
        self.rows[i].chk = APP_GET_WIDGET(name);
        snprintf(name, sizeof(name), "name-%d", i);
        self.rows[i].name = APP_GET_WIDGET(name);
        snprintf(name, sizeof(name), "size-%d", i);
        self.rows[i].size = APP_GET_WIDGET(name);
        snprintf(name, sizeof(name), "addr-%d", i);
        self.rows[i].addr = APP_GET_WIDGET(name);
        snprintf(name, sizeof(name), "result-%d", i);
        self.rows[i].result = APP_GET_WIDGET(name);

        if(i >= self.plan.region_count) {
            if(self.rows[i].row) {
                GUI_WidgetSetFlags(self.rows[i].row, WIDGET_HIDDEN);
            }
            continue;
        }

        if(self.rows[i].row) {
            GUI_WidgetClearFlags(self.rows[i].row, WIDGET_HIDDEN);
        }
        if(self.rows[i].name) {
            GUI_LabelSetText(self.rows[i].name, self.plan.regions[i].name);
        }
        format_size(self.plan.regions[i].size, buf, sizeof(buf));
        if(self.rows[i].size) {
            GUI_LabelSetText(self.rows[i].size, buf);
        }
        format_addr(self.plan.regions[i].base, buf, sizeof(buf));
        if(self.rows[i].addr) {
            GUI_LabelSetText(self.rows[i].addr, buf);
        }
        if(self.rows[i].chk) {
            GUI_WidgetSetState(self.rows[i].chk, self.plan.regions[i].enabled);
        }
        if(self.rows[i].result) {
            GUI_LabelSetText(self.rows[i].result, "--");
        }
    }
}

static void read_options(void) {
    int i;

    self.plan.quick = self.quick_chk && GUI_WidgetGetState(self.quick_chk);
    self.plan.cs_led = self.led_chk && GUI_WidgetGetState(self.led_chk);
    for(i = 0; i < self.plan.region_count; i++) {
        if(self.rows[i].chk) {
            self.plan.regions[i].enabled = GUI_WidgetGetState(self.rows[i].chk);
        }
    }
}

static void append_body(const char *line) {
    size_t used = strlen(self.result_body);

    if(used >= sizeof(self.result_body) - 1) {
        return;
    }
    strncat(self.result_body, line, sizeof(self.result_body) - used - 1);
}

static void append_sub(char *line, size_t len, const memtest_sub_t *sub, int id) {
    char piece[128];
    size_t used;

    if(sub->status == MEMTEST_ST_IDLE) {
        return;
    }
    if(sub->status == MEMTEST_ST_FAIL) {
        snprintf(piece, sizeof(piece),
            "%s%s [color=red]FAIL[/color] @%08lX exp %08lX got %08lX",
            line[0] ? "  " : "",
            sub_tag(id),
            (unsigned long)(sub->fail_addr & MEM_AREA_CACHE_MASK),
            (unsigned long)sub->expected,
            (unsigned long)sub->actual);
    }
    else {
        snprintf(piece, sizeof(piece),
            "%s%s [color=green]%s[/color]",
            line[0] ? "  " : "",
            sub_tag(id), status_text(sub->status));
    }
    used = strlen(line);
    if(used >= len - 1) {
        return;
    }
    strncat(line, piece, len - used - 1);
}

static void build_results(void) {
    int i;
    int s;
    int max_sub;
    char title[128];
    char subs[384];
    memtest_region_t *reg;
    const char *color;

    self.result_body[0] = '\0';
    max_sub = self.plan.quick ? MEMTEST_SUB_ADDRBUS : (MEMTEST_SUBTESTS - 1);

    for(i = 0; i < self.plan.region_count; i++) {
        reg = &self.plan.regions[i];
        if(!reg->enabled) {
            continue;
        }
        color = (reg->status == MEMTEST_ST_FAIL) ? "red" :
            (reg->status == MEMTEST_ST_PASS) ? "green" : "blue";
        if(reg->status == MEMTEST_ST_FAIL && reg->fail_addr) {
            snprintf(title, sizeof(title),
                "[size=18][b]%s[/b][/size]  [color=%s][b]%s[/b][/color]  @%08lX  %u ms\n",
                reg->name, color, status_text(reg->status),
                (unsigned long)(reg->fail_addr & MEM_AREA_CACHE_MASK),
                (unsigned)reg->msec);
        }
        else {
            snprintf(title, sizeof(title),
                "[size=18][b]%s[/b][/size]  [color=%s][b]%s[/b][/color]  %u ms\n",
                reg->name, color, status_text(reg->status), (unsigned)reg->msec);
        }
        append_body(title);

        subs[0] = '\0';
        for(s = 0; s <= max_sub; s++) {
            append_sub(subs, sizeof(subs), &reg->sub[s], s);
        }
        if(subs[0]) {
            append_body(subs);
            append_body("\n");
        }
        append_body("\n");
    }
}

static void show_dialog(GUI_DialogMode mode, const char *title, const char *body) {
    if(!self.dialog) {
        return;
    }
    GUI_DialogShow(self.dialog, mode, title, body);
}

static void hide_dialog(void) {
    if(self.dialog) {
        GUI_DialogHide(self.dialog);
    }
}

static void *memtest_thread(void *arg) {
    int i;
    char status[96];
    memtest_region_t *reg;

    (void)arg;
    self.testing = 1;
    self.plan.cancel = 0;
    self.result_body[0] = '\0';

    for(i = 0; i < self.plan.region_count; i++) {
        reg = &self.plan.regions[i];
        if(!reg->enabled || self.plan.cancel) {
            reg->status = MEMTEST_ST_SKIPPED;
            refresh_row(i);
            continue;
        }

        if(reg->type == MEMTEST_REGION_RAM) {
            snprintf(status, sizeof(status), "Testing %s (screen may freeze)...", reg->name);
        }
        else if(reg->type == MEMTEST_REGION_VRAM) {
            snprintf(status, sizeof(status),
                "Testing %s (brief screen glitches possible)...", reg->name);
        }
        else {
            snprintf(status, sizeof(status), "Testing %s...", reg->name);
        }
        GUI_LabelSetText(self.status, status);
        GUI_ProgressBarSetPosition(self.pbar, (float)i / (float)self.plan.region_count);
        ds_printf("DS_PROCESS: Memtest %s...\n", reg->name);
        thd_sleep(80);

        if(reg->type == MEMTEST_REGION_RAM || reg->type == MEMTEST_REGION_VRAM) {
            ShutdownVideoThread();
            pvr_wait_ready();
            pvr_wait_render_done();
        }
        if(reg->type == MEMTEST_REGION_AICA) {
            spu_disable();
        }

        memtest_run_region(&self.plan, i);

        if(reg->type == MEMTEST_REGION_AICA) {
            spu_enable();
        }
        if(reg->type == MEMTEST_REGION_RAM || reg->type == MEMTEST_REGION_VRAM) {
            InitVideoThread();
        }

        ds_printf("DS_OK: Memtest %s: %s (%u ms)\n",
            reg->name, status_text(reg->status), (unsigned)reg->msec);
        refresh_row(i);
        GUI_ProgressBarSetPosition(self.pbar,
            (float)(i + 1) / (float)self.plan.region_count);
    }

    GUI_ProgressBarSetPosition(self.pbar, 1.0);
    if(self.plan.cancel) {
        GUI_LabelSetText(self.status, "Stopped");
    }
    else {
        GUI_LabelSetText(self.status, "Done");
    }

    build_results();
    show_dialog(DIALOG_MODE_ALERT, "Memtest results", self.result_body);

    self.testing = 0;
    set_controls(0);
    if(self.app) {
        self.app->thd = NULL;
    }
    return NULL;
}

void MemtestApp_Start(GUI_Widget *widget) {
    int i;
    int any;

    (void)widget;

    if(self.testing) {
        return;
    }

    memtest_plan_init(&self.plan);
    read_options();
    bind_rows();
    update_sys_label();

    any = 0;
    for(i = 0; i < self.plan.region_count; i++) {
        if(self.plan.regions[i].enabled) {
            any = 1;
            break;
        }
    }
    if(!any) {
        GUI_LabelSetText(self.status, "Select at least one region");
        return;
    }
    GUI_ProgressBarSetPosition(self.pbar, 0.0);
    GUI_LabelSetText(self.status, "Starting...");
    set_controls(1);

    self.app->thd = thd_create(0, memtest_thread, NULL);
}

void MemtestApp_Stop(GUI_Widget *widget) {
    (void)widget;
    self.plan.cancel = 1;
    GUI_LabelSetText(self.status, "Stopping...");
}

void MemtestApp_DialogConfirm(GUI_Widget *widget) {
    (void)widget;
    hide_dialog();
}

void MemtestApp_Init(App_t *app) {
    memset(&self, 0, sizeof(self));

    if(!app) {
        ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n",
            lib_get_name(), __func__);
        return;
    }

    self.app = app;
    self.sys_label = APP_GET_WIDGET("sys-label");
    self.pbar = APP_GET_WIDGET("progress-bar");
    self.status = APP_GET_WIDGET("status-label");
    self.start_btn = APP_GET_WIDGET("start_btn");
    self.stop_btn = APP_GET_WIDGET("stop_btn");
    self.quick_chk = APP_GET_WIDGET("quick-checkbox");
    self.led_chk = APP_GET_WIDGET("led-checkbox");
    self.dialog = APP_GET_WIDGET("results-dialog");

    memtest_plan_init(&self.plan);
    bind_rows();
    update_sys_label();
    set_controls(0);
    GUI_ProgressBarSetPosition(self.pbar, 0.0);
    GUI_LabelSetText(self.status, "Ready");
}

void MemtestApp_Open(App_t *app) {
    if(app) {
        self.app = app;
    }
    if(!self.testing) {
        memtest_plan_init(&self.plan);
        bind_rows();
        update_sys_label();
        GUI_LabelSetText(self.status, "Ready");
    }
}

void MemtestApp_Shutdown(App_t *app) {
    (void)app;
    self.plan.cancel = 1;
    if(self.app && self.app->thd) {
        thd_join(self.app->thd, NULL);
        self.app->thd = NULL;
    }
    self.testing = 0;
}
