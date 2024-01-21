/* DreamShell ##version##

   module.c - Network app module
   Copyright (C) 2024 SWAT 
*/

#include <ds.h>
#include <stdbool.h>

DEFAULT_MODULE_EXPORTS(app_network);

typedef enum {
    APP_ACTION_IDLE,
    APP_ACTION_CONNECT_BBA,
    APP_ACTION_CONNECT_MODEM,
    APP_ACTION_CONNECT_FTPD,
    APP_ACTION_CONNECT_HTTPD
} app_action_t;

static struct {

    App_t *app;
    char *last_ip;
    app_action_t action;

    GUI_Widget *pages;

    GUI_Widget *ip_addr;
    GUI_Widget *net_status;
    GUI_Widget *ftpd_status;
    GUI_Widget *httpd_status;

} self;

static void *app_thread(void *params) {
    (void)params;

    while(self.app->state & APP_STATE_OPENED) {
        switch(self.action) {
            case APP_ACTION_CONNECT_BBA:
                if(dsystem("net --init") == CMD_OK) {
                    GUI_LabelSetTextColor(self.ip_addr, 51, 51, 51);
                    GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
                    GUI_LabelSetText(self.net_status, "Connected");
                }
                else {
                    GUI_LabelSetText(self.net_status, "Failed");
                }
                GUI_LabelSetText(self.ip_addr, getenv("NET_IPV4"));
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_CONNECT_MODEM:
                if(dsystem("ppp --init") == CMD_OK) {
                    GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
                    GUI_LabelSetText(self.net_status, "Connected");
                }
                else {
                    GUI_LabelSetText(self.net_status, "Failed");
                }
                GUI_LabelSetText(self.ip_addr, getenv("NET_IPV4"));
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_CONNECT_FTPD:
                if(dsystem("ftpd -s -p 21 -d /") == CMD_OK) {
                    GUI_LabelSetTextColor(self.ftpd_status, 18, 172, 16);
                    GUI_LabelSetText(self.ftpd_status, "Started, port 21");
                }
                else {
                    GUI_LabelSetText(self.ftpd_status, "Failed");
                }
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_CONNECT_HTTPD:
                if(dsystem("httpd -s -p 80") == CMD_OK) {
                    GUI_LabelSetTextColor(self.httpd_status, 18, 172, 16);
                    GUI_LabelSetText(self.httpd_status, "Started, port 80");
                }
                else {
                    GUI_LabelSetText(self.httpd_status, "Failed");
                }
                self.action = APP_ACTION_IDLE;
                break;
            default:
            case APP_ACTION_IDLE:
                thd_sleep(100);
                break;
        }
    }
    ds_printf("NETWORK: Exit thread\n");
    return NULL;
}


void NetworkApp_Init(App_t *app) {

    memset(&self, 0, sizeof(self));
    self.app = app;

    self.pages = APP_GET_WIDGET("pages");
    self.ip_addr = APP_GET_WIDGET("ip-addr");
    self.net_status = APP_GET_WIDGET("net-status");
    self.ftpd_status = APP_GET_WIDGET("ftpd-status");
    self.httpd_status = APP_GET_WIDGET("httpd-status");
}

void NetworkApp_Shutdown(App_t *app) {
    (void)app;
}

void NetworkApp_Open(App_t *app) {
    (void)app;
    char *ip = getenv("NET_IPV4");
    if(strncmp(ip, "0.0.0.0", 7)) {
        GUI_LabelSetTextColor(self.ip_addr, 51, 51, 51);
        GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
        GUI_LabelSetText(self.net_status, "Connected");
    }
    GUI_LabelSetText(self.ip_addr, ip);
    self.app->thd = thd_create(0, app_thread, NULL);
}

void NetworkApp_ConnectBBA(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
    GUI_LabelSetTextColor(self.ip_addr, 207, 51, 17);
    GUI_LabelSetText(self.net_status, "Connecting...");
    self.action = APP_ACTION_CONNECT_BBA;
}

void NetworkApp_ConnectModem(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
    GUI_LabelSetTextColor(self.ip_addr, 207, 51, 17);
    GUI_LabelSetText(self.net_status, "Connecting...");
    self.action = APP_ACTION_CONNECT_MODEM;
}

void NetworkApp_FTP(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.ftpd_status, 207, 51, 17);
    GUI_LabelSetText(self.ftpd_status, "Starting...");
    self.action = APP_ACTION_CONNECT_FTPD;
}

void NetworkApp_HTTP(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.httpd_status, 207, 51, 17);
    GUI_LabelSetText(self.httpd_status, "Starting...");
    self.action = APP_ACTION_CONNECT_HTTPD;
}
