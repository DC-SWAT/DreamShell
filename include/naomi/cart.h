/** \file    naomi/cart.h
    \brief   NAOMI cartridge information.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2025 SWAT
*/

#ifndef __NAOMI_CART_H
#define __NAOMI_CART_H

#include <sys/types.h>

/** \brief   NAOMI ROM load entry. */
typedef struct naomi_load_entry {
    uint32_t offset;
    void    *dst_buf;
    uint32_t size;
} naomi_load_entry_t;

/** \brief   NAOMI cartridge header. */
typedef struct naomi_cart_header {
    char system_name[16];           /**< \brief "NAOMI" space padded */
    char publisher[32];             /**< \brief "SEGA ENTERPRISES,LTD." space padded */
    char regional_name[8][32];      /**< \brief Space padded */

    uint16_t year;                  /**< \brief Manufacture date: year */
    uint8_t  month;                 /**< \brief Manufacture date: month */
    uint8_t  day;                   /**< \brief Manufacture date: day */
    char serial[4];                 /**< \brief Serial number */

    /** \brief  8MB ROM mode flag.

        Non-zero value specifies that ROM board offsets should be OR'd with
        0x20000000.
    */
    uint16_t rom_mode_flag;

    /** \brief  G1 BUS initialization flag.

        Non-zero value specifies that the below G1 BUS register values should
        be used.
    */
    uint16_t g1bus_init_flag;

    uint32_t SB_G1RRC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1RWC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1FRC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1FWC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1CRC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1CWC_val;          /**< \brief G1 BUS register value */
    uint32_t SB_G1GDRC_val;         /**< \brief G1 BUS register value */
    uint32_t SB_G1GDWC_val;         /**< \brief G1 BUS register value */

    uint8_t ROM_checksums[132];     /**< \brief 132 bytes of M2/M4-type ROM checksums */
    uint8_t EEPROM_init_val[128];   /**< \brief EEPROM initial values */

    char credits_str[8][32];        /**< \brief Credits strings */

    naomi_load_entry_t game_exe[8];  /**< \brief Game executable entries */
    naomi_load_entry_t test_exe[8]; /**< \brief Test mode executable entries */

    uint32_t game_execute_adr;      /**< \brief Jump to this address to run game */
    uint32_t test_execute_adr;      /**< \brief Jump to this address to run test mode */

    /** \brief  Region mask.

        Mask of supported regions: bit 0 - Japan, bit 1 - USA, bit 2 - Export,
        bit 3 - Korea, bit 4 - Australia.
    */
    uint8_t region;

    /** \brief  Player count mask.

        Mask of supported player numbers: 0 - any; bit 0 - 1 player,
        bit 1 - 2 players, bit 2 - 3 players, bit 3 - 4 players.
    */
    uint8_t players;

    /** \brief  Video mode mask.

        Mask of supported video modes: 0 - any; bit 0 - 31khz, bit 1 - 15khz.
    */
    uint8_t vid_mode;

    /** \brief  Display orientation mask.

        0 - any; bit 0 - horizontal, bit 1 - vertical.
    */
    uint8_t disp_orientation;

    /** \brief  Serial number check flag.

        Flag to check ROM/DIMM board serial number EEPROM. The BIOS checks the
        EEPROM if this set to 1.
    */
    uint8_t check_serial;

    /** \brief  Service type.

        A value of 0 means common and a value of 1 means individual.
    */
    uint8_t service_type;

    uint8_t ROM_checksums2[144];    /**< \brief 144 bytes of M1-type ROM checksums */
    uint8_t padding[71];            /**< \brief Padding */

    /** \brief  Encryption flag.

        If this is 0xFF then the header is unencrypted. If it is not 0xFF then
        the header is encrypted starting at offset 0x010.
    */
    uint8_t encryption;
} naomi_cart_header_t;

#endif  /* __NAOMI_CART_H */
