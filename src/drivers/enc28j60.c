/* KallistiOS ##version##

   drivers/enc28j60.c

   Copyright (C) 2011-2014 SWAT

*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dc/asic.h>
#include <dc/flashrom.h>
#include <arch/irq.h>
#include <arch/timer.h>
#include <kos/net.h>
#include <kos/netcfg.h>
#include "drivers/enc28j60.h"
#include "drivers/spi.h"


/**
 * partitioning of the internal 8kB tx/rx buffers
 */
#define RX_START 0x0000     /* do not change this! */
#define TX_END (0x2000 - 1) /* do not change this! */
#define TX_START 0x1a00
#define RX_END (TX_START - 1)

/**
 * register addresses consist of three parts:
 * bits 4-0: real address
 * bits 6-5: bank number
 * bit    7: skip a dummy byte (for mac/phy access)
 */
#define MASK_ADDR 0x1f
#define MASK_BANK 0x60
#define MASK_DBRD 0x80
#define SHIFT_ADDR 0
#define SHIFT_BANK 5
#define SHIFT_DBRD 7

/* bank independent */
#define EIE             0x1b
#define EIR             0x1c
#define ESTAT           0x1d
#define ECON2           0x1e
#define ECON1           0x1f

/* bank 0 */
#define ERDPTL          (0x00 | 0x00)
#define ERDPTH          (0x01 | 0x00)
#define EWRPTL          (0x02 | 0x00)
#define EWRPTH          (0x03 | 0x00)
#define ETXSTL          (0x04 | 0x00)
#define ETXSTH          (0x05 | 0x00)
#define ETXNDL          (0x06 | 0x00)
#define ETXNDH          (0x07 | 0x00)
#define ERXSTL          (0x08 | 0x00)
#define ERXSTH          (0x09 | 0x00)
#define ERXNDL          (0x0a | 0x00)
#define ERXNDH          (0x0b | 0x00)
#define ERXRDPTL        (0x0c | 0x00)
#define ERXRDPTH        (0x0d | 0x00)
#define ERXWRPTL        (0x0e | 0x00)
#define ERXWRPTH        (0x0f | 0x00)
#define EDMASTL         (0x10 | 0x00)
#define EDMASTH         (0x11 | 0x00)
#define EDMANDL         (0x12 | 0x00)
#define EDMANDH         (0x13 | 0x00)
#define EDMADSTL        (0x14 | 0x00)
#define EDMADSTH        (0x15 | 0x00)
#define EDMACSL         (0x16 | 0x00)
#define EDMACSH         (0x17 | 0x00)

/* bank 1 */
#define EHT0            (0x00 | 0x20)
#define EHT1            (0x01 | 0x20)
#define EHT2            (0x02 | 0x20)
#define EHT3            (0x03 | 0x20)
#define EHT4            (0x04 | 0x20)
#define EHT5            (0x05 | 0x20)
#define EHT6            (0x06 | 0x20)
#define EHT7            (0x07 | 0x20)
#define EPMM0           (0x08 | 0x20)
#define EPMM1           (0x09 | 0x20)
#define EPMM2           (0x0a | 0x20)
#define EPMM3           (0x0b | 0x20)
#define EPMM4           (0x0c | 0x20)
#define EPMM5           (0x0d | 0x20)
#define EPMM6           (0x0e | 0x20)
#define EPMM7           (0x0f | 0x20)
#define EPMCSL          (0x10 | 0x20)
#define EPMCSH          (0x11 | 0x20)
#define EPMOL           (0x14 | 0x20)
#define EPMOH           (0x15 | 0x20)
#define EWOLIE          (0x16 | 0x20)
#define EWOLIR          (0x17 | 0x20)
#define ERXFCON         (0x18 | 0x20)
#define EPKTCNT         (0x19 | 0x20)

/* bank 2 */
#define MACON1          (0x00 | 0x40 | 0x80)
#define MACON2          (0x01 | 0x40 | 0x80)
#define MACON3          (0x02 | 0x40 | 0x80)
#define MACON4          (0x03 | 0x40 | 0x80)
#define MABBIPG         (0x04 | 0x40 | 0x80)
#define MAIPGL          (0x06 | 0x40 | 0x80)
#define MAIPGH          (0x07 | 0x40 | 0x80)
#define MACLCON1        (0x08 | 0x40 | 0x80)
#define MACLCON2        (0x09 | 0x40 | 0x80)
#define MAMXFLL         (0x0a | 0x40 | 0x80)
#define MAMXFLH         (0x0b | 0x40 | 0x80)
#define MAPHSUP         (0x0d | 0x40 | 0x80)
#define MICON           (0x11 | 0x40 | 0x80)
#define MICMD           (0x12 | 0x40 | 0x80)
#define MIREGADR        (0x14 | 0x40 | 0x80)
#define MIWRL           (0x16 | 0x40 | 0x80)
#define MIWRH           (0x17 | 0x40 | 0x80)
#define MIRDL           (0x18 | 0x40 | 0x80)
#define MIRDH           (0x19 | 0x40 | 0x80)
                
