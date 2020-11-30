/**
 * DreamShell ISO Loader
 * (c)2009-2020 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <ubc.h>
#include <exception.h>
#include <asic.h>

isoldr_info_t *IsoInfo;
uint32 loader_addr = 0;

int main(int argc, char *argv[]) {

	(void)argc;
	(void)argv;

	int emu_all_sc = 0;
	
	if(is_custom_bios() && is_no_syscalls()) {
		bfont_saved_addr = 0xa0001000;
	}
	
	printf(NULL);
	
	if(IsoInfo != NULL) {
		Load_DS();
	}
	
	IsoInfo = (isoldr_info_t *)LOADER_ADDR;
	loader_addr = (uint32)IsoInfo;

#if defined(DEV_TYPE_GD) || defined(DEV_TYPE_IDE)
	timer_init();
#endif

	printf("DreamShell ISO from "DEV_NAME" loader v"VERSION"\n");
	
	if(loader_addr < 0x8c004000 || (is_custom_bios() && is_no_syscalls())) {
		emu_all_sc = 1;
		printf("Emulate all syscalls: enabled\n");
	}
	
	enable_syscalls(emu_all_sc);
	OpenLog();

	if(IsoInfo->magic[0] != 'D' || IsoInfo->magic[2] != 'I' || IsoInfo->magic[9] != '6') {
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

	LOGF("Loader size: %ld + %d = %ld at %08lx\n", 
		loader_size, 
		ISOLDR_PARAMS_SIZE, 
		loader_size + ISOLDR_PARAMS_SIZE,
		loader_addr
	);
	
	printf("Initializing "DEV_NAME"...\n");
	
	if(!InitReader()) {
		goto error;
	}

#ifdef HAVE_CDDA
	if(IsoInfo->emu_cdda) {
		CDDA_Init();
//		CDDA_Test();
	}
#endif

#ifdef LOG
	printf("Loading %s %d Kb\n", IsoInfo->exec.file, IsoInfo->exec.size / 1024);
#else
	printf("Loading executable...\n");
#endif

	if(!Load_BootBin()) {
		goto error;
	}

	if(IsoInfo->exec.type == BIN_TYPE_KOS) {
		
		uint8 *src = (uint8*)(IsoInfo->exec.addr);
		
		if(src[1] != 0xD0) {

			LOGF("Detected scrambled homebrew executable\n");
			printf("Descrambling...\n");
			
			uint32 bin_addr = IsoInfo->exec.addr & 0x0fffffff;
			bin_addr |= 0x80000000;
			uint8 *dest = (uint8*)(bin_addr + (IsoInfo->exec.size * 3));
			
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
#ifndef LOG
		(void)cnt;
#endif
	}

	/* HW widescreen and VGA forcing (incomplete) */
//	ubc_init();
//	ubc_configure_channel(UBC_CHANNEL_A, 0xa05f80f4, UBC_BBR_OPERAND | UBC_BBR_WRITE);
//	ubc_configure_channel(UBC_CHANNEL_B, (uint32)&GDS->dma_status_reg, UBC_BBR_OPERAND | UBC_BBR_READ | UBC_BBR_WRITE);

	/* Setup BIOS timer */
	timer_prime_bios(TMU0);
	timer_start(TMU0);
	
#ifdef HAVE_EXPT
	if(IsoInfo->exec.type == BIN_TYPE_WINCE && IsoInfo->use_irq) {
		/* Check WinCE version and hack VBR before executing */
		if(*((uint8 *)0xac01000c) == 0xe0) {
			exception_init(0x8c012110);
		} else {
			exception_init(0x8c0120f0);
		}
	}
#endif

	/* Clear IRQ stuff */
	*ASIC_IRQ9_MASK  = 0;
	*ASIC_IRQ11_MASK = 0;
	*ASIC_IRQ13_MASK = 0;
	ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = 0x04038;
	uint8 st = *((volatile uint8 *)0xA05F709C);
	(void)st;

	if(IsoInfo->boot_mode == BOOT_MODE_DIRECT) {
#ifdef LOG
		printf("Executing at 0x%08lx...\n", IsoInfo->exec.addr);
#else
		printf("Executing...\n");
#endif
		launch(IsoInfo->exec.addr);
	}

	printf("Executing from IP.BIN...\n");
	uint32 addr = 0xac00b800; //(uint32)IsoInfo < 0x8c010000 ? 0xac00b800 : 0xac008300;
	launch((IsoInfo->boot_mode == BOOT_MODE_IPBIN_TRUNC ? 0xac00e000 : addr));
	
error:
	printf("Failed!\n");
	disable_syscalls(emu_all_sc);
//	timer_spin_sleep(3000);
	return -1;
}
