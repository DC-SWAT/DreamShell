/**
 * DreamShell ISO Loader
 * (c)2009-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <ubc.h>
#include <exception.h>

isoldr_info_t *IsoInfo;
uint32 loader_addr = 0;

int main(int argc, char *argv[]) {

	(void)argc;
	(void)argv;
	
	if(is_custom_bios() && is_no_syscalls()) {
		bfont_saved_addr = 0xa0001000;
	}

	IsoInfo = (isoldr_info_t *)LOADER_ADDR;
	loader_addr = (uint32)IsoInfo;

	OpenLog();
	printf(NULL);

	malloc_init();

	/* Setup BIOS timer */
	timer_init();
	timer_prime_bios(TMU0);
	timer_start(TMU0);

	printf("DreamShell ISO from "DEV_NAME" loader v"VERSION"\n");
	int emu_all_sc = 0;

	if(loader_addr < ISOLDR_DEFAULT_ADDR_LOW
		|| (IsoInfo->heap >= HEAP_MODE_SPECIFY && IsoInfo->heap < ISOLDR_DEFAULT_ADDR_LOW)
		|| (is_custom_bios() && is_no_syscalls())) {
		emu_all_sc = 1;
	}

	enable_syscalls(emu_all_sc);

	if(IsoInfo->magic[0] != 'D' || IsoInfo->magic[1] != 'S' || IsoInfo->magic[2] != 'I') {
		LOGF("Magic is incorrect!\n");
		goto error;
	}

	if(IsoInfo->gdtex > 0) {
		draw_gdtex((uint8 *)IsoInfo->gdtex);
	}

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

	if(!InitReader()) {
		goto error;
	}

	if (!IsoInfo->use_dma) {
		fs_enable_dma(FS_DMA_DISABLED);
	}

	printf("Loading executable...\n");

	if(!Load_BootBin()) {
		goto error;
	}

	if(IsoInfo->exec.type == BIN_TYPE_KOS) {

		uint8 *src = (uint8 *)UNCACHED_ADDR(IsoInfo->exec.addr);

		if(src[1] != 0xD0) {

			printf("Descrambling...\n");

			uint32 exec_addr = CACHED_ADDR(IsoInfo->exec.addr);
			uint8 *dest = (uint8*)(exec_addr + (IsoInfo->exec.size * 3));

			descramble(src, dest, IsoInfo->exec.size);
			memcpy(src, dest, IsoInfo->exec.size);
		}
	}

	if(IsoInfo->boot_mode != BOOT_MODE_DIRECT) {

		printf("Loading IP.BIN...\n");

		if(!Load_IPBin()) {
			goto error;
		}
	}

	if(IsoInfo->exec.type != BIN_TYPE_KOS) {

		/* Patch GDC driver entry */
		gdc_syscall_patch();

		/* Patch G1 DMA regs in binary */
		gd_state_t *GDS = get_GDS();
		int cnt = patch_memory(0xA05F7418, (uint32)&GDS->dma_status);

		LOGF("Found DMA status reg %d times\n", cnt);
		cnt = patch_memory(0xA05F7414, (uint32)&GDS->streamed);
		LOGF("Found DMA len reg %d times\n", cnt);
		(void)cnt;
	}

#ifdef HAVE_EXPT
	if(IsoInfo->exec.type == BIN_TYPE_WINCE && IsoInfo->use_irq) {
		uint32 exec_addr = CACHED_ADDR(IsoInfo->exec.addr);
		uint8 wince_version = *((uint8 *)UNCACHED_ADDR(IsoInfo->exec.addr + 0x0c));
		/* Check WinCE version and hack VBR before executing */
		if(wince_version == 0xe0) {
			exception_init(exec_addr + 0x2110);
		} else {
			exception_init(exec_addr + 0x20f0);
		}
	}
#endif

	setup_machine_state();

	if(IsoInfo->boot_mode == BOOT_MODE_DIRECT) {
		printf("Executing...\n");
		launch(UNCACHED_ADDR(IsoInfo->exec.addr));
	}

	printf("Executing from IP.BIN...\n");
	launch((IsoInfo->boot_mode == BOOT_MODE_IPBIN_TRUNC ? 0xac00e000 : 0xac00b800));

error:
	printf("Failed!\n");
	disable_syscalls(emu_all_sc);
	timer_spin_sleep_bios(3000);
	Load_DS();
	return -1;
}
