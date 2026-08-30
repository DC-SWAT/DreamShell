/**
 * DreamShell ISO Loader
 * NAOMI ROM support
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 *
 * Boot raw NAOMI cartridge dumps through the ISO loader (ENABLE_NAOMI).
 * The dump is never rewritten; after each load, cart PIO/DMA/G1 and window
 * copies are patched in RAM so runtime reads go through gdGdcCartRead.
 */

#include <main.h>
#include <drivers/hollysh.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <syscalls.h>
#include <naomi/cart.h>
#include <exception.h>
#ifdef DEV_TYPE_SD
#include <drivers/aica.h>
#endif

#define CART_READ_STACK_SIZE 4096

static uint32_t cart_read_stack_top;
static uint32_t cart_read_old_sp;

static naomi_state_t _naomi;
static naomi_state_t *naomi = &_naomi;

naomi_state_t *get_naomi(void) {
    return naomi;
}

static uint32_t naomi_aw_map_dest(uint32_t dst) {
    uint32_t phys = PHYS_ADDR(dst);

    if(phys >= 0x05000000 && phys < 0x06000000) {
        return NONCACHED_ADDR(0x0d100000 + (phys - 0x05000000));
    }
    return dst;
}

static void naomi_aw_fix_lits(uint8_t *p, uint32_t size) {
    uint32_t i;
    uint32_t n;
    uint32_t *w;
    uint32_t prev;

    if(size < 4) {
        return;
    }
    w = (uint32_t *)NONCACHED_ADDR((uint32_t)p);
    n = size >> 2;
    prev = 0;
    for(i = 0; i < n; i++) {
        uint32_t v = w[i];

        if((v & 0xff000000) == 0xa5000000) {
            w[i] = 0xad100000 + (v & 0x00ffffff);
        }
        else if(v == 0x05000000 && (prev & 0xff000000) == 0x8c000000) {
            w[i] = 0x0d100000;
        }
        prev = v;
    }
}


/* Trampoline: game passes (file_base, dst, size); C wants (dst, size, file_base). */
void naomi_cart_win_hook(void);
__asm__(
    ".section .text\n"
    ".global _naomi_cart_win_hook\n"
    ".type _naomi_cart_win_hook, @function\n"
    "_naomi_cart_win_hook:\n"
    "sts.l	pr, @-r15\n"
    "mov.l	r8, @-r15\n"
    "mov.l	r9, @-r15\n"
    "mov.l	r10, @-r15\n"
    "mov.l	r11, @-r15\n"
    "mov.l	r12, @-r15\n"
    "mov.l	r13, @-r15\n"
    "mov.l	r14, @-r15\n"
    "mov	r4, r0\n"
    "mov	r5, r4\n"
    "mov	r6, r5\n"
    "mov	r0, r6\n"
    "mov.l	1f, r0\n"
    "jsr	@r0\n"
    "nop\n"
    "mov.l	@r15+, r14\n"
    "mov.l	@r15+, r13\n"
    "mov.l	@r15+, r12\n"
    "mov.l	@r15+, r11\n"
    "mov.l	@r15+, r10\n"
    "mov.l	@r15+, r9\n"
    "mov.l	@r15+, r8\n"
    "lds.l	@r15+, pr\n"
    "rts\n"
    "nop\n"
    ".align	2\n"
    "1:\n"
    ".long	_naomi_cart_win_read\n"
);

#if defined(_FS_ASYNC) && !defined(DEV_TYPE_SD)
static int cart_wait_dma_busy;
static int cart_aica_async;
static int cart_stream_kick;

#define STREAM_TRACK 8

static struct {
    uint32_t dest;
    uint32_t size;
    uint32_t offset;
    int hits;
} stream_seen[STREAM_TRACK];
static int stream_next;

