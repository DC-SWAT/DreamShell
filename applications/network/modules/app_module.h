/* DreamShell ##version##

   app_module.h - Network app module header
   Copyright (C) 2024-2025 SWAT
*/

#include <ds.h>

void NetworkApp_Init(App_t *app);

void NetworkApp_Shutdown(App_t *app);

void NetworkApp_Open(App_t *app);

void NetworkApp_ToggleEthernet(GUI_Widget *widget);

void NetworkApp_ToggleModem(GUI_Widget *widget);

void NetworkApp_ToggleFTP(GUI_Widget *widget);

void NetworkApp_ToggleHTTP(GUI_Widget *widget);

void NetworkApp_Exit(GUI_Widget *widget);

void NetworkApp_DialogConfirm(GUI_Widget *widget);

void NetworkApp_DialogCancel(GUI_Widget *widget);

void NetworkApp_ToggleEth(GUI_Widget *widget);

void NetworkApp_TogglePpp(GUI_Widget *widget);
