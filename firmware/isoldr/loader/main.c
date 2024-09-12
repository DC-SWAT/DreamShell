/**
 * DreamShell ISO Loader
 * (c)2009-2024 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <ubc.h>
#include <exception.h>
#include <maple.h>

isoldr_info_t *IsoInfo;
uint32 loader_addr = 0;
uint32 loader_end = 0;

int main(int argc, char *argv[]) {

	(void)argc;
	(void)argv;

	if(is_custom_bios() && is_no_syscalls()) {
		bfont_saved_addr = 0xa0001000;
	}

	IsoInfo = (isoldr_info_t *)LOADER_ADDR;
	loader_addr = (uint32)IsoInfo;
	loader_end = loader_addr + loader_size + ISOLDR_PARAMS_SIZE + 32;

	OpenLog();
	printf(NULL);
	printf("DreamShell ISO from "DEV_NAME" loader v"VERSION"\n");

	malloc_init(1);

	/* Setup BIOS timer */
	timer_init();
	timer_prime_bios(TMU0);
	timer_start(TMU0);

	int emu_all_sc = 0;

	if (IsoInfo->syscalls == 0) {
		if(loader_addr < ISOLDR_DEFAULT_ADDR_LOW
			|| (malloc_heap_pos() < ISOLDR_DEFAULT_ADDR_LOW)
			|| (is_custom_bios() && is_no_syscalls())) {
			emu_all_sc = 1;
		}
		enable_syscalls(emu_all_sc);
	}

#ifndef HAVE_LIMIT
	if(IsoInfo->magic[0] != 'D' || IsoInfo->magic[1] != 'S' || IsoInfo->magic[2] != 'I') {
		LOGF("Magic is incorrect!\n");
		goto error;
	}
#endif

#if 0//ndef HAVE_LIMIT
	if(IsoInfo->gdtex > 0) {
		draw_gdtex((uint8 *)IsoInfo->gdtex);
	}
#endif

	LOGF("Magic: %s\n"
		"LBA: %d (%d)\n"
		"Sector size: %d\n"
		"Image type: %d\n",
		IsoInfo->magic, 
		IsoInfo->track_lba[0],
		IsoInfo->track_lba[1],
		IsoInfo->sector_size,
		IsoInfo->image_type
	);

	LOGF("Boot: %s at %d size %d addr %08lx type %d mode %d\n",
		IsoInfo->exec.file,
		IsoInfo->exec.lba,
		IsoInfo->exec.size,
		IsoInfo->exec.addr,
		IsoInfo->exec.type,
		IsoInfo->boot_mode
	);

	LOGF("Loader: %08lx - %08lx size %d, heap %08lx\n", 
		loader_addr,
		loader_addr + loader_size,
		loader_size + ISOLDR_PARAMS_SIZE,
		malloc_heap_pos()
	);

	printf("Initializing "DEV_NAME"...\n");
	gdcReset();

	if(!InitReader()) {
		goto error;
	}

	if (!IsoInfo->use_dma) {
		fs_enable_dma(FS_DMA_DISABLED);
	}

#ifdef HAVE_BLEEM
	if(IsoInfo->bleem) {
		printf("Loading Bleem!...\n");
		Load_Bleem();
	}
	else 
#endif
	{
		printf("Loading executable...\n");

		if(!Load_BootBin()) {
			goto error;
		}
	}

	if((IsoInfo->boot_mode != BOOT_MODE_DIRECT) ||
		((loader_end < CACHED_ADDR(IP_BIN_ADDR) ||
			loader_addr > CACHED_ADDR(APP_BIN_ADDR)) &&
		(malloc_heap_pos() < CACHED_ADDR(IP_BIN_ADDR) ||
			malloc_heap_pos() > CACHED_ADDR(APP_BIN_ADDR)))
	) {
		printf("Loading IP.BIN...\n");

		if(!Load_IPBin(IsoInfo->boot_mode == BOOT_MODE_DIRECT ? 1 : 0)) {
			goto error;
		}
	}

	if(IsoInfo->exec.type != BIN_TYPE_KOS) {
		/* Patch GDC driver entry */
		gdc_syscall_patch();
	}

#ifdef HAVE_EXPT
	if(IsoInfo->exec.type == BIN_TYPE_WINCE && IsoInfo->use_irq) {
		uint32 vbr_offset = *((uint32 *)NONCACHED_ADDR(IsoInfo->exec.addr + 0x0c)) + 0x30;
		exception_init(vbr_offset);
	}
#endif

#ifdef HAVE_EXT_SYSCALLS
	if(IsoInfo->syscalls) {
		printf("Loading syscalls...\n");
		Load_Syscalls();
	}
#endif
#ifdef HAVE_CDDA
	if(IsoInfo->emu_cdda) {
		CDDA_Init();
# ifdef HAVE_CDDA_TEST
		CDDA_Test();
# endif
	}
#endif
#ifdef HAVE_MAPLE
	if(IsoInfo->emu_vmu) {
		maple_init_vmu(IsoInfo->emu_vmu, IsoInfo->exec.type == BIN_TYPE_KATANA);
	}
#endif

	setup_machine();

	if(IsoInfo->boot_mode == BOOT_MODE_DIRECT) {
		printf("Executing...\n");
		launch(IsoInfo->exec.addr);
	} else {
		printf("Executing from IP.BIN...\n");
		launch(IP_BIN_BOOTSTRAP_2_ADDR);
	}

error:
	printf("Failed!\n");
	Load_DS();
	launch(IsoInfo->exec.addr);
	return -1;
}
