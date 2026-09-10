/* DreamShell ##version##

   memtest_core.c - Memory test engine
   Copyright (C) 2026 SWAT
*/

#include "memtest_core.h"

#include <arch/arch.h>
#include <dc/elan.h>
#include <dc/g2bus.h>
#include <dc/memory.h>
#include <dc/perfctr.h>
#include <dc/pvr.h>
#include <dc/spu.h>
#include <kos/cache.h>
#include <kos/irq.h>
#include <stdlib.h>
#include <string.h>

#define PATTERN_A             0xAAAAAAAA
#define PATTERN_5             0x55555555
#define ADDR_SAVE_MAX         32
#define RAM_PHYS_BASE         0x0c000000

typedef uint32_t datum;

typedef struct {
    memtest_plan_t *plan;
    memtest_region_t *reg;
    memtest_sub_t *sub;
    uintptr_t island_lo;
    uintptr_t island_hi;
    uint8_t *backup;
    size_t backup_size;
    int sub_id;
} run_ctx_t;

void memtest_ocram_set(int enable);
uint32_t memtest_call_on_ocram(uint32_t (*fn)(void *), void *arg);

static uintptr_t phys_addr(uintptr_t addr) {
    return addr & MEM_AREA_CACHE_MASK;
}

static void ocram_enter(void) {
    dcache_wback_all();
    memtest_ocram_set(1);
}

static void ocram_leave(void) {
    memtest_ocram_set(0);
    dcache_wback_all();
}

static int range_overlap(uintptr_t a, size_t asz, uintptr_t b, size_t bsz) {
    a = phys_addr(a);
    b = phys_addr(b);
    return a < (b + bsz) && b < (a + asz);
}

static void copy32(volatile datum *dst, const volatile datum *src, size_t nbytes) {
    size_t n = nbytes / sizeof(datum);

    while(n--) {
        *dst++ = *src++;
    }
}

static void g2_wait(void) {
    while(FIFO_STATUS & (FIFO_AICA | FIFO_G2 | FIFO_SH4));
}

static uint32_t g2_read(uintptr_t addr) {
    uint32_t v;

    g2_wait();
    v = *(volatile datum *)addr;
    return v;
}

static void g2_write(uintptr_t addr, uint32_t value) {
    g2_wait();
    *(volatile datum *)addr = value;
    g2_wait();
}

static uint32_t bus_read(int access, uintptr_t addr) {
    if(access == MEMTEST_ACC_G2) {
        return g2_read(addr);
    }
    return *(volatile datum *)addr;
}

static void bus_write(int access, uintptr_t addr, uint32_t value) {
    if(access == MEMTEST_ACC_G2) {
        g2_write(addr, value);
        return;
    }
    *(volatile datum *)addr = value;
}

static void backup_read(int access, uint8_t *dst, uintptr_t src, size_t nbytes) {
    volatile datum *s;
    datum *d;
    size_t n;

    if(access != MEMTEST_ACC_G2) {
        copy32((volatile datum *)dst, (const volatile datum *)src, nbytes);
        return;
    }

    s = (volatile datum *)src;
    d = (datum *)dst;
    n = nbytes / sizeof(datum);
    while(n--) {
        if(((uintptr_t)s & 31) == 0) {
            g2_wait();
        }
        *d++ = *s++;
    }
    g2_wait();
}

static void backup_write(int access, uintptr_t dst, const uint8_t *src, size_t nbytes) {
    volatile datum *d;
    const datum *s;
    size_t n;
    size_t i;

    if(access != MEMTEST_ACC_G2) {
        copy32((volatile datum *)dst, (const volatile datum *)src, nbytes);
        return;
    }

    d = (volatile datum *)dst;
    s = (const datum *)src;
    n = nbytes / sizeof(datum);
    for(i = 0; i < n; i++) {
        if((i & 7) == 0) {
            g2_wait();
        }
        *d++ = *s++;
    }
    g2_wait();
}

