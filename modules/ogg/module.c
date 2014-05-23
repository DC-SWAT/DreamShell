/* DreamShell ##version##

   module.c - oggvorbis module
   Copyright (C)2009-2014 SWAT 

*/
            
#include "ds.h"
#include <oggvorbis/sndoggvorbis.h>

DEFAULT_MODULE_HEADER(oggvorbis);

static int oggvorbis_inited = 0;

static int builtin_oggvorbis(int argc, char *argv[]) { 
     
    if(argc == 1) {
        ds_printf("Usage: %s option args...\n\n"
                  "Options: \n"
                  " -p, --play     -Start playing\n"
                  " -s, --stop     -Stop playing\n\n", argv[0]);
        ds_printf("Arguments: \n"
                  " -l, --loop     -Loop N times\n"
                  " -i, --info     -Show song info\n"
                  " -v, --volume   -Set volume\n"
                  " -f, --file     -File for playing\n\n"
                  "Examples: %s --play --file /cd/file.ogg\n"
                  "          %s -s", argv[0], argv[0]);
        return CMD_NO_ARG; 
    } 

	int start = 0, stop = 0, loop = 0, volume = 0, info = 0;
	char *file = NULL;

	struct cfg_option options[] = {
		{"play",   'p', NULL, CFG_BOOL, (void *) &start,  0},
		{"stop",   's', NULL, CFG_BOOL, (void *) &stop,   0},
		{"info",   'i', NULL, CFG_BOOL, (void *) &info,   0},
		{"loop",   'l', NULL, CFG_INT,  (void *) &loop,   0},
		{"volume", 'v', NULL, CFG_INT,  (void *) &volume, 0},
		{"file",   'f', NULL, CFG_STR,  (void *) &file,   0},
		CFG_END_OF_LIST
	};
  
	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(!oggvorbis_inited) {
		//snd_stream_init();
		sndoggvorbis_init(); 
		oggvorbis_inited = 1;           
	}

	if(volume) 
		sndoggvorbis_volume(volume); 
	
	if(start) {
       if(file == NULL) {
          ds_printf("DS_ERROR: Need file for playing\n");
          return CMD_ERROR;
       }

       sndoggvorbis_stop();
	   
       if(sndoggvorbis_start(file, loop) < 0) {
          ds_printf("DS_ERROR: Can't play file: %s\n", file);
          return CMD_ERROR; 
       }
    }
    
	if(stop) 
		sndoggvorbis_stop();

	if(info) {

		long bitrate = sndoggvorbis_getbitrate();
		ds_printf(" Artist:  %s\n", sndoggvorbis_getartist()); 
		ds_printf(" Title:   %s\n", sndoggvorbis_gettitle()); 
		ds_printf(" Genre:   %s\n", sndoggvorbis_getgenre());
		ds_printf(" Bitrate: %ld\n", bitrate); 
	} 

	if(!start && !stop && !volume) {
		ds_printf("DS_ERROR: There is no option.\n");
		return CMD_NO_ARG;
	} else {
		return CMD_OK; 
	}
}


int lib_open(klibrary_t *lib) {
	AddCmd(lib_get_name(), "Oggvorbis player", (CmdHandler *) builtin_oggvorbis); 
	return nmmgr_handler_add(&ds_oggvorbis_hnd.nmmgr);
}


int lib_close(klibrary_t *lib) {
	RemoveCmd(GetCmdByName(lib_get_name()));
	if(oggvorbis_inited) 
		sndoggvorbis_shutdown();
	return nmmgr_handler_remove(&ds_oggvorbis_hnd.nmmgr); 
}
