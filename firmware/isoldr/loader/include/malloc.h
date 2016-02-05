/**
 * \file malloc.h
 */

#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

void malloc_init(uint8 *ptr, size_t size_in_bytes);
//void malloc_stat(uint32 *freesize, uint32 *max_freesize);
void *malloc(uint32 size);
void free(void *data);
//void *realloc(void *data, uint32 size);

__END_DECLS

#endif /*__MALLOC_H */