static void add_region(memtest_plan_t *plan, int type, int access,
        const char *name, uintptr_t base, size_t size) {
    memtest_region_t *reg;

    if(!size || plan->region_count >= MEMTEST_REGIONS_MAX) {
        return;
    }

    reg = &plan->regions[plan->region_count++];
    memset(reg, 0, sizeof(*reg));
    reg->type = type;
    reg->access = access;
    reg->enabled = 1;
    strncpy(reg->name, name, MEMTEST_NAME_MAX - 1);
    reg->name[MEMTEST_NAME_MAX - 1] = '\0';
    reg->base = base;
    reg->size = size;
}

void memtest_plan_init(memtest_plan_t *plan) {
    int hw;

    memset(plan, 0, sizeof(*plan));

    hw = hardware_sys_mode(NULL);
    plan->ram_size = HW_MEMSIZE;
    plan->vram_size = PVR_RAM_SIZE;
    plan->aica_size = SPU_RAM_SIZE;

    if(elan_present()) {
        plan->hw_name = "NAOMI 2";
        plan->elan_size = ELAN_RAM_SIZE;
        plan->vram_b_size = plan->vram_size;
    }
    else if(hw == HW_TYPE_NAOMI) {
        plan->hw_name = "NAOMI";
    }
    else {
        plan->hw_name = DBL_MEM ? "Dreamcast 32MB" : "Dreamcast";
    }

    add_region(plan, MEMTEST_REGION_AICA, MEMTEST_ACC_G2,
        "AICA RAM", SPU_RAM_UNCACHED_BASE, plan->aica_size);
    add_region(plan, MEMTEST_REGION_VRAM, MEMTEST_ACC_CPU,
        plan->vram_b_size ? "VRAM A" : "Video RAM",
        PVR_RAM_BASE, plan->vram_size);

    if(plan->vram_b_size) {
        add_region(plan, MEMTEST_REGION_VRAM_B, MEMTEST_ACC_CPU,
            "VRAM B", PVR2_RAM_BASE, plan->vram_b_size);
    }

    if(plan->elan_size) {
        add_region(plan, MEMTEST_REGION_ELAN, MEMTEST_ACC_CPU,
            "Elan RAM", ELAN_RAM_BASE, plan->elan_size);
    }

    add_region(plan, MEMTEST_REGION_RAM, MEMTEST_ACC_CPU,
        "System RAM", MEM_AREA_P2_BASE | RAM_PHYS_BASE, plan->ram_size);
}

static void fail_sub(memtest_sub_t *sub, uintptr_t addr, uint32_t expected, uint32_t actual) {
    if(sub->status == MEMTEST_ST_FAIL) {
        return;
    }
    sub->status = MEMTEST_ST_FAIL;
    sub->fail_addr = (uint32_t)addr;
    sub->expected = expected;
    sub->actual = actual;
}

static int test_databus(int access, uintptr_t addr, memtest_sub_t *sub) {
    datum orig;
    datum pattern;
    datum got;

    orig = bus_read(access, addr);

    for(pattern = 1; pattern != 0; pattern <<= 1) {
        bus_write(access, addr, pattern);
        got = bus_read(access, addr);
        if(got != pattern) {
            fail_sub(sub, addr, pattern, got);
            bus_write(access, addr, orig);
            return -1;
        }
    }

    bus_write(access, addr, orig);
    return 0;
}

static int in_skip(uintptr_t addr, uintptr_t skip_lo, uintptr_t skip_hi) {
    if(skip_hi <= skip_lo) {
        return 0;
    }
    return range_overlap(addr, sizeof(datum), skip_lo, skip_hi - skip_lo);
}

