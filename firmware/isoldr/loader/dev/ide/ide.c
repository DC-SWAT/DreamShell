/**
 * Copyright (c) 2014-2023 SWAT <http://www.dc-swat.ru>
 * Copyright (c) 2017 Megavolt85
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <main.h>
#include <exception.h>
#include <asic.h>
#include <mmu.h>
#include <arch/cache.h>
#include <arch/timer.h>

#define ATA_SR_BSY				0x80
#define ATA_SR_DRDY				0x40
#define ATA_SR_DF				0x20
#define ATA_SR_DSC				0x10
#define ATA_SR_DRQ				0x08
#define ATA_SR_CORR				0x04
#define ATA_SR_IDX				0x02
#define ATA_SR_ERR				0x01

#define ATA_ER_BBK				0x80
#define ATA_ER_UNC				0x40
#define ATA_ER_MC				0x20
#define ATA_ER_IDNF				0x10
#define ATA_ER_MCR				0x08
#define ATA_ER_ABRT				0x04
#define ATA_ER_TK0NF			0x02
#define ATA_ER_AMNF				0x01

// ATA-Commands:
#define ATA_CMD_READ_PIO		0x20
#define ATA_CMD_READ_PIO_EXT	0x24
#define ATA_CMD_READ_DMA		0xC8
#define ATA_CMD_READ_DMA_EXT	0x25
#define ATA_CMD_WRITE_PIO		0x30
#define ATA_CMD_WRITE_PIO_EXT	0x34
#define ATA_CMD_WRITE_DMA		0xCA
#define ATA_CMD_WRITE_DMA_EXT	0x35
#define ATA_CMD_CACHE_FLUSH		0xE7
#define ATA_CMD_CACHE_FLUSH_EXT	0xEA
#define ATA_CMD_IDENTIFY		0xEC
#define ATA_CMD_SET_FEATURES	0xEF

// ATAPI-Commands:
#define ATAPI_CMD_PACKET		0xA0
#define ATAPI_CMD_IDENTIFY		0xA1
#define ATAPI_CMD_RESET         0x08
#define ATAPI_CMD_READ			0xA8
#define ATAPI_CMD_EJECT			0x1B
#define ATAPI_CMD_READ_TOC		0x43
#define ATAPI_CMD_MODE_SENSE	0x5A
#define ATAPI_CMD_READ_CD		0xBE

// SPI-Commands:

#define SPI_CMD_EJECT			0x16
#define SPI_CMD_READ_TOC		0x14
#define SPI_CMD_REQ_STAT		0x10
#define SPI_CMD_READ_CD			0x30
#define SPI_CMD_SEEK			0x21
#define SPI_CMD_REQ_MODE		0x11

#define ATA_MASTER				0x00
#define ATA_SLAVE				0x01

#define IDE_ATA					0x00
#define IDE_ATAPI				0x01
#define IDE_SPI					0x02

#define G1_ATA_CTRL             0xA05F7018
#define G1_ATA_BASE             0xA05F7080

/* ATA-related registers. Some of these serve very different purposes when read
   than they do when written (hence why some addresses are duplicated). */
#define G1_ATA_ALTSTATUS        0xA05F7018      /* Read */
#define G1_ATA_CTL              0xA05F7018      /* Write */
#define G1_ATA_DATA             0xA05F7080      /* Read/Write */
#define G1_ATA_ERROR            0xA05F7084      /* Read */
#define G1_ATA_FEATURES         0xA05F7084      /* Write */
#define G1_ATA_IRQ_REASON       0xA05F7088      /* Read */
#define G1_ATA_SECTOR_COUNT     0xA05F7088      /* Write */
#define G1_ATA_LBA_LOW          0xA05F708C      /* Read/Write */
#define G1_ATA_LBA_MID          0xA05F7090      /* Read/Write */
#define G1_ATA_LBA_HIGH         0xA05F7094      /* Read/Write */
#define G1_ATA_DEVICE_SELECT    0xA05F7098      /* Read/Write */
#define G1_ATA_STATUS_REG       0xA05F709C      /* Read */
#define G1_ATA_COMMAND_REG      0xA05F709C      /* Write */

/* DMA-related registers. */
#define G1_ATA_DMA_RACCESS_WAIT 0xA05F74A0      /* Write-only */
#define G1_ATA_DMA_WACCESS_WAIT 0xA05F74A4      /* Write-only */
#define G1_ATA_DMA_ADDRESS      0xA05F7404      /* Read/Write */
#define G1_ATA_DMA_LENGTH       0xA05F7408      /* Read/Write */
#define G1_ATA_DMA_DIRECTION    0xA05F740C      /* Read/Write */
#define G1_ATA_DMA_ENABLE       0xA05F7414      /* Read/Write */
#define G1_ATA_DMA_STATUS       0xA05F7418      /* Read/Write */
#define G1_ATA_DMA_STARD        0xA05F74F4      /* Read-only */
#define G1_ATA_DMA_LEND         0xA05F74F8      /* Read-only */
#define G1_ATA_DMA_PRO          0xA05F74B8      /* Write-only */
#define G1_ATA_DMA_PRO_SYSMEM   0x8843407F

/* PIO-related registers. */
#define G1_ATA_PIO_RACCESS_WAIT 0xA05F7490      /* Write-only */
#define G1_ATA_PIO_WACCESS_WAIT 0xA05F7494      /* Write-only */
#define G1_ATA_PIO_IORDY_CTRL   0xA05F74B4      /* Write-only */

/* Subcommands we might care about for the SET FEATURES command. */
#define ATA_FEATURE_TRANSFER_MODE   0x03

/* Transfer mode values. */
#define ATA_TRANSFER_PIO_DEFAULT    0x00
#define ATA_TRANSFER_PIO_NOIORDY    0x01
#define ATA_TRANSFER_PIO_FLOW(x)    0x08 | ((x) & 0x07)
#define ATA_TRANSFER_WDMA(x)        0x20 | ((x) & 0x07)
#define ATA_TRANSFER_UDMA(x)        0x40 | ((x) & 0x07)

/* Access timing data. */
#define G1_ACCESS_WDMA_MODE2        0x00001001
#define G1_ACCESS_PIO_DEFAULT       0x00000222

/* DMA Settings. */
#define G1_DMA_TO_DEVICE            0
#define G1_DMA_TO_MEMORY            1

/* Macros to access the ATA registers */
#define OUT32(addr, data) *((volatile u32 *)addr) = data
#define OUT16(addr, data) *((volatile u16 *)addr) = data
#define OUT8(addr, data)  *((volatile u8  *)addr) = data
#define IN32(addr)        *((volatile u32 *)addr)
#define IN16(addr)        *((volatile u16 *)addr)
#define IN8(addr)         *((volatile u8  *)addr)

typedef struct ide_req {
	void *buff;
	u64	lba;
	u32	count;
	u32	bytes;
	u8 cmd;
	u8 async;
	struct ide_device *dev;
} ide_req_t;

#ifdef DEV_TYPE_EMU
# define MAX_DEVICE_COUNT 1
#else
# define MAX_DEVICE_COUNT 2
#endif

static struct ide_device ide_devices[MAX_DEVICE_COUNT];
static u32 g1_dma_part_avail = 0;
static u32 g1_dma_irq_visible = 1;
static s32 g1_dma_irq_idx_game = 0;

static u32 g1_pio_total = 0;
static u32 g1_pio_avail = 0;
static u32 g1_pio_trans = 0;

#ifdef HAVE_EXPT
static s8 g1_dma_irq_idx_internal = 0;
#endif


#define g1_ata_wait_status(n) \
    do {} while((IN8(G1_ATA_ALTSTATUS) & (n)))

#define g1_ata_wait_nbsy() g1_ata_wait_status(ATA_SR_BSY)

#define g1_ata_wait_bsydrq() g1_ata_wait_status(ATA_SR_DRQ | ATA_SR_BSY)

#define g1_ata_wait_drdy() \
    do {} while(!(IN8(G1_ATA_ALTSTATUS) & ATA_SR_DRDY))
		
#define g1_ata_wait_dma() \
    do {} while(IN32(G1_ATA_DMA_STATUS))

u8 swap8(u8 n) {
	return ((n >> 4) | (n << 4));
}

u16 swap16(u16 n) {
	return ((n >> 8) | (n << 8));
}

/* Is a G1 DMA in progress? */
s32 g1_dma_in_progress(void) {
    return IN32(G1_ATA_DMA_STATUS);
}

u32 g1_dma_transfered(void) {
	return IN32(G1_ATA_DMA_LEND);
}

static void delay_1ms() {
	// timer_spin_sleep_bios(1);
	for(int d = 0; d < 10; d++) IN8(G1_ATA_ALTSTATUS);
}

#ifdef HAVE_EXPT

