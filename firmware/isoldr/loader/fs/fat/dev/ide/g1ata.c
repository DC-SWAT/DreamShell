/* KallistiOS ##version##

   hardware/g1ata.c
   Copyright (C) 2013-2014 Lawrence Sebald
   Copyright (C) 2013-2016 SWAT <http://www.dc-swat.ru>
*/

#include <main.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <arch/timer.h>
#include <dc/sq.h>
#include <exception.h>
#include <asic.h>
#include <mmu.h>
#include "g1ata.h"

#define USE_BYTE_ACCESS 1


/*
   This file implements support for accessing devices over the G1 bus by the
   AT Attachment (aka ATA, PATA, or IDE) protocol. See, the GD-ROM drive is
   actually just an ATA device that implements a different packet command set
   than the normal ATAPI set. Not only that, but Sega left everything in the
   hardware to actually support two devices on the bus at a time. Thus, you can
   put together a very simple passthrough adapter to get a normal 40-pin IDE
   port to work with and hook up a hard drive to. In theory, you could also hook
   up various other devices including DVD drives, CD Burners, and the whole nine
   yards, but for now this just supports hard drives (and Compact Flash cards).

   The setup here is relatively simple, because we only have one channel which
   can have a maximum of two devices attached to it at a time. Normally the
   primary device would be the GD-ROM drive itself, so we're only actually
   concerned with the secondary device (use the normal cdrom_* functions to
   access the GD-ROM drive -- there's not a particularly compelling reason to
   support its odd packet interface here). Also, at the moment, only PIO
   transfers are supported. I'll look into DMA at some point in the future.

   There are a few potentially useful outward facing functions here, but most of
   the time all you'll need here is the function to get a block device for a
   given partition. The individual block read/write functions are all public as
   well, in case you have a reason to want to use them directly. Just keep in
   mind that all block numbers in those are absolute (i.e, not offset by any
   partition boundaries or whatnot).

   If you want to learn more about ATA, look around the internet for the
   AT Attachment - 8 ATA/ATAPI Command Set document. That's where most of the
   fun stuff in here comes from. Register locations and such were derived from
   a couple of different sources, including Quzar's GDINFO program, my own SPI
   CD Player program (which I should eventually release), and the source code to
   the emulator NullDC. Also, various postings at OSDev were quite useful in 
   working some of this out.

   Anyway, that's enough for this wall of text...
*/

/* An ATA device. For the moment, we only support one of these, which happens to
   be the slave device on the only ATA bus Sega gave us. */
static struct {
    uint32_t command_sets;
    uint32_t capabilities;
    uint64_t max_lba;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint16_t wdma_modes;
} device;


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
#define G1_ATA_CHS_SECTOR       G1_ATA_LBA_LOW
#define G1_ATA_CHS_CYL_LOW      G1_ATA_LBA_MID
#define G1_ATA_CHS_CYL_HIGH     G1_ATA_LBA_HIGH
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

/* Bitmasks for the STATUS_REG/ALT_STATUS registers. */
#define G1_ATA_SR_ERR   0x01
#define G1_ATA_SR_IDX   0x02
#define G1_ATA_SR_CORR  0x04
#define G1_ATA_SR_DRQ   0x08
#define G1_ATA_SR_DSC   0x10
#define G1_ATA_SR_DF    0x20
#define G1_ATA_SR_DRDY  0x40
#define G1_ATA_SR_BSY   0x80

/* ATA Commands we might like to send. */
#define ATA_CMD_RECALIBRATE         0x10
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_READ_SECTORS_EXT    0x24
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_WRITE_SECTORS_EXT   0x34
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_SPINDOWN            0xE0
#define ATA_CMD_SPINUP              0xE1
#define ATA_CMD_STANDBY_5SU         0xE2
#define ATA_CMD_IDLE_5SU            0xE3
#define ATA_CMD_SLEEP               0xE6
#define ATA_CMD_FLUSH_CACHE         0xE7
#define ATA_CMD_FLUSH_CACHE_EXT     0xEA
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_SET_FEATURES        0xEF
#define ATA_CMD_STANDBY_01SU        0xF2
#define ATA_CMD_IDLE_01SU           0xF3

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
#define OUT32(addr, data) *((volatile uint32_t *)addr) = data
#define OUT16(addr, data) *((volatile uint16_t *)addr) = data
#define OUT8(addr, data)  *((volatile uint8_t  *)addr) = data
#define IN32(addr)        *((volatile uint32_t *)addr)
#define IN16(addr)        *((volatile uint16_t *)addr)
#define IN8(addr)         *((volatile uint8_t  *)addr)

static int devices = 0;
static uint8_t dev_selected = 0x00;
static int dma_in_progress = 0;
static int dma_irq_inited = 0;

#define g1_ata_wait_dma() \
    do {} while(IN32(G1_ATA_DMA_STATUS))

#define g1_ata_wait_status(n) \
    do {} while((IN8(G1_ATA_ALTSTATUS) & (n)))


#define g1_ata_wait_drdy() \
    do {} while(!(IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_DRDY))

#define g1_ata_wait_nbsy() g1_ata_wait_status(G1_ATA_SR_BSY)

#define g1_ata_wait_bsydrq() g1_ata_wait_status(G1_ATA_SR_DRQ | G1_ATA_SR_BSY)


static inline int use_lba28(uint64_t sector, size_t count) {
    return ((sector + count) < 0x0FFFFFFF) && (count <= 256);
}

