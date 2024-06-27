/**
 * DreamShell ISO Loader
 * SH4 MMU
 * (c)2015-2016, 2024 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>

#define MMUCR *((vuint32 *)0xff000010)
static int old_mmu_val = -1;

int mmu_enabled(void) {
    uint32 val = MMUCR;
    return val & 0x01;
}

void mmu_disable(void) {
    uint32 val = MMUCR;

    if (!(val & 0x01)) {
        return;
    }
    old_mmu_val = val;

    // Diable MMU
    val &= ~0x01;

    // Set SQ mode
    // val &= ~(1 << 9);

    MMUCR = val;
}

void mmu_restore() {
    if (old_mmu_val != -1) {
        MMUCR = old_mmu_val;
        old_mmu_val = -1;
    }
}
