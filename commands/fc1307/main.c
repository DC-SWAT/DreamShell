/* DreamShell ##version##

   main.c - FC1307 SD-IDE firmware tools
   Copyright (C) 2026 SWAT

   Based on fc1307-tools by Adam Mnemonic:
   https://github.com/amnemonic/fc1307-tools
*/

#include "ds.h"
#include <dc/g1ata.h>

#define FC1307_ROM_SIZE         0x10000
#define FC1307_BLOCK_SIZE       512
#define FC1307_DUMP_STRIDE      0x200
#define FC1307_ATA_ALIGN        32

#define FC1307_CMD_VENDOR       0xFE
#define FC1307_FEAT_READ        0x04
#define FC1307_FEAT_WRITE       0x0B
#define FC1307_FEAT_ERASE       0x1C
#define FC1307_ERASE_OK         0xAA

#define G1_ATA_ALTSTATUS        0xA05F7018
#define G1_ATA_DATA             0xA05F7080
#define G1_ATA_FEATURES         0xA05F7084
#define G1_ATA_SECTOR_COUNT     0xA05F7088
#define G1_ATA_LBA_LOW          0xA05F708C
#define G1_ATA_LBA_MID          0xA05F7090
#define G1_ATA_LBA_HIGH         0xA05F7094
#define G1_ATA_COMMAND_REG      0xA05F709C

#define G1_ATA_SR_ERR           0x01
#define G1_ATA_SR_DRQ           0x08
#define G1_ATA_SR_DF            0x20
#define G1_ATA_SR_DRDY          0x40
#define G1_ATA_SR_BSY           0x80

#define OUT16(addr, data) *((volatile uint16_t *)(addr)) = (data)
#define OUT8(addr, data)  *((volatile uint8_t *)(addr)) = (data)
#define IN16(addr)        *((volatile uint16_t *)(addr))
#define IN8(addr)         *((volatile uint8_t *)(addr))

#define fc1307_wait_status(n) \
    do {} while((IN8(G1_ATA_ALTSTATUS) & (n)))

#define fc1307_wait_nbsy() fc1307_wait_status(G1_ATA_SR_BSY)
#define fc1307_wait_bsydrq() fc1307_wait_status(G1_ATA_SR_DRQ | G1_ATA_SR_BSY)
#define fc1307_wait_drdy() \
    do {} while(!(IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_DRDY))

static int fc1307_wait_drq(void) {
    uint8_t val = IN8(G1_ATA_ALTSTATUS);

    while(!(val & G1_ATA_SR_DRQ) && !(val & (G1_ATA_SR_ERR | G1_ATA_SR_DF))) {
        val = IN8(G1_ATA_ALTSTATUS);
    }

    return (val & (G1_ATA_SR_ERR | G1_ATA_SR_DF)) ? -1 : 0;
}

static int fc1307_ata_begin(void) {
    if(g1_ata_init() < 0) {
        ds_printf("DS_ERROR: G1 ATA not available\n");
        return -1;
    }

    if(g1_ata_mutex_lock()) {
        ds_printf("DS_ERROR: G1 ATA mutex lock failed\n");
        return -1;
    }

    fc1307_wait_bsydrq();
    g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
    return 0;
}

static void fc1307_ata_end(void) {
    g1_ata_mutex_unlock();
}

static int fc1307_pio_read(uint8_t *buf) {
    unsigned int i;
    uint16_t word;

    if(fc1307_wait_drq()) {
        return -1;
    }

    for(i = 0; i < (FC1307_BLOCK_SIZE / 2); i++) {
        word = IN16(G1_ATA_DATA);
        buf[i * 2] = (uint8_t)word;
        buf[i * 2 + 1] = (uint8_t)(word >> 8);
    }

    fc1307_wait_bsydrq();
    return 0;
}

static int fc1307_pio_write(const uint8_t *buf) {
    unsigned int i;
    uint16_t word;

    fc1307_wait_nbsy();

    for(i = 0; i < (FC1307_BLOCK_SIZE / 2); i++) {
        word = buf[i * 2] | (buf[i * 2 + 1] << 8);
        OUT16(G1_ATA_DATA, word);
    }

    fc1307_wait_bsydrq();
    return 0;
}