#define CAN_USE_LBA48() ((device.command_sets & (1 << 26)))


/* Is a G1 DMA in progress? */
int g1_dma_in_progress(void) {
#if defined(LOG) && defined(DEBUG)
	uint32 status = IN32(G1_ATA_DMA_STATUS);
	if(status)
		DBGFF("%08lx\n", status);
	return status;
#else
    return IN32(G1_ATA_DMA_STATUS);
#endif
}

uint32_t g1_dma_transfered(void) {
	return IN32(G1_ATA_DMA_LEND);
}

static int g1_dma_irq_visible = 0;
static int _g1_dma_irq_enabled = 0;
static int g1_dma_part_avail = 0;
//static int pre_read_progress = 0;

int g1_dma_irq_enabled() {
	return _g1_dma_irq_enabled;
}

void *g1_dma_handler(void *passer, register_stack *stack, void *current_vector) {
	
	uint32 st = 0;
	
#ifdef LOG
	if(stack != NULL) {
		st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
		LOGFF("%lx %08lx %08lx %d %d\n", *REG_INTEVT & 0x0fff, st, 
				ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT],
				dma_in_progress, g1_dma_irq_visible);
//		dump_regs(stack);
	}
#endif
	
//	if(pre_read_progress) {
//		pre_read_progress = 0;
//		dma_in_progress = 0;
//		return current_vector;
//	}
	
	if(!_g1_dma_irq_enabled) {
		_g1_dma_irq_enabled = 1;
	}
	
	if(dma_in_progress <= 0) {
		
		dma_in_progress = 0;
		
		if(!g1_dma_irq_visible) {
			ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
			st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
			st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
			return my_exception_finish;
		}
		
		return current_vector;

	} else {
	
		if(!g1_dma_irq_visible) {

			/* Ack the IRQ. */
			uint8_t status = IN8(G1_ATA_STATUS_REG);
			
			if(status & G1_ATA_SR_ERR) {

				LOGFF("ERR=%d DRQ=%d DSC=%d DF=%d DRDY=%d BSY=%d\n",
						(status & G1_ATA_SR_ERR ? 1 : 0), (status & G1_ATA_SR_DRQ ? 1 : 0), 
						(status & G1_ATA_SR_DSC ? 1 : 0), (status & G1_ATA_SR_DF ? 1 : 0), 
						(status & G1_ATA_SR_DRDY ? 1 : 0), (status & G1_ATA_SR_BSY ? 1 : 0));

				dma_in_progress = -1;
			} else {
				dma_in_progress = 0;
			}

		} else {
			dma_in_progress = 0;
		}
	}
	
	if(stack != NULL) {
		
		if(!g1_dma_irq_visible) {
			
			ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
			st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
			st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
			
			/* Processing filesystem */
			poll_all();
			return my_exception_finish;
		}
		
		/* Processing filesystem */
		poll_all();
		return current_vector;
		
	} else {
		(void)st;
		(void)passer;
		return current_vector;
	}
}

int g1_dma_init_irq() {

	if(dma_irq_inited)
		return 0;
		
	dma_irq_inited = 1;

#ifdef NO_ASIC_LT

	return 0;
	
#else
	asic_lookup_table_entry a_entry;
	
	a_entry.irq = EXP_CODE_ALL;
	a_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
	a_entry.mask[ASIC_MASK_EXT_INT] = 0; //ASIC_EXT_GD_CMD;
	a_entry.mask[ASIC_MASK_ERR_INT] = 0; /*ASIC_ERR_G1DMA_ILLEGAL | 
											ASIC_ERR_G1DMA_OVERRUN | 
											ASIC_ERR_G1DMA_ROM_FLASH;*/
	a_entry.handler = g1_dma_handler;
	return asic_add_handler(&a_entry, NULL, 0);
#endif
}


void g1_dma_set_irq_mask(int enable) {
	
	if(fs_dma_enabled() == FS_DMA_HIDDEN) {
		/* Hide all internal DMA transfers (CDDA, etc...) */
		if(g1_dma_irq_visible) {
			*ASIC_IRQ11_MASK &= ~ASIC_NRM_GD_DMA;
			*ASIC_IRQ9_MASK |= ASIC_NRM_GD_DMA;
			g1_dma_irq_visible = 0;
		}
		return;
	}
	
	if(!enable && ((*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) || !(*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA))) {
		
		*ASIC_IRQ11_MASK &= ~ASIC_NRM_GD_DMA;
		*ASIC_IRQ9_MASK |= ASIC_NRM_GD_DMA;
		
		LOGFF("%d %d %d\n",
			(*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA) ? 1 : 0, 
			(*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) ? 1 : 0, 
			(*ASIC_IRQ13_MASK & ASIC_NRM_GD_DMA) ? 1 : 0);
		
	} else if(enable && (!(*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) || (*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA))) {
		
		*ASIC_IRQ11_MASK |= ASIC_NRM_GD_DMA;
		*ASIC_IRQ9_MASK &= ~ASIC_NRM_GD_DMA;
		
		LOGFF("%d %d %d\n",
			(*ASIC_IRQ9_MASK & ASIC_NRM_GD_DMA) ? 1 : 0, 
			(*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA) ? 1 : 0, 
			(*ASIC_IRQ13_MASK & ASIC_NRM_GD_DMA) ? 1 : 0);
	}

	g1_dma_irq_visible = enable;
}

