/* DreamShell ##version##

   module.c - DreamEye app module
   Copyright (C) 2023 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>

DEFAULT_MODULE_EXPORTS(app_dreameye);

static struct {

	App_t *app;
	GUI_Widget *pages;

} self;

void DreamEyeApp_Init(App_t *app) {
	self.app = app;
	self.pages = APP_GET_WIDGET("pages");
}

void DreamEyeApp_ShowPage(GUI_Widget *widget) {
}

void DreamEyeApp_ShowPreview(GUI_Widget *widget) {
	const char *object_name = GUI_ObjectGetName((GUI_Object *)widget);
	maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CAMERA);
	int isp_mode = DREAMEYE_ISP_MODE_SIF;

	if(!dev) {
		ds_printf("DS_ERROR: No camera present\n");
		return;
	}

	if(object_name[8] == '1') {
		isp_mode = DREAMEYE_ISP_MODE_QSIF;
	}

	dreameye_preview_init(dev, isp_mode);
}
