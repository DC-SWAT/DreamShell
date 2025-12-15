/* DreamShell ##version##

   module.h - Main app module header
   Copyright (C)2023-2025 SWAT 

*/

#include <ds.h>

void MainApp_SlideLeft(GUI_Widget *widget);
void MainApp_SlideRight(GUI_Widget *widget);
void MainApp_Init(App_t *app);
void MainApp_Shutdown(App_t *app);
void MainApp_ShortcutDeleteConfirm(GUI_Widget *widget);
void MainApp_ShortcutDeleteCancel(GUI_Widget *widget);