int g1_ata_poll() {
	
	int rv = 0;
	
	if(exception_inside_int()) {

		rv = dma_in_progress;
		dma_in_progress = 0;
		return rv;

	} else if(g1_dma_in_progress()) {
		
		rv = IN32(G1_ATA_DMA_LEND);
		DBGFF("%d\n", rv);
		return rv > 0 ? rv : 32;
		
	} else if(dma_in_progress < 0) {
		
		rv = -1;
		
	} else if(dma_in_progress > 0) {

		g1_dma_handler(NULL, NULL, NULL);
	
		if(dma_in_progress < 0) {
			rv = -1;
		}
	}
	
	dma_in_progress = 0;
	
	if(!rv) {
		g1_ata_wait_bsydrq();
	}
	
	return rv;
}


int g1_ata_abort() {
	
	dma_in_progress = 0;
	g1_dma_part_avail = 0;
	
	if(IN32(G1_ATA_DMA_STATUS)) {
		OUT32(G1_ATA_DMA_ENABLE, 0);
		/* Wait until the drive is ready */
		g1_ata_wait_dma();
		dma_in_progress = 0;
		g1_ata_wait_bsydrq();
	}
	
	// TODO: send NOP command to device
	
	return 0;
}


/* Set the device select register to select a particular device. */
uint8_t g1_ata_select_device(uint8_t dev) {
    uint8_t old = IN8(G1_ATA_DEVICE_SELECT);

    /* Are we actually switching devices? */
    if(((dev ^ dev_selected) & 0x10)) {
        /* We might run into some trouble here if this is called in an IRQ
           handler, so treat that case specially... */
//        if(exception_inside_int()) {
//            /* If there's a DMA going, then punt. We don't want to sit around
//               forever waiting... */
//            if(g1_dma_in_progress())
//                return 0x0F;
//
//            if(IN8(G1_ATA_ALTSTATUS) & (G1_ATA_SR_DRQ | G1_ATA_SR_BSY))
//                return 0x0F;
//        }
//        else {
            /* Wait for any in-progress DMA transfers to finish. */
            g1_ata_wait_dma();

            /* According to section 7.10 of the ATA-5 spec, setting the device
               select register with either of BSY or DRQ asserted produces an
               indeterminite result. */
            g1_ata_wait_bsydrq();
//        }
    }

    /* Write the register value out and return the old value. */
    OUT8(G1_ATA_DEVICE_SELECT, dev);
    dev_selected = dev;

    return old;
}

/* This one is an inline function since it needs to return something... */
static inline int g1_ata_wait_drq(void) {
    uint8_t val = IN8(G1_ATA_ALTSTATUS);

    while(!(val & G1_ATA_SR_DRQ) && !(val & (G1_ATA_SR_ERR | G1_ATA_SR_DF))) {
        val = IN8(G1_ATA_ALTSTATUS);
    }

    return (val & (G1_ATA_SR_ERR | G1_ATA_SR_DF)) ? -1 : 0;
}

static void _g1_dma_start(uint8_t cmd, size_t bytes, uint32_t addr, int dir) {

    LOGF("g1_dma_start: %d 0x%08lx\n", bytes, addr);
	
    /* Set the DMA parameters up. */
    OUT32(G1_ATA_DMA_ADDRESS, addr);
    OUT32(G1_ATA_DMA_LENGTH, bytes);
    OUT32(G1_ATA_DMA_DIRECTION, dir);

    /* Enable G1 DMA. */
    OUT32(G1_ATA_DMA_ENABLE, 1);

	if(cmd != 0) {
		/* Wait until the drive is ready to accept the command. */
		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		/* Write out the command to the device. */
		OUT8(G1_ATA_COMMAND_REG, cmd);
	}

    /* Start the DMA transfer. */
    OUT32(G1_ATA_DMA_STATUS, 1);
}

void g1_dma_start(uint32_t addr, size_t bytes) {
//	g1_ata_wait_drq();
	dma_in_progress = 1;
//	pre_read_progress = 1;

	_g1_dma_start(0, bytes, addr, G1_DMA_TO_MEMORY);
}


static int dma_common(uint8_t cmd, size_t nsects, uint32_t addr, int dir,
                      int block) {
    uint8_t status = 0;

    _g1_dma_start(cmd, nsects * 512, addr, dir);

    if(block) {

        g1_ata_wait_dma();

        /* Ack the IRQ. */
        status = IN8(G1_ATA_STATUS_REG);

        /* Was there an error doing the transfer? */
        if(status & G1_ATA_SR_ERR) {
			LOGFF("PTR=0x%08lx CNT=%d ERR=%d DRQ=%d DSC=%d DF=%d DRDY=%d BSY=%d\n",
					addr, nsects, 
					(status & G1_ATA_SR_ERR ? 1 : 0), (status & G1_ATA_SR_DRQ ? 1 : 0), 
					(status & G1_ATA_SR_DSC ? 1 : 0), (status & G1_ATA_SR_DF ? 1 : 0), 
					(status & G1_ATA_SR_DRDY ? 1 : 0), (status & G1_ATA_SR_BSY ? 1 : 0));
			dma_in_progress = 0;
			return -1;
        }
		
        /* Since we're blocking, make sure the drive is completely done. */
        g1_ata_wait_bsydrq();
        dma_in_progress = 0;
    }

    return 0;
}


