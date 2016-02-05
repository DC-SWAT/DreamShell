/**
 * Copyright (c) 2014 by SWAT <swat@211.ru>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <arch/types.h>
#include <arch/timer.h>
#include <arch/cache.h>
#include <drivers/spi.h>
#include <drivers/sd.h>
#include <dc/sd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* For CRC16-CCITT */
#include <kos/net.h>

#include <kos/blockdev.h>
#include <kos/dbglog.h>

//#define SD_DEBUG 1

#define MAX_RETRIES     500000
#define READ_RETRIES    50000
#define WRITE_RETRIES   150000
#define MAX_RETRY       10


/* MMC/SD command (in SPI) */
#define CMD(n) (n | 0x40)
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(0x40+41)	/* SEND_OP_COND (ACMD) */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */
#define CMD59	(0x40+59)	/* CRC_ON_OFF */


static inline uint16_t sdc_get_uint16( const uint8_t *s)
{
	return (uint16_t) ((s [0] << 8) | s [1]);
}

static inline uint32_t sdc_get_uint32( const uint8_t *s)
{
	return ((uint32_t) s [0] << 24) | ((uint32_t) s [1] << 16) | ((uint32_t) s [2] << 8) | (uint32_t) s [3];
}


/**
 * @name Card Identification
 * @{
 */

#define SD_MMC_CID_SIZE 16

#define SD_MMC_CID_GET_MID( cid) ((cid) [0])
#define SD_MMC_CID_GET_OID( cid) sdc_get_uint16( cid + 1)
#define SD_MMC_CID_GET_PNM( cid, i) ((char) (cid) [3 + (i)])
#define SD_MMC_CID_GET_PRV( cid) ((cid) [9])
#define SD_MMC_CID_GET_PSN( cid) sdc_get_uint32( cid + 10)
#define SD_MMC_CID_GET_MDT( cid) ((cid) [14])
#define SD_MMC_CID_GET_CRC7( cid) ((cid) [15] >> 1)

/** @} */


/* The type of the dev_data in the block device structure */
typedef struct sd_devdata {
    uint64_t block_count;
    uint64_t start_block;
} sd_devdata_t;


//static const int card_spi_delay = SPI_SDC_MMC_DELAY;
//static int old_spi_delay = SPI_SDC_MMC_DELAY;

static int byte_mode = 0;
static int is_mmc = 0;
static int initted = 0;

#define SELECT()   spi_cs_on(SPI_CS_SDC)
#define DESELECT() spi_cs_off(SPI_CS_SDC)
/*
#define SELECT() do {	\
	spi_cs_on(SPI_CS_SDC); \
	old_spi_delay = spi_get_delay(); \
	if(old_spi_delay != card_spi_delay) \
		spi_set_delay(card_spi_delay); \
} while(0)

	
#define DESELECT() do { \
	if(old_spi_delay != card_spi_delay) \
		spi_set_delay(old_spi_delay); \
	spi_cs_off(SPI_CS_SDC); \
} while(0)
*/

static uint8 wait_ready (void) {
	int i;
	uint8 res;

	(void)spi_sr_byte(0xFF);
	i = 0;
	do {
		res = spi_sr_byte(0xFF);
		i++;
	} while ((res != 0xFF) && i < MAX_RETRIES);
	
	return res;
}


static uint8 send_cmd (
	uint8 cmd,		/* Command byte */
	uint32 arg		/* Argument */
)
{
	uint8 n, res;
	uint8 cb[6];

	if (wait_ready() != 0xFF) {
#ifdef SD_DEBUG
		dbglog(DBG_DEBUG, "%s: CMD 0x%02x wait ready error\n", __func__, cmd);
#endif
		return 0xFF;
	}

	cb[0] = cmd;
	cb[1] = (uint8)(arg >> 24);
	cb[2] = (uint8)(arg >> 16);
	cb[3] = (uint8)(arg >> 8);
	cb[4] = (uint8)arg;
	cb[5] = sd_crc7(cb, 5, 0);
	/* Send command packet */

	int old = irq_disable();
	spi_send_byte(cmd);		/* Command */
	spi_send_byte(cb[1]);		/* Argument[31..24] */
	spi_send_byte(cb[2]);		/* Argument[23..16] */
	spi_send_byte(cb[3]);		/* Argument[15..8] */
	spi_send_byte(cb[4]);		/* Argument[7..0] */
	spi_send_byte(cb[5]);           // CRC7

	/* Receive command response */
	if (cmd == CMD12) 
		(void)spi_rec_byte();		/* Skip a stuff byte when stop reading */

	n = 20;						/* Wait for a valid response in timeout of 10 attempts */
	
	do {
		
		res = spi_rec_byte();
		
	} while ((res & 0x80) && --n);
	
#ifdef SD_DEBUG
	dbglog(DBG_DEBUG, "%s: CMD 0x%02x response 0x%02x\n", __func__, cmd, res);
#endif
	
	irq_restore(old);
	return res;			/* Return with the response value */
}


