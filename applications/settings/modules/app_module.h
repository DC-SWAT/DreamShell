/* DreamShell ##version##

   app_module.h - Settings app module header
   Copyright (C)2016-2025 SWAT
*/

#include "ds.h"

void SettingsApp_Init(App_t *app);

void SettingsApp_ShowPage(GUI_Widget *widget);

void SettingsApp_ResetSettings(GUI_Widget *widget);

void SettingsApp_Reboot(GUI_Widget *widget);

void SettingsApp_ToggleNativeMode(GUI_Widget *widget);

void SettingsApp_ToggleScreenMode(GUI_Widget *widget);

void SettingsApp_ToggleScreenFilter(GUI_Widget *widget);

void SettingsApp_ToggleApp(GUI_Widget *widget);

void SettingsApp_ToggleRoot(GUI_Widget *widget);

void SettingsApp_ToggleStartup(GUI_Widget *widget);

void SettingsApp_TimeChange(GUI_Widget *widget);

void SettingsApp_Time(GUI_Widget *widget);

void SettingsApp_Time_Clr(GUI_Widget *widget);

void SettingsApp_TimezoneChange(GUI_Widget *widget);

void SettingsApp_TimezoneClr(GUI_Widget *widget);

void SettingsApp_ToggleSfx(GUI_Widget *widget);

void SettingsApp_ToggleClick(GUI_Widget *widget);

void SettingsApp_ToggleHover(GUI_Widget *widget);

void SettingsApp_ToggleStartupSound(GUI_Widget *widget);

void SettingsApp_VolumeChange(GUI_Widget *widget);
