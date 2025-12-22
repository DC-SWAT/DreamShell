/* DreamShell ##version##

   module.c - Network app module
   Copyright (C) 2024-2025 SWAT 
*/

#include <ds.h>
#include <stdbool.h>
#include <kos/net.h>

#include "app_module.h"

DEFAULT_MODULE_EXPORTS(app_network);

typedef enum {
    APP_ACTION_IDLE,
    APP_ACTION_ETHERNET,
    APP_ACTION_MODEM,
    APP_ACTION_FTPD,
    APP_ACTION_HTTPD
} app_action_t;

static struct {

    App_t *app;
    char *last_ip;
    app_action_t action;
    Settings_t *settings;
    bool is_eth_connected;
    bool is_ppp_connected;
    bool is_ftpd_started;
    bool is_httpd_started;

    GUI_Widget *pages;
    GUI_Widget *dialog;

    GUI_Widget *ip_addr;
    GUI_Widget *net_status;
    GUI_Widget *ftpd_status;
    GUI_Widget *httpd_status;
    GUI_Widget *startup_connect_eth_but;
    GUI_Widget *startup_connect_ppp_but;
    GUI_Widget *startup_ntp_but;
    GUI_Widget *keep_running_but;

    GUI_Widget *eth_but_txt;
    GUI_Widget *ppp_but_txt;
    GUI_Widget *ftpd_but_txt;
    GUI_Widget *httpd_but_txt;

    Item_list_t *loaded_modules;

} self;

static void SetupNetworkSettings() {
    if(self.startup_connect_eth_but) {
        GUI_WidgetSetState(self.startup_connect_eth_but, self.settings->network.startup_connect_eth);
    }
    if(self.startup_connect_ppp_but) {
        GUI_WidgetSetState(self.startup_connect_ppp_but, self.settings->network.startup_connect_ppp);
    }
    if(self.startup_ntp_but) {
        GUI_WidgetSetState(self.startup_ntp_but, self.settings->network.startup_ntp);
    }
    if(self.keep_running_but) {
        GUI_WidgetSetState(self.keep_running_but, self.is_ftpd_started || self.is_httpd_started);
    }
}

static void UpdateButtons() {
    if(self.is_eth_connected) {
        GUI_LabelSetText(self.eth_but_txt, "Disconnect Ethernet");
    }
    else {
        GUI_LabelSetText(self.eth_but_txt, "Connect Ethernet");
    }

    if(self.is_ppp_connected) {
        GUI_LabelSetText(self.ppp_but_txt, "Disconnect Modem");
    }
    else {
        GUI_LabelSetText(self.ppp_but_txt, "Connect Modem");
    }

    if(self.is_ftpd_started) {
        GUI_LabelSetText(self.ftpd_but_txt, "Stop FTP Server");
    }
    else {
        GUI_LabelSetText(self.ftpd_but_txt, "Start FTP Server");
    }

    if(self.is_httpd_started) {
        GUI_LabelSetText(self.httpd_but_txt, "Stop HTTP Server");
    }
    else {
        GUI_LabelSetText(self.httpd_but_txt, "Start HTTP Server");
    }
    GUI_WidgetMarkChanged(self.pages);
}