static uint8 send_slow_cmd (
	uint8 cmd,		/* Command byte */
	uint32 arg		/* Argument */
)
{
	uint8 n, res;
	uint8 cb[6];
	int i;

	(void)spi_slow_sr_byte(0xff);
	i = 0;
	
	do {
		
		res = spi_slow_sr_byte(0xff);
		i++;
		
	} while ((res != 0xFF) && i < 100000);
	
	if (res != 0xff) {
#ifdef SD_DEBUG
		dbglog(DBG_DEBUG, "%s: CMD 0x%02x error\n", __func__, cmd);
#endif
		return(0xff);
	}

	cb[0] = cmd;
	cb[1] = (uint8)(arg >> 24);
	cb[2] = (uint8)(arg >> 16);
	cb[3] = (uint8)(arg >> 8);
	cb[4] = (uint8)arg;
	cb[5] = sd_crc7(cb, 5, 0);
	/* Send command packet */
	spi_slow_sr_byte(cmd);		/* Command */
	spi_slow_sr_byte(cb[1]);		/* Argument[31..24] */
	spi_slow_sr_byte(cb[2]);		/* Argument[23..16] */
	spi_slow_sr_byte(cb[3]);		/* Argument[15..8] */
	spi_slow_sr_byte(cb[4]);		/* Argument[7..0] */
	spi_slow_sr_byte(cb[5]);		// CRC7

	/* Receive command response */
	if (cmd == CMD12) 
		(void)spi_slow_sr_byte(0xff);/* Skip a stuff byte when stop reading */
		
	n = 20; /* Wait for a valid response in timeout of 10 attempts */
	
	do {
		res = spi_slow_sr_byte(0xff);
	} while ((res & 0x80) && --n);
	
#ifdef SD_DEBUG
	dbglog(DBG_DEBUG, "%s: CMD 0x%02x response 0x%02x\n", __func__, cmd, res);
#endif
	
	return res; /* Return with the response value */
}


int sdc_init(void) {
	
	int i;
	uint8 n, ty = 0, ocr[4];
	
    if(initted)
        return 0;
	
	if(spi_init(0)) {
		return -1;
	}
	
	timer_spin_sleep(20);
	SELECT();
	
	/* 80 dummy clocks */
	for (n = 10; n; n--) 
		(void)spi_slow_sr_byte(0xff);

	
	if (send_slow_cmd(CMD0, 0) == 1) {			/* Enter Idle state */

#ifdef SD_DEBUG
		dbglog(DBG_DEBUG, "%s: Enter Idle state\n", __func__);
#endif
		timer_spin_sleep(20);
//		thd_sleep(100);
		
		i = 0;
		
		if (send_slow_cmd(CMD8, 0x1AA) == 1) {	/* SDC Ver2+  */
		
			for (n = 0; n < 4; n++) 
				ocr[n] = spi_slow_sr_byte(0xff);
			
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */
			
				do {
					
					/* ACMD41 with HCS bit */
					if (send_slow_cmd(CMD55, 0) <= 1 && send_slow_cmd(CMD41, 1UL << 30) == 0) 
						break;
						
					++i;
					
				} while (i < 300000);
				
				if (i < 300000 && send_slow_cmd(CMD58, 0) == 0) { /* Check CCS bit */
				
					for (n = 0; n < 4; n++) 
						ocr[n] = spi_slow_sr_byte(0xff);
						
					ty = (ocr[0] & 0x40) ? 6 : 2;
				}
			}
			
		} else { /* SDC Ver1 or MMC */
		
			ty = (send_slow_cmd(CMD55, 0) <= 1 && send_slow_cmd(CMD41, 0) <= 1) ? 2 : 1; /* SDC : MMC */
			
			do {
				
				if (ty == 2) {
					
					if (send_slow_cmd(CMD55, 0) <= 1 && send_slow_cmd(CMD41, 0) == 0) /* ACMD41 */
						break;
						
				} else {
					
					if (send_slow_cmd(CMD1, 0) == 0) { /* CMD1 */
						is_mmc = 1;
						break;
					}								
				}
				
				++i;
				
			} while (i < 300000);
			
			if (!(i < 300000) || send_slow_cmd(CMD16, 512) != 0)	/* Select R/W block length */
				ty = 0;
		}
	}
	
	send_slow_cmd(CMD59, 1);		// crc check
	
