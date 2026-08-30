/**
 * DreamShell ISO Loader
 * NAOMI ROM patches
 * (c)2026 SWAT <http://www.dc-swat.ru>
 *
 * Find cart PIO/DMA/G1/window I/O in the loaded NAOMI binary and
 * patch it in RAM to gdGdcCartRead. The dump file is never rewritten.
 */

#include <main.h>
#include <arch/cache.h>
#include <naomi.h>

/* Redirect TEST switch into the running game (two 18 Wheeler binaries). */
static const naomi_ingame_test_t naomi_ingame_tests[] = {
    { NAOMI_ID_BBK0, 0x0c027e8c, 0x0c02cbf0, 0x0c02249c, 0x0c13c0d8, 0x0c0e5b10 },
    { NAOMI_ID_BBK0, 0x0c027dec, 0x0c02cb2c, 0x0c0223fc, 0x0c13baf8, 0x0c0e5530 }
};

const naomi_ingame_test_t *naomi_ingame_test_by_id(const char *id) {
    uint32_t i;
    uint32_t sid;

    memcpy(&sid, id, 4);
    for(i = 0; i < sizeof(naomi_ingame_tests) / sizeof(naomi_ingame_tests[0]); i++) {
        if(naomi_ingame_tests[i].id == sid) {
            return &naomi_ingame_tests[i];
        }
    }
    return NULL;
}

/* Same as by_id, but only if hook/task opcodes still match this binary build. */
const naomi_ingame_test_t *naomi_ingame_test_match(const char *id) {
    uint32_t i;
    uint32_t sid;

    memcpy(&sid, id, 4);
    for(i = 0; i < sizeof(naomi_ingame_tests) / sizeof(naomi_ingame_tests[0]); i++) {
        const naomi_ingame_test_t *t = &naomi_ingame_tests[i];

        if(t->id != sid) {
            continue;
        }
        if(*(volatile uint32_t *)NONCACHED_ADDR(t->hook) == 0xe3012fc6
            && *(volatile uint32_t *)NONCACHED_ADDR(t->task) == 0x65432fe6) {
            return t;
        }
    }
    return NULL;
}

/* If the loader was relocated, rewrite a function pointer to the live copy. */
static uint32_t naomi_reloc_addr(uint32_t fn) {
    uint32_t phys = PHYS_ADDR(fn);
    uint32_t link = PHYS_ADDR(LOADER_ADDR);
    uint32_t live = PHYS_ADDR(loader_addr);

    if(phys >= live && phys < live + loader_size + ISOLDR_PARAMS_SIZE) {
        return NONCACHED_ADDR(fn);
    }
    if(phys >= link) {
        fn = loader_addr + (phys - link);
    }
    return NONCACHED_ADDR(fn);
}

/* Patch wait pointer inside a game-specific DMA overlay (offset 52). */
static void naomi_dma_stub_set_wait(uint8_t *stub) {
    *(uint32_t *)(stub + 52) = naomi_reloc_addr((uint32_t)naomi_cart_wait_dma);
}

/* Calls gdGdcCartRead then naomi_cart_wait_dma; lit at +0x34 is the wait addr. */
static const uint8_t dma_stub_tmpl[56] = {
    0x22, 0x4f, 0xf0, 0x7f, 0x01, 0xe0, 0x42, 0x2f,
    0x51, 0x1f, 0x62, 0x1f, 0x03, 0x1f, 0xf3, 0x64,
    0x00, 0xe5, 0x00, 0xe6, 0x06, 0xd0, 0x02, 0x60,
    0x0b, 0x40, 0x10, 0xe7, 0x05, 0xd0, 0x0b, 0x40,
    0x09, 0x00, 0x09, 0x00, 0x12, 0x2f, 0x10, 0x7f,
    0x26, 0x4f, 0x0b, 0x00, 0x09, 0x00, 0x09, 0x00,
    0xbc, 0x00, 0x00, 0xac, 0xac, 0x00, 0x00, 0xac
};

/* SH-4 32-bit stores need 4-byte alignment; insert nop if the site is at +2. */
static uint32_t naomi_align4(uint8_t *dst, uint32_t start, uint32_t need, uint32_t size) {
    if(start & 2) {
        if(start + 2 + need > size) {
            return 0xffffffff;
        }
        dst[start] = 0x09;
        dst[start + 1] = 0x00;
        start += 2;
    }
    if(start + need > size) {
        return 0xffffffff;
    }
    return start;
}

/* Replace a matched cart-DMA site with dma_stub_tmpl (idempotent). */
static void naomi_dma_stub_apply(uint8_t *dst, uint32_t start) {
    if(!memcmp(dst + start, dma_stub_tmpl, 8)) {
        return;
    }
    memcpy(dst + start, dma_stub_tmpl, sizeof(dma_stub_tmpl));
    *(uint32_t *)(dst + start + 0x34) = naomi_reloc_addr((uint32_t)naomi_cart_wait_dma);
}

