/* DreamShell ##version##

   app_module.h - Settings app module header
   Copyright (C)2016-2023 SWAT
*/

#include "ds.h"

void SettingsApp_Init(App_t *app);

void SettingsApp_ShowPage(GUI_Widget *widget);

void SettingsApp_ResetSettings(GUI_Widget *widget);

void SettingsApp_ToggleNativeMode(GUI_Widget *widget);

void SettingsApp_ToggleScreenMode(GUI_Widget *widget);

void SettingsApp_ToggleScreenFilter(GUI_Widget *widget);

void SettingsApp_ToggleApp(GUI_Widget *widget);

void SettingsApp_ToggleRoot(GUI_Widget *widget);

void SettingsApp_ToggleStartup(GUI_Widget *widget);

void SettingsApp_TimeChange(GUI_Widget *widget);

void SettingsApp_Time(GUI_Widget *widget);

void SettingsApp_Time_Clr(GUI_Widget *widget);
