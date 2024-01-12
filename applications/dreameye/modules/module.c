/* DreamShell ##version##

   module.c - DreamEye app module
   Copyright (C) 2023, 2024 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>
#include "qr_code.h"

DEFAULT_MODULE_EXPORTS(app_dreameye);

static struct {

    App_t *app;
    dreameye_preview_t preview;

    GUI_Widget *pages;
    GUI_Widget *qr_data;

} self;

static void ShowCameraPreview() {

    int w, h;
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    if(!dev) {
        return;
    }
    dreameye_preview_shutdown(dev);
        
    if(self.preview.isp_mode == DREAMEYE_ISP_MODE_QSIF) {
        w = 160;
        h = 120;
    } else {
        w = 320;
        h = 240;
    }
    if(qr_scan_resize(w, h) < 0) {
        ds_printf("DS_ERROR: Couldn't allocate QR buffer\n");
    }
    dreameye_preview_init(dev, &self.preview);
}

static int execute_string(const char *str) {
    if (!strncmp(str, "dsc:", 4)) {
        if(dsystem_buff(str + 4) == CMD_OK) {
            return 0;
        }
    }
    else if (!strncmp(str, "lua:", 4)) {
        if (LuaDo(LUA_DO_STRING, str + 4, GetLuaState()) < 1) {
            return 0;
        }
    }
    return -1;
}

static void frame_callback(maple_device_t *dev, uint8_t *frame, size_t len) {
    (void)dev;

    /* Max payload is 8896, but Dreameye can't this I guess. */
    char qr_str[2048];

    if(!qr_scan_frame(self.preview.bpp, frame, len, qr_str)) {
        if(strcmp(GUI_LabelGetText(self.qr_data), qr_str)) {
            GUI_LabelSetText(self.qr_data, qr_str);
            execute_string(qr_str);
        }
    }
    else if(strncmp(GUI_LabelGetText(self.qr_data), "No QR Code", 10)) {
        GUI_LabelSetText(self.qr_data, "No QR Code");
    }
}

void DreamEyeApp_Init(App_t *app) {
    self.app = app;
    self.pages = APP_GET_WIDGET("pages");
    self.qr_data = APP_GET_WIDGET("qr-data");

    self.preview.isp_mode = DREAMEYE_ISP_MODE_QSIF;
    self.preview.bpp = 12;
    self.preview.fullscreen = 0;
    self.preview.scale = 2;
    self.preview.x = 160;
    self.preview.y = 235;
    self.preview.callback = frame_callback;

    qr_scan_init();
}

void DreamEyeApp_Shutdown(App_t *app) {
    (void)app;
    dreameye_preview_shutdown(NULL);
    qr_scan_shutdown();
}

void DreamEyeApp_Open(App_t *app) {
    (void)app;
    ShowCameraPreview();
}

void DreamEyeApp_ShowPage(GUI_Widget *widget) {
    (void)widget;
}

void DreamEyeApp_ShowPreview(GUI_Widget *widget) {
    const char *object_name = GUI_ObjectGetName((GUI_Object *)widget);

    if(object_name[8] == '1') {
        self.preview.isp_mode = DREAMEYE_ISP_MODE_QSIF;
        self.preview.scale = 2;
    } else {
        self.preview.isp_mode = DREAMEYE_ISP_MODE_SIF;
        self.preview.scale = 1;
    }

    ShowCameraPreview();
}
