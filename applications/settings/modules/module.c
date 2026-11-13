/* DreamShell ##version##

   module.c - Settings app module
   Copyright (C)2016-2026 SWAT 
*/

#include "ds.h"

DEFAULT_MODULE_EXPORTS(app_settings);

/* The first child of panel with checkboxes it's a label */
#define WIDGET_VALUE_OFFSET 1

enum {
	DIALOG_ACTION_NONE = 0,
	DIALOG_ACTION_RESET,
	DIALOG_ACTION_SAVE,
	DIALOG_ACTION_EXIT,
	DIALOG_ACTION_DONE
};

#define DIALOG_CONFIRM_BUTTON 0
#define DIALOG_CANCEL_BUTTON 1
#define DIALOG_CONFIRM_TEXT "Confirm"
#define DIALOG_CANCEL_TEXT "Cancel"
#define DIALOG_VMU_TEXT "VMU"
#define DIALOG_ROOT_TEXT "Root"

enum {
	NATIVE_MODE_AUTO = 0,
	NATIVE_MODE_PAL,
	NATIVE_MODE_NTSC,
	NATIVE_MODE_VGA
};

enum {
	SCREEN_MODE_4_3 = 0,
	SCREEN_MODE_3_2,
	SCREEN_MODE_16_10,
	SCREEN_MOVE_16_9
};

enum {
	SCREEN_FILTER_AUTO = 0,
	SCREEN_FILTER_NEAREST,
	SCREEN_FILTER_BILINEAR,
	SCREEN_FILTER_TRILINEAR
};

static struct {
	
	App_t *app;
	Settings_t *settings;
	GUI_Widget *pages;
	GUI_Widget *dialog;
	int dialog_action;
	int exit_after_dialog;
	Settings_t stored;
	GUI_Widget *sysdate[6];
	GUI_Widget *volume_knob;
	GUI_Widget *volume_label;
	GUI_Widget *sfx_chk;
	GUI_Widget *click_chk;
	GUI_Widget *hover_chk;
	GUI_Widget *startup_chk;
	GUI_Widget *timezone_entry;
	
	int prev_sfx_enabled;
	int prev_click_enabled;
	int prev_hover_enabled;
	int prev_startup_enabled;
	int volume_was_zero;
	
} self;

static void SetupVideoSettings();
static void SetupBootSettings();
static void SetupAudioSettings();
static void SetupTimezoneSettings();
static void SetupAppListPanel(GUI_Widget *panel, GUI_Widget *scroll, GUI_Widget *up, GUI_Widget *down, const char *selected, void (*toggle_fn)(GUI_Widget *));

static int UncheckBesides(GUI_Widget *widget, const char *label) {

	int index = 0;
	GUI_Widget *panel, *child;
	
	if(label) {

		panel = widget;
		
		for(int i = 0; i < GUI_ContainerGetCount(panel); ++i) {
			
			widget = GUI_ContainerGetChild(panel, i);
			GUI_Widget *l = GUI_ToggleButtonGetCaption(widget);
			
			if(!strcasecmp(label, GUI_LabelGetText(l))) {
				break;
			}
		}
		
	} else {
		panel = GUI_WidgetGetParent(widget);
	}
	
	if(!GUI_WidgetGetState(widget)) {
		GUI_WidgetSetState(widget, 1);
	}
	
	for(int i = 0; i < GUI_ContainerGetCount(panel); ++i) {
		
		child = GUI_ContainerGetChild(panel, i);
		
		if(widget != child) {
			GUI_WidgetSetState(child, 0);
		} else {
			index = i;
		}
	}

	return index - WIDGET_VALUE_OFFSET;
}


void SettingsApp_Init(App_t *app) {

	if(app != NULL) {

		memset(&self, 0, sizeof(self));
		self.app = app;
		self.settings = GetSettings();
		self.pages = APP_GET_WIDGET("pages");
		self.dialog = APP_GET_WIDGET("dialog");
		self.dialog_action = DIALOG_ACTION_NONE;
		self.sysdate[0] = APP_GET_WIDGET("year");
		self.sysdate[1] = APP_GET_WIDGET("month");
		self.sysdate[2] = APP_GET_WIDGET("day");
		self.sysdate[3] = APP_GET_WIDGET("hours");
		self.sysdate[4] = APP_GET_WIDGET("minute");
		
		self.timezone_entry = APP_GET_WIDGET("timezone");
		self.volume_knob = APP_GET_WIDGET("volume-slider");
		self.volume_label = APP_GET_WIDGET("volume-label");
		self.sfx_chk = APP_GET_WIDGET("sfx-chk");
		self.click_chk = APP_GET_WIDGET("click-chk");
		self.hover_chk = APP_GET_WIDGET("hover-chk");
		self.startup_chk = APP_GET_WIDGET("startup-chk");
		
		self.prev_sfx_enabled = 1;
		self.prev_click_enabled = 1;
		self.prev_hover_enabled = 1;
		self.prev_startup_enabled = 1;
		self.volume_was_zero = 0;
		
		SetupVideoSettings();
		SetupBootSettings();
		SetupAudioSettings();
		SetupTimezoneSettings();
		memcpy(&self.stored, self.settings, sizeof(self.stored));
	}
}


