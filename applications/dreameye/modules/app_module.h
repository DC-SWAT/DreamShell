/* DreamShell ##version##

   app_module.h - Dreameye app module header
   Copyright (C) 2023-2025 SWAT
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

void DreameyeApp_ShowGalleryPage(GUI_Widget *widget);

void DreameyeApp_GalleryPrevPage(GUI_Widget *widget);

void DreameyeApp_GalleryNextPage(GUI_Widget *widget);

void DreameyeApp_ViewPhoto(GUI_Widget *widget);

void DreameyeApp_ViewPrevPhoto(GUI_Widget *widget);

void DreameyeApp_ViewNextPhoto(GUI_Widget *widget);

void DreameyeApp_DeleteCurrentPhoto(GUI_Widget *widget);

void DreameyeApp_ExportCurrentPhoto(GUI_Widget *widget);

void DreameyeApp_CancelDelete(GUI_Widget *widget);

void DreameyeApp_CancelDeleteFromViewer(GUI_Widget *widget);

void DreameyeApp_ConfirmDelete(GUI_Widget *widget);

void DreameyeApp_ShowFullscreenPhoto(GUI_Widget *widget);

void DreameyeApp_ExitFullscreen(GUI_Widget *widget);

void DreameyeApp_CancelExport(GUI_Widget *widget);

void DreameyeApp_ChangeBpp(GUI_Widget *widget);