static int test_addrbus(int access, uintptr_t base, size_t nbytes,
        uintptr_t skip_lo, uintptr_t skip_hi, memtest_sub_t *sub) {
    unsigned long mask = (nbytes / sizeof(datum)) - 1;
    unsigned long offset;
    unsigned long test;
    datum orig[ADDR_SAVE_MAX];
    int nsave = 0;
    uintptr_t addr;
    datum got;

    orig[nsave++] = bus_read(access, base);

    for(offset = 1; (offset & mask) != 0 && nsave < ADDR_SAVE_MAX; offset <<= 1) {
        orig[nsave++] = bus_read(access, base + offset * sizeof(datum));
    }

    for(offset = 1; (offset & mask) != 0; offset <<= 1) {
        addr = base + offset * sizeof(datum);
        if(!in_skip(addr, skip_lo, skip_hi)) {
            bus_write(access, addr, PATTERN_A);
        }
    }

    if(!in_skip(base, skip_lo, skip_hi)) {
        bus_write(access, base, PATTERN_5);
    }

    for(offset = 1; (offset & mask) != 0; offset <<= 1) {
        addr = base + offset * sizeof(datum);
        if(in_skip(addr, skip_lo, skip_hi)) {
            continue;
        }
        got = bus_read(access, addr);
        if(got != PATTERN_A) {
            fail_sub(sub, addr, PATTERN_A, got);
            goto restore;
        }
    }

    if(!in_skip(base, skip_lo, skip_hi)) {
        bus_write(access, base, PATTERN_A);
    }

    for(test = 1; (test & mask) != 0; test <<= 1) {
        addr = base + test * sizeof(datum);
        if(in_skip(addr, skip_lo, skip_hi) || in_skip(base, skip_lo, skip_hi)) {
            continue;
        }
        bus_write(access, addr, PATTERN_5);
        got = bus_read(access, base);
        if(got != PATTERN_A) {
            fail_sub(sub, addr, PATTERN_A, got);
            goto restore;
        }
        for(offset = 1; (offset & mask) != 0; offset <<= 1) {
            if(offset == test) {
                continue;
            }
            addr = base + offset * sizeof(datum);
            if(in_skip(addr, skip_lo, skip_hi)) {
                continue;
            }
            got = bus_read(access, addr);
            if(got != PATTERN_A) {
                fail_sub(sub, addr, PATTERN_A, got);
                goto restore;
            }
        }
        bus_write(access, base + test * sizeof(datum), PATTERN_A);
    }

restore:
    nsave = 0;
    bus_write(access, base, orig[nsave++]);
    for(offset = 1; (offset & mask) != 0 && nsave < ADDR_SAVE_MAX; offset <<= 1) {
        bus_write(access, base + offset * sizeof(datum), orig[nsave++]);
    }

    return sub->status == MEMTEST_ST_FAIL ? -1 : 0;
}

static int test_device_range(int access, uintptr_t base, size_t nbytes,
        memtest_sub_t *sub) {
    unsigned long nwords = nbytes / sizeof(datum);
    unsigned long i;
    datum pattern;
    datum anti;
    datum got;
    uintptr_t addr;

    for(i = 0, pattern = 1; i < nwords; i++, pattern++) {
        bus_write(access, base + i * sizeof(datum), pattern);
    }

    for(i = 0, pattern = 1; i < nwords; i++, pattern++) {
        addr = base + i * sizeof(datum);
        got = bus_read(access, addr);
        if(got != pattern) {
            fail_sub(sub, addr, pattern, got);
            return -1;
        }
        anti = ~pattern;
        bus_write(access, addr, anti);
    }

    for(i = 0, pattern = 1; i < nwords; i++, pattern++) {
        addr = base + i * sizeof(datum);
        anti = ~pattern;
        got = bus_read(access, addr);
        if(got != anti) {
            fail_sub(sub, addr, anti, got);
            return -1;
        }
    }

    return 0;
}