void SettingsApp_ShowPage(GUI_Widget *widget) {
	
	const char *object_name, *name_ptr;
	char name[64];
	size_t size;
	
	ScreenFadeOutEx(NULL, 1);
	object_name = GUI_ObjectGetName((GUI_Object *)widget);
	name_ptr = strchr(object_name, '-');
	size = (name_ptr - object_name);
	
	if(size < sizeof(name) - 5) {

		strncpy(name, object_name, size);
		strncpy(name + size, "-page", 6);

		GUI_CardStackShow(self.pages, name);
	}
	
	ScreenFadeIn();
}

static void DoResetSettings() {
	ResetSettings();
	SetupBootSettings();
	SetupVideoSettings();
	SetupAudioSettings();
	SetupTimezoneSettings();
	SetScreenMode(self.settings->video.virt_width, self.settings->video.virt_height, 0.0f, 0.0f, 1.0f);
	SetScreenFilter(self.settings->video.tex_filter);
	memcpy(&self.stored, self.settings, sizeof(self.stored));
}

static int SettingsChanged(void) {
	return memcmp(self.settings, &self.stored, sizeof(self.stored)) != 0;
}

static int VmuPresent(void) {
	return maple_enum_type(0, MAPLE_FUNC_MEMCARD) != NULL;
}

static void SetDialogButtonText(int index, const char *text) {
	GUI_Widget *content;
	GUI_Widget *buttons;
	GUI_Widget *button;
	GUI_Widget *caption;
	int count;

	if(!self.dialog) {
		return;
	}

	content = GUI_ContainerGetChild(self.dialog, 0);

	if(!content) {
		return;
	}

	count = GUI_ContainerGetCount(content);

	if(count <= 0) {
		return;
	}

	buttons = GUI_ContainerGetChild(content, count - 1);
	button = buttons ? GUI_ContainerGetChild(buttons, index) : NULL;
	caption = button ? GUI_ButtonGetCaption(button) : NULL;

	if(caption) {
		GUI_LabelSetText(caption, text);
	}
}

static void ShowConfirmDialog(const char *title, const char *body, const char *confirm_text, const char *cancel_text, int action) {
	self.dialog_action = action;
	SetDialogButtonText(DIALOG_CONFIRM_BUTTON, confirm_text);
	SetDialogButtonText(DIALOG_CANCEL_BUTTON, cancel_text);
	GUI_DialogShow(self.dialog, DIALOG_MODE_CONFIRM, title, body);
}

static void ShowSaveTargetDialog(void) {
	ShowConfirmDialog("Save settings?", "Choose where to save settings.", DIALOG_VMU_TEXT, DIALOG_ROOT_TEXT, DIALOG_ACTION_SAVE);
}

static void FinishSave(int where) {
	int ok;
	int exit_after;
	char body[64];

	SetDialogButtonText(DIALOG_CONFIRM_BUTTON, DIALOG_CONFIRM_TEXT);
	SetDialogButtonText(DIALOG_CANCEL_BUTTON, DIALOG_CANCEL_TEXT);
	GUI_DialogShow(self.dialog, DIALOG_MODE_INFO, "Saving settings", "Please wait, saving settings to device...");
	ok = SaveSettingsTo(where);
	exit_after = self.exit_after_dialog;

	if(ok) {
		memcpy(&self.stored, self.settings, sizeof(self.stored));
	}
	else {
		exit_after = 0;
		self.exit_after_dialog = 0;
	}

	if(exit_after) {
		self.dialog_action = DIALOG_ACTION_NONE;
		self.exit_after_dialog = 0;
		GUI_DialogHide(self.dialog);
		dsystem("app -o");
		return;
	}

	if(ok) {
		snprintf(body, sizeof(body), where == DS_SAVE_DEST_VMU ? "Saved to VMU." : "Saved to root.");
	}
	else {
		snprintf(body, sizeof(body), where == DS_SAVE_DEST_VMU ? "Can't save to VMU." : "Can't save to root.");
	}

	self.dialog_action = DIALOG_ACTION_DONE;
	GUI_DialogShow(self.dialog, DIALOG_MODE_ALERT, "Saving settings", body);
}

void SettingsApp_ResetSettings(GUI_Widget *widget) {
	(void)widget;
	ShowConfirmDialog("Reset settings?", "Are you sure you want to reset all settings?", DIALOG_CONFIRM_TEXT, DIALOG_CANCEL_TEXT, DIALOG_ACTION_RESET);
}

void SettingsApp_SaveSettings(GUI_Widget *widget) {
	(void)widget;

	self.exit_after_dialog = 0;

	if(!VmuPresent()) {
		FinishSave(DS_SAVE_DEST_HOME);
		return;
	}

	ShowSaveTargetDialog();
}

