/**
 * DreamShell ISO Loader
 * (c)2009-2025 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <naomi/cart.h>
#include <asic.h>
#include <exception.h>
#include <ubc.h>
#include <maple.h>

isoldr_info_t *IsoInfo;
uint32 loader_end;

int main(int argc, char *argv[]) {

	(void)argc;
	(void)argv;

	if(is_custom_bios() && is_no_syscalls()) {
		bfont_saved_addr = 0xa0001000;
	}

	loader_end = loader_addr + loader_size + ISOLDR_PARAMS_SIZE + 32;
	IsoInfo = (isoldr_info_t *)(loader_addr - ISOLDR_PARAMS_SIZE);

	OpenLog();
	printf(NULL);
	printf("DreamShell ISO from "DEV_NAME" loader v"VERSION"\n");

	malloc_init(1);

	/* Setup BIOS timer */
	timer_init();
	timer_prime_bios(TMU0);
	timer_start(TMU0);
#ifdef LOG
	/* Setup Perfomance Counter timer */
	timer_ns_enable();
#endif

	int emu_all_sc = is_dreamcast() ? 0 : 1;

	if(IsoInfo->image_type == IMAGE_TYPE_ROM_NAOMI || IsoInfo->syscalls == 0) {
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

	LOGF("Boot: %s at %d size %d load %08lx exec %08lx type %d mode %d\n",
		IsoInfo->exec.file,
		IsoInfo->exec.lba,
		IsoInfo->exec.size,
		IsoInfo->exec.addr,
		IsoInfo->exec_addr > 0 ? IsoInfo->exec_addr : IsoInfo->exec.addr,
		IsoInfo->exec.type,
		IsoInfo->boot_mode
	);

	LOGF("Loader: %08lx - %08lx size %d, params %08lx, heap %08lx\n", 
		loader_addr,
		loader_addr + loader_size,
		loader_size + ISOLDR_PARAMS_SIZE,
		(uint32)IsoInfo,
		malloc_heap_pos()
	);

	printf("Initializing "DEV_NAME"...\n");
	gdcReset();

	if(!InitReader()) {
		goto error;
	}

	if(!IsoInfo->use_dma) {
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
#ifdef HAVE_EXT_SYSCALLS
	if(IsoInfo->image_type == IMAGE_TYPE_ROM_NAOMI) {

		/* Clear ROM DMA busy flag */
		*((uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
		/* Patch some values */
		*((uint32_t *)NONCACHED_ADDR(NAOMI_CART_REGION_ADDR)) = IsoInfo->region > 0 ?
			IsoInfo->region - 1 : 0; // See naomi/cart.h for region values
		*((uint32_t *)NONCACHED_ADDR(0x0c01f104)) = 1;
		*((uint32_t *)NONCACHED_ADDR(0x0c01f108)) = 0;
		*((uint32_t *)NONCACHED_ADDR(0x0c01f10c)) = 1; // 31 KHz

		/* Set VBR address for IRQ vectors */
		boot_vbr = 0x0c000000;
		LOGF("Setting VBR address to %08lx\n", boot_vbr);

		/* Set stack pointer */
		boot_stack = 0x0cc00000;
		LOGF("Setting stack pointer to %08lx\n", boot_stack);

		/* Set SR value */
		boot_sr = 0x60000101;
		LOGF("Setting SR to %08lx\n", boot_sr);

		uint8_t *dst = (uint8_t *) NONCACHED_ADDR(SYSCALLS_FW_ADDR);
		uint8_t *src = (uint8_t *) IsoInfo->firmware;
		LOGF("Loading IRQ vectors from %08lx to %08lx %d bytes\n",
			(uintptr_t)src, (uintptr_t)dst, 0x4f00);
		memcpy(dst, src, 0x4f00);

		dst = (uint8_t *) NONCACHED_ADDR(0x0c018000);
		src = (uint8_t *) IsoInfo->firmware + 0x4f00;
		LOGF("Loading IRQ handlers from %08lx to %08lx %d bytes\n",
			(uintptr_t)src, (uintptr_t)dst, 0x7000);
		memcpy(dst, src, 0x7000);

# ifdef HAVE_EXPT
		if(IsoInfo->use_irq) {
			exception_init(boot_vbr);
			asic_init();
		}
# endif
	}
	else 
#endif
	{
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

		if(!is_dreamcast() && IsoInfo->exec.type == BIN_TYPE_KATANA) {
			/* Patch GPIO register to prevent cable detection */
			argc = patch_memory(0xff800030,
				(uintptr_t)&IsoInfo->cdda_offset[(sizeof(IsoInfo->cdda_offset) / 4) - 1], 0);
			LOGF("Patch GPIO register: %d\n", argc);

			if(IsoInfo->firmware) {
				uintptr_t new_addr = NONCACHED_ADDR(loader_addr - ISOLDR_PARAMS_SIZE - 0x20000);
				memcpy((void *)new_addr, (void *)IsoInfo->firmware, 0x20000);
				IsoInfo->firmware = new_addr;
				LOGF("Loading flashROM dump to %08lx\n", new_addr);
			}
		}
	}

#ifdef HAVE_EXPT
	if(IsoInfo->exec.type == BIN_TYPE_WINCE && IsoInfo->use_irq) {
		uint32 vbr_offset = *((uint32 *)NONCACHED_ADDR(IsoInfo->exec.addr + 0x0c)) + 0x30;
		exception_init(vbr_offset);
	}
#endif

#ifdef HAVE_EXT_SYSCALLS
	if(IsoInfo->syscalls && IsoInfo->image_type != IMAGE_TYPE_ROM_NAOMI) {
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
		launch(IsoInfo->exec_addr > 0 ? IsoInfo->exec_addr : IsoInfo->exec.addr);
	} else {
		printf("Executing from IP.BIN...\n");
		launch(IP_BIN_BOOTSTRAP_2_ADDR);
	}

error:
	printf("Failed!\n");
	Load_DS();
	launch(APP_BIN_ADDR);
	return -1;
}