#ifdef SD_DEBUG
	dbglog(DBG_DEBUG, "%s: card type = 0x%02x\n", __func__, ty & 0xff);
#endif

	if(!(ty & 4)) {
		byte_mode = 1;
	}
	
	DESELECT();
	(void)spi_slow_sr_byte(0xff); /* Idle (Release DO) */

	if (ty) { /* Initialization succeded */
//		sdc_print_ident();
		initted = 1;
		return 0;
	}
	
	/* Initialization failed */
	sdc_shutdown();
	return -1;
}


int sdc_shutdown(void) {
	
	if(!initted)
        return -1;
	
	SELECT();
	wait_ready();
	DESELECT();
	(void)spi_rec_byte();
	
	spi_shutdown();
	initted = 0;
	return 0;
}


static int read_data (
	uint8 *buff,		/* Data buffer to store received data */
	size_t len			/* Byte count (must be even number) */
)
{
	uint8 token;
	int i, old;
	
	uint16 crc, crc2;
	i = 0;
	
	do {							/* Wait for data packet in timeout of 100ms */
		token = spi_rec_byte();
		++i;
	} while ((token == 0xFF) && i < READ_RETRIES);

	if(token != 0xFE) {
#ifdef SD_DEBUG
		dbglog(DBG_DEBUG, "%s: not valid data token: %02x\n", __func__, token);
#endif
		return -1;	/* If not valid data token, return with error */
	}
	
//	dcache_alloc_range((uint32)buff, len);
	
	old = irq_disable();
	
	spi_rec_data(buff, len);
	crc = (uint16)spi_rec_byte() << 8;
	crc |= (uint16)spi_rec_byte();
	
	irq_restore(old);
	
	crc2 = net_crc16ccitt(buff, len, 0);
	
//	dcache_purge_range((uint32)buff, len);
	
	if(crc != crc2) {
		errno = EIO;
		return -1;
	}
	
	return 0;					/* Return with success */
}


int sdc_read_blocks(uint32 block, size_t count, uint8 *buf) {
	
	if(!initted) {
		errno = ENXIO;
		return -1;
	}
	
#ifdef SD_DEBUG
	dbglog(DBG_DEBUG, "%s: block=%ld count=%d\n", __func__, block, count);
#endif

	uint8 *p;
	int retry, cnt;
	
	if (byte_mode) block <<= 9;	/* Convert to byte address if needed */

	for (retry = 0; retry < MAX_RETRY; retry++) {
		p = buf;
		cnt = count;
		
		SELECT();

		if (cnt == 1) { /* Single block read */
		
			if ((send_cmd(CMD17, block) == 0) && !read_data(p, 512)) {
				cnt = 0;
			}
			
		} else { /* Multiple block read */
			if (send_cmd(CMD18, block) == 0) {	
				
				do {
					
					if (read_data(p, 512)) 
						break;
						
					p += 512;
					
				} while (--cnt);
				
				send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
			}
		}

		DESELECT();
		(void)spi_rec_byte();			/* Idle (Release DO) */
		if (cnt == 0) break;
	}
	
//#ifdef SD_DEBUG
//	dbglog(DBG_DEBUG, "%s: retry = %d (MAX=%d) cnt = %d\n", __func__, retry, MAX_RETRY, cnt);
//#endif

	if((retry >= MAX_RETRY || cnt > 0)) {
		errno = EIO;
		return -1;
	}

	return 0;
}


static int write_data (
	uint8 *buff,	/* 512 byte data block to be transmitted */
	uint8 token	/* Data/Stop token */
)
{
	uint8 resp;
	uint16 crc;
	int old;

	if (wait_ready() != 0xFF) 
		return -1;

	spi_send_byte(token);	 /* Xmit data token */
	
	if (token != 0xFD) {	/* Is data token */
	
		dcache_pref_range((uint32)buff, 512);
		crc = net_crc16ccitt(buff, 512, 0);
		
		old = irq_disable();
		
		spi_send_data(buff, 512);
		spi_send_byte((uint8)(crc >> 8));
		spi_send_byte((uint8)crc);
		
		irq_restore(old);
		
		resp = spi_rec_byte();				/* Reсeive data response */
		
		if ((resp & 0x1F) != 0x05) {		/* If not accepted, return with error */
#ifdef SD_DEBUG
			dbglog(DBG_DEBUG, "%s: not accepted: %02x\n", __func__, resp);
#endif
			errno = EIO;
			return -1;
		}
	}
	
	return 0;
}


