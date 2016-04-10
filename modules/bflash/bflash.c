/* DreamShell ##version##

	bflash.c

	(c)2009-2014 SWAT
	http://www.dc-swat.ru

	04/10/2016 jfdelnero : MX29L3211 & MX29F1610 support fixed.
*/

#include <assert.h>
#include <stdio.h>
#include "ds.h"
#include "drivers/bflash.h"
#include "flash_if.h"

/* Flash port address */
static vuint8 *const flashport = (vuint8 *)BIOS_FLASH_ADDR;

/* Accurate usec delay for JEDEC */
/* jfdn : I am very suspicious about this function. Seems a way slower than it should in my setup... To be checked */
static void jedec_delay(int usec) {

    timer_prime(TMU1, 1000000, 0);
    timer_clear(TMU1);
    timer_start(TMU1);

    while(usec--) {
    	while(!timer_clear(TMU1)) ;
    }

    timer_stop(TMU1);
}

static void delay(int usec) {

	if(usec)
	{
		timer_prime(TMU1, 1000000 / usec, 0);
		timer_clear(TMU1);
		timer_start(TMU1);

		while(!timer_clear(TMU1)) ;

		timer_stop(TMU1);
	}
}

/* Read a flash value */
#define bflash_read(addr) flashport[addr]

/* Determine if the flash memory is busy writing */
#define bflash_busy(addr) ((bflash_read(addr) & D6_MASK) != (bflash_read(addr) & D6_MASK))

/* We'll do this before sending a command */
#define send_unlock(dev)                           \
	flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1; \
	flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2

#define send_unlock_jedec(dev)                     \
	flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1; \
	jedec_delay(10);                               \
	flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2; \
	jedec_delay(10)


/* Send a command (including unlock) */
static void send_cmd(bflash_dev_t *dev, uint16 cmd) {

	if(dev->unlock[0] == ADDR_UNLOCK_1_JEDEC) {

		// int old = irq_disable();

		switch(cmd) {
			case CMD_MANUFACTURER_UNLOCK_DATA:
				send_unlock_jedec(dev);
				flashport[dev->unlock[0]] = cmd;
				timer_spin_sleep(10); // Same as jedec_delay(10000);
				break;
			case CMD_RESET_DATA:
				send_unlock_jedec(dev);
				flashport[dev->unlock[0]] = cmd;
				jedec_delay(40);
			case CMD_PROGRAM_UNLOCK_DATA:
				send_unlock(dev);
				flashport[dev->unlock[0]] = cmd;
			default:
				send_unlock_jedec(dev);
				flashport[dev->unlock[0]] = cmd;
				jedec_delay(10);
				break;
		}

		// irq_restore(old);

	} else {
		send_unlock(dev);
		flashport[dev->unlock[0]] = cmd;
	}
}


bflash_manufacturer_t *bflash_find_manufacturer(uint16 mfr_id) {
	
	int i = 0;
	
	while (bflash_manufacturers[i].id != 0) {
		if (mfr_id == bflash_manufacturers[i].id) 
			return (bflash_manufacturer_t *)&bflash_manufacturers[i];
		i++;
	}
	
	return NULL;
}

bflash_dev_t *bflash_find_dev(uint16 dev_id) {
	
	int i = 0;
	
	while (bflash_devs[i].id != 0) {
		if (dev_id == bflash_devs[i].id) 
			return (bflash_dev_t *)&bflash_devs[i];
		i++;
	}
	
	return NULL;
}

/* Return sector index by sector addr */
int bflash_get_sector_index(bflash_dev_t *dev, uint32 addr) {

	int i = 0;

	for (i = 0; i < dev->sec_count; i++) {
		if(dev->sectors[i] == addr) {
			return i;
		}
	}

	return -1;
}