void SettingsApp_Exit(GUI_Widget *widget) {
	(void)widget;

	if(!SettingsChanged()) {
		dsystem("app -o");
		return;
	}

	self.exit_after_dialog = 0;
	ShowConfirmDialog("Saving settings", "Do you want to save changed settings?", DIALOG_CONFIRM_TEXT, DIALOG_CANCEL_TEXT, DIALOG_ACTION_EXIT);
}

void SettingsApp_DialogConfirm(GUI_Widget *widget) {
	switch(self.dialog_action) {
		case DIALOG_ACTION_RESET:
			GUI_DialogShow(widget, DIALOG_MODE_INFO, "Resetting settings", "Please wait, removing settings files...");
			DoResetSettings();
			self.dialog_action = DIALOG_ACTION_DONE;
			GUI_DialogShow(widget, DIALOG_MODE_ALERT, "Resetting settings", "Completed.");
			break;
		case DIALOG_ACTION_SAVE:
			FinishSave(DS_SAVE_DEST_VMU);
			break;
		case DIALOG_ACTION_EXIT:
			if(VmuPresent()) {
				self.exit_after_dialog = 1;
				ShowSaveTargetDialog();
			}
			else {
				self.exit_after_dialog = 1;
				FinishSave(DS_SAVE_DEST_HOME);
			}
			break;
		case DIALOG_ACTION_DONE:
			{
				int exit_after = self.exit_after_dialog;

				self.dialog_action = DIALOG_ACTION_NONE;
				self.exit_after_dialog = 0;
				GUI_DialogHide(widget);

				if(exit_after) {
					dsystem("app -o");
				}
			}
			break;
		default:
			self.dialog_action = DIALOG_ACTION_NONE;
			GUI_DialogHide(widget);
			break;
	}
}

void SettingsApp_DialogCancel(GUI_Widget *widget) {
	int action = self.dialog_action;

	if(action == DIALOG_ACTION_SAVE) {
		FinishSave(DS_SAVE_DEST_HOME);
		return;
	}

	self.dialog_action = DIALOG_ACTION_NONE;
	self.exit_after_dialog = 0;
	GUI_DialogHide(widget);

	if(action == DIALOG_ACTION_EXIT) {
		dsystem("app -o");
	}
}

void SettingsApp_Reboot(GUI_Widget *widget) {
	ScreenFadeOutEx("Loading...", 0);
	dsystemf("exec -b -f %s/DS_CORE.BIN", getenv("PATH"));
}

void SettingsApp_ToggleNativeMode(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case NATIVE_MODE_AUTO:
			memset(&self.settings->video.mode, 0, sizeof(self.settings->video.mode));
//			SetVideoMode(-1);
			break;
		case NATIVE_MODE_PAL:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_PAL_IL], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_PAL_IL);
			break;
		case NATIVE_MODE_NTSC:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_NTSC_IL], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_NTSC_IL);
			break;
		case NATIVE_MODE_VGA:
			memcpy(&self.settings->video.mode, &vid_builtin[DM_640x480_VGA], sizeof(self.settings->video.mode));
//			SetVideoMode(DM_640x480_VGA);
			break;
		default:
			return;
	}
}


void SettingsApp_ToggleScreenMode(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case SCREEN_MODE_4_3:
			self.settings->video.virt_width = 640;
			break;
		case SCREEN_MODE_3_2:
			self.settings->video.virt_width = 720;
			break;
		case SCREEN_MODE_16_10:
			self.settings->video.virt_width = 768;
			break;
		case SCREEN_MOVE_16_9:
			self.settings->video.virt_width = 854;
			break;
		default:
			return;
	}
	
	self.settings->video.virt_height = 480;
	SetScreenMode(self.settings->video.virt_width, self.settings->video.virt_height, 0.0f, 0.0f, 1.0f);
}


void SettingsApp_ToggleScreenFilter(GUI_Widget *widget) {

	int value = UncheckBesides(widget, NULL);
	
	switch(value) {
		case SCREEN_FILTER_AUTO:
			self.settings->video.tex_filter = -1;
			break;
		case SCREEN_FILTER_NEAREST:
			self.settings->video.tex_filter = PVR_FILTER_NEAREST;
			break;
		case SCREEN_FILTER_BILINEAR:
			self.settings->video.tex_filter = PVR_FILTER_BILINEAR;
			break;
//		case SCREEN_FILTER_TRILINEAR:
//			self.settings->video.tex_filter = PVR_FILTER_TRILINEAR1;
//			break;
		default:
			return;
	}
	
	SetScreenFilter(self.settings->video.tex_filter);
}


