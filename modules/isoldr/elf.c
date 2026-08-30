/* DreamShell ##version##

   ELF loader for ISO Loader
   Copyright (C) 2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include <kos/elf.h>

#define ET_EXEC 2
#define ET_DYN  3

#define PT_LOAD 1

#define R_SH_NONE     0
#define R_SH_REL32    2
#define R_SH_RELATIVE 165

typedef struct elf_phdr {
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
} elf_phdr_t;

static int elf_hdr_ok(const elf_hdr_t *hdr) {
	if(hdr->ident[0] != 0x7f || memcmp(hdr->ident + 1, "ELF", 3)) {
		ds_printf("DS_ERROR: File is not a valid ELF file\n");
		return 0;
	}
	if(hdr->ident[EI_CLASS] != ELFCLASS32 || hdr->ident[EI_DATA] != ELFDATA2LSB) {
		ds_printf("DS_ERROR: Invalid ELF architecture flags\n");
		return 0;
	}
	if(hdr->machine != EM_SH) {
		ds_printf("DS_ERROR: Invalid ELF machine %02x\n", hdr->machine);
		return 0;
	}
	if(hdr->type != ET_EXEC && hdr->type != ET_DYN) {
		ds_printf("DS_ERROR: Unsupported ELF type %d\n", hdr->type);
		return 0;
	}
	return 1;
}

static int patch_dir32(uint8_t *img, size_t img_size, uint32_t off, uint32_t add) {
	uint32_t v;

	if(off + 4 > img_size) {
		ds_printf("DS_ERROR: ELF reloc offset %08lx out of image\n", (unsigned long)off);
		return -1;
	}
	memcpy(&v, img + off, 4);
	v += add;
	memcpy(img + off, &v, 4);
	return 0;
}

static int apply_rel_exec(uint8_t *img, size_t img_size, uint32_t link_base,
	uint32_t delta, const uint8_t *elf, const elf_hdr_t *hdr, const elf_shdr_t *shdrs) {
	int i, j;
	int nrel = 0;

	for(i = 0; i < hdr->shnum; i++) {
		if(shdrs[i].type != SHT_REL && shdrs[i].type != SHT_RELA) {
			continue;
		}
		if(shdrs[i].info >= hdr->shnum || !(shdrs[shdrs[i].info].flags & SHF_ALLOC)) {
			continue;
		}
		if(shdrs[i].type == SHT_REL) {
			const elf_rel_t *rel = (const elf_rel_t *)(elf + shdrs[i].offset);
			int cnt = shdrs[i].size / sizeof(elf_rel_t);

			for(j = 0; j < cnt; j++) {
				uint8_t type = ELF32_R_TYPE(rel[j].info);
				uint32_t off;

				if(type == R_SH_NONE || type == R_SH_REL32) {
					continue;
				}
				if(type != R_SH_DIR32 && type != R_SH_RELATIVE) {
					ds_printf("DS_ERROR: Unknown ELF REL type %02x\n", type);
					return -1;
				}
				if(rel[j].offset < link_base) {
					ds_printf("DS_ERROR: ELF REL address %08lx below link base\n",
						(unsigned long)rel[j].offset);
					return -1;
				}
				off = rel[j].offset - link_base;
				if(patch_dir32(img, img_size, off, delta) < 0) {
					return -1;
				}
				nrel++;
			}
		}
		else {
			const elf_rela_t *rela = (const elf_rela_t *)(elf + shdrs[i].offset);
			int cnt = shdrs[i].size / sizeof(elf_rela_t);

			for(j = 0; j < cnt; j++) {
				uint8_t type = ELF32_R_TYPE(rela[j].info);
				uint32_t off;

				if(type == R_SH_NONE || type == R_SH_REL32) {
					continue;
				}
				if(type != R_SH_DIR32 && type != R_SH_RELATIVE) {
					ds_printf("DS_ERROR: Unknown ELF RELA type %02x\n", type);
					return -1;
				}
				if(rela[j].offset < link_base) {
					ds_printf("DS_ERROR: ELF RELA address %08lx below link base\n",
						(unsigned long)rela[j].offset);
					return -1;
				}
				off = rela[j].offset - link_base;
				if(patch_dir32(img, img_size, off, delta) < 0) {
					return -1;
				}
				nrel++;
			}
		}
	}

	if(nrel == 0 && delta != 0) {
		ds_printf("DS_WARNING: ELF has no relocations, image is linked at 0x%08lx\n",
			(unsigned long)link_base);
	}
	else if(nrel > 0) {
		ds_printf("DS_PROCESS: Applied %d ELF relocations, delta 0x%08lx\n",
			nrel, (unsigned long)delta);
	}
	return 0;
}

static int load_exec(const uint8_t *elf, elf_hdr_t *hdr, uint32_t dest,
	uint8_t **out_data, size_t *out_size) {
	elf_phdr_t *phdrs;
	elf_shdr_t *shdrs;
	uint32_t vmin = 0xffffffff;
	uint32_t vmax = 0;
	uint32_t link_base;
	uint32_t delta;
	size_t img_size;
	uint8_t *img;
	int i;

	if(hdr->phnum < 1 || hdr->phoff == 0) {
		ds_printf("DS_ERROR: ELF has no program headers\n");
		return -1;
	}

	phdrs = (elf_phdr_t *)(elf + hdr->phoff);

	for(i = 0; i < hdr->phnum; i++) {
		uint32_t end;

		if(phdrs[i].type != PT_LOAD) {
			continue;
		}
		if(phdrs[i].vaddr < vmin) {
			vmin = phdrs[i].vaddr;
		}
		end = phdrs[i].vaddr + phdrs[i].memsz;
		if(end < phdrs[i].vaddr) {
			ds_printf("DS_ERROR: ELF segment overflow\n");
			return -1;
		}
		if(end > vmax) {
			vmax = end;
		}
	}

	if(vmin == 0xffffffff || vmax <= vmin) {
		ds_printf("DS_ERROR: ELF has no loadable segments\n");
		return -1;
	}
	if(vmin < ISOLDR_PARAMS_SIZE) {
		link_base = vmin;
	}
	else {
		link_base = vmin - ISOLDR_PARAMS_SIZE;
	}

	img_size = vmax - link_base;
	img = (uint8_t *)aligned_alloc(32, img_size);

	if(img == NULL) {
		ds_printf("DS_ERROR: No free memory, needed %d bytes\n", (int)img_size);
		return -1;
	}

	memset(img, 0, img_size);
	delta = dest - link_base;

	for(i = 0; i < hdr->phnum; i++) {
		uint32_t off;

		if(phdrs[i].type != PT_LOAD) {
			continue;
		}
		off = phdrs[i].vaddr - link_base;
		if(phdrs[i].filesz > 0) {
			if(off + phdrs[i].filesz > img_size) {
				ds_printf("DS_ERROR: ELF segment does not fit\n");
				free(img);
				return -1;
			}
			memcpy(img + off, elf + phdrs[i].offset, phdrs[i].filesz);
		}
	}

	shdrs = (elf_shdr_t *)(elf + hdr->shoff);
	if(apply_rel_exec(img, img_size, link_base, delta, elf, hdr, shdrs) < 0) {
		free(img);
		return -1;
	}

	ds_printf("DS_PROCESS: ELF image %d bytes, link 0x%08lx -> dest 0x%08lx\n",
		(int)img_size, (unsigned long)link_base, (unsigned long)dest);

	*out_data = img;
	*out_size = img_size;
	return 0;
}

int isoldr_elf_load(const char *path, uint32_t dest,
	uint8_t **out_data, size_t *out_size) {
	file_t fd;
	size_t sz;
	size_t rsz;
	uint8_t *elf;
	elf_hdr_t *hdr;
	int rc;

	fd = fs_open(path, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open file: %s\n", path);
		return -1;
	}

	sz = fs_total(fd);
	elf = (uint8_t *)aligned_alloc(32, sz);

	if(elf == NULL) {
		fs_close(fd);
		ds_printf("DS_ERROR: No free memory, needed %d bytes\n", (int)sz);
		return -1;
	}

	rsz = fs_read(fd, elf, sz);
	fs_close(fd);

	if(rsz != sz) {
		free(elf);
		ds_printf("DS_ERROR: Can't load %s\n", path);
		return -1;
	}

	hdr = (elf_hdr_t *)elf;

	if(!elf_hdr_ok(hdr)) {
		free(elf);
		return -1;
	}

	ds_printf("DS_PROCESS: Loading ELF %s %d bytes, dest 0x%08lx\n",
		path, (int)sz, (unsigned long)dest);

	rc = load_exec(elf, hdr, dest, out_data, out_size);

	free(elf);
	return rc;
}
