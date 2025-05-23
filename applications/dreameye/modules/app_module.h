/* DreamShell ##version##

   app_module.h - Dreameye app module header
   Copyright (C) 2023, 2024 SWAT
*/

#include <ds.h>

void DreameyeApp_Init(App_t *app);

void DreameyeApp_Shutdown(App_t *app);

void DreameyeApp_Open(App_t *app);

void DreameyeApp_ShowMainPage(GUI_Widget *widget);

void DreameyeApp_ShowPhotoPage(GUI_Widget *widget);

void DreameyeApp_ExportPhoto(GUI_Widget *widget);

void DreameyeApp_ErasePhoto(GUI_Widget *widget);

void DreameyeApp_FileBrowserItemClick(GUI_Widget *widget);

void DreameyeApp_FileBrowserConfirm(GUI_Widget *widget);

void DreameyeApp_ChangeResolution(GUI_Widget *widget);

void DreameyeApp_ToggleDetectQR(GUI_Widget *widget);

void DreameyeApp_ToggleExecQR(GUI_Widget *widget);

void DreameyeApp_Abort(GUI_Widget *widget);