int sdc_write_blocks(uint32 block, size_t count, const uint8 *buf) {
	
	if(!initted) {
		errno = ENXIO;
		return -1;
	}
	
#ifdef SD_DEBUG
	dbglog(DBG_DEBUG, "%s: block=%ld count=%d\n", __func__, block, count);
#endif
	
	uint8 cnt, *p;
	int retry;

	if (byte_mode) block <<= 9;	/* Convert to byte address if needed */

	for (retry = 0; retry < MAX_RETRY; retry++) {
		
		p = (uint8 *)buf;
		cnt = count;
		
		SELECT();			/* CS = L */

		if (count == 1) {	/* Single block write */
			if ((send_cmd(CMD24, block) == 0) && !write_data(p, 0xFE))
				cnt = 0;
		}
		else {	/* Multiple block write */
			if (!is_mmc) {
				send_cmd(CMD55, 0); 
				send_cmd(CMD23, cnt);	/* ACMD23 */
			}
			if (send_cmd(CMD25, block) == 0) {	
				
				do {

					if (write_data(p, 0xFC)) 
						break;
						
					p += 512;
					
				} while (--cnt);
				
				if (write_data(0, 0xFD))	/* STOP_TRAN token */
					cnt = 1;
			}
		}

		DESELECT();			/* CS = H */
		(void)spi_rec_byte();			/* Idle (Release DO) */
		if (cnt == 0) break;
	}
	
//#ifdef SD_DEBUG
//	dbglog(DBG_DEBUG, "%s: retry = %d (MAX=%d) cnt = %d\n", __func__, retry, MAX_RETRY, cnt);
//#endif
	
	if((retry >= MAX_RETRY || cnt > 0)) {
		errno = EIO;
		return -1;
	}
	
	return 0;
}


uint64 sdc_get_size(void) {
	
	uint8 csd[16];
	int exponent;
	uint64 rv = 0;
	
	if(!initted) {
		errno = ENXIO;
		return (uint64)-1;
	}
	
	SELECT();
	
	if(send_cmd(CMD9, 0)) {
		rv = (uint64)-1;
		errno = EIO;
		goto out;
	}

	/* Read back the register */
	if(read_data(csd, 16)) {
		rv = (uint64)-1;
		errno = EIO;
		goto out;
	}

	/* Figure out what version of the CSD register we're looking at */
	switch(csd[0] >> 6) {
		case 0:
			/* CSD version 1.0 (SD)
			   C_SIZE is bits 62-73 of the CSD, C_SIZE_MULT is bits 47-49,
			   READ_BL_LEN is bits 80-83.
			   Card size is calculated as follows:
			   (C_SIZE + 1) * 2^(C_SIZE_MULT + 2) * 2^(READ_BL_LEN) */
			exponent = (csd[5] & 0x0F) + ((csd[9] & 0x03) << 1) +
				(csd[10] >> 7) + 2;
			rv = ((csd[8] >> 6) | (csd[7] << 2) | ((csd[6] & 0x03) << 10)) + 1;
			rv <<= exponent;
			break;

		case 1:
			/* CSD version 2.0 (SDHC/SDXC)
			   C_SIZE is bits 48-69 of the CSD, card size is calculated as
			   (C_SIZE + 1) * 512KiB */
			rv = ((((uint64)csd[9]) | (uint64)(csd[8] << 8) |
				   ((uint64)(csd[7] & 0x3F) << 16)) + 1) << 19;
			break;

		default:
			/* Unknown version, punt. */
			rv = (uint64)-1;
			errno = ENODEV;
			goto out;
	}

out:
	DESELECT();
	(void)spi_rec_byte();	
    return rv;
}