/* bank 3 */
#define MAADR1          (0x00 | 0x60 | 0x80)
#define MAADR0          (0x01 | 0x60 | 0x80)
#define MAADR3          (0x02 | 0x60 | 0x80)
#define MAADR2          (0x03 | 0x60 | 0x80)
#define MAADR5          (0x04 | 0x60 | 0x80)
#define MAADR4          (0x05 | 0x60 | 0x80)
#define EBSTSD          (0x06 | 0x60)
#define EBSTCON         (0x07 | 0x60)
#define EBSTCSL         (0x08 | 0x60)
#define EBSTCSH         (0x09 | 0x60)
#define MISTAT          (0x0a | 0x60 | 0x80)
#define EREVID          (0x12 | 0x60)
#define ECOCON          (0x15 | 0x60)
#define EFLOCON         (0x17 | 0x60)
#define EPAUSL          (0x18 | 0x60)
#define EPAUSH          (0x19 | 0x60)

/* phy */
#define PHCON1          0x00
#define PHSTAT1         0x01
#define PHID1           0x02
#define PHID2           0x03
#define PHCON2          0x10
#define PHSTAT2         0x11
#define PHIE            0x12
#define PHIR            0x13
#define PHLCON          0x14

/* EIE */
#define EIE_INTIE       7
#define EIE_PKTIE       6
#define EIE_DMAIE       5
#define EIE_LINKIE      4
#define EIE_TXIE        3
#define EIE_WOLIE       2
#define EIE_TXERIE      1
#define EIE_RXERIE      0

/* EIR */
#define EIR_PKTIF       6
#define EIR_DMAIF       5
#define EIR_LINKIF      4
#define EIR_TXIF        3
#define EIR_WOLIF       2
#define EIR_TXERIF      1
#define EIR_RXERIF      0

/* ESTAT */
#define ESTAT_INT       7
#define ESTAT_LATECOL   4
#define ESTAT_RXBUSY    2
#define ESTAT_TXABRT    1
#define ESTAT_CLKRDY    0

/* ECON2 */
#define ECON2_AUTOINC   7
#define ECON2_PKTDEC    6
#define ECON2_PWRSV     5
#define ECON2_VRPS      4

/* ECON1 */
#define ECON1_TXRST     7
#define ECON1_RXRST     6
#define ECON1_DMAST     5
#define ECON1_CSUMEN    4
#define ECON1_TXRTS     3
#define ECON1_RXEN      2
#define ECON1_BSEL1     1
#define ECON1_BSEL0     0

/* EWOLIE */
#define EWOLIE_UCWOLIE  7
#define EWOLIE_AWOLIE   6
#define EWOLIE_PMWOLIE  4
#define EWOLIE_MPWOLIE  3
#define EWOLIE_HTWOLIE  2
#define EWOLIE_MCWOLIE  1
#define EWOLIE_BCWOLIE  0

/* EWOLIR */
#define EWOLIR_UCWOLIF  7
#define EWOLIR_AWOLIF   6
#define EWOLIR_PMWOLIF  4
#define EWOLIR_MPWOLIF  3
#define EWOLIR_HTWOLIF  2
#define EWOLIR_MCWOLIF  1
#define EWOLIR_BCWOLIF  0

/* ERXFCON */
#define ERXFCON_UCEN    7
#define ERXFCON_ANDOR   6
#define ERXFCON_CRCEN   5
#define ERXFCON_PMEN    4
#define ERXFCON_MPEN    3
#define ERXFCON_HTEN    2
#define ERXFCON_MCEN    1
#define ERXFCON_BCEN    0

/* MACON1 */
#define MACON1_LOOPBK   4
#define MACON1_TXPAUS   3
#define MACON1_RXPAUS   2
#define MACON1_PASSALL  1
#define MACON1_MARXEN   0

/* MACON2 */
#define MACON2_MARST    7
#define MACON2_RNDRST   6
#define MACON2_MARXRST  3
#define MACON2_RFUNRST  2
#define MACON2_MATXRST  1
#define MACON2_TFUNRST  0

/* MACON3 */
#define MACON3_PADCFG2  7
#define MACON3_PADCFG1  6
#define MACON3_PADCFG0  5
#define MACON3_TXCRCEN  4
#define MACON3_PHDRLEN  3
#define MACON3_HFRMEN   2
#define MACON3_FRMLNEN  1
#define MACON3_FULDPX   0

/* MACON4 */
#define MACON4_DEFER    6
#define MACON4_BPEN     5
#define MACON4_NOBKOFF  4
#define MACON4_LONGPRE  1
#define MACON4_PUREPRE  0

/* MAPHSUP */
#define MAPHSUP_RSTINTFC 7
#define MAPHSUP_RSTRMII  3

/* MICON */
#define MICON_RSTMII    7

/* MICMD */
#define MICMD_MIISCAN   1
#define MICMD_MIIRD     0

