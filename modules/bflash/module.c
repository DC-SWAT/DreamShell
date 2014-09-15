/* DreamShell ##version##

   module.c - bflash module
   Copyright (C)2009-2014 SWAT 

*/          

#include "ds.h"
#include "drivers/bflash.h"

DEFAULT_MODULE_EXPORTS_CMD(bflash, "BootROM flasher");

int builtin_bflash_cmd(int argc, char *argv[]) { 
     
	if(argc < 2) {
		ds_printf("Usage: %s options args\n"
					"Options: \n"
					" -r, --read      -Read flash sector(s) and write to file\n"
					" -w, --write     -Write flash sector(s) from file (can be used with -a)\n"
					" -e, --erase     -Erase flash sector(s)\n"
					" -a, --eraseall  -Erase the whole flash chip\n"
					" -l, --list      -Print list of supported devices\n"
					" -t, --sectors   -Print list of device sectors\n"
					" -c, --check     -Attempt to detect the flash chip\n\n", argv[0]);
		ds_printf("Arguments: \n"
					" -m, --manufacturer  -Manufacturer id\n"
					" -d, --device        -Device id\n"
					" -f, --file          -Binary file (input or output)\n"
					" -b, --sector        -Start sector\n"
					" -s, --size          -Size to read\n\n"
					"Example: %s -w -f /cd/firmware.bios\n\n", argv[0]);
		return CMD_NO_ARG; 
	} 

	int bbflash_write = 0, 
		bbflash_read = 0, 
		bbflash_erase = 0, 
		bbflash_eraseall = 0, 
		bbflash_printlist = 0,
		bbflash_printsectors = 0,
		bbflash_check = 0;
	
	char *file = NULL;
	uint size = 0;
	uint32 sector = 0x0, mfr_id = 0x0, dev_id = 0x0;
	bflash_dev_t *dev = NULL;

	struct cfg_option options[] = {
		{"write",			'w', NULL, CFG_BOOL, (void *) &bbflash_write,				0},
		{"read",			'r', NULL, CFG_BOOL, (void *) &bbflash_read,				0},
		{"erase",			'e', NULL, CFG_BOOL, (void *) &bbflash_erase,				0},
		{"eraseall",		'a', NULL, CFG_BOOL, (void *) &bbflash_eraseall,			0},
		{"list",			'l', NULL, CFG_BOOL, (void *) &bbflash_printlist,			0},
		{"sectors",		't', NULL, CFG_BOOL, (void *) &bbflash_printsectors,		0},
		{"check",			'c', NULL, CFG_BOOL, (void *) &bbflash_check,				0},
		{"file",			'f', NULL, CFG_STR,  (void *) &file,						0},
		{"sector",			'b', NULL, CFG_ULONG,(void *) &sector,						0},
		{"size",			's', NULL, CFG_INT,  (void *) &size,						0},
		{"device",			'd', NULL, CFG_ULONG,(void *) &dev_id,						0},
		{"manufacturer",	'm', NULL, CFG_ULONG,(void *) &mfr_id,						0},
		CFG_END_OF_LIST
	};
	
	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(bbflash_printlist) {
		
		size = 0, sector = 0;
	
		while (bflash_manufacturers[size].id != 0) {
			
			ds_printf(" %s [0x%04x]\n", bflash_manufacturers[size].name, bflash_manufacturers[size].id);
			
			while (bflash_devs[sector].id != 0) {
				
				if((bflash_manufacturers[size].id & 0xff) == (bflash_devs[sector].id >> 8 & 0xff)) {
					ds_printf("    %s [0x%04x] %d KB %s\n", bflash_devs[sector].name, 
														 bflash_devs[sector].id, 
														 bflash_devs[sector].size,
														 (bflash_devs[sector].flags & F_FLASH_LOGIC_5V) ? 
														 ((bflash_devs[sector].flags & F_FLASH_LOGIC_3V) ? "3V,5V" : "5V") : "3V");
				}
				
				sector++;
			}
			
			sector = 0;
			size++;
		}
		
		return CMD_OK;
	}
	
	if(bbflash_printsectors) {
		
		char ac[512], secb[32];
		dev = bflash_find_dev(dev_id);
		
		if(dev == NULL) {
			ds_printf("DS_ERROR: Device 0x%04x not found\n", (uint16)dev_id);
			return CMD_ERROR;
		}

		for(sector = 0; sector < dev->sec_count; sector++) {
			
			memset(secb, 0, sizeof(secb));
			sprintf(secb, "0x%06lx%s", dev->sectors[sector], (sector == dev->sec_count - 1) ? "" : ", ");
			
			if(sector % 4) {
				strcat(ac, secb);
			} else {
				if(sector) ds_printf("%s\n", ac);
				memset(ac, 0, sizeof(ac));
				strcpy(ac, secb);
			}
		}
		
		if(sector) ds_printf("%s\n", ac);
		
		return CMD_OK;
	}
	
	if(bbflash_check) {
		
		if(bflash_detect(NULL, NULL) < 0) {
			return CMD_ERROR;
		}
		
		return CMD_OK;
	}
	
    if(bbflash_write && file != NULL) {

		if(bflash_auto_reflash(file, sector, bbflash_eraseall ? F_FLASH_ERASE_ALL : F_FLASH_ERASE_SECTOR) < 0) {
    		return CMD_ERROR;
    	}

		return CMD_OK; 
    }

	if(bbflash_erase) {
		
		if(bflash_detect(NULL, &dev) < 0) {
			return CMD_ERROR;
		}
		
		ds_printf("DS_PROCESS: Erasing sector: 0x%08x\n", sector);
		
		if (bflash_erase_sector(dev, sector) < 0) {
			return CMD_ERROR;
		}

		ds_printf("DS_OK: Complete!\n");
		return CMD_OK; 
	}
	
	if(bbflash_eraseall) {
		
		if(bflash_detect(NULL, &dev) < 0) {
			return CMD_ERROR;
		}
		
		ds_printf("DS_PROCESS: Erasing full flash chip...\n");
		
		if (bflash_erase_all(dev) < 0) {
			return CMD_ERROR;
		}

		ds_printf("DS_OK: Complete!\n");
		return CMD_OK; 
	}

	if(bbflash_read && file != NULL) {

		ds_printf("DS_PROCESS: Reading from 0x%08x size %d and writing to file %s\n", sector, size, file);
		file_t fw = fs_open(file, O_WRONLY | O_TRUNC | O_CREAT);

		if(fw < 0) {
			ds_printf("DS_ERROR: Can't create %s\n", file);
			return CMD_ERROR;
		}

		fs_write(fw, (uint8*)(BIOS_FLASH_ADDR | sector), size); 
		fs_close(fw);
		ds_printf("DS_OK: Complete!\n");
		return CMD_OK; 
	}
    
	ds_printf("DS_ERROR: There is no option.\n");
	return CMD_OK; 
}