static int naomi_cart_dst_stream(const void *dst, uint32_t size, uint32_t offset) {
    uint32_t a = (uint32_t)dst;
    uint32_t p;
    int i;
    int slot;

    if((a >> 24) == 0) {
        return naomi->game_id == NAOMI_ID_BDS0
            || naomi->game_id == NAOMI_ID_BAC0
            || naomi->game_id == NAOMI_ID_BCV0
            || naomi->game_id == NAOMI_ID_BDF0;
    }
    if(size != 1024) {
        return 0;
    }
    p = PHYS_ADDR(a);
    if(p < 0x0c200000 || p >= 0x0d000000) {
        return 0;
    }
    slot = -1;
    for(i = 0; i < STREAM_TRACK; i++) {
        if(stream_seen[i].dest == p && stream_seen[i].size == size) {
            if(stream_seen[i].offset == offset) {
                return 0;
            }
            stream_seen[i].offset = offset;
            if(stream_seen[i].hits < 2) {
                stream_seen[i].hits++;
            }
            return stream_seen[i].hits >= 2;
        }
        if(stream_seen[i].hits == 0) {
            slot = i;
            break;
        }
        if(slot < 0 && stream_seen[i].hits < 2) {
            slot = i;
        }
    }
    if(slot < 0) {
        slot = stream_next;
        stream_next = (stream_next + 1) & (STREAM_TRACK - 1);
    }
    stream_seen[slot].dest = p;
    stream_seen[slot].size = size;
    stream_seen[slot].offset = offset;
    stream_seen[slot].hits = 1;
    return 0;
}

static int naomi_cart_ctx_blocked(void) {
    return exception_inside_int() || (irq_get_sr() & 0x10000000);
}

static int naomi_cart_use_async(const gdc_cart_read_params_t *params) {
    cart_stream_kick = 0;
    if(naomi_cart_dst_stream(params->dst_buf, params->size, params->offset)) {
        if(((uint32_t)params->dst_buf >> 24) != 0 && naomi_cart_ctx_blocked()) {
            return 0;
        }
        cart_stream_kick = 1;
        return 1;
    }
    if(!exception_inited()) {
        return 0;
    }
    if(naomi->game_id == NAOMI_ID_BCV0) {
        return 1;
    }
    if(naomi_cart_ctx_blocked()) {
        return 0;
    }
    return naomi->game_id == NAOMI_ID_BAU0 || naomi->game_id == NAOMI_ID_BAC0;
}

static void cart_dma_cb(size_t size) {
    (void)size;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
    cart_aica_async = 0;
}

static void naomi_cart_drain_dma(void) {
    volatile uint32_t *st = (volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR);

    if(cart_wait_dma_busy) {
        return;
    }
    if(!cart_aica_async && !*st && !fs_async_active(iso_fd)) {
        return;
    }
    cart_wait_dma_busy = 1;
    while(cart_aica_async || *st || fs_async_active(iso_fd)) {
        poll(iso_fd);
        if(!fs_async_active(iso_fd) && !cart_aica_async) {
            *st = 0;
            break;
        }
    }
    cart_aica_async = 0;
    cart_wait_dma_busy = 0;
}

__attribute__((used, noinline))
int naomi_cart_wait_dma(void) {
    if(cart_aica_async && naomi->game_id != NAOMI_ID_BDS0) {
        return 0;
    }
    naomi_cart_drain_dma();
    return 0;
}

#else
int naomi_cart_wait_dma(void) {
    while(*((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR))) {
    }
    return 0;
}
#endif

/* Private stack for cart reads; game SP is often too small for FAT/DEV. */
void naomi_cart_stack_setup(void) {
    uint8_t *p;

    if(cart_read_stack_top) {
        return;
    }
    p = aligned_alloc(32, CART_READ_STACK_SIZE);
    if(p) {
        cart_read_stack_top = (uint32_t)(p + CART_READ_STACK_SIZE);
        LOGFF("%08lx-%08lx\n", (uint32_t)p, cart_read_stack_top);
    }
}

