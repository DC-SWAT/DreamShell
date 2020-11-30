/**
 * Copyright (c) 2014-2020 SWAT <http://www.dc-swat.ru>
 * Copyright (c) 2017 Megavolt85
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __IDE_H
#define __IDE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/cdrom.h>

typedef unsigned char u8; 
typedef unsigned short u16; 
typedef unsigned int u32; 
typedef unsigned long long u64;
typedef char s8;
typedef short s16; 
typedef int s32; 
typedef long long s64;

// Directions:
#define G1_READ_PIO		0
#define G1_WRITE_PIO	1
#define G1_READ_DMA		2
#define G1_WRITE_DMA	3

typedef struct pt_struct 
{ 
	u8 bootable; 
	u8 start_part[3]; 
	u8 type_part; 
	u8 end_part[3]; 
	u32 sect_before; 
	u32 sect_total; 
} pt_t;

typedef struct cdrom_info
{ 
	CDROM_TOC tocs[2];
	u8 disc_type;
	u8 sec_type;
	u8 inited;
} cdri_t;

typedef struct cd_sense
{
	u8 sk;
	u8 asc;
	u8 ascq;
} cd_sense_t;

typedef struct ide_device
{
	u8  reserved;		// 0 (Empty) or 1 (This Drive really exists).
	u8  drive;			// 0 (Master Drive) or 1 (Slave Drive).
	u8  type;			// 0: ATA, 1: ATAPI, 2: SPI (Sega Packet Interface)
	u16 sign;			// Drive Signature
	u16 capabilities;	// Features.
	u32 command_sets;	// Command Sets Supported.
//	s8  model[41];		// Model in string.
	u64 max_lba;
    u16 wdma_modes;
#if defined(DEV_TYPE_IDE) && defined(DEBUG)
    pt_t pt[4];
    u8 pt_num;
#endif
#ifdef DEV_TYPE_GD
    cdri_t cd_info;
#endif
} ide_device_t;


s32 g1_bus_init(void);

void g1_dma_set_irq_mask(s32 last_transfer);
s32 g1_dma_has_irq_mask();
s32 g1_dma_init_irq(void);
s32 g1_dma_irq_enabled(void);
void g1_dma_irq_hide(s32 all);
void g1_dma_irq_restore(void);

void g1_dma_start(u32 addr, size_t bytes);
void g1_dma_abort(void);
s32 g1_dma_in_progress(void);
u32 g1_dma_transfered(void);

void cdrom_spin_down(u8 drive);
s32 cdrom_get_status(s32 *status, u8 *disc_type, u8 drive);
s32 cdrom_read_toc(CDROM_TOC *toc_buffer, u8 session, u8 drive);
CDROM_TOC *cdrom_get_toc(u8 session, u8 drive);
u32 cdrom_locate_data_track(CDROM_TOC *toc);

void cdrom_set_sector_size(s32 sec_size, u8 drive);
s32 cdrom_reinit(s32 sec_size, u8 drive);
s32 cdrom_cdda_play(u32 start, u32 end, u32 loops, s32 mode);
s32 cdrom_cdda_pause();
s32 cdrom_cdda_resume();
s32 cdrom_read_sectors(void *buffer, u32 sector, u32 cnt, u8 drive);
s32 cdrom_read_sectors_ex(void *buffer, u32 sector, u32 cnt, u8 async, u8 dma, u8 drive);
s32 cdrom_read_sectors_part(void *buffer, u32 sector, size_t offset, size_t bytes, u8 drive);
s32 cdrom_chk_disc_change(u8 drive);
u8 cdrom_get_dev_type(u8 drive);

/** \brief  DMA read disk sector part with Linear Block Addressing (LBA).

    This function reads partial disk block from the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).

    \param  sector          The sector reading.
    \param  offset          The number of bytes to skip.
    \param  bytes           The number of bytes to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least 32-byte aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.
*/
s32 g1_ata_read_lba_dma_part(u64 sector, size_t offset, size_t bytes, u8 *buf);


/** \brief  Pre-read disk sector part with Linear Block Addressing (LBA).
 * 
 * This function for pre-reading sectors from the slave device.
 **/
s32 g1_ata_pre_read_lba(u64 sector, size_t count);


/** \brief  Read one or more blocks from the ATA device.

    This function reads the specified number of blocks from the ATA device from the
    beginning block specified into the buffer passed in. It is your
    responsibility to allocate the buffer properly for the number of bytes that
    is to be read (512 * the number of blocks requested).

    \param  block           The starting block number to read from.
    \param  count           The number of 512 byte blocks of data to read.
    \param  buf             The buffer to read into.
    \param  wait_dma        Wait DMA complete
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.
*/
s32 g1_ata_read_blocks(u64 block, size_t count, u8 *buf, u8 wait_dma);


/** \brief  Write one or more blocks to the ATA device.

    This function writes the specified number of blocks to the ATA device at the
    beginning block specified from the buffer passed in. Each block is 512 bytes
    in length, and you must write at least one block at a time. You cannot write
    partial blocks.

    If this function returns an error, you have quite possibly corrupted
    something on the ATA device or have a damaged device in general (unless errno is
    ENXIO).

    \param  block           The starting block number to write to.
    \param  count           The number of 512 byte blocks of data to write.
    \param  buf             The buffer to write from.
    \param  wait_dma        Wait DMA complete
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.
*/
s32 g1_ata_write_blocks(u64 block, size_t count, const u8 *buf, u8 wait_dma);


/** \brief  Polling the ATA device.

    This function used for async reading.

    \return                 On succes, the transfered size. On error, -1.
*/
s32 g1_ata_poll(void);


/** \brief  Retrieve the size of the ATA device.

    \return                 On succes, max LBA
                            error, (uint64)-1.
*/
u64 g1_ata_max_lba(void);


/** \brief  Flush the write cache on the attached disk.

    This function flushes the write cache on the disk attached as the slave
    device on the G1 ATA bus. This ensures that all writes that have previously
    completed are fully persisted to the disk. You should do this before
    unmounting any disks or exiting your program if you have called any of the
    write functions in here.

    \return                 0 on success. <0 on error, setting errno as
                            appropriate.
*/
s32 g1_ata_flush(void);


/** \brief  Abort the ATA device.
 * 
 */
s32 g1_ata_abort(void);

__END_DECLS

#endif  /* __IDE_H */