/* Wait until the flash is ready, with timeout */
static int bflash_wait_ready(bflash_dev_t *dev, uint32 addr, int timeout) {
	int wait = 0;
	
	if (timeout < 0) {
		timeout = -timeout;
		wait = 1;
	}
	while (timeout-- && bflash_busy(addr)) {
		if (wait)
			thd_sleep(100);
		else
			thd_pass();
	}
	if (timeout <= 0) {
		ds_printf("DS_ERROR: Writing to flash timed out\n");
		return -1;
	}
	
	return 0;
}

/* Wait a flag from the flash, with timeout */
static int bflash_wait_flag(bflash_dev_t *dev, uint32 addr, int timeout,uint8 mask) {
	int wait = 0;
	
	if (timeout < 0) {
		timeout = -timeout;
		wait = 1;
	}

	while (timeout-- && !( bflash_read(0) & mask ) ) {
		if (wait)
			thd_sleep(10);
		else
			thd_pass();
	}

	if (timeout <= 0) {
		ds_printf("DS_ERROR: Writing to flash timed out (mask : 0x%.2X)\n",mask);
		return -1;
	}
	
	return 0;
}

/* Write a single flash value, return -1 if we fail or timeout */
int bflash_write_value(bflash_dev_t *dev, uint32 addr, uint8 value) {
	int irqState;

	switch(dev->prog_mode)
	{
		case F_FLASH_DATAPOLLING_PM:
			send_cmd(dev, CMD_PROGRAM_UNLOCK_DATA);
			flashport[addr] = value;

			if (bflash_wait_ready(dev, addr, 1000) < 0) {
				ds_printf("DS_ERROR: Failed writing value to flash at 0x%08lx (0x%02x vs 0x%02x)\n",
					addr, flashport[addr], value);
				return -1;
			}
		break;

		case F_FLASH_REGPOLLING_PM:

			flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1;
			flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2;

			/* The following loop must not be interrupted !
			   The flash device flush its buffer after 100us without access. */

			irqState = irq_disable(); // Disable interrupts

			flashport[dev->unlock[0]] = CMD_PROGRAM_UNLOCK_DATA;

			flashport[addr] = value;

			irq_restore(irqState); // re-enable interrupts

			delay(250); // let's start the flash process (wait 100us min)

			if (bflash_wait_flag(dev, addr + dev->page_size, -800,D7_MASK) < 0) {

				send_cmd(dev, CMD_RESET_DATA);

				ds_printf("DS_ERROR: Failed writing page to flash at 0x%08lx : timeout !\n",addr);
				return -1;
			}

			/* Fast CMD_RESET_DATA */
			flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1;
			delay(5);
			flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2;
			delay(5);
			flashport[dev->unlock[0]] = CMD_RESET_DATA;
			delay(5);

		break;
		default:
			return -2;
		break;

	}

	if (flashport[addr] != value) {
		ds_printf("DS_ERROR: Failed writing value to flash at 0x%08lx (0x%02x vs 0x%02x)\n",
			addr, flashport[addr], value);
		return -1;
	}

	return 0;
}


