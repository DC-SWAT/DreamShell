/****************************
 * DreamShell ##version##   *
 * module.c                 *
 * DreamShell modules       *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/

#include "ds.h"

#ifdef DEBUG
	#define MODULE_DEBUG
#endif

extern export_sym_t ds_symtab[];
static symtab_handler_t st_ds = {
	{ "sym/ds/kernel",
	  0,
	  0x00010000,
	  0,
	  NMMGR_TYPE_SYMTAB,
	  NMMGR_LIST_INIT },
	ds_symtab
};

extern export_sym_t gcc_symtab[];
static symtab_handler_t st_gcc = {
	{ "sym/ds/gcc",
	  0,
	  0x00010000,
	  0,
	  NMMGR_TYPE_SYMTAB,
	  NMMGR_LIST_INIT },
	gcc_symtab
};


// Get exported symbol addr
uint32 GetExportSymAddr(const char *name, const char *path) {
	export_sym_t *sym; 
	
	if(path != NULL) {
		sym = export_lookup_path(name, path);
	} else {
		sym = export_lookup(name);
	}
	
	return (sym != NULL ? sym->ptr : 0);
}

// Get exported symbol name
const char *GetExportSymName(uint32 addr) {
	export_sym_t *sym; 
	sym = export_lookup_addr(addr);
	return (sym != NULL ? sym->name : NULL);
}


int InitModules() {

	if(nmmgr_handler_add(&st_ds.nmmgr) < 0) {                                
		return -1;
	}
	if(nmmgr_handler_add(&st_gcc.nmmgr) < 0) {                                
		return -1;
	}
	return 0;
}


void ShutdownModules() {
	
//	EXPT_GUARD_BEGIN;
//
//		LIST_FOREACH(cur, &library_list, list) {
//			library_close(cur);
//		}
//
//	EXPT_GUARD_CATCH;
//		ds_printf("DS_ERROR: Failed unloading all modules\n");
//	EXPT_GUARD_END;

	nmmgr_handler_remove(&st_ds.nmmgr);
	nmmgr_handler_remove(&st_gcc.nmmgr);
}


Module_t *OpenModule(const char *fn) {

	Module_t *m;
	char full_path[NAME_MAX];
	char name[NAME_MAX / 2];
	fs_normalize_path(fn, full_path);

	char *file = strrchr(full_path, '/');
	char *mname = file + 1;
	char *ext = strrchr(file, '.') + 1;

	memset(name, 0, sizeof(name));
	strncpy(name, mname, ext - mname - 1);

#ifdef MODULE_DEBUG
	ds_printf("DS_PROCESS: Opening module '%s' type '%s' from '%s'\n", name, ext, full_path);
#endif

	EXPT_GUARD_BEGIN;

		if((m = library_open(name, full_path)) == NULL) {
			EXPT_GUARD_RETURN NULL;
		}

	EXPT_GUARD_CATCH;
		m = NULL;
#ifdef MODULE_DEBUG
		ds_printf("DS_ERROR: library_open failed\n");
#endif
	EXPT_GUARD_END;

	if(m != NULL) {
		strncpy(m->image.fn, full_path, NAME_MAX);
	}

	return m;
}


int CloseModule(Module_t *m) {
	int r;

	if(m == NULL) {
		return -1;
	}

	EXPT_GUARD_BEGIN;
		r = library_close(m);
	EXPT_GUARD_CATCH;
		r = -1;
#ifdef MODULE_DEBUG
		ds_printf("DS_ERROR: library_close failed\n");
#endif
	EXPT_GUARD_END;

	return r;
}


int PrintModuleList(int (*pf)(const char *fmt, ...)) {
	klibrary_t *cur;

	pf(" id       base        size      name\n");

	LIST_FOREACH(cur, &library_list, list) {
		pf("%04lu   %p   %08lu   %s\n", 
			cur->libid,  
			cur->image.data, 
			cur->image.size,
			cur->lib_get_name());
	}
	
	pf("--end of list--\n");
	return 0;
}
