/* DreamShell ##version##

   module.c - DreamEye app module
   Copyright (C) 2023, 2024 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>

DEFAULT_MODULE_EXPORTS(app_dreameye);

static struct {

	App_t *app;
	GUI_Widget *pages;

	int preview_isp_mode;
	int preview_fullscreen;
	int preview_x;
	int preview_y;
	int preview_scale;

} self;

static void ShowCameraPreview() {
	maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);
	dreameye_preview_shutdown();
	if(dev) {
		dreameye_preview_init(dev, self.preview_isp_mode,
			self.preview_fullscreen, self.preview_scale,
			self.preview_x, self.preview_y);
	} else {
		ds_printf("DS_ERROR: No camera present\n");
	}
}

void DreamEyeApp_Init(App_t *app) {
	self.app = app;
	self.pages = APP_GET_WIDGET("pages");

	self.preview_isp_mode = DREAMEYE_ISP_MODE_QSIF;
	self.preview_fullscreen = 0;
	self.preview_scale = 2;
	self.preview_x = 160;
	self.preview_y = 220;
}

void DreamEyeApp_Shutdown(App_t *app) {
	(void)app;
	dreameye_preview_shutdown();
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
		self.preview_isp_mode = DREAMEYE_ISP_MODE_QSIF;
		self.preview_scale = 2;
	} else {
		self.preview_isp_mode = DREAMEYE_ISP_MODE_SIF;
		self.preview_scale = 1;
	}

	ShowCameraPreview();
}