/* Write a page of flash, return -1 if we fail or timeout */
int bflash_write_page(bflash_dev_t *dev, uint32 addr, uint8 *data) {
	uint32 irqState;
	uint32 i;
	
	if(dev->page_size <= 1) {
		ds_printf("DS_ERROR: The flash device doesn't support page program mode.\n");
		return -1;
	}
	
	switch(dev->prog_mode)
	{
		case F_FLASH_DATAPOLLING_PM:

			send_cmd(dev, CMD_PROGRAM_UNLOCK_DATA);

			for(i = 0; i < dev->page_size; i++) {
				flashport[addr + i] = data[i];
			}

			if (bflash_wait_ready(dev, addr + dev->page_size, -15000) < 0) {
				ds_printf("DS_ERROR: Failed writing page to flash at 0x%08lx (0x%02x vs 0x%02x)\n",
					addr, flashport[addr + dev->page_size], data[dev->page_size - 1]);
				return -1;
			}

			if (flashport[addr + dev->page_size - 1] != data[dev->page_size - 1]) {
				ds_printf("DS_ERROR: Failed writing page to flash at 0x%08lx (0x%02x vs 0x%02x)\n",
					addr, flashport[addr + dev->page_size], data[dev->page_size - 1]);
				return -1;
			}
		break;

		case F_FLASH_REGPOLLING_PM:

			flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1;
			flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2;

			/* The following loop must not be interrupted !
			   The flash device flush its buffer after 100us without access.*/

			irqState = irq_disable(); // Disable interrupts

			flashport[dev->unlock[0]] = CMD_PROGRAM_UNLOCK_DATA;

			for(i = 0; i < dev->page_size; i++) {
				flashport[addr + i] = data[i];
			}

			irq_restore(irqState); // re-enable interrupts

			delay(250); // let's start the flash process (wait 100us min)

			if (bflash_wait_flag(dev, addr + dev->page_size, -800,D7_MASK) < 0) {

				send_cmd(dev, CMD_RESET_DATA);

				ds_printf("DS_ERROR: Failed writing page to flash at 0x%08lx : timeout !\n",addr);
				return -1;
			}

			/* Fast CMD_RESET_DATA */
			flashport[dev->unlock[0]] = CMD_UNLOCK_DATA_1;
			delay(5);
			flashport[dev->unlock[1]] = CMD_UNLOCK_DATA_2;
			delay(5);
			flashport[dev->unlock[0]] = CMD_RESET_DATA;
			delay(5);

			/* Check the whole page */
			for(i = 0; i < dev->page_size; i++) {
				if(flashport[addr + i] != data[i]){
					ds_printf("DS_ERROR: Failed writing page to flash at 0x%08lx (0x%02x vs 0x%02x)\n",
						addr+i, flashport[addr + dev->page_size], data[dev->page_size - 1]);
					return -1;
				}
			}

		break;
		default:
			return -2;
		break;
	}

	return 0;
}

/* Write a buffer of data */
int bflash_write_data(bflash_dev_t *dev, uint32 addr, void *data, uint32 len) {
	uint8 *db = (uint8 *)data;
	uint32	i, cur_len = len, pos = 0;
	
	if(!(dev->flags & F_FLASH_PROGRAM)) {
		ds_printf("DS_ERROR: The flash device doesn't support program mode.\n");
		return -1;
	}
	
	if(dev->page_size > 1) {
		
		while(cur_len >= dev->page_size) {
			
			pos = (len - cur_len);
			
			if (!(pos % 0x10000)) {
				ds_printf("DS_PROCESS: Writing page at 0x%08lx\n", addr + pos);
			}
			
			if (bflash_write_page(dev, addr + pos, db + pos) < 0) {
				ds_printf("DS_PROCESS: Aborting write page at 0x%08lx\n", addr + pos);
				return -1;
			}

			cur_len -= dev->page_size;
		}
	}

	for (i = 0; i < cur_len; i++) {
		
		if (!(i % 0x10000)) {
			ds_printf("DS_PROCESS: Writing value at 0x%08lx\n", i + addr);
		}
		
		if (bflash_write_value(dev, addr + i, db[i]) < 0) {
			ds_printf("DS_PROCESS: Aborting write value at 0x%08lx\n", i + addr);
			return -1;
		}
	}
	return 0;
}

