/**
 * DreamShell ISO Loader
 * SH4 MMU
 * (c)2015-2016 SWAT <http://www.dc-swat.ru>
 */

#include <string.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <dc/sq.h>
#include <main.h>

#define MMUCR *((vuint32*)0xff000010)
//static int old_irq, old_mmu;


int mmu_enabled(void) {
	uint32 val = MMUCR;
	return val & 0x01;
}

/*
void mmu_disable(void) {
	
	old_irq = irq_disable();
	
	old_mmu = MMUCR;
	uint32 val = old_mmu;
	
	// Diable MMU
	val &= ~0x01;
	
	// Set SQ mode
//	val &= ~(1 << 9);
	
	MMUCR = val;
}

void mmu_restore() {
	MMUCR = old_mmu;
	irq_restore(old_irq);
}

void enable_sq() {
	uint32 val = MMUCR;
	
	LOGFF("SQ = %d\n", val & 0x200);
	
	val &= ~(1 << 9);
	
	LOGFF("SQ = %d\n", val & 0x200);
	MMUCR = val;
	
	val = MMUCR;
	LOGFF("SQ = %d\n", val & 0x200);
}


void mmu_memcpy(void *dest, const void *src, size_t count) {

	uint32 addr = (uint32)dest;
	
	if(!(addr & 0xF0000000) && mmu_enabled()) {
		
//		LOGFF("0x%08lx -> 0x%08lx %ld bytes, MMUCR: 0x%08lx\n", (uint32)src, addr, count, MMUCR);

//		if(addr & 0x1F) {
			
////			mmu_disable();
////			memcpy(dest, src, count);
////			dcache_purge_range(addr, count);
////			mmu_restore();
			
//			void *ncdest = (void *)(addr | 0xE0000000);
//			memcpy(ncdest, src, count);
//			dcache_purge_range((uint32)ncdest, count);
			
//		} else {
////			enable_sq();
			sq_cpy(dest, src, count);
//		}
		
	} else {
		memcpy(dest, src, count);
	}
}
*/
