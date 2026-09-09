/* DreamShell ##version##

   app_module.h - Cart Ripper app module header
   Copyright (C) 2026 SWAT
*/

#include <ds.h>

void CartRipperApp_Init(App_t *app);
void CartRipperApp_Shutdown(App_t *app);
void CartRipperApp_Open(App_t *app);
void CartRipperApp_StartRip(GUI_Widget *widget);
void CartRipperApp_CancelRip(GUI_Widget *widget);
void CartRipperApp_Refresh(GUI_Widget *widget);
void CartRipperApp_Launch(GUI_Widget *widget);
void CartRipperApp_Gamename(void);
void CartRipperApp_ShowFileBrowser(GUI_Widget *widget);
void CartRipperApp_ShowMainPage(GUI_Widget *widget);
void CartRipperApp_FileBrowserItemClick(dirent_fm_t *fm_ent);
void CartRipperApp_FileBrowserConfirm(GUI_Widget *widget);
