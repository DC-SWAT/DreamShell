/* DreamShell ##version##

   module.c - mpg123 module
   Copyright (C)2009-2014 SWAT 

*/
            
#include "ds.h"
#include "audio/mp3.h"

DEFAULT_MODULE_HEADER(mpg123);

static int builtin_mpg123(int argc, char *argv[]) { 
     
    if(argc == 1) {
        ds_printf("Usage: %s option args...\n\n"
                  "Options: \n"
                  " -p, --play     -Start playing\n"
                  " -s, --stop     -Stop playing\n"
                  " -t, --restart  -Restart playing\n"
                  " -a, --pause    -Pause playing\n"
                  " -o, --forward  -Fast forward\n"
                  " -e, --rewind   -Rewind\n\n"
                  "Arguments: \n"
                  " -l, --loop     -Loop N times (not supported yet)\n"
                  " -f, --file     -File for playing\n\n"
                  "Examples: %s --play --file /cd/file.mp3\n"
                  "          %s -s", argv[0], argv[0], argv[0]);
        return CMD_NO_ARG; 
    } 

    int start = 0, stop = 0, loop = 0, forward = 0, 
		rewind = 0, restart = 0, pause = 0;
    char *file = NULL;
  
	struct cfg_option options[] = {
		{"play",    'p', NULL, CFG_BOOL, (void *) &start,   0},
		{"stop",    's', NULL, CFG_BOOL, (void *) &stop,    0},
		{"forward", 'o', NULL, CFG_BOOL, (void *) &forward, 0},
		{"rewind",  'e', NULL, CFG_BOOL, (void *) &rewind,  0},
		{"restart", 't', NULL, CFG_BOOL, (void *) &restart, 0},
		{"pause",   'a', NULL, CFG_BOOL, (void *) &pause,   0},
		{"loop",    'l', NULL, CFG_INT,  (void *) &loop,    0},
		{"file",    'f', NULL, CFG_STR,  (void *) &file,    0},
		CFG_END_OF_LIST
	};
  
	CMD_DEFAULT_ARGS_PARSER(options);

	if(start) {
		
		if(file == NULL) {
			ds_printf("DS_ERROR: Need file for playing\n");
			return CMD_ERROR;
		}

		sndmp3_stop();

		if(sndmp3_start(file, loop) < 0) {
			ds_printf("DS_ERROR: Maybe bad or unsupported MP3 file: %s\n", file);
			return CMD_ERROR; 
		}
		
		return CMD_OK;
    }
    
	if(stop) {
		sndmp3_stop();
		return CMD_OK; 
	}
	if(forward)  {
		sndmp3_fastforward();
		return CMD_OK; 
	}
	if(rewind)  {
		sndmp3_rewind();
		return CMD_OK; 
	}
	if(restart)  {
		sndmp3_restart();
		return CMD_OK; 
	}
	if(pause)  {
		sndmp3_pause();
		return CMD_OK; 
	}
	
	return CMD_NO_ARG;
} 

int lib_open(klibrary_t * lib) {
	AddCmd(lib_get_name(), "MP1/MP2/MP3 player", (CmdHandler *) builtin_mpg123); 
	return nmmgr_handler_add(&ds_mpg123_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	RemoveCmd(GetCmdByName(lib_get_name()));
	sndmp3_stop();
	return nmmgr_handler_remove(&ds_mpg123_hnd.nmmgr); 
}

