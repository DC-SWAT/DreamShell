/**
 * DreamShell ISO Loader
 * (c)2009-2020 SWAT <http://www.dc-swat.ru>
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

#define APP_ADDR	0xac010000
#define IP_ADDR 	0xac008000

#define SYD_DDS_FLAG_ADR   0x8c0080fc
#define SYD_DDS_FLAG_CLEAR 0x20

#define is_custom_bios() (*(uint16 *)0x00100018 != 0x4e46)
#define is_no_syscalls() (*(uint16 *)0x8c000100 != 0x2f06)

extern isoldr_info_t *IsoInfo;
extern uint32 loader_size;
extern uint32 loader_addr;
extern void boot_stub(void *, uint32) __attribute__((noreturn));

#define launch(addr) \
	void (*fboot)(uint32) __attribute__((noreturn));     \
	fboot = (void *)((uint32)(&boot_stub) | 0xa0000000); \
	fboot(addr)

void video_init();
void draw_gdtex(uint8 *src);
void video_screen_shot();

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

int malloc_init(void);
void malloc_stat(uint32 *free_size, uint32 *max_free_size);
void *malloc(uint32 size);
void free(void *data);
void *realloc(void *data, uint32 size);

#endif /* _ISO_LOADER */
