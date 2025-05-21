/* DreamShell ##version##

   app_system_menu.h
   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __APP_SYSTEM_MENU_H
#define __APP_SYSTEM_MENU_H

#include <ds.h>
#include "tsunami/tsudefinition.h"
#include "tsunami/drawable.h"
#include "tsunami/dsapp.h"
#include "tsunami/drawables/scene.h"
#include "tsunami/drawables/form.h"

#include "app_utils.h"
#include "app_definition.h"

void SaveSystemMenuConfig();
void CreateSystemMenu(DSApp *dsapp_ptr, Scene *scene_ptr, Font *menu_font, Font *message_font, void (*RefreshMainView)(), void (*ReloadPage)());
void CreateSystemMenuView(Form *form_ptr);
void CreateStyleView(Form *form_ptr);
void CreateCacheView(Form *form_ptr);
void OnSystemViewIndexChangedEvent(Drawable *drawable, int view_index);
void DestroySystemMenu();
void SystemMenuRemoveAll();
void EnableColorControls(bool enable);
void SetAllThemeProperties(const char *theme);
void SetColorPropertyValue(TextBox *textbox, Color *property_color, uint32 current_color);

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
void ChangePageWithPadOptionClick(Drawable *drawable);
void StartInLastGameOptionClick(Drawable *drawable);

void CategoryOptionClick(Drawable *drawable);
void CategoryInputEvent(int type, int key);

void ThemeOptionClick(Drawable *drawable);
void ThemeInputEvent(int type, int key);

void BackgroundColorOptionClick(Drawable *drawable);
void BackgroundColorInputEvent(int type, int key);

void BorderColorOptionClick(Drawable *drawable);
void BorderColorInputEvent(int type, int key);

void TitleColorOptionClick(Drawable *drawable);
void TitleColorInputEvent(int type, int key);

void BodyColorOptionClick(Drawable *drawable);
void BodyColorInputEvent(int type, int key);

void AreaColorOptionClick(Drawable *drawable);
void AreaColorInputEvent(int type, int key);

void ControlTopColorOptionClick(Drawable *drawable);
void ControlTopColorInputEvent(int type, int key);

void ControlBodyColorOptionClick(Drawable *drawable);
void ControlBodyColorInputEvent(int type, int key);

void ControlBottomColorOptionClick(Drawable *drawable);
void ControlBottomColorInputEvent(int type, int key);

void CacheGamesOptionClick(Drawable *drawable);
void RebuildCacheClick(Drawable *drawable);

void ShowOptimizeCoverPopup();
void HideOptimizeCoverPopup();
void SetMessageOptimizer(const char *fmt, const char *message);
int StopOptimizeCovers();

void ShowCoverScan();
void HideCoverScan();
void SetMessageScan(const char *fmt, const char *message);
int StopScanCovers();

#endif // __APP_SYSTEM_MENU_H