/* Align + idempotence check + apply; returns 1 if the site was patched. */
static int naomi_dma_stub_patch(uint8_t *dst, uint32_t start, uint32_t size) {
    start = naomi_align4(dst, start, sizeof(dma_stub_tmpl), size);
    if(start == 0xffffffff || !memcmp(dst + start, dma_stub_tmpl, 8)) {
        return 0;
    }
    naomi_dma_stub_apply(dst, start);
    return 1;
}

/* Match BIOS-style programmed-I/O cart read prologue. */
static int naomi_pio_head(const uint16_t *w, uint32_t i, uint32_t size) {
    uint32_t hi;
    uint32_t j;
    int has_bd;

    if(i + 16 > size) {
        return 0;
    }
    hi = i >> 1;
    if((w[hi] & 0xff00) != 0xd200) {
        return 0;
    }
    if(w[hi + 1] != 0x634d || w[hi + 2] != 0x2232) {
        return 0;
    }
    if((w[hi + 3] & 0xff00) != 0xd300 && (w[hi + 3] & 0xff00) != 0xd000) {
        return 0;
    }
    if((w[hi + 4] & 0xff00) != 0xd100) {
        return 0;
    }
    if(w[hi + 5] != 0x6032 && w[hi + 5] != 0x6302) {
        return 0;
    }
    if(w[hi + 6] != 0x2419) {
        return 0;
    }
    if((w[hi + 7] & 0xff00) != 0xd200) {
        return 0;
    }
    has_bd = 0;
    for(j = (i + 3) & ~3; j + 4 <= i + 0x100 && j + 4 <= size; j += 4) {
        uint32_t lit = *(uint32_t *)((const uint8_t *)w + j);

        if(lit == 0xa05f7000 || lit == 0xa05f7008 || lit == 0xa05f7004) {
            has_bd = 1;
            break;
        }
    }
    return has_bd;
}

static int naomi_eq16(const uint8_t *p, const uint16_t *sig, uint32_t n) {
    const uint16_t *s = (const uint16_t *)p;
    uint32_t i;

    for(i = 0; i < n; i++) {
        if(s[i] != sig[i]) {
            return 0;
        }
    }
    return 1;
}

/* Match cart DMA setup (size in r2). */
static int naomi_dma_head(const uint16_t *w, uint32_t i, uint32_t size) {
    uint32_t j;
    uint32_t hi;
    int has_tst = 0;

    if(i + 8 > size) {
        return 0;
    }
    hi = i >> 1;
    if(w[hi] == 0x2668 && w[hi + 1] == 0xe2e0
        && (w[hi + 2] == 0x7ffc || w[hi + 2] == 0x7ff8)) {
        return 1;
    }
    if(w[hi] != 0x2fe6) {
        return 0;
    }
    if(w[hi + 1] == 0x2668 && w[hi + 2] == 0xe2e0
        && (w[hi + 3] == 0x7ffc || w[hi + 3] == 0x7ff8)) {
        return 1;
    }
    if(w[hi + 1] != 0x7ffc && w[hi + 1] != 0x7ff8) {
        return 0;
    }
    for(j = i + 4; j + 2 <= i + 40 && j + 2 <= size; j += 2) {
        uint16_t op = w[j >> 1];

        if(!has_tst) {
            if(op == 0x2668) {
                has_tst = 1;
            }
            continue;
        }
        if(op == 0xe2e0) {
            return 1;
        }
    }
    return 0;
}

/* Tight poll of G1 DMA status. */
static int naomi_g1dma_wait(const uint16_t *w, uint32_t i, uint32_t size) {
    uint32_t hi;

    if(i + 8 > size) {
        return 0;
    }
    hi = i >> 1;
    return w[hi] == 0x6271 && w[hi + 1] == 0x622d
        && w[hi + 2] == 0x2258 && w[hi + 3] == 0x89fb;
}

/* Function that pokes Holly G1 (0xa05f74xx) to start a cart DMA. */
static int naomi_g1dma_body(const uint16_t *w, uint32_t i, uint32_t size) {
    static const uint16_t head[9] = {
        0x2fe6, 0x2fd6, 0x2fc6, 0x2fb6, 0x2fa6,
        0x2f96, 0x2f86, 0x4f22, 0x7ff8
    };
    uint32_t j;

    if(i + 18 > size) {
        return 0;
    }
    if(!naomi_eq16((const uint8_t *)w + i, head, 9)) {
        return 0;
    }
    for(j = i; j + 4 <= i + 0x100 && j + 4 <= size; j += 2) {
        if(w[j >> 1] == 0x700c && w[(j >> 1) + 1] == 0xa05f) {
            return 1;
        }
    }
    return 0;
}

