/* DreamShell ##version##

   app_module.h - GD Ripper app module header
   Copyright (C)2014 megavolt85

*/

#include "ds.h"

void gd_ripper_toggleSavedevice(GUI_Widget *widget);

void gd_ripper_Number_read();

void gd_ripper_Gamename();

void gd_ripper_ipbin_name();

void gd_ripper_Init(App_t *app, const char* fileName);

void gd_ripper_StartRip(GUI_Widget *widget);

void gd_ripper_CancelRip(GUI_Widget *widget);

int create_gdi_file(char *dst_folder,char *dst_file, char *text, int disc_type);

void gd_ripper_Exit();

void gd_ripper_ShowFileBrowser(GUI_Widget *widget);

void gd_ripper_ShowMainPage(GUI_Widget *widget);

void gd_ripper_FileBrowserItemClick(GUI_Widget *widget);

void gd_ripper_FileBrowserConfirm(GUI_Widget *widget);

void gd_ripper_ShowFileBrowser(GUI_Widget *widget);