/* Erase a sector of flash */
int bflash_erase_sector(bflash_dev_t *dev, uint32 addr) {
	
	if(!(dev->flags & F_FLASH_ERASE_SECTOR)) {
		ds_printf("DS_ERROR: The flash device doesn't support erase sectors.\n");
		return -1;
	}

	switch(dev->prog_mode)
	{
		case F_FLASH_DATAPOLLING_PM:

			send_cmd(dev, CMD_SECTOR_ERASE_UNLOCK_DATA);
			send_unlock(dev);
			flashport[addr] = CMD_SECTOR_ERASE_UNLOCK_DATA_2;
			if (bflash_wait_ready(dev, addr, -15000) < 0) {
				ds_printf("DS_ERROR: Failed erasing flash sector at 0x%08lx (timeout...)\n", addr);
				return -1;
			}
		break;

		case F_FLASH_REGPOLLING_PM:

			send_cmd(dev, CMD_SECTOR_ERASE_UNLOCK_DATA);
			send_unlock(dev);
			flashport[addr] = CMD_SECTOR_ERASE_UNLOCK_DATA_2;

			if (bflash_wait_flag(dev, addr, -400,D7_MASK) < 0) {
				ds_printf("DS_ERROR: Failed erasing flash sector at 0x%08lx (timeout...)\n", addr);

				send_cmd(dev, CMD_RESET_DATA);

				return -1;
			}

			send_cmd(dev, CMD_RESET_DATA);
		break;
		default:
			return -2;
		break;

	}

	if (flashport[addr] != 0xff) {
		ds_printf("DS_ERROR: Failed erasing flash sector at 0x%08lx\n", addr);
		return -1;
	}

	return 0;
}

/* Erase the whole flash chip */
int bflash_erase_all(bflash_dev_t *dev) {
	uint32 i;

	if(!(dev->flags & F_FLASH_ERASE_ALL)) {
		ds_printf("DS_ERROR: The flash device doesn't support erase full chip.\n");
		return -1;
	}

	switch(dev->prog_mode)
	{
		case F_FLASH_DATAPOLLING_PM:
			send_cmd(dev, CMD_SECTOR_ERASE_UNLOCK_DATA);
			send_cmd(dev, CMD_ERASE_ALL);

			if (bflash_wait_ready(dev, 0, -30000) < 0) {
				ds_printf("DS_ERROR: Failed erasing full chip.\n");
				return -1;
			}
		break;

		case F_FLASH_REGPOLLING_PM:
			send_cmd(dev, CMD_SECTOR_ERASE_UNLOCK_DATA);
			send_cmd(dev, CMD_ERASE_ALL);

			if (bflash_wait_flag(dev, 0, -30000,D6_MASK) < 0) {
				ds_printf("DS_ERROR: Failed erasing full chip. (ready timeout...)\n");
				send_cmd(dev, CMD_RESET_DATA);
				return -1;
			}

			send_cmd(dev, CMD_RESET_DATA);

		break;
	}

	for(i=0;i<dev->size;i++)
	{
		if (flashport[i] != 0xff) {
			ds_printf("DS_ERROR: Failed erasing full chip.\n");
			return -1;
		}
	}

	return 0;
}


int bflash_erase_suspend(bflash_dev_t *dev) {
	
	if(dev->flags & F_FLASH_ERASE_SUSPEND) {
		send_cmd(dev, CMD_ERASE_SUSPEND);
		return 0;
	}
	
	return -1;
}


int bflash_erase_resume(bflash_dev_t *dev) {
	
	if(dev->flags & F_FLASH_ERASE_SUSPEND) {
		send_cmd(dev, CMD_ERASE_RESUME);
		return 0;
	}
	
	return -1;
}


int bflash_sleep(bflash_dev_t *dev) {
	
	if(dev->flags & F_FLASH_SLEEP) {
		send_cmd(dev, CMD_SLEEP);
		return 0;
	}
	
	return -1;
}


int bflash_abort(bflash_dev_t *dev) {
	
	if(dev->flags & F_FLASH_ABORT) {
		send_cmd(dev, CMD_ABORT);
		return 0;
	}
	
	return -1;
}


void bflash_reset(bflash_dev_t *dev) {
	send_cmd(dev, CMD_RESET_DATA);
}


/* Reset and ask for manufacturer code */
static void bflash_get_id(bflash_dev_t *dev, uint16 *mfr_id, uint16 *dev_id) {
	send_cmd(dev, CMD_RESET_DATA);
	send_cmd(dev, CMD_MANUFACTURER_UNLOCK_DATA);
	
	*mfr_id = bflash_read(ADDR_MANUFACTURER);
	*dev_id = bflash_read(ADDR_DEVICE_ID) | (*mfr_id & 0xff) << 8;
}