#ifdef DEV_TYPE_SD
static void naomi_cart_read_sd(gdc_cart_read_params_t *params) {
    void *dst_buf = params->dst_buf;
    static uint8_t *stream_buffer = NULL;

    if(params->type) {
        fs_enable_dma(IsoInfo->emu_async);

        if((uintptr_t)params->dst_buf >> 24 == 0) {
            if(stream_buffer == NULL) {
                stream_buffer = aligned_alloc(32, params->size);
            }
            dst_buf = stream_buffer;
        }
    }
    else {
        fs_enable_dma(FS_DMA_DISABLED);
    }
    ReadSectors(dst_buf,
        params->offset / IsoInfo->sector_size,
        params->size / IsoInfo->sector_size, NULL);

    if(params->type) {
        dcache_purge_range((uintptr_t)dst_buf, params->size);
        if(dst_buf != params->dst_buf) {
            aica_dma_transfer(PHYS_ADDR((uintptr_t)stream_buffer),
                (uintptr_t)params->dst_buf, params->size);
        }
    }
}
#else
/* Sector-aligned mid: HIDDEN async (kick+return; drain via wait_dma) or SHARED.
 * Unaligned head/tail stay PIO. */
static void naomi_cart_read_ide(gdc_cart_read_params_t *params) {
    uint8_t *dst = (uint8_t *)params->dst_buf;
    uint32_t off = params->offset;
    uint32_t size = params->size;
    uint32_t ss = IsoInfo->sector_size;
    uint32_t dest_hi = ((uint32_t)dst) >> 24;
    uint32_t head = 0;
    uint32_t mid;
    uint32_t tail;
    uint32_t step = ss;

    if(params->type) {
        while(step & 0x1f) {
            step += ss;
        }
    }

    if(params->type && (((off ^ (uint32_t)dst) & 0x1f) != 0)) {
        head = size;
    }
    else if(off % ss) {
        head = ss - (off % ss);
        if(head > size) {
            head = size;
        }
    }

    mid = size - head;
    tail = mid % step;
    mid -= tail;
    if(params->type && mid && ((((uint32_t)dst + head) & 0x1f) != 0)) {
        tail += mid;
        mid = 0;
    }

    if(params->type) {
        *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 1;
        if((dest_hi & 0xf0) != 0xa0) {
            dcache_inval_range(CACHED_ADDR((uint32_t)dst), size);
        }
    }

    if(head || tail) {
        fs_enable_dma(FS_DMA_DISABLED);
        if(head) {
            lseek(iso_fd, off, SEEK_SET);
            read(iso_fd, dst, head);
            if(params->type && (dest_hi & 0xf0) != 0xa0) {
                dcache_purge_range(CACHED_ADDR((uint32_t)dst), head);
            }
        }
        if(tail) {
            lseek(iso_fd, off + head + mid, SEEK_SET);
            read(iso_fd, dst + head + mid, tail);
            if(params->type && (dest_hi & 0xf0) != 0xa0) {
                dcache_purge_range(CACHED_ADDR((uint32_t)(dst + head + mid)), tail);
            }
        }
    }

    if(mid) {
        if(params->type) {
#ifdef _FS_ASYNC
            if(naomi_cart_use_async(params)) {
                fs_enable_dma(FS_DMA_HIDDEN);
                if(ReadSectors(dst + head, (off + head) / ss, mid / ss, cart_dma_cb) == CMD_STAT_PROCESSING) {
                    if(cart_stream_kick) {
                        cart_aica_async = 1;
                        *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
                    }
                    return;
                }
            }
            if(naomi_cart_ctx_blocked()) {
                fs_enable_dma(FS_DMA_DISABLED);
            }
            else {
                fs_enable_dma(FS_DMA_SHARED);
            }
#else
            fs_enable_dma(FS_DMA_SHARED);
#endif
        }
        else {
            fs_enable_dma(FS_DMA_DISABLED);
        }
        ReadSectors(dst + head, (off + head) / ss, mid / ss, NULL);
    }

    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
}
#endif

__attribute__((noinline))
static void naomi_cart_read_work(gdc_cart_read_params_t *params) {
#ifdef LOG
    static int req_count;
    ++req_count;
    LOGF("gdGdcCartRead: %d %s %08lx %08lx %d %s\n",
        req_count,
        params->type ? "DMA" : "PIO",
        params->offset,
        (uintptr_t)params->dst_buf,
        params->size,
        exception_inside_int() ? "irq" :
        ((irq_get_sr() & 0x10000000) ? "bl" : "thd"));
#endif

#ifdef DEV_TYPE_SD
    naomi_cart_wait_dma();
    naomi_cart_read_sd(params);
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
#elif defined(_FS_ASYNC)
    naomi_cart_drain_dma();
    naomi_cart_read_ide(params);
    if(!cart_aica_async) {
        naomi_cart_wait_dma();
    }
#else
    naomi_cart_wait_dma();
    naomi_cart_read_ide(params);
    naomi_cart_wait_dma();
#endif
}

