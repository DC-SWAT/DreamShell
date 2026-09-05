/** \file    naomi/cart.c
    \brief   NAOMI cartridge ROM access.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#include <stdalign.h>
#include <string.h>
#include <arch/cache.h>
#include <dc/asic.h>
#include <dc/g1ata.h>
#include <dc/memory.h>
#include <kos/irq.h>
#include <kos/sem.h>
#include <kos/thread.h>

#include <naomi/cart.h>

#define CART_IN16(addr)  (*(volatile uint16_t *)(addr))
#define CART_OUT16(addr, data) do { *(volatile uint16_t *)(addr) = (uint16_t)(data); } while(0)
#define CART_IN32(addr)  (*(volatile uint32_t *)(addr))
#define CART_OUT32(addr, data) do { *(volatile uint32_t *)(addr) = (data); } while(0)

#define G1_ATA_DMA_ADDRESS      0xA05F7404
#define G1_ATA_DMA_LENGTH       0xA05F7408
#define G1_ATA_DMA_DIRECTION    0xA05F740C
#define G1_ATA_DMA_ENABLE       0xA05F7414
#define G1_ATA_DMA_STATUS       0xA05F7418
#define G1_ATA_DMA_LENGTH_STAT  0xA05F74F8
#define G1_RRC                  0xA05F7480
#define G1_RWC                  0xA05F7484
#define G1_FRC                  0xA05F7488
#define G1_FWC                  0xA05F748C
#define G1_ATA_PIO_RACCESS_WAIT 0xA05F7490
#define G1_ATA_PIO_WACCESS_WAIT 0xA05F7494
#define G1_ATA_DMA_RACCESS_WAIT 0xA05F74A0
#define G1_ATA_DMA_WACCESS_WAIT 0xA05F74A4
#define G1_CRDYC                0xA05F74B4
#define G1_DMA_TO_MEMORY        1

#define CART_G1_ATA_PIO      0x00000222
#define CART_G1_ATA_DMA      0x00001001
#define CART_G1_WAIT_ROM     0x00000600
#define CART_G1_WAIT_FLASH   0x00000200
#define CART_G1_WAIT_PIO     0x00000511
#define CART_G1_WAIT_DMA     0x00001006
#define CART_G1_CRDYC_ATA    0x00000001
#define CART_DMA_MAX         (0xFFFF * NAOMI_CART_DMA_UNIT)
#define CART_PROBE_STEP      NAOMI_CART_PROBE_STEP
#define CART_HDR_SCAN_MAX    NAOMI_CART_HDR_SCAN_MAX
#define CART_ROM_IDLE_WORD   0xFFFF
#define CART_ROM_ZERO_WORD   0x0000
#define CART_DMA_TIMEOUT_MS  2000
#define CART_DMA_HOLD        (NAOMI_CART_ADDR_AUTO | NAOMI_CART_ADDR_CRYPT)
#define CART_M4_ID_MAGIC     0x5500
#define CART_M4_PIC_BIT      0x2000
#define CART_PIO_STROBE      0x0001
#define CART_CFI_QUERY_ADDR  0x00AA
#define CART_CFI_QUERY_CMD   0x0098
#define CART_CFI_RESET_CMD   0x00F0
#define CART_CFI_QRY_OFF     0x20
#define CART_CFI_SIZE_OFF    0x4E
#define CART_ROM_EMPTY_BYTE  0xFF
#define CART_ROM_MODE_MASK   (~(uint32_t)NAOMI_CART_ADDR_MASK)

static void cart_off_out(uint32_t addr, uint16_t data) {
    CART_OUT32(addr, data);
}

static void cart_reg_out(uint32_t addr, uint16_t data) {
    CART_OUT16(addr, data);
}

static uint16_t cart_reg_in(uint32_t addr) {
    return CART_IN16(addr);
}

static bool cart_m4_id_ok(uint16_t id) {
    return (id & 0xFF00) == CART_M4_ID_MAGIC && (id & 0x00FF) != 0xFF;
}

static uint32_t cart_wait_rrc = CART_G1_WAIT_ROM;
static uint32_t cart_wait_rwc = CART_G1_WAIT_ROM;
static uint32_t cart_wait_frc = CART_G1_WAIT_FLASH;
static uint32_t cart_wait_fwc = CART_G1_WAIT_FLASH;
static uint32_t cart_wait_crc = CART_G1_WAIT_PIO;
static uint32_t cart_wait_cwc = CART_G1_WAIT_PIO;
static uint32_t cart_wait_gdrc = CART_G1_WAIT_DMA;
static uint32_t cart_wait_gdwc = CART_G1_WAIT_DMA;
static bool cart_m4_pio;
static volatile int cart_dma_in_progress;
static volatile int cart_dma_err;
static int cart_dma_irq_hooked;
static semaphore_t cart_dma_done = SEM_INITIALIZER(0);
static asic_evt_handler_entry_t cart_old_dma_irq;
static asic_evt_handler_entry_t cart_old_dma_over;
static asic_evt_handler_entry_t cart_old_dma_ill;

static void cart_g1_ata_mode(void);
static void cart_pio_begin(void);
static bool cart_rom_alive(void);
static bool cart_probe_hdr(const uint8_t *buf);
static bool cart_probe_empty(const uint8_t *buf);
static bool cart_probe_fetch(uint32_t addr, uint8_t *buf, uint32_t flags);

static bool cart_lock(void) {
    if(g1_ata_mutex_lock() != 0) {
        return false;
    }
    cart_pio_begin();
    if(!cart_rom_alive()) {
        cart_g1_ata_mode();
        g1_ata_mutex_unlock();
        return false;
    }
    return true;
}

static void cart_g1_stop_dma(void) {
    CART_OUT32(G1_ATA_DMA_STATUS, 0);
    CART_OUT32(G1_ATA_DMA_ENABLE, 0);
}

static void cart_dma_irq_fwd(const asic_evt_handler_entry_t *old, uint32_t code) {
    if(old->hdl) {
        old->hdl(code, old->data);
    }
}

static void cart_dma_irq_hnd(uint32_t code, void *data) {
    (void)data;

    if(cart_dma_in_progress) {
        cart_dma_err = (code != ASIC_EVT_GD_DMA);
        cart_dma_in_progress = 0;
        sem_signal(&cart_dma_done);
        thd_schedule(true);
        return;
    }
    if(code == ASIC_EVT_GD_DMA) {
        cart_dma_irq_fwd(&cart_old_dma_irq, code);
    }
    else if(code == ASIC_EVT_GD_DMA_OVERRUN) {
        cart_dma_irq_fwd(&cart_old_dma_over, code);
    }
    else {
        cart_dma_irq_fwd(&cart_old_dma_ill, code);
    }
}

static void cart_dma_irq_hook(void) {
    if(cart_dma_irq_hooked) {
        return;
    }
    cart_old_dma_irq = asic_evt_set_handler(ASIC_EVT_GD_DMA, cart_dma_irq_hnd, NULL);
    cart_old_dma_over = asic_evt_set_handler(ASIC_EVT_GD_DMA_OVERRUN, cart_dma_irq_hnd, NULL);
    cart_old_dma_ill = asic_evt_set_handler(ASIC_EVT_GD_DMA_ILLADDR, cart_dma_irq_hnd, NULL);
    if(cart_old_dma_irq.hdl == NULL) {
        asic_evt_enable(ASIC_EVT_GD_DMA, ASIC_IRQB);
        asic_evt_enable(ASIC_EVT_GD_DMA_OVERRUN, ASIC_IRQB);
        asic_evt_enable(ASIC_EVT_GD_DMA_ILLADDR, ASIC_IRQB);
    }
    cart_dma_irq_hooked = 1;
}

static bool cart_dma_abort(void) {
    int done;

    {
        irq_disable_scoped();
        done = !cart_dma_in_progress;
        if(!done) {
            cart_dma_in_progress = 0;
            cart_g1_stop_dma();
        }
    }
    if(done) {
        return sem_wait(&cart_dma_done) == 0 && !cart_dma_err;
    }
    return false;
}

static bool cart_dma_wait(void) {
    if(sem_wait_timed(&cart_dma_done, CART_DMA_TIMEOUT_MS) == 0) {
        return !cart_dma_err;
    }
    return cart_dma_abort();
}

static void cart_g1_apply_waits(void) {
    CART_OUT32(G1_RRC, cart_wait_rrc);
    CART_OUT32(G1_RWC, cart_wait_rwc);
    CART_OUT32(G1_FRC, cart_wait_frc);
    CART_OUT32(G1_FWC, cart_wait_fwc);
    CART_OUT32(G1_ATA_PIO_RACCESS_WAIT, cart_wait_crc);
    CART_OUT32(G1_ATA_PIO_WACCESS_WAIT, cart_wait_cwc);
    CART_OUT32(G1_ATA_DMA_RACCESS_WAIT, cart_wait_gdrc);
    CART_OUT32(G1_ATA_DMA_WACCESS_WAIT, cart_wait_gdwc);
}

static void cart_dma_hold(void) {
    cart_off_out(NAOMI_CART_DMA_OFFSETH, (uint16_t)(CART_DMA_HOLD >> 16));
}

static void cart_fpga_idle(void) {
    cart_off_out(NAOMI_CART_DMA_COUNT, 0);
    cart_off_out(NAOMI_CART_DMA_OFFSETL, 0);
    cart_dma_hold();
}

static void cart_g1_ata_mode(void) {
    cart_g1_stop_dma();
    cart_fpga_idle();
    CART_OUT32(G1_ATA_PIO_RACCESS_WAIT, CART_G1_ATA_PIO);
    CART_OUT32(G1_ATA_PIO_WACCESS_WAIT, CART_G1_ATA_PIO);
    CART_OUT32(G1_ATA_DMA_RACCESS_WAIT, CART_G1_ATA_DMA);
    CART_OUT32(G1_ATA_DMA_WACCESS_WAIT, CART_G1_ATA_DMA);
    CART_OUT32(G1_CRDYC, CART_G1_CRDYC_ATA);
}

static void cart_unlock(void) {
    cart_g1_ata_mode();
    g1_ata_mutex_unlock();
}

static void cart_pio_begin(void) {
    cart_g1_stop_dma();
    cart_g1_apply_waits();
    cart_fpga_idle();
    cart_m4_pio = cart_m4_id_ok(cart_reg_in(NAOMI_CART_M4_ID));
}

void naomi_cart_apply_g1_timing(const naomi_cart_header_t *hdr) {
    if(!hdr || !hdr->g1bus_init_flag) {
        cart_wait_rrc = CART_G1_WAIT_ROM;
        cart_wait_rwc = CART_G1_WAIT_ROM;
        cart_wait_frc = CART_G1_WAIT_FLASH;
        cart_wait_fwc = CART_G1_WAIT_FLASH;
        cart_wait_crc = CART_G1_WAIT_PIO;
        cart_wait_cwc = CART_G1_WAIT_PIO;
        cart_wait_gdrc = CART_G1_WAIT_DMA;
        cart_wait_gdwc = CART_G1_WAIT_DMA;
    }
    else {
        cart_wait_rrc = hdr->SB_G1RRC_val;
        cart_wait_rwc = hdr->SB_G1RWC_val;
        cart_wait_frc = hdr->SB_G1FRC_val;
        cart_wait_fwc = hdr->SB_G1FWC_val;
        cart_wait_crc = hdr->SB_G1CRC_val;
        cart_wait_cwc = hdr->SB_G1CWC_val;
        cart_wait_gdrc = hdr->SB_G1GDRC_val;
        cart_wait_gdwc = hdr->SB_G1GDWC_val;
    }
}

static void cart_pio_set_offset(uint32_t offset) {
    uint16_t lo = (uint16_t)(offset & 0xFFFF);
    uint16_t hi = (uint16_t)((offset >> 16) & 0xFFFF);

    if(cart_m4_pio) {
        cart_off_out(NAOMI_CART_ROM_OFFSETH, hi);
        cart_off_out(NAOMI_CART_ROM_OFFSETL, lo | CART_PIO_STROBE);
    }
    else {
        cart_off_out(NAOMI_CART_ROM_OFFSETL, lo);
        cart_off_out(NAOMI_CART_ROM_OFFSETH, hi);
    }
}

static void cart_dma_set_offset(uint32_t offset) {
    uint16_t lo = (uint16_t)(offset & 0xFFFF);
    uint16_t hi = (uint16_t)((offset >> 16) & 0xFFFF);

    if(cart_m4_pio) {
        cart_off_out(NAOMI_CART_DMA_OFFSETH, hi);
        cart_off_out(NAOMI_CART_DMA_OFFSETL, lo);
    }
    else {
        cart_off_out(NAOMI_CART_DMA_OFFSETL, lo);
        cart_off_out(NAOMI_CART_DMA_OFFSETH, hi);
    }
}

static void cart_dma_set_count(uint16_t blocks) {
    cart_off_out(NAOMI_CART_DMA_COUNT, blocks);
}

static uint16_t cart_pio_read16(void) {
    return cart_reg_in(NAOMI_CART_ROM_DATA);
}

static void cart_pio_write16(uint16_t data) {
    cart_reg_out(NAOMI_CART_ROM_DATA, data);
}

static uint32_t cart_addr_flags(uint32_t offset, uint32_t flags) {
    return (flags & CART_ROM_MODE_MASK) | (offset & NAOMI_CART_ADDR_MASK);
}

static size_t cart_pio_read_locked(uint32_t offset, uint8_t *dst, size_t len,
        uint32_t flags) {
    uint16_t w;
    size_t done = 0;

    if(!len) {
        return 0;
    }

    cart_pio_begin();
    cart_pio_set_offset(cart_addr_flags(offset & ~1u, flags));

    if(offset & 1) {
        w = cart_pio_read16();
        dst[done++] = (uint8_t)(w >> 8);
        len--;
    }

    while(len >= 2) {
        w = cart_pio_read16();
        dst[done++] = (uint8_t)w;
        dst[done++] = (uint8_t)(w >> 8);
        len -= 2;
    }

    if(len) {
        w = cart_pio_read16();
        dst[done++] = (uint8_t)w;
    }

    return done;
}

static bool cart_dma_start(void *dst, size_t len) {
    uint32_t addr;

    if(!len || (len & (NAOMI_CART_DMA_UNIT - 1))) {
        return false;
    }

    addr = (uint32_t)dst & MEM_AREA_CACHE_MASK & ~(NAOMI_CART_DMA_UNIT - 1);

    cart_dma_irq_hook();
    cart_dma_err = 0;

    {
        irq_disable_scoped();
        cart_dma_in_progress = 1;
        CART_OUT32(G1_ATA_DMA_ADDRESS, addr);
        CART_OUT32(G1_ATA_DMA_LENGTH, (uint32_t)len);
        CART_OUT32(G1_ATA_DMA_DIRECTION, G1_DMA_TO_MEMORY);
        CART_OUT32(G1_ATA_DMA_ENABLE, 1);
        CART_OUT32(G1_ATA_DMA_STATUS, 1);
    }

    if(!cart_dma_wait()) {
        return false;
    }

    CART_OUT32(G1_ATA_DMA_ENABLE, 0);
    if(CART_IN32(G1_ATA_DMA_LENGTH_STAT) != (uint32_t)len) {
        return false;
    }

    return true;
}

static size_t cart_dma_read_locked(uint32_t offset, uint8_t *dst, size_t len,
        uint32_t flags) {
    uint32_t dma_flags;
    size_t chunk;
    size_t done = 0;

    if((offset & (NAOMI_CART_DMA_UNIT - 1)) ||
            ((uintptr_t)dst & (NAOMI_CART_DMA_UNIT - 1))) {
        return 0;
    }

    cart_pio_begin();
    cart_pio_set_offset(flags & CART_ROM_MODE_MASK);
    dma_flags = flags & CART_ROM_MODE_MASK & ~NAOMI_CART_ADDR_CRYPT;

    while(len >= NAOMI_CART_DMA_UNIT) {
        chunk = len;
        if(chunk > CART_DMA_MAX) {
            chunk = CART_DMA_MAX;
        }
        chunk &= ~(NAOMI_CART_DMA_UNIT - 1);

        dcache_inval_range((uintptr_t)(dst + done), chunk);
        cart_dma_set_count((uint16_t)(chunk / NAOMI_CART_DMA_UNIT));
        cart_dma_set_offset(dma_flags |
            ((offset + done) & NAOMI_CART_ADDR_MASK));

        if(!cart_dma_start(dst + done, chunk)) {
            break;
        }

        done += chunk;
        len -= chunk;
    }

    return done;
}

static bool cart_data_present(uint16_t data) {
    return data != CART_ROM_IDLE_WORD && data != CART_ROM_ZERO_WORD;
}

static bool cart_scan_header(uint32_t flags, uint32_t *off, uint8_t *peek) {
    uint32_t addr;

    for(addr = 0; addr < CART_HDR_SCAN_MAX; addr += CART_PROBE_STEP) {
        if(!cart_probe_fetch(addr, peek, flags)) {
            continue;
        }
        if(cart_probe_hdr(peek)) {
            if(off) {
                *off = addr;
            }
            return true;
        }
    }
    return false;
}

static bool cart_rom_alive(void) {
    uint32_t addr;
    uint16_t data;

    cart_pio_set_offset(NAOMI_CART_READ_DEFAULT);
    data = cart_pio_read16();
    if(cart_data_present(data)) {
        return true;
    }

    for(addr = CART_PROBE_STEP; addr < CART_HDR_SCAN_MAX; addr += CART_PROBE_STEP) {
        cart_pio_set_offset(cart_addr_flags(addr, NAOMI_CART_READ_DEFAULT));
        data = cart_pio_read16();
        if(cart_data_present(data)) {
            return true;
        }
    }
    return false;
}

size_t naomi_cart_pio_read(uint32_t offset, void *dst, size_t len, uint32_t flags) {
    size_t n;

    if(!dst || !len || !cart_lock()) {
        return 0;
    }

    n = cart_pio_read_locked(offset, (uint8_t *)dst, len, flags);
    cart_unlock();
    return n;
}

size_t naomi_cart_dma_read(uint32_t offset, void *dst, size_t len, uint32_t flags) {
    size_t n;

    if(!dst || !len || !cart_lock()) {
        return 0;
    }

    n = cart_dma_read_locked(offset, (uint8_t *)dst, len, flags);
    cart_unlock();
    return n;
}

size_t naomi_cart_read_ex(uint32_t offset, void *dst, size_t len, uint32_t flags) {
    uint8_t *p;
    size_t done = 0;
    size_t chunk;
    size_t n;

    if(!dst || !len || offset >= NAOMI_CART_SIZE_MAX || !cart_lock()) {
        return 0;
    }

    if(len > NAOMI_CART_SIZE_MAX - offset) {
        len = NAOMI_CART_SIZE_MAX - offset;
    }

    p = (uint8_t *)dst;

    while(len) {
        if((offset & (NAOMI_CART_DMA_UNIT - 1)) ||
                ((uintptr_t)p & (NAOMI_CART_DMA_UNIT - 1)) ||
                len < NAOMI_CART_DMA_UNIT) {
            chunk = NAOMI_CART_DMA_UNIT - (offset & (NAOMI_CART_DMA_UNIT - 1));
            if(chunk > len) {
                chunk = len;
            }
            if(((uintptr_t)p & (NAOMI_CART_DMA_UNIT - 1)) &&
                    chunk > (NAOMI_CART_DMA_UNIT - ((uintptr_t)p & (NAOMI_CART_DMA_UNIT - 1)))) {
                chunk = NAOMI_CART_DMA_UNIT - ((uintptr_t)p & (NAOMI_CART_DMA_UNIT - 1));
            }
            n = cart_pio_read_locked(offset, p, chunk, flags);
            done += n;
            if(n != chunk) {
                break;
            }
            p += n;
            offset += n;
            len -= n;
            continue;
        }

        chunk = len & ~(NAOMI_CART_DMA_UNIT - 1);
        n = cart_dma_read_locked(offset, p, chunk, flags);
        done += n;
        if(n != chunk) {
            break;
        }
        p += n;
        offset += n;
        len -= n;
    }

    cart_unlock();
    return done;
}

size_t naomi_cart_read(uint32_t offset, void *dst, size_t len) {
    return naomi_cart_read_ex(offset, dst, len, NAOMI_CART_READ_DEFAULT);
}

bool naomi_cart_read_header(naomi_cart_header_t *hdr) {
    uint32_t flags = NAOMI_CART_READ_DEFAULT;
    uint32_t addr;
    uint16_t m4;
    alignas(32) uint8_t peek[NAOMI_CART_DMA_UNIT];

    if(!hdr) {
        return false;
    }

    m4 = naomi_cart_m4_id();
    if(cart_m4_id_ok(m4)) {
        flags = naomi_cart_dump_flags(NAOMI_CART_M4);
        if(!cart_lock()) {
            return false;
        }
        cart_pio_begin();
        if(!(cart_reg_in(NAOMI_CART_ROM_OFFSETH) & CART_M4_PIC_BIT)) {
            flags &= ~NAOMI_CART_ADDR_CRYPT;
        }
        cart_unlock();
    }

    if(!cart_lock()) {
        return false;
    }

    if(!cart_scan_header(flags, &addr, peek)) {
        cart_unlock();
        return false;
    }

    if(cart_pio_read_locked(addr, (uint8_t *)hdr, sizeof(*hdr), flags) != sizeof(*hdr) ||
            !naomi_cart_valid(hdr)) {
        cart_unlock();
        return false;
    }

    naomi_cart_apply_g1_timing(hdr);
    cart_unlock();
    return true;
}

bool naomi_cart_dimm_present(void) {
    uint16_t cmd;
    uint16_t sts;

    if(!cart_lock()) {
        return false;
    }

    cmd = cart_reg_in(NAOMI_CART_DIMM_COMMAND);
    sts = cart_reg_in(NAOMI_CART_DIMM_STATUS);
    cart_unlock();

    if(cmd == 0xFFFF || cmd == 0x0000) {
        return false;
    }
    if(sts == 0xFFFF || sts == 0x7FFF) {
        return false;
    }

    return true;
}

uint16_t naomi_cart_m4_id(void) {
    uint16_t id;

    if(!cart_lock()) {
        return 0;
    }

    id = cart_reg_in(NAOMI_CART_M4_ID);
    cart_unlock();
    return id;
}

uint16_t naomi_cart_m1_id(void) {
    uint16_t id;

    if(!cart_lock()) {
        return 0;
    }

    id = cart_reg_in(NAOMI_CART_DMA_COUNT);
    cart_unlock();

    if(id == 0 || id == 0xFFFF) {
        return 0;
    }

    return id;
}

naomi_cart_type_t naomi_cart_type(void) {
    alignas(32) naomi_cart_header_t hdr;
    uint16_t m4;
    uint16_t m1;

    m4 = naomi_cart_m4_id();
    if(cart_m4_id_ok(m4)) {
        return NAOMI_CART_M4;
    }

    m1 = naomi_cart_m1_id();
    if(m1) {
        return NAOMI_CART_M1;
    }

    if(naomi_cart_read_header(&hdr)) {
        return NAOMI_CART_M2;
    }

    if(naomi_cart_dimm_present()) {
        return NAOMI_CART_DIMM;
    }

    return NAOMI_CART_NONE;
}

static bool cart_probe_fetch(uint32_t addr, uint8_t *buf, uint32_t flags) {
    return cart_pio_read_locked(addr, buf, NAOMI_CART_DMA_UNIT, flags) ==
        NAOMI_CART_DMA_UNIT;
}

static bool cart_probe_hdr(const uint8_t *buf) {
    return (buf[0] == 'N' || buf[0] == 'S') && buf[15] == ' ';
}

static bool cart_probe_empty(const uint8_t *buf) {
    for(int i = 0; i < NAOMI_CART_DMA_UNIT; i++) {
        if(buf[i] != CART_ROM_EMPTY_BYTE) {
            return false;
        }
    }
    return true;
}

size_t naomi_cart_probe_size(void) {
    alignas(32) uint8_t ref[NAOMI_CART_DMA_UNIT];
    alignas(32) uint8_t buf[NAOMI_CART_DMA_UNIT];
    uint32_t flags;
    uint32_t addr;
    uint32_t hdr_off;
    size_t last;
    uint32_t limit = NAOMI_CART_ADDR_MASK + 1;

    flags = naomi_cart_dump_flags(naomi_cart_type());

    if(!cart_lock()) {
        return 0;
    }

    cart_pio_begin();

    if(!cart_scan_header(flags, &hdr_off, ref)) {
        cart_unlock();
        return 0;
    }

    last = hdr_off + CART_PROBE_STEP;
    for(addr = CART_PROBE_STEP; addr < limit; addr += CART_PROBE_STEP) {
        if(!cart_probe_fetch(addr, buf, flags)) {
            continue;
        }
        if(addr != hdr_off && !memcmp(buf, ref, 16)) {
            break;
        }
        if(!cart_probe_empty(buf)) {
            last = addr + CART_PROBE_STEP;
        }
    }

    cart_unlock();
    return last;
}

static void cart_cfi_enter_locked(void) {
    cart_dma_hold();
    cart_pio_set_offset(CART_CFI_QUERY_ADDR);
    cart_pio_write16(CART_CFI_QUERY_CMD);
}

static void cart_cfi_exit_locked(void) {
    cart_dma_hold();
    cart_pio_set_offset(0);
    cart_pio_write16(CART_CFI_RESET_CMD);
}

bool naomi_cart_cfi_query(naomi_cart_cfi_t *out) {
    uint16_t q, r, y, exp;
    uint16_t m4;
    uint8_t chips;

    if(!out) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    m4 = naomi_cart_m4_id();
    if(!cart_m4_id_ok(m4)) {
        return false;
    }
    out->m4_id = m4;
    chips = (uint8_t)(m4 & 0x7F);
    if(!chips) {
        chips = 1;
    }

    if(!cart_lock()) {
        return false;
    }

    cart_pio_begin();
    cart_cfi_enter_locked();
    cart_pio_set_offset(NAOMI_CART_READ_DEFAULT | CART_CFI_QRY_OFF);
    q = cart_pio_read16();
    r = cart_pio_read16();
    y = cart_pio_read16();

    if((q & 0xFF) != 'Q' || (r & 0xFF) != 'R' || (y & 0xFF) != 'Y') {
        cart_cfi_exit_locked();
        cart_unlock();
        return false;
    }

    cart_pio_set_offset(NAOMI_CART_READ_DEFAULT | CART_CFI_SIZE_OFF);
    exp = cart_pio_read16();
    cart_cfi_exit_locked();
    cart_unlock();

    out->size_exp = (uint8_t)exp;
    out->chips = chips;
    if(out->size_exp >= 32) {
        return false;
    }

    out->chip_size = 1 << out->size_exp;
    out->total_size = out->chip_size * chips;
    if(out->total_size > NAOMI_CART_SIZE_MAX) {
        out->total_size = NAOMI_CART_SIZE_MAX;
    }

    return true;
}
