/* DreamShell ##version##

   flashrom.c
   Copyright (C) 2007-2014 SWAT
*/

#include "ds.h"

static const char * langs[] = {
	"Invalid",
	"Japanese",
	"English",
	"German",
	"French",
	"Spanish",
	"Italian"
};

int main(int argc, char *argv[]) {

	uint8 *buffer;
	int start, size, i, part;
	flashrom_syscfg_t sc;
	file_t ff;

	if(argc == 1) {
		ds_printf("Usage: flashrom -flag arguments(if needed)...\n"
					"Flags Implements: \n"
					" -read      - FLASHROM_READ syscall\n"
					" -write     - FLASHROM_WRITE syscall\n"
					" -del       - FLASHROM_DELETE syscall\n"
					" -inf       - FLASHROM_INFO syscall\n"
					" -ps        - Print settings\n\n"
					" -help      - Print usages help\n"); 
		return CMD_NO_ARG; 
	} 


	if (!strcmp(argv[1], "-help")) {
		ds_printf("Flag usages: \n"
					" -read offset size to_file\n"
					" -write offset from_file\n"
					" -del offset\n"
					" -inf part(0-4)\n"
					" -ps\n"
					" -help\n\n"
					"Parts(for -inf): \n"
					" 0 - Factory settings (read-only, 8K)\n"
					" 1 - Reserved (all 0s, 8K)\n"
					" 2 - Block allocated (16K)\n"
					" 3 - Game settings (block allocated, 32K)\n"
					" 4 - Block allocated (64K)\n"); 
		return CMD_OK; 
	} 


	if (!strcmp(argv[1], "-read")) {

		start = atoi(argv[2]);
		size = atoi(argv[3]);
		buffer = (uint8 *)malloc(size);
		
		if(buffer == NULL) {
			ds_printf("DS_ERROR: No free memory\n"); 
			return CMD_ERROR; 
		}

		if(flashrom_read(start, buffer, size)  < 0) {
			ds_printf("DS_ERROR: Can't read part %d\n", start);
			return CMD_ERROR;
		}

		ff = fs_open(argv[4], O_CREAT | O_WRONLY);

		if(ff == FILEHND_INVALID) {
			ds_printf("DS_ERROR: Can't open %s\n", argv[4]);
			return CMD_ERROR;
		}

		fs_write(ff, buffer, size);
		fs_close(ff);
		free(buffer);
		return CMD_OK;
	} 

	if (!strcmp(argv[1], "-write")) {
		 
		start = atoi(argv[2]);
		ff = fs_open(argv[3], O_RDONLY);

		if(ff == FILEHND_INVALID) {
			ds_printf("DS_ERROR: Can't open %s\n", argv[3]);
			return CMD_ERROR;
		}

		size = fs_total(ff);
		buffer = (uint8 *)malloc(size);
		
		if(buffer == NULL) {
			ds_printf("DS_ERROR: No free memory\n");
			return CMD_ERROR;
		}
		
		fs_read(ff, buffer, size);
		flashrom_delete(start);

		if(flashrom_write(start, buffer, size) < 0) {
			ds_printf("DS_ERROR: Can't write part %d\n", start); 
			return CMD_ERROR; 
		}
		
		fs_close(ff);
		free(buffer);
		return CMD_OK;
	}

	if (!strcmp(argv[1], "-del")) {
		
		start = atoi(argv[2]); 
		flashrom_delete(start); 
		ds_printf("DS_INF: Complete.\n");
		return CMD_OK; 
	} 


	if (!strcmp(argv[1], "-inf")) {
		
		part = atoi(argv[2]); 
		flashrom_info(part, &start, &size); 
		ds_printf("Part:    %d\n", part); 
		ds_printf("Offset:  %d\n", start); 
		ds_printf("Size:    %12d\n", size); 
		return CMD_OK; 
	} 


	if (!strcmp(argv[1], "-ps")) {

		ds_printf("\nDS_PROCESS: Reading flashrom...\n");

		if (flashrom_info(0, &start, &size) < 0) {
			ds_printf("DS_ERROR: Couldn't get the start/size of partition 0\n");
			return CMD_ERROR;
		}

		buffer = (uint8 *)malloc(size);
		
		if(buffer == NULL) {
			ds_printf("DS_ERROR: No free memory\n");
			return CMD_ERROR;
		}
		
		if (flashrom_read(start, buffer, size) < 0) {
			ds_printf("DS_ERROR: Couldn't read partition 0\n");
			free(buffer);
			return CMD_ERROR;
		}

		ds_printf("DS: Your flash header: '");

		for (i=0; i < 16; i++) {
			ds_printf("%c", buffer[i]);
		}

		ds_printf("'\nDS: Again in hex:\n");
		
		for (i=0; i < 16; i++) {
			ds_printf("%02x ", buffer[i]);
		}

		if (!memcmp(buffer, "00000", 5)) {
			ds_printf("This appears to be a Japanese DC.\n");
		} else if (!memcmp(buffer, "00110", 5)) {
			ds_printf("This appears to be a USA DC.\n");
		} else if (!memcmp(buffer, "00211", 5)) {
			ds_printf("This appears to be a European DC.\n");
		} else {
			ds_printf("DS_ERROR: I don't know what the region of this DC is.\n");
		}

		free(buffer);

		if (flashrom_get_syscfg(&sc) < 0) {
			ds_printf("DS_ERROR: flashrom_get_syscfg failed\n");
			return CMD_ERROR;
		}

		i = sc.language;
		
		if (i > FLASHROM_LANG_ITALIAN)
			i = -1;
			
		ds_printf("Your selected language is %s.\n", langs[i+1]);

		i = sc.audio;
		ds_printf("Your audio is set to %s.\n",i ? "stereo" : "mono");

		i = sc.autostart;
		ds_printf("Your auto-start is set to %s.\n",i ? "on" : "off");
		return CMD_OK;
	}
	
	return CMD_OK;
}