/* This code not used because inside interrupt the masks doesn't apply =(
 * but it's works good =)
static int dma_common_part(uint8_t cmd, size_t offset, size_t bytes, uint32_t addr) {
	
	uint8_t restore_dma_irq = 0;
	size_t end = 512 - (offset + bytes);
	uint32_t sb = ((uint32_t)sector_buffer) & 0x0FFFFFFF;
	int old, inside_irq = exception_inside_irq();
	
	if(inside_irq) {
		old = irq_disable();
	}
	
	LOGFF("%d %d 0x%08lx\n", offset, bytes, addr);

	g1_dma_part_avail = 1;

	if((*ASIC_IRQ11_MASK & ASIC_NRM_GD_DMA)) {
		*ASIC_IRQ11_MASK &= ~ASIC_NRM_GD_DMA;
		restore_dma_irq = 1;
		
	}
	
	if(offset > 0) {
		
		_g1_dma_start(cmd, offset, sb, G1_DMA_TO_MEMORY);

		// Wait DMA complete.
		g1_ata_wait_dma();
	}
	
	if(!end) {
		
		if(restore_dma_irq) {
			*ASIC_IRQ11_MASK |= ASIC_NRM_GD_DMA;
		}
		
		if(inside_irq) {
			irq_restore(old);
		}
		g1_dma_part_avail = 0;
	}

	_g1_dma_start((offset == 0 ? cmd : 0), bytes, addr, G1_DMA_TO_MEMORY);

	if(end > 0) {
		
		// Wait DMA complete.
		g1_ata_wait_dma();
		
		if(restore_dma_irq) {
			*ASIC_IRQ11_MASK |= ASIC_NRM_GD_DMA;
		}
		
		g1_dma_part_avail = 0;
		
		if(inside_irq) {
			irq_restore(old);
		}
		_g1_dma_start(0, end, sb, G1_DMA_TO_MEMORY);
	}

	return 0;
}
*/

int g1_ata_read_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                    uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
#ifdef USE_BYTE_ACCESS
    uint16_t data;
    uint8_t *pdata = (uint8_t *)&data;
    uint8_t *buff = (uint8_t*)buf;
#endif

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        g1_ata_select_device(G1_ATA_SLAVE | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW,  (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h == device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }
			
//            dcache_pref_range((uint32)buff, 512);

            /* Wait for data */
            if(g1_ata_wait_drq()) {
				LOGFF("error reading CHS "
                       "%d, %d, %d of device: %02x\n", (int)c, (int)h, (int)s,
                       IN8(G1_ATA_ALTSTATUS));
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
#ifdef USE_BYTE_ACCESS
                data = IN16(G1_ATA_DATA);
                buff[0] = pdata[0];
                buff[1] = pdata[1];
                buff += 2;
#else
                *buf++ = IN16(G1_ATA_DATA);
#endif
            }
        }
    }

    rv = 0;

out:
    return rv;
}


int g1_ata_read_lba(uint64_t sector, size_t count, uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
	
#ifdef USE_BYTE_ACCESS
    uint16_t data;
    uint8_t *pdata = (uint8_t *)&data;
    uint8_t *buff = (uint8_t*)buf;
	
//    int need_sq = (((uint32_t)buf) & 0xF0000000);
//	
//	if(!need_sq && mmu_is_enabled()) {
//		buff = sector_buffer;
//		need_sq = 1;
//	} else {
//		need_sq = 0;
//	}
	
#endif

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Make sure the disk supports LBA mode. */
//    if(!device.max_lba) {
//        return -1;
//    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        if((sector + nsects) <= 0x0FFFFFFF) {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                                 ((sector >> 24) & 0x0F));

            /* Write out the number of sectors we want and the lower 24-bits of
               the LBA we're looking for. */
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Wait until the drive is ready to accept the command. */
            g1_ata_wait_nbsy();
            g1_ata_wait_drdy();

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);
        }
        else {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

            /* Write out the number of sectors we want and the LBA. */
            OUT8(G1_ATA_SECTOR_COUNT, 0);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Wait until the drive is ready to accept the command. */
            g1_ata_wait_nbsy();
            g1_ata_wait_drdy();

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS_EXT);
        }

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
			
//            dcache_alloc_range((uint32)buff, 512);
			
            /* Wait for data */
            if(g1_ata_wait_drq()) {
				LOGFF("error reading sector %d "
                       "of device: %02x\n", (int)sector, IN8(G1_ATA_ALTSTATUS));
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
#ifdef USE_BYTE_ACCESS
                data = IN16(G1_ATA_DATA);
                buff[0] = pdata[0];
                buff[1] = pdata[1];
                buff += 2;
#else
                *buf++ = IN16(G1_ATA_DATA);
#endif
            }
			
//#ifdef USE_BYTE_ACCESS
//            if(need_sq) {
//				sq_cpy(buf, buff, 512);
//				buf += 512;
//				buff = sector_buffer;
//            }
//#endif
        }
    }

    rv = 0;
    
out:
    return rv;
}


