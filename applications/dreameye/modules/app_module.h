/* DreamShell ##version##

   app_module.h - DreamEye app module header
   Copyright (C) 2023, 2024 SWAT
*/

#include <ds.h>

void DreamEyeApp_Init(App_t *app);

void DreamEyeApp_Shutdown(App_t *app);

void DreamEyeApp_Open(App_t *app);

void DreamEyeApp_ShowMainPage(GUI_Widget *widget);

void DreamEyeApp_ShowPhotoPage(GUI_Widget *widget);

void DreamEyeApp_ExportPhoto(GUI_Widget *widget);

void DreamEyeApp_ErasePhoto(GUI_Widget *widget);

void DreamEyeApp_FileBrowserItemClick(GUI_Widget *widget);

void DreamEyeApp_FileBrowserConfirm(GUI_Widget *widget);

void DreamEyeApp_ChangeResolution(GUI_Widget *widget);

void DreamEyeApp_ToggleDetectQR(GUI_Widget *widget);

void DreamEyeApp_ToggleExecQR(GUI_Widget *widget);

void DreamEyeApp_Abort(GUI_Widget *widget);