void *g1_dma_handler(void *passer, register_stack *stack, void *current_vector) {

	uint32 code = *REG_INTEVT;
	uint32 status = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
	uint32 statusExt = ASIC_IRQ_STATUS[ASIC_MASK_EXT_INT];
	uint32 statusErr = ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT];

	(void)passer;
	(void)stack;

	if ((statusErr & ASIC_ERR_G1DMA_ILLEGAL) || (statusErr & ASIC_ERR_G1DMA_OVERRUN) || (statusErr & ASIC_ERR_G1DMA_ROM_FLASH)) {
		LOGFF("ERROR 0x%08lx %d\n", statusErr, g1_dma_irq_visible);
		ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT] = ASIC_ERR_G1DMA_ROM_FLASH | ASIC_ERR_G1DMA_ILLEGAL | ASIC_ERR_G1DMA_OVERRUN;
		poll_all();
		return my_exception_finish;
	}

	uint32 g1_dma_irq_idx = (g1_dma_irq_visible ? g1_dma_irq_idx_game : g1_dma_irq_idx_internal);

	if ((g1_dma_irq_idx == 13 && code != EXP_CODE_INT13) ||
		(g1_dma_irq_idx == 11 && code != EXP_CODE_INT11) || 
		(g1_dma_irq_idx == 9 && code != EXP_CODE_INT9)
	) {
		return current_vector;
	}

#ifdef DEBUG
	DBGFF("IRQ: %08lx NRM: 0x%08lx EXT: 0x%08lx ERR: 0x%08lx, VISIBLE: %d\n",
	      *REG_INTEVT, status, statusExt, statusErr, g1_dma_irq_visible);
//	dump_regs(stack);
#else
	// LOGFF("%08lx 0x%08lx 0x%02lx %d 0x%08lx\n",
	// 	*REG_INTEVT, status, statusExt, g1_dma_irq_visible, (uint32)r15());
#endif

	if (statusExt & ASIC_EXT_GD_CMD) {
		if (g1_dma_irq_visible == 0) {
			g1_ata_ack_irq();
		}
	}
	if (status & ASIC_NRM_GD_DMA) {
		/* Processing filesystem */
		poll_all();

		if (g1_dma_irq_visible == 0) {
			/* Ack DMA IRQ. */
			ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
		}
	}
	if (g1_dma_irq_visible) {
		return current_vector;
	}
	return my_exception_finish;
}


s32 g1_dma_init_irq() {

	g1_dma_irq_idx_game = 0;
	g1_dma_irq_idx_internal = 0;
	g1_dma_irq_visible = 1;

#ifdef NO_ASIC_LT
	return 0;
#else

	asic_lookup_table_entry a_entry;

	a_entry.irq = EXP_CODE_ALL;
	a_entry.clear_irq = 0;
	a_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
	a_entry.mask[ASIC_MASK_EXT_INT] = ASIC_EXT_GD_CMD;
	a_entry.mask[ASIC_MASK_ERR_INT] = (ASIC_ERR_G1DMA_ILLEGAL | ASIC_ERR_G1DMA_OVERRUN | ASIC_ERR_G1DMA_ROM_FLASH);
	a_entry.handler = g1_dma_handler;
	return asic_add_handler(&a_entry, NULL, 0);
#endif
}

#endif

void g1_dma_abort(void) {
	OUT8(G1_ATA_DMA_ENABLE, 0);
	if (g1_dma_in_progress()) {
		g1_ata_wait_dma();
	}
}

void g1_dma_start(u32 addr, size_t bytes) {

	/* Set the DMA parameters up. */
	OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
	OUT32(G1_ATA_DMA_ADDRESS, (addr & 0x0FFFFFFF));
	OUT32(G1_ATA_DMA_LENGTH, bytes);
	OUT8(G1_ATA_DMA_DIRECTION, G1_DMA_TO_MEMORY);
	
	 /* Enable G1 DMA. */
	OUT8(G1_ATA_DMA_ENABLE, 1);
	OUT8(G1_ATA_DMA_STATUS, 1);
}

void g1_dma_irq_hide(s32 all) {

	g1_dma_irq_visible = 0;

	if (*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA) {

		g1_dma_irq_idx_game = 9;
		*ASIC_IRQ9_MASK &= ~ASIC_NRM_GD_DMA;

	} else if (*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) {

		g1_dma_irq_idx_game = 11;
		*ASIC_IRQ11_MASK &= ~ASIC_NRM_GD_DMA;

	} else if (*ASIC_IRQ13_MASK & ASIC_NRM_GD_DMA) {

		g1_dma_irq_idx_game = 13;
		*ASIC_IRQ13_MASK &= ~ASIC_NRM_GD_DMA;

	} else {
		g1_dma_irq_idx_game = 0;
	}

#ifdef HAVE_EXPT
	if (!all && exception_inited()) {
		if (g1_dma_irq_idx_game == 9) {
			*ASIC_IRQ11_MASK |= ASIC_NRM_GD_DMA;
			g1_dma_irq_idx_internal = 11;
		} else {
			*ASIC_IRQ9_MASK |= ASIC_NRM_GD_DMA;
			g1_dma_irq_idx_internal = 9;
		}
	} else {
		g1_dma_irq_idx_internal = 0;
	}
#else
	(void)all;
#endif
}

void g1_dma_irq_restore(void) {

#ifdef HAVE_EXPT
	if (g1_dma_irq_idx_internal == 9) {
		*ASIC_IRQ9_MASK &= ~ASIC_NRM_GD_DMA;
	} else if(g1_dma_irq_idx_internal == 11) {
		*ASIC_IRQ11_MASK &= ~ASIC_NRM_GD_DMA;
	}
	// g1_dma_irq_idx_internal = 0;
#endif

	if(g1_dma_irq_idx_game == 9) {
		*ASIC_IRQ9_MASK |= ASIC_NRM_GD_DMA;
	} else if(g1_dma_irq_idx_game == 11) {
		*ASIC_IRQ11_MASK |= ASIC_NRM_GD_DMA;
	} else if(g1_dma_irq_idx_game == 13) {
		*ASIC_IRQ13_MASK |= ASIC_NRM_GD_DMA;
	}

	// g1_dma_irq_idx_game = 0;
	g1_dma_irq_visible = 1;
}


void g1_dma_set_irq_mask(s32 last_transfer) {

	s32 dma_mode = fs_dma_enabled();

	if (dma_mode == FS_DMA_DISABLED) {

		return;

	} else if (dma_mode == FS_DMA_HIDDEN) {

		/* Hide all internal DMA transfers (CDDA, etc...) */
		if (g1_dma_irq_visible) {
			g1_dma_irq_hide(0);
		} else {
			return;
		}

	} else if (dma_mode == FS_DMA_SHARED) {

		if (last_transfer && !g1_dma_irq_visible) {
			g1_dma_irq_restore();
		} else if (!last_transfer && g1_dma_irq_visible) {
			g1_dma_irq_hide(0);
		} else {
			if(!g1_dma_irq_idx_game) {
				g1_dma_irq_idx_game = g1_dma_has_irq_mask();
			}
			return;
		}
	}

#ifdef LOG
	LOGFF("%d %d %d (mode=%d last=%d game=%d int=%d)\n",
		(*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA) ? 9 : 0, 
		(*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) ? 11 : 0, 
		(*ASIC_IRQ13_MASK & ASIC_NRM_GD_DMA) ? 13 : 0,
		dma_mode, last_transfer,
		g1_dma_irq_idx_game,
# ifdef HAVE_EXPT
		g1_dma_irq_idx_internal
# else
		0
# endif
	);
#endif
}

s32 g1_dma_has_irq_mask() {
	if (*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA) return 9; 
	if (*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) return 11; 
	if (*ASIC_IRQ13_MASK & ASIC_NRM_GD_DMA) return 13;
	return 0;
}

/* This one is an inline function since it needs to return something... */
static inline s32 g1_ata_wait_drq(void) 
{
    u8 val = IN8(G1_ATA_ALTSTATUS);

    while(!(val & ATA_SR_DRQ) && !(val & (ATA_SR_ERR | ATA_SR_DF))) 
    {
        val = IN8(G1_ATA_ALTSTATUS);
    }

    return (val & (ATA_SR_ERR | ATA_SR_DF)) ? -1 : 0;
}

static s32 g1_ata_set_transfer_mode(u8 mode) 
{
    u8 status;

    /* Fill in the registers as is required. */
    OUT8(G1_ATA_FEATURES, ATA_FEATURE_TRANSFER_MODE);
    OUT8(G1_ATA_SECTOR_COUNT, mode);
    OUT8(G1_ATA_LBA_LOW, 0);
    OUT8(G1_ATA_LBA_MID, 0);
    OUT8(G1_ATA_LBA_HIGH,0);

    /* Send the SET FEATURES command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_SET_FEATURES);
    delay_1ms();

    /* Wait for command completion. */
    g1_ata_wait_nbsy();

    /* See if the command completed. */
    status = IN8(G1_ATA_STATUS_REG);

    if((status & ATA_SR_ERR) || (status & ATA_SR_DF)) 
    {
        LOGFF("Error setting transfer mode %02x\n", mode);
        return -1;
    }

    return 0;
}