int g1_ata_read_lba_part(uint64_t sector, size_t offset, size_t bytes, uint8_t *buf) {
    int rv = 0;
    unsigned int i, data;
	uint16_t *buff = (uint16_t *)buf;
	size_t end = 512 - (offset + bytes);

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

	/* Which mode are we using: LBA28 or LBA48? */
	if(sector <= 0x0FFFFFFF) {
		g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
							 ((sector >> 24) & 0x0F));

		/* Write out the number of sectors we want and the lower 24-bits of
		   the LBA we're looking for. */
		OUT8(G1_ATA_SECTOR_COUNT, 1);
		OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
		OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
		OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

		/* Wait until the drive is ready to accept the command. */
		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		/* Write out the command to the device. */
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);
	}
	else {
		g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

		/* Write out the number of sectors we want and the LBA. */
		OUT8(G1_ATA_SECTOR_COUNT, 0);
		OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
		OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
		OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
		OUT8(G1_ATA_SECTOR_COUNT, 1);
		OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
		OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
		OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

		/* Wait until the drive is ready to accept the command. */
		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		/* Write out the command to the device. */
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS_EXT);
	}

	/* Wait for data */
	if(g1_ata_wait_drq()) {
		LOGFF("error reading sector %d "
			   "of device: %02x\n", (int)sector, IN8(G1_ATA_ALTSTATUS));
		rv = -1;
		goto out;
	}
	
	for(i = 0; i < offset; ++i) {
		data = IN16(G1_ATA_DATA);
	}

	for(i = 0; i < (bytes >> 1); ++i) {
		*buff++ = IN16(G1_ATA_DATA);
	}
	
	for(i = 0; i < end; ++i) {
		data = IN16(G1_ATA_DATA);
	}

	(void)data;
    rv = 0;
    
out:
    return rv;
}

int g1_ata_pre_read_lba(uint64_t sector, size_t count) {

    int /*old, */can_lba48 = CAN_USE_LBA48();

	LOGFF("%ld %ld\n", (int)sector, count);

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        return -1;
    }
	
    /* Disable IRQs temporarily... */
//    old = irq_disable();

    /* Make sure there is no DMA in progress already. */
//    if(/*dma_in_progress || */g1_dma_in_progress()) {
//        LOGFF("DMA in progress: %d %d\n", 
//					__func__, dma_in_progress, g1_dma_in_progress());
//        return -1;
//    }
	
    /* Set the settings for this transfer and reenable IRQs. */
//    dma_in_progress = 1;
//    irq_restore(old);

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* Which mode are we using: LBA28 or LBA48? */
    if(!can_lba48 || use_lba28(sector, count)) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));

        /* Write out the number of sectors we want and the lower 24-bits of
           the LBA we're looking for. Note that putting 0 into the sector count
           register returns 256 sectors. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));
		
		/* Wait until the drive is ready to accept the command. */
		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		/* Write out the command to the device. */
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_DMA);
    }
    else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

        /* Write out the number of sectors we want and the LBA. Note that in
           LBA48 mode, putting 0 into the sector count register returns 65536
           sectors (not that we have that much RAM on the Dreamcast). */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(count >> 8));
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

		/* Wait until the drive is ready to accept the command. */
		g1_ata_wait_nbsy();
		g1_ata_wait_drdy();

		/* Write out the command to the device. */
		OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_DMA_EXT);
    }
	
	/* Wait for data */
//	if(g1_ata_wait_drq()) {
//		LOGFF("error reading sector %d "
//			   "of device: %02x\n", (int)sector, IN8(G1_ATA_ALTSTATUS));
//		return -1;
//	}

    return 0;
}


int g1_ata_read_lba_dma(uint64_t sector, size_t count, uint16_t *buf,
                        int block) {
    int rv = 0;
    uint32_t addr;
    int /*old, */can_lba48 = CAN_USE_LBA48();
    int length = count * 512, used_secbuf = 0;

    /* Make sure we're actually being asked to do work... */
//    if(!count)
//        return 0;
//
//    if(!buf) {
//        return -1;
//    }

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Make sure the disk supports LBA mode. */
//    if(!device.max_lba) {
//        return -1;
//    }

    /* Make sure the disk supports Multi-Word DMA mode 2. */
//    if(!device.wdma_modes) {
//        return -1;
//    }

    /* Chaining isn't done yet, so make sure we don't need to. */
    if(count > 65536 || (!can_lba48 && count > 256)) {
//        return -1;
		rv = g1_ata_read_lba(sector, count, buf);
		dcache_flush_range((uint32_t)buf, length);
		return rv;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        return -1;
    }

    /* Check the alignment of the address. */
    addr = ((uint32_t)buf) & 0x0FFFFFFF;

    if(addr & 0x1F) {

		if(length > sector_buffer_size || !block) {
			LOGFF("Unaligned output address 0x%08lx (%ld), using PIO mode.\n", (uint32_t)buf, length);
			rv = g1_ata_read_lba(sector, count, buf);
			dcache_flush_range((uint32_t)buf, length);
			return rv;
		}
		
        LOGFF("Unaligned output address 0x%08lx (%ld), using sector buffer at 0x%08lx\n", 
					__func__, (uint32_t)buf, length, (uint32_t)sector_buffer);

        used_secbuf = 1;
        addr = ((uint32_t)sector_buffer) & 0x0FFFFFFF;
		
        /* Invalidate the dcache over the range of the data. */
        dcache_inval_range((uint32_t)sector_buffer, length);
		
    } else {
		/* Invalidate the dcache over the range of the data. */
		if(((uint32_t)buf) & 0xF0000000) {
			dcache_inval_range((uint32_t)buf, length);
		}
    }
	
    /* Disable IRQs temporarily... */
//    old = irq_disable();

    /* Make sure there is no DMA in progress already. */
    if(/*dma_in_progress || */g1_dma_in_progress()) {
        LOGFF("DMA in progress: %d %d\n", 
					__func__, dma_in_progress, g1_dma_in_progress());
        return -1;
    }
	
    /* Set the settings for this transfer and reenable IRQs. */
    dma_in_progress = 1;
//    irq_restore(old);

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* Which mode are we using: LBA28 or LBA48? */
    if(!can_lba48 || use_lba28(sector, count)) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));

        /* Write out the number of sectors we want and the lower 24-bits of
           the LBA we're looking for. Note that putting 0 into the sector count
           register returns 256 sectors. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
        rv = dma_common(ATA_CMD_READ_DMA, count, addr, G1_DMA_TO_MEMORY, block);
    }
    else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

        /* Write out the number of sectors we want and the LBA. Note that in
           LBA48 mode, putting 0 into the sector count register returns 65536
           sectors (not that we have that much RAM on the Dreamcast). */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(count >> 8));
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
        rv = dma_common(ATA_CMD_READ_DMA_EXT, count, addr, G1_DMA_TO_MEMORY,
                        block);
    }
	
    if(block && used_secbuf) {
		memcpy((uint8_t*)buf, sector_buffer, length);
		dcache_flush_range((uint32_t)buf, length);
    }

    return rv;
}


