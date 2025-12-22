/* DreamShell ##version##

   module.c - FTP server module
   Copyright (C) 2023-2025 SWAT 
*/

#include <ds.h>
#include <lftpd.h>

DEFAULT_MODULE_HEADER(ftpd);

typedef struct ftpd {
    int port;
    char *dir;
    kthread_t *thread;
    lftpd_t lftpd;
} ftpd_t;

void *ftpd_thread(void *param) {
    ftpd_t *srv = (ftpd_t *)param;
    lftpd_start(srv->dir, srv->port, &srv->lftpd);
    return NULL;
}

static void ftpd_free(void *ptr) {
    ftpd_t *srv = (ftpd_t *)ptr;
    lftpd_stop(&srv->lftpd);
    /* FIXME: "accept" is not aborted on socket close */
    // thd_join(srv->thread, NULL);
    free(srv->dir);
    free(srv);
}

static Item_list_t *servers;

int builtin_ftpd_cmd(int argc, char *argv[]) {

    if(argc < 2) {
        ds_printf("Usage: %s options args\n"
                  "Options: \n"
                  " -s, --start     	-Start FTP server\n"
                  " -t, --stop   		-Stop FTP server\n"
                  " -q, --status        -Check FTP server status\n\n");
        ds_printf("Arguments: \n"
                  " -p, --port      	-Server listen port (default 21)\n"
                  " -d, --dir           -Root directory (default /)\n\n"
                  "Example: %s --start\n\n", argv[0], argv[0]);
        return CMD_NO_ARG; 
    }

    int start_srv = 0, stop_srv = 0, status_srv = 0, port = 21;
    char *dir = NULL;
    Item_t *item;
    ftpd_t *srv;

    struct cfg_option options[] = {
        {"start",  	's', NULL, CFG_BOOL, (void *) &start_srv, 0},
        {"stop",  	't', NULL, CFG_BOOL, (void *) &stop_srv,  0},
        {"status",  'q', NULL, CFG_BOOL, (void *) &status_srv, 0},
        {"port", 	'p', NULL, CFG_INT,  (void *) &port,      0},
		{"dir",     'd', NULL, CFG_STR,  (void *) &dir,       0},
        CFG_END_OF_LIST
    };

    CMD_DEFAULT_ARGS_PARSER(options);

    if(status_srv) {
        if(listGetItemFirst(servers) != NULL) {
            ds_printf("DS_INFO: FTP server is running\n");
            return CMD_OK;
        }
        else {
            ds_printf("DS_INFO: FTP server is not running\n");
            return CMD_ERROR;
        }
    }

    if(start_srv) {

        if(dir == NULL) {
            dir = "/";
        }
        item = listGetItemByName(servers, dir);

        if(item != NULL) {
            srv = (ftpd_t *)item->data;
            ds_printf("DS_PROCESS: Stopping FTP server on port %d, root %s\n",
                srv->port, srv->dir);
            listRemoveItem(servers, item, (listFreeItemFunc *)ftpd_free);
        }

        ds_printf("DS_PROCESS: Starting FTP server on port %d, root %s\n", port, dir);
        srv = (ftpd_t *)malloc(sizeof(ftpd_t));

        if(srv == NULL) {
            ds_printf("DS_ERROR: Out of memory\n");
            return CMD_ERROR;
        }

        srv->port = port;
        srv->dir = strdup(dir);
        srv->thread = thd_create(0, ftpd_thread, (void *)srv);
        listAddItem(servers, LIST_ITEM_USERDATA, srv->dir, srv, sizeof(ftpd_t));
        return CMD_OK;
    }
    if(stop_srv) {

        if(dir == NULL) {
            dir = "/";
        }
	    item = listGetItemByName(servers, dir);

        if(item != NULL) {
            srv = (ftpd_t *)item->data;
            ds_printf("DS_PROCESS: Stopping FTP server on port %d, root %s\n",
                srv->port, srv->dir);
            listRemoveItem(servers, item, (listFreeItemFunc *)ftpd_free);
        } else {
            ds_printf("DS_ERROR: FTP server with root %s is not running\n");
        }
        return CMD_OK;
    }
    return CMD_NO_ARG;
}

int lib_open(klibrary_t * lib) {
    servers = listMake();
    AddCmd(lib_get_name(), "FTP server", (CmdHandler *) builtin_ftpd_cmd);
    return nmmgr_handler_add(&ds_ftpd_hnd.nmmgr);
}

int lib_close(klibrary_t * lib) {
    listDestroy(servers, (listFreeItemFunc *)ftpd_free);
    RemoveCmd(GetCmdByName(lib_get_name()));
    return nmmgr_handler_remove(&ds_ftpd_hnd.nmmgr);
}