/* EBSTCON */
#define EBSTCON_PSV2    7
#define EBSTCON_PSV1    6
#define EBSTCON_PSV0    5
#define EBSTCON_PSEL    4
#define EBSTCON_TMSEL1  3
#define EBSTCON_TMSEL0  2
#define EBSTCON_TME     1
#define EBSTCON_BISTST  0

/* MISTAT */
#define MISTAT_NVALID   2
#define MISTAT_SCAN     1
#define MISTAT_BUSY     0

/* ECOCON */
#define ECOCON_COCON2   2
#define ECOCON_COCON1   1
#define ECOCON_COCON0   0

/* EFLOCON */
#define EFLOCON_FULDPXS 2
#define EFLOCON_FCEN1   1
#define EFLOCON_FCEN0   0

/* PHCON1 */
#define PHCON1_PRST     15
#define PHCON1_PLOOPBK  14
#define PHCON1_PPWRSV   11
#define PHCON1_PDPXMD   8

/* PHSTAT1 */
#define PHSTAT1_PFDPX   12
#define PHSTAT1_PHDPX   11
#define PHSTAT1_LLSTAT  2
#define PHSTAT1_JBSTAT  1

/* PHCON2 */
#define PHCON2_FRCLNK   14
#define PHCON2_TXDIS    13
#define PHCON2_JABBER   10
#define PHCON2_HDLDIS   8

/* PHSTAT2 */
#define PHSTAT2_TXSTAT  13
#define PHSTAT2_RXSTAT  12
#define PHSTAT2_COLSTAT 11
#define PHSTAT2_LSTAT   10
#define PHSTAT2_DPXSTAT 9
#define PHSTAT2_PLRITY  4

/* PHIE */
#define PHIE_PLINKIE    4
#define PHIE_PGEIE      1

/* PHIR */
#define PHIR_LINKIF     4
#define PHIR_PGIF       2

/* PHLCON */
#define PHLCON_LACFG3   11
#define PHLCON_LACFG2   10
#define PHLCON_LACFG1   9
#define PHLCON_LACFG0   8
#define PHLCON_LBCFG3   7
#define PHLCON_LBCFG2   6
#define PHLCON_LBCFG1   5
#define PHLCON_LBCFG0   4
#define PHLCON_LFRQ1    3
#define PHLCON_LFRQ0    2
#define PHLCON_STRCH    1

#define ENC28J60_FULL_DUPLEX 1



/**
 * \internal
 * Microchip ENC28J60 I/O implementation
 *
 */

#define enc28j60_rcr(address) spi_cc_send_byte(0x00 | (address))
#define enc28j60_rbm() spi_cc_send_byte(0x3a)
#define enc28j60_wcr(address) spi_cc_send_byte(0x40 | (address))
#define enc28j60_wbm(address) spi_cc_send_byte(0x7a)
#define enc28j60_bfs(address) spi_cc_send_byte(0x80 | (address))
#define enc28j60_bfc(address) spi_cc_send_byte(0xa0 | (address))
#define enc28j60_sc() spi_cc_send_byte(0xff)


static int spi_cs = SPI_CS_ENC28J60;
static int spi_reset = -1;
static int old_spi_delay = SPI_DEFAULT_DELAY;


#define enc28j60_select() do {	\
	spi_cs_on(spi_cs); \
	old_spi_delay = spi_get_delay(); \
	if(old_spi_delay != SPI_ENC28J60_DELAY) \
		spi_set_delay(SPI_ENC28J60_DELAY); \
} while(0)

#define enc28j60_deselect() do { \
	if(old_spi_delay != SPI_ENC28J60_DELAY) \
		spi_set_delay(old_spi_delay); \
	spi_cs_off(spi_cs); \
} while(0)

	
#define enc28j60_reset_on()  spi_cs_on(spi_reset)
#define enc28j60_reset_off() spi_cs_off(spi_reset)

//#define ENC_DEBUG 1


/**
 * \internal
 * Initializes the SPI interface to the ENC28J60 chips.
 */
void enc28j60_io_init(int cs, int rs)
{
	if(cs > -1) {
		spi_cs = cs;
	}
	
	if(rs > -1) {
		spi_reset = rs;
	}
	
    /* Initialize SPI */
    spi_init(0);
}

/**
 * \internal
 * Forces a reset to the ENC28J60.
 *
 * After the reset a reinitialization is necessary.
 */
void enc28j60_reset()
{
    enc28j60_reset_on();
    thd_sleep(10);
    enc28j60_reset_off();
    thd_sleep(10);
}

/**
 * \internal
 * Reads the value of a hardware register.
 *
 * \param[in] address The address of the register to read.
 * \returns The register value.
 */
uint8 enc28j60_read(uint8 address)
{
    uint8 result;

    /* switch to register bank */
    enc28j60_bank((address & MASK_BANK) >> SHIFT_BANK);
    
    /* read from address */
    enc28j60_select();
    enc28j60_rcr((address & MASK_ADDR) >> SHIFT_ADDR);
    if(address & MASK_DBRD)
        spi_cc_rec_byte();
    result = spi_cc_rec_byte();
    enc28j60_deselect();

    return result;
}