int g1_ata_read_lba_dma_part(uint64_t sector, size_t offset, size_t bytes, uint8_t *buf) {

    int rv = 0, count = 1;
    uint32_t addr;
    int /*old, */can_lba48 = CAN_USE_LBA48();

    /* Make sure the range of sectors is valid. */
//    if((sector + count) > device.max_lba) {
//        return -1;
//    }

    addr = ((uint32_t)buf) & 0x0FFFFFFF;
	
	if(offset && g1_dma_part_avail > 0) {

		g1_dma_part_avail -= bytes;
		g1_dma_start(addr, bytes);
		return 0;

	} else if(g1_dma_part_avail > 0) {
		
		// TODO: abort last command
		
	} else if(bytes < 512) {
		
		g1_dma_part_avail = 512 - bytes;
	}

    /* Disable IRQs temporarily... */
//    old = irq_disable();

    /* Make sure there is no DMA in progress already. */
    if(/*dma_in_progress || */g1_dma_in_progress()) {
        LOGFF("DMA in progress: %d %d\n", 
					__func__, dma_in_progress, g1_dma_in_progress());
        return -1;
    }
	
//	LOGFF("%d %d 0x%08lx\n", offset, bytes, (uint32_t)buf);
	
    /* Set the settings for this transfer and reenable IRQs. */
    dma_in_progress = 1;
//    irq_restore(old);

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* Which mode are we using: LBA28 or LBA48? */
    if(!can_lba48 || use_lba28(sector, count)) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));

        /* Write out the number of sectors we want and the lower 24-bits of
           the LBA we're looking for. Note that putting 0 into the sector count
           register returns 256 sectors. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
//        rv = dma_common_part(ATA_CMD_READ_DMA, offset, bytes, addr);
        _g1_dma_start(ATA_CMD_READ_DMA, bytes, addr, G1_DMA_TO_MEMORY);
		
    } else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

        /* Write out the number of sectors we want and the LBA. Note that in
           LBA48 mode, putting 0 into the sector count register returns 65536
           sectors (not that we have that much RAM on the Dreamcast). */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(count >> 8));
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
//        rv = dma_common_part(ATA_CMD_READ_DMA_EXT, offset, bytes, addr);
        _g1_dma_start(ATA_CMD_READ_DMA_EXT, bytes, addr, G1_DMA_TO_MEMORY);
    }

    return rv;
}

#if _FS_READONLY == 0

int g1_ata_write_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                     const uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        g1_ata_select_device(G1_ATA_SLAVE | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW,  (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h >= device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }

            dcache_pref_range((uint32)buf, 512);
			
            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
                OUT16(G1_ATA_DATA, *buf++);
            }
        }
    }


    /* Wait for the device to signal that it has finished writing the data. */
    g1_ata_wait_bsydrq();
	
    rv = 0;
	
    return rv;
}

int g1_ata_write_lba(uint64_t sector, size_t count, const uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
	
#ifdef USE_BYTE_ACCESS
    uint8_t *buff = (uint8_t*)buf;
    uint16_t data;
#endif

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Make sure the disk supports LBA mode. */
//    if(!device.max_lba) {
//        return -1;
//    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        if((sector + nsects) <= 0x0FFFFFFF) {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                                 ((sector >> 24) & 0x0F));

            /* Write out the number of sectors we want and the lower 24-bits of
               the LBA we're looking for. */
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);
        }
        else {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

            /* Write out the number of sectors we want and the LBA. */
            OUT8(G1_ATA_SECTOR_COUNT, 0);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS_EXT);
        }

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
			
            dcache_pref_range((uint32)buf, 512);
			
            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
#ifdef USE_BYTE_ACCESS
                data = buff[0] | buff[1] << 8;
                OUT16(G1_ATA_DATA, data);
                buff += 2;
#else
                OUT16(G1_ATA_DATA, *buf++);
#endif
            }
        }
    }

    /* Wait for the device to signal that it has finished writing the data. */
    g1_ata_wait_bsydrq();
	
    rv = 0;

    return rv;
}

