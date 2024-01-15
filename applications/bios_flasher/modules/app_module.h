/* DreamShell ##version##

   app_module.h - Bios flasher app module header
   Copyright (C)2013 Yev
   Copyright (C)2013, 2014, 2023, 2024 SWAT

*/

#include "ds.h"

void BiosFlasher_ItemClick(dirent_fm_t *fm_ent);

void BiosFlasher_OnWritePressed(GUI_Widget *widget);

void BiosFlasher_OnReadPressed(GUI_Widget *widget);

void BiosFlasher_OnComparePressed(GUI_Widget *widget);

void BiosFlasher_OnDetectPressed(GUI_Widget *widget);

void BiosFlasher_OnBackPressed(GUI_Widget *widget);

void BiosFlasher_OnSettingsPressed(GUI_Widget *widget);

void BiosFlasher_OnConfirmPressed(GUI_Widget *widget);

void BiosFlasher_OnSaveSettingsPressed(GUI_Widget *widget);

void BiosFlasher_OnSupportedPressed(GUI_Widget *widget);

void BiosFlasher_OnExitPressed(GUI_Widget *widget);

void BiosFlasher_Init(App_t* app);
