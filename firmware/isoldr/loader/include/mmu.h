/**
 * DreamShell ISO Loader
 * SH4 MMU
 * (c)2015-2016 SWAT <http://www.dc-swat.ru>
 */

#ifndef __MMU_H__
#define __MMU_H__

int mmu_enabled(void);
void mmu_disable(void);
void mmu_restore(void);

/**
 * MMU safe memory copy
 */
void mmu_memcpy(void *dest, const void *src, size_t count);

#endif
