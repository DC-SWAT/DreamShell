/** 
 * \file    module.h
 * \brief   DreamShell module system
 * \date    2006-2015
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_MODULE_H
#define _DS_MODULE_H

#include <kos.h>
#include "utils.h"

/**
 * \brief DreamShell module system
 */

typedef klibrary_t Module_t;

int InitModules();
void ShutdownModules();

Module_t *OpenModule(const char *fn);
int CloseModule(Module_t *m);
int PrintModuleList(int (*pf)(const char *fmt, ...));

#define GetModuleById   library_by_libid
#define GetModuleByName library_lookup
#define GetModuleByFileName library_lookup_fn

#define GetModuleName(m)    m->lib_get_name()
#define GetModuleVersion(m) m->lib_get_version()
#define GetModuleId(m)      library_get_libid(m)
#define GetModuleRefCnt(m)  library_get_refcnt(m)

uint32 GetExportSymAddr(const char *name, const char *path);
const char *GetExportSymName(uint32 addr);

#define GET_EXPORT_ADDR(sym_name)               GetExportSymAddr(sym_name, NULL)
#define GET_EXPORT_ADDR_EX(sym_name, path_name) GetExportSymAddr(sym_name, path_name)
#define GET_EXPORT_NAME(sym_addr)               GetExportSymName(sym_addr)


#define DEFAULT_MODULE_HEADER(name)                             \
	extern export_sym_t ds_##name##_symtab[];                   \
	static symtab_handler_t ds_##name##_hnd = {                 \
		{ "sym/ds/"#name,                                       \
		  0, 0x00010000, 0,                                     \
		  NMMGR_TYPE_SYMTAB,                                    \
		  NMMGR_LIST_INIT },                                    \
		ds_##name##_symtab                                      \
	};                                                          \
	char *lib_get_name() {                                      \
		return ds_##name##_hnd.nmmgr.pathname + 7;              \
	}                                                           \
	uint32 lib_get_version() {                                  \
		return DS_MAKE_VER(VER_MAJOR, VER_MINOR,                \
							VER_MICRO, VER_BUILD);              \
	}


#define DEFAULT_MODULE_EXPORTS(name)                            \
	DEFAULT_MODULE_HEADER(name);                                \
	int lib_open(klibrary_t * lib) {                            \
		return nmmgr_handler_add(&ds_##name##_hnd.nmmgr);       \
	}                                                           \
	int lib_close(klibrary_t * lib) {                           \
		return nmmgr_handler_remove(&ds_##name##_hnd.nmmgr);    \
	}


#define DEFAULT_MODULE_EXPORTS_CMD(name, descr)                 \
	DEFAULT_MODULE_HEADER(name);                                \
	int builtin_##name##_cmd(int argc, char *argv[]);           \
	int lib_open(klibrary_t * lib) {                            \
		AddCmd(lib_get_name(), descr,                           \
				(CmdHandler *) builtin_##name##_cmd);           \
		return nmmgr_handler_add(&ds_##name##_hnd.nmmgr);       \
	}                                                           \
	int lib_close(klibrary_t * lib) {                           \
		RemoveCmd(GetCmdByName(lib_get_name()));                \
		return nmmgr_handler_remove(&ds_##name##_hnd.nmmgr);    \
	}

#endif
