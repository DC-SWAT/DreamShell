/**
 * Copyright (c) 2017 Megavolt85
 *
 * Modified for ISO Loader by SWAT <http://www.dc-swat.ru>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

typedef unsigned char u8; 
typedef unsigned short u16; 
typedef unsigned int u32; 
typedef unsigned long long u64;
typedef char s8;
typedef short s16; 
typedef int s32; 
typedef long long s64;

// Directions:
#define G1_READ_PIO				0
#define G1_WRITE_PIO			1
#define G1_READ_DMA				2
#define G1_WRITE_DMA			3

/** \defgroup cd_cmd_response       CD-ROM command responses

    These are the values that the various functions can return as error codes.
    @{
*/
#define ERR_OK          0   /**< \brief No error */
#define ERR_NO_DISC     1   /**< \brief No disc in drive */
#define ERR_DISC_CHG    2   /**< \brief Disc changed, but not reinitted yet */
#define ERR_SYS         3   /**< \brief System error */
#define ERR_ABORTED     4   /**< \brief Command aborted */
#define ERR_NO_ACTIVE   5   /**< \brief System inactive? */
/** @} */

/** \defgroup cd_status_values      CD-ROM status values

    These are the values that can be returned as the status parameter from the
    cdrom_get_status() function.
    @{
*/
#define CD_STATUS_BUSY      0   /**< \brief Drive is busy */
#define CD_STATUS_PAUSED    1   /**< \brief Disc is paused */
#define CD_STATUS_STANDBY   2   /**< \brief Drive is in standby */
#define CD_STATUS_PLAYING   3   /**< \brief Drive is currently playing */
#define CD_STATUS_SEEKING   4   /**< \brief Drive is currently seeking */
#define CD_STATUS_SCANNING  5   /**< \brief Drive is scanning */
#define CD_STATUS_OPEN      6   /**< \brief Disc tray is open */
#define CD_STATUS_NO_DISC   7   /**< \brief No disc inserted */
/** @} */

/** \defgroup cd_disc_types         CD-ROM drive disc types

    These are the values that can be returned as the disc_type parameter from
    the cdrom_get_status() function.
    @{
*/
#define CD_CDDA     0       /**< \brief Audio CD (Red book) */
#define CD_CDROM    0x10    /**< \brief CD-ROM or CD-R (Yellow book) */
#define CD_CDROM_XA 0x20    /**< \brief CD-ROM XA (Yellow book extension) */
#define CD_CDI      0x30    /**< \brief CD-i (Green book) */
#define CD_GDROM    0x80    /**< \brief GD-ROM */
/** @} */

//typedef struct 
//{
//    uint32  entry[99];          /**< \brief TOC space for 99 tracks */
//    uint32  first;              /**< \brief Point A0 information (1st track) */
//    uint32  last;               /**< \brief Point A1 information (last track) */
//    uint32  leadout_sector;     /**< \brief Point A2 information (leadout) */
//} CDROM_TOC;

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
	u8  type;			// 0: ATA, 1: ATAPI, 2: SPI.
	u16 sign;			// Drive Signature
	u16 capabilities;	// Features.
	u32 command_sets;	// Command Sets Supported.
//	s8  model[41];		// Model in string.
	u64 max_lba;
    u16 cylinders;
    u16 heads;
    u16 sectors;
    u16 wdma_modes;
#ifdef DEV_TYPE_IDE
    pt_t pt[4];
    u8 pt_num;
#endif
#ifdef DEV_TYPE_GD
    cdri_t cd_info;
#endif
} ide_device_t;

/** \defgroup cd_toc_access         CD-ROM TOC access macros
    @{
*/
/** \brief  Get the FAD address of a TOC entry.
    \param  n               The actual entry from the TOC to look at.
    \return                 The FAD of the entry.
*/
#define TOC_LBA(n) ((n) & 0x00ffffff)

/** \brief  Get the address of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's address.
*/
#define TOC_ADR(n) ( ((n) & 0x0f000000) >> 24 )

/** \brief  Get the control data of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's control value.
*/
#define TOC_CTRL(n) ( ((n) & 0xf0000000) >> 28 )

/** \brief  Get the track number of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's track.
*/
#define TOC_TRACK(n) ( ((n) & 0x00ff0000) >> 16 )
/** @} */

void g1_bus_init(void);
void g1_dma_abort(void);
void g1_dma_set_irq_mask(s32 enable);
s32 g1_dma_init_irq(void);
s32 g1_dma_irq_enabled(void);
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
s32 cdrom_chk_disc_change(u8 drive);
