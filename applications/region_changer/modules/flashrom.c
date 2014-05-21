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
		memcpy(data, (uint8 *)0x0021A000, size);
		
	} else {
		
		data = (uint8 *) malloc(size);
		
		if (flashrom_read(start, data, size) < 0) {
			ds_printf("DS_ERROR: Flash read lash factory failed\n");
			return NULL;
		}
	}
	
	return data;
}
