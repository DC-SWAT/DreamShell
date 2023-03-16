/**
 * DreamShell ISO Loader
 * (c)2009-2023 SWAT <http://www.dc-swat.ru>
 */

#ifndef _ISO_LOADER_H
#define _ISO_LOADER_H

#if defined(LOG) && (defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD))
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

#define APP_ADDR 0x8c010000
#define IPBIN_ADDR 0x8c008000
#define GDC_SYS_ADDR 0x8c001000

#define RAM_START_ADDR 0x8c000000
#define RAM_END_ADDR 0x8d000000

#define PHYS_ADDR(addr) ((addr) & 0x0fffffff)
#define CACHED_ADDR(addr) (PHYS_ADDR(addr) | 0x80000000)
#define NONCACHED_ADDR(addr) (PHYS_ADDR(addr) | 0xa0000000)
#define ALIGN32_ADDR(addr) (((addr) + 0x1f) & ~0x1f)

#define SYD_DDS_FLAG_ADDR NONCACHED_ADDR(IPBIN_ADDR + 0xfc))
#define SYD_DDS_FLAG_CLEAR 0x20

#define is_custom_bios() (*(uint16 *)0x00100018 != 0x4e46)
#define is_no_syscalls() (*(uint16 *)(RAM_START_ADDR + 0x100) != 0x2f06)

extern isoldr_info_t *IsoInfo;
extern uint32 loader_size;
extern uint32 loader_addr;
extern uint32 loader_end;
extern void boot_stub(void *, uint32) __attribute__((noreturn));
void setup_machine_state();

#define launch(addr) \
	void (*fboot)(uint32) __attribute__((noreturn));     \
	fboot = (void *)(NONCACHED_ADDR((uint32)&boot_stub)); \
	fboot(addr)

void video_init();
void video_screenshot();
void draw_gdtex(uint8 *src);
void set_file_number(char *filename, int num);
char *relative_filename(char *filename);

void descramble(uint8 *source, uint8 *dest, uint32 size);

void *search_memory(const uint8 *key, uint32 key_size);
int patch_memory(const uint32 key, const uint32 val);
void apply_patch_list();

#ifndef printf
int printf(const char *fmt, ...);
#endif

uint Load_BootBin();
uint Load_IPBin();
void Load_DS();
void Load_Syscalls();

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
