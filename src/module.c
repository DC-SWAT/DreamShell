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


export_sym_t *export_lookup_by_addr(uint32 addr) {
	nmmgr_handler_t	*nmmgr;
	nmmgr_list_t	*nmmgrs;
	int	i;
	symtab_handler_t *sth;

	uint dist = ~0;
	ptr_t a = addr; //(ptr_t) addr;
	export_sym_t *best = NULL;

	/* Get the name manager list */
	nmmgrs = nmmgr_get_list();

	/* Go through and look at each symtab entry */
	LIST_FOREACH(nmmgr, nmmgrs, list_ent) {
		/* Not a symtab -> ignore */
		if (nmmgr->type != NMMGR_TYPE_SYMTAB)
			continue;

		sth = (symtab_handler_t *)nmmgr;

		/* First look through the kernel table */
		for (i = 0; sth->table[i].name; i++) {

			//if (sth->table[i].ptr == (ptr_t)addr)
			//	return sth->table+i;
			if (a - sth->table[i].ptr < dist) {
				dist = a - sth->table[i].ptr;
				best = sth->table + i;
			}
		}
	}

	return best;
}

export_sym_t *export_lookup_ex(const char *name, const char *path) {
	
	nmmgr_handler_t *nmmgr;
	symtab_handler_t *sth;
	int i;

	nmmgr = nmmgr_lookup(path);
	
	if(nmmgr == NULL) {
		return NULL;
	}
	
	sth = (symtab_handler_t *)nmmgr;

	for(i = 0; sth->table[i].name; i++) {
		if(!strcmp(name, sth->table[i].name))
			return sth->table + i;
	}

	return NULL;
}


klibrary_t *library_lookup_fn(const char * fn) {
	int old;
	klibrary_t *lib;

	old = irq_disable();

	LIST_FOREACH(lib, &library_list, list) {
		if(!strncasecmp(lib->image.fn, fn, MAX_FN_LEN))
			break;
	}

	irq_restore(old);

	if(!lib)
		errno = ENOENT;

	return lib;
}


// Get exported symbol addr
uint32 GetExportSymAddr(const char *name, const char *path) {
    export_sym_t *sym; 
	
	if(path != NULL) {
		sym = export_lookup_ex(name, path);
	} else {
		sym = export_lookup(name);
	}
	
	return (sym != NULL ? sym->ptr : 0);
}

// Get exported symbol name
const char *GetExportSymName(uint32 addr) {
	export_sym_t *sym; 
	sym = export_lookup_by_addr(addr);
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
	char full_path[MAX_FN_LEN];
	char name[MAX_FN_LEN / 2];
	realpath(fn, full_path);
	
	char *file = strrchr(full_path, '/');
	char *mname = file + 1;
	char *ext = strrchr(file, '.') + 1;
	
	strncpy(name, mname, ext - mname - 1);
    
#ifdef MODULE_DEBUG
	ds_printf("DS_PROCESS: Opening module '%s' type '%s' from '%s'\n", name, ext, full_path);
#endif

    if(name != NULL && (m = library_lookup(name)) != NULL) {
#ifdef MODULE_DEBUG
		ds_printf("DS: Module '%s' already opened\n", m->lib_get_name());
#endif
		m->refcnt++;
		return m;
    }
	
	if((m = library_lookup_fn(full_path)) != NULL) {
#ifdef MODULE_DEBUG
		ds_printf("DS: Module '%s' already opened\n", m->lib_get_name());
#endif
		m->refcnt++;
		return m;
	}

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
		strncpy(m->image.fn, full_path, MAX_FN_LEN);
	}
	
	return m;
}


int CloseModule(Module_t *m) {

   if(m == NULL) {
	   return -1;
   }

   if(m->refcnt <= 0) {
	   
	   int r = -1;
	   
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
   
   return --m->refcnt;
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
