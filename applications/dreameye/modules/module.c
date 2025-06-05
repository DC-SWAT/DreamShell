/* DreamShell ##version##

   module.c - Dreameye app module
   Copyright (C) 2023-2025 SWAT 
*/

#include <ds.h>
#include <sfx.h>
#include <kos.h>
#include <dc/maple.h>
#include <drivers/dreameye.h>
#include <stdbool.h>
#include "qr_code.h"
#include "photo.h"
#include "gallery.h"
#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_dreameye);

typedef enum {
    APP_ACTION_IDLE = 0,
    APP_ACTION_PHOTO_EXPORT,
    APP_ACTION_PHOTO_EXPORT_SINGLE,
    APP_ACTION_PHOTO_ERASE,
    APP_ACTION_PHOTO_ERASE_SINGLE
} app_action_t;

typedef enum {
    APP_PAGE_MAIN = 0,
    APP_PAGE_PHOTO,
    APP_PAGE_GALLERY,
    APP_PAGE_PHOTO_VIEWER,
    APP_PAGE_FULLSCREEN_VIEWER,
    APP_PAGE_FILE_BROWSER,
    APP_PAGE_PROGRESS,
    APP_PAGE_CONFIRM_DELETE,
} app_page_t;

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
    GUI_Widget *gallery_page;
    GUI_Widget *photo_viewer_page;

    GUI_Widget *progress_bar;
    GUI_Widget *progress_text;
    GUI_Widget *progress_desc;

    GUI_Widget *qr_data;
    GUI_Widget *photo_count_main_page;
    GUI_Widget *photo_count_photo_page;
    GUI_Widget *file_browser;
    GUI_Widget *isp_mode[2];
    GUI_Widget *bpp_mode[2];

    GUI_Widget *viewer_status;
    GUI_Widget *viewer_photo_info;
    GUI_Widget *gallery_page_info;
    GUI_Widget *fullscreen_status;
    GUI_Widget *fullscreen_photo;
    GUI_Widget *confirm_delete_text;
    GUI_Widget *header_panel;

    GUI_Widget *thumb_widgets[GALLERY_THUMBS_PER_PAGE];
    GUI_Widget *thumb_labels[GALLERY_THUMBS_PER_PAGE];
    GUI_Widget *thumb_buttons[GALLERY_THUMBS_PER_PAGE];
    GUI_Widget *photo_viewer_button;

    GUI_Surface *default_thumb_surface;
    GUI_Surface *default_thumb_hl_surface;

    Event_t *slide_input_event;

} self;

#define NO_QR_CODE_TEXT "QR code is not detected"
#define ACTION_COMPLETE_TIMEOUT_MS 1500

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

static void UpdatePhotoCountLabels(void) {
    char cnt[32];
    snprintf(cnt, sizeof(cnt), "%d photos", self.photo_count);
    GUI_LabelSetText(self.photo_count_main_page, cnt);
    GUI_LabelSetText(self.photo_count_photo_page, cnt);
}

static void UpdateProgress(const char *desc, float progress) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%d%%", (int)(progress * 100));
    GUI_LabelSetText(self.progress_text, msg);
    GUI_LabelSetText(self.progress_desc, desc);
    GUI_ProgressBarSetPosition(self.progress_bar, progress);
}

static void on_photo_loaded(GUI_Surface *surface, GUI_Surface *hl_surface) {

    LockVideo();
    GUI_LabelSetText(self.viewer_status, " ");
    GUI_WidgetSetEnabled(self.photo_viewer_button, 1);

    if (surface) {
        GUI_ButtonSetNormalImage(self.photo_viewer_button, surface);
        GUI_ObjectDecRef((GUI_Object*)surface);
        
        if (hl_surface) {
            GUI_ButtonSetHighlightImage(self.photo_viewer_button, hl_surface);

            if (hl_surface != surface) {
                GUI_ObjectDecRef((GUI_Object*)hl_surface);
            }
        }
    }
    UnlockVideo();
}

static void LoadPhotoIntoViewer(int photo_index) {
    gallery_state_t *state = gallery_get_state();
    state->current_photo = photo_index;

    GUI_LabelSetText(self.viewer_status, "Loading...");
    GUI_WidgetSetEnabled(self.photo_viewer_button, 0);
    
    char photo_info[32];
    snprintf(photo_info, sizeof(photo_info), "Photo %d/%d", 
            photo_index + 1, (int)self.photo_count);
    GUI_LabelSetText(self.viewer_photo_info, photo_info);

    gallery_load_photo(photo_index, 380, 280, on_photo_loaded);
}