/* gdGdcCartRead entry: run work on the private stack when available. */
__attribute__((noinline))
void naomi_cart_read(gdc_cart_read_params_t *params) {
    if(naomi->aw) {
        params->dst_buf = (void *)naomi_aw_map_dest((uint32_t)params->dst_buf);
    }
    if(!cart_read_stack_top) {
        naomi_cart_stack_setup();
    }
    if(cart_read_stack_top) {
        __asm__ volatile(
            "mov.l r15, @%[old]\n\t"
            "mov %[top], r15\n\t"
            "jsr @%[fn]\n\t"
            " mov %[arg], r4\n\t"
            "mov.l @%[old], r15"
            :
            : [old] "r"(&cart_read_old_sp),
              [top] "r"(cart_read_stack_top),
              [fn] "r"(naomi_cart_read_work),
              [arg] "r"(params)
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "pr", "t", "memory"
        );
    }
    else {
        naomi_cart_read_work(params);
    }
}

/* Byte-swap 16-bit words in place (cart dumps are big-endian). */
static void naomi_bswap16(void *dst, uint32_t size) {
    uint16_t *w = (uint16_t *)dst;
    uint32_t n = size >> 1;
    uint32_t i;

    for(i = 0; i < n; i++) {
        uint16_t v = w[i];
        w[i] = (uint16_t)((v >> 8) | (v << 8));
    }
}

/* Copy a cart window from the dump. Offset = file_base + dest - first RAM window.
 * used+noinline: address is taken from asm in naomi_cart_win_hook (LTO/gc). */
__attribute__((used, noinline))
void naomi_cart_win_read(void *dst, uint32_t size, uint32_t file_base) {
    gdc_cart_read_params_t p;
    uint32_t dest = PHYS_ADDR((uint32_t)dst);
    static uint32_t ram_base;

    if(!ram_base || dest < ram_base || (dest - ram_base) > 0x01000000) {
        ram_base = dest;
    }
    p.offset = file_base + dest - ram_base;
    p.dst_buf = dst;
    p.size = size;
    p.type = 1;
    gdGdcCartRead(&p);
    naomi_cart_wait_dma();
    naomi_bswap16(dst, size);
    icache_flush_range(CACHED_ADDR((uint32_t)dst), size);
}


/* Copy the stub window table from the dump into SH-4 RAM (BAL1: 16 chunks
 * into 0x0c020000) and switch IsoInfo->exec.addr to that destination. */
static int naomi_boot_preload(uint8_t *stub, uint32_t size, uint32_t file_base) {
    uint32_t i;
    uint32_t dest;
    uint32_t *tbl;
    int n;
    int k;
    uint32_t ram;

    dest = 0;
    for(i = 0; i + 4 <= 0xe0 && i + 4 <= size; i += 4) {
        uint32_t lit = *(uint32_t *)(stub + i) & 0x1fffffff;

        if(lit >= 0x0c020000 && lit < 0x0c800000 && (lit & 0xffff) == 0) {
            dest = lit;
            break;
        }
    }
    if(!dest) {
        return 0;
    }
    tbl = NULL;
    n = 0;
    /* Copy table: 16-byte rows, first size is 0x20000. */
    for(i = 0; i + 32 <= size; i += 4) {
        int cnt = 0;

        if(*(uint32_t *)(stub + i) != 0x00020004
            || *(uint32_t *)(stub + i + 8) != 0x00020000) {
            continue;
        }
        for(k = 0; k < 16 && i + (uint32_t)(k * 16) + 12 <= size; k++) {
            uint32_t sz = *(uint32_t *)(stub + i + k * 16 + 8);

            if(sz < 0x1000 || sz > 0x80000) {
                break;
            }
            cnt++;
        }
        if(cnt >= 8) {
            tbl = (uint32_t *)(stub + i);
            n = cnt;
            break;
        }
    }
    if(!tbl) {
        return 0;
    }
    ram = dest;
    for(k = 0; k < n; k++) {
        uint32_t sz = tbl[k * 4 + 2];

        LOGF("NAOMI window preload %d: %08lx -> %08lx, %d bytes\n",
            k, file_base + (ram - dest), ram, sz);
        naomi_cart_win_read((void *)NONCACHED_ADDR(ram), sz, file_base);
        ram += sz;
    }
    naomi_patch_cart_read((uint8_t *)NONCACHED_ADDR(dest), ram - dest);
    IsoInfo->exec.addr = dest;
    LOGF("NAOMI window preload done exec %08lx\n", dest);
    return 1;
}