static int naomi_g1_poll(uint16_t *w, uint32_t i, uint32_t size) {
    uint32_t hi;
    uint32_t k;
    uint32_t t;
    uint32_t hi2232;

    if(i + 12 > size) {
        return 0;
    }
    hi = i >> 1;
    if(w[hi] != 0x6416 || w[hi + 1] != 0x2469 || w[hi + 2] != 0x3400
        || w[hi + 4] != 0x7501 || w[hi + 5] != 0x3573) {
        return 0;
    }
    if(i >= 2 && w[hi - 1] == 0x2232) {
        hi2232 = hi - 1;
        for(k = 1; k <= 8 && hi2232 >= k; k++) {
            if(w[hi2232 - k] == 0xe500) {
                w[hi2232 - k] = SH4_OPCODE_NOP;
                for(t = hi2232 - k + 1; t < hi2232; t++) {
                    if((w[t] & 0xf000) == 0xd000) {
                        w[t] = SH4_OPCODE_NOP;
                    }
                }
                break;
            }
        }
        for(k = 1; k <= 8 && hi2232 >= k; k++) {
            if(w[hi2232 - k] == 0xe708) {
                w[hi2232 - k] = SH4_OPCODE_NOP;
                break;
            }
        }
    }
    w[hi] = SH4_OPCODE_RTS;
    w[hi + 1] = SH4_OPCODE_NOP;
    return 1;
}

/* Rewrite a G1 DMA kick so it does not wait on missing cart hardware. */
static int naomi_g1_kick(uint16_t *w, uint32_t i, uint32_t size) {
    uint32_t hi;
    uint32_t k;
    uint32_t imm;
    uint32_t st;
    uint32_t t;
    uint16_t op;

    if(i + 4 > size) {
        return 0;
    }
    hi = i >> 1;
    if(w[hi] != 0xe770) {
        return 0;
    }
    imm = 0xffffffff;
    for(k = 1; k <= 8 && hi >= k; k++) {
        op = w[hi - k];
        if(op == 0xe700 || op == 0xe400) {
            imm = hi - k;
            break;
        }
    }
    if(imm == 0xffffffff) {
        return 0;
    }
    st = 0xffffffff;
    for(t = imm + 1; t <= hi + 6 && (t << 1) + 2 <= size; t++) {
        op = w[t];
        if(op == 0x2372 || op == 0x2342 || op == 0x4223) {
            st = t;
            break;
        }
    }
    if(st == 0xffffffff || (st << 1) + 6 > size) {
        return 0;
    }
    w[imm] = (uint16_t)((w[imm] & 0xff00) | 0x0001);
    for(t = imm + 1; t < st; t++) {
        w[t] = SH4_OPCODE_NOP;
    }
    w[st + 1] = SH4_OPCODE_RTS;
    w[st + 2] = SH4_OPCODE_NOP;
    return 1;
}

/* ROM-board stub in high RAM (BAL1 etc.). Keep the first cache/CCR jsr;
 * nop later mov.l/jsr pairs in +0x10..+0x50 that talk to cart MMIO. */
static int naomi_skip_cart_boot(uint8_t *dst, uint16_t *w, uint32_t size) {
    uint32_t i;
    uint32_t lim;
    uint32_t base;
    int n = 0;

    if(((uint32_t)dst & 0x1fffffff) < 0x0d000000) {
        return 0;
    }
    lim = 0x50;
    if(lim > size) {
        lim = size;
    }
    base = (uint32_t)dst & 0x1fffffff;
    for(i = 0x10; i + 4 <= lim; i += 2) {
        uint16_t op;
        uint32_t rn;
        uint32_t lit_off;
        uint32_t fn;

        op = w[i >> 1];
        if((op & 0xf000) != 0xd000) {
            continue;
        }
        rn = (op >> 8) & 0xf;
        if(w[(i >> 1) + 1] != (uint16_t)(0x400b | (rn << 8))) {
            continue;
        }
        lit_off = (i & ~3) + 4 + (op & 0xff) * 4;
        if(lit_off + 4 > size) {
            continue;
        }
        fn = *(uint32_t *)(dst + lit_off) & 0x1fffffff;
        if(fn < base || fn >= base + size) {
            continue;
        }
        w[(i >> 1) + 1] = SH4_OPCODE_NOP;
        n++;
        LOGF("NAOMI skip cart boot jsr %08lx at %p\n", fn, dst + i);
    }
    return n;
}

/* True if lit is NAOMI cart mailbox MMIO (0xa080xxxx). */
static int naomi_cart_mbox_lit(uint32_t lit) {
    return lit >= 0xa0800000 && lit < 0xa0810000;
}