static void SetupVideoSettings() {
	
	GUI_Widget *panel;
	int value = NATIVE_MODE_AUTO;

	panel = APP_GET_WIDGET("display-native-mode");
	
	if(self.settings->video.mode.width > 0) {

		for(int i = 0; i < DM_SENTINEL; ++i) {

			if(!memcmp(&self.settings->video.mode, &vid_builtin[i], sizeof(vid_builtin[i]))) {
				
				switch(i) {
					case DM_640x480_PAL_IL:
						value = NATIVE_MODE_PAL;
						break;
					case DM_640x480_NTSC_IL:
						value = NATIVE_MODE_NTSC;
						break;
					case DM_640x480_VGA:
						value = NATIVE_MODE_VGA;
						break;
					default:
						break;
				}
				break;
			}
		}
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
	
	panel = APP_GET_WIDGET("display-screen-mode");
	
	switch(self.settings->video.virt_width) {
		case 640:
			value = SCREEN_MODE_4_3;
			break;
		case 720:
			value = SCREEN_MODE_3_2;
			break;
		case 854:
			value = SCREEN_MOVE_16_9;
			break;
		default:
			value = SCREEN_MODE_4_3;
			break;
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
	
	panel = APP_GET_WIDGET("display-screen-filter");
	
	switch(self.settings->video.tex_filter) {
		case -1:
			value = SCREEN_FILTER_AUTO;
			break;
		case PVR_FILTER_NEAREST:
			value = SCREEN_FILTER_AUTO;
			break;
		case PVR_FILTER_BILINEAR:
			value = SCREEN_FILTER_AUTO;
			break;
//			case PVR_FILTER_TRILINEAR1:
//				value = SCREEN_FILTER_TRILINEAR;
//				break;
		default:
			value = SCREEN_FILTER_AUTO;
			break;
	}
	
	UncheckBesides(GUI_ContainerGetChild(panel, value + WIDGET_VALUE_OFFSET), NULL);
}



void SettingsApp_ToggleStartupApp(GUI_Widget *widget) {
	UncheckBesides(widget, NULL);
	strncpy(self.settings->startup_app, GUI_ObjectGetName((GUI_Object *)widget), sizeof(self.settings->startup_app));
}


void SettingsApp_ToggleMainApp(GUI_Widget *widget) {
	UncheckBesides(widget, NULL);
	strncpy(self.settings->main_app, GUI_ObjectGetName((GUI_Object *)widget), sizeof(self.settings->main_app));
}


static void StoreDirPath(char *dst, size_t dst_size, const char *path, const char *label_name) {
	GUI_Widget *label;

	if(path == NULL || path[0] == '\0') {
		dst[0] = '\0';
		path = "Auto";
	}
	else {
		strncpy(dst, path, dst_size - 1);
		dst[dst_size - 1] = '\0';
		path = dst;
	}

	label = APP_GET_WIDGET(label_name);

	if(label != NULL) {
		GUI_LabelSetText(label, path);
	}
}

static void SelectDirectory(dirent_fm_t *fm_ent, char *dst, size_t dst_size, const char *label_name) {
	GUI_Widget *browser;
	const char *path;

	if(fm_ent == NULL || fm_ent->ent.attr != O_DIR) {
		return;
	}

	browser = (GUI_Widget *)fm_ent->obj;
	GUI_FileManagerChangeDir(browser, fm_ent->ent.name, fm_ent->ent.size);
	path = GUI_FileManagerGetPath(browser);
	StoreDirPath(dst, dst_size, path, label_name);
}

void SettingsApp_SelectRoot(dirent_fm_t *fm_ent) {
	SelectDirectory(fm_ent, self.settings->root, sizeof(self.settings->root), "boot-root-path");
}

void SettingsApp_SelectHome(dirent_fm_t *fm_ent) {
	SelectDirectory(fm_ent, self.settings->home, sizeof(self.settings->home), "boot-home-path");
}


void SettingsApp_ToggleArcade(GUI_Widget *widget) {
	self.settings->arcade = GUI_WidgetGetState(widget);
}

void SettingsApp_ToggleStartupSound(GUI_Widget *widget) {
	self.settings->audio.startup_enabled = GUI_WidgetGetState(widget);
}


typedef struct AppListScroll {
	GUI_Widget *panel;
	GUI_Widget *scroll;
	GUI_Widget *up;
	GUI_Widget *down;
	int content_h;
	int view_h;
	int travel;
	int step;
} AppListScroll_t;

static void AppListScrollUpdateButtons(AppListScroll_t *scroll) {
	int offset;
	int max_offset;

	if(!scroll->up || !scroll->down) {
		return;
	}

	max_offset = scroll->content_h - scroll->view_h;
	offset = GUI_PanelGetYOffset(scroll->panel);
	GUI_WidgetSetEnabled(scroll->up, offset > 0);
	GUI_WidgetSetEnabled(scroll->down, offset < max_offset);
}

static void AppListScrollMoved(void *data) {
	AppListScroll_t *scroll = (AppListScroll_t *)data;
	int knob_y;
	int max_offset;
	int offset;

	if(!scroll || !scroll->panel) {
		return;
	}

	max_offset = scroll->content_h - scroll->view_h;

	if(max_offset <= 0 || scroll->travel <= 0) {
		GUI_PanelSetYOffset(scroll->panel, 0);
		AppListScrollUpdateButtons(scroll);
		return;
	}

	knob_y = GUI_ScrollBarGetVerticalPosition(scroll->scroll);
	offset = (knob_y * max_offset) / scroll->travel;

	if(offset > max_offset) {
		offset = max_offset;
	}

	GUI_PanelSetYOffset(scroll->panel, offset);
	AppListScrollUpdateButtons(scroll);
}

static void AppListScrollByButton(AppListScroll_t *scroll, int direction) {
	int max_offset;
	int offset;
	int knob_y;

	if(!scroll || !scroll->panel || scroll->step < 1) {
		return;
	}

	max_offset = scroll->content_h - scroll->view_h;
	offset = GUI_PanelGetYOffset(scroll->panel) + direction * scroll->step;

	if(offset < 0) {
		offset = 0;
	}

	if(offset > max_offset) {
		offset = max_offset;
	}

	GUI_PanelSetYOffset(scroll->panel, offset);

	if(max_offset <= 0 || scroll->travel <= 0) {
		knob_y = 0;
	}
	else {
		knob_y = (offset * scroll->travel) / max_offset;
	}

	GUI_ScrollBarSetVerticalPosition(scroll->scroll, knob_y);
	AppListScrollUpdateButtons(scroll);
}

static void AppListScrollUp(void *data) {
	AppListScrollByButton((AppListScroll_t *)data, -1);
}

static void AppListScrollDown(void *data) {
	AppListScrollByButton((AppListScroll_t *)data, 1);
}

static void AppListScrollHide(GUI_Widget *scroll, GUI_Widget *up, GUI_Widget *down) {
	if(scroll) {
		GUI_WidgetSetFlags(scroll, WIDGET_HIDDEN);
	}

	if(up) {
		GUI_WidgetSetFlags(up, WIDGET_HIDDEN);
	}

	if(down) {
		GUI_WidgetSetFlags(down, WIDGET_HIDDEN);
	}
}

static void BindAppListScrollButton(GUI_Widget *button, void (*fn)(void *), AppListScroll_t *data) {
	GUI_Callback *callback;

	if(!button) {
		return;
	}

	callback = GUI_CallbackCreate(fn, NULL, data);

	if(!callback) {
		return;
	}

	GUI_ButtonSetClick(button, callback);
	GUI_ObjectDecRef((GUI_Object *)callback);
	GUI_WidgetClearFlags(button, WIDGET_HIDDEN);
}

static void BindAppListScroll(GUI_Widget *panel, GUI_Widget *scroll, GUI_Widget *up, GUI_Widget *down) {
	AppListScroll_t *data;
	GUI_Callback *callback;
	GUI_Surface *knob;
	SDL_Rect view;
	SDL_Rect track;
	int count;
	int content_h;
	int knob_h;
	int travel;
	int step;

	if(!panel || !scroll) {
		return;
	}

	view = GUI_WidgetGetArea(panel);
	count = GUI_ContainerGetCount(panel);
	content_h = 0;
	step = 0;

	if(count > 0) {
		SDL_Rect first = GUI_WidgetGetArea(GUI_ContainerGetChild(panel, 0));
		SDL_Rect last = GUI_WidgetGetArea(GUI_ContainerGetChild(panel, count - 1));
		content_h = last.y + last.h;
		step = first.h;
	}

	if(content_h <= view.h) {
		AppListScrollHide(scroll, up, down);
		GUI_PanelSetYOffset(panel, 0);
		return;
	}

	track = GUI_WidgetGetArea(scroll);
	knob = GUI_ScrollBarGetKnobImage(scroll);
	knob_h = knob ? GUI_SurfaceGetHeight(knob) : track.w * 2;
	travel = track.h - knob_h;

	if(travel < 1) {
		AppListScrollHide(scroll, up, down);
		return;
	}

	data = (AppListScroll_t *)malloc(sizeof(AppListScroll_t));

	if(!data) {
		return;
	}

	data->panel = panel;
	data->scroll = scroll;
	data->up = up;
	data->down = down;
	data->content_h = content_h;
	data->view_h = view.h;
	data->travel = travel;
	data->step = step;

	callback = GUI_CallbackCreate(AppListScrollMoved, free, data);

	if(!callback) {
		free(data);
		return;
	}

	GUI_WidgetClearFlags(scroll, WIDGET_HIDDEN);
	GUI_ScrollBarSetVerticalPosition(scroll, 0);
	GUI_ScrollBarSetMovedCallback(scroll, callback);
	GUI_ObjectDecRef((GUI_Object *)callback);
	BindAppListScrollButton(up, AppListScrollUp, data);
	BindAppListScrollButton(down, AppListScrollDown, data);
	GUI_PanelSetYOffset(panel, 0);
	AppListScrollUpdateButtons(data);
}

static void SetupAppListPanel(GUI_Widget *panel, GUI_Widget *scroll, GUI_Widget *up, GUI_Widget *down, const char *selected, void (*toggle_fn)(GUI_Widget *)) {

	Item_list_t *applist = GetAppList();
	GUI_Font *font = APP_GET_FONT("arial");
	SDL_Rect area;

	if(!applist || !panel) {
		return;
	}

	if(GUI_ContainerGetCount(panel) > 0) {
		UncheckBesides(panel, selected);
		return;
	}

	area = GUI_WidgetGetArea(panel);

	Item_t *item = listGetItemFirst(applist);

	while(item != NULL) {

		App_t *app = (App_t *)item->data;
		item = listGetItemNext(item);

		SDL_Rect ts = GUI_FontGetTextSize(font, app->name);

		int h = ts.h + 8;
		int w = area.w > 0 ? area.w : ts.w + h + 12;

		GUI_Widget *b = GUI_ToggleButtonCreate(app->name, 0, 0, w, h);

		GUI_Callback *c = GUI_CallbackCreate((GUI_CallbackFunction *)toggle_fn, NULL, b);
		GUI_ToggleButtonSetClick(b, c);
		GUI_ObjectDecRef((GUI_Object *) c);

		if(!strncasecmp(app->name, selected, sizeof(app->name))) {
			GUI_WidgetSetState(b, 1);
		}
		else {
			GUI_WidgetSetState(b, 0);
		}

		GUI_Widget *l = GUI_LabelCreate(app->name, 22, 0, w - 24, h - 5, font, app->name);
		GUI_LabelSetTextColor(l, 51, 51, 51);
		GUI_WidgetSetAlign(l, WIDGET_HORIZ_LEFT | WIDGET_VERT_CENTER);
		GUI_ButtonSetCaption(b, l);
		GUI_ObjectDecRef((GUI_Object *) l);

		GUI_ContainerAdd(panel, b);
		GUI_ObjectDecRef((GUI_Object *) b);
	}

	BindAppListScroll(panel, scroll, up, down);
}


static void OpenDirectoryBrowser(const char *browser_name, const char *label_name, const char *stored, const char *fallback) {
	GUI_Widget *browser;
	GUI_Widget *label;
	const char *open_path;
	const char *shown;

	browser = APP_GET_WIDGET(browser_name);
	label = APP_GET_WIDGET(label_name);

	if(stored != NULL && stored[0] != '\0') {
		open_path = stored;
		shown = stored;
	}
	else {
		open_path = (fallback != NULL && fallback[0] != '\0') ? fallback : "/";
		shown = "Auto";
	}

	if(browser != NULL) {
		GUI_FileManagerSetPath(browser, open_path);
	}

	if(label != NULL) {
		GUI_LabelSetText(label, shown);
	}
}

static void SetupBootSettings() {

	GUI_Widget *panel;

	OpenDirectoryBrowser("boot-root", "boot-root-path", self.settings->root, getenv("PATH"));
	OpenDirectoryBrowser("boot-home", "boot-home-path", self.settings->home, getenv("HOME"));

	panel = APP_GET_WIDGET("arcade-chk");

	if(panel) {
		GUI_WidgetSetState(panel, self.settings->arcade ? 1 : 0);
	}

	panel = APP_GET_WIDGET("boot-startup-app");
	SetupAppListPanel(panel, APP_GET_WIDGET("boot-startup-scroll"), APP_GET_WIDGET("boot-startup-up"), APP_GET_WIDGET("boot-startup-down"), self.settings->startup_app, SettingsApp_ToggleStartupApp);

	panel = APP_GET_WIDGET("boot-main-app");
	SetupAppListPanel(panel, APP_GET_WIDGET("boot-main-scroll"), APP_GET_WIDGET("boot-main-up"), APP_GET_WIDGET("boot-main-down"), self.settings->main_app, SettingsApp_ToggleMainApp);
}

static void update_rtc_ui() {
	char buf[5];
	struct tm *time;
	time_t unix_time;

	unix_time = rtc_unix_secs();
	time = gmtime(&unix_time);

	if (time != NULL) {
		sprintf(buf, "%04d", time->tm_year + 1900);
		GUI_TextEntrySetText(self.sysdate[0], buf);
		sprintf(buf, "%02d", time->tm_mon + 1);
		GUI_TextEntrySetText(self.sysdate[1], buf);
		sprintf(buf, "%02d", time->tm_mday);
		GUI_TextEntrySetText(self.sysdate[2], buf);
		sprintf(buf, "%02d", time->tm_hour);
		GUI_TextEntrySetText(self.sysdate[3], buf);
		sprintf(buf, "%02d", time->tm_min);
		GUI_TextEntrySetText(self.sysdate[4], buf);
	}
}

void SettingsApp_TimeChange(GUI_Widget *widget)
{
    struct tm time;
	time_t unix_time;

	if(strcmp(GUI_ObjectGetName(widget), "get-time") == 0)
	{
		update_rtc_ui();
	}
	else if(strcmp(GUI_ObjectGetName(widget), "set-time") == 0)
	{
		time.tm_sec = 0;
		time.tm_min = atoi(GUI_TextEntryGetText(self.sysdate[4]));
		time.tm_hour = atoi(GUI_TextEntryGetText(self.sysdate[3]));
		time.tm_mday = atoi(GUI_TextEntryGetText(self.sysdate[2]));
		time.tm_mon = atoi(GUI_TextEntryGetText(self.sysdate[1])) - 1;
		time.tm_year = atoi(GUI_TextEntryGetText(self.sysdate[0])) - 1900;

		unix_time = mktime(&time);
		if(unix_time != -1) {
			rtc_set_unix_secs(unix_time);
		}
	}
	else if(strcmp(GUI_ObjectGetName(widget), "sync-time") == 0)
	{
		ShowConsole();
		dsystem("ntp --sync");
		thd_sleep(1000);
		update_rtc_ui();
		HideConsole();
	}
}

void SettingsApp_Time(GUI_Widget *widget)
{
	char wname[32];
	sprintf(wname,"%s",GUI_ObjectGetName(widget));
	int entryvar = atoi(GUI_TextEntryGetText(widget));
	
	switch (wname[strlen(wname)-1])
	{	
		case 'r':
			if(entryvar < 1998) GUI_TextEntrySetText(widget, "1998");
			if(entryvar > 2086) GUI_TextEntrySetText(widget, "2086");
			break;
		case 'h':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "01");
			if(entryvar > 12) GUI_TextEntrySetText(widget, "12");
			break;
		case 'y':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "01");
			if(entryvar > 31) GUI_TextEntrySetText(widget, "31");
			break;
		case 's':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "00");
			if(entryvar > 23) GUI_TextEntrySetText(widget, "23");
			break;
		case 'e':
			if(entryvar < 1) GUI_TextEntrySetText(widget, "00");
			if(entryvar > 59) GUI_TextEntrySetText(widget, "59");
			break;
	}
}