__attribute__((used, noinline))
void naomi_aw_gdst_read(void *dst, uint32_t size, uint32_t addr) {
    gdc_cart_read_params_t p;

    p.dst_buf = dst;
    p.size = size;
    p.offset = PHYS_ADDR(addr);
    p.type = 0;
    gdGdcCartRead(&p);
    naomi_cart_wait_dma();
    naomi_aw_fix_lits(dst, size);
    icache_flush_range(CACHED_ADDR((uint32_t)dst), size);
}

__attribute__((used, noinline))
void naomi_aw_gdst_xor(void *dst, uint32_t size, uint32_t addr, uint32_t key_off) {
    uint16_t *w;
    uint16_t *k;
    uint32_t i;
    uint32_t n;

    naomi_aw_gdst_read(dst, size, addr);
    if(!*(uint32_t *)NONCACHED_ADDR(0x0d000130)) {
        return;
    }
    k = (uint16_t *)NONCACHED_ADDR(0x0d800000 + key_off);
    w = (uint16_t *)dst;
    n = size >> 1;
    for(i = 0; i < n; i++) {
        w[i] ^= k[i];
    }
}

__attribute__((used, noinline))
void naomi_aw_boot_copy(void) {
    uint8_t *tmp = (uint8_t *)NONCACHED_ADDR(0x0d100000);
    uint8_t *copy0 = (uint8_t *)NONCACHED_ADDR(0x0c00ff00);
    uint8_t *copy1 = (uint8_t *)NONCACHED_ADDR(0x0d010000);
    gdc_cart_read_params_t p;

    *(uint32_t *)NONCACHED_ADDR(0x0d000ff0) = 0xa0000000;
    LOGF("NAOMI AW converter copy loader %08lx\n", loader_addr);
    printf("AW converter loading...\n");

    p.dst_buf = tmp;
    p.size = 0x480000;
    p.offset = 0xff00;
    p.type = 1;
    gdGdcCartRead(&p);
    naomi_cart_wait_dma();
    naomi_aw_fix_lits(tmp, 0x480000);

    p.dst_buf = copy1;
    p.size = 0x80000;
    p.offset = 0x01000000;
    p.type = 1;
    gdGdcCartRead(&p);
    naomi_cart_wait_dma();
    naomi_aw_fix_lits(copy1, 0x80000);

    irq_disable();
    memcpy(copy0, tmp, 0x480000);
    icache_flush_range(CACHED_ADDR(0x0c00ff00), 0x480000);
    memset(tmp, 0, 0x800000);
    printf("AW converter loaded\n");
}

void naomi_aw_prepare(void) {
    uint8_t *p;
    uint16_t *w;
    uint32_t n;
    uint32_t i;
    uint32_t h;

    if(!naomi->aw) {
        return;
    }
    naomi_aw_boot_copy();
    p = (uint8_t *)NONCACHED_ADDR(0x0c00ff00);
    w = (uint16_t *)p;
    n = 0x480000 >> 1;
    h = 0;
    for(i = 0; i + 8 < n; i++) {
        if(w[i] == 0x2f06 && w[i + 1] == 0xc754 && w[i + 7] == 0x4f33) {
            h = CACHED_ADDR(0x0c00ff00 + (i << 1));
            break;
        }
    }
    if(h) {
        volatile uint16_t *b = (volatile uint16_t *)NONCACHED_ADDR(RAM_START_ADDR + 0x10);
        b[0] = SH4_OPCODE_MOVL_R0_PC(4);
        b[1] = SH4_OPCODE_JMP_R0;
        b[2] = SH4_OPCODE_NOP;
        b[3] = SH4_OPCODE_NOP;
        *(volatile uint32_t *)NONCACHED_ADDR(RAM_START_ADDR + 0x18) = h;
        icache_flush_range(CACHED_ADDR(RAM_START_ADDR + 0x10), 16);
        LOGF("NAOMI AW BIOS exc %08lx\n", h);
    }
}

