/**
 * DreamShell ISO Loader
 * (c)2009-2025 SWAT <http://www.dc-swat.ru>
 */

#ifndef _ISO_LOADER_H
#define _ISO_LOADER_H

#if defined(LOG)
#include <dc/scif.h>
#endif

#include <dc/video.h>
#include <dc/biosfont.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <isoldr.h>
#include "fs.h"
#include "syscalls.h"
#include "reader.h"
#include "cdda.h"
#include "malloc.h"

/* Physycal addresses for RAM and ROM's */
#define BIOS_ROM_ADDR                0x00000000
#define BIOS_ROM_SYSCALLS_ADDR       (BIOS_ROM_ADDR + 0x100)
#define BIOS_ROM_APP_BIN_ADDR        (BIOS_ROM_ADDR + 0x10000)
#define BIOS_ROM_FONT_ADDR           (BIOS_ROM_ADDR + 0x100020)
#define FLASH_ROM_ADDR               0x00200000
#define FLASH_ROM_REGION_ADDR        (FLASH_ROM_ADDR + 0x1a000)
#define FLASH_ROM_SYS_ID_ADDR        (FLASH_ROM_ADDR + 0x1a056)
#define FLASH_ROM_ICON_ADDR          (FLASH_ROM_ADDR + 0x1a480)
#define RAM_START_ADDR               0x0c000000
#define RAM_END_ADDR                 0x0d000000

/* Software environment structure in RAM */
#define SYSCALLS_INFO_ADDR           (RAM_START_ADDR)
#define SYSCALLS_INFO_SYS_ID_ADDR    (RAM_START_ADDR + 0x68)
#define SYSCALLS_INFO_REGION_ADDR    (RAM_START_ADDR + 0x70)
#define SYSCALLS_FW_ADDR             (RAM_START_ADDR + 0x100)
#define SYSCALLS_FW_GDC_ADDR         (RAM_START_ADDR + 0x1000)
#define SYSCALLS_FW_GDC_ENTRY_ADDR   (RAM_START_ADDR + 0x10f0)
#define SYSCALLS_RESERVED_ADDR       (RAM_START_ADDR + 0x4000)
#define IP_BIN_ADDR                  (RAM_START_ADDR + 0x8000)
#define IP_BIN_REGION_ADDR           (RAM_START_ADDR + 0x8030)
#define IP_BIN_BOOTSTRAP_1_ADDR      (RAM_START_ADDR + 0x8300)
#define IP_BIN_BOOTSTRAP_2_ADDR      (RAM_START_ADDR + 0xe000)
#define APP_BIN_ADDR                 (RAM_START_ADDR + 0x10000)

/* Address conversion */
#define PHYS_ADDR(addr) ((addr) & 0x1fffffff)
#define CACHED_ADDR(addr) (PHYS_ADDR(addr) | 0x80000000)
#define NONCACHED_ADDR(addr) (PHYS_ADDR(addr) | 0xa0000000)

/* CPU cache and all DMA's is 32-byte aligned */
#define ALIGN32_ADDR(addr) (((addr) + 0x1f) & ~0x1f)

#define SYD_DDS_FLAG_ADDR NONCACHED_ADDR(IP_BIN_ADDR + 0xfc))
#define SYD_DDS_FLAG_CLEAR 0x20

#define SH4_OPCODE_NOP 0x0009

#define is_custom_bios() (*(uint16 *)NONCACHED_ADDR(BIOS_ROM_ADDR + 0x100018) != 0x4e46)
#define is_no_syscalls() (*(uint16 *)NONCACHED_ADDR(RAM_START_ADDR + 0x00100) != 0x2f06)

#define HOLLY_REV_VA1 0x10
#define HOLLY_REV_VA0 0x0b
#define holly_revision() (*(vuint32 *)NONCACHED_ADDR(0x005f689c))

extern isoldr_info_t *IsoInfo;
extern uint32 loader_size;
extern uint32 loader_addr;
extern uint32 loader_end;
extern void boot_stub(void *, uint32) __attribute__((noreturn));
void setup_machine(void);
void shutdown_machine(void);
void setup_region(void);

#define launch(addr) \
	void (*fboot)(uint32) __attribute__((noreturn));     \
	fboot = (void *)(NONCACHED_ADDR((uint32)&boot_stub)); \
	fboot(NONCACHED_ADDR(addr))

void video_init();
void video_screenshot();
void draw_gdtex(uint8 *src);
void set_file_number(char *filename, int num);
char *relative_filename(char *filename);

void descramble(uint8 *source, uint8 *dest, uint32 size);

void *search_memory(const uint8 *key, uint32 key_size);
int patch_memory(const uint32 key, const uint32 val);
void apply_patch_list();
void rom_memcpy(void* dst, void* src, size_t cnt);

#ifndef printf
int printf(const char *fmt, ...);
#endif

uint Load_BootBin();
uint Load_IPBin(int header_only);
int Load_DS();
void Load_Syscalls();
void Load_Bleem();

#ifdef LOG

int OpenLog();
int WriteLog(const char *fmt, ...);
int WriteLogFunc(const char *func, const char *fmt, ...);
void CloseLog();

#   define LOGF(...) WriteLog(__VA_ARGS__)
#   define LOGFF(...) WriteLogFunc(__func__, __VA_ARGS__)

#	ifdef DEBUG
#		define LOG_DEBUG = 1
#		define DBGF(...) WriteLog(__VA_ARGS__)
#		define DBGFF(...) WriteLogFunc(__func__, __VA_ARGS__)
#	else
#		define DBGF(...)
#		define DBGFF(...)
#	endif

#else

#	define OpenLog()
#	define CloseLog()
#	define LOGF(...)
#	define LOGFF(...)
#	define DBGF(...)
#	define DBGFF(...)

#endif

#endif /* _ISO_LOADER */