/**
 * \internal
 * Writes the value of a hardware register.
 *
 * \param[in] address The address of the register to write.
 * \param[in] value The value to write into the register.
 */
void enc28j60_write(uint8 address, uint8 value)
{
    /* switch to register bank */
    enc28j60_bank((address & MASK_BANK) >> SHIFT_BANK);
    
    /* write to address */
    enc28j60_select();
    enc28j60_wcr((address & MASK_ADDR) >> SHIFT_ADDR);
    spi_cc_send_byte(value);
    enc28j60_deselect();
}

/**
 * \internal
 * Clears bits in a hardware register.
 *
 * Performs a NAND operation on the current register value
 * and the given bitmask.
 *
 * \param[in] address The address of the register to alter.
 * \param[in] bits A bitmask specifiying the bits to clear.
 */
void enc28j60_clear_bits(uint8 address, uint8 bits)
{
    /* switch to register bank */
    enc28j60_bank((address & MASK_BANK) >> SHIFT_BANK);
    
    /* write to address */
    enc28j60_select();
    enc28j60_bfc((address & MASK_ADDR) >> SHIFT_ADDR);
    spi_cc_send_byte(bits);
    enc28j60_deselect();
}

/**
 * \internal
 * Sets bits in a hardware register.
 *
 * Performs an OR operation on the current register value
 * and the given bitmask.
 *
 * \param[in] address The address of the register to alter.
 * \param[in] bits A bitmask specifiying the bits to set.
 */
void enc28j60_set_bits(uint8 address, uint8 bits)
{
    /* switch to register bank */
    enc28j60_bank((address & MASK_BANK) >> SHIFT_BANK);
    
    /* write to address */
    enc28j60_select();
    enc28j60_bfs((address & MASK_ADDR) >> SHIFT_ADDR);
    spi_cc_send_byte(bits);
    enc28j60_deselect();
}

/**
 * \internal
 * Reads the value of a hardware PHY register.
 *
 * \param[in] address The address of the PHY register to read.
 * \returns The register value.
 */
uint16 enc28j60_read_phy(uint8 address)
{
    enc28j60_write(MIREGADR, address);
    enc28j60_set_bits(MICMD, (1 << MICMD_MIIRD));

    thd_sleep(10);

    while(enc28j60_read(MISTAT) & (1 << MISTAT_BUSY));
    
    enc28j60_clear_bits(MICMD, (1 << MICMD_MIIRD));

    return ((uint16_t) enc28j60_read(MIRDH)) << 8 |
           ((uint16_t) enc28j60_read(MIRDL));
}

/**
 * \internal
 * Writes the value to a hardware PHY register.
 *
 * \param[in] address The address of the PHY register to write.
 * \param[in] value The value to write into the register.
 */
void enc28j60_write_phy(uint8 address, uint16 value)
{
    enc28j60_write(MIREGADR, address);
    enc28j60_write(MIWRL, value & 0xff);
    enc28j60_write(MIWRH, value >> 8);
    
    thd_sleep(10);

    while(enc28j60_read(MISTAT) & (1 << MISTAT_BUSY));
}

/**
 * \internal
 * Reads a byte from the RAM buffer at the current position.
 *
 * \returns The byte read from the current RAM position.
 */
uint8 enc28j60_read_buffer_byte()
{
    uint8_t b;

    enc28j60_select();
    enc28j60_rbm();

    b = spi_cc_rec_byte();
    
    enc28j60_deselect();

    return b;
}

/**
 * \internal
 * Writes a byte to the RAM buffer at the current position.
 *
 * \param[in] b The data byte to write.
 */
void enc28j60_write_buffer_byte(uint8 b)
{
    enc28j60_select();
    enc28j60_wbm();

    spi_cc_send_byte(b);
    
    enc28j60_deselect();
}

/**
 * \internal
 * Reads multiple bytes from the RAM buffer.
 *
 * \param[out] buffer A pointer to the buffer which receives the data.
 * \param[in] buffer_len The buffer length and number of bytes to read.
 */
void enc28j60_read_buffer(uint8* buffer, uint16 buffer_len)
{
    enc28j60_select();
    enc28j60_rbm();

    spi_cc_rec_data(buffer, buffer_len);
    
    enc28j60_deselect();
}

/**
 * \internal
 * Writes multiple bytes to the RAM buffer.
 *
 * \param[in] buffer A pointer to the buffer containing the data to write.
 * \param[in] buffer_len The number of bytes to write.
 */
void enc28j60_write_buffer(const uint8* buffer, uint16 buffer_len)
{
    enc28j60_select();
    enc28j60_wbm();

    spi_cc_send_data(buffer, buffer_len);
    
    enc28j60_deselect();
}

/**
 * Switches the hardware register bank.
 *
 * \param[in] num The index of the register bank to switch to.
 */
void enc28j60_bank(uint8 num)
{
    static uint8 bank = 0xff;

    if(num == bank)
        return;
    
    /* clear bank bits */
    enc28j60_select();
    enc28j60_bfc(ECON1);
    spi_cc_send_byte(0x03);
    enc28j60_deselect();

    /* set bank bits */
    enc28j60_select();
    enc28j60_bfs(ECON1);
    spi_cc_send_byte(num);
    enc28j60_deselect();

    bank = num;
}