/* Fill the NAOMI BIOS cart-info area the game reads after boot. */
void naomi_setup_env(void) {
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DMA_STATUS_ADDR)) = 0;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_REGION_ADDR)) = IsoInfo->region > 0 ?
        IsoInfo->region - 1 : 0;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_ADVSND_ADDR)) = 1;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_MONITOR_ADDR)) = 0;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_DISPLAY_ADDR)) = 1;

    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_GAMEID_ADDR)) = naomi->game_id;
    *((volatile uint32_t *)NONCACHED_ADDR(NAOMI_CART_TEST_ADDR)) = naomi->test ? 1 : 0;

    boot_vbr = RAM_START_ADDR;
    boot_stack = RAM_START_ADDR + 0x00c00000;
    boot_sr = 0x60000101;
    LOGF("NAOMI VBR=%08lx SP=%08lx SR=%08lx\n", boot_vbr, boot_stack, boot_sr);
}

/* test_mode: 0 = follow IsoInfo, 1 = force test, 2 = force game. */
uint32_t naomi_load_bin(int test_mode) {
    naomi_cart_header_t hdr __attribute__((aligned(32)));
    const uint32_t sec_size = IsoInfo->sector_size;

    if(ReadSectors((uint8_t *)&hdr, 0, sizeof(hdr) / sec_size, NULL) != COMPLETED) {
        LOGF("Failed to read NAOMI header\n");
        return 0;
    }
    memcpy(&naomi->game_id, hdr.serial, 4);
    naomi->aw = false;
    LOGF("NAOMI test_execute_adr %08lx game_execute_adr %08lx game_id %.4s\n",
        hdr.test_execute_adr, hdr.game_execute_adr, hdr.serial);

    int use_test;

    if(test_mode == 1) {
        if(!hdr.test_execute_adr || hdr.test_execute_adr == 0xffffffff) {
            return 0;
        }
        IsoInfo->exec.addr = hdr.test_execute_adr;
        use_test = 1;
    }
    else if(test_mode == 2) {
        if(!hdr.game_execute_adr || hdr.game_execute_adr == 0xffffffff) {
            return 0;
        }
        IsoInfo->exec.addr = hdr.game_execute_adr;
        use_test = 0;
    }
    else {
        use_test = 0;
        /* Same execute addr for test and game (BAL1) must not force test. */
        if(IsoInfo->exec.addr == hdr.test_execute_adr
            && IsoInfo->exec.addr != hdr.game_execute_adr) {
            use_test = 1;
        }
        else if(IsoInfo->exec.addr == hdr.test_execute_adr
            && hdr.test_exe[0].offset != (uint32_t)-1
            && hdr.game_exe[0].offset != (uint32_t)-1
            && IsoInfo->exec.lba == (hdr.test_exe[0].offset / sec_size)
            && IsoInfo->exec.lba != (hdr.game_exe[0].offset / sec_size)) {
            use_test = 1;
        }
    }

    naomi_load_entry_t *exe = use_test ? hdr.test_exe : hdr.game_exe;
    naomi->test = use_test ? NAOMI_TEST_MENU : NAOMI_TEST_NONE;

    /* Some carts ship a different game in test_exe; load this game's test instead. */
    if(use_test && naomi_ingame_test_by_id(hdr.serial)
        && exe[0].offset != (uint32_t)-1 && exe[0].size >= 0x138) {
        uint8_t testhdr[0x140] __attribute__((aligned(32)));

        if(ReadSectors(testhdr, exe[0].offset / sec_size, 0x140 / sec_size, NULL) == COMPLETED
            && naomi_cart_valid((const naomi_cart_header_t *)testhdr)
            && memcmp(testhdr + 0x134, hdr.serial, 4) != 0) {
            LOGF("NAOMI test_exe id %.4s != %.4s, loading game test\n",
                testhdr + 0x134, hdr.serial);
            exe = hdr.game_exe;
            IsoInfo->exec.addr = hdr.game_execute_adr;
        }
    }

    LOGF("NAOMI %s mode\n", use_test ? "test" : "game");

    naomi->have_win = false;
    naomi->win_off = 0;

    uint8_t *stub = NULL;
    uint32_t stub_size = 0;

    for(int i = 0; i < 8; i++) {
        if(exe[i].offset == (uint32_t)-1) {
            break;
        }
        if(exe[i].size == 0) {
            continue;
        }

        uint32_t lba = exe[i].offset / sec_size;
        int sectors = exe[i].size / sec_size;
        uint8_t *dst = (uint8_t *)NONCACHED_ADDR((uint32_t)exe[i].dst_buf);

        LOGF("Loading %d: %08lx -> %p, %d bytes\n",
            i, exe[i].offset, exe[i].dst_buf, exe[i].size);

        if(ReadSectors(dst, lba, sectors, NULL) != COMPLETED) {
            LOGF("Failed to load NAOMI binary %d\n", i);
            return 0;
        }
        if(naomi_is_aw_stub(dst, exe[i].size)) {
            naomi_aw_patch_stub(dst, exe[i].size);
        }
        else {
            naomi_patch_cart_read(dst, exe[i].size);
            if(PHYS_ADDR((uint32_t)exe[i].dst_buf) >= 0x0d000000
                && exe[i].size > 0x1000) {
                stub = dst;
                stub_size = exe[i].size;
            }
        }
    }
    if(stub && naomi->have_win) {
        naomi_boot_preload(stub, stub_size, naomi->win_off);
    }

    /* In-game test: hook TEST into a task already in the game binary. */
    if(use_test && exe == hdr.game_exe) {
        const naomi_ingame_test_t *t = naomi_ingame_test_match(hdr.serial);

        if(t) {
            volatile uint16_t *p = (volatile uint16_t *)NONCACHED_ADDR(t->hook);
            volatile uint16_t *w = (volatile uint16_t *)NONCACHED_ADDR(t->watch);

            naomi->test = NAOMI_TEST_HOOK;
            p[0] = SH4_OPCODE_MOVL_R0_PC(4);
            p[1] = SH4_OPCODE_JMP_R0;
            p[2] = SH4_OPCODE_NOP;
            p[3] = SH4_OPCODE_NOP;
            *((volatile uint32_t *)NONCACHED_ADDR(t->hook + 8)) = t->task;
            w[0] = SH4_OPCODE_RTS;
            w[1] = SH4_OPCODE_NOP;
            *((volatile uint32_t *)NONCACHED_ADDR(t->exit)) = t->jump;
            LOGF("NAOMI in-game test %08lx -> %08lx watch %08lx exit %08lx\n",
                t->hook, t->task, t->watch, t->jump);
        }
    }

    memcpy((void *)NONCACHED_ADDR(NAOMI_BOOTID_ADDR), &hdr, 0x500);

    return 1;
}