#if 0

int g1_ata_write_lba_dma(uint64_t sector, size_t count, const uint16_t *buf,
                        int block) {
    int rv = 0;
    uint32_t addr;
    int can_lba48 = CAN_USE_LBA48();

//    /* Make sure we're actually being asked to do work... */
//    if(!count)
//        return 0;
//
//    if(!buf) {
//        return -1;
//    }
//
//    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }
//
//    /* Make sure the disk supports LBA mode. */
//    if(!device.max_lba) {
//        return -1;
//    }
//
//    /* Make sure the disk supports Multi-Word DMA mode 2. */
//    if(!device.wdma_modes) {
//        return -1;
//    }
//
//    /* Chaining isn't done yet, so make sure we don't need to. */
//    if(count > 65536 || (!can_lba48 && count > 256)) {
//        return -1;
//    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        return -1;
    }

    /* Check the alignment of the address. */
    addr = ((uint32_t)buf) & 0x0FFFFFFF;

    if(addr & 0x1F) {
        LOGFF("Unaligned output address - 0x%08lx\n", (uint32_t)buf);
        return -1;
    }

    /* Flush the dcache over the range of the data. */
    dcache_flush_range((uint32)buf, count * 512);

    /* Make sure there is no DMA in progress already. */
    if(g1_dma_in_progress()) {
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* For now, just assume we're accessing the slave device. We don't care
       about the primary device, since it should always be the GD-ROM drive. */
//    dsel = IN8(G1_ATA_DEVICE_SELECT);

    /* Which mode are we using: LBA28 or LBA48? */
    if(!can_lba48 || use_lba28(sector, count)) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));

        /* Write out the number of sectors we have and the lower 24-bits of
           the LBA we're looking for. Note that putting 0 into the sector count
           register writes 256 sectors. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
        rv = dma_common(ATA_CMD_WRITE_DMA, count, addr, G1_DMA_TO_DEVICE,
                        block);
    }
    else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);

        /* Write out the number of sectors we have and the LBA. Note that in
           LBA48 mode, putting 0 into the sector count register writes 65536
           sectors (not that we have that much RAM on the Dreamcast). */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(count >> 8));
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
        OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
        OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

        /* Do the rest of the work... */
        rv = dma_common(ATA_CMD_WRITE_DMA_EXT, count, addr, G1_DMA_TO_DEVICE,
                        block);
    }

    return rv;
}

#endif

int g1_ata_flush(void) {
    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }

    /* Select the slave device. */
    g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
    timer_spin_sleep(1);

    /* Flush the disk's write cache to make sure everything gets written out. */
    if(device.max_lba > 0x0FFFFFFF)
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE_EXT);
    else
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE);

    timer_spin_sleep(1);
    g1_ata_wait_bsydrq();

    return 0;
}

#endif

//int g1_ata_standby(void) {
//    uint8_t dsel;
//
//    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return -1;
//    }
//
//    /* Select the slave device. */
//    g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
//	
//    timer_spin_sleep(1);
//    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_STANDBY_5SU);
//    timer_spin_sleep(1);
//    g1_ata_wait_bsydrq();
//
//    return 0;
//}

#if _USE_MKFS && !_FS_READONLY
uint64_t g1_ata_max_lba(void) {

    /* Make sure that we've been initialized and there's a disk attached. */
//    if(!devices) {
//        return (uint64_t)-1;
//    }

    if(device.max_lba)
		return device.max_lba;
    else
		return (device.cylinders * device.heads * device.sectors);
}
#endif

static int g1_ata_set_transfer_mode(uint8_t mode) {
    uint8_t status;

    /* Fill in the registers as is required. */
    OUT8(G1_ATA_FEATURES, ATA_FEATURE_TRANSFER_MODE);
    OUT8(G1_ATA_SECTOR_COUNT, mode);
    OUT8(G1_ATA_CHS_SECTOR, 0);
    OUT8(G1_ATA_CHS_CYL_LOW, 0);
    OUT8(G1_ATA_CHS_CYL_HIGH, 0);

    /* Send the SET FEATURES command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_SET_FEATURES);
    timer_spin_sleep(1);

    /* Wait for command completion. */
    g1_ata_wait_nbsy();

    /* See if the command completed. */
    status = IN8(G1_ATA_STATUS_REG);
    if((status & G1_ATA_SR_ERR) || (status & G1_ATA_SR_DF)) {
        LOGFF("Error setting transfer mode %02x\n", mode);
        return -1;
    }

    return 0;
}


