/**
 * DreamShell HollySH BIOS loader
 * Main
 * (c) 2025-2026 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/timer.h>
#include <stdio.h>

uintptr_t loader_addr = 0;
uintptr_t loader_end = 0;

static int open_boot_file(void) {

    static const char *boot_paths[] = {
        "0:/DS_BOOT.BIN",
        "0:/DS/DS_BOOT.BIN",
        "0:/DS/DS_CORE.BIN",
        "0:/DS_CORE.BIN",
        NULL
    };

    static const int boot_vols[] = { FS_VOL_SD, FS_VOL_IDE };

    for(int v = 0; v < (int)(sizeof(boot_vols) / sizeof(boot_vols[0])); v++) {
        if(!fs_volume_mounted(boot_vols[v])) {
            continue;
        }

        for(int i = 0; boot_paths[i] != NULL; i++) {
            char path[32];
            const char *src = boot_paths[i];

            memcpy(path, src, strlen(src) + 1);
            path[0] = (char)('0' + boot_vols[v]);

            int fd = open(path, O_RDONLY);

            if(fd >= 0) {
                return fd;
            }
        }
    }

    return -1;
}

static int load_embedded_bootloader(void) {

    if(BOOTLOADER_ROM_SIZE == 0) {
        return -1;
    }

    uint8 *src = (uint8 *)NONCACHED_ADDR(BOOTLOADER_ROM_OFFSET);
    uint8 *dst = (uint8 *)CACHED_ADDR(APP_BIN_ADDR);

    memcpy(dst, src, BOOTLOADER_ROM_SIZE);
    icache_flush_range(CACHED_ADDR(APP_BIN_ADDR), BOOTLOADER_ROM_SIZE);

    return 0;
}

static char *state_to_string(uint32_t status) {
    switch (status) {
        case G1_ATA_BUS_PROT_STATE_IN_PROGRESS:
            return "In Progress";
        case G1_ATA_BUS_PROT_STATE_FAILED:
            return "Failed";
        case G1_ATA_BUS_PROT_STATE_PASSED:
            return "Passed";
        default:
            return "Unknown";
    }
}

int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    irq_disable();

    loader_end = loader_addr + loader_size;

    /* Setup BIOS timer */
    timer_init();
    timer_ns_enable();
    timer_prime_bios(TMU0);
    timer_start(TMU0);

    bfont_saved_addr = (uint32)get_font_address();

    printf(NULL);
    OpenLog();
    malloc_init();

    /* Setup Perfomance Counter timer */
    timer_ns_enable();

    uint32_t holly_rev = holly_revision();
    uint32_t hw_type = hw_sys_type();
    uint32_t g1_state = g1_ata_prot_state();

    printf("DreamShell HollySH BIOS firmware v"VERSION"\n");

    printf("Hardware type: 0x%04lx %s\n", hw_type,
        hw_type == HW_TYPE_RETAIL ?
            "Dreamcast" : is_naomi_2() ? "NAOMI 2" : "NAOMI"
    );

    printf("Hardware revision: 0x%04lx %s\n", holly_rev,
        holly_rev == HOLLY_REV_VA0 ?
            "VA0" : holly_rev == HOLLY_REV_VA1 ? "VA1" : "VA2"
    );

    printf("G1 ATA bus protection: %s\n", state_to_string(g1_state));

    if(g1_state != G1_ATA_BUS_PROT_STATE_PASSED) {
        volatile uint32_t *react = (uint32_t *)NONCACHED_ADDR(G1_ATA_BUS_PROTECTION);
        volatile uint32_t *bios = (uint32_t *)NONCACHED_ADDR(0);

        printf("Running BIOS verification...\n");
        *react = 0x3ff;

        for(uint32_t p = 0; p < 0x400 / sizeof(bios[0]); p++) {
            (void)bios[p];
        }
        g1_state = g1_ata_prot_state();
        printf("G1 ATA bus protection: %s\n", state_to_string(g1_state));

        if(g1_state != G1_ATA_BUS_PROT_STATE_PASSED) {
            printf("Failed to reactivate G1 ATA bus\n");
        }
    }

    printf("Initializing file system...\n");
    fs_enable_dma(FS_DMA_SHARED);
    int part;

    for(part = 0; part < 4; part++) {
        if(!fs_init(part)) {
            break;
        }
    }

    if(part == 4) {
        printf("Failed to initialize file system\n");
        goto error;
    }

    printf("Loading executable...\n");

    int fd = open_boot_file();

    if(fd >= 0) {
        uint32_t total_len = total(fd);
        uint64_t begin_ns = timer_ns_gettime64();
        int len = read(fd, (void *)NONCACHED_ADDR(APP_BIN_ADDR), total_len);
        uint32_t elapsed_ms = (uint32_t)((timer_ns_gettime64() - begin_ns) / 1000000);
        uint32_t speed_kbs = 0;

        if (elapsed_ms > 0) {
            speed_kbs = (uint32_t)(((uint64_t)total_len * 1000) / ((uint64_t)elapsed_ms * 1024));
        }
        printf("Loaded %d bytes in %d ms, %d KB/s\n", len, elapsed_ms, speed_kbs);

        if(len > 0 && (uint32_t)len == total_len) {
            close(fd);
            printf("Executing from 0x%08lx...\n", NONCACHED_ADDR(APP_BIN_ADDR));
            launch(APP_BIN_ADDR);
        }
        else {
            printf("Failed to load executable\n");
            goto error;
        }
    }
    else {
        printf("Loading embedded bootloader...\n");

        if(load_embedded_bootloader() == 0) {
            printf("Executing from 0x%08lx...\n", APP_BIN_ADDR);
            launch(APP_BIN_ADDR);
        }
        else {
            printf("Failed to load executable\n");
            goto error;
        }
    }

    return 0;

error:
    printf("System shutdown in 10 seconds.\n");
    timer_spin_sleep_bios(10000);
    return 0;
}
