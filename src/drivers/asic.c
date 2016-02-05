/* DreamShell ##version##

   asic.c
   Copyright (C) 2014-2015 SWAT
*/

#include <arch/types.h>
#include <errno.h>
#include <dc/asic.h>
#include <kos/dbglog.h>
#include <drivers/asic.h>


void asic_sys_reset(void) {
	ASIC_SYS_RESET = 0x00007611;
	while(1);
}

void asic_ide_enable(void) {
	uint32 p;
	volatile uint32 *bios = (uint32*)0xa0000000;

	/* 
	 * Send the BIOS size and then read each word 
	 * across the bus so the controller can verify it. 
	 */
	
	if((*(uint16 *)0xa0000000) == 0xe6ff) {
		
		// Initiate BIOS checking for 1KB
		ASIC_BIOS_PROT = 0x3ff;

		for(p = 0; p < 0x400 / sizeof(bios[0]); p++) {
			(void)bios[p];
		}
		
	} else {

		// Initiate BIOS checking for 2MB
		ASIC_BIOS_PROT = 0x1fffff;

		for(p = 0; p < 0x200000 / sizeof(bios[0]); p++) {
			(void)bios[p];
		}
	}
}

void asic_ide_disable(void) {
	ASIC_BIOS_PROT = 0x000042FE;
}