static int g1_ata_scan(void) {
    uint8_t dsel = IN8(G1_ATA_DEVICE_SELECT), st;
    int rv, i;
    uint16_t data[256];

    /* For now, just check if there's a slave device. We don't care about the
       primary device, since it should always be the GD-ROM drive. */
    OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
    timer_spin_sleep(1);

    OUT8(G1_ATA_SECTOR_COUNT, 0);
    OUT8(G1_ATA_LBA_LOW, 0);
    OUT8(G1_ATA_LBA_MID, 0);
    OUT8(G1_ATA_LBA_HIGH, 0);

    /* Send the IDENTIFY command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_IDENTIFY);
    timer_spin_sleep(1);
    st = IN8(G1_ATA_STATUS_REG);

    /* Check if there's anything on the bus. */
    if(!st || st == 0xFF) {
        rv = 0;
        goto out;
    }

    /* Wait for the device to finish. */
    g1_ata_wait_nbsy();

    /* Wait for data. */
    if(g1_ata_wait_drq()) {
        LOGFF("error while identifying device\n"
                           "             possibly ATAPI? %02x %02x\n",
               IN8(G1_ATA_LBA_MID), IN8(G1_ATA_LBA_HIGH));
        rv = 0;
        goto out;
    }

    /* Read out the data from the device. There will always be 256 words of
       data, according to the spec. */
    for(i = 0; i < 256; ++i)
        data[i] = IN16(G1_ATA_DATA);

    /* Read off some information we might need. */
    device.command_sets = (uint32_t)(data[82]) | ((uint32_t)(data[83]) << 16);
    device.capabilities = (uint32_t)(data[49]) | ((uint32_t)(data[50]) << 16);
    device.wdma_modes = data[63];

    /* See if we support LBA mode or not... */
    if(!(device.capabilities & (1 << 9))) {
        /* Nope. We have to use CHS addressing... >_< */
        device.max_lba = 0;
        device.cylinders = data[1];
        device.heads = data[3];
        device.sectors = data[6];
        LOGFF("found device with CHS: %d %d %d\n",
               device.cylinders, device.heads, device.sectors);
    }
    /* Do we support LBA48? */
    else if(!(device.command_sets & (1 << 26))) {
        /* Nope, use LBA28 */
        device.max_lba = (uint64_t)(data[60]) | ((uint64_t)(data[61]) << 16);
        device.cylinders = device.heads = device.sectors = 0;
        LOGFF("found device with LBA28: %ld\n",
               device.max_lba);
    }
    else {
        /* Yep, we support LBA48 */
        device.max_lba = (uint64_t)(data[100]) | ((uint64_t)(data[101]) << 16) |
            ((uint64_t)(data[102]) << 32) | ((uint64_t)(data[103]) << 48);
        device.cylinders = device.heads = device.sectors = 0;
        LOGFF("found device with LBA48: %ld\n", device.max_lba);
    }

    rv = 1;

    /* Set our transfer modes. */
    g1_ata_set_transfer_mode(ATA_TRANSFER_PIO_DEFAULT);
//    OUT32(G1_ATA_PIO_RACCESS_WAIT, G1_ACCESS_PIO_DEFAULT);
//    OUT32(G1_ATA_PIO_WACCESS_WAIT, G1_ACCESS_PIO_DEFAULT);

    /* Do we support Multiword DMA mode 2? If so, enable it. Otherwise, we won't
       even bother doing DMA at all. */
    if(device.wdma_modes & 0x0004) {
        if(!g1_ata_set_transfer_mode(ATA_TRANSFER_WDMA(2))) {
            OUT32(G1_ATA_DMA_RACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
            OUT32(G1_ATA_DMA_WACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
            /* Set DMA transfer range to the system memory area. */
            OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
        }
        else {
            device.wdma_modes = 0;
        }
    }
    else {
        device.wdma_modes = 0;
    }
	
	/* Disable external interrupts */
	OUT8(G1_ATA_CTL, 0x02);

out:
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

int g1_ata_init(void) {
	
    memset(&device, 0, sizeof(device));

    /* Scan for devices. */
    if((devices = g1_ata_scan()) < 1) {
        devices = 0;
        return -1;
    }

	if(!devices) {
        printf("%s: no adapter or device present\n");
        return -1;
    }

    return 0;
}

//void g1_ata_shutdown(void) {
//    /* Make sure to flush any cached data out. */
//    g1_ata_flush();
//
//    devices = 0;
//    initted = 0;
//
//    memset(&device, 0, sizeof(device));
//}


int g1_ata_read_blocks(uint32 block, size_t count, uint8 *buf, int wait_dma) {
	
	DBGFF("%ld %d 0x%08lx %s\n", block, count, (uint32)buf, fs_dma_enabled() ? "DMA" : "PIO");

//	if(device.max_lba) {
		if(fs_dma_enabled() && device.wdma_modes) {
			return g1_ata_read_lba_dma(block, count, (uint16_t *)buf, wait_dma);
		} else {
			return g1_ata_read_lba(block, count, (uint16_t *)buf);
		}
//	}
	
//	uint8_t h, s;
//	uint16_t c;
//		
//	c = (uint16_t)(block / (device.sectors * device.heads));
//	h = (uint8_t)((block / device.sectors) % device.heads);
//	s = (uint8_t)((block % device.sectors) + 1);
//
//	return g1_ata_read_chs(c, h, s, count, (uint16_t *)buf);
}

#if _FS_READONLY == 0

int g1_ata_write_blocks(uint32 block, size_t count, const uint8 *buf, int wait_dma) {
	
	(void)wait_dma;
//	if(device.max_lba) {
//		if(dma && device.wdma_modes) {
//			return g1_ata_write_lba_dma(block, count, (uint16_t *)buf, 1);
//		} else {
			return g1_ata_write_lba(block, count, (uint16_t *)buf);
//		}
//	}
	
//	uint8_t h, s;
//	uint16_t c;
//		
//	c = (uint16_t)(block / (device.sectors * device.heads));
//	h = (uint8_t)((block / device.sectors) % device.heads);
//	s = (uint8_t)((block % device.sectors) + 1);
//
//	return g1_ata_write_chs(c, h, s, count, (uint16_t *)buf);
}

#endif