static void DreameyeApp_FullscreenPrevPhoto(void);
static void DreameyeApp_FullscreenNextPhoto(void);

static void Slide_EventHandler(void *ds_event, void *param, int action) {
    SDL_Event *event = (SDL_Event *) param;
    switch(event->type) {
        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case SDL_DC_L:
                    if (IsVirtKeyboardVisible()) {
                        break;
                    }
                    switch(GUI_CardStackGetIndex(self.pages)) {
                        case APP_PAGE_GALLERY:
                            DreameyeApp_GalleryPrevPage(NULL);
                            break;
                        case APP_PAGE_PHOTO_VIEWER:
                            DreameyeApp_ViewPrevPhoto(NULL);
                            break;
                        case APP_PAGE_FULLSCREEN_VIEWER:
                            DreameyeApp_FullscreenPrevPhoto();
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_DC_R:
                    if (IsVirtKeyboardVisible()) {
                        break;
                    }
                    switch(GUI_CardStackGetIndex(self.pages)) {
                        case APP_PAGE_GALLERY:
                            DreameyeApp_GalleryNextPage(NULL);
                            break;
                        case APP_PAGE_PHOTO_VIEWER:
                            DreameyeApp_ViewNextPhoto(NULL);
                            break;
                        case APP_PAGE_FULLSCREEN_VIEWER:
                            DreameyeApp_FullscreenNextPhoto();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case SDL_KEYDOWN:
            switch (event->key.keysym.sym) {
                case SDLK_COMMA:
                    switch(GUI_CardStackGetIndex(self.pages)) {
                        case APP_PAGE_GALLERY:
                            DreameyeApp_GalleryPrevPage(NULL);
                            break;
                        case APP_PAGE_PHOTO_VIEWER:
                            DreameyeApp_ViewPrevPhoto(NULL);
                            break;
                        case APP_PAGE_FULLSCREEN_VIEWER:
                            DreameyeApp_FullscreenPrevPhoto();
                            break;
                        default:
                            break;
                    }
                    break;
                case SDLK_PERIOD:
                    switch(GUI_CardStackGetIndex(self.pages)) {
                        case APP_PAGE_GALLERY:
                            DreameyeApp_GalleryNextPage(NULL);
                            break;
                        case APP_PAGE_PHOTO_VIEWER:
                            DreameyeApp_ViewNextPhoto(NULL);
                            break;
                        case APP_PAGE_FULLSCREEN_VIEWER:
                            DreameyeApp_FullscreenNextPhoto();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void DreameyeApp_Init(App_t *app) {
    self.app = app;
    self.action = APP_ACTION_IDLE;
    self.dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    self.pages = APP_GET_WIDGET("pages");
    self.filebrowser_page = APP_GET_WIDGET("file-browser-page");
    self.progress_page = APP_GET_WIDGET("progress-page");
    self.gallery_page = APP_GET_WIDGET("gallery-page");
    self.photo_viewer_page = APP_GET_WIDGET("photo-viewer-page");

    self.qr_data = APP_GET_WIDGET("qr-data");
    self.file_browser = APP_GET_WIDGET("file-browser");
    self.progress_bar = APP_GET_WIDGET("progress-bar");
    self.progress_text = APP_GET_WIDGET("progress-text");
    self.progress_desc = APP_GET_WIDGET("progress-desc");
    self.photo_count_main_page = APP_GET_WIDGET("photo-count-main-page");
    self.photo_count_photo_page = APP_GET_WIDGET("photo-count-photo-page");

    self.isp_mode[0] = APP_GET_WIDGET("isp-mode-qsif");
    self.isp_mode[1] = APP_GET_WIDGET("isp-mode-sif");
    
    self.bpp_mode[0] = APP_GET_WIDGET("bpp-12");
    self.bpp_mode[1] = APP_GET_WIDGET("bpp-16");

    self.viewer_status = APP_GET_WIDGET("viewer-status");
    self.viewer_photo_info = APP_GET_WIDGET("viewer-photo-info");
    self.gallery_page_info = APP_GET_WIDGET("gallery-page-info");
    self.fullscreen_status = APP_GET_WIDGET("fullscreen-status");
    self.fullscreen_photo = APP_GET_WIDGET("fullscreen-photo-button");
    self.confirm_delete_text = APP_GET_WIDGET("confirm-delete-text");
    self.header_panel = APP_GET_WIDGET("header-panel");

    for (int i = 0; i < GALLERY_THUMBS_PER_PAGE; i++) {
        char widget_name[32];
        char label_name[32];
        char button_name[32];

        snprintf(widget_name, sizeof(widget_name), "thumb-%d", i);
        snprintf(label_name, sizeof(label_name), "thumb-label-%d", i);
        snprintf(button_name, sizeof(button_name), "thumb-button-%d", i);

        self.thumb_widgets[i] = APP_GET_WIDGET(widget_name);
        self.thumb_labels[i] = APP_GET_WIDGET(label_name);
        self.thumb_buttons[i] = APP_GET_WIDGET(button_name);
    }

    self.photo_viewer_button = APP_GET_WIDGET("photo-viewer-button");

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
    UpdatePhotoCountLabels();

    gallery_init(self.dev);

    self.default_thumb_surface = GUI_ButtonGetNormalImage(self.thumb_buttons[0]);
    GUI_ObjectIncRef((GUI_Object *)self.default_thumb_surface);

    self.default_thumb_hl_surface = GUI_ButtonGetHighlightImage(self.thumb_buttons[0]);
    GUI_ObjectIncRef((GUI_Object *)self.default_thumb_hl_surface);

    self.slide_input_event = AddEvent(
        "Slide_Input",
        EVENT_TYPE_INPUT,
        EVENT_PRIO_DEFAULT,
        Slide_EventHandler,
        NULL
    );
}

void DreameyeApp_Shutdown(App_t *app) {
    (void)app;
    HideCameraPreview();
    gallery_shutdown();

    GUI_ObjectDecRef((GUI_Object *)self.default_thumb_surface);
    GUI_ObjectDecRef((GUI_Object *)self.default_thumb_hl_surface);

    RemoveEvent(self.slide_input_event);
}

void DreameyeApp_Open(App_t *app) {
    (void)app;
    ShowCameraPreview();
}

void DreameyeApp_ShowMainPage(GUI_Widget *widget) {
    (void)widget;

    gallery_load_abort();

    int index = GUI_CardStackGetIndex(self.pages);
    GUI_CardStackShowIndex(self.pages, APP_PAGE_MAIN);
    self.action = APP_ACTION_IDLE;

    if(index > APP_PAGE_PHOTO) {
        ShowCameraPreview();
    }
}

void DreameyeApp_ShowPhotoPage(GUI_Widget *widget) {
    (void)widget;

    gallery_load_abort();

    int index = GUI_CardStackGetIndex(self.pages);
    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO);
    self.action = APP_ACTION_IDLE;

    if(index > APP_PAGE_PHOTO) {
        ShowCameraPreview();
    }
}

void DreameyeApp_ExportPhoto(GUI_Widget *widget) {
    (void)widget;
    HideCameraPreview();
    GUI_LabelSetText(self.progress_desc, "Exporting photos...");
    GUI_CardStackShowIndex(self.pages, APP_PAGE_FILE_BROWSER);
    self.action = APP_ACTION_PHOTO_EXPORT;
}

void DreameyeApp_ErasePhoto(GUI_Widget *widget) {
    (void)widget;
    HideCameraPreview();
    GUI_LabelSetText(self.confirm_delete_text, "Delete all photos?");
    GUI_CardStackShowIndex(self.pages, APP_PAGE_CONFIRM_DELETE);
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

void DreameyeApp_ChangeBpp(GUI_Widget *widget) {

    GUI_WidgetSetState(widget, 1);

    if(self.bpp_mode[0] == widget) {
        self.preview.bpp = 12;
        GUI_WidgetSetState(self.bpp_mode[1], 0);
    } else {
        self.preview.bpp = 16;
        GUI_WidgetSetState(self.bpp_mode[0], 0);
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

static void *ExportSinglePhoto(void *param) {
    gallery_state_t *state = gallery_get_state();
    const char *dir = GUI_FileManagerGetPath(self.file_browser);
    char *desc = "Photo exported.";
    (void)param;

    if(strlen(dir) < 2 || state->current_photo < 0) {
        desc = "Export failed.";
    }
    else {
        UpdateProgress("Exporting photo...", 0.5f);
        if(export_photo(self.dev, dir, state->current_photo) < 0) {
            desc = "Export failed.";
        }
    }

    UpdateProgress(desc, 1.0f);
    GUI_FileManagerScan(self.file_browser);
    thd_sleep(ACTION_COMPLETE_TIMEOUT_MS);

    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    return NULL;
}

static void *EraseSinglePhoto(void *param) {
    gallery_state_t *state = gallery_get_state();
    char *desc = "Photo deleted.";
    (void)param;

    if (state->current_photo < 0) {
        desc = "Delete failed.";
    }
    else {
        if(erase_photo(self.dev, state->current_photo) < 0) {
            desc = "Delete failed.";
        }
        else {
            self.photo_count = get_photo_count(self.dev);
            gallery_clear_cache();
            UpdatePhotoCountLabels();
        }
    }

    UpdateProgress(desc, 1.0f);
    thd_sleep(ACTION_COMPLETE_TIMEOUT_MS);

    if (self.photo_count == 0) {
        DreameyeApp_ShowPhotoPage(NULL);
    }
    else {
        if (state->current_photo >= (int)self.photo_count) {
            state->current_photo = self.photo_count - 1;
        }
        GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
        LoadPhotoIntoViewer(state->current_photo);
    }
    return NULL;
}

static void *ErasePhotos(void *param) {
    char *desc = "Erasing completed.";
    (void)param;

    if(!self.photo_count) {
        desc = "Nothing to erase.";
    }

    for(int i = 0; i < self.photo_count; i++) {
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

    UpdateProgress("Checking...", 1.0f);
    self.photo_count = get_photo_count(self.dev);

    if(self.photo_count) {
       desc = "Not all photos have been erased.";
    }

exit_usr_abort:

    gallery_clear_cache();
    UpdatePhotoCountLabels();
    UpdateProgress(desc, 1.0f);

    thd_sleep(ACTION_COMPLETE_TIMEOUT_MS);
    DreameyeApp_ShowPhotoPage(NULL);
    return NULL;
}

void DreameyeApp_FileBrowserConfirm(GUI_Widget *widget) {
    if(!self.dev) {
        return;
    }

    GUI_CardStackShowIndex(self.pages, APP_PAGE_PROGRESS);
    UpdateProgress("Retrieving photo count...", 0.0f);
    self.photo_count = get_photo_count(self.dev);

    switch(self.action) {
        case APP_ACTION_PHOTO_EXPORT:
            thd_create(1, ExportPhotos, NULL);
            break;
        case APP_ACTION_PHOTO_EXPORT_SINGLE:
            thd_create(1, ExportSinglePhoto, NULL);
            break;
        default:
            UpdatePhotoCountLabels();
            DreameyeApp_ShowPhotoPage(widget);
            break;
    }
}

void DreameyeApp_Abort(GUI_Widget *widget) {
    (void)widget;
    self.action = APP_ACTION_IDLE;
}

static void on_thumb_loaded(int thumb_index, GUI_Surface *surface, GUI_Surface *hl_surface) {
    if (thumb_index < 0 || thumb_index >= GALLERY_THUMBS_PER_PAGE) {
        return;
    }

    if (surface) {
        // GUI_LabelSetText(self.thumb_labels[thumb_index], GUI_ObjectGetName((GUI_Object *)surface));
        GUI_LabelSetText(self.thumb_labels[thumb_index], " ");
        GUI_ButtonSetNormalImage(self.thumb_buttons[thumb_index], surface);

        if (hl_surface) {
            GUI_ButtonSetHighlightImage(self.thumb_buttons[thumb_index], hl_surface);
        }

        GUI_WidgetSetEnabled(self.thumb_widgets[thumb_index], 1);
    }
    else {
        GUI_LabelSetText(self.thumb_labels[thumb_index], "Empty");
    }
}

static void SwitchGalleryPage(int new_page) {
    gallery_state_t *state = gallery_get_state();
    int start_photo = new_page * GALLERY_THUMBS_PER_PAGE;

    gallery_load_abort();

    int photos_on_page = self.photo_count - start_photo;
    if (photos_on_page > GALLERY_THUMBS_PER_PAGE) {
        photos_on_page = GALLERY_THUMBS_PER_PAGE;
    }

    for (int i = 0; i < GALLERY_THUMBS_PER_PAGE; ++i) {
        GUI_WidgetSetEnabled(self.thumb_widgets[i], 0);

        GUI_ButtonSetNormalImage(self.thumb_buttons[i], self.default_thumb_surface);
        GUI_ButtonSetHighlightImage(self.thumb_buttons[i], self.default_thumb_hl_surface);

        if (i < photos_on_page) {
            GUI_LabelSetText(self.thumb_labels[i], "Loading...");
        }
        else {
            GUI_LabelSetText(self.thumb_labels[i], "Empty");
        }
    }

    char page_info[32];
    snprintf(page_info, sizeof(page_info), "Page %d/%d", new_page + 1, state->total_pages);
    GUI_LabelSetText(self.gallery_page_info, page_info);

    gallery_load_page(new_page, on_thumb_loaded);
}

void DreameyeApp_ShowGalleryPage(GUI_Widget *widget) {
    (void)widget;
    HideCameraPreview();

    gallery_load_abort();

    gallery_state_t *state = gallery_get_state();
    state->total_photos = self.photo_count;
    state->total_pages = (self.photo_count + GALLERY_THUMBS_PER_PAGE - 1) / GALLERY_THUMBS_PER_PAGE;

    if (state->total_pages == 0) {
        state->total_pages = 1;
    }

    GUI_CardStackShowIndex(self.pages, APP_PAGE_GALLERY);
    self.action = APP_ACTION_IDLE;
    
    SwitchGalleryPage(0);
}

void DreameyeApp_GalleryPrevPage(GUI_Widget *widget) {
    if(widget == NULL) {
        ds_sfx_play(DS_SFX_SLIDE);
    }

    gallery_state_t *state = gallery_get_state();
    if (state->current_page > 0 && !state->loading) {
        SwitchGalleryPage(state->current_page - 1);
    }
}

void DreameyeApp_GalleryNextPage(GUI_Widget *widget) {
    if(widget == NULL) {
        ds_sfx_play(DS_SFX_SLIDE);
    }

    gallery_state_t *state = gallery_get_state();
    if (state->current_page < state->total_pages - 1 && !state->loading) {
        SwitchGalleryPage(state->current_page + 1);
    }
}

void DreameyeApp_ViewPhoto(GUI_Widget *widget) {
    int thumb_index = -1;

    for (int i = 0; i < GALLERY_THUMBS_PER_PAGE; i++) {
        if (self.thumb_buttons[i] == widget) {
            thumb_index = i;
            break;
        }
    }

    if (thumb_index < 0) {
        return;
    }

    gallery_state_t *state = gallery_get_state();
    int global_photo_index = (state->current_page * GALLERY_THUMBS_PER_PAGE) + thumb_index;

    if (global_photo_index >= (int)self.photo_count) {
        return;
    }

    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    LoadPhotoIntoViewer(global_photo_index);
}

void DreameyeApp_ViewPrevPhoto(GUI_Widget *widget) {
    if(widget == NULL) {
        ds_sfx_play(DS_SFX_SLIDE);
    }

    gallery_state_t *state = gallery_get_state();
    if (state->current_photo > 0 && !state->loading) {
        LoadPhotoIntoViewer(state->current_photo - 1);
    }
}

void DreameyeApp_ViewNextPhoto(GUI_Widget *widget) {
    if(widget == NULL) {
        ds_sfx_play(DS_SFX_SLIDE);
    }

    gallery_state_t *state = gallery_get_state();
    if (state->current_photo < (int)self.photo_count - 1 && !state->loading) {
        LoadPhotoIntoViewer(state->current_photo + 1);
    }
}

void DreameyeApp_DeleteCurrentPhoto(GUI_Widget *widget) {
    (void)widget;

    gallery_state_t *state = gallery_get_state();
    if (!state->loading) {
        char confirm_text[64];
        snprintf(confirm_text, sizeof(confirm_text), "Delete photo %d/%d?", 
                state->current_photo + 1, (int)self.photo_count);
        GUI_LabelSetText(self.confirm_delete_text, confirm_text);
        GUI_CardStackShowIndex(self.pages, APP_PAGE_CONFIRM_DELETE);
        self.action = APP_ACTION_PHOTO_ERASE_SINGLE;
    }
}

void DreameyeApp_ExportCurrentPhoto(GUI_Widget *widget) {
    (void)widget;

    gallery_state_t *state = gallery_get_state();
    if (!state->loading) {
        GUI_CardStackShowIndex(self.pages, APP_PAGE_FILE_BROWSER);
        self.action = APP_ACTION_PHOTO_EXPORT_SINGLE;
    }
}

void DreameyeApp_CancelDelete(GUI_Widget *widget) {
    (void)widget;
    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    self.action = APP_ACTION_IDLE;
}

void DreameyeApp_CancelDeleteFromViewer(GUI_Widget *widget) {
    (void)widget;
    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    self.action = APP_ACTION_IDLE;
}

void DreameyeApp_CancelDeleteFromGallery(GUI_Widget *widget) {
    (void)widget;
    GUI_CardStackShowIndex(self.pages, APP_PAGE_GALLERY);
    self.action = APP_ACTION_IDLE;
}

void DreameyeApp_CancelExportFromGallery(GUI_Widget *widget) {
    (void)widget;
    GUI_CardStackShowIndex(self.pages, APP_PAGE_GALLERY);
    self.action = APP_ACTION_IDLE;
}

void DreameyeApp_CancelExport(GUI_Widget *widget) {
    (void)widget;

    if (self.action == APP_ACTION_PHOTO_EXPORT_SINGLE) {
        GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    }
    else {
        GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO);
    }

    self.action = APP_ACTION_IDLE;
}

void DreameyeApp_ConfirmDelete(GUI_Widget *widget) {
    (void)widget;
    GUI_CardStackShowIndex(self.pages, APP_PAGE_PROGRESS);

    if (self.action == APP_ACTION_PHOTO_ERASE) {
        UpdateProgress("Erasing photos...", 0.0f);
        thd_create(1, ErasePhotos, NULL);
    }
    else if (self.action == APP_ACTION_PHOTO_ERASE_SINGLE) {
        UpdateProgress("Deleting photo...", 0.0f);
        thd_create(1, EraseSinglePhoto, NULL);
    }
}

static void on_fullscreen_loaded(GUI_Surface *surface, GUI_Surface *hl_surface) {

    LockVideo();
    GUI_LabelSetText(self.fullscreen_status, " ");

    if (surface) {
        GUI_ButtonSetNormalImage(self.fullscreen_photo, surface);
        GUI_ObjectDecRef((GUI_Object*)surface);

        if (hl_surface) {
            GUI_ButtonSetHighlightImage(self.fullscreen_photo, hl_surface);

            if (hl_surface != surface) {
                GUI_ObjectDecRef((GUI_Object*)hl_surface);
            }
        }
    }

    GUI_WidgetSetEnabled(self.fullscreen_photo, 1);
    UnlockVideo();
}

void DreameyeApp_ShowFullscreenPhoto(GUI_Widget *widget) {
    (void)widget;

    LockVideo();
    GUI_WidgetSetEnabled(self.fullscreen_photo, 0);
    GUI_LabelSetText(self.fullscreen_status, "Loading...");
    GUI_CardStackShowIndex(self.pages, APP_PAGE_FULLSCREEN_VIEWER);
    GUI_ContainerRemove(self.app->body, self.header_panel);
    GUI_WidgetSetPosition(self.pages, 0, 0);
    GUI_WidgetMarkChanged(self.app->body);
    UnlockVideo();

    gallery_state_t *state = gallery_get_state();
    gallery_load_photo(state->current_photo, 0, 0, on_fullscreen_loaded);
}

void DreameyeApp_ExitFullscreen(GUI_Widget *widget) {
    (void)widget;

    LockVideo();
    GUI_ContainerRemove(self.app->body, self.pages);
    GUI_ContainerAdd(self.app->body, self.header_panel);
    GUI_ContainerAdd(self.app->body, self.pages);
    GUI_WidgetSetPosition(self.pages, 0, 45);
    GUI_CardStackShowIndex(self.pages, APP_PAGE_PHOTO_VIEWER);
    GUI_WidgetMarkChanged(self.app->body);
    UnlockVideo();
}

static void DreameyeApp_FullscreenPrevPhoto(void) {
    gallery_state_t *state = gallery_get_state();

    if (state->current_photo > 0 && !state->loading) {

        ds_sfx_play(DS_SFX_SLIDE);
        state->current_photo--;

        GUI_WidgetSetEnabled(self.fullscreen_photo, 0);
        GUI_LabelSetText(self.fullscreen_status, "Loading...");

        gallery_load_photo(state->current_photo, 0, 0, on_fullscreen_loaded);
    }
}

static void DreameyeApp_FullscreenNextPhoto(void) {
    gallery_state_t *state = gallery_get_state();

    if (state->current_photo < (int)self.photo_count - 1 && !state->loading) {

        ds_sfx_play(DS_SFX_SLIDE);
        state->current_photo++;

        GUI_WidgetSetEnabled(self.fullscreen_photo, 0);
        GUI_LabelSetText(self.fullscreen_status, "Loading...");

        gallery_load_photo(state->current_photo, 0, 0, on_fullscreen_loaded);
    }
}
