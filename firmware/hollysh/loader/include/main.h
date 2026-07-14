/**
 * DreamShell HollySH BIOS loader
 * (c) 2025-2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef _HOLLYSH_LOADER_MAIN_H
#define _HOLLYSH_LOADER_MAIN_H

#include <arch/types.h>
#include <dc/biosfont.h>
#include <string.h>
#include <stdio.h>
#include "fs.h"
#include "malloc.h"

#ifndef BOOTLOADER_ROM_OFFSET
# define BOOTLOADER_ROM_OFFSET 0
#endif
#ifndef BOOTLOADER_ROM_SIZE
# define BOOTLOADER_ROM_SIZE 0
#endif

#ifndef LOADER_BIN_ADDR
# define LOADER_BIN_ADDR        0x8c004000
#endif

#define BIOS_ROM_ADDR           0x00000000
#define BIOS_ROM_FONT_ADDR      (BIOS_ROM_ADDR + 0x100020)
#define APP_BIN_ADDR            0x8c010000
#define RAM_END_ADDR            0x0e000000

#define PHYS_ADDR(addr)         ((addr) & 0x1fffffff)
#define CACHED_ADDR(addr)       (PHYS_ADDR(addr) | 0x80000000)
#define NONCACHED_ADDR(addr)    (PHYS_ADDR(addr) | 0xa0000000)
#define ALIGN32_ADDR(addr)      (((addr) + 0x1f) & ~0x1f)

#define HOLLY_REV_VA1           0x10
#define HOLLY_REV_VA0           0x0b
#define holly_revision()        (*(vuint32 *)NONCACHED_ADDR(0x005f689c))

#define HW_TYPE_RETAIL          0x0
#define HW_TYPE_SET5            0x9
#define HW_TYPE_NAOMI           0xa
#define hw_sys_type()           (*(vuint32 *)NONCACHED_ADDR(0x005f74b0))

#define is_dreamcast()          (hw_sys_type() == HW_TYPE_RETAIL)
#define is_naomi_2()            (*(vuint32 *)NONCACHED_ADDR(0xA8800000) == 0xE1AD0000)

#define G1_ATA_BUS_PROT_STATE_IN_PROGRESS 0x00
#define G1_ATA_BUS_PROT_STATE_FAILED      0x02
#define G1_ATA_BUS_PROT_STATE_PASSED      0x03
#define g1_ata_prot_state() (*(vuint32 *)NONCACHED_ADDR(0x005F74EC))
#define G1_ATA_BUS_PROTECTION 0x005F74E0

extern uintptr_t loader_addr;
extern uintptr_t loader_end;
extern uint32_t loader_size;
extern uint32 bfont_saved_addr;

uint8 *get_font_address(void);

extern void boot_stub(void *, uint32) __attribute__((noreturn));
void setup_machine(void);

#define launch(addr) \
	void (*fboot)(uint32) __attribute__((noreturn)); \
	fboot = (void *)(NONCACHED_ADDR((uint32)&boot_stub)); \
	fboot(NONCACHED_ADDR(addr))

#ifndef printf
int printf(const char *fmt, ...);
#endif

#ifdef LOG

int OpenLog();
int WriteLog(const char *fmt, ...);
int WriteLogFunc(const char *func, const char *fmt, ...);
void CloseLog();

#   define LOGF(...) WriteLog(__VA_ARGS__)
#   define LOGFF(...) WriteLogFunc(__func__, __VA_ARGS__)

#	ifdef DEBUG
#		define LOG_DEBUG 1
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

#endif /* _HOLLYSH_LOADER_MAIN_H */
