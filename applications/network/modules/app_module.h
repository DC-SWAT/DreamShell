/* DreamShell ##version##

   app_module.h - Network app module header
   Copyright (C) 2024-2025 SWAT
*/

#include <ds.h>

void NetworkApp_Init(App_t *app);

void NetworkApp_Shutdown(App_t *app);

void NetworkApp_Open(App_t *app);

void NetworkApp_ConnectBBA(GUI_Widget *widget);

void NetworkApp_ConnectModem(GUI_Widget *widget);

void NetworkApp_FTP(GUI_Widget *widget);

void NetworkApp_HTTP(GUI_Widget *widget);

void NetworkApp_Exit(GUI_Widget *widget);

void NetworkApp_DialogConfirm(GUI_Widget *widget);

void NetworkApp_DialogCancel(GUI_Widget *widget);