static int test_device_safe(int access, uintptr_t base, size_t nbytes,
        memtest_sub_t *sub, volatile int *cancel) {
    unsigned long nwords = nbytes / sizeof(datum);
    unsigned long i;
    datum orig;
    datum pattern;
    datum got;
    uintptr_t addr;

    for(i = 0; i < nwords; i++) {
        if((i & 0xFFF) == 0 && *cancel) {
            return 1;
        }
        addr = base + i * sizeof(datum);
        orig = bus_read(access, addr);
        pattern = (datum)(i + 1);
        bus_write(access, addr, pattern);
        got = bus_read(access, addr);
        if(got != pattern) {
            fail_sub(sub, addr, pattern, got);
            bus_write(access, addr, orig);
            return -1;
        }
        pattern = ~pattern;
        bus_write(access, addr, pattern);
        got = bus_read(access, addr);
        if(got != pattern) {
            fail_sub(sub, addr, pattern, got);
            bus_write(access, addr, orig);
            return -1;
        }
        bus_write(access, addr, orig);
    }

    return 0;
}

static int chunk_is_island(const run_ctx_t *ctx, uintptr_t addr, size_t size) {
    if(ctx->island_hi <= ctx->island_lo) {
        return 0;
    }
    return range_overlap(addr, size, ctx->island_lo, ctx->island_hi - ctx->island_lo);
}

static int run_sub_on_range(run_ctx_t *ctx, int sub_id, uintptr_t base, size_t size, int destructive) {
    memtest_sub_t *sub = ctx->sub;

    if(sub_id == MEMTEST_SUB_DATABUS) {
        return test_databus(ctx->reg->access, base, sub);
    }
    if(sub_id == MEMTEST_SUB_ADDRBUS) {
        return test_addrbus(ctx->reg->access, base, size,
            ctx->island_lo, ctx->island_hi, sub);
    }
    if(ctx->plan->cancel) {
        return 1;
    }
    if(destructive) {
        return test_device_range(ctx->reg->access, base, size, sub);
    }
    return test_device_safe(ctx->reg->access, base, size, sub, &ctx->plan->cancel);
}

static uint32_t run_sub_body(void *arg) {
    run_ctx_t *ctx = arg;
    memtest_region_t *reg = ctx->reg;
    uintptr_t chunk;
    uintptr_t end;
    size_t left;
    size_t chunk_size;
    int sub_id = ctx->sub_id;
    int destructive;
    uint8_t *backup = ctx->backup;
    int rc = 0;

    end = reg->base + reg->size;

    if(sub_id != MEMTEST_SUB_DEVICE) {
        return (uint32_t)run_sub_on_range(ctx, sub_id, reg->base, reg->size, 0);
    }

    for(chunk = reg->base; chunk < end; chunk += chunk_size) {
        if(ctx->plan->cancel) {
            return 1;
        }
        left = end - chunk;
        chunk_size = left < ctx->backup_size ? left : ctx->backup_size;
        chunk_size &= ~(sizeof(datum) - 1);
        if(!chunk_size) {
            break;
        }

        destructive = !chunk_is_island(ctx, chunk, chunk_size);
        if(destructive) {
            backup_read(reg->access, backup, chunk, chunk_size);
            rc = run_sub_on_range(ctx, sub_id, chunk, chunk_size, 1);
            backup_write(reg->access, chunk, backup, chunk_size);
        }
        else {
            rc = run_sub_on_range(ctx, sub_id, chunk, chunk_size, 0);
        }
        if(rc < 0) {
            return (uint32_t)rc;
        }

        if(ctx->plan->cancel) {
            return 1;
        }
    }

    return (uint32_t)rc;
}

static uintptr_t island_lo(void) {
    uintptr_t a = (uintptr_t)memtest_plan_init;
    uintptr_t b = (uintptr_t)memtest_run_region;
    uintptr_t c = (uintptr_t)memtest_ocram_set;
    uintptr_t lo = a;

    if(b < lo) {
        lo = b;
    }
    if(c < lo) {
        lo = c;
    }
    return lo & ~31;
}

static uintptr_t island_hi(void) {
    return island_lo() + MEMTEST_ISLAND_PAD;
}

static int region_uses_ocram(const memtest_region_t *reg) {
    return reg->type == MEMTEST_REGION_RAM;
}

