/* DreamShell ##version##

   module.c - zip module
   Copyright (C)2009-2014 SWAT 
*/
            
#include "ds.h"

DEFAULT_MODULE_HEADER(zip);

int builtin_zip_cmd(int argc, char *argv[]);
int builtin_unzip_cmd(int argc, char *argv[]);

int lib_open(klibrary_t *lib) {
	AddCmd(lib_get_name(), "zip archiver", (CmdHandler *) builtin_zip_cmd);
	AddCmd("unzip", "unzip archives", (CmdHandler *) builtin_unzip_cmd);
	return nmmgr_handler_add(&ds_zip_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
	RemoveCmd(GetCmdByName(lib_get_name()));
	RemoveCmd(GetCmdByName("unzip"));
	return nmmgr_handler_remove(&ds_zip_hnd.nmmgr); 
}
