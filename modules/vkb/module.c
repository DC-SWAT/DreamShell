/* DreamShell ##version##
   module.c - virtual keyboard module
   Copyright (C)2004 - 2014 SWAT
*/
#include "ds.h"

DEFAULT_MODULE_HEADER(vkb);

int lib_open(klibrary_t *lib) {
	if(VirtKeyboardInit() < 0) {
		ds_printf("DS_ERROR: Can't initialize virtual keyboard.\n");
	}
	return nmmgr_handler_add(&ds_vkb_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
	VirtKeyboardShutdown();
	return nmmgr_handler_remove(&ds_vkb_hnd.nmmgr);
}