/* BAL1: nop branches that wait on cart mailbox (no board present). */
static int naomi_fix_cart_mbox(uint8_t *dst, uint32_t size) {
    uint16_t *w = (uint16_t *)dst;
    uint32_t i;
    int n = 0;

    for(i = 0; i + 8 <= size; i += 2) {
        uint16_t op = w[i >> 1];
        uint32_t disp;
        uint32_t poff;
        uint32_t lit;
        uint32_t rm;
        uint32_t rn;
        uint16_t ld;
        uint16_t tst;
        uint16_t br;
        uint32_t rn2;
        uint32_t k;
        int hit;

        if((op & 0xf000) == 0xd000) {
            disp = op & 0xff;
            poff = (i & ~3) + 4 + disp * 4;
            if(poff + 4 > size) {
                continue;
            }
            lit = *(uint32_t *)(dst + poff);
            if(!naomi_cart_mbox_lit(lit)) {
                continue;
            }
            rm = (op >> 8) & 0xf;
            ld = w[(i >> 1) + 1];
            if((ld & 0xf00f) != 0x6002 || ((ld >> 4) & 0xf) != rm) {
                continue;
            }
            rn = (ld >> 8) & 0xf;
            tst = w[(i >> 1) + 2];
            if(tst != (uint16_t)(0x2008 | (rn << 8) | (rn << 4))) {
                continue;
            }
            br = w[(i >> 1) + 3];
            if(((br & 0xff00) == 0x8900 || (br & 0xff00) == 0x8b00
                || (br & 0xff00) == 0x8f00) && (br & 0x80)) {
                w[(i >> 1) + 3] = SH4_OPCODE_NOP;
                n++;
            }
            continue;
        }

        if(op != 0xe3ff || i + 10 > size) {
            continue;
        }
        ld = w[(i >> 1) + 1];
        if((ld & 0xf00f) != 0x2002 || ((ld >> 4) & 0xf) != 3) {
            continue;
        }
        rn2 = (ld >> 8) & 0xf;
        if((w[(i >> 1) + 2] & 0xf00f) != 0x6002
            || ((w[(i >> 1) + 2] >> 4) & 0xf) != rn2
            || w[(i >> 1) + 3] != 0x88ff
            || (w[(i >> 1) + 4] & 0xff00) != 0x8b00
            || !(w[(i >> 1) + 4] & 0x80)) {
            continue;
        }
        hit = 0;
        for(k = 2; k < 0x40 && i >= k * 2; k++) {
            uint16_t p = w[(i >> 1) - k];
            uint32_t pd;
            uint32_t pp;
            uint32_t pl;

            if((p & 0xf000) != 0xd000) {
                continue;
            }
            if(((p >> 8) & 0xf) != rn2) {
                continue;
            }
            pd = p & 0xff;
            pp = ((i - k * 2) & ~3) + 4 + pd * 4;
            if(pp + 4 > size) {
                continue;
            }
            pl = *(uint32_t *)(dst + pp);
            if(naomi_cart_mbox_lit(pl)) {
                hit = 1;
                break;
            }
        }
        if(hit) {
            w[(i >> 1) + 4] = SH4_OPCODE_NOP;
            n++;
        }
    }
    return n;
}

/* Point G1 GDST (0xa05f7418) load sites at our fake cart DMA status word. */
static int naomi_replace_gdst(uint8_t *dst, uint32_t size) {
    naomi_state_t *naomi = get_naomi();
    uint16_t *w = (uint16_t *)dst;
    uint32_t i;
    int n = 0;

    if(naomi->game_id == NAOMI_ID_BCW0) {
        return 0;
    }
    for(i = 0; i + 4 <= size; i += 2) {
        uint16_t op = w[i >> 1];
        uint32_t disp;
        uint32_t poff;
        uint16_t ld;
        uint32_t rm;

        if((op & 0xf000) != 0xd000) {
            continue;
        }
        disp = op & 0xff;
        poff = (i & ~3) + 4 + disp * 4;
        if(poff + 4 > size) {
            continue;
        }
        if(*(uint32_t *)(dst + poff) != 0xa05f7418) {
            continue;
        }
        rm = (op >> 8) & 0xf;
        ld = w[(i >> 1) + 1];
        if((ld & 0xf00f) != 0x6002 || ((ld >> 4) & 0xf) != rm) {
            continue;
        }
        *(uint32_t *)(dst + poff) = 0xac0000ac;
        n++;
    }
    return n;
}

