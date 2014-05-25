/* Region Changer ##version##

   flashrom.c
   Copyright (C)2007-2014 SWAT
*/

#include "rc.h"

int _flashrom_write(int offset, void * buffer, int bytes) {
	int	(*sc)(int, void*, int, int);
	int	old, rv;

	old = irq_disable();
	*((uint32 *)&sc) = *((uint32 *)0x8c0000b8);
	rv = sc(offset, buffer, bytes, 2);
	irq_restore(old);
	return rv;
}

int _flashrom_delete(int offset) {
	int	(*sc)(int, int, int, int);
	int	old, rv;

	old = irq_disable();
	*((uint32 *)&sc) = *((uint32 *)0x8c0000b8);
	rv = sc(offset, 0, 0, 3);
	irq_restore(old);
	return rv;
}

int flash_clear(int block) {
	int size, start;

	if (flashrom_info(block, &start, &size) < 0) {
		ds_printf("DS_ERROR: Get offset error\n");
		return -1;
	}


	if (_flashrom_delete(start) < 0) {
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

	if (_flashrom_delete(start) < 0) {
		ds_printf("DS_ERROR: Deleting old flash factory data error\n");
		return -1;
	}

	if (_flashrom_write(start, data, size) < 0) {
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

int flash_write_file(const char *filename) {
   	
	file_t fd;
	uint8 *data;
	size_t size;
	
	fd = fs_open(filename, O_RDONLY);
	
	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}
	
	size = fs_total(fd);
	
	if (size > 128 * 1024) {
		fs_close(fd);
		ds_printf("DS_ERROR: Firmware is too big: %d bytes\n", size);
		return -1;
	}
	
	data = (uint8*) malloc(size);
	
	if (data == NULL) {
		fs_close(fd);
		ds_printf("DS_ERROR: Not enough of memory\n");
		return -1;
	}
	
	if (fs_read(fd, data, size) < 0) {
		free(data);
		fs_close(fd);
		ds_printf("DS_ERROR: Error in reading file: %d\n", errno);
		return -1;
	}
	
	fs_close(fd);
    
	if (_flashrom_write(0, data, size) < 0) {
		ds_printf("DS_ERROR: Write new flash factory data error\n");
		free(data);
		return -1;
	}
	
	free(data);
	return 0;
}