/**
 * Reset and initialize the ENC28J60 and starts packet transmission/reception.
 *
 * \param[in] mac A pointer to a 6-byte buffer containing the MAC address.
 * \returns \c true on success, \c false on failure.
 */
int enc28j60_init(const uint8* mac)
{
    /* init i/o */
    enc28j60_io_init(spi_cs, spi_reset);

    /* reset device */
    enc28j60_reset();

    /* configure rx/tx buffers */
    enc28j60_write(ERXSTL, RX_START & 0xff);
    enc28j60_write(ERXSTH, RX_START >> 8);
    enc28j60_write(ERXNDL, RX_END & 0xff);
    enc28j60_write(ERXNDH, RX_END >> 8);

    enc28j60_write(ERXWRPTL, RX_START & 0xff);
    enc28j60_write(ERXWRPTH, RX_START >> 8);
    enc28j60_write(ERXRDPTL, RX_START & 0xff);
    enc28j60_write(ERXRDPTH, RX_START >> 8);

    /* configure frame filters */
    enc28j60_write(ERXFCON, (1 << ERXFCON_UCEN)  | /* accept unicast packets */
                            (1 << ERXFCON_CRCEN) | /* accept packets with valid CRC only */
                            (0 << ERXFCON_PMEN)  | /* no pattern matching */
                            (0 << ERXFCON_MPEN)  | /* ignore magic packets */
                            (0 << ERXFCON_HTEN)  | /* disable hash table filter */
                            (0 << ERXFCON_MCEN)  | /* ignore multicast packets */
                            (1 << ERXFCON_BCEN)  | /* accept broadcast packets */
                            (0 << ERXFCON_ANDOR)   /* packets must meet at least one criteria */
                  );

    /* configure MAC */
    enc28j60_clear_bits(MACON2, (1 << MACON2_MARST));
    enc28j60_write(MACON1, (0 << MACON1_LOOPBK) |
#if ENC28J60_FULL_DUPLEX 
                           (1 << MACON1_TXPAUS) |
                           (1 << MACON1_RXPAUS) |
#else
                           (0 << MACON1_TXPAUS) |
                           (0 << MACON1_RXPAUS) |
#endif
                           (0 << MACON1_PASSALL) |
                           (1 << MACON1_MARXEN)
                  );
    enc28j60_write(MACON3, (1 << MACON3_PADCFG2) |
                           (1 << MACON3_PADCFG1) |
                           (1 << MACON3_PADCFG0) |
                           (1 << MACON3_TXCRCEN) |
                           (0 << MACON3_PHDRLEN) |
                           (0 << MACON3_HFRMEN) |
                           (1 << MACON3_FRMLNEN) |
#if ENC28J60_FULL_DUPLEX 
                           (1 << MACON3_FULDPX)
#else
                           (0 << MACON3_FULDPX)
#endif
                  );
    enc28j60_write(MAMXFLL, 0xee);
    enc28j60_write(MAMXFLH, 0x05);
#if ENC28J60_FULL_DUPLEX 
    enc28j60_write(MABBIPG, 0x15);
#else
    enc28j60_write(MABBIPG, 0x12);
#endif
    enc28j60_write(MAIPGL, 0x12);
#if !ENC28J60_FULL_DUPLEX 
    enc28j60_write(MAIPGH, 0x0c);
#endif
    enc28j60_write(MAADR0, mac[5]);
    enc28j60_write(MAADR1, mac[4]);
    enc28j60_write(MAADR2, mac[3]);
    enc28j60_write(MAADR3, mac[2]);
    enc28j60_write(MAADR4, mac[1]);
    enc28j60_write(MAADR5, mac[0]);

    /* configure PHY */
    enc28j60_write_phy(PHLCON, (1 << PHLCON_LACFG3) |
                               (1 << PHLCON_LACFG2) |
                               (0 << PHLCON_LACFG1) |
                               (1 << PHLCON_LACFG0) |
                               (0 << PHLCON_LBCFG3) |
                               (1 << PHLCON_LBCFG2) |
                               (0 << PHLCON_LBCFG1) |
                               (1 << PHLCON_LBCFG0) |
                               (0 << PHLCON_LFRQ1) |
                               (0 << PHLCON_LFRQ0) |
                               (1 << PHLCON_STRCH)
                      );
    enc28j60_write_phy(PHCON1, (0 << PHCON1_PRST) |
                               (0 << PHCON1_PLOOPBK) |
                               (0 << PHCON1_PPWRSV) |
#if ENC28J60_FULL_DUPLEX 
                               (1 << PHCON1_PDPXMD)
#else
                               (0 << PHCON1_PDPXMD)
#endif
                      );
    enc28j60_write_phy(PHCON2, (0 << PHCON2_FRCLNK) |
                               (0 << PHCON2_TXDIS) |
                               (0 << PHCON2_JABBER) |
                               (1 << PHCON2_HDLDIS)
                      );

    /* start receiving packets */
    enc28j60_set_bits(ECON2, (1 << ECON2_AUTOINC));
    enc28j60_set_bits(ECON1, (1 << ECON1_RXEN));
    
    return 1;
}