int sdc_print_ident(void) {
	
	uint8 cid[SD_MMC_CID_SIZE];
	int rv = 0;
	
	if(!initted) {
		errno = ENXIO;
		return -1;
	}
	
	SELECT();
	
	if(send_cmd(CMD10, 0)) {
		rv = -1;
		errno = EIO;
		goto out;
	}
	
	memset(cid, 0, SD_MMC_CID_SIZE);

	/* Read back the register */
	if(read_data(cid, SD_MMC_CID_SIZE)) {
		rv = -1;
		errno = EIO;
		goto out;
	}

//	dbglog(DBG_DEBUG, "*** Card Identification ***\n");
	dbglog(DBG_DEBUG, "Manufacturer ID          : %" PRIu8 "\n",  SD_MMC_CID_GET_MID(cid));
	dbglog(DBG_DEBUG, "OEM/Application ID       : %" PRIu16 "\n", SD_MMC_CID_GET_OID(cid));
	dbglog(DBG_DEBUG,
	"Product name             : %c%c%c%c%c%c\n",
		SD_MMC_CID_GET_PNM(cid, 0),
		SD_MMC_CID_GET_PNM(cid, 1),
		SD_MMC_CID_GET_PNM(cid, 2),
		SD_MMC_CID_GET_PNM(cid, 3),
		SD_MMC_CID_GET_PNM(cid, 4),
		SD_MMC_CID_GET_PNM(cid, 5)
	);
	dbglog(DBG_DEBUG, "Product revision         : %" PRIu8 "\n",  SD_MMC_CID_GET_PRV(cid));
	dbglog(DBG_DEBUG, "Product serial number    : %" PRIu32 "\n", SD_MMC_CID_GET_PSN(cid));
	dbglog(DBG_DEBUG, "Manufacturing date       : %" PRIu8 "\n",  SD_MMC_CID_GET_MDT(cid));
	dbglog(DBG_DEBUG, "7-bit CRC checksum       : %" PRIu8 "\n",  SD_MMC_CID_GET_CRC7(cid));


out:
	DESELECT();
	(void)spi_rec_byte();	
    return rv;
}


static int sdb_init(kos_blockdev_t *d) {
    (void)d;

    if(!initted) {
        errno = ENODEV;
        return -1;
    }

    return 0;
}

static int sdb_shutdown(kos_blockdev_t *d) {
    free(d->dev_data);
    return 0;
}

static int sdb_read_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                           void *buf) {
    sd_devdata_t *data = (sd_devdata_t *)d->dev_data;

    return sdc_read_blocks(block + data->start_block, count, (uint8 *)buf);
}

static int sdb_write_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                            const void *buf) {
    sd_devdata_t *data = (sd_devdata_t *)d->dev_data;

    return sdc_write_blocks(block + data->start_block, count,
                           (const uint8 *)buf);
}

static uint64_t sdb_count_blocks(kos_blockdev_t *d) {
    sd_devdata_t *data = (sd_devdata_t *)d->dev_data;

    return data->block_count;
}

static int sdb_flush(kos_blockdev_t *d) {
    (void)d;
	
	SELECT();
	wait_ready();
	DESELECT();
	(void)spi_rec_byte();
	
    return 0;
}

static kos_blockdev_t sd_blockdev = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &sdb_init,              /* init */
    &sdb_shutdown,          /* shutdown */
    &sdb_read_blocks,       /* read_blocks */
    &sdb_write_blocks,      /* write_blocks */
    &sdb_count_blocks,      /* count_blocks */
    &sdb_flush              /* flush */
};

int sdc_blockdev_for_partition(int partition, kos_blockdev_t *rv,
                              uint8 *partition_type) {
    uint8 buf[512];
    int pval;
    sd_devdata_t *ddata;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    if(!rv || !partition_type) {
        errno = EFAULT;
        return -1;
    }

    /* Make sure the partition asked for is sane */
    if(partition < 0 || partition > 3) {
        dbglog(DBG_DEBUG, "Invalid partition number given: %d\n", partition);
        errno = EINVAL;
        return -1;
    }

    /* Read the MBR from the card */
    if(sdc_read_blocks(0, 1, buf)) {
        return -1;
    }

    /* Make sure the SD card uses MBR partitions.
       TODO: Support GPT partitioning at some point. */
    if(buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
        dbglog(DBG_DEBUG, "SD card doesn't appear to have a MBR\n");
        errno = ENOENT;
        return -1;
    }

    /* Figure out where the partition record we're concerned with is, and make
       sure that the partition actually exists. */
    pval = 16 * partition + 0x01BE;

    if(buf[pval + 4] == 0) {
        dbglog(DBG_DEBUG, "Partition %d appears to be empty\n", partition);
        errno = ENOENT;
        return -1;
    }

    /* Allocate the device data */
    if(!(ddata = (sd_devdata_t *)malloc(sizeof(sd_devdata_t)))) {
        errno = ENOMEM;
        return -1;
    }

    /* Copy in the template block device and fill it in */
    memcpy(rv, &sd_blockdev, sizeof(kos_blockdev_t));
    ddata->block_count = buf[pval + 0x0C] | (buf[pval + 0x0D] << 8) |
        (buf[pval + 0x0E] << 16) | (buf[pval + 0x0F] << 24);
    ddata->start_block = buf[pval + 0x08] | (buf[pval + 0x09] << 8) |
        (buf[pval + 0x0A] << 16) | (buf[pval + 0x0B] << 24);
    rv->dev_data = ddata;
    *partition_type = buf[pval + 4];

    return 0;
}