static void finish_sub(memtest_sub_t *sub, uint32_t rc) {
    if(sub->status == MEMTEST_ST_FAIL) {
        return;
    }
    if(rc > 0) {
        sub->status = MEMTEST_ST_SKIPPED;
        return;
    }
    sub->status = MEMTEST_ST_PASS;
}

static uint32_t run_sub_exec(run_ctx_t *ctx) {
    run_ctx_t *ocr;
    uint32_t rc;

    if(!region_uses_ocram(ctx->reg) || ctx->sub_id == MEMTEST_SUB_DATABUS) {
        return run_sub_body(ctx);
    }

    ocram_enter();
    ocr = (run_ctx_t *)MEMTEST_OCRAM_BASE;
    *ocr = *ctx;
    rc = memtest_call_on_ocram(run_sub_body, ocr);
    ocram_leave();
    return rc;
}

int memtest_run_region(memtest_plan_t *plan, int index) {
    memtest_region_t *reg;
    run_ctx_t ctx;
    uint64_t t0;
    uint32_t rc;
    int sub;
    int max_sub;
    int failed = 0;
    uint8_t *heap_backup = NULL;
    int pvr2_up = 0;

    if(!plan || index < 0 || index >= plan->region_count) {
        return -1;
    }

    reg = &plan->regions[index];
    if(!reg->enabled) {
        reg->status = MEMTEST_ST_SKIPPED;
        return 0;
    }

    if(reg->type == MEMTEST_REGION_VRAM_B) {
        if(pvr2_init() < 0) {
            reg->status = MEMTEST_ST_FAIL;
            return -1;
        }
        pvr2_up = 1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.plan = plan;
    ctx.reg = reg;
    ctx.backup_size = MEMTEST_CHUNK_SIZE;
    if(reg->type == MEMTEST_REGION_RAM) {
        ctx.island_lo = island_lo();
        ctx.island_hi = island_hi();
    }

    if(region_uses_ocram(reg)) {
        ctx.backup = (uint8_t *)PVR_RAM_BASE;
    }
    else {
        heap_backup = aligned_alloc(32, MEMTEST_CHUNK_SIZE);
        if(!heap_backup) {
            if(pvr2_up) {
                pvr2_shutdown();
            }
            reg->status = MEMTEST_ST_FAIL;
            return -1;
        }
        ctx.backup = heap_backup;
    }

    reg->status = MEMTEST_ST_RUNNING;
    t0 = perf_cntr_timer_ns();
    max_sub = plan->quick ? MEMTEST_SUB_ADDRBUS : (MEMTEST_SUBTESTS - 1);

    for(sub = 0; sub <= max_sub; sub++) {
        if(plan->cancel) {
            break;
        }
        ctx.sub_id = sub;
        ctx.sub = &reg->sub[sub];
        memset(ctx.sub, 0, sizeof(*ctx.sub));
        ctx.sub->status = MEMTEST_ST_RUNNING;

        if(reg->access == MEMTEST_ACC_G2) {
            g2_lock_scoped();
            rc = run_sub_exec(&ctx);
        }
        else {
            irq_disable_scoped();
            rc = run_sub_exec(&ctx);
        }

        finish_sub(ctx.sub, rc);
        if(ctx.sub->status == MEMTEST_ST_FAIL) {
            reg->fail_addr = ctx.sub->fail_addr;
            failed = 1;
            break;
        }
    }

    reg->msec = (uint32_t)((perf_cntr_timer_ns() - t0) / 1000000);

    if(failed) {
        reg->status = MEMTEST_ST_FAIL;
    }
    else if(plan->cancel) {
        reg->status = MEMTEST_ST_SKIPPED;
    }
    else {
        reg->status = MEMTEST_ST_PASS;
    }

    if(heap_backup) {
        free(heap_backup);
    }

    if(pvr2_up) {
        pvr2_shutdown();
    }

    return failed ? -1 : 0;
}
