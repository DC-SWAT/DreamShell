/**
 * DreamShell ISO Loader
 * Memory allocation
 * (c)2022 SWAT <http://www.dc-swat.ru>
 */

#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

int malloc_init(void);
void malloc_stat(uint32 *free_size, uint32 *max_free_size);
uint32 malloc_heap_pos();

void *malloc(uint32 size);
void free(void *data);
void *realloc(void *data, uint32 size);

__END_DECLS

#endif /*__MALLOC_H */