void SettingsApp_Time_Clr(GUI_Widget *widget)
{
	GUI_TextEntrySetText(widget, "");
}

void SettingsApp_TimezoneClr(GUI_Widget *widget)
{
	GUI_TextEntrySetText(widget, "");
}

static void SetupTimezoneSettings() {
	char buf[16];
	int tz_minutes_total = self.settings->time_zone;
	int hours = tz_minutes_total / 60;
	int minutes = abs(tz_minutes_total % 60);

	if (minutes == 0) {
		snprintf(buf, sizeof(buf), "%+d", hours);
	} else {
		snprintf(buf, sizeof(buf), "%+d:%02d", hours, minutes);
	}
	GUI_TextEntrySetText(self.timezone_entry, buf);
}

static void SetupAudioSettings() {
	
	if(self.volume_knob && self.volume_label) {
		int volume = self.settings->audio.volume;
		int knob_pos = (volume * 180) / 255;
		char buf[8];
		
		GUI_ScrollBarSetHorizontalPosition(self.volume_knob, knob_pos);
		sprintf(buf, "%d", volume);
		GUI_LabelSetText(self.volume_label, buf);
	}
	
	int volume_enabled = (self.settings->audio.volume > 0);
	int sfx_and_volume_enabled = volume_enabled && self.settings->audio.sfx_enabled;
	
	if(self.sfx_chk) {
		GUI_WidgetSetState(self.sfx_chk, self.settings->audio.sfx_enabled);
		GUI_WidgetSetEnabled(self.sfx_chk, volume_enabled);
	}
	
	if(self.click_chk) {
		GUI_WidgetSetState(self.click_chk, self.settings->audio.click_enabled);
		GUI_WidgetSetEnabled(self.click_chk, sfx_and_volume_enabled);
	}
	
	if(self.hover_chk) {
		GUI_WidgetSetState(self.hover_chk, self.settings->audio.hover_enabled);
		GUI_WidgetSetEnabled(self.hover_chk, sfx_and_volume_enabled);
	}

	if(self.startup_chk) {
		GUI_WidgetSetState(self.startup_chk, self.settings->audio.startup_enabled);
		GUI_WidgetSetEnabled(self.startup_chk, volume_enabled);
	}
}

