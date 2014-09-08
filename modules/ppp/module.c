/* DreamShell ##version##

   module.c - PPP module
   Copyright (C)2014 SWAT 

*/          

#include "ds.h"
#include <ppp/ppp.h>

DEFAULT_MODULE_EXPORTS_CMD(ppp, "PPP connection manager");


int builtin_ppp_cmd(int argc, char *argv[]) {
	
    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
	              "Options: \n"
				  " -i, --init       -Initialize PPP connection\n"
				  " -s, --shut       -Shutdown PPP connection\n"
				  " -B, --blind      -Blind dial (don't wait for a dial tone)\n\n", argv[0]);
		ds_printf("Arguments: \n"
				  " -b, --bps        -Baudrate for scif\n"
				  " -n, --number     -Number to dial\n"
				  " -u, --username   -Username (dream by default)\n"
				  " -p, --password   -Password (shell by default)\n"
				  " -d, --device     -Device, scif or modem (default)\n\n");
        return CMD_NO_ARG; 
    }
    
	int rc, init_ppp = 0, shut_ppp = 0;
	int conn_rate = 0, blind = 0, bps = 115200;
	char *number = "123", *username = "dream", *password = "shell";
	char *device = "modem";

	struct cfg_option options[] = {
		{"init",     'i', NULL, CFG_BOOL, (void *) &init_ppp, 0},
		{"shut",     's', NULL, CFG_BOOL, (void *) &shut_ppp, 0},
		{"bps",      'b', NULL, CFG_INT,  (void *) &bps,      0},
		{"blind",    'B', NULL, CFG_BOOL, (void *) &blind,    0},
		{"number",   'n', NULL, CFG_STR,  (void *) &number,   0},
		{"username", 'u', NULL, CFG_STR,  (void *) &username, 0},
		{"password", 'p', NULL, CFG_STR,  (void *) &password, 0},
		{"device",   'd', NULL, CFG_STR,  (void *) &device,   0},
		CFG_END_OF_LIST
	};
	
	CMD_DEFAULT_ARGS_PARSER(options);

	if(init_ppp) {
		
		ppp_init();
		
		if(!strncasecmp(device, "scif", 4)) {
			rc = ppp_scif_init(bps);
		} else {
			rc = ppp_modem_init(number, blind, &conn_rate);
		}
		
		if(rc < 0) {
			ds_printf("DS_ERROR: Modem PPP initialization failed!\n");
			return CMD_ERROR;
		}

		ppp_set_login(username, password);
		rc = ppp_connect();

		if(rc < 0) {
			ds_printf("DS_ERROR: Link establishment failed!\n");
			return CMD_ERROR;
		}

		ds_printf("DS_OK: Link established, rate is %d bps\n", conn_rate);
		return CMD_OK;
	}

	if(shut_ppp) {
		ppp_shutdown();
		ds_printf("DS_OK: Complete!\n");
		return CMD_OK;
	}
	
	return CMD_NO_ARG;
}

