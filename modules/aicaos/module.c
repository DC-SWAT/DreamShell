/* DreamShell ##version##

   module.c - aicaos module
   Copyright (C)2013-2014 SWAT 

*/          

#include "ds.h"
#include "aicaos.h"
#include "drivers/aica.h"


DEFAULT_MODULE_EXPORTS_CMD(aicaos, "AICAOS boot manager");

int builtin_aicaos_cmd(int argc, char *argv[]) { 
	
    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
					"Options: \n"
					" -i, --init       -Load and initialize AICAOS\n"
					" -s, --shutdown   -Shutdown AICAOS and initialize KOS driver\n"
					" -v, --version    -Display version\n\n"
					"Arguments: \n"
					" -f, --file       -Firmware file\n\n"
					"Example: %s -i -f aicaos.drv\n", argv[0], argv[0]);
        return CMD_NO_ARG; 
    }

	int a_init = 0, a_shut = 0, a_ver = 0;
	char *file = NULL;
	char fn[MAX_FN_LEN];

	struct cfg_option options[] = {
		{"init",     'i', NULL, CFG_BOOL, (void *) &a_init,  0},
		{"shutdown", 's', NULL, CFG_BOOL, (void *) &a_shut,  0},
		{"version",  'v', NULL, CFG_BOOL, (void *) &a_ver,   0},
		{"file",     'f', NULL, CFG_STR,  (void *) &file,    0},
		CFG_END_OF_LIST
	};
	
	CMD_DEFAULT_ARGS_PARSER(options);
		
	if(a_ver) {
		ds_printf("%s module version: %d.%d.%d build %d\n", 
					lib_get_name(), VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
		return CMD_OK;
	}
	
	if(a_shut) {
		ds_printf("DS_PROCESS: Initializing default KOS driver...\n");
		aica_exit();
		snd_init();
		return CMD_OK;
	}
	
	if(a_init) {
		
		if(file == NULL) {
			sprintf(fn, "%s/firmware/aica/aicaos.drv", getenv("PATH"));
			file = (char*)&fn;
		}
		
		ds_printf("DS_PROCESS: Loading '%s' in to AICA SPU...\n", file);
		snd_shutdown();
		a_init = aica_init(file);
		
		if(a_init) {
			ds_printf("DS_ERROR: Failed %i.\n", a_init);
			return CMD_ERROR;
		}
		
		return CMD_OK;
	}

	return CMD_OK;
}