/* Install IRQ vectors/handlers and redirect BIOS test/game enter jumps. */
void naomi_hook_boot_services(uint32_t test_enter, uint32_t game_enter) {
    volatile uint32_t *tbl = (volatile uint32_t *)NONCACHED_ADDR(NAOMI_BOOTSVC_ADDR);
    volatile uint16_t *p;
    uint32_t orig;
    uint32_t i;
    const uint32_t fn[NAOMI_BOOTSVC_JUMP_COUNT] = {
        HOLLYSH_GAME_ADDR,
        HOLLYSH_TEST_ADDR,
        HOLLYSH_TEST_ADDR,
        HOLLYSH_GAME_ADDR
    };

    uint8_t *dst = (uint8_t *)NONCACHED_ADDR(SYSCALLS_FW_ADDR);
    uint8_t *src = (uint8_t *)IsoInfo->firmware;
    LOGF("Loading IRQ vectors from %08lx to %08lx %d bytes\n",
        (uintptr_t)src, (uintptr_t)dst, 0x4f00);
    memcpy(dst, src, 0x4f00);
    dst = (uint8_t *)NONCACHED_ADDR(NAOMI_BOOTSVC_ADDR);
    LOGF("Loading IRQ handlers from %08lx to %08lx %d bytes\n",
        (uintptr_t)src + 0x4f00, (uintptr_t)dst, 0x7000);
    memcpy(dst, src + 0x4f00, 0x7000);

    *(volatile uint32_t *)NONCACHED_ADDR(NAOMI_BOOT_FLAG_ADDR) = 1;
    *(volatile uint32_t *)NONCACHED_ADDR(NAOMI_TEST_ENTER_ADDR) = test_enter;
    *(volatile uint32_t *)NONCACHED_ADDR(NAOMI_GAME_ENTER_ADDR) = game_enter;

    for(i = 0; i < NAOMI_BOOTSVC_JUMP_COUNT; i++) {
        orig = tbl[NAOMI_BOOTSVC_JUMP_INDEX + i];
        if(orig && orig != 0xffffffff) {
            p = (volatile uint16_t *)NONCACHED_ADDR(orig);
            p[0] = SH4_OPCODE_MOVL_R0_PC(4);
            p[1] = SH4_OPCODE_JMP_R0;
            p[2] = SH4_OPCODE_NOP;
            p[3] = SH4_OPCODE_NOP;
            *(volatile uint32_t *)NONCACHED_ADDR(orig + 8) = fn[i];
        }
        tbl[NAOMI_BOOTSVC_JUMP_INDEX + i] = fn[i];
    }
    naomi_bad0_test_svc();
}

