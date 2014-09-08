/* DreamShell ##version##

   module.c - http module
   Copyright (C)2011-2014 SWAT 

*/

#include "ds.h"
#include "network/http.h"

DEFAULT_MODULE_HEADER(http);

int lib_open(klibrary_t *lib) {
	tcpfs_init();
	httpfs_init();
	return nmmgr_handler_add(&ds_http_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
	httpfs_shutdown();
	tcpfs_shutdown();
	return nmmgr_handler_remove(&ds_http_hnd.nmmgr); 
}
