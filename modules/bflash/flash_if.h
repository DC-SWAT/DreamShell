/** 
 *	\file      flash_if.h
 *	\brief     Bios flash interface
 *	\date      2009-2014
 *	\author    SWAT
 *	\copyright http://www.dc-swat.ru
 */

#ifndef __BIOS_FLASH_IF_H
#define __BIOS_FLASH_IF_H

#include <arch/types.h>
#include "drivers/bflash.h"

#define ADDR_MANUFACTURER               0x0000
#define ADDR_DEVICE_ID                  0x0002
#define ADDR_SECTOR_LOCK                0x0004
#define ADDR_HANDSHAKE                  0x0006

/* Byte mode */
#define ADDR_UNLOCK_1_BM                0x0AAA
#define ADDR_UNLOCK_2_BM                0x0555

/* Word mode */
#define ADDR_UNLOCK_1_WM                0x0555
#define ADDR_UNLOCK_2_WM                0x02AA

/* JEDEC */
/* words to put on A14-A0 without A-1 (Q15) ! -> The address need to be shifted. */
#define ADDR_UNLOCK_1_JEDEC             (0x5555<<1)
#define ADDR_UNLOCK_2_JEDEC             (0x2AAA<<1)

#define CMD_UNLOCK_DATA_1               0x00AA
#define CMD_UNLOCK_DATA_2               0x0055
#define CMD_MANUFACTURER_UNLOCK_DATA    0x0090
#define CMD_UNLOCK_BYPASS_MODE          0x0020
#define CMD_PROGRAM_UNLOCK_DATA         0x00A0
#define CMD_SLEEP                       0x00C0
#define CMD_ABORT                       0x00E0
#define CMD_RESET_DATA                  0x00F0
#define CMD_SECTOR_ERASE_UNLOCK_DATA    0x0080
#define CMD_SECTOR_ERASE_UNLOCK_DATA_2  0x0030
#define CMD_ERASE_ALL                   0x0010
#define CMD_ERASE_SUSPEND               0x00B0
#define CMD_ERASE_RESUME                0x0030
#define CMD_UNLOCK_SECTOR               0x0060
#define CMD_CFI_QUERY                   0x0062

#define D6_MASK                         0x40 /* Ready bit */
#define D7_MASK                         0x80 /* Data polling bit */

#define SEGA_FLASH_DEVICE_ID            0xFF28 /* The "SEGA ID" is just the data read back from the ROM at the address 0 and 2... */

#endif /* __BIOS_FLASH_IF_H */
