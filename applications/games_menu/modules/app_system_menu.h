/* DreamShell ##version##

   app_system_menu.h
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __APP_SYSTEM_MENU_H
#define __APP_SYSTEM_MENU_H

#include <ds.h>
#include "tsunami/tsudefinition.h"
#include "tsunami/drawable.h"
#include "tsunami/dsapp.h"
#include "tsunami/drawables/scene.h"

#include "app_utils.h"
#include "app_definition.h"

void CreateSystemMenu(DSApp *dsapp_ptr, Scene *scene_ptr, Font *menu_font, Font *message_font);
void DestroySystemMenu();
void SystemMenuRemoveAll();

void SystemMenuInputEvent(int type, int key);
int StateSystemMenu();
void ShowSystemMenu();
void HideSystemMenu();
void SystemMenuSelectedEvent(Drawable *drawable, uint bottom_index, uint column, uint row);
void OptimizeCoversClick(Drawable *drawable);
void ExitSystemMenuClick(Drawable *drawable);
void ScanMissingCoversClick(Drawable *drawable);
void DefaultSavePresetOptionClick(Drawable *drawable);
void CoverBackgroundOptionClick(Drawable *drawable);

void ShowOptimizeCoverPopup();
void HideOptimizeCoverPopup();
void SetMessageOptimizer(const char *fmt, const char *message);
int StopOptimizeCovers();

void ShowCoverScan();
void HideCoverScan();
void SetMessageScan(const char *fmt, const char *message);
int StopScanCovers();

#endif // __APP_SYSTEM_MENU_H
