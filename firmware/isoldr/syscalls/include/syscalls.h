
/**
 * This file is part of DreamShell ISO Loader
 * Copyright (C)2019 megavolt85
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __uint32_t_defined
typedef unsigned int		uint32_t;
# define __uint32_t_defined
#endif

#ifndef __uint16_t_defined
typedef unsigned short		uint16_t;
# define __uint16_t_defined
#endif

#ifndef __uint8_t_defined
typedef unsigned char		uint8_t;
# define __uint8_t_defined
#endif

/* Macros to access the ATA registers */
#define OUT32(addr, data) *((volatile uint32_t *)addr) = data
#define OUT16(addr, data) *((volatile uint16_t *)addr) = data
#define OUT8(addr, data)  *((volatile uint8_t  *)addr) = data
#define IN32(addr)        *((volatile uint32_t *)addr)
#define IN16(addr)        *((volatile uint16_t *)addr)
#define IN8(addr)         *((volatile uint8_t  *)addr)

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
#define G1_ATA_DMA_ADDRESS      0xA05F7404      /* Read/Write */
#define G1_ATA_DMA_LENGTH       0xA05F7408      /* Read/Write */
#define G1_ATA_DMA_DIRECTION    0xA05F740C      /* Read/Write */
#define G1_ATA_DMA_ENABLE       0xA05F7414      /* Read/Write */
#define G1_ATA_DMA_STATUS       0xA05F7418      /* Read/Write */
#define G1_ATA_DMA_STARD        0xA05F74F4      /* Read-only */
#define G1_ATA_DMA_LEND         0xA05F74F8      /* Read-only */
#define G1_ATA_DMA_PRO          0xA05F74B8      /* Write-only */
#define G1_ATA_DMA_PRO_SYSMEM   0x8843407F

#define ASIC_BASE         (0xa05f6900)
#define ASIC_IRQ_STATUS   ((volatile uint32_t *) (ASIC_BASE + 0x00))
#define ASIC_IRQ13_MASK   ((volatile uint32_t *) (ASIC_BASE + 0x10))
#define ASIC_IRQ11_MASK   ((volatile uint32_t *) (ASIC_BASE + 0x20))
#define ASIC_IRQ9_MASK    ((volatile uint32_t *) (ASIC_BASE + 0x30))

#define ASIC_MASK_NRM_INT 0
#define ASIC_MASK_EXT_INT 1
#define ASIC_MASK_ERR_INT 2

/* status of the external interrupts
 * bit 3 = External Device interrupt
 * bit 2 = Modem interrupt
 * bit 1 = AICA interrupt
 * bit 0 = GD-ROM interrupt */
#define EXT_INT_STAT			0xA05F6904      /* Read */

/* status of the nonmal interrupts */
#define NORMAL_INT_STAT			0xA05F6900      /* Read/Write */

/* Bitmasks for the STATUS_REG/ALT_STATUS registers. */
#define G1_ATA_SR_ERR   		0x01
#define G1_ATA_SR_IDX   		0x02
#define G1_ATA_SR_CORR  		0x04
#define G1_ATA_SR_DRQ   		0x08
#define G1_ATA_SR_DSC   		0x10
#define G1_ATA_SR_DF    		0x20
#define G1_ATA_SR_DRDY  		0x40
#define G1_ATA_SR_BSY   		0x80

/* Bitmasks for the G1_ATA_ERROR registers. */
#define G1_ATA_ER_BBK			0x80
#define G1_ATA_ER_UNC			0x40
#define G1_ATA_ER_MC			0x20
#define G1_ATA_ER_IDNF			0x10
#define G1_ATA_ER_MCR			0x08
#define G1_ATA_ER_ABRT			0x04
#define G1_ATA_ER_TK0NF			0x02
#define G1_ATA_ER_AMNF			0x01

/* ATA Commands we might like to send. */
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_READ_SECTORS_EXT    0x24
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_READ_DMA            0xC8

#define g1_ata_wait_status(n) \
    do {} while((IN8(G1_ATA_ALTSTATUS) & (n)))

