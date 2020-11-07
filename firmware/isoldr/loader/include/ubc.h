/**
 * DreamShell ISO Loader
 * SH4 UBC
 * (c)2013-2020 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#include <arch/types.h>

/* UBC register bitmasks */
#define UBC_BAMR_NOASID     (1<<2)
#define UBC_BAMR_MASK_10    (1)
#define UBC_BAMR_MASK_12    (1<<1)
#define UBC_BAMR_MASK_16    (1<<3)
#define UBC_BAMR_MASK_20    (UBC_BAMR_MASK_16 | UBC_BAMR_MASK_10)
#define UBC_BAMR_MASK_ALL   (UBC_BAMR_MASK_10 | UBC_BAMR_MASK_12)

#define UBC_BBR_OPERAND     (1<<5)
#define UBC_BBR_INSTRUCT    (1<<4)
#define UBC_BBR_WRITE       (1<<3)
#define UBC_BBR_READ        (1<<2)
#define UBC_BBR_UNI         (UBC_BBR_OPERAND | UBC_BBR_INSTRUCT)
#define UBC_BBR_RW          (UBC_BBR_WRITE | UBC_BBR_READ)

#define UBC_BRCR_CMFA       (1<<15)
#define UBC_BRCR_CMFB       (1<<14)

#define UBC_BRCR_PCBA       (1<<10)
#define UBC_BRCR_PCBB       (1<<6)

#define UBC_BRCR_UBDE       (1)


/* UBC channel A registers */
#define UBC_R_BARA	*(volatile uint32 *)0xff200000
#define UBC_R_BAMRA	*(volatile uint8 *)0xff200004
#define UBC_R_BBRA	*(volatile uint16 *)0xff200008
#define UBC_R_BASRA	*(volatile uint8 *)0xff000014

/* UBC channel B registers */
#define UBC_R_BARB	*(volatile uint32 *)0xff20000c
#define UBC_R_BAMRB	*(volatile uint8 *)0xff200010
#define UBC_R_BBRB	*(volatile uint16 *)0xff200014
#define UBC_R_BASRB	*(volatile uint8 *)0xff000018

/* UBC global registers */
#define UBC_R_BDRB	*(volatile uint32 *)0xff200018
#define UBC_R_BDRMB	*(volatile uint32 *)0xff20001c
#define UBC_R_BRCR	*(volatile uint16 *)0xff200020


typedef enum {
    UBC_CHANNEL_A,
    UBC_CHANNEL_B
} ubc_channel;


/* Initializing the UBC */
void ubc_init(void);

/* Configure the appropriate channel with the specified breakpoint. */
int  ubc_configure_channel (ubc_channel channel, uint32 breakpoint, uint16 options);

/* Clear the appropriate channel's options. */
void ubc_clear_channel     (ubc_channel channel);

/* Clear the appropriate channel's break bit. */
void ubc_clear_break       (ubc_channel channel);

int  ubc_is_channel_break  (ubc_channel channel);