void SettingsApp_ToggleSfx(GUI_Widget *widget) {
	int enabled = GUI_WidgetGetState(widget);
	self.settings->audio.sfx_enabled = enabled;
	
	if(!enabled) {
		self.settings->audio.click_enabled = 0;
		self.settings->audio.hover_enabled = 0;
		
		if(self.click_chk) {
			GUI_WidgetSetState(self.click_chk, 0);
			GUI_WidgetSetEnabled(self.click_chk, 0);
		}
		if(self.hover_chk) {
			GUI_WidgetSetState(self.hover_chk, 0);
			GUI_WidgetSetEnabled(self.hover_chk, 0);
		}
	}
	else {
		self.settings->audio.click_enabled = 1;
		self.settings->audio.hover_enabled = 1;
		
		if(self.click_chk) {
			GUI_WidgetSetState(self.click_chk, 1);
			GUI_WidgetSetEnabled(self.click_chk, 1);
		}
		if(self.hover_chk) {
			GUI_WidgetSetState(self.hover_chk, 1);
			GUI_WidgetSetEnabled(self.hover_chk, 1);
		}
	}
}

void SettingsApp_ToggleClick(GUI_Widget *widget) {
	self.settings->audio.click_enabled = GUI_WidgetGetState(widget);
}

void SettingsApp_ToggleHover(GUI_Widget *widget) {
	self.settings->audio.hover_enabled = GUI_WidgetGetState(widget);
}

