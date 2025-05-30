/* DreamShell ##version##

   module.c - Dreameye app module
   Copyright (C) 2023, 2024 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>
#include <stdbool.h>
#include "qr_code.h"
#include "photo.h"

DEFAULT_MODULE_EXPORTS(app_dreameye);

typedef enum {
    APP_ACTION_IDLE,
    APP_ACTION_PHOTO_EXPORT,
    APP_ACTION_PHOTO_ERASE
} app_action_t;

static struct {

    App_t *app;
    maple_device_t *dev;
    dreameye_preview_t preview;
    bool preview_visible;
    bool qr_exec;
    bool qr_detected;
    size_t photo_count;
    app_action_t action;

    GUI_Widget *pages;
    GUI_Widget *filebrowser_page;
    GUI_Widget *progress_page;

    GUI_Widget *progress_bar;
    GUI_Widget *progress_text;
    GUI_Widget *progress_desc;

    GUI_Widget *qr_data;
    GUI_Widget *photo_count_text;
    GUI_Widget *file_browser;
    GUI_Widget *isp_mode[2];

} self;

#define NO_QR_CODE_TEXT "QR code is not detected"
#define ACTION_COMPLETE_TIMEOUT_MS 2000

static void HideCameraPreview(void) {
    dreameye_preview_shutdown(self.dev);
    qr_scan_shutdown();
    self.preview_visible = false;
}

static void ShowCameraPreview(void) {
    self.dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);
    if(!self.dev) {
        return;
    }
    if(self.preview_visible) {
        HideCameraPreview();
    }
    if(self.preview.callback) {
        qr_scan_init();
        qr_scan_resize(self.preview.isp_mode);
    }
    dreameye_preview_init(self.dev, &self.preview);
    self.preview_visible = true;
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

    /* Max payload is 8896, but Dreameye can max 400-500 at 320x240. */
    char qr_str[1024];
    char qr_label_str[sizeof(qr_str) + 8];
    char *qr_str_p = qr_str;
    size_t qr_len;
    int qr_count;

    memset(qr_str, 0, sizeof(qr_str));
    qr_count = qr_scan_frame(self.preview.bpp, frame, len, qr_str);

    if(qr_count > 0) {

        memset(qr_label_str, 0, sizeof(qr_label_str));
        self.qr_detected = true;

        while(qr_count--) {
            if(self.qr_exec) {
                execute_string(qr_str_p);
            }
            qr_len = strlen(qr_label_str);
            if(qr_len) {
                qr_label_str[qr_len] = '\n';
                qr_label_str[qr_len + 1] = '\0';
            }
            qr_len = strlen(qr_str_p);
            strncat(qr_label_str, qr_str_p, qr_len);
            qr_str_p += qr_len + 1;
        }

        if(strcmp(GUI_LabelGetText(self.qr_data), qr_label_str)) {
            GUI_LabelSetText(self.qr_data, qr_label_str);
        }
    }
    else {
        if(self.qr_detected == true) {
             GUI_LabelSetText(self.qr_data, NO_QR_CODE_TEXT);
        }
        self.qr_detected = false;
    }
}

static void UpdatePhotoCount(void) {
    char cnt[32];
    snprintf(cnt, sizeof(cnt), "%d photos", self.photo_count);
    GUI_LabelSetText(self.photo_count_text, cnt);
}

void DreameyeApp_Init(App_t *app) {
    self.app = app;
    self.action = APP_ACTION_IDLE;
    self.dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    self.pages = APP_GET_WIDGET("pages");
    self.filebrowser_page = APP_GET_WIDGET("file-browser-page");
    self.progress_page = APP_GET_WIDGET("progress-page");

    self.qr_data = APP_GET_WIDGET("qr-data");
    self.file_browser = APP_GET_WIDGET("file-browser");
    self.progress_bar = APP_GET_WIDGET("progress-bar");
    self.progress_text = APP_GET_WIDGET("progress-text");
    self.progress_desc = APP_GET_WIDGET("progress-desc");
    self.photo_count_text = APP_GET_WIDGET("photo-count");

    self.isp_mode[0] = APP_GET_WIDGET("isp-mode-qsif");
    self.isp_mode[1] = APP_GET_WIDGET("isp-mode-sif");

    self.preview.isp_mode = DREAMEYE_ISP_MODE_QSIF;
    self.preview.bpp = 12;
    self.preview.fullscreen = 0;
    self.preview.scale = 2;
    self.preview.x = 160;
    self.preview.y = 220;
    self.preview.callback = frame_callback;

    self.qr_exec = true;
    self.qr_detected = false;
    self.photo_count = get_photo_count(self.dev);
    UpdatePhotoCount();
}

void DreameyeApp_Shutdown(App_t *app) {
    (void)app;
    HideCameraPreview();
}

void DreameyeApp_Open(App_t *app) {
    (void)app;
    ShowCameraPreview();
}

void DreameyeApp_ShowMainPage(GUI_Widget *widget) {
    (void)widget;

    int index = GUI_CardStackGetIndex(self.pages);
    UpdatePhotoCount();
    GUI_CardStackShowIndex(self.pages, 0);
    self.action = APP_ACTION_IDLE;

    if(index > 1) {
        ShowCameraPreview();
    }
}

void DreameyeApp_ShowPhotoPage(GUI_Widget *widget) {
    (void)widget;

    int index = GUI_CardStackGetIndex(self.pages);
    UpdatePhotoCount();
    GUI_CardStackShowIndex(self.pages, 1);
    self.action = APP_ACTION_IDLE;

    if(index > 1) {
        ShowCameraPreview();
    }
}

