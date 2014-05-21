/* DreamShell ##version##

   module.c - bzip2 module
   Copyright (C)2009-2014 SWAT
*/

#include "ds.h"
#include "bzlib.h"

DEFAULT_MODULE_EXPORTS_CMD(bzip2, "bzip2 archiver");

int builtin_bzip2_cmd(int argc, char *argv[]) {
    
	if(argc < 3) {
		ds_printf("Usage: %s option infile outfile\n"
					"Options: \n"
					" -9      -Compress with mode from 0 to 9\n"
					" -d      -Decompress\n\n"
					"Examples: %s -9 /cd/file.dat /ram/file.dat.bz2\n"
					"          %s -d /ram/file.dat.bz2 /ram/file.dat\n", argv[0], argv[0], argv[0]);
		return CMD_NO_ARG; 
	} 

	char dst[MAX_FN_LEN];
	char buff[512];
	int len = 0;
	file_t fd;
	BZFILE *zfd;

	if(argc < 4) {

		char tmp[MAX_FN_LEN];
		relativeFilePath_wb(tmp, argv[2], strrchr(argv[2], '/'));
		sprintf(dst, "%s.bz2", tmp);

	} else {
		strcpy(dst, argv[3]); 
	}
    
	if(!strcmp("-d", argv[1])) {

		ds_printf("DS_PROCESS: Decompressing '%s' ...\n", argv[2]);
		zfd = BZ2_bzopen(argv[2], "rb");

		if(zfd == NULL) {
			ds_printf("DS_ERROR: Can't create file: %s\n", argv[2]);
			return CMD_ERROR;
		}

		fd = fs_open(dst, O_WRONLY | O_CREAT);

		if(fd < 0) {
			ds_printf("DS_ERROR: Can't open file: %s\n", dst);
			BZ2_bzclose(zfd);
			return CMD_ERROR;
		}

		while((len = BZ2_bzread(zfd, buff, sizeof(buff))) > 0) {
			fs_write(fd, buff, len);
		}

		BZ2_bzclose(zfd);
		fs_close(fd);
		ds_printf("DS_OK: File decompressed.");

	} else if(argv[1][1] >= '0' && argv[1][1] <= '9') {

		char mode[3];
		sprintf(mode, "wb%c", argv[1][1]);
		ds_printf("DS_PROCESS: Compressing '%s' with mode '%c' ...\n", argv[2], argv[1][1]);

		zfd = BZ2_bzopen(dst, mode);

		if(zfd == NULL) {
			ds_printf("DS_ERROR: Can't create file: %s\n", dst);
			return CMD_ERROR;
		}

		fd = fs_open(argv[2], O_RDONLY);

		if(fd < 0) {
			ds_printf("DS_ERROR: Can't open file: %s\n", argv[2]);
			BZ2_bzclose(zfd);
			return CMD_ERROR;
		}

		while((len = fs_read(fd, buff, sizeof(buff))) > 0) {

			if(!BZ2_bzwrite(zfd, buff, len)) {
				ds_printf("DS_ERROR: Error writing to file: %s\n", dst);
				BZ2_bzclose(zfd);
				fs_close(fd);
				return CMD_ERROR;
			}
		}

		BZ2_bzclose(zfd);
		fs_close(fd);
		ds_printf("DS_OK: File compressed.");

	} else {
		return CMD_NO_ARG;
	}

	return CMD_OK;
}
