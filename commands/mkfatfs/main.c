/* DreamShell ##version##

   main.c - Make FAT volume
   Copyright (C)2014 SWAT 

*/
            
#include "ds.h"
#include "fatfs/ff.h"


static void put_rc(FRESULT rc) {
	const char *p;
	static const char str[] =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0";
	FRESULT i;

	for (p = str, i = 0; i != rc && *p; i++) {
		while(*p++);
	}
	
	ds_printf("DS_ERROR: %u FR_%s\n", (UINT)rc, p);
}


int main(int argc, char *argv[]) { 
     
	if(argc < 2) {
		ds_printf("Usage: %s drive_path(sd=0,g1=1) partition\n"
					"Example: %s 0: 1\n\n", argv[0], argv[0]);
		return CMD_NO_ARG; 
	}

	int partition = atoi(argv[2]);
	FRESULT rc = FR_OK;
	
	ds_printf("DS_PROCESS: Formatting...\n");

	if((rc = f_mkfs((const TCHAR*)argv[1], (BYTE)partition, 0)) != FR_OK) {
		put_rc(rc);
		return CMD_ERROR;
	}

	ds_printf("DS_OK: Format complete.\n");
	return CMD_OK; 
}