static void *app_thread(void *params) {
    (void)params;

    while(self.app->state & APP_STATE_OPENED) {
        switch(self.action) {
            case APP_ACTION_ETHERNET:
                if(self.is_eth_connected) {
                    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
                    self.is_eth_connected = false;

                    if(dsystem("net --shutdown") == CMD_OK) {
                        GUI_LabelSetText(self.net_status, "Disconnected");
                    }
                    else {
                        GUI_LabelSetText(self.net_status, "Failed");
                    }
                }
                else if(dsystem("net --init") == CMD_OK) {
                    GUI_LabelSetTextColor(self.ip_addr, 51, 51, 51);
                    GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
                    GUI_LabelSetText(self.net_status, "Connected");
                    self.is_eth_connected = true;
                }
                else {
                    GUI_LabelSetText(self.net_status, "Failed");
                    self.is_eth_connected = false;
                }
                GUI_LabelSetText(self.ip_addr, getenv("NET_IPV4"));
                UpdateButtons();
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_MODEM:
                if(self.is_ppp_connected) {
                    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
                    self.is_ppp_connected = false;

                    if(dsystem("ppp --shutdown") == CMD_OK) {
                        GUI_LabelSetText(self.net_status, "Disconnected");
                    }
                    else {
                        GUI_LabelSetText(self.net_status, "Failed");
                    }
                }
                else if(dsystem("ppp --init") == CMD_OK) {
                    GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
                    GUI_LabelSetText(self.net_status, "Connected");
                    self.is_ppp_connected = true;
                }
                else {
                    GUI_LabelSetText(self.net_status, "Failed");
                }
                GUI_LabelSetText(self.ip_addr, getenv("NET_IPV4"));
                UpdateButtons();
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_FTPD:
                if(self.is_ftpd_started) {
                    GUI_LabelSetTextColor(self.ftpd_status, 207, 51, 17);
                    self.is_ftpd_started = false;

                    if(dsystem("ftpd -t") == CMD_OK) {
                        GUI_LabelSetText(self.ftpd_status, "Stopped");
                        self.is_ftpd_started = false;
                    }
                    else {
                        GUI_LabelSetText(self.ftpd_status, "Failed");
                    }
                }
                else if(dsystem("ftpd -s -p 21 -d /") == CMD_OK) {
                    GUI_LabelSetTextColor(self.ftpd_status, 18, 172, 16);
                    GUI_LabelSetText(self.ftpd_status, "Started, port 21");
                    self.is_ftpd_started = true;
                }
                else {
                    GUI_LabelSetText(self.ftpd_status, "Failed");
                }
                UpdateButtons();
                self.action = APP_ACTION_IDLE;
                break;
            case APP_ACTION_HTTPD:
                if(self.is_httpd_started) {
                    GUI_LabelSetTextColor(self.httpd_status, 207, 51, 17);
                    self.is_httpd_started = false;

                    if(dsystem("httpd -t") == CMD_OK) {
                        GUI_LabelSetText(self.httpd_status, "Stopped");
                    }
                    else {
                        GUI_LabelSetText(self.httpd_status, "Failed");
                    }
                }
                else if(dsystem("httpd -s -p 80") == CMD_OK) {
                    GUI_LabelSetTextColor(self.httpd_status, 18, 172, 16);
                    GUI_LabelSetText(self.httpd_status, "Started, port 80");
                    self.is_httpd_started = true;
                }
                else {
                    GUI_LabelSetText(self.httpd_status, "Failed");
                }
                UpdateButtons();
                self.action = APP_ACTION_IDLE;
                break;
            default:
            case APP_ACTION_IDLE:
                thd_sleep(100);
                break;
        }
    }
    return NULL;
}

static void LoadModuleSafe(const char *name) {
    if(!GetModuleByName(name)) {
        char path[NAME_MAX];
        snprintf(path, sizeof(path), "%s/modules/%s.klf", getenv("PATH"), name);

        if(OpenModule(path)) {
            listAddItem(self.loaded_modules, LIST_ITEM_MODULE, name, NULL, 0);
        }
    }
}

void NetworkApp_Init(App_t *app) {

    memset(&self, 0, sizeof(self));
    self.app = app;
    self.settings = GetSettings();

    self.pages = APP_GET_WIDGET("pages");
    self.dialog = APP_GET_WIDGET("dialog");
    self.ip_addr = APP_GET_WIDGET("ip-addr");
    self.net_status = APP_GET_WIDGET("net-status");
    self.ftpd_status = APP_GET_WIDGET("ftpd-status");
    self.httpd_status = APP_GET_WIDGET("httpd-status");
    self.startup_connect_eth_but = APP_GET_WIDGET("startup-connect-eth-but");
    self.startup_connect_ppp_but = APP_GET_WIDGET("startup-connect-ppp-but");
    self.startup_ntp_but = APP_GET_WIDGET("startup-ntp-but");
    self.keep_running_but = APP_GET_WIDGET("keep-running-but");

    self.eth_but_txt = APP_GET_WIDGET("eth-but-txt");
    self.ppp_but_txt = APP_GET_WIDGET("ppp-but-txt");
    self.ftpd_but_txt = APP_GET_WIDGET("ftpd-but-txt");
    self.httpd_but_txt = APP_GET_WIDGET("httpd-but-txt");

    self.loaded_modules = listMake();

    LoadModuleSafe("ppp");
    LoadModuleSafe("ftpd");
    LoadModuleSafe("httpd");

    char *ip = getenv("NET_IPV4");

    if(net_default_dev && (net_default_dev->flags & NETIF_RUNNING)) {

        GUI_LabelSetTextColor(self.ip_addr, 51, 51, 51);
        GUI_LabelSetTextColor(self.net_status, 18, 172, 16);
        GUI_LabelSetText(self.net_status, "Connected");

        if(strncmp(net_default_dev->name, "ppp", 3) == 0) {
            self.is_ppp_connected = true;
        }
        else {
            self.is_eth_connected = true;
        }

        if(dsystem("ftpd -q") == CMD_OK) {
            GUI_LabelSetTextColor(self.ftpd_status, 18, 172, 16);
            GUI_LabelSetText(self.ftpd_status, "Started, port 21");
            self.is_ftpd_started = true;
        }
        if(dsystem("httpd -q") == CMD_OK) {
            GUI_LabelSetTextColor(self.httpd_status, 18, 172, 16);
            GUI_LabelSetText(self.httpd_status, "Started, port 80");
            self.is_httpd_started = true;
        }
    }
    GUI_LabelSetText(self.ip_addr, ip);
    UpdateButtons();
    SetupNetworkSettings();
}

