/* DreamShell ##version##

   app_module.h - Memtest app module header
   Copyright (C) 2026 SWAT
*/

#include "ds.h"

void MemtestApp_Init(App_t *app);
void MemtestApp_Shutdown(App_t *app);
void MemtestApp_Open(App_t *app);
void MemtestApp_Start(GUI_Widget *widget);
void MemtestApp_Stop(GUI_Widget *widget);
void MemtestApp_DialogConfirm(GUI_Widget *widget);