/* Return 0 if detected supported flashchip */
int bflash_detect(bflash_manufacturer_t **mfr, bflash_dev_t **dev) {
	uint16 mfr_id = 0, dev_id = 0;
	bflash_manufacturer_t *b_mfr = NULL;
	bflash_dev_t *b_dev = NULL;
	bflash_dev_t tmp_dev;
	uint8 prev_mfrid,prev_devid;

	// To slowdown the rom access timings...
	// The default read/write speed is almost
	// the max eeprom read/write speed.
	// And the CS signal is not very clean due to the diode D501.
	//*((vuint32 *)0xA05F7484) = 0x1FF7; // SB_G1RWC - System ROM write access timing
	//*((vuint32 *)0xA05F7480) = 0x1FF7; // SB_G1RRC - System ROM read access timing
	// Commented - seems working fine at full speed now.

	memset(&tmp_dev, 0, sizeof(tmp_dev));
	tmp_dev.flags = F_FLASH_READ;
	tmp_dev.unlock[0] = ADDR_UNLOCK_1_BM;
	tmp_dev.unlock[1] = ADDR_UNLOCK_2_BM;

	/* Store current rom value. Used to detect
	   if the chip respond to the read id command */
	prev_mfrid = flashport[ADDR_MANUFACTURER];
	prev_devid = flashport[ADDR_DEVICE_ID];

	bflash_get_id(&tmp_dev, &mfr_id, &dev_id);

	if( ( ( prev_mfrid == ( mfr_id & 0xFF ) ) && ( prev_devid == ( dev_id & 0xFF ) ) ) && getenv("EMU") == NULL) {
		
		tmp_dev.unlock[0] = ADDR_UNLOCK_1_JEDEC;
		tmp_dev.unlock[1] = ADDR_UNLOCK_2_JEDEC;
		
		bflash_get_id(&tmp_dev, &mfr_id, &dev_id);
		
		if( ( prev_mfrid == ( mfr_id & 0xFF ) ) && ( prev_devid == ( dev_id & 0xFF ) ) ) {
			
			tmp_dev.unlock[0] = ADDR_UNLOCK_1_WM;
			tmp_dev.unlock[1] = ADDR_UNLOCK_2_WM;
			
			bflash_get_id(&tmp_dev, &mfr_id, &dev_id);
		}
	}
	
	send_cmd(&tmp_dev, CMD_RESET_DATA);
	
	//ds_printf("DS_PROCESS: Searching device %04x/%04x\n", mfr_id, dev_id);
	
	b_mfr = bflash_find_manufacturer(mfr_id);
	b_dev = bflash_find_dev(dev_id);
	
	if(b_mfr == NULL && b_dev == NULL) {
		ds_printf("DS_WARNING: Unknown manufacturer/device pair 0x%02x/0x%02x\n", 
					(mfr_id & 0xff), (dev_id & 0xff));
		return -1;
	}
	
	if(b_mfr != NULL) {
		ds_printf("DS_OK: %s flash detected [0x%04x]\n", b_mfr->name, b_mfr->id);
		if(mfr != NULL) {
			*mfr = b_mfr;
		}
	} else {
		ds_printf("DS_OK: Uknown manufacturer flash detected [0x%04x]\n", mfr_id);
	}
	
	if(b_dev != NULL) {
		ds_printf("DS_OK: Flash device is %s [0x%04x]\n", b_dev->name, b_dev->id);
		
		if(dev != NULL) {
			*dev = b_dev;
			
			if(b_dev->id != SEGA_FLASH_DEVICE_ID && tmp_dev.unlock[0] != b_dev->unlock[0]) {
				ds_printf("DS_WARNING: Flash device detected with another commands than in database.\n"
							" Please send bug report about it (0x%04x/0x%04x)\n", tmp_dev.unlock[0], tmp_dev.unlock[1]);
			}
		}
	} else {
		ds_printf("DS_ERROR: Unknown flash device 0x%04x\n", dev_id);
		return -1;
	}
	
	return 0;
}