void NetworkApp_Shutdown(App_t *app) {
    (void)app;

    if(!GUI_WidgetGetState(self.keep_running_but)) {

        if(self.is_httpd_started) {
            dsystem("httpd -t");
        }
        if(listGetItemByName(self.loaded_modules, "httpd")) {
            CloseModule(GetModuleByName("httpd"));
        }

        if(self.is_ftpd_started) {
            dsystem("ftpd -t");
        }
        if(listGetItemByName(self.loaded_modules, "ftpd")) {
            CloseModule(GetModuleByName("ftpd"));
        }

        bool ppp_active = false;
        if(net_default_dev && (net_default_dev->flags & NETIF_RUNNING)) {
             if(strncmp(net_default_dev->name, "ppp", 3) == 0) {
                 ppp_active = true;
             }
        }

        if(!ppp_active && listGetItemByName(self.loaded_modules, "ppp")) {
            CloseModule(GetModuleByName("ppp"));
        }
    }
    listDestroy(self.loaded_modules, NULL);
}

static int SettingsChanged() {
    int startup_connect_eth = GUI_WidgetGetState(self.startup_connect_eth_but);
    int startup_connect_ppp = GUI_WidgetGetState(self.startup_connect_ppp_but);
    int startup_ntp = GUI_WidgetGetState(self.startup_ntp_but);

    return self.settings->network.startup_connect_eth != startup_connect_eth ||
           self.settings->network.startup_connect_ppp != startup_connect_ppp ||
           self.settings->network.startup_ntp != startup_ntp;
}

void NetworkApp_Exit(GUI_Widget *widget) {
    if (SettingsChanged()) {
        GUI_DialogShow(self.dialog, DIALOG_MODE_CONFIRM, "Saving settings", "Do you want to save changed settings?");
    }
    else {
        dsystem("app -o -n Main");
    }
}

void NetworkApp_DialogConfirm(GUI_Widget *widget) {
    GUI_DialogShow(widget, DIALOG_MODE_INFO, "Saving settings", "Please wait, saving settings to device...");
    self.settings->network.startup_connect_eth = GUI_WidgetGetState(self.startup_connect_eth_but);
    self.settings->network.startup_connect_ppp = GUI_WidgetGetState(self.startup_connect_ppp_but);
    self.settings->network.startup_ntp = GUI_WidgetGetState(self.startup_ntp_but);
    SaveSettings();
    NetworkApp_DialogCancel(widget);
}

void NetworkApp_DialogCancel(GUI_Widget *widget) {
    GUI_DialogHide(widget);
    dsystem("app -o -n Main");
}

void NetworkApp_Open(App_t *app) {
    app->thd = thd_create(0, app_thread, NULL);
}

void NetworkApp_ToggleEthernet(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
    GUI_LabelSetTextColor(self.ip_addr, 207, 51, 17);
    GUI_LabelSetText(self.net_status, "Connecting...");
    self.action = APP_ACTION_ETHERNET;
}

void NetworkApp_ToggleModem(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.net_status, 207, 51, 17);
    GUI_LabelSetTextColor(self.ip_addr, 207, 51, 17);
    GUI_LabelSetText(self.net_status, "Connecting...");
    self.action = APP_ACTION_MODEM;
}

void NetworkApp_ToggleFTP(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.ftpd_status, 207, 51, 17);
    GUI_LabelSetText(self.ftpd_status, "Starting...");
    self.action = APP_ACTION_FTPD;
}

void NetworkApp_ToggleHTTP(GUI_Widget *widget) {
    (void)widget;
    GUI_LabelSetTextColor(self.httpd_status, 207, 51, 17);
    GUI_LabelSetText(self.httpd_status, "Starting...");
    self.action = APP_ACTION_HTTPD;
}

void NetworkApp_ToggleEth(GUI_Widget *widget) {
    (void)widget;
    if(GUI_WidgetGetState(self.startup_connect_eth_but)) {
        GUI_WidgetSetState(self.startup_connect_ppp_but, 0);
    }
}

void NetworkApp_TogglePpp(GUI_Widget *widget) {
    (void)widget;
    if(GUI_WidgetGetState(self.startup_connect_ppp_but)) {
        GUI_WidgetSetState(self.startup_connect_eth_but, 0);
    }
}
