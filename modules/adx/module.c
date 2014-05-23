/* DreamShell ##version##

   module.c - adx module
   Copyright (C)2011-2014 SWAT 

*/
            
#include "ds.h"
#include "audio/adx.h" /* ADX Decoder Library */
#include "audio/snddrv.h" /* Direct Access to Sound Driver */

DEFAULT_MODULE_HEADER(adx);

static int builtin_adx(int argc, char *argv[]) { 
     
	if(argc < 2) {
		ds_printf("Usage: %s option args...\n\n"
					"Options: \n"
					" -p, --play     -Start playing\n"
					" -s, --stop     -Stop playing\n"
					" -e, --pause    -Pause playing\n"
					" -r, --resume   -Resume playing\n\n", argv[0]);
		ds_printf("Arguments: \n"
					" -l, --loop     -Loop N times\n"
					" -f, --file     -File for playing\n\n"
					"Examples: %s --play --file /cd/file.adx\n"
					"          %s -s", argv[0], argv[0]);
		return CMD_NO_ARG; 
	} 

	int start = 0, stop = 0, loop = 0, pause = 0, resume = 0, restart = 0;
	char *file = NULL;

	struct cfg_option options[] = {
		{"play",   'p', NULL, CFG_BOOL, (void *) &start,  0},
		{"stop",   's', NULL, CFG_BOOL, (void *) &stop,   0},
		{"pause",  'e', NULL, CFG_BOOL, (void *) &pause,  0},
		{"resume", 'r', NULL, CFG_BOOL, (void *) &resume, 0},
		{"restart",'t', NULL, CFG_BOOL, (void *) &restart,0},
		{"loop",   'l', NULL, CFG_INT,  (void *) &loop,   0},
		{"file",   'f', NULL, CFG_STR,  (void *) &file,   0},
		CFG_END_OF_LIST
	};
  
	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(start) {
		if(file == NULL) {
			ds_printf("DS_ERROR: Need file for playing\n");
			return CMD_ERROR;
		}
		
		adx_stop();

		/* Start the ADX stream, with looping enabled */
		if(adx_dec(file, loop) < 1 ) {
			ds_printf("DS_ERROR: Invalid ADX file\n");
			return CMD_ERROR;
		}

		/* Wait for the stream to start */
		while(snddrv.drv_status == SNDDRV_STATUS_NULL)
			thd_pass(); 
	}
    
	if(stop) {
		if(adx_stop())
			ds_printf(" ADX streaming stopped\n");
	}
	
	if(restart) {
		if(adx_restart())
			ds_printf(" ADX streaming restarted\n");
	}
	
	if(pause) {
		if(adx_pause())
			ds_printf("ADX streaming paused\n");
	}

	if(resume) {
		if(adx_resume())
			ds_printf("ADX streaming resumed\n");
	}
    
    if(!start && !stop && !pause && !resume) {
        ds_printf("DS_ERROR: There is no option.\n");
        return CMD_NO_ARG;
    } else {
        return CMD_OK; 
    }
}

int lib_open(klibrary_t * lib) {
	AddCmd(lib_get_name(), "ADX player", (CmdHandler *) builtin_adx); 
	return nmmgr_handler_add(&ds_adx_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	RemoveCmd(GetCmdByName(lib_get_name()));
	adx_stop();
	return nmmgr_handler_remove(&ds_adx_hnd.nmmgr); 
}
