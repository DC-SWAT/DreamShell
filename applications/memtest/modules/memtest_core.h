/* DreamShell ##version##

   memtest_core.h - Memory test engine
   Copyright (C) 2026 SWAT
*/

#ifndef MEMTEST_CORE_H
#define MEMTEST_CORE_H

#include <stddef.h>
#include <stdint.h>

#define MEMTEST_NAME_MAX      20
#define MEMTEST_REGIONS_MAX   5
#define MEMTEST_SUBTESTS      3
#define MEMTEST_CHUNK_SIZE    (512 * 1024)
#define MEMTEST_OCRAM_BASE    0x7C001000
#define MEMTEST_ISLAND_PAD    0xC000

enum {
    MEMTEST_REGION_RAM = 0,
    MEMTEST_REGION_VRAM,
    MEMTEST_REGION_VRAM_B,
    MEMTEST_REGION_AICA,
    MEMTEST_REGION_ELAN
};

enum {
    MEMTEST_ACC_CPU = 0,
    MEMTEST_ACC_G2
};

enum {
    MEMTEST_SUB_DATABUS = 0,
    MEMTEST_SUB_ADDRBUS,
    MEMTEST_SUB_DEVICE
};

enum {
    MEMTEST_ST_IDLE = 0,
    MEMTEST_ST_SKIPPED,
    MEMTEST_ST_PASS,
    MEMTEST_ST_FAIL,
    MEMTEST_ST_RUNNING
};

typedef struct memtest_sub {
    int status;
    uint32_t fail_addr;
    uint32_t expected;
    uint32_t actual;
} memtest_sub_t;

typedef struct memtest_region {
    int type;
    int access;
    int enabled;
    int status;
    char name[MEMTEST_NAME_MAX];
    uintptr_t base;
    size_t size;
    uint32_t fail_addr;
    uint32_t msec;
    memtest_sub_t sub[MEMTEST_SUBTESTS];
} memtest_region_t;

typedef struct memtest_plan {
    const char *hw_name;
    size_t ram_size;
    size_t vram_size;
    size_t vram_b_size;
    size_t aica_size;
    size_t elan_size;
    int region_count;
    int quick;
    int cs_led;
    volatile int cancel;
    memtest_region_t regions[MEMTEST_REGIONS_MAX];
} memtest_plan_t;

void memtest_plan_init(memtest_plan_t *plan);
int memtest_run_region(memtest_plan_t *plan, int index);

#endif
