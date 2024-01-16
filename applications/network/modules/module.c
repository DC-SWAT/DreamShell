/* DreamShell ##version##

   module.c - Network app module
   Copyright (C) 2024 SWAT 
*/

#include <ds.h>
#include <stdbool.h>

DEFAULT_MODULE_EXPORTS(app_network);

static struct {

    App_t *app;
    GUI_Widget *pages;

} self;


void NetworkApp_Init(App_t *app) {
    self.app = app;
    self.pages = APP_GET_WIDGET("pages");
}

void NetworkApp_Shutdown(App_t *app) {
    (void)app;
}

void NetworkApp_Open(App_t *app) {
    (void)app;
}

static void ExecCommand(const char *str) {
    ShowConsole();
    dsystem(str);
    thd_sleep(1000);
    HideConsole();
}

void NetworkApp_ConnectBBA(GUI_Widget *widget) {
    (void)widget;
    ExecCommand("net --init");
}

void NetworkApp_ConnectModem(GUI_Widget *widget) {
    (void)widget;
    ExecCommand("ppp --init");
}

void NetworkApp_FTP(GUI_Widget *widget) {
    (void)widget;
    ExecCommand("ftpd -s -p 21 -d /");
}

void NetworkApp_HTTP(GUI_Widget *widget) {
    (void)widget;
    ExecCommand("httpd -s -p 80");
}
