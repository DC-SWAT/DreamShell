/* DreamShell ##version##

   module.c - mongoose module
   Copyright (C)2012-2014 SWAT 

*/
            

#include "ds.h"
#include "network/mongoose.h"

DEFAULT_MODULE_EXPORTS_CMD(mongoose, "Mongoose http server");

/*
static void *callback(enum mg_event event,
                      struct mg_connection *conn,
                      const struct mg_request_info *request_info) {
	if (event == MG_NEW_REQUEST) {
		// Echo requested URI back to the client
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/plain\r\n\r\n"
						"%s", request_info->uri);
		return "";  // Mark as processed
	} else {
		return NULL;
	}
}
*/
static struct mg_context *ctx;

int builtin_mongoose_cmd(int argc, char *argv[]) { 
	
    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
	              "Options: \n"
					" -s, --start      -Start server\n"
					" -t, --stop       -Stop server\n\n"
					"Arguments: \n"
					" -p, --port       -Server listen port (default 80)\n"
					" -r, --root       -Document root (default /)\n"
					" -n, --threads    -Num of threads (default 3)\n\n"
					"Example: %s --start\n\n", argv[0], argv[0]);
        return CMD_NO_ARG; 
    }
    
    int start_srv = 0, stop_srv = 0;
	char *port = NULL, *docroot = NULL, *thds = NULL;
	
	const char *mg_options[] = {
		"listening_ports", "80", 
		"document_root", "/",
		"num_threads", "3",
		NULL
	};

	struct cfg_option options[] = {
		{"start",		's', NULL, CFG_BOOL, (void *) &start_srv, 0},
		{"stop",		't', NULL, CFG_BOOL, (void *) &stop_srv, 0},
		{"port",		'p', NULL, CFG_STR,  (void *) &port, 0},
		{"root",		'r', NULL, CFG_STR,  (void *) &docroot, 0},
		{"threads",	'n', NULL, CFG_STR,  (void *) &thds, 0},
		CFG_END_OF_LIST
	};
	
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(start_srv) {
		ctx = mg_start(/*&callback*/NULL, NULL, mg_options);
		ds_printf("Mongoose web server v. %s started on port(s) %s with web root [%s]\n", 
					mg_version(), 
					mg_get_option(ctx, "listening_ports"), 
					mg_get_option(ctx, "document_root"));
		return CMD_OK;
	}
	
	if(stop_srv) {
		ds_printf("mongoose: Waiting for all threads to finish...");
		mg_stop(ctx);
		return CMD_OK;
	}
	
	return CMD_OK;
}
