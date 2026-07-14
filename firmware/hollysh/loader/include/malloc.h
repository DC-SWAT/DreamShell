/**
 * DreamShell HollySH BIOS loader
 * Memory allocation
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stddef.h>
#include <stdint.h>

int malloc_init(void);
void malloc_stat(uint32_t *free_size, uint32_t *max_free_size);
uint32_t malloc_heap_pos(void);

void *malloc(size_t size);
void free(void *data);
void *realloc(void *data, size_t size);
void *aligned_alloc(size_t alignment, size_t size);

__END_DECLS

#endif /*__MALLOC_H */
