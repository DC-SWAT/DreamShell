/* Region Changer ##version##

   flashrom.c
   Copyright (C)2007-2014 SWAT
*/

#include "rc.h"

int flash_clear(int block) {
	int size, start;

	if (flashrom_info(block, &start, &size) < 0) {
		ds_printf("DS_ERROR: Get offset error\n");
		return -1;
	}

	if (flashrom_delete(start) < 0) {
		ds_printf("DS_ERROR: Deleting old flash factory data error\n");
		return -1;
	}
	
	return 0;
}

int flash_write_factory(uint8 *data) {
	int size, start;

	if (flashrom_info(FLASHROM_PT_SYSTEM, &start, &size) < 0) {
		printf("Get offset error\n");
		return -1;
	}

	if (flashrom_delete(start) < 0) {
		ds_printf("DS_ERROR: Deleting old flash factory data error\n");
		return -1;
	}

	if (flashrom_write(start, data, size) < 0) {
		ds_printf("DS_ERROR: Write new flash factory data error\n");
		return -1;
	}

	return 0;
}

uint8 *flash_read_factory() {
	int size, start;
	uint8 *data;

	if (flashrom_info(FLASHROM_PT_SYSTEM, &start, &size) < 0) {
		
		size = 8192;
		data = (uint8 *) malloc(size);
		
		if (data == NULL) {
			ds_printf("DS_ERROR: Not enough of memory\n");
			return NULL;
		}
		
		memcpy(data, (uint8 *)0x0021A000, size);
		
	} else {
		
		data = (uint8 *) malloc(size);
		
		if (data == NULL) {
			ds_printf("DS_ERROR: Not enough of memory\n");
			return NULL;
		}
		
		if (flashrom_read(start, data, size) < 0) {
			ds_printf("DS_ERROR: Flash read lash factory failed\n");
			return NULL;
		}
	}
	
	return data;
}

/* Read file to RAM and write to flashrom */
int flash_write_file(const char *filename) {
   	
	file_t fd;
	uint8 *data = NULL;
	size_t size;
	
	fd = fs_open(filename, O_RDONLY);
	
	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}
	
	size = fs_total(fd);
	
	if (size > 0x20000) {
		size = 0x20000;
	}
	
	data = (uint8*) memalign(32, size);
	
	if (data == NULL) {
		ds_printf("DS_ERROR: Not enough of memory\n");
		goto error;
	}
	
	if (fs_read(fd, data, size) < 0) {
		ds_printf("DS_ERROR: Error in reading file: %d\n", errno);
		goto error;
	}
	
	if (flashrom_write(0, data, size) < 0) {
		ds_printf("DS_ERROR: Write new flash data error\n");
		goto error;
	}
	
	fs_close(fd);
	free(data);
	return 0;
	
error:
	fs_close(fd);
	if(data)
		free(data);
	return -1;
}

/* Read flashrom to RAM and write to file */
int flash_read_file(const char *filename) {
   	
	file_t fd;
	uint8 *data = NULL;
	size_t size = 0x20000;
	
	fd = fs_open(filename, O_WRONLY);
	
	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}
	
	data = (uint8*) memalign(32, size);
	
	if (data == NULL) {
		ds_printf("DS_ERROR: Not enough of memory\n");
		goto error;
	}
	
	if (flashrom_read(0, data, size) < 0) {
		ds_printf("DS_ERROR: Read flash data error\n");
		goto error;
	}
	
	if (fs_write(fd, data, size) < 0) {
		ds_printf("DS_ERROR: Error in writing to file: %d\n", errno);
		goto error;
	}
	
	fs_close(fd);
	free(data);
	return 0;
	
error:
	fs_close(fd);
	if(data)
		free(data);
	return -1;
}

