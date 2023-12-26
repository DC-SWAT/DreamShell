/* DreamShell ##version##

   module.c - QR-code module
   Copyright (C) 2023 SWAT
*/

#include <ds.h>
#include <quirc.h>

DEFAULT_MODULE_HEADER(quirc);

int builtin_quirc_cmd(int argc, char *argv[]) {
    return CMD_NO_ARG;
}

int lib_open(klibrary_t * lib) {
    // AddCmd(lib_get_name(), "QR-code recognition", (CmdHandler *) builtin_quirc_cmd);
    return nmmgr_handler_add(&ds_quirc_hnd.nmmgr);
}

int lib_close(klibrary_t * lib) {
    // RemoveCmd(GetCmdByName(lib_get_name()));
    return nmmgr_handler_remove(&ds_quirc_hnd.nmmgr);
}
