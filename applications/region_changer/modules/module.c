/* DreamShell ##version##

   module.c - Region Changer module
   Copyright (C)2010-2014 SWAT 

*/

#include "ds.h"
#include "rc.h"

DEFAULT_MODULE_HEADER(app_region_changer);

int tolua_RC_open(lua_State* tolua_S);


/*
	Japan (red swirl) - 0x30
	Japan (black swirl) - 0x58
	USA (red swirl) - 0x31
	USA (black swirl) - 0x59
	Europe (blue swirl) - 0x32
	Europe (black swirl) - 0x5A
*/

uint8 *flash_factory_set_country(uint8 *data, int country, int black_swirl) {

	switch(country) {
		case 1:
			data[2] = black_swirl ? 0x58 : 0x30;
			break;
		case 2:
			data[2] = black_swirl ? 0x59 : 0x31;
			break;
		case 3:
			data[2] = black_swirl ? 0x5A : 0x32;
			break;
		default:
			break;
	}
	return data;
}

uint8 *flash_factory_set_broadcast(uint8 *data, int broadcast) {

	switch(broadcast) {
		case 1:
			data[4] = 0x30;
			break;
		case 2:
			data[4] = 0x31;
			break;
		case 3:
			data[4] = 0x32;
			break;
		case 4:
			data[4] = 0x33;
			break;
		default:
			break;
	}
	return data;
}


uint8 *flash_factory_set_lang(uint8 *data, int lang) {

	switch(lang) {
		case 1:
			data[3] = 0x30;
			break;
		case 2:
			data[3] = 0x31;
			break;
		case 3:
			data[3] = 0x32;
			break;
		case 4:
			data[3] = 0x33;
			break;
		case 5:
			data[3] = 0x34;
			break;
		case 6:
			data[3] = 0x35;
			break;
		default:
			break;
	}
	return data;
}


flash_factory_values_t *flash_factory_get_values(uint8 *data) {

	flash_factory_values_t *values = (flash_factory_values_t*) malloc(sizeof(flash_factory_values_t));
	
	// Country
	if(data[2] == 0x58 || data[2] == 0x30) {
		values->country = 1;
		values->black_swirl = data[2] == 0x58 ? 1 : 0;
	} else if(data[2] == 0x59 || data[2] == 0x31) {
		values->country = 2;
		values->black_swirl = data[2] == 0x59 ? 1 : 0;
	} else if(data[2] == 0x5A || data[2] == 0x32) {
		values->country = 3;
		values->black_swirl = data[2] == 0x5A ? 1 : 0;
	} else {
		values->country = 0;
		values->black_swirl = 0;
	}

	// Lang
	if(data[3] == 0x30) {
		values->lang = 1;
	} else if(data[3] == 0x31) {
		values->lang = 2;
	} else if(data[3] == 0x32) {
		values->lang = 3;
	} else if(data[3] == 0x33) {
		values->lang = 4;
	} else if(data[3] == 0x34) {
		values->lang = 5;
	} else if(data[3] == 0x35) {
		values->lang = 6;
	} else {
		values->lang = 0;
	}

	// Broadcast
	if(data[4] == 0x30) {
		values->broadcast = 1;
	} else if(data[4] == 0x31) {
		values->broadcast = 2;
	} else if(data[4] == 0x32) {
		values->broadcast = 3;
	} else if(data[4] == 0x33) {
		values->broadcast = 4;
	} else {
		values->broadcast = 0;
	}

	return values;
}


void flash_factory_free_values(flash_factory_values_t *values) {
	free(values);
}

void flash_factory_free_data(uint8 *data) {
	free(data);
}


int lib_open(klibrary_t * lib) {

	lua_State *lua = GetLuaState();
	
	if(lua != NULL) {
		tolua_RC_open(lua);
	}
	
	RegisterLuaLib(lib_get_name(), (LuaRegLibOpen *)tolua_RC_open);
	return nmmgr_handler_add(&ds_app_region_changer_hnd.nmmgr);
}


int lib_close(klibrary_t * lib) {
	UnregisterLuaLib(lib_get_name());
	return nmmgr_handler_remove(&ds_app_region_changer_hnd.nmmgr); 
}