int enc28j60_link_up() {
    uint16 phstat1 = enc28j60_read_phy(PHSTAT1);
    return phstat1 & (1 << PHSTAT1_LLSTAT);
}



/**
 * \internal
 * Microchip ENC28J60 packet handling
 *
 */

static uint16 packet_ptr = RX_START;

/**
 * Fetches a pending packet from the RAM buffer of the ENC28J60.
 *
 * The packet is written into the given buffer and the size of the packet
 * (ethernet header plus payload, exclusive the CRC) is returned.
 *
 * Zero is returned in the following cases:
 * - There is no packet pending.
 * - The packet is too large to completely fit into the buffer.
 * - Some error occured.
 *
 * \param[out] buffer The pointer to the buffer which receives the packet.
 * \param[in] buffer_len The length of the buffer.
 * \returns The packet size in bytes on success, \c 0 in the cases noted above.
 */
uint16 enc28j60_receive_packet(uint8* buffer, uint16 buffer_len)
{
    if(!enc28j60_read(EPKTCNT))
        return 0;

    /* set read pointer */
    enc28j60_write(ERDPTL, packet_ptr & 0xff);
    enc28j60_write(ERDPTH, packet_ptr >> 8);

    /* read pointer to next packet */
    packet_ptr = ((uint16_t) enc28j60_read_buffer_byte()) |
                 ((uint16_t) enc28j60_read_buffer_byte()) << 8;

    /* read packet length */
    uint16 packet_len = ((uint16_t) enc28j60_read_buffer_byte()) |
                          ((uint16_t) enc28j60_read_buffer_byte()) << 8;
    packet_len -= 4; /* ignore CRC */

    /* read receive status */
    enc28j60_read_buffer_byte();
    enc28j60_read_buffer_byte();

    /* read packet */
    if(packet_len <= buffer_len)
        enc28j60_read_buffer(buffer, packet_len);

    /* free packet memory */
    /* errata #13 */
    uint16 packet_ptr_errata = packet_ptr - 1;
    if(packet_ptr_errata < RX_START || packet_ptr_errata > RX_END)
        packet_ptr_errata = RX_END;
    enc28j60_write(ERXRDPTL, packet_ptr_errata & 0xff);
    enc28j60_write(ERXRDPTH, packet_ptr_errata >> 8);

    /* decrement packet counter */
    enc28j60_set_bits(ECON2, (1 << ECON2_PKTDEC));

    if(packet_len <= buffer_len)
        return packet_len;
    else
        return 0;
}

/**
 * Writes a packet to the RAM buffer of the ENC28J60 and starts transmission.
 *
 * The packet buffer contains the ethernet header and the payload without CRC.
 * The checksum is automatically generated by the on-chip calculator.
 *
 * \param[in] buffer A pointer to the buffer containing the packet to be sent.
 * \param[in] buffer_len The length of the ethernet packet header plus payload.
 * \returns \c true if the packet was sent, \c false otherwise.
 */
int enc28j60_send_packet(const uint8* buffer, uint16 buffer_len)
{
    if(!buffer || !buffer_len)
        return 0;

    /* errata #12 */
    if(enc28j60_read(EIR) & (1 << EIR_TXERIF))
    {
        enc28j60_set_bits(ECON1, (1 << ECON1_TXRST));
        enc28j60_clear_bits(ECON1, (1 << ECON1_TXRST));
    }

    /* wait until previous packet was sent */
    while(enc28j60_read(ECON1) & (1 << ECON1_TXRTS));

    /* set start of packet */
    enc28j60_write(ETXSTL, TX_START & 0xff);
    enc28j60_write(ETXSTH, TX_START >> 8);
    
    /* set end of packet */
    enc28j60_write(ETXNDL, (TX_START + buffer_len) & 0xff);
    enc28j60_write(ETXNDH, (TX_START + buffer_len) >> 8);

    /* set write pointer */
    enc28j60_write(EWRPTL, TX_START & 0xff);
    enc28j60_write(EWRPTH, TX_START >> 8);
    
    /* per packet control byte */
    enc28j60_write_buffer_byte(0x00);

    /* send packet */
    enc28j60_write_buffer(buffer, buffer_len);

    /* start transmission */
    enc28j60_set_bits(ECON1, (1 << ECON1_TXRTS));

    return 1;
}



/****************************************************************************/
/* Netcore interface */

netif_t enc_if;

static void set_ipv6_lladdr() {
	/* Set up the IPv6 link-local address. This is done in accordance with
	   Section 4/5 of RFC 2464 based on the MAC Address of the adapter. */
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[0]  = 0xFE;
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[1]  = 0x80;
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[8]  = enc_if.mac_addr[0] ^ 0x02;
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[9]  = enc_if.mac_addr[1];
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[10] = enc_if.mac_addr[2];
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[11] = 0xFF;
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[12] = 0xFE;
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[13] = enc_if.mac_addr[3];
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[14] = enc_if.mac_addr[4];
	enc_if.ip6_lladdr.__s6_addr.__s6_addr8[15] = enc_if.mac_addr[5];
}