static int fc1307_read_block(uint32_t addr, uint8_t *buf) {
    OUT8(G1_ATA_FEATURES, FC1307_FEAT_READ);
    OUT8(G1_ATA_SECTOR_COUNT, 0);
    OUT8(G1_ATA_LBA_LOW, (uint8_t)(addr & 0xFF));
    OUT8(G1_ATA_LBA_MID, (uint8_t)((addr >> 8) & 0xFF));
    OUT8(G1_ATA_LBA_HIGH, 0);

    fc1307_wait_nbsy();
    fc1307_wait_drdy();
    OUT8(G1_ATA_COMMAND_REG, FC1307_CMD_VENDOR);

    return fc1307_pio_read(buf);
}

static int fc1307_write_block(uint32_t addr, const uint8_t *buf) {
    OUT8(G1_ATA_FEATURES, FC1307_FEAT_WRITE);
    OUT8(G1_ATA_SECTOR_COUNT, 1);
    OUT8(G1_ATA_LBA_LOW, (uint8_t)(addr & 0xFF));
    OUT8(G1_ATA_LBA_MID, (uint8_t)((addr >> 8) & 0xFF));
    OUT8(G1_ATA_LBA_HIGH, (uint8_t)((addr >> 16) & 0xFF));

    fc1307_wait_nbsy();
    fc1307_wait_drdy();
    OUT8(G1_ATA_COMMAND_REG, FC1307_CMD_VENDOR);

    return fc1307_pio_write(buf);
}

static int fc1307_erase_chip(void) {
    alignas(FC1307_ATA_ALIGN) uint8_t status[FC1307_BLOCK_SIZE];

    OUT8(G1_ATA_FEATURES, FC1307_FEAT_ERASE);
    OUT8(G1_ATA_SECTOR_COUNT, 0);
    OUT8(G1_ATA_LBA_LOW, 0);
    OUT8(G1_ATA_LBA_MID, 0);
    OUT8(G1_ATA_LBA_HIGH, 0);

    fc1307_wait_nbsy();
    fc1307_wait_drdy();
    OUT8(G1_ATA_COMMAND_REG, FC1307_CMD_VENDOR);

    if(fc1307_pio_read(status) < 0) {
        ds_printf("DS_ERROR: FC1307 erase command failed\n");
        return -1;
    }

    if(status[0] != FC1307_ERASE_OK) {
        ds_printf("DS_ERROR: FC1307 erase status 0x%02x (expected 0x%02x)\n",
                  status[0], FC1307_ERASE_OK);
        return -1;
    }

    return 0;
}

static int fc1307_cmd_dump(const char *outfile) {
    file_t ff;
    alignas(FC1307_ATA_ALIGN) uint8_t block[FC1307_BLOCK_SIZE];
    uint32_t addr;
    int rv = CMD_OK;

    ff = fs_open(outfile, O_CREAT | O_WRONLY | O_TRUNC);
    if(ff == FILEHND_INVALID) {
        ds_printf("DS_ERROR: Can't open %s\n", outfile);
        return CMD_ERROR;
    }

    if(fc1307_ata_begin() < 0) {
        return CMD_ERROR;
    }

    ds_printf("DS_PROCESS: Dumping FC1307 firmware to %s\n", outfile);

    for(addr = 0; addr < FC1307_ROM_SIZE; addr += FC1307_DUMP_STRIDE) {
        if(fc1307_read_block(addr, block) < 0) {
            ds_printf("DS_ERROR: FC1307 read failed at 0x%05lx\n",
                      (unsigned long)addr);
            rv = CMD_ERROR;
            break;
        }

        if(fs_write(ff, block, FC1307_BLOCK_SIZE) != (ssize_t)FC1307_BLOCK_SIZE) {
            ds_printf("DS_ERROR: Can't write %s\n", outfile);
            rv = CMD_ERROR;
            break;
        }
    }

    fs_close(ff);
    fc1307_ata_end();

    if(rv == CMD_OK) {
        ds_printf("DS_INFO: Dump complete (%u bytes)\n", FC1307_ROM_SIZE);
    }

    return rv;
}