static int naomi_fix_gdst_wait(uint8_t *dst, uint32_t size) {
    naomi_state_t *naomi = get_naomi();
    uint16_t *w = (uint16_t *)dst;
    uint32_t i;
    int n = 0;

    if(naomi->game_id != NAOMI_ID_BDS0) {
        return 0;
    }
    for(i = 0; i + 12 <= size; i += 2) {
        uint16_t op = w[i >> 1];
        uint32_t disp;
        uint32_t poff;
        uint32_t lit;

        if((op & 0xff00) != 0xd000) {
            continue;
        }
        disp = op & 0xff;
        poff = (i & ~3) + 4 + disp * 4;
        if(poff + 4 > size) {
            continue;
        }
        lit = *(uint32_t *)(dst + poff);
        if(lit != 0xac0000ac && lit != 0xa05f7418) {
            continue;
        }
        if(w[(i >> 1) + 1] != 0x6202) {
            continue;
        }
        if(w[(i >> 1) + 2] != 0x2228) {
            continue;
        }
        if((w[(i >> 1) + 3] & 0xff00) != 0x8900) {
            continue;
        }
        if(w[(i >> 1) + 4] != SH4_OPCODE_RTS || w[(i >> 1) + 5] != SH4_OPCODE_MOV_0_R0) {
            continue;
        }
        w[i >> 1] = 0xe001;
        w[(i >> 1) + 1] = SH4_OPCODE_RTS;
        w[(i >> 1) + 2] = SH4_OPCODE_NOP;
        n++;
    }
    return n;
}

int naomi_is_aw_stub(uint8_t *dst, uint32_t size) {
    uint16_t *w;

    if(PHYS_ADDR((uint32_t)dst) != 0x0d000000 || size < 0x710) {
        return 0;
    }
    w = (uint16_t *)NONCACHED_ADDR((uint32_t)dst);
    if(w[0x500 >> 1] != 0xd066 || w[0x502 >> 1] != 0x400e) {
        return 0;
    }
    return 1;
}

void naomi_aw_patch_stub(uint8_t *dst, uint32_t size) {
    uint16_t *w;
    uint32_t i;
    uint32_t caddr;

    dst = (uint8_t *)NONCACHED_ADDR((uint32_t)dst);
    w = (uint16_t *)dst;

    w[0x500 >> 1] = SH4_OPCODE_NOP;
    w[0x502 >> 1] = SH4_OPCODE_NOP;
    w[0x506 >> 1] = SH4_OPCODE_NOP;
    w[0x50a >> 1] = SH4_OPCODE_NOP;
    for(i = 0x50e; i < 0x56e; i += 2) {
        w[i >> 1] = SH4_OPCODE_NOP;
    }
    w[0x58c >> 1] = SH4_OPCODE_NOP;

    w[0x111a >> 1] = SH4_OPCODE_NOP;
    w[0x111c >> 1] = 0x6533;    /* mov r3, r5 */
    w[0x111e >> 1] = SH4_OPCODE_MOVL_R0_PC(8);
    w[0x1120 >> 1] = 0x400b;    /* jsr @r0 */
    w[0x1122 >> 1] = SH4_OPCODE_NOP;
    w[0x1124 >> 1] = SH4_OPCODE_RTS;
    w[0x1126 >> 1] = SH4_OPCODE_MOV_0_R0;
    *(uint32_t *)(dst + 0x1128) = naomi_reloc_addr((uint32_t)naomi_aw_gdst_read);
    for(i = 0x112c; i < 0x1136; i += 2) {
        w[i >> 1] = SH4_OPCODE_NOP;
    }

    w[0x120e >> 1] = 0x64e3;    /* mov r14, r4 */
    w[0x1210 >> 1] = 0x6563;    /* mov r6, r5 */
    w[0x1212 >> 1] = 0x6633;    /* mov r3, r6 */
    w[0x1214 >> 1] = 0x67b3;    /* mov r11, r7 */
    w[0x1216 >> 1] = SH4_OPCODE_MOVL_R0_PC(8);
    w[0x1218 >> 1] = 0x400b;    /* jsr @r0 */
    w[0x121a >> 1] = SH4_OPCODE_NOP;
    w[0x121c >> 1] = SH4_OPCODE_RTS;
    w[0x121e >> 1] = SH4_OPCODE_MOV_0_R0;
    *(uint32_t *)(dst + 0x1220) = naomi_reloc_addr((uint32_t)naomi_aw_gdst_xor);
    for(i = 0x1224; i < 0x1232; i += 2) {
        w[i >> 1] = SH4_OPCODE_NOP;
    }

    if(*(uint32_t *)(dst + 0x6f4) == 0 && *(uint32_t *)(dst + 0x6d4) == 0xa05f6890) {
        *(uint32_t *)(dst + 0x6f4) = 0xa5000000;
        *(uint32_t *)(dst + 0x6f8) = 0xad100000;
    }

    caddr = CACHED_ADDR((uint32_t)dst);
    dcache_flush_range(caddr, size);
    icache_flush_range(caddr, size);
    get_naomi()->aw = true;
    LOGF("NAOMI AW converter stub patched\n");
}

/* Rewrite cart reads in loaded code to gdGdcCartRead. Game-specific overlays
 * run first; Jambo/Crazy Taxi return without the generic scan. */