int bflash_auto_reflash(const char *file, uint32 start_sector, int erase_mode) {

	file_t f;
	uint8 *ptr;
	uint32 len;
	int	i, b_idx;
	bflash_dev_t *dev = NULL;
	bflash_manufacturer_t *mrf = NULL;
	
	if(bflash_detect(&mrf, &dev) < 0) {
		return -1;
	}
	
	if(!(dev->flags & F_FLASH_PROGRAM)) {
		ds_printf("DS_ERROR: %s %s flash chip is not programmable.\n", mrf->name, dev->name);
		return -1;
	}

	f = fs_open(file, O_RDONLY);
	
	if (f < 0) {
		ds_printf("DS_ERROR: Couldn't open %s\n", file);
		return -1;
	}
	
	len = fs_total(f);
	
	if((len / 1024) > dev->size) {
		ds_printf("DS_ERROR: The firmware larger than a flash chip (%d KB vs %d KB).\n", (len / 1024), dev->size);
		fs_close(f);
		return -1;
	}
	
	ds_printf("DS_PROCESS: Loading firmware in to memory...\n");
	ptr = (uint8 *) malloc(len);

	if(ptr == NULL) {
		ds_printf("DS_ERROR: Not enough memory\n");
		fs_close(f);
		return -1;
	}

	memset(ptr, 0, len);
	fs_read(f, ptr, len);
	fs_close(f);

	ds_printf("DS_WARTING: Don't power off your DC and don't switch the bios!\n");
	
	EXPT_GUARD_BEGIN;
	
		b_idx = bflash_get_sector_index(dev, start_sector);
		
		if(b_idx < 0) {
			ds_printf("DS_ERROR: Uknown sector addr 0x%08x\n", start_sector);
			free(ptr);
			EXPT_GUARD_RETURN -1;
		}
		
		if(erase_mode) {
			
			ds_printf("DS_PROCESS: Erasing flash chip...\n");
			
			if(erase_mode == F_FLASH_ERASE_ALL && (dev->size<=2048) ) {
				/* Don't full erase a with more than 2MB -> The others bank datas may be usefull ;) */
				if (bflash_erase_all(dev) < 0) {
					free(ptr);
					EXPT_GUARD_RETURN -1;
				}
				
			} else if(erase_mode == F_FLASH_ERASE_SECTOR) {

				for (i = b_idx; i < dev->sec_count; i++) {
					
					if (dev->sectors[i] >= (len + dev->sectors[b_idx]))
						break;

					/* Don't try to erase the others banks sectors. */
					if(dev->sectors[i] >= 0x200000)
						break;

					ds_printf("DS_PROCESS: Erasing sector: 0x%08x\n", dev->sectors[i]);

					if (bflash_erase_sector(dev, dev->sectors[i]) < 0) {
						free(ptr);
						EXPT_GUARD_RETURN -1;
					}
				}
				
			} else {
				ds_printf("DS_ERROR: Unsupported erase mode: %04x\n", erase_mode);
				free(ptr);
				EXPT_GUARD_RETURN -1;
			}
			
		} else {
			ds_printf("DS_WARNING: Writing to flash without erasing.\n");
		}

		ds_printf("DS_PROCESS: Writing firmware (%d KB) into the flash chip...\n", len / 1024);
		
		if (bflash_write_data(dev, start_sector, ptr, len) < 0) {
			free(ptr);
			EXPT_GUARD_RETURN -1;
		}
	
	EXPT_GUARD_CATCH;
	
		ds_printf("DS_ERROR: Fatal error in reflash process.\n");
		ds_printf("DS_WARNING: Maybe when you reboot your DC can't boot from the current bios.\n");
		ds_printf("DS_INFO: Try to reflash again without rebooting.\n");
		free(ptr);
		EXPT_GUARD_RETURN -1;
		
	EXPT_GUARD_END;

	free(ptr);
	ds_printf("DS_OK: Firmware written successfully\n");
	return 0;
}