void naomi_bad0_test_svc(void) {
    volatile uint32_t *tbl;
    volatile uint16_t *stub;
    uint32_t sa;

    if(!naomi->test || naomi->game_id != NAOMI_ID_BAD0) {
        return;
    }
    sa = NAOMI_BOOTSVC_ADDR + 0x7000;
    stub = (volatile uint16_t *)NONCACHED_ADDR(sa);
    stub[0] = SH4_OPCODE_MOV_0_R0;
    stub[1] = SH4_OPCODE_RTS;
    stub[2] = SH4_OPCODE_NOP;
    tbl = (volatile uint32_t *)NONCACHED_ADDR(NAOMI_BOOTSVC_ADDR);
    tbl[8] = sa;
    tbl[9] = sa;
    icache_flush_range(CACHED_ADDR(sa), 8);
    LOGF("NAOMI BAD0 test svc stub %08lx\n", sa);
}

/* Reload the NAOMI binary and jump, used when switching test <-> game. */
static void naomi_relaunch(int test_mode) {
    do {} while(pre_read_xfer_busy());
    fs_enable_dma(FS_DMA_DISABLED);
    irq_disable();
    aica_halt();
    printf(NULL);
    memset((uint32_t *)VIDEO_VRAM_START, 0, 2 * 1024 * 1024);

    if(test_mode) {
        printf("Entering test menu...\n");
    }
    else {
        printf("Entering game...\n");
    }

    memset((void *)NONCACHED_ADDR(RAM_START_ADDR + 0x20000), 0, 0x100000);

    if(!naomi_load_bin(test_mode ? 1 : 2)) {
        if(test_mode) {
            printf("Entering test menu failed, resetting...\n");
        }
        else {
            printf("Entering game failed, resetting...\n");
        }
        hollysh_bios_reset();
    }

    LOGF("NAOMI %s execute=%08lx insn=%08lx\n", test_mode ? "TEST" : "GAME",
        IsoInfo->exec.addr, *(volatile uint32_t *)NONCACHED_ADDR(IsoInfo->exec.addr));

    naomi_bad0_test_svc();
    setup_machine();
    printf("Executing...\n");
    launch(IsoInfo->exec.addr);
}

/* BIOS test-enter: toggle test/game, or ignore when in-game TEST hook is used. */
void naomi_enter_test(void) {
    if(naomi->test == NAOMI_TEST_HOOK) {
        return;
    }
    if(naomi->test) {
        naomi_relaunch(0);
    }
    else {
        naomi_relaunch(1);
    }
}

void naomi_enter_game(void) {
    naomi_relaunch(0);
}
