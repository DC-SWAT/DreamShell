/* DreamShell ##version##

   module.c - Region Changer module
   Copyright (C)2010-2014 SWAT 

*/


#include "ds.h"


typedef struct flash_factory_values {

	int country;
	int broadcast;
	int lang;
	int black_swirl;

} flash_factory_values_t;


/**
 * Flashrom addons
 */
int _flashrom_write(int offset, void * buffer, int bytes);
int _flashrom_delete(int offset);


/**
 * Application data
 */
flash_factory_values_t *flash_factory_get_values(uint8 *data);
uint8 *flash_factory_set_lang(uint8 *data, int lang);
uint8 *flash_factory_set_broadcast(uint8 *data, int broadcast);
uint8 *flash_factory_set_country(uint8 *data, int country, int black_swirl);

int flash_clear(int block);
uint8 *flash_read_factory();
int flash_write_factory(uint8 *data);
int flash_write_file(const char *filename);

void flash_factory_free_values(flash_factory_values_t *values);
void flash_factory_free_data(uint8 *data);


/**
 * Lua binding
 */
int tolua_RC_open (lua_State* tolua_S);
