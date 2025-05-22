/** 
 * \file    cmd_elf.h
 * \brief   DreamShell external binary commands
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */


#include <sys/cdefs.h>
#include <arch/types.h>


/* Kernel-specific definition of a loaded ELF binary */
typedef struct cmd_elf_prog {
	void	*data;		/* Data block containing the program */
	uint32_t	size;		/* Memory image size (rounded up to page size) */

	/* Elf exports */
	uintptr_t	main;

} cmd_elf_prog_t;


/* Load an ELF binary and return the relevant data in an cmd_elf_prog_t structure. */
cmd_elf_prog_t *cmd_elf_load(const char *fn);


/* Free a loaded ELF program */
void cmd_elf_free(cmd_elf_prog_t *prog);
