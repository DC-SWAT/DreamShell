/* DreamShell ##version##

   module.c - s3m module
   Copyright (C)2009-2014 SWAT

*/
            
#include "ds.h"
#include "audio/s3m.h"

DEFAULT_MODULE_EXPORTS_CMD(s3m, "S3M Player");

int builtin_s3m_cmd(int argc, char *argv[]) { 
     
    if(argc < 2) {
        ds_printf("Usage: %s option args...\n\n"
                  "Options: \n"
                  " -p, --play     -Start playing\n"
                  " -s, --stop     -Stop playing\n\n"
                  "Arguments: \n"
                  " -f, --file     -File for playing\n\n"
                  "Examples: %s --play --file /cd/file.s3m\n"
                  "          %s -s", argv[0], argv[0], argv[0]);
        return CMD_NO_ARG; 
    } 

	int start = 0, stop = 0, stat = 0;
	char *file = NULL;
  
	struct cfg_option options[] = {
		{"play",   'p', NULL, CFG_BOOL, (void *) &start,  0},
		{"stop",   's', NULL, CFG_BOOL, (void *) &stop,   0},
		{"file",   'f', NULL, CFG_STR,  (void *) &file,   0},
		CFG_END_OF_LIST
	};
  
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(start) {
		
		if(file == NULL) {
			ds_printf("DS_ERROR: Need file for playing\n");
			return CMD_ERROR;
		}

		stat = s3m_play(file);

		if(stat != S3M_SUCCESS) {
			switch (stat) {
				case S3M_ERROR_IO:
					ds_printf("DS_ERROR: Unabled to open s3m file\n");
					return CMD_ERROR;
				case S3M_ERROR_MEM:
					ds_printf("DS_ERROR: s3m file exceeds available SRAM\n");
					return CMD_ERROR;
				default:
					ds_printf("DS_ERROR: s3m uknown error.\n");
					return CMD_ERROR;
			}
		}
	}
    
	if(stop)
		s3m_stop();

	if(!start && !stop) {
		ds_printf("DS_ERROR: There is no option.\n");
		return CMD_NO_ARG;
	} 

	return CMD_OK; 
}