static int enc_if_detect(netif_t * self) {
	if (self->flags & NETIF_DETECTED)
		return 0;

	uint8 reg;
	
	/* init i/o */
	enc28j60_io_init(spi_cs, spi_reset);

    /* reset device */
	enc28j60_reset();
	reg = enc28j60_read(EREVID);
	
	if(reg == 0x00 || reg == 0xff) {
#ifdef ENC_DEBUG
		dbglog(DBG_DEBUG, "DS_ENC28J60: Can't detect rev: %02x\n", reg);
#endif
		return -1;
		//return 0;
	}
//#ifdef ENC_DEBUG	
	dbglog(DBG_INFO, "DS_ENC28J60: Detected rev: %02x\n", reg);
//#endif

	self->flags |= NETIF_DETECTED;
	return 0;
}

static int enc_if_init(netif_t * self) {
	if (self->flags & NETIF_INITIALIZED)
		return 0;

	//Initialise the device
	uint8 mac[6] = {'D', 'S', '4', 'N', 'E', 'T'};
	enc28j60_init(mac);
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Inited\n");
#endif
	memcpy(self->mac_addr, mac, 6);
	set_ipv6_lladdr();
	self->flags |= NETIF_INITIALIZED;
	return 0;
}

static int enc_if_shutdown(netif_t * self) {
	if (!(self->flags & NETIF_INITIALIZED))
		return 0;

	enc28j60_reset();
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Shutdown\n");
#endif
	
	self->flags &= ~(NETIF_DETECTED | NETIF_INITIALIZED | NETIF_RUNNING);
	return 0;
}

static int enc_if_start(netif_t * self) {
	if (!(self->flags & NETIF_INITIALIZED))
		return -1;

	self->flags |= NETIF_RUNNING;

	/* start receiving packets */
    //enc28j60_set_bits(ECON1, (1 << ECON1_RXEN));
	
	//enc28j60_write_phy(PHIE, PHIE_PGEIE | PHIE_PLNKIE);
	//enc28j60_clear_bits(EIR, EIR_DMAIF | EIR_LINKIF | EIR_TXIF | EIR_TXERIF | EIR_RXERIF | EIR_PKTIF);
	//enc28j60_write(EIE, EIE_INTIE | EIE_PKTIE | EIE_LINKIE | EIE_TXIE | EIE_TXERIE | EIE_RXERIE);
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Start\n");
#endif
	return 0;
}

static int enc_if_stop(netif_t * self) {
	if (!(self->flags & NETIF_RUNNING))
		return -1;

	self->flags &= ~NETIF_RUNNING;

	/* stop receiving packets */
	//enc28j60_write(EIE, 0x00);
    enc28j60_clear_bits(ECON1, (1 << ECON1_RXEN));
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Stop\n");
#endif

	return 0;
}

static int enc_if_tx(netif_t * self, const uint8 * data, int len, int blocking) {
	if (!(self->flags & NETIF_RUNNING))
		return -1;

#ifdef ENC_DEBUG
	dbglog(DBG_INFO, "DS_ENC28J60: Send packet ");
#endif

	if(!enc28j60_send_packet(data, len)) {
#ifdef ENC_DEBUG
		dbglog(DBG_DEBUG, "Error\n");
#endif
		return -1;
	}

#ifdef ENC_DEBUG	
	dbglog(DBG_DEBUG, "OK\n");
#endif

	return 0;
}

/* We'll auto-commit for now */
static int enc_if_tx_commit(netif_t * self) {
	return 0;
}

static int enc_if_rx_poll(netif_t * self) {

	uint len = 1514;
	uint8 pkt[1514];
	
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Receive packet ");
#endif
	
	len = enc28j60_receive_packet(pkt, len);
	
	if(!len) {
#ifdef ENC_DEBUG
		dbglog(DBG_DEBUG, "Error\n");
#endif
		return -1;
	}
	
#ifdef ENC_DEBUG
	dbglog(DBG_INFO, "OK\n");
#endif

	/* Submit it for processing */
	net_input(&enc_if, pkt, len);
	return 0;
}

/* Don't need to hook anything here yet */
static int enc_if_set_flags(netif_t * self, uint32 flags_and, uint32 flags_or) {
	self->flags = (self->flags & flags_and) | flags_or;
	return 0;
}

