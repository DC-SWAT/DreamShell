/* KallistiOS ##version##

   elf.c
   Copyright (C)2000,2001,2003 Dan Potter
   Copyright (C)2006-2014, 2025 SWAT
*/


#include "ds.h"
#include "cmd_elf.h"

#define ARCH_CODE EM_SH
#define ELF_DEBUG 0

#if ELF_DEBUG
#	define DBG(x) printf x
#else
#	define DBG(x)
#endif

/* Finds a given symbol in a relocated ELF symbol table */
static int find_sym(char *name, elf_sym_t *table, int tablelen) {
	int i;
	for (i=0; i<tablelen; i++) {
		if (!strcmp((char*)table[i].name, name))
			return i;
	}
	return -1;
}

/* Pass in a file descriptor from the virtual file system, and the
   result will be NULL if the file cannot be loaded, or a pointer to
   the loaded and relocated executable otherwise. The second variable
   will be set to the entry point. */
/* There's a lot of shit in here that's not documented or very poorly
   documented by Intel.. I hope that this works for future compilers. */
cmd_elf_prog_t *cmd_elf_load(const char * fn) {
	uint8			*img, *imgout;
	int			sz, i, j, sect; 
    elf_hdr_t   *hdr;
    elf_shdr_t  *shdrs, *symtabhdr;
    elf_sym_t   *symtab;
	int			symtabsize;
    elf_rel_t   *reltab;
    elf_rela_t  *relatab;
	int			reltabsize;
	char			*stringtab;
	uint32			vma;
	file_t			fd;
	cmd_elf_prog_t *out;
	
	out = malloc(sizeof(cmd_elf_prog_t));
	memset(out, 0, sizeof(cmd_elf_prog_t));

	/* Load the file: needs to change to just load headers */
	fd = fs_open(fn, O_RDONLY);
	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open input file '%s'\n", fn);
		return NULL;
	}
	sz = fs_total(fd);
	DBG(("Loading ELF file of size %d\n", sz));

	img = memalign(32, sz);
	if (img == NULL) {
		ds_printf("DS_ERROR: Can't allocate %d bytes for ELF load\n", sz);
		fs_close(fd);
		return NULL;
	}
	fs_read(fd, img, sz);
	fs_close(fd);

	/* Header is at the front */
    hdr = (elf_hdr_t *)(img + 0);
	if (hdr->ident[0] != 0x7f || strncmp((const char*)hdr->ident+1, "ELF", 3)) {
		ds_printf("DS_ERROR: File is not a valid ELF file\n");
		hdr->ident[4] = 0;
		ds_printf("DS_ERROR: hdr->ident is %d/%s\n", hdr->ident[0], hdr->ident+1);
		goto error1;
	}
	if (hdr->ident[4] != 1 || hdr->ident[5] != 1) {
		ds_printf("DS_ERROR: Invalid architecture flags in ELF file\n");
		goto error1;
	}
	if (hdr->machine != ARCH_CODE) {
		ds_printf("DS_ERROR: Invalid architecture %02x in ELF file\n", hdr->machine);
		goto error1;
	}

	/* Print some debug info */
	DBG(("File size is %d bytes\n", sz));
	DBG(("	entry point	%08lx\n", hdr->entry));
	DBG(("	ph offset	%08lx\n", hdr->phoff));
	DBG(("	sh offset	%08lx\n", hdr->shoff));
	DBG(("	flags		%08lx\n", hdr->flags));
	DBG(("	ehsize		%08x\n", hdr->ehsize));
	DBG(("	phentsize	%08x\n", hdr->phentsize));
	DBG(("	phnum		%08x\n", hdr->phnum));
	DBG(("	shentsize	%08x\n", hdr->shentsize));
	DBG(("	shnum		%08x\n", hdr->shnum));
	DBG(("	shstrndx	%08x\n", hdr->shstrndx));

	/* Locate the string table; SH elf files ought to have
	   two string tables, one for section names and one for object
	   string names. We'll look for the latter. */
    shdrs = (elf_shdr_t *)(img + hdr->shoff);
	stringtab = NULL;
	for (i=0; i<hdr->shnum; i++) {
		if (shdrs[i].type == SHT_STRTAB && i != hdr->shstrndx) {
			stringtab = (char*)(img + shdrs[i].offset);
		}
	}
	if (!stringtab) {
		ds_printf("DS_ERROR: ELF contains no object string table\n");
		goto error1;
	}

	/* Locate the symbol table */
	symtabhdr = NULL;
	for (i=0; i<hdr->shnum; i++) {
		if (shdrs[i].type == SHT_SYMTAB || shdrs[i].type == SHT_DYNSYM) {
			symtabhdr = shdrs+i;
			break;
		}
	}
	if (!symtabhdr) {
		ds_printf("DS_ERROR: ELF contains no symbol table\n");
		goto error1;
	}
    symtab = (elf_sym_t *)(img + symtabhdr->offset);
    symtabsize = symtabhdr->size / sizeof(elf_sym_t);

	/* Relocate symtab entries for quick access */
	for (i=0; i<symtabsize; i++)
		symtab[i].name = (uint32)(stringtab + symtab[i].name);

	/* Build the final memory image */
	sz = 0;
	for (i=0; i<hdr->shnum; i++) {
		if (shdrs[i].flags & SHF_ALLOC) {
			shdrs[i].addr = sz;
			sz += shdrs[i].size;
			if (shdrs[i].addralign && (shdrs[i].addr % shdrs[i].addralign)) {
				uint32 orig = shdrs[i].addr;
				shdrs[i].addr = (shdrs[i].addr + shdrs[i].addralign)
					& ~(shdrs[i].addralign-1);
				sz += shdrs[i].addr - orig;
			}
		}
	}
	DBG(("Final image is %d bytes\n", sz));
	out->data = imgout = malloc(sz);
	if (out->data == NULL) {
		ds_printf("DS_ERROR: Can't allocate %d bytes for ELF program data\n", sz);
		goto error1;
	}
	out->size = sz;
	vma = (uint32)imgout;
	for (i=0; i<hdr->shnum; i++) {
		if (shdrs[i].flags & SHF_ALLOC) {
			if (shdrs[i].type == SHT_NOBITS) {
				DBG(("  setting %d bytes of zeros at %08x\n",
					shdrs[i].size, shdrs[i].addr));
				memset(imgout+shdrs[i].addr, 0, shdrs[i].size);
			}
			else {
				DBG(("  copying %d bytes from %08x to %08x\n",
					shdrs[i].size, shdrs[i].offset, shdrs[i].addr));
				memcpy(imgout+shdrs[i].addr,
					img+shdrs[i].offset,
					shdrs[i].size);
			}
		}
	}

	/* Go through and patch in any symbols that are undefined */
	for (i=1; i<symtabsize; i++) {
		export_sym_t * sym;

		/* DBG((" symbol '%s': value %04lx, size %04lx, info %02x, other %02x, shndx %04lx\n",
			(const char *)(symtab[i].name),
			symtab[i].value, symtab[i].size,
			symtab[i].info,
			symtab[i].other,
			symtab[i].shndx)); */
		if (symtab[i].shndx != SHN_UNDEF || ELF32_ST_TYPE(symtab[i].info) == STT_SECTION) {
			// DBG((" symbol '%s': skipping\n", (const char *)(symtab[i].name)));
			continue;
		}

		/* Find the symbol in our exports */
		sym = export_lookup((const char *)(symtab[i].name + ELF_SYM_PREFIX_LEN));
		if (!sym/* && strcmp((symtab[i].name + ELF_SYM_PREFIX_LEN), "start")*/) {
			ds_printf("DS_ERROR: Function '%s' is undefined. Maybe need load module?\n", (const char *)(symtab[i].name + ELF_SYM_PREFIX_LEN));
			goto error3;
		}

		/* Patch it in */
		DBG((" symbol '%s' patched to 0x%lx\n",
			(const char *)(symtab[i].name),
			sym->ptr));
		symtab[i].value = sym->ptr;
	}

	/* Process the relocations */
	reltab = NULL; relatab = NULL;
	for (i=0; i<hdr->shnum; i++) {
		if (shdrs[i].type != SHT_REL && shdrs[i].type != SHT_RELA) continue;

		sect = shdrs[i].info;
		DBG(("Relocating (%s) on section %d\n", shdrs[i].type == SHT_REL ? "SHT_REL" : "SHT_RELA", sect));

		switch (shdrs[i].type) {
		case SHT_RELA:
            relatab = (elf_rela_t *)(img + shdrs[i].offset);
            reltabsize = shdrs[i].size / sizeof(elf_rela_t);
		
			for (j=0; j<reltabsize; j++) {
				int sym;

				// XXX Does non-sh ever use RELA?
				if (ELF32_R_TYPE(relatab[j].info) != R_SH_DIR32) {
					dbglog(DBG_ERROR, "cmd_elf_load: ELF contains unknown RELA type %02x\n",
						ELF32_R_TYPE(relatab[j].info));
					goto error3;
				}

				sym = ELF32_R_SYM(relatab[j].info);
				if (symtab[sym].shndx == SHN_UNDEF) {
					DBG(("  Writing undefined RELA %08x(%08lx+%08lx) -> %08x\n",
						symtab[sym].value + relatab[j].addend,
						symtab[sym].value,
						relatab[j].addend,
						vma + shdrs[sect].addr + relatab[j].offset));
					*((uint32 *)(imgout
						+ shdrs[sect].addr
						+ relatab[j].offset))
						=	  symtab[sym].value
							+ relatab[j].addend;
				} else {
					DBG(("  Writing RELA %08x(%08x+%08x+%08x+%08x) -> %08x\n",
						vma + shdrs[symtab[sym].shndx].addr + symtab[sym].value + relatab[j].addend,
						vma, shdrs[symtab[sym].shndx].addr, symtab[sym].value, relatab[j].addend,
						vma + shdrs[sect].addr + relatab[j].offset));
					*((uint32*)(imgout
						+ shdrs[sect].addr		/* assuming 1 == .text */
						+ relatab[j].offset))
						+=	  vma
							+ shdrs[symtab[sym].shndx].addr
							+ symtab[sym].value
							+ relatab[j].addend;
				}
			}
			break;

		case SHT_REL:
            reltab = (elf_rel_t *)(img + shdrs[i].offset);
            reltabsize = shdrs[i].size / sizeof(elf_rel_t);
		
			for (j=0; j<reltabsize; j++) {
				int sym, info, pcrel;

				// XXX Does non-ia32 ever use REL?
				info = ELF32_R_TYPE(reltab[j].info);
				if (info != R_386_32 && info != R_386_PC32) {
					ds_printf("DS_ERROR: ELF contains unknown REL type %02x\n", info);
					goto error3;
				}
				pcrel = (info == R_386_PC32);

				sym = ELF32_R_SYM(reltab[j].info);
				if (symtab[sym].shndx == SHN_UNDEF) {
					uint32 value = symtab[sym].value;
					if (sect == 1 && j < 5) {
						DBG(("  Writing undefined %s %08x -> %08x",
							pcrel ? "PCREL" : "ABSREL",
							value,
							vma + shdrs[sect].addr + reltab[j].offset));
					}
					if (pcrel)
						value -= vma + shdrs[sect].addr + reltab[j].offset;

					*((uint32 *)(imgout
						+ shdrs[sect].addr
						+ reltab[j].offset))
						+= value;

					if (sect == 1 && j < 5) {
						DBG(("(%08x)\n", *((uint32 *)(imgout + shdrs[sect].addr + reltab[j].offset))));
					}
				} else {
					uint32 value = vma + shdrs[symtab[sym].shndx].addr
						+ symtab[sym].value;
					if (sect == 1 && j < 5) {
						DBG(("  Writing %s %08x(%08x+%08x+%08x) -> %08x",
							pcrel ? "PCREL" : "ABSREL",
							value,
							vma, shdrs[symtab[sym].shndx].addr, symtab[sym].value,
							vma + shdrs[sect].addr + reltab[j].offset));
					}
					if (pcrel)
						value -= vma + shdrs[sect].addr + reltab[j].offset;
					
					*((uint32*)(imgout
						+ shdrs[sect].addr
						+ reltab[j].offset))
						+= value;

					if (sect == 1 && j < 5) {
						DBG(("(%08x)\n", *((uint32 *)(imgout + shdrs[sect].addr + reltab[j].offset))));
					}
				}
			}
			break;
		
		}
	}
	if (reltab == NULL && relatab == NULL) {
		ds_printf("DS_WARNING: found no REL(A) sections; did you forget -r?\n");
	}

	/* Look for the program entry points and deal with that */
	{
		int sym;

	#define DO_ONE(symname, outp) \
		sym = find_sym(ELF_SYM_PREFIX symname, symtab, symtabsize); \
		if (sym < 0) { \
			ds_printf("DS_ERROR: ELF contains no %s()\n", symname); \
			goto error3; \
		} \
		\
		out->outp = (vma + shdrs[symtab[sym].shndx].addr \
			+ symtab[sym].value);

		DO_ONE("main", main);
		
	#undef DO_ONE
	}

	free(img);
	DBG(("elf_load final ELF stats: memory image at %p, size %08lx\n\tentry pt %p\n", out->data, out->size, out->start));
	
	/* Flush the icache for that zone */
	icache_flush_range((uint32)out->data, out->size);

	return out;

error3:
	free(out->data);

error1:
	free(img);
	return NULL;
}

/* Free a loaded ELF program */
void cmd_elf_free(cmd_elf_prog_t *prog) {
	free(prog->data);
}
