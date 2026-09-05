/** \file    naomi/cart.h
    \brief   NAOMI cartridge and DIMM-slot ROM access.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2025-2026 SWAT
*/

#ifndef __NAOMI_CART_H
#define __NAOMI_CART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <kos/regfield.h>

#define NAOMI_G1_BASE                    0xA05F7000

#define NAOMI_CART_ROM_OFFSETH           0xA05F7000
#define NAOMI_CART_ROM_OFFSETL           0xA05F7004
#define NAOMI_CART_ROM_DATA              0xA05F7008
#define NAOMI_CART_DMA_OFFSETH           0xA05F700C
#define NAOMI_CART_DMA_OFFSETL           0xA05F7010
#define NAOMI_CART_DMA_COUNT             0xA05F7014
#define NAOMI_CART_M4_ID                 0xA05F7034
#define NAOMI_CART_DIMM_COMMAND          0xA05F703C
#define NAOMI_CART_DIMM_OFFSETL          0xA05F7040
#define NAOMI_CART_DIMM_PARAMETERL       0xA05F7044
#define NAOMI_CART_DIMM_PARAMETERH       0xA05F7048
#define NAOMI_CART_DIMM_STATUS           0xA05F704C
#define NAOMI_CART_LED                   0xA05F7068
#define NAOMI_CART_BOARDID_WRITE         0xA05F7078
#define NAOMI_CART_BOARDID_READ          0xA05F707C

#define NAOMI_COMM_CTRL                  0xA05F7018
#define NAOMI_COMM_OFFSET                0xA05F701C
#define NAOMI_COMM_DATA                  0xA05F7020
#define NAOMI_COMM_STATUS1               0xA05F7024
#define NAOMI_COMM_STATUS2               0xA05F7028

#define NAOMI_MBOARD_OFFSET              0xA05F7050
#define NAOMI_MBOARD_DATA                0xA05F7054
#define NAOMI_MBOARD_DMA_OFFSET          0xA05F7058
#define NAOMI_MBOARD_STATUS              0xA05F705C
#define NAOMI_MBOARD_BOARD_ID            0xA05F706C
#define NAOMI_MBOARD_UART_IDX            0xA05F7070
#define NAOMI_MBOARD_UART_DATA           0xA05F7074
#define NAOMI_MBOARD_CONFIG_SLOT         0xA05F70C4

typedef enum naomi_region {
    NAOMI_REGION_JAPAN = 0,
    NAOMI_REGION_USA = 1,
    NAOMI_REGION_EXPORT = 2,
    NAOMI_REGION_KOREA = 3,
    NAOMI_REGION_AUSTRALIA = 4,
} naomi_region_t;

typedef enum naomi_cart_type {
    NAOMI_CART_NONE = 0,
    NAOMI_CART_M1,
    NAOMI_CART_M2,
    NAOMI_CART_M4,
    NAOMI_CART_DIMM,
} naomi_cart_type_t;

/** \brief   NAOMI ROM load entry. */
typedef struct naomi_load_entry {
    uint32_t offset;
    void    *dst_buf;
    uint32_t size;
} naomi_load_entry_t;

#define NAOMI_EEPROM_GAME_ID_OFFSET  0x134
#define NAOMI_EEPROM_INIT_OFFSET     0x1e0
#define NAOMI_CART_HEADER_SIZE       0x500
#define NAOMI_CART_SIZE_MAX          0x20000000
#define NAOMI_CART_DMA_UNIT          0x20
#define NAOMI_CART_PROBE_STEP        0x200000
#define NAOMI_CART_HDR_SCAN_MAX      0x04000000

/** \brief  Bits in ROM_OFFSET / DMA_OFFSET.

    These are not a separate register: the cart address is 32-bit and the top
    bits select access mode.

    - AUTO: increment address after each PIO/DMA beat
    - CRYPT: M4 decrypt / M2 crypto device (ROM_OFFSET). Same bit on DMA_OFFSET
      holds the DMA engine so PIO can run
    - 8MB: linear 8MB ROM mapping (needed for a raw dump; M1 DMA decrypts if clear)
    - SLAVE: stacked slave ROM board
*/
#define NAOMI_CART_ADDR_AUTO         BIT(31)
#define NAOMI_CART_ADDR_CRYPT        BIT(30)
#define NAOMI_CART_ADDR_8MB          BIT(29)
#define NAOMI_CART_ADDR_SLAVE        BIT(28)
#define NAOMI_CART_ADDR_MASK         GENMASK(27, 0)

#define NAOMI_CART_READ_DEFAULT      (NAOMI_CART_ADDR_AUTO | NAOMI_CART_ADDR_8MB)

static inline uint32_t naomi_cart_dump_flags(naomi_cart_type_t type) {
    uint32_t flags = NAOMI_CART_READ_DEFAULT;

    if(type == NAOMI_CART_M4) {
        flags |= NAOMI_CART_ADDR_CRYPT;
    }
    if(type == NAOMI_CART_DIMM) {
        flags = NAOMI_CART_ADDR_AUTO;
    }
    return flags;
}

typedef struct naomi_cart_cfi {
    uint16_t m4_id;
    uint8_t  size_exp;
    uint8_t  chips;
    size_t   chip_size;
    size_t   total_size;
} naomi_cart_cfi_t;

/** \brief   NAOMI cartridge header. */
typedef struct naomi_cart_header {
    char system_name[16];           /**< \brief "NAOMI" or "Naomi2" space padded */
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
    uint8_t padding[65];            /**< \brief Padding */

    /** \brief  Encryption flag.

        If this is 0xFF then the header is unencrypted. If it is not 0xFF then
        the header is encrypted starting at offset 0x010.
    */
    uint8_t encryption;
} naomi_cart_header_t;

_Static_assert(sizeof(naomi_cart_header_t) == NAOMI_CART_HEADER_SIZE,
    "naomi_cart_header_t must be 0x500 bytes");

static inline bool naomi_cart_valid(const naomi_cart_header_t *hdr) {
    return (hdr->system_name[0] == 'N' || hdr->system_name[0] == 'S')
        && hdr->system_name[15] == ' ';
}

static inline bool naomi_cart_header_encrypted(const naomi_cart_header_t *hdr) {
    return hdr->encryption != 0xFF;
}

#ifndef _ISO_LOADER_H
size_t naomi_cart_pio_read(uint32_t offset, void *dst, size_t len, uint32_t flags);
size_t naomi_cart_dma_read(uint32_t offset, void *dst, size_t len, uint32_t flags);
size_t naomi_cart_read_ex(uint32_t offset, void *dst, size_t len, uint32_t flags);
size_t naomi_cart_read(uint32_t offset, void *dst, size_t len);

bool naomi_cart_read_header(naomi_cart_header_t *hdr);
bool naomi_cart_dimm_present(void);
uint16_t naomi_cart_m4_id(void);
uint16_t naomi_cart_m1_id(void);
naomi_cart_type_t naomi_cart_type(void);

size_t naomi_cart_probe_size(void);
void naomi_cart_apply_g1_timing(const naomi_cart_header_t *hdr);

bool naomi_cart_cfi_query(naomi_cart_cfi_t *out);
#endif

#endif  /* __NAOMI_CART_H */