void SettingsApp_VolumeChange(GUI_Widget *widget) {
	
	if(self.volume_knob && self.volume_label) {
		int knob_pos = GUI_ScrollBarGetHorizontalPosition(self.volume_knob);
		int volume = (knob_pos * 255) / 180;
		char buf[8];
		
		if(volume < 0) volume = 0;
		if(volume > 255) volume = 255;
		
		int was_zero = (self.settings->audio.volume == 0);
		int is_zero = (volume == 0);
		
		if(!was_zero && is_zero) {
			self.prev_sfx_enabled = self.settings->audio.sfx_enabled;
			self.prev_click_enabled = self.settings->audio.click_enabled;
			self.prev_hover_enabled = self.settings->audio.hover_enabled;
			self.prev_startup_enabled = self.settings->audio.startup_enabled;
			
			self.settings->audio.sfx_enabled = 0;
			self.settings->audio.click_enabled = 0;
			self.settings->audio.hover_enabled = 0;
			self.settings->audio.startup_enabled = 0;
			
			if(self.sfx_chk) {
				GUI_WidgetSetState(self.sfx_chk, 0);
				GUI_WidgetSetEnabled(self.sfx_chk, 0);
			}
			if(self.click_chk) {
				GUI_WidgetSetState(self.click_chk, 0);
				GUI_WidgetSetEnabled(self.click_chk, 0);
			}
			if(self.hover_chk) {
				GUI_WidgetSetState(self.hover_chk, 0);
				GUI_WidgetSetEnabled(self.hover_chk, 0);
			}
			if(self.startup_chk) {
				GUI_WidgetSetState(self.startup_chk, 0);
				GUI_WidgetSetEnabled(self.startup_chk, 0);
			}
		}
		else if(was_zero && !is_zero) {
			self.settings->audio.sfx_enabled = self.prev_sfx_enabled;
			self.settings->audio.click_enabled = self.prev_click_enabled;
			self.settings->audio.hover_enabled = self.prev_hover_enabled;
			self.settings->audio.startup_enabled = self.prev_startup_enabled;
			
			if(self.sfx_chk) {
				GUI_WidgetSetState(self.sfx_chk, self.settings->audio.sfx_enabled);
				GUI_WidgetSetEnabled(self.sfx_chk, 1);
			}
			if(self.startup_chk) {
				GUI_WidgetSetState(self.startup_chk, self.settings->audio.startup_enabled);
				GUI_WidgetSetEnabled(self.startup_chk, 1);
			}
			if(self.click_chk) {
				GUI_WidgetSetState(self.click_chk, self.settings->audio.click_enabled);
				GUI_WidgetSetEnabled(self.click_chk, self.settings->audio.sfx_enabled);
			}
			if(self.hover_chk) {
				GUI_WidgetSetState(self.hover_chk, self.settings->audio.hover_enabled);
				GUI_WidgetSetEnabled(self.hover_chk, self.settings->audio.sfx_enabled);
			}
		}
		
		self.settings->audio.volume = volume;
		sprintf(buf, "%d", volume);
		GUI_LabelSetText(self.volume_label, buf);
	}
}

