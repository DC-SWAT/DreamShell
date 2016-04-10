/** 
 * \file    bflash.h
 * \brief   Bios flash chip driver
 * \date    2009-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef __BIOS_FLASH_H
#define __BIOS_FLASH_H

#include <arch/types.h>

/** 
 * Currently supported flash chips: 
 * 
	AMD
		Am29LV800T      1024 KB 3V
		Am29LV800B      1024 KB 3V
		Am29LV160DT     2048 KB 3V
		Am29LV160DB     2048 KB 3V
		Am29F160DT      2048 KB 5V
		Am29F160DB      2048 KB 5V
	
	STMicroelectronics
		M29W800T		1024 KB	3V
		M29W800B		1024 KB	3V
		M29W160BT		2048 KB	3V
		M29W160BB		2048 KB	3V
	
	Macronix
		MX29F400		512  KB	5V
		MX29F1610		2048 KB	5V
		MX29F016		2048 KB	5V
		MX29LV160T		2048 KB	3V
		MX29LV160B		2048 KB	3V
		MX29LV320T		4096 KB	3V
		MX29LV320B		4096 KB	3V
		MX29L3211		4096 KB	3V
	
	AMIC
		A29L160AT		2048 KB	3V
		A29L160AB		2048 KB	3V
	
	ESMT
		F49L160UA		2048 KB	3V
		F49L160BA		2048 KB	3V
	
	Sega
		MPR-XXXXX		2048 KB	3V,5V	(detect and read only)
 */


/**
 * \brief Flash chip base address
 */
#define BIOS_FLASH_ADDR        0xa0000000


/** \defgroup bflash_flags     Flash chip features flags
    @{
*/
#define F_FLASH_READ           0x0001
#define F_FLASH_PROGRAM        0x0002
#define F_FLASH_SLEEP          0x0004
#define F_FLASH_ABORT          0x0008

#define F_FLASH_ERASE_SECTOR   0x0020
#define F_FLASH_ERASE_ALL      0x0040
#define F_FLASH_ERASE_SUSPEND  0x0080

#define F_FLASH_LOGIC_3V       0x0200
#define F_FLASH_LOGIC_5V       0x0400
/** @} */

/** \defgroup prog_method_flags     Flash chip programming algorithm flags
    @{
*/
#define F_FLASH_UNKNOWN_PM        0x0000
#define F_FLASH_DATAPOLLING_PM    0x0001
#define F_FLASH_REGPOLLING_PM     0x0002
/** @} */


/**
 * \brief Flash chip manufacturer info
 */
typedef struct bflash_manufacturer {
	char *name;
	uint16 id;
} bflash_manufacturer_t;

/**
 * \brief Flash chip device info
 */
typedef struct bflash_dev {
	char *name;
	uint16 id;
	uint16 flags;
	uint16 unlock[2];
	uint16 size; 			/* KBytes */
	uint16 page_size;		/* Bytes  */
	uint16 sec_count;
	uint32 *sectors;
	uint16 prog_mode;
} bflash_dev_t;

/**
 * \brief Internal manufacturers list 
 * Last entry zero filled (except for name)
 */
extern const bflash_manufacturer_t bflash_manufacturers[];

/**
 * \brief Internal devices list
 * Last entry zero filled (except for name)
 */
extern const bflash_dev_t bflash_devs[];

/**
 * \brief Find flash manufacturer by id in internal list
 */
bflash_manufacturer_t *bflash_find_manufacturer(uint16 mfr_id);

/**
 * \brief Find flash chip device by id in internal list
 */
bflash_dev_t *bflash_find_dev(uint16 dev_id);

/** 
 * \brief Return sector index by sector addr
 */
int bflash_get_sector_index(bflash_dev_t *dev, uint32 addr);

/** 
 * \brief Return 0 if detected supported flash chip
 */
int bflash_detect(bflash_manufacturer_t **mfr, bflash_dev_t **dev);

/** 
 * \brief Erase a sector of flash
 */
int bflash_erase_sector(bflash_dev_t *dev, uint32 addr);

/** 
 * \brief Erase the whole flash chip
 */
int bflash_erase_all(bflash_dev_t *dev);

/** 
 * \brief Sector/Chip erase suspend
 */
int bflash_erase_suspend(bflash_dev_t *dev);

/** 
 * \brief Sector/Chip erase resume
 */
int bflash_erase_resume(bflash_dev_t *dev);

/** 
 * \brief The Sleep command allows the device to complete current operations 
 * before going into Sleep mode.
 */
int bflash_sleep(bflash_dev_t *dev);

/** 
 * \brief This mode only stops Page program or Sector/Chip erase operation currently in progress 
 * and puts the device in Sleep mode.
 */
int bflash_abort(bflash_dev_t *dev);

/** 
 * \brief If program-fail or erase-fail happen, reset the device to abort the operation
 */
void bflash_reset(bflash_dev_t *dev);

/** 
 * \brief Write a single flash value, return -1 if we fail or timeout
 */
int bflash_write_value(bflash_dev_t *dev, uint32 addr, uint8 value);

/**
 * \brief Write a page of flash, return -1 if we fail or timeout 
 */
int bflash_write_page(bflash_dev_t *dev, uint32 addr, uint8 *data);

/** 
 * \brief Write a buffer of data
 */
int bflash_write_data(bflash_dev_t *dev, uint32 addr, void *data, uint32 len);

/** 
 * \brief Detecting flash device, erasing needed sectors 
 * (or all chip, use F_BIOS_ERASE* flags value for erase_mode) 
 * and writing firmware file to the flash memory
 */
int bflash_auto_reflash(const char *file, uint32 start_sector, int erase_mode);


#endif	/* __BIOS_FLASH_H */
