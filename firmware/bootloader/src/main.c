/* DreamShell boot loader
	
   main.c
   (c) 2011-2014 SWAT
*/

#include "main.h"
#include "fs.h"


extern uint8 romdisk[];
//KOS_INIT_ROMDISK(romdisk);

pvr_init_params_t params = {
	{ PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0 },
	512*1024
};

const char	title[28] = "DreamShell boot loader v2.3";

//#define EMU
//#define BIOS_MODE

#ifndef EMU
KOS_INIT_FLAGS(INIT_DEFAULT/* | INIT_NET*/);
#else
KOS_INIT_FLAGS(INIT_DEFAULT);
#endif


int FileSize(const char *fn) {
	int size;
	file_t f;

	f = fs_open(fn, O_RDONLY); 

	if(f == FILEHND_INVALID) {
		return -1; 
	}

	size = fs_total(f); 
	fs_close(f); 
	return size; 
}

int FileExists(const char *fn) {
    file_t f;
    
    f = fs_open(fn, O_RDONLY);
	
    if(f == FILEHND_INVALID) {
       return 0; 
    }
    
    fs_close(f);
    return 1;
}

int DirExists(const char *dir) {
    file_t f;
    
    f = fs_open(dir, O_DIR | O_RDONLY);
	
    if(f == FILEHND_INVALID) {
       return 0; 
    }
    
    fs_close(f);
    return 1;
}


int flashrom_get_region_only() {
	
	int start, size;
	uint8 region[6] = { 0 };
	region[2] = *(uint8*)0x0021A002;

	/* Find the partition */
	if(flashrom_info(FLASHROM_PT_SYSTEM, &start, &size) < 0) {
		
		dbglog(DBG_ERROR, "%s: can't find partition %d\n", __func__, FLASHROM_PT_SYSTEM);
		
	} else {

		/* Read the first 5 characters of that partition */
		if(flashrom_read(start, region, 5) < 0) {
			dbglog(DBG_ERROR, "%s: can't read partition %d\n", __func__, FLASHROM_PT_SYSTEM);
		}
	}

	if(region[2] == 0x58 || region[2] == 0x30) {
		spiral_color = 0x44ed1800;
		return FLASHROM_REGION_JAPAN;
	} else if(region[2] == 0x59 || region[2] == 0x31) {
		spiral_color = 0x44ed1800;
		return FLASHROM_REGION_US;
	} else if(region[2] == 0x5A || region[2] == 0x32) {
		spiral_color = 0x443370d4;
		return FLASHROM_REGION_EUROPE;
	} else {
		spiral_color = 0x44000000;
		dbglog(DBG_ERROR, "%s: Unknown region code %02x\n", __func__, region[2]);
		return FLASHROM_REGION_UNKNOWN;
	}
}


int is_custom_bios() {
	return (*(uint16 *)0x80000004) != 0x6439;
}

#ifdef BIOS_MODE
//int is_no_syscalls() {
//	return (*(uint16 *)0x8c000100) != 0x2f06;
//}


static int setup_syscalls() {

	file_t fd;
	size_t fd_size;
	int (*sc)(int, int, int, int);
	
	dbglog(DBG_INFO, "Loading and setup syscalls...\n");

	fd = fs_open(RES_PATH"/syscalls.bin", O_RDONLY);
	
	if(fd != FILEHND_INVALID) {

		fd_size = fs_total(fd);
		dbgio_dev_select("null");
		dcache_flush_range(0x8c000000, fd_size);
		icache_flush_range(0x8c000000, fd_size);

		if(fs_read(fd, (uint8*)0x8c000000, fd_size) < 0) {
			
			fs_close(fd);
			dbgio_dev_select("fb");
			dbglog(DBG_ERROR, "Error at loading syscalls\n");
			return -1;
			
		} else {
		
			fs_close(fd);
			dcache_flush_range(0x8c000000, fd_size);
			icache_flush_range(0x8c000000, fd_size);
			
			dbgio_dev_select("fb");
			*((uint32 *)&sc) = *((uint32 *)0x8c0000b0);
			
			if(sc(0, 0, 0, 0)) {
				dbglog(DBG_ERROR, "Error in sysinfo syscall\n");
				return -1;
			}
			
			dbglog(DBG_INFO, "Syscalls successfully installed\n");
			return 0;
		}
	} else {
		dbglog(DBG_ERROR, "Can't open file with syscalls\n");
	}
	
	return -1;
}
#endif


int start_pressed = 0;

static void start_callback(void) {
	if(!start_pressed) {
		dbglog(DBG_INFO, "Enter the boot menu...\n");
		start_pressed = 1;
	}
}

static void show_boot_message(void) {
	dbglog(DBG_INFO, "         %s\n", title);
	dbglog(DBG_INFO, "  !!! Press START to enter the boot menu !!!\n\n");
}


int main(int argc, char **argv) {
	
	cont_btn_callback(0, CONT_START, (cont_btn_callback_t)start_callback);
	
	if(vid_check_cable() == CT_VGA) {
		
		vid_set_mode(DM_640x480_VGA, PM_RGB565);
		
	} else {
		
		int region = flashrom_get_region_only();

		switch(region) {
			case FLASHROM_REGION_US:
			case FLASHROM_REGION_JAPAN:
				vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);
				break;
			case FLASHROM_REGION_EUROPE:
				vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);
				break;
			case FLASHROM_REGION_UNKNOWN:
			default:
				break;
		}
	}

#ifndef EMU

#ifdef BIOS_MODE


	if(!fs_romdisk_mount(RES_PATH, (const uint8 *)romdisk, 0)) {
		
		dbgio_dev_select("fb");
		show_boot_message();
		
		if(is_custom_bios()/* && is_no_syscalls()*/) {
			setup_syscalls();
			cdrom_init();
			fs_iso9660_init();
		}
		
	} else {
		dbgio_dev_select("fb");
		show_boot_message();
	}

	InitIDE();
	InitSDCard();

#else

	int ird = 1;
	dbgio_dev_select("fb");
	show_boot_message();
	
	if(!InitIDE()) {
		ird = 0;
	}
	
	if(!InitSDCard()) {
		ird = 0;
	}
	
	if(ird && is_custom_bios()) {
		InitRomdisk();
	}
#endif

	dbgio_disable();
	
#else
	dbgio_dev_select("scif");
	show_boot_message();
#endif

	if(!start_pressed) {
		menu_init();
		loading_core(1);
	}

	pvr_init(&params);

#ifdef BIOS_MODE	
	spiral_init();
	fs_romdisk_unmount(RES_PATH);
#else
	if(!fs_romdisk_mount(RES_PATH, (const uint8 *)romdisk, 0)) {
		spiral_init();
		fs_romdisk_unmount(RES_PATH);
	}
#endif
	
	if(!start_pressed) 
		init_menu_txr();
	else
		menu_init();
		
	pvr_set_bg_color(192.0/255.0, 192.0/255.0, 192.0/255.0);
	
	while(1) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_TR_POLY);
		spiral_frame();
		menu_frame();
		pvr_list_finish();
		pvr_scene_finish();
	}

	return 0;
}