void SettingsApp_TimezoneChange(GUI_Widget *widget) {
	const char *object_name = GUI_ObjectGetName((GUI_Object *)widget);

	if(!strcmp(object_name, "tz-plus") || !strcmp(object_name, "tz-minus")) {
        if (!strcmp(object_name, "tz-plus")) {
		    self.settings->time_zone += 30;
        }
		else {
		    self.settings->time_zone -= 30;
        }
	}
	else if(!strcmp(object_name, "timezone")) {
		const char *text = GUI_TextEntryGetText(widget);
		int hours = 0, minutes = 0;
		float value_f = 0;
		const char *p = text;
        int parsed_tz;

		if (strlen(p) == 0) {
			parsed_tz = 0;
		}
		else if (strchr(p, ':')) {
			sscanf(p, "%d:%d", &hours, &minutes);

			if (hours < 0 || (hours == 0 && strchr(p, '-'))) {
				parsed_tz = hours * 60 - minutes;
			}
			else {
				parsed_tz = hours * 60 + minutes;
			}
		}
		else {
			value_f = atof(p);
			parsed_tz = (int)(value_f * 60);
		}
        self.settings->time_zone = parsed_tz;
	}

	int clamped_tz = self.settings->time_zone;

	if(clamped_tz > 14 * 60) {
		clamped_tz = 14 * 60;
	}
	if(clamped_tz < -12 * 60) {
		clamped_tz = -12 * 60;
	}

	if (strcmp(object_name, "timezone") != 0 || self.settings->time_zone != clamped_tz) {
		self.settings->time_zone = clamped_tz;
		SetupTimezoneSettings();
	}
}
