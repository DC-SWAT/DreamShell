/* DreamShell

   app_module.h - Launch App module header
   Copyright (C) 2026 SWAT

*/

#include <app.h>
#include <tsunami/tsunami.h>

void LaunchApp_Init(App_t *app);
void LaunchApp_Open(App_t *app);
void LaunchApp_Close(App_t *app);
void LaunchApp_Shutdown(App_t *app);
void LaunchAppDeleteConfirm(Drawable *drawable);
void LaunchAppDeleteCancel(Drawable *drawable);