static int enc_if_set_mc(netif_t *self, const uint8 *list, int count) {
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Set multicast: %d\n", count);
#endif
	//return -1;
	if(count == 0) {
	    enc28j60_write(ERXFCON, (1 << ERXFCON_UCEN)  | /* accept unicast packets */
								(1 << ERXFCON_CRCEN) | /* accept packets with valid CRC only */
								(0 << ERXFCON_PMEN)  | /* no pattern matching */
								(0 << ERXFCON_MPEN)  | /* ignore magic packets */
								(0 << ERXFCON_HTEN)  | /* disable hash table filter */
								(0 << ERXFCON_MCEN)  | /* ignore multicast packets */
								(1 << ERXFCON_BCEN)  | /* accept broadcast packets */
								(0 << ERXFCON_ANDOR)   /* packets must meet at least one criteria */
		);
	} else {
	    enc28j60_write(ERXFCON, (1 << ERXFCON_UCEN)  | /* accept unicast packets */
								(1 << ERXFCON_CRCEN) | /* accept packets with valid CRC only */
								(0 << ERXFCON_PMEN)  | /* no pattern matching */
								(0 << ERXFCON_MPEN)  | /* ignore magic packets */
								(0 << ERXFCON_HTEN)  | /* disable hash table filter */
								(1 << ERXFCON_MCEN)  | /* accept multicast packets */
								(1 << ERXFCON_BCEN)  | /* accept broadcast packets */
								(0 << ERXFCON_ANDOR)   /* packets must meet at least one criteria */
		);
	}

    return 0;
}

/* Set ISP configuration from the flashrom, as long as we're configured staticly */
static void enc_set_ispcfg() {
	
	uint8 ip_addr[4] = {192, 168, 1, 110};
	uint8 netmask[4] = {255, 255, 255, 0};
	uint8 gateway[4] = {192, 168, 1, 1};
	uint8 broadcast[4] = {0, 0, 0, 0};
	
	memcpy(enc_if.ip_addr, &ip_addr, 4);
	memcpy(enc_if.netmask, &netmask, 4);
	memcpy(enc_if.gateway, &gateway, 4);
	memcpy(enc_if.broadcast, &broadcast, 4);
	
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "DS_ENC28J60: Default IP=%d.%d.%d.%d\n", enc_if.ip_addr[0], enc_if.ip_addr[1], enc_if.ip_addr[2], enc_if.ip_addr[3]);
#endif

	flashrom_ispcfg_t isp;
	uint32 fields = FLASHROM_ISP_IP | FLASHROM_ISP_NETMASK |
		FLASHROM_ISP_BROADCAST | FLASHROM_ISP_GATEWAY;

	if(flashrom_get_ispcfg(&isp) == -1)
		return;

	if((isp.valid_fields & fields) != fields)
		return;

	if(isp.method != FLASHROM_ISP_STATIC)
		return;
	
	
	memcpy(enc_if.ip_addr, isp.ip, 4);
	memcpy(enc_if.netmask, isp.nm, 4);
	memcpy(enc_if.gateway, isp.gw, 4);
	memcpy(enc_if.broadcast, isp.bc, 4);
	
#ifdef ENC_DEBUG
	dbglog(DBG_DEBUG, "enc: Setting up IP=%d.%d.%d.%d\n", enc_if.ip_addr[0], enc_if.ip_addr[1], enc_if.ip_addr[2], enc_if.ip_addr[3]);
#endif
}

/* Initialize */
int enc28j60_if_init(int cs, int rs) {
	
	if(cs > -1) {
		spi_cs = cs;
	}
	
	if(rs > -1) {
		spi_reset = rs;
	}
	
	/* Setup the netcore structure */
	enc_if.name = "enc";
	enc_if.descr = "ENC28J60";
	enc_if.index = 0;
	enc_if.dev_id = 0;
	enc_if.flags = NETIF_NO_FLAGS;
	memset(enc_if.ip_addr, 0, sizeof(enc_if.ip_addr));
	memset(enc_if.netmask, 0, sizeof(enc_if.netmask));
	memset(enc_if.gateway, 0, sizeof(enc_if.gateway));
	memset(enc_if.broadcast, 0, sizeof(enc_if.broadcast));
	memset(enc_if.dns, 0, sizeof(enc_if.dns));
	enc_if.mtu = 1500; /* The Ethernet v2 MTU */
	memset(&enc_if.ip6_lladdr, 0, sizeof(enc_if.ip6_lladdr));
	enc_if.ip6_addrs = NULL;
	enc_if.ip6_addr_count = 0;
	memset(&enc_if.ip6_gateway, 0, sizeof(enc_if.ip6_gateway));
	enc_if.mtu6 = 0;
	enc_if.hop_limit = 0;
	enc_if.if_detect = enc_if_detect;
	enc_if.if_init = enc_if_init;
	enc_if.if_shutdown = enc_if_shutdown;
	enc_if.if_start = enc_if_start;
	enc_if.if_stop = enc_if_stop;
	enc_if.if_tx = enc_if_tx;
	enc_if.if_tx_commit = enc_if_tx_commit;
	enc_if.if_rx_poll = enc_if_rx_poll;
	enc_if.if_set_flags = enc_if_set_flags;
	enc_if.if_set_mc = enc_if_set_mc;

	/* Attempt to set up our IP address et al from the flashrom */
	enc_set_ispcfg();

	/* Append it to the chain */
	return net_reg_device(&enc_if);
}

/* Shutdown */
int enc28j60_if_shutdown() {
	enc_if_shutdown(&enc_if);
	return 0;
}

