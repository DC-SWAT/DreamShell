/**
 * DreamShell ISO Loader
 * (c)2009-2026 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <asic.h>
#include <exception.h>
#include <ubc.h>
#include <maple.h>
#include <gpio.h>

isoldr_info_t *IsoInfo;
uint32 loader_addr = 0;
uint32 loader_end;
extern uint32 loader_size;
extern void start(void) __asm__("start");

int main(int argc, char *argv[]) {

	(void)argc;
	(void)argv;

	IsoInfo = (isoldr_info_t *)((uint32)start - ISOLDR_PARAMS_SIZE);
	loader_addr = (uint32)IsoInfo;
	loader_end = loader_addr + loader_size + ISOLDR_PARAMS_SIZE + 32;

	OpenLog();
	printf(NULL);
	printf("DreamShell ISO from "DEV_NAME" loader v"VERSION"\n");

	malloc_init(1);
#ifdef HAVE_NAOMI
	naomi_cart_stack_setup();
#endif

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
			|| (malloc_heap_pos() < ISOLDR_DEFAULT_ADDR_LOW)) {
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

#ifdef HAVE_NAOMI
	if(IsoInfo->image_type == IMAGE_TYPE_ROM_NAOMI) {
		printf("Loading executable...\n");

		if(!naomi_load_bin(0)) {
			goto error;
		}

		if(!get_naomi()->aw) {
			naomi_hook_boot_services(NONCACHED_ADDR((uint32)naomi_enter_test),
				NONCACHED_ADDR((uint32)naomi_enter_game));
		}

# ifdef HAVE_EXPT
		if(IsoInfo->use_irq) {
			if(!exception_init(RAM_START_ADDR)) {
				asic_init();
#  if defined(DEV_TYPE_GD) || defined(DEV_TYPE_IDE)
				g1_dma_init_irq();
#  endif
			}
		}
# endif
		naomi_aw_prepare();
	}
	else
#endif
	{
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

			if(IsoInfo->exec.type == BIN_TYPE_KATANA) {
				*(volatile uint8 *)NONCACHED_ADDR(SYD_DDS_FLAG_ADDR) &= (uint8)~SYD_DDS_FLAG_CLEAR;
			}
		}
		if(IsoInfo->exec.type != BIN_TYPE_KOS) {
			/* Patch GDC driver entry */
			gdc_syscall_patch();
		}

		if(!is_dreamcast()) {
			if(IsoInfo->exec.type == BIN_TYPE_KATANA) {
				argc = patch_cable_detection(GPIO_CABLE_VGA);
				LOGF("Patch GPIO cable detection: %d\n", argc);
			}
			if(IsoInfo->firmware) {
				const uint32 dump_size = ISOLDR_FLASHROM_PATH_SIZE + ISOLDR_FLASHROM_SIZE;
				uintptr_t new_addr = NONCACHED_ADDR(loader_addr - dump_size);
				if(loader_addr < APP_BIN_ADDR) {
					new_addr = NONCACHED_ADDR(ISOLDR_DEFAULT_ADDR_NAOMI_DC);
				}
				memcpy((void *)new_addr, (void *)IsoInfo->firmware, dump_size);
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
		launch(IsoInfo->exec.addr);
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
