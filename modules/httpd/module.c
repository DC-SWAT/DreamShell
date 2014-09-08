/* DreamShell ##version##

   module.c - httpd module
   Copyright (C)2012-2013 SWAT 

*/

#include "ds.h"
#include "network/http.h"

DEFAULT_MODULE_EXPORTS_CMD(httpd, "DreamShell http server");

int builtin_httpd_cmd(int argc, char *argv[]) { 
	
    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
	              "Options: \n"
				  " -s, --start     	-Start server\n"
				  " -t, --stop   		-Stop server\n\n"
	              "Arguments: \n"
				  " -p, --port      	-Server listen port (default 80)\n\n"
	              "Example: %s --start\n\n", argv[0], argv[0]);
        return CMD_NO_ARG; 
    }
    
    int start_srv = 0, stop_srv = 0, port = 0;

	struct cfg_option options[] = {
		{"start",  	's', NULL, CFG_BOOL, (void *) &start_srv, 0},
		{"stop",  	't', NULL, CFG_BOOL, (void *) &stop_srv, 0},
		{"port", 	'p', NULL, CFG_INT,  (void *) &port, 0},
		CFG_END_OF_LIST
	};
	
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(start_srv) {
		httpd_init(port);
		return CMD_OK;
	}
	
	if(stop_srv) {
		httpd_shutdown();
		return CMD_OK;
	}
	
	return CMD_NO_ARG;
}