#if defined(LOG) && !defined(DEV_TYPE_EMU)
static const s8 *dev_bus_name[] = {"MASTER", "SLAVE"};
static const s8 *dev_proto_name[] = {"ATAPI", "SPI"};
#endif

static s32 g1_dev_scan(void)
{
	memset(&ide_devices[0], 0, sizeof(ide_devices));

#ifdef DEV_TYPE_EMU
	ide_devices[0].wdma_modes = 0x0407;
	ide_devices[0].cd_info.sec_type = 0x10;
	ide_devices[0].reserved     = 1;
	ide_devices[0].type         = IDE_SPI;
	ide_devices[0].drive        = 0;
	g1_ata_set_transfer_mode(ATA_TRANSFER_PIO_DEFAULT);
	g1_ata_set_transfer_mode(ATA_TRANSFER_WDMA(2));
	OUT32(G1_ATA_DMA_RACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
	OUT32(G1_ATA_DMA_WACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
	OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
	return 1;
#else

	s32 i;
	u8 j, st, err, type, count = 0;
	int d = 0;
	u16 *data = (u16 *) malloc(512);

	if (!data) {
		LOGFF("Memory failed");
		return 0;
	}

	memset(data, 0, 512);

	for (j = 0; j < MAX_DEVICE_COUNT; j++)
	{
		err = 0;
		type = IDE_ATA;
		ide_devices[j].reserved   = 0; // Assuming that no drive here.

#if defined(DEV_TYPE_IDE)
		// FIXME: Skip master drive, so do not spin GD-ROM
		// Must add new driver to DS core before.
		if (j == 0) continue;
#endif
		OUT8(G1_ATA_DEVICE_SELECT, (0xA0 | (j << 4)));
		delay_1ms();

		OUT8(G1_ATA_SECTOR_COUNT, 0);
		OUT8(G1_ATA_LBA_LOW, 0);
		OUT8(G1_ATA_LBA_MID, 0);
		OUT8(G1_ATA_LBA_HIGH, 0);

		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_IDENTIFY);
		delay_1ms();

		st = IN8(G1_ATA_STATUS_REG);

		if(!(st & (ATA_SR_DRDY | ATA_SR_DSC)) && !(st & ATA_SR_ERR))
		{
			LOGFF("%s device not found\n", dev_bus_name[j]);
			continue;
		}
		
		while (d++ < 10000)
		{
			if (st & ATA_SR_ERR)
			{
				err = 1;
				break;
			} else if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ)) {
				break; // Everything is right.
			}
			
			st = IN8(G1_ATA_STATUS_REG);
		}
		
		if (err)
		{
			OUT8(G1_ATA_COMMAND_REG, ATAPI_CMD_RESET);
			g1_ata_wait_nbsy();
			
			u8 cl = IN8(G1_ATA_LBA_MID);
			u8 ch = IN8(G1_ATA_LBA_HIGH);
			
			if ((cl == 0x14 && ch == 0xEB) || 
				(cl == 0x69 && ch == 0x96)) 
			{
				type = IDE_ATAPI;
			}
			else 
				continue; // Unknown Type (And always not be a device).
			
			OUT8(G1_ATA_COMMAND_REG, ATAPI_CMD_IDENTIFY);
			g1_ata_wait_drq();
		}
		
		for(i = 0; i < 256; i++)
			data[i] = IN16(G1_ATA_DATA);

#if defined(DEV_TYPE_GD)
		if (!memcmp(&data[40], &data[136], 192))
		{
			type = IDE_SPI;
			ide_devices[j].wdma_modes = 0x0407;
			ide_devices[j].cd_info.sec_type = 0x10;
			ide_devices[j].lba48 = 1;
			
			g1_ata_set_transfer_mode(ATA_TRANSFER_PIO_DEFAULT);
			g1_ata_set_transfer_mode(ATA_TRANSFER_WDMA(2));
			OUT32(G1_ATA_DMA_RACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
			OUT32(G1_ATA_DMA_WACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
			OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
		}
		else
#endif
		{
			
#if defined(DEV_TYPE_IDE)
			if (type == IDE_ATA)
			{
				ide_devices[j].command_sets  = (u32)(data[82]) | ((u32)(data[83]) << 16);
				ide_devices[j].capabilities  = (u32)(data[49]) | ((u32)(data[50]) << 16);
				ide_devices[j].wdma_modes = data[63];

				if (!(ide_devices[j].capabilities & (1 << 9)))
				{
					LOGF("CHS don't supported\n");
					// continue;
				}

				if(!(ide_devices[j].command_sets & (1 << 26)))
				{
					ide_devices[j].max_lba = (u64)(data[60]) | ((u64)(data[61]) << 16);
					ide_devices[j].lba48 = 0;
				}
				else
				{
					ide_devices[j].max_lba = (u64)(data[100]) | 
											((u64)(data[101]) << 16) |
											((u64)(data[102]) << 32) |
											((u64)(data[103]) << 48);
					ide_devices[j].lba48 = 1;
				}

				g1_ata_set_transfer_mode(ATA_TRANSFER_PIO_DEFAULT);

				/*  Do we support Multiword DMA mode 2? If so, enable it. Otherwise, we won't
					even bother doing DMA at all. */
				if(ide_devices[j].wdma_modes & 0x0004 && !g1_ata_set_transfer_mode(ATA_TRANSFER_WDMA(2))) 
				{
					OUT32(G1_ATA_DMA_RACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
					OUT32(G1_ATA_DMA_WACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
					OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
				}
				else 
				{
					ide_devices[j].wdma_modes = 0;
				}
			}
#elif defined(DEV_TYPE_GD)
			if (type == IDE_ATAPI) {
				ide_devices[j].cd_info.sec_type = 0x10;
				ide_devices[j].lba48 = 1;
			}
#endif
		}
		
		ide_devices[j].reserved     = 1;
		ide_devices[j].type         = type;
		ide_devices[j].drive        = j;
		count++;
	}

#ifdef LOG
	for (i = 0; i < 2; i++) {
		if (ide_devices[i].reserved == 1) 
		{
			if (ide_devices[i].type == IDE_ATA)
			{
				LOGF("%s %s ATA drive %ld Kb\n",
					dev_bus_name[i],
					ide_devices[i].lba48 ? "LBA48":"LBA28",
					(u32)(ide_devices[i].max_lba >> 1));
			}
			else
			{
				LOGF("%s %s drive\n", dev_bus_name[i], dev_proto_name[ide_devices[i].type-1]);
			}
		}
		else
		{
			LOGF("%s device not found\n", dev_bus_name[i]);
		}
	}
#endif
	free(data);
	return count;
#endif /* DEV_TYPE_EMU */
}


#ifdef DEV_TYPE_IDE

static s32 g1_ata_access(struct ide_req *req)
{
	struct ide_device *dev = req->dev;
	u8 *buff = req->buff;
	u32 count = req->count;
	u32 len;
	u64 lba = req->lba;
	u8 lba_io[6];
	u8 head;
	u16 cmd;
	const u32 sector_size = 512;

	if ((req->cmd & 2))
	{
		if ((u32)buff & 0x1f)
		{
			LOGFF("Unaligned output address: 0x%08lx (32 byte)\n", (u32)buff);
			return -1;
		}
		
		cmd = (req->cmd & 1) ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
	}
	else
	{
		if ((u32)buff & 0x01)
		{
			LOGFF("Unaligned output address: 0x%08lx (2 byte)\n", (u32)buff);
			return -1;
		}

		cmd = (req->cmd & 1) ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO;
	}
	
	// LOGFF("STATUS=%lx\n", IN8(G1_ATA_ALTSTATUS));
	g1_ata_wait_bsydrq();
	
	while(count)
	{
		// (I) Select one from LBA28, LBA48;
		if (dev->lba48) // Sure Drive should support LBA in this case, or you are
		{					  // giving a wrong LBA.
			// LBA48:
			lba_io[0] = (lba & 0x000000FF) >> 0;
			lba_io[1] = (lba & 0x0000FF00) >> 8;
			lba_io[2] = (lba & 0x00FF0000) >> 16;
			lba_io[3] = (lba & 0xFF000000) >> 24;
			lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
			lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
			head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
		}
		else
		{
			// LBA28:
			lba_io[0] = (lba & 0x00000FF) >> 0;
			lba_io[1] = (lba & 0x000FF00) >> 8;
			lba_io[2] = (lba & 0x0FF0000) >> 16;
			lba_io[3] = 0; // These Registers are not used here.
			lba_io[4] = 0; // These Registers are not used here.
			lba_io[5] = 0; // These Registers are not used here.
			head      = (lba & 0xF000000) >> 24;
		}
		
		OUT8(G1_ATA_DEVICE_SELECT, (0xE0 | (dev->drive << 4) | head));
		
		if (dev->lba48) 
		{
			len = (count > 65536) ? 65536 : count;
			count -= len;
			
			if ((req->cmd & 2))
				cmd = (req->cmd & 1) ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;
			else
				cmd = (req->cmd & 1) ? ATA_CMD_WRITE_PIO_EXT : ATA_CMD_READ_PIO_EXT;

			OUT8(G1_ATA_SECTOR_COUNT, (u8)(len >> 8));
			OUT8(G1_ATA_LBA_LOW, lba_io[3]);
			OUT8(G1_ATA_LBA_MID, lba_io[4]);
			OUT8(G1_ATA_LBA_HIGH, lba_io[5]);
		}
		else
		{
			len = (count > 256) ? 256 : count;
			count -= len;
		}

		OUT8(G1_ATA_SECTOR_COUNT, (u8)(len & 0xff));
		OUT8(G1_ATA_LBA_LOW,  lba_io[0]);
		OUT8(G1_ATA_LBA_MID,  lba_io[1]);
		OUT8(G1_ATA_LBA_HIGH, lba_io[2]);
		
		if ((req->cmd & 2))
		{
			g1_dma_abort();
			
			if (req->cmd == G1_READ_DMA) {
				 /* Invalidate the dcache over the range of the data. */
				if((u32)buff & 0xF0000000) {
					dcache_inval_range((u32) buff, req->bytes ? req->bytes : (len * sector_size));
				}
			}
#if _FS_READONLY == 0
			else {
				/* Flush the dcache over the range of the data. */
				dcache_flush_range((u32) buff, len * sector_size);
			}
#endif

			/* Set the DMA parameters up. */
			OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
			OUT32(G1_ATA_DMA_ADDRESS, ((u32) buff & 0x0FFFFFFF));
			OUT32(G1_ATA_DMA_LENGTH, req->bytes ? req->bytes : (len * sector_size));
			OUT8(G1_ATA_DMA_DIRECTION, (req->cmd == G1_READ_DMA));

			 /* Enable G1 DMA. */
			OUT8(G1_ATA_DMA_ENABLE, 1);
		}

		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		OUT8(G1_ATA_COMMAND_REG, cmd);

		if (req->cmd == G1_READ_PIO)
		{
			g1_pio_reset(len * sector_size);
			g1_pio_xfer((u32)buff, len * sector_size);
		}
#if _FS_READONLY == 0
		else if (req->cmd == G1_WRITE_PIO)
		{
			OUT8(G1_ATA_CTL, 2);
			for (u32 i = 0; i < len; i++)
			{
				dcache_pref_range((u32)buff, sector_size);
				g1_ata_wait_nbsy();

				for (u32 j = 0; j < sector_size >> 1; ++j)
				{
					OUT16(G1_ATA_DATA, (u16)(buff[0] | buff[1] << 8));
					buff += 2;
				}
				// dcache_purge_range(((u32)buff) - sector_size, sector_size);
			}
			OUT8(G1_ATA_COMMAND_REG, dev->lba48 ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
			g1_ata_wait_bsydrq();
			g1_ata_ack_irq();
			OUT8(G1_ATA_CTL, 0);
		}
#endif
		else
		{
			/* Start the DMA transfer. */
			OUT8(G1_ATA_DMA_STATUS, 1);
			
			if (req->async) {
				return 0;
			}

			buff += len;
			g1_ata_wait_dma();
			OUT8(G1_ATA_DMA_ENABLE, 0);

			if (g1_ata_ack_irq() < 0) {
				return -1;
			}

			g1_ata_wait_bsydrq();
		}
	}
	
	return 0;
}

#if defined(LOG) && defined(DEBUG)

static const s8 *dev_fs_name[] = { "FAT16", "FAT16B", "FAT32", "FAT32X", "EXT2"};

void g1_get_partition(void) 
{
	s32 i, j; 
	u8 buff[0x200];
	struct ide_req req;
	
	req.buff = (void *) buff;
	req.cmd = G1_READ_PIO;
	req.count = 1;
	req.lba = 0;

	//Опрашиваем все ATA устройства и получаем от каждого таблицу разделов: 
	for(i = 0; i < MAX_DEVICE_COUNT; i++) 
	{  
		if(!ide_devices[i].reserved || ide_devices[i].type != IDE_ATA) 
			continue;
		
		req.dev = &ide_devices[i];
		
		if(g1_ata_access(&req)) 
			continue; 
		
		/* Make sure the ATA disk uses MBR partitions.
			TODO: Support GPT partitioning at some point. */
		if(((u16 *)buff)[0xFF] != 0xAA55) 
		{
			LOGFF("ATA device doesn't appear to have a MBR %04X\n", ((u16 *)buff)[0xFF]);
			continue;
		}
		
		memcpy(ide_devices[i].pt, (struct pt_struct *)(&buff[0x1BE]), 0x40);
		
		for (j = 0; j < 4; j++)
		{
			if (!ide_devices[i].pt[j].sect_total)
				continue;
			
			u8 type = 0;
			
			switch (ide_devices[i].pt[j].type_part)
			{
				case 0x00:
					type = 0;
					break;
				case 0x04:
					type = 1;
					break;
				case 0x06:
					type = 2;
					break;
				case 0x0B:
					type = 3;
					break;
				case 0x0C:
					type = 4;
					break;
				case 0x83:
					type = 5;
					break;
				case 0x07:
					type = 6;
					break;
				case 0x05:
				case 0x0F:
				case 0xCF:
					type = 7;
					break;
				default:
					type = 8;
					break;
					
			}
			
			if (!type)
			{
				LOGF("%s device don't have partitions\n", dev_bus_name[ide_devices[i].drive]);
				break;
			}
			else if (type < 6)
			{
				LOGF("%s device part%d %s - %ld Kb\n",
					dev_bus_name[ide_devices[i].drive], j,
					dev_fs_name[type - 1], ide_devices[i].pt[j].sect_total >> 1);
			}
			else if (type < 8)
			{
				LOGF("%s device part%d EXTENDED partition don't supported\n",
					dev_bus_name[ide_devices[i].drive], j);
			}
			else
			{
				LOGF("%s device part%d partition type 0x%02X don't supported\n",
					dev_bus_name[ide_devices[i].drive], j, ide_devices[i].pt[j].type_part);
			}
			ide_devices[i].pt_num++;
		}
	}
	return;
}
#endif

s32 g1_ata_read_blocks(u64 block, size_t count, u8 *buf, u8 wait_dma) {

	const u8 drive = 1; // TODO
	struct ide_req req;

	req.buff = buf;
	req.count = count;
	req.bytes = 0;
	req.dev = &ide_devices[drive & 1];
	req.cmd = fs_dma_enabled() ? G1_READ_DMA : G1_READ_PIO;
	req.lba = block;
	req.async = (wait_dma || req.cmd == G1_READ_PIO) ? 0 : 1;

	g1_dma_part_avail = 0;

	DBGF("G1_ATA_READ: %ld %d 0x%08lx %s[%d] %s\n", (uint32)block, count, (uint32)buf,
		req.cmd == G1_WRITE_DMA ? "DMA" : "PIO", fs_dma_enabled(),
		req.async ? "ASYNC" : "BLOCKED");

	return g1_ata_access(&req);
}

#if _FS_READONLY == 0

s32 g1_ata_write_blocks(u64 block, size_t count, const u8 *buf, u8 wait_dma) {

	const u8 drive = 1; // TODO
	struct ide_req req;

	req.buff = (u8 *)buf;
	req.count = count;
	req.bytes = 0;
	req.dev = &ide_devices[drive & 1];
	req.cmd = fs_dma_enabled() ? G1_WRITE_DMA : G1_WRITE_PIO;
	req.lba = block;
	req.async = (wait_dma || req.cmd == G1_WRITE_PIO) ? 0 : 1;

	DBGF("G1_ATA_WRITE: %ld %d 0x%08lx %s[%d] %s\n", (uint32)block, count, (uint32)buf,
		req.cmd == G1_WRITE_DMA ? "DMA" : "PIO", fs_dma_enabled(),
		req.async ? "ASYNC" : "BLOCKED");

	return g1_ata_access(&req);
}

#endif

s32 g1_ata_read_lba_dma_part(u64 sector, size_t bytes, u8 *buf) {

	LOGF("G1_ATA_PART: b=%d a=%d", bytes, g1_dma_part_avail);

	if (g1_dma_part_avail > 0) {
		LOGF(" continue\n");
		g1_dma_part_avail -= bytes;
		g1_dma_start((u32)buf, bytes);
		return 0;
	}
	if (bytes > 512) {
		g1_dma_part_avail = 512 - (bytes % 512);
	} else {
		g1_dma_part_avail = 512 - bytes;
	}

	const u8 drive = 1; // TODO
	struct ide_req req;

	req.buff = buf;
	req.count = (bytes / 512) + 1;
	req.bytes = bytes;
	req.dev = &ide_devices[drive & 1];
	req.cmd = G1_READ_DMA;
	req.lba = sector;
	req.async = 1;

	LOGF(" read c=%d a=%d\n", req.count, g1_dma_part_avail);
	return g1_ata_access(&req);
}

s32 g1_ata_pre_read_lba(u64 sector, size_t count) {

	const u8 drive = 1; // TODO
	struct ide_device *dev = &ide_devices[drive & 1];
	u8 lba_io[6];
	u8 head;

	LOGF("G1_ATA_PRE_READ: s=%ld c=%ld\n", (uint32)sector, count);
	g1_dma_abort();

	// LBA48 only
	lba_io[0] = (sector & 0x000000FF) >> 0;
	lba_io[1] = (sector & 0x0000FF00) >> 8;
	lba_io[2] = (sector & 0x00FF0000) >> 16;
	lba_io[3] = (sector & 0xFF000000) >> 24;
	lba_io[4] = 0;
	lba_io[5] = 0;
	head      = 0;

	g1_ata_wait_bsydrq();
	OUT8(G1_ATA_DEVICE_SELECT, (0xE0 | (dev->drive << 4) | head));

	OUT8(G1_ATA_SECTOR_COUNT, (u8)(count >> 8));
	OUT8(G1_ATA_LBA_LOW, lba_io[3]);
	OUT8(G1_ATA_LBA_MID, lba_io[4]);
	OUT8(G1_ATA_LBA_HIGH, lba_io[5]);

	OUT8(G1_ATA_SECTOR_COUNT, (u8)(count & 0xff));
	OUT8(G1_ATA_LBA_LOW,  lba_io[0]);
	OUT8(G1_ATA_LBA_MID,  lba_io[1]);
	OUT8(G1_ATA_LBA_HIGH, lba_io[2]);

	g1_ata_wait_bsydrq();

	if (fs_dma_enabled()) {
		g1_dma_part_avail = 0;
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_DMA_EXT);
	} else {
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_PIO_EXT);
		g1_pio_reset(count * 512);
	}
	return 0;
}

s32 g1_ata_poll(void) {
	int rv = 0;

	if(!exception_inside_int() && g1_dma_in_progress()) {
		rv = g1_dma_transfered();
		DBGFF("%d\n", rv);
		return rv > 0 ? rv : 32;
	}

	if(!g1_dma_irq_visible) {
		rv = g1_ata_ack_irq();
	}

	if(!g1_dma_part_avail) {
		OUT8(G1_ATA_DMA_ENABLE, 0);
	}
	return rv;
}


#if _USE_MKFS && !_FS_READONLY
u64 g1_ata_max_lba(void) {
	
	const u8 drive = 1; // TODO
	
	return ide_devices[drive & 1].max_lba;
}
#endif

s32 g1_ata_flush(void) {
	// TODO
	return 0;
}

#endif /* DEV_TYPE_IDE */

void g1_pio_reset(size_t total_bytes) {
	if (total_bytes) {
		OUT8(G1_ATA_CTL, 2);
	} else {
		OUT8(G1_ATA_CTL, 0);
	}

	g1_pio_total = total_bytes;
	g1_pio_avail = g1_pio_total;
	g1_pio_trans = 0;
}

void g1_pio_xfer(u32 addr, size_t bytes) {
	u16 *buff = (u16 *)addr;
	const u32 sec_size = 512;
	u32 words_count = bytes >> 1;

	g1_pio_trans = 1;

	for(u32 w = 0; w < words_count; ++w) {

		if (((g1_pio_total - g1_pio_avail) % sec_size) == 0) {
			if (g1_ata_wait_drq()) {
				LOGFF("Error, status=%02x\n", IN8(G1_ATA_ALTSTATUS));
				break;
			}
		}

		buff[w] = IN16(G1_ATA_DATA);
		g1_pio_avail -= 2;

		if (g1_pio_avail == 0) {
			break;
		}
	}

	g1_pio_trans = 0;

	if (g1_pio_avail == 0) {
		g1_ata_ack_irq();
		OUT8(G1_ATA_CTL, 0);
	}
}

void g1_pio_abort(void) {
	g1_pio_reset(0);
	g1_ata_ack_irq();
}

s32 g1_pio_in_progress(void) {
	return g1_pio_trans;
}

u32 g1_pio_transfered(void) {
	return g1_pio_total - g1_pio_avail;
}


void g1_ata_xfer(u32 addr, size_t bytes) {
	if (fs_dma_enabled()) {
		g1_dma_start(addr, bytes);
	} else {
		g1_pio_xfer(addr, bytes);
	}
}

s32 g1_ata_in_progress(void) {
	if (fs_dma_enabled()) {
		return g1_dma_in_progress();
	} else {
		return g1_pio_in_progress();
	}
}

u32 g1_ata_transfered(void) {
	if (fs_dma_enabled()) {
		return g1_dma_transfered();
	} else {
		return g1_pio_transfered();
	}
}

void g1_ata_abort(void) {

	if (fs_dma_enabled()) {
		g1_dma_abort();
	} else {
		g1_pio_abort();
	}
	/*
	OUT8(G1_ATA_DEVICE_SELECT, 0x10);
	OUT8(G1_ATA_FEATURES, 0);

	g1_ata_wait_nbsy();
	OUT8(G1_ATA_COMMAND_REG, 0);

	g1_ata_wait_bsydrq();
	*/
	g1_ata_ack_irq();
}

s32 g1_ata_ack_irq(void) {
	/* Ack device IRQ. */
	u8 st = IN8(G1_ATA_STATUS_REG);

	if(st & ATA_SR_ERR || st & ATA_SR_DF) {
		LOGFF("ERR=%d DRQ=%d DSC=%d DF=%d DRDY=%d BSY=%d\n",
				(st & ATA_SR_ERR ? 1 : 0), (st & ATA_SR_DRQ ? 1 : 0), 
				(st & ATA_SR_DSC ? 1 : 0), (st & ATA_SR_DF ? 1 : 0), 
				(st & ATA_SR_DRDY ? 1 : 0), (st & ATA_SR_BSY ? 1 : 0));
		return -1;
	}
	return 0;
}

s32 g1_bus_init(void)
{

	s32 count = g1_dev_scan();

	if (!count) {
		return -1;
	}

#if defined(DEV_TYPE_IDE) && defined(DEBUG)
	g1_get_partition();
#endif

	return 0;
}


#ifdef DEV_TYPE_GD

static s32 ide_polling(u8 advanced_check) 
{
	u8 i;
	// (I) Delay 400 nanosecond for BSY to be set:
	// -------------------------------------------------
	for(i = 0; i < 4; i++)
		IN8(G1_ATA_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
	
	// (II) Wait for BSY to be cleared:
	// -------------------------------------------------
	g1_ata_wait_nbsy();
 
	if (advanced_check) 
	{
		u8 state = IN8(G1_ATA_STATUS_REG); // Read Status Register.
		
		// (III) Check For Errors:
		// -------------------------------------------------
		if (state & ATA_SR_ERR)
			return -2; // Error.
		
		// (IV) Check If Device fault:
		// -------------------------------------------------
		if (state & ATA_SR_DF)
			return -1; // Device Fault.
 
		// (V) Check DRQ:
		// -------------------------------------------------
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if ((state & ATA_SR_DRQ) == 0)
			return -3; // DRQ should be set
	}
	
	return 0; // No Error.
}

static void send_packet_command(u8 *cmd_buff, u8 drive)
{
	s32 i;
	
	/* В соответствии с алгоритмом ждем нулевого значения  битов BSY и DRQ */
	
	g1_ata_wait_bsydrq();
	
	/* Выбираем устройство и в его регистр команд записываем код пакетной команды */  
	 
	OUT8(G1_ATA_DEVICE_SELECT, 0xA0 | (drive << 4));
	
	OUT8(G1_ATA_COMMAND_REG, ATAPI_CMD_PACKET);
	
	/* Ждём сброса бита BSY и установки DRQ */
	
	if (ide_polling(1))
		return;
	
	/* Записываем в регистр данных переданный 12-байтный командный пакет */
	
	for(i = 0; i < 6; i++) 
		OUT16(G1_ATA_DATA, ((u16 *)cmd_buff)[i]);

	/* Ждём сброса бита BSY и установки DRDY */
	
	g1_ata_wait_nbsy();
	g1_ata_wait_drdy();
}

static s32 send_packet_data_command(u16 data_len, u8 *cmd_buff, u8 drive, u8 dma)
{
    s32 i, err;
	
	/* Ожидаем сброса битов BSY и DRQ */
	
	g1_ata_wait_bsydrq();

	/* Выбираем устройство */
	
	OUT8(G1_ATA_DEVICE_SELECT, 0xA0 | (drive << 4));
	
	/* В младший байт счетчика байтов (CL) заносим размер запрашиваемых данных */
	
	OUT8(G1_ATA_LBA_MID, (data_len & 0xff));
	OUT8(G1_ATA_LBA_HIGH,  (data_len >> 8));
	OUT8(G1_ATA_FEATURES, dma);
	
	/* В регистр команд записываем код пакетной команды */
	
	OUT8(G1_ATA_COMMAND_REG, ATAPI_CMD_PACKET);
	
	/* Ждём установки бита DRQ */
	
	if ((err = ide_polling(1)))
		return err;
	
	/* В регистр данных записываем 12-байтный командный пакет */
	
	for(i = 0; i < 6; i++) 
		OUT16(G1_ATA_DATA, ((u16 *)cmd_buff)[i]);

	/* Ждём завершения команды - установленного бита DRQ. Если произошла ошибка - фиксируем этот факт */
	
	err = ide_polling(1);
	
	return err;
}

static void wait_while_ready(struct ide_device *dev)
{
	u8 cmd_buff[12];
	
	if (!dev->reserved || 
		 dev->type == IDE_ATA)
		return;
	
	memset((void *)cmd_buff, 0, 12);
	
	send_packet_command(cmd_buff, dev->drive);
}

void close_cdrom(u8 drive)
{
	u8 cmd_buff[12];
	struct ide_device *dev = &ide_devices[drive & 1];
	
	if (!dev->reserved || 
		 dev->type != IDE_ATAPI)
		return;
	
	memset((void *)cmd_buff, 0, 12);
	
	cmd_buff[0] = ATAPI_CMD_EJECT; // код команды START/STOP UNIT
	cmd_buff[4] = 0x3; // LoEj = 1, Start = 1
	
	send_packet_command(cmd_buff, dev->drive);
}

void open_cdrom(u8 drive)
{
	u8 cmd_buff[12];
	struct ide_device *dev = &ide_devices[drive & 1];
	
	if (!dev->reserved || 
		 dev->type == IDE_ATA)
		return;
	
	memset((void *)cmd_buff, 0, 12);
	
	if (dev->type == IDE_ATAPI)
	{
		cmd_buff[0] = ATAPI_CMD_EJECT;
		cmd_buff[4] = 0x2; // LoEj = 1, Start = 0
	}
	else
		cmd_buff[0] = SPI_CMD_EJECT;
	
	send_packet_command(cmd_buff, dev->drive);
}

static cd_sense_t *request_sense(u8 drive)
{
	s32 i = 0;
	u8 cmd_buff[12];
	u8 type = (ide_devices[drive].type == IDE_ATAPI) ? 0 : 4;
	u8 sense_buff[14-type];
	static cd_sense_t sense;
	
#define SK (sense_buff[2] & 0x0F)
#define ASC sense_buff[12-type]
#define ASCQ sense_buff[13-type]

	memset((void *)cmd_buff, 0, 12);
	memset((void *)sense_buff, 0, 14 - type);
	
	/* Формируем пакетную команду REQUEST SENSE. Из блока sense data считываем первые 14 байт -
	 * этого нам хватит, чтобы определить причину ошибки
	 */
	cmd_buff[0] = 0x3 + (type << 2);
	cmd_buff[4] = 14-type;
	
	/* Посылаем устройству команду и считываем sense data */
	if(send_packet_data_command(14-type, cmd_buff, drive, 0) < 0) 
	{
		LOGF("Error request sense\n");
		sense.sk = 0XFF;
		return &sense;
	}
	
	for(i = 0; i < ((14-type) >> 1); i++) 
		((u16 *)sense_buff)[i] = IN16(G1_ATA_DATA);

	if (ide_polling(1)) {
		return NULL;
	}

	if (SK)
	{
		LOGF("SK/ASC/ASCQ: 0x%X/0x%X/0x%X\n", SK, ASC, ASCQ);
		
		sense.sk = SK;
		sense.asc = ASC;
		sense.ascq = ASCQ;
		
		return &sense;
	}
	
	return NULL;
}

static s32 read_toc(u8 drive)
{
	struct ide_device *dev = &ide_devices[drive & 1];
	
	if (!dev->reserved || 
		(dev->type != IDE_ATAPI &&
		 dev->type != IDE_SPI))
		return -1;
	
	s32 i, j = 0, n, total_tracks;
	u8 cmd_buff[12];
	u16 toc_len = (dev->type == IDE_ATAPI) ? 804 : 408; // toc_len - размер TOC
	u8 data_buff[toc_len];
	
	/* Формируем пакетную команду. Поле Track/Session Number содержит 0,
	* по команде READ TOC будет выдана информация обо всех треках диска,
	* начиная с первого
	*/
	
	memset((void *)cmd_buff, 0, 12);
	
	if (dev->type == IDE_ATAPI)
	{
		j = 1;
		cmd_buff[0] = ATAPI_CMD_READ_TOC;
		cmd_buff[7] = (u8)(toc_len >> 8);
		cmd_buff[8] = (u8) toc_len;
	}
	else
	{
		j = (dev->cd_info.disc_type == CD_GDROM) ? 2 : 1;
		cmd_buff[0] = SPI_CMD_READ_TOC;
		cmd_buff[3] = (u8)(toc_len >> 8);
		cmd_buff[4] = (u8) toc_len;
	}
	
	for (n = 0; n < j; n++)
	{
		//wait_while_ready(dev);
		
		/* Отправляем устройству сформированную пакетную команду */
		if(send_packet_data_command(toc_len, cmd_buff, dev->drive, 0) < 0) 
		{
			request_sense(dev->drive);
			return -1;
		}
		
		if (dev->type == IDE_ATAPI)
		{
			toc_len = swap16(IN16(G1_ATA_DATA));
			
			/* Считываем результат */
			for(i = 0; i < (toc_len >> 1); i++) 
				((u16 *)data_buff)[i] = IN16(G1_ATA_DATA);
			
			/* Номер последнего трека на диске */
			total_tracks = data_buff[1];
			
			for (i = 0; i < 99; i++)
				if (i < total_tracks)
				{
					dev->cd_info.tocs[0].entry[i] = (u32)((swap8(data_buff[i*8+2+1]) << 24) | 
																(data_buff[i*8+2+5]  << 16) | 
																(data_buff[i*8+2+6]  << 8 ) | 
																 data_buff[i*8+2+7]);
					//dev->cd_info.tocs[0].entry[i] += 150;
				}
				else
					dev->cd_info.tocs[0].entry[i] = (u32) -1;
			
			dev->cd_info.tocs[0].first = (u32)((swap8(data_buff[3]) << 24) | 
													 (data_buff[0]  << 16) );
			
			dev->cd_info.tocs[0].last  = (u32)((swap8(data_buff[(total_tracks-1)*8+3]) << 24) | 
													 (data_buff[1]					   << 16) );
			
			dev->cd_info.tocs[0].leadout_sector = (u32)((swap8(data_buff[total_tracks*8+2+1]) << 24) | 
															  (data_buff[total_tracks*8+2+5]  << 16) | 
															  (data_buff[total_tracks*8+2+6]  << 8 ) | 
															   data_buff[total_tracks*8+2+7]	   );
		}
		else
		{
			for (i = 0; i < (toc_len >> 2); i++)
			{
				((u32 *)&dev->cd_info.tocs[n])[i] = ((swap16(IN16(G1_ATA_DATA)) << 16) | swap16(IN16(G1_ATA_DATA)));
			}
			
			cmd_buff[1] = 1;
		}
	}
	
#if 0
	for (n = 0; n < j; n++)
	{
		/* Отобразим результаты чтения TOC */
		LOGF("Session: %d\t", n);
		LOGF("First: %d\t",   (u8) (dev->cd_info.tocs[n].first >> 16));
		LOGF("Last: %d\n\n" , (u8) (dev->cd_info.tocs[n].last  >> 16));
		
		total_tracks = (s32)((dev->cd_info.tocs[n].last >> 16) & 0xFF);
		
		for(i = ((u8) (dev->cd_info.tocs[n].first >> 16)) - 1; i < total_tracks; i++)
			LOGF("track: %d\tlba: %05lu\tadr/cntl: %02X\n", (i + 1), (dev->cd_info.tocs[n].entry[i] & 0xffffff), 
																		  (u8)(dev->cd_info.tocs[n].entry[i] >> 24));
		
		LOGF("lead out \tlba: %lu\tadr/cntl: %02X\n\n", (dev->cd_info.tocs[n].leadout_sector & 0xffffff),
																	  (u8)(dev->cd_info.tocs[n].leadout_sector >> 24));
	}
#endif
	
	return 0;
}

s32 cdrom_get_status(s32 *status, u8 *disc_type, u8 drive)
{
	struct ide_device *dev = &ide_devices[drive & 1];
	
	u8 cmd_buff[12];
	u8 stat, type;
	s32 i = 0;
	cd_sense_t *sense;
	memset((void *)cmd_buff, 0, 12);
	
	wait_while_ready(dev);
	
	if (dev->type == IDE_ATAPI)
	{
		cmd_buff[0] = ATAPI_CMD_MODE_SENSE;
		cmd_buff[2] = 0x0D;
		cmd_buff[8] = 4;
		
		while (send_packet_data_command(4, cmd_buff, dev->drive, 0) < 0)
		{
			i++;
			sense = request_sense(dev->drive);
			
			if (!sense)
				break;
			
			if (!(sense->sk == 6 && sense->asc == 0x29) || i > 20)
				return ERR_DISC_CHG;

			wait_while_ready(dev);
		}
		
		if (swap16(IN16(G1_ATA_DATA)) != 0x0E)
			return ERR_NO_DISC;
		
		type = (u8) IN16(G1_ATA_DATA);
		
		sense = request_sense(dev->drive);
		
		if (sense)
			return ERR_SYS;
		
		LOGFF("Disc type: %02X\n",type);
		
		if (type < 0x40)
		{
			switch (type & 0xF)
			{
				case 1:
				case 5:
					if (status)
						*status = CD_STATUS_PLAYING;
					
					if (disc_type)
						*disc_type = CD_CDROM;
					break;
				
				case 2:
				case 6:
					if (status)
						*status = CD_STATUS_PLAYING;
					
					if (disc_type)
						*disc_type = CD_CDDA;
					break;
				
				case 3:
				case 4:
				case 7:
				case 8:
					if (status)
						*status = CD_STATUS_PLAYING;
					
					if (disc_type)
						*disc_type = CD_CDROM_XA;
					break;
				
				default:
					if (status)
						*status = CD_STATUS_NO_DISC;
					return ERR_NO_DISC;
					break;
			}
			
			return ERR_OK;
		}
		else if ((type & 0xF0) == 0x40)
		{
			if (status)
				*status = CD_STATUS_PLAYING;
					
			if (disc_type)
				*disc_type = CD_DVDROM;
			
			return ERR_OK;
		}
		else if (type == 0x71)
		{
			if (status)
				*status = CD_STATUS_OPEN;
			return ERR_NO_DISC;
		}
		
		if (status)
			*status = CD_STATUS_NO_DISC;
		
		return ERR_NO_DISC;
	}
	else if (dev->type == IDE_SPI)
	{
		cmd_buff[0] = SPI_CMD_REQ_STAT;
		cmd_buff[4] = 2;
		
		if (send_packet_data_command(4, cmd_buff, dev->drive, 0) < 0)
		{
			sense = request_sense(dev->drive);
			
			if (sense)
				
				switch (sense->sk)
				{
					case 2:
					case 6:
						LOGFF("NO DISC\n");
						
						if (status)
							*status = CD_STATUS_NO_DISC;
						
						return ERR_NO_DISC;
						break;
					
					default:
						LOGF("SK = %02X\tASC = %02X\tASCQ = %02X\n", sense->sk, sense->asc, sense->ascq);
				}
			
			return ERR_SYS;
		}
		
		u16 a = swap16(IN16(G1_ATA_DATA));
		
		stat = ((a >> 8) & 0xF);
		type = ((a >> 4) & 0xF);
		
		sense = request_sense(dev->drive);
		
		if (sense)
			switch (sense->sk)
			{
				case 0:
					break;
				
				case 1:
				case 3:
				case 4:
				case 5:
					return ERR_SYS;
					break;
				case 2:
				case 6:
					LOGFF("NO DISC");
					if (status)
						*status = CD_STATUS_NO_DISC;
					return ERR_NO_DISC;
					break;
				
				default:
					return ERR_SYS;
					break;
			}
			
#ifdef DEBUG
		const s8 *status_name[] = {
			"BUSY", "PAUSE", "STANDBY", "PLAY", "SEEK",
			"SCAN", "OPEN", "NO DISC", "RETRY","ERROR"
		};
		const s8 *disc_type_name[] = {
			"CD-DA", "CD-ROM", "CD-ROM XA", "CD-I",
			NULL, NULL, NULL, NULL, "GD-ROM"
		};
		DBGF("Status: %s, disc type: %s\n", status_name[stat], disc_type_name[type]);
#endif
		if (status)
			*status = stat;

		switch (stat)
		{
			case CD_STATUS_NO_DISC:
				return ERR_NO_DISC;
				break;
		}

		if (disc_type)
			*disc_type = type << 4;
	}
	else
		return ERR_SYS;

	return ERR_OK;
}

static s32 send_packet_dma_command(u8 *cmd_buff, u8 drive, s32 timeout, u8 async)
{
	s32 i, err;
	
	g1_ata_wait_bsydrq();
	
	/* Выбираем устройство */
	
	OUT8(G1_ATA_DEVICE_SELECT, 0xA0 | (drive << 4));
	
	g1_ata_wait_bsydrq();
	
	OUT8(G1_ATA_FEATURES, 1);
	
	g1_ata_wait_bsydrq();
	
	OUT8(G1_ATA_COMMAND_REG, ATAPI_CMD_PACKET);
	
	/* Ждём сброса бита BSY и установки DRQ */
	
	ide_polling(0);
	
	/* В регистр данных записываем 12-байтный командный пакет */
	
	for(i = 0; i < 6; i++) 
		OUT16(G1_ATA_DATA, ((u16 *)cmd_buff)[i]);
	
	/*if ((err = ide_polling(1)))
		return err;*/
	
	OUT8(G1_ATA_DMA_ENABLE, 1);
	OUT8(G1_ATA_DMA_STATUS, 1);
	
	(void)timeout;

	if (!async) {
		g1_ata_wait_dma();
		OUT8(G1_ATA_DMA_ENABLE, 0);
	}

	err = ide_polling(1);
	return err;
}

static s32 g1_packet_read(struct ide_req *req)
{
	s32 err;
	u8 cmd_buff[12];
	u16 *buff = req->buff;
	u32 i, j, data_len;
	
	switch (req->dev->cd_info.sec_type)
	{
		case 0x10:
			data_len = 1024;
			break;
		
		case 0xF8:
			data_len = 1176;
			break;
		
		default:
			return 0;
			break;
	}
	
	memset((void *)cmd_buff, 0, 12);
	
	/* Формируем командный пакет */
	if (req->dev->type == IDE_ATAPI)
	{
		cmd_buff[2] = (u8)(req->lba >> 24);
		cmd_buff[3] = (u8)(req->lba >> 16);
		cmd_buff[4] = (u8)(req->lba >>  8);
		cmd_buff[5] = (u8)(req->lba >>  0);
		
		if (req->dev->cd_info.disc_type != CD_DVDROM)
		{
			cmd_buff[0] = ATAPI_CMD_READ_CD; // код команды READ CD
			cmd_buff[1] = 0; // считываем сектор любого типа (Any Type)
			cmd_buff[6] = (u8)(req->count >> 16);
			cmd_buff[7] = (u8)(req->count >>  8);
			cmd_buff[8] = (u8)(req->count >>  0);
			cmd_buff[9] = req->dev->cd_info.sec_type;
		}
		else
		{
			cmd_buff[0] = ATAPI_CMD_READ; // код команды READ
			cmd_buff[6] = (u8)(req->count >> 24);
			cmd_buff[7] = (u8)(req->count >> 16);
			cmd_buff[8] = (u8)(req->count >>  8);
			cmd_buff[9] = (u8)(req->count >>  0);
		}
	}
	else
	{
		cmd_buff[0] = SPI_CMD_READ_CD;
		cmd_buff[1] = req->dev->cd_info.sec_type << 1;
		cmd_buff[2] = (u8)(req->lba >> 16);
		cmd_buff[3] = (u8)(req->lba >>  8);
		cmd_buff[4] = (u8)(req->lba >>  0);
		cmd_buff[8] = (u8)(req->count >> 16);
		cmd_buff[9] = (u8)(req->count >>  8);
		cmd_buff[10] =(u8)(req->count >>  0);
	}
	
	if (req->cmd & 2)
	{
		if ((u32)buff & 0x1f)
		{
			LOGFF("Unaligned output address: 0x%08lx (32 byte)\n", (u32)buff);
			err = -1;
			goto exit_packet_read;
		}
		
		g1_dma_abort();
		OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
		OUT32(G1_ATA_DMA_ADDRESS, ((u32) buff) & 0x0FFFFFFF);
		OUT32(G1_ATA_DMA_LENGTH, req->bytes ? req->bytes : (req->count * (data_len << 1)));
		OUT8(G1_ATA_DMA_DIRECTION, G1_DMA_TO_MEMORY);
		
		send_packet_dma_command(cmd_buff, req->dev->drive, req->count*100+5000, req->async);
	}
	else
	{

		if ((u32)buff & 0x01)
		{
			LOGFF("Unaligned output address: 0x%08lx (2 byte)\n", (u32)buff);
			err = -1;
			goto exit_packet_read;
		}

		/* Посылаем устройству командный пакет */
	
		if(send_packet_data_command((data_len << 1), cmd_buff, req->dev->drive, (req->cmd >> 1)) < 0) 
		{
			request_sense(req->dev->drive); 
			err = -1;
			goto exit_packet_read;
		}
	
		/* Считываем результат */
		for (j = 0; j < req->count; j++)
		{
			if ((err = ide_polling(1)))
				goto exit_packet_read;
			
			for(i = 0; i < data_len; i++)
			{
				buff[(j*data_len)+i] = IN16(G1_ATA_DATA);
			}
		}
	}
	
	
	err = (!request_sense(req->dev->drive)) ? 0 : -1;
	
exit_packet_read:
	return err;
}

static s32 spi_check_license(void)
{
	u8 cmd_buff[12];
	
	if (ide_devices[0].type != IDE_SPI)
		return -1;
	
	wait_while_ready(ide_devices);
	
	memset((void *)cmd_buff, 0, 12);
	
	cmd_buff[0] = 0x70;
	cmd_buff[1] = 0x1F;
	
	send_packet_command(cmd_buff, 0);
	
	return 0;
}

void cdrom_spin_down(u8 drive)
{
	u8 cmd_buff[12];
	
	drive &= 1;
	
	if (!ide_devices[drive].reserved)
		return;
	
	memset((void *)cmd_buff, 0, 12);
	
	if (ide_devices[drive].type == IDE_SPI)
	{
		cmd_buff[0] = SPI_CMD_SEEK;
		cmd_buff[1] = 3;
	}
	else
	if (ide_devices[drive].type == IDE_ATAPI)
	{
		cmd_buff[0] = ATAPI_CMD_EJECT;
	}
	else
		return;
	
	send_packet_command(cmd_buff, 0);
}

void spi_req_mode(void)
{
	u8 cmd_buff[12];
	s32 i;
	
	memset((void *)cmd_buff, 0, 12);
	
	cmd_buff[0] = SPI_CMD_REQ_MODE;
	cmd_buff[2] = 0x12;
	cmd_buff[4] = 8;
	
	wait_while_ready(ide_devices);
	send_packet_data_command(8, cmd_buff, 0, 0);
	
	for (i = 0; i < 4; i++)
		IN16(G1_ATA_DATA);
}

void packet_soft_reset(u8 drive)
{
	drive &= 1;
	
	OUT8(G1_ATA_DEVICE_SELECT, 0xA0 | (drive << 4));
	OUT8(G1_ATA_COMMAND_REG, 0x08);
	g1_ata_wait_nbsy();
}

s32 cdrom_read_toc(CDROM_TOC *toc_buffer, u8 session, u8 drive)
{
	struct ide_device *dev = &ide_devices[drive & 1];
	
	if (!dev->reserved || 
		(dev->type != IDE_ATAPI &&
		 dev->type != IDE_SPI))
		return -1;
	
	session &= 1;
	
	if (session && dev->type != IDE_SPI)
		return -1;
	
	memcpy((void *) toc_buffer, (void *) &dev->cd_info.tocs[session], sizeof(CDROM_TOC));
	
	return 0;
}

CDROM_TOC *cdrom_get_toc(u8 session, u8 drive) 
{
	struct ide_device *dev = &ide_devices[drive & 1];
	
	if (!dev->reserved || 
		(dev->type != IDE_ATAPI &&
		 dev->type != IDE_SPI))
		return NULL;
	
	session &= 1;
	
	if (session && dev->type != IDE_SPI)
		return NULL;
		
	return &dev->cd_info.tocs[session];
}

/* Locate the LBA sector of the data track; use after reading TOC */
u32 cdrom_locate_data_track(CDROM_TOC *toc) 
{
    s32 i, first, last;

    first = TOC_TRACK(toc->first);
    last = TOC_TRACK(toc->last);

    if(first < 1 || last > 99 || first > last)
        return -1;

    /* Find the last track which as a CTRL of 4 */
    for(i = last; i >= first; i--) 
    {
        if(TOC_CTRL(toc->entry[i - 1]) == 4 || TOC_CTRL(toc->entry[i - 1]) == 7)
            return TOC_LBA(toc->entry[i - 1]);
    }

    return -1;
}

void cdrom_set_sector_size(s32 sec_size, u8 drive)
{
	switch (sec_size)
	{
		case 2352:
			ide_devices[drive&1].cd_info.sec_type = 0xF8;
			break;
		
		case 2048:
		default:
			ide_devices[drive&1].cd_info.sec_type = 0x10;
	}
	
}

s32 cdrom_reinit(s32 sec_size, u8 drive)
{
	cdrom_set_sector_size(sec_size, drive);
	
	if (cdrom_get_status(NULL, &ide_devices[drive].cd_info.disc_type, drive))
		return -2;
	
	if (ide_devices[drive&1].type == IDE_SPI)
		spi_check_license();
	
	return read_toc(drive);
}

s32 cdrom_chk_disc_change(u8 drive)
{
	wait_while_ready(&ide_devices[drive&1]);
	
	if (request_sense(drive))
		return -1;
	
	return 0;
}

s32 cdrom_cdda_play(u32 start, u32 end, u32 loops, s32 mode)
{
	(void) start;
	(void) end;
	(void) loops;
	(void) mode;
	
	return 0;
}

s32 cdrom_cdda_pause()
{
	return 0;
}

s32 cdrom_cdda_resume()
{
	return 0;
}

s32 cdrom_read_sectors(void *buffer, u32 sector, u32 cnt, u8 drive)
{
	struct ide_req req;
	
	req.buff = buffer;
	req.count = cnt;
	req.dev = &ide_devices[drive & 1];
	req.cmd = G1_READ_DMA;
	req.lba = sector;
	req.async = 0;
	g1_dma_part_avail = 0;
	
	if ((((u32) buffer) & 0x1f))
	{
		LOGFF("PIO\n");
		req.cmd = G1_READ_PIO;
	}
	else
	{
		LOGFF("DMA\n");
	}
	
	return g1_packet_read(&req);
}

s32 cdrom_read_sectors_ex(void *buffer, u32 sector, u32 cnt, u8 async, u8 dma, u8 drive)
{
	struct ide_req req;
	
	req.buff = buffer;
	req.count = cnt;
	req.bytes = 0;
	req.dev = &ide_devices[drive & 1];
	req.cmd = dma ? G1_READ_DMA : G1_READ_PIO;
	req.lba = sector;
	req.async = async;
	g1_dma_part_avail = 0;
	
	if (dma && (((u32) buffer) & 0x1f)) {
		req.cmd = G1_READ_PIO;
		req.async = 0;
	}

	LOGFF("%s%s READ\n", async ? "ASYNC " : "", dma ? "DMA" : "PIO");

	if (dma) {
		g1_dma_set_irq_mask(fs_dma_enabled() != FS_DMA_HIDDEN);
	}
	
	return g1_packet_read(&req);
}

s32 cdrom_read_sectors_part(void *buffer, u32 sector, size_t offset, size_t bytes, u8 drive) {

	if(offset && g1_dma_part_avail > 0) {

		g1_dma_part_avail -= bytes;
		g1_dma_start((u32)buffer, bytes);
		return 0;
	}

	g1_dma_part_avail = 512 - bytes;
	struct ide_req req;

	req.buff = buffer;
	req.count = 1;
	req.bytes = bytes;
	req.dev = &ide_devices[drive & 1];
	req.cmd = G1_READ_DMA;
	req.lba = sector;
	req.async = 1;
	
	g1_dma_set_irq_mask(1);
	return g1_packet_read(&req);
}

s32 cdrom_pre_read_sectors(u32 sector, size_t bytes, u8 drive) {
	// TODO
	(void)sector;
	(void)bytes;
	(void)drive;
	return 0;
}

u8 cdrom_get_dev_type(u8 drive)
{
	return ide_devices[drive].type;
}
#endif /* DEV_TYPE_GD */