static int fc1307_cmd_write(const char *infile) {
    file_t ff;
    uint8_t *rom;
    uint32_t addr;
    ssize_t rom_size;
    int rv = CMD_OK;

    ff = fs_open(infile, O_RDONLY);
    if(ff == FILEHND_INVALID) {
        ds_printf("DS_ERROR: Can't open %s\n", infile);
        return CMD_ERROR;
    }

    rom_size = fs_total(ff);
    if(rom_size != (ssize_t)FC1307_ROM_SIZE) {
        ds_printf("DS_ERROR: ROM size must be 0x%x bytes (got %ld)\n",
                  FC1307_ROM_SIZE, (long)rom_size);
        fs_close(ff);
        return CMD_ERROR;
    }

    rom = (uint8_t *)aligned_alloc(32, FC1307_ROM_SIZE);
    if(!rom) {
        ds_printf("DS_ERROR: No free memory\n");
        fs_close(ff);
        return CMD_ERROR;
    }

    if(fs_read(ff, rom, FC1307_ROM_SIZE) != (ssize_t)FC1307_ROM_SIZE) {
        ds_printf("DS_ERROR: Can't read %s\n", infile);
        free(rom);
        fs_close(ff);
        return CMD_ERROR;
    }

    fs_close(ff);

    if(fc1307_ata_begin() < 0) {
        free(rom);
        return CMD_ERROR;
    }

    ds_printf("DS_PROCESS: Erasing FC1307 SPI flash\n");
    if(fc1307_erase_chip() < 0) {
        free(rom);
        fc1307_ata_end();
        return CMD_ERROR;
    }

    ds_printf("DS_PROCESS: Writing FC1307 firmware from %s\n", infile);

    for(addr = 0; addr < FC1307_ROM_SIZE; addr += FC1307_BLOCK_SIZE) {
        if(fc1307_write_block(addr, rom + addr) < 0) {
            ds_printf("DS_ERROR: FC1307 write failed at 0x%05lx\n",
                      (unsigned long)addr);
            rv = CMD_ERROR;
            break;
        }
    }

    free(rom);
    fc1307_ata_end();

    if(rv == CMD_OK) {
        ds_printf("DS_INFO: Write complete (%u bytes)\n", FC1307_ROM_SIZE);
    }

    return rv;
}

static int fc1307_cmd_erase(void) {
    int rv;

    if(fc1307_ata_begin() < 0) {
        return CMD_ERROR;
    }

    ds_printf("DS_PROCESS: Erasing FC1307 SPI flash\n");
    rv = fc1307_erase_chip();
    fc1307_ata_end();

    if(rv < 0) {
        return CMD_ERROR;
    }

    ds_printf("DS_INFO: Erase complete\n");
    return CMD_OK;
}

int main(int argc, char *argv[]) {
    int dump = 0, write_cmd = 0, erase = 0;
    char *file = NULL;

    if(argc < 2) {
        ds_printf("Usage: %s options args...\n"
                  "Options:\n"
                  " -d, --dump   Dump 64 KiB firmware\n"
                  " -w, --write  Erase chip and flash 64 KiB firmware\n"
                  " -e, --erase  Erase SPI flash only\n"
                  " -f, --file   ROM file (dump/write)\n\n"
                  "Examples:\n"
                  " %s -d -f /ram/fc1307.bin\n"
                  " %s -w -f /ram/fc1307.bin\n"
                  " %s -e\n",
                  argv[0], argv[0], argv[0], argv[0]);
        return CMD_NO_ARG;
    }

    struct cfg_option options[] = {
        {"dump",  'd', NULL, CFG_BOOL, (void *)&dump,      0},
        {"write", 'w', NULL, CFG_BOOL, (void *)&write_cmd, 0},
        {"erase", 'e', NULL, CFG_BOOL, (void *)&erase,     0},
        {"file",  'f', NULL, CFG_STR,  (void *)&file,      0},
        CFG_END_OF_LIST
    };

    CMD_DEFAULT_ARGS_PARSER(options);

    if(dump + write_cmd + erase != 1) {
        ds_printf("DS_ERROR: Specify one of -d, -w or -e\n");
        return CMD_NO_ARG;
    }

    if(dump || write_cmd) {
        if(!file) {
            ds_printf("DS_ERROR: Missing ROM file (-f)\n");
            return CMD_NO_ARG;
        }
    }

    if(dump) {
        return fc1307_cmd_dump(file);
    }

    if(write_cmd) {
        return fc1307_cmd_write(file);
    }

    return fc1307_cmd_erase();
}