/* Values for CMD_GETSCD command */
#define SCD_REQ_ALL_SUBCODE      0x0
#define SCD_REQ_Q_SUBCODE        0x1
#define SCD_REQ_MEDIA_CATALOG    0x2
#define SCD_REQ_ISRC             0x3
#define SCD_REQ_RESERVED         0x4

#define SCD_AUDIO_STATUS_INVALID 0x00
#define SCD_AUDIO_STATUS_PLAYING 0x11
#define SCD_AUDIO_STATUS_PAUSED  0x12
#define SCD_AUDIO_STATUS_ENDED   0x13
#define SCD_AUDIO_STATUS_ERROR   0x14
#define SCD_AUDIO_STATUS_NO_INFO 0x15

#define SCD_DATA_SIZE_INDEX      3

typedef enum 
{
	CMD_STAT_FAILED		=-1,
	CMD_STAT_NO_ACTIVE	= 0,
	CMD_STAT_PROCESSING	= 1,
	CMD_STAT_COMPLETED	= 2,
	CMD_STAT_ABORTED	= 3,
	CMD_STAT_WAITING	= 4,
	CMD_STAT_ERROR		= 5
} gd_cmd_stat_t;

typedef enum 
{
	GDCMD_OK			= 0,
	GDCMD_HW_ERR		= 2,
	GDCMD_INVALID_CMD	= 5,
	GDCMD_NOT_INITED	= 6,
	GDCMD_GDSYS_LOCKED	= 32,
	
} gd_cmd_err_t;

/* GD status */
typedef enum 
{
	STAT_BUSY		= 0,
	STAT_PAUSE		= 1,
	STAT_STANDBY	= 2,
	STAT_PLAY		= 3,
	STAT_SEEK		= 4,
	STAT_SCAN		= 5,
	STAT_OPEN		= 6,
	STAT_NODISK		= 7,
	STAT_RETRY		= 8,
	STAT_ERROR		= 9
} gd_drv_stat_t;

/* Different media types */
typedef enum 
{
	TYPE_CDDA		= 0x00,
	TYPE_CDROM		= 0x10,
	TYPE_CDROMXA	= 0x20,
	TYPE_CDI		= 0x30,
	TYPE_GDROM		= 0x80
} gd_media_t;

typedef struct 
{
	uint32_t entry[99];
    uint32_t first;
    uint32_t last;
    uint32_t leadout_sector;
} toc_t;

typedef struct
{
	int gd_cmd;
	int gd_cmd_stat;
	int gd_cmd_err;
	int gd_cmd_err2;
	uint32_t param[4];
	void *gd_hw_base;
	uint32_t transfered;
	int ata_status;
	int drv_stat;
	int drv_media;
	int cmd_abort;
	uint32_t requested;
	int gd_chn;
	int dma_in_progress;
	int need_reinit;
	void *callback;
	int callback_param;
	short *pioaddr;
	int piosize;
	char cmdp[28];
	toc_t TOC;
	uint32_t dtrkLBA[3];
	uint32_t dsLBA;
	uint32_t dsseccnt;
	uint32_t currentLBA;
} GDS;

extern short disc_type;
extern uint32_t disc_id[5];
extern uint32_t gd_vector2;
extern uint8_t  display_cable;

extern int gd_gdrom_syscall(int, uint32_t*, int, int);
extern void Exit_to_game(void);
extern int allocate_GD(void);
extern void release_GD(void);
extern GDS *get_GDS(void);
extern int lock_gdsys(int lock);
extern void gd_do_cmd(uint32_t *param, GDS *my_gds, int cmd);

extern uint32_t irq_disable();
extern void irq_restore(uint32_t old);

extern void flush_cache(void);
extern int flashrom_lock(void);
extern void flashrom_unlock(void);

#ifdef LOG
int scif_init();
int WriteLog(const char *fmt, ...);
int WriteLogFunc(const char *func, const char *fmt, ...);

#define LOGF(...) WriteLog(__VA_ARGS__)
#define LOGFF(...) WriteLogFunc(__func__, __VA_ARGS__)
#endif