void naomi_patch_cart_read(uint8_t *dst, uint32_t size) {
    static const uint16_t win_head[5] = {
        0x2fa6, 0x2fb6, 0x2fc6, 0x2fd6, 0x2fe6
    };
    static const uint8_t pio_stub[40] = {
        0x22, 0x4f, 0xf0, 0x7f, 0x00, 0xe0, 0x42, 0x2f,
        0x51, 0x1f, 0x62, 0x1f, 0x03, 0x1f, 0xf3, 0x64,
        0x03, 0x65, 0x03, 0x66, 0x03, 0xd0, 0x02, 0x60,
        0x0b, 0x40, 0x10, 0xe7, 0x10, 0x7f, 0x26, 0x4f,
        0x0b, 0x00, 0x09, 0x00, 0xbc, 0x00, 0x00, 0xac
    };
    static const uint8_t win_stub[8] = {
        0x02, 0xd0, 0x2b, 0x40, 0x00, 0xd4, 0x09, 0x00
    };
    uint32_t caddr = CACHED_ADDR((uint32_t)dst);
    naomi_state_t *naomi = get_naomi();
    uint16_t *w;
    uint32_t i;
    int n_pio = 0;
    int n_dma = 0;
    int n_win = 0;
    int n_boot = 0;
    int n_gdst = 0;
    int n_mbox = 0;
    int patched = 0;

    if(size < 56) {
        return;
    }
    dcache_inval_range(caddr, size);
    dst = (uint8_t *)caddr;
    /* TESTING I/O loop: 0x0708 = 1800 frames (~30s at 60Hz) -> 180 (~3s). */
    if(naomi->game_id == NAOMI_ID_BAL1
        && ((uint32_t)dst & 0x1fffffff) == 0x0c020000
        && size > 0x97e
        && *(uint16_t *)(dst + 0x97c) == 0x0708) {
        *(uint16_t *)(dst + 0x97c) = 180;
        patched = 1;
        LOGF("NAOMI BAL1 I/O test 3s\n");
    }
    if(naomi->game_id == NAOMI_ID_ABC0
        && ((uint32_t)dst & 0x1fffffff) == 0x0c021000
        && size > 0x424fe
        && *(uint16_t *)(dst + 0x424fc) == 0xaffe) {
        *(uint16_t *)(dst + 0x424fc) = SH4_OPCODE_NOP;
        *(uint16_t *)(dst + 0x424fe) = SH4_OPCODE_NOP;
        patched = 1;
        LOGF("NAOMI ABC0 boot spin nop\n");
    }
    if(naomi->game_id == NAOMI_ID_BBJ0
        && ((uint32_t)dst & 0x1fffffff) == 0x0c021000
        && size > 0x720
        && *(uint16_t *)(dst + 0x71e) == 0xaffe) {
        *(uint16_t *)(dst + 0x71e) = SH4_OPCODE_NOP;
        *(uint16_t *)(dst + 0x720) = SH4_OPCODE_NOP;
        patched = 1;
        LOGF("NAOMI BBJ0 boot spin nop\n");
    }
    if(naomi->game_id == NAOMI_ID_BBK0
        && ((uint32_t)dst & 0x1fffffff) == 0x0c020000
        && size > 0x1b2a
        && *(uint16_t *)(dst + 0x1b28) == 0xaffe) {
        *(uint16_t *)(dst + 0x1b28) = SH4_OPCODE_NOP;
        *(uint16_t *)(dst + 0x1b2a) = SH4_OPCODE_NOP;
        patched = 1;
        LOGF("NAOMI BBK0 boot spin nop\n");
    }
    /* Jambo Safari: known-good cart-read overlay, skip generic scan. */
    if(naomi->game_id == NAOMI_ID_BAU0
        && ((uint32_t)dst & 0x1fffffff) == 0x0c020000
        && size > 0x30841e) {
        static const uint8_t p0[] = { 0x09,0x00,0x09,0x00,0x09,0x00 };
        static const uint8_t p1[] = {
            0x09,0x00,0x22,0x4f,0xf0,0x7f,0x01,0xe0,0x42,0x2f,0x51,0x1f,
            0x62,0x1f,0x03,0x1f,0xf3,0x64,0x03,0x65,0x03,0x66,0x03,0xd0,
            0x02,0x60,0x0b,0x40,0x10,0xe7,0x10,0x7f,0x26,0x4f,0x0b,0x00,
            0x09,0x00,0xbc,0x00,0x00,0xac,0x09,0x00
        };
        static const uint8_t p5[] = { 0x09,0x00 };
        static const uint8_t p6[] = { 0x09,0x00,0x09,0x00,0x09,0x00,0x09,0x00 };
        static const uint8_t p7[] = { 0x0b,0x00,0x09,0x00 };
        static const uint8_t p8[] = { 0x01 };
        static const uint8_t p9[] = { 0x72,0x23,0x0b,0x00,0x09,0x00 };
        static const uint8_t p10[] = { 0x09,0x00,0x09,0x00,0x09,0x00 };
        memcpy(dst + 0x000c85c2, p0, sizeof(p0));
        memcpy(dst + 0x000c85ce, p1, sizeof(p1));
        memcpy(dst + 0x000c8638, dma_stub_tmpl, 3);
        memcpy(dst + 0x000c863c, dma_stub_tmpl + 4, 28);
        memcpy(dst + 0x000c8658, dma_stub_tmpl + 32, sizeof(dma_stub_tmpl) - 32);
        naomi_dma_stub_set_wait(dst + 0x000c8638);
        memcpy(dst + 0x000c8e08, p5, sizeof(p5));
        memcpy(dst + 0x000c8e0c, p6, sizeof(p6));
        memcpy(dst + 0x000c8e16, p7, sizeof(p7));
        memcpy(dst + 0x000c9348, p8, sizeof(p8));
        memcpy(dst + 0x000c934a, p9, sizeof(p9));
        memcpy(dst + 0x000cc0a6, p10, sizeof(p10));
        memcpy(dst + 0x0030840c, p5, sizeof(p5));
        memcpy(dst + 0x00308410, p6, sizeof(p6));
        memcpy(dst + 0x0030841a, p7, sizeof(p7));
        dcache_flush_range(caddr, size);
        icache_flush_range(caddr, size);
        LOGF("NAOMI jambo overlay applied\n");
        return;
    }
    /* Crazy Taxi: known-good cart-read overlay, skip generic scan. */
    if(naomi->game_id == NAOMI_ID_BAC0
        && ((uint32_t)dst & 0x1fffffff) == 0x0c020000
        && size > 0x4b322) {
        static const uint8_t p0[] = {
            0x09,0x00,0x09,0x00
        };
        static const uint8_t p2[] = {
            0x09,0x00,0x09,0x00,0x09,0x00,0x09,0x00,0x09,0x00
        };
        static const uint8_t p3[] = {
            0x09,0x00,0x09,0x00,0x09,0x00,0x01,0xe0,0x0b,0x00,0x09,0x00,
            0x09,0x00,0x09,0x00,0x09,0x00,0x22,0x4f,0xf0,0x7f,0x00,0xe0,
            0x42,0x2f,0x51,0x1f,0x62,0x1f,0x03,0x1f,0xf3,0x64,0x03,0x65,
            0x03,0x66,0x03,0xd0,0x02,0x60,0x0b,0x40,0x10,0xe7,0x10,0x7f,
            0x26,0x4f,0x0b,0x00,0x09,0x00,0xbc,0x00,0x00,0xac,0x09,0x00,
            0x09,0x00,0x09,0x00,0x09,0x00,0x22,0x4f,0xf0,0x7f,0x01,0xe0,
            0x42,0x2f,0x51,0x1f,0x62,0x1f,0x03,0x1f,0xf3,0x64,0x00,0xe5,
            0x00,0xe6,0x06,0xd0,0x02,0x60,0x0b,0x40,0x10,0xe7,0x05,0xd0,
            0x0b,0x40,0x09,0x00,0x09,0x00,0x12,0x2f,0x10,0x7f,0x26,0x4f,
            0x0b,0x00,0x09,0x00,0x09,0x00,0xbc,0x00,0x00,0xac,0xac,0x00,
            0x00,0xac
        };
        static const uint8_t p4[] = {
            0x09,0x00,0x09,0x00,0x09,0x00
        };
        static const uint8_t p5[] = {
            0x09,0x00,0x27,0xd3,0x09,0x00,0x32,0x22,0x0b,0x00,0x09,0x00
        };
        static const uint8_t p6[] = {
            0x01,0xe7,0x09,0x00,0x09,0x00,0x72,0x23,0x0b,0x00,0x09,0x00,
            0x09,0x00
        };
        static const uint8_t p7[] = {
            0x0b,0x00,0x09,0x00
        };
        memcpy(dst + 0x0000b416, p0, sizeof(p0));
        memcpy(dst + 0x0000b87c, p0, sizeof(p0));
        memcpy(dst + 0x0003e6e6, p2, sizeof(p2));
        memcpy(dst + 0x0004a026, p3, sizeof(p3));
        naomi_dma_stub_set_wait(dst + 0x0004a026 + sizeof(p3) - 56);
        memcpy(dst + 0x0004a120, p4, sizeof(p4));
        memcpy(dst + 0x0004a948, p5, sizeof(p5));
        memcpy(dst + 0x0004af96, p6, sizeof(p6));
        memcpy(dst + 0x0004b322, p7, sizeof(p7));
        dcache_flush_range(caddr, size);
        icache_flush_range(caddr, size);
        LOGF("NAOMI crazy taxi overlay applied\n");
        return;
    }
    /* Generic scan: PIO, DMA, G1 wait/kick, cart window memcpy. */
    w = (uint16_t *)dst;
    for(i = 0; i + 16 <= size; i += 2) {
        uint32_t start;

        if(!naomi_pio_head(w, i, size)) {
            continue;
        }
        start = naomi_align4(dst, i, sizeof(pio_stub), size);
        if(start == 0xffffffff || !memcmp(dst + start, pio_stub, 8)) {
            continue;
        }
        memcpy(dst + start, pio_stub, sizeof(pio_stub));
        n_pio++;
    }
    for(i = 0; i + sizeof(dma_stub_tmpl) <= size; i += 2) {
        if(!naomi_dma_head(w, i, size)) {
            continue;
        }
        if(naomi_dma_stub_patch(dst, i, size)) {
            n_dma++;
        }
    }
    for(i = 2; i + 8 <= size; i += 2) {
        uint32_t start;
        uint32_t hi = i >> 1;

        if(w[hi] != 0x7ffc || w[hi + 1] != 0x6ef3) {
            continue;
        }
        if(!(w[hi + 2] == 0x2668)
            && !(w[hi + 2] == 0x6873 && i + 8 <= size
                && w[hi + 3] == 0x2668)) {
            continue;
        }
        start = i;
        if(w[hi - 1] == 0x2fe6) {
            start = i - 2;
        }
        if(start >= 2 && w[(start >> 1) - 1] == 0x2f86) {
            start -= 2;
        }
        if(naomi_dma_stub_patch(dst, start, size)) {
            n_dma++;
        }
    }
    for(i = 0; i + 8 <= size; i += 2) {
        if(!naomi_g1dma_wait(w, i, size)) {
            continue;
        }
        w[(i >> 1) + 3] = SH4_OPCODE_NOP;
        n_dma++;
    }
    for(i = 0; i + sizeof(dma_stub_tmpl) <= size; i += 2) {
        if(!naomi_g1dma_body(w, i, size)) {
            continue;
        }
        if(naomi_dma_stub_patch(dst, i, size)) {
            n_dma++;
        }
    }
    for(i = 0; i + 12 <= size; i += 2) {
        if(naomi_g1_poll(w, i, size)) {
            n_dma++;
        }
    }
    for(i = 0; i + 4 <= size; i += 2) {
        if(naomi_g1_kick(w, i, size)) {
            n_dma++;
            i += 8;
        }
    }
    /* memcpy from cart window (0xa0xxxxxx, not Holly 0xa05fxxxx). */
    for(i = 0; i + 0xa0 <= size; i += 2) {
        uint32_t start;
        uint32_t off;
        uint32_t win;
        uint32_t j;
        uint32_t fn;

        if(w[i >> 1] != win_head[0] || !naomi_eq16(dst + i, win_head, 5)) {
            continue;
        }
        win = 0;
        for(j = (i + 3) & ~3; j + 4 <= i + 0xa0 && j + 4 <= size; j += 4) {
            uint32_t lit = *(uint32_t *)(dst + j);

            if(lit >= 0xa0000000 && lit < 0xa2000000
                && (lit & 0xff0000) != 0x5f0000
                && (lit & 0xffff) == 0
                && lit != 0xa0600000) {
                win = lit;
            }
        }
        if(!win) {
            continue;
        }
        start = naomi_align4(dst, i, 16, size);
        if(start == 0xffffffff || !memcmp(dst + start, win_stub, 8)) {
            continue;
        }
        off = win & 0x1fffffff;
        memcpy(dst + start, win_stub, sizeof(win_stub));
        fn = naomi_reloc_addr((uint32_t)naomi_cart_win_hook);
        *(uint32_t *)(dst + start + 8) = off;
        *(uint32_t *)(dst + start + 12) = fn;
        n_win++;
        naomi->have_win = true;
        naomi->win_off = off;
        LOGF("NAOMI cart window %08lx -> off %08lx at %p\n", win, off, dst + start);
    }
    n_boot = naomi_skip_cart_boot(dst, w, size);
    n_gdst += naomi_replace_gdst(dst, size);
    n_gdst += naomi_fix_gdst_wait(dst, size);
    if(naomi->game_id == NAOMI_ID_BAL1) {
        n_mbox = naomi_fix_cart_mbox(dst, size);
    }
    if(!n_pio && !n_dma && !n_win && !n_boot && !n_gdst && !n_mbox && !patched) {
        return;
    }
    dcache_flush_range(caddr, size);
    icache_flush_range(caddr, size);
    LOGF("NAOMI cart read patched pio %d dma %d win %d boot %d gdst %d mbox %d\n",
        n_pio, n_dma, n_win, n_boot, n_gdst, n_mbox);
}