void DreameyeApp_ExportPhoto(GUI_Widget *widget) {
    (void)widget;
    HideCameraPreview();
    GUI_LabelSetText(self.progress_desc, "Exporting photos...");
    GUI_CardStackShowIndex(self.pages, 2);
    self.action = APP_ACTION_PHOTO_EXPORT;
}

void DreameyeApp_ErasePhoto(GUI_Widget *widget) {
    (void)widget;
    HideCameraPreview();
    GUI_LabelSetText(self.progress_desc, "Erasing photos...");
    GUI_CardStackShowIndex(self.pages, 2);
    self.action = APP_ACTION_PHOTO_ERASE;
}

void DreameyeApp_ChangeResolution(GUI_Widget *widget) {

    GUI_WidgetSetState(widget, 1);

    if(self.isp_mode[0] == widget) {
        self.preview.isp_mode = DREAMEYE_ISP_MODE_QSIF;
        self.preview.scale = 2;
        GUI_WidgetSetState(self.isp_mode[1], 0);
    } else {
        self.preview.isp_mode = DREAMEYE_ISP_MODE_SIF;
        self.preview.scale = 1;
        GUI_WidgetSetState(self.isp_mode[0], 0);
    }

    ShowCameraPreview();
}

void DreameyeApp_ToggleDetectQR(GUI_Widget *widget) {

    HideCameraPreview();

    if(GUI_WidgetGetState(widget)) {
        GUI_LabelSetText(self.qr_data, NO_QR_CODE_TEXT);
        self.preview.callback = frame_callback;
    }
    else {
        self.preview.callback = NULL;
    }
    ShowCameraPreview();
}

void DreameyeApp_ToggleExecQR(GUI_Widget *widget) {
    self.qr_exec = GUI_WidgetGetState(widget);
}

void DreameyeApp_FileBrowserItemClick(dirent_fm_t *fm_ent) {
    if(!fm_ent) {
        return;
    }
    dirent_t *ent = &fm_ent->ent;
    GUI_FileManagerChangeDir(self.file_browser, ent->name, ent->size);
}

static void UpdateProgress(const char *desc, float progress) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%d%%", (int)(progress * 100));
    GUI_LabelSetText(self.progress_text, msg);
    GUI_LabelSetText(self.progress_desc, desc);
    GUI_ProgressBarSetPosition(self.progress_bar, progress);
}

static void *ExportPhotos(void *param) {
    int i;
    const char *dir = GUI_FileManagerGetPath(self.file_browser);
    char *desc = "Exporting completed.";
    (void)param;

    if(strlen(dir) < 2) {
        return NULL;
    }
    if(!self.photo_count) {
        desc = "Nothing to export.";
    }

    for(i = 0; i < self.photo_count; i++) {
        UpdateProgress("Exporting photos...", (1.0f / self.photo_count) * i);
        if(export_photo(self.dev, dir, i) < 0) {
            desc = "Exporting failed.";
            break;
        }
        if(self.action != APP_ACTION_PHOTO_EXPORT) {
            desc = "Exporting aborted.";
            break;
        }
    }

    UpdateProgress(desc, 1.0f);
    GUI_FileManagerScan(self.file_browser);
    thd_sleep(ACTION_COMPLETE_TIMEOUT_MS);
    DreameyeApp_ShowPhotoPage(NULL);
    return NULL;
}

static void *ErasePhotos(void *param) {
    int i;
    char *desc = "Erasing completed.";
    (void)param;

    if(!self.photo_count) {
        desc = "Nothing to erase.";
    }

	for(i = 0; i < self.photo_count; i++) {
		if(erase_photo(self.dev, i) < 0) {
			desc = "Erasing failed.";
			break;
        }
        if(self.action != APP_ACTION_PHOTO_ERASE) {
            desc = "Erasing aborted.";
            break;
        }
		
		do {
			if(self.action != APP_ACTION_PHOTO_ERASE) {
				desc = "Erasing aborted.";
				self.photo_count = get_photo_count(self.dev);
				goto exit_usr_abort;
			}
			thd_pass();
		} while (get_photo_count(self.dev) != (self.photo_count - i - 1));
		
		UpdateProgress("Erasing photos...", (1.0f / self.photo_count) * i);
    }

    UpdateProgress("Checking...", 0.99f);
    self.photo_count = get_photo_count(self.dev);

    if(self.photo_count) {
       desc = "Not all photos have been erased.";
    }

exit_usr_abort:
    UpdateProgress(desc, 1.0f);
    thd_sleep(ACTION_COMPLETE_TIMEOUT_MS);
    DreameyeApp_ShowPhotoPage(NULL);
    return NULL;
}

void DreameyeApp_FileBrowserConfirm(GUI_Widget *widget) {
    if(!self.dev) {
        return;
    }

    GUI_CardStackShowIndex(self.pages, 3);
    UpdateProgress("Retrieving photo count...", 0.0f);
    self.photo_count = get_photo_count(self.dev);

    switch(self.action) {
        case APP_ACTION_PHOTO_EXPORT:
            thd_create(1, ExportPhotos, NULL);
            break;
        case APP_ACTION_PHOTO_ERASE:
            thd_create(1, ErasePhotos, NULL);
            break;
        default:
            DreameyeApp_ShowPhotoPage(widget);
            break;
    }
}

void DreameyeApp_Abort(GUI_Widget *widget) {
    (void)widget;
    self.action = APP_ACTION_IDLE;
}
