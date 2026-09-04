/** \file    flashrom.h
    \brief   Flashrom factory block info

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT

    Thanks to MetalliC for the information.
*/

 #ifndef __DC_FLASHROM_H
 #define __DC_FLASHROM_H
 
 #include <sys/types.h>

 typedef struct flashrom_factory_record {
    // everything 'char' below is decimal numbers in ASCII, unless noted else
    /* '0' - Dreamcast, 0xFF - dev.box */
    char machine_code1;
    char machine_code2;
    /* 0 - Japan, 1 - America, 2 - Europe */
    char country_code;
    /* 0 - Japanese, 1 - English, etc */
    char language;
    /* 0 - NTSC, 1 - PAL, 2 - PAL-M, 3 - PAL-N */
    char broadcast_format;
    /* ASCII text 'Dreamcast', trail is 0x20 filled */
    char machine_name[32];
    /* software tool # */
    char tool_number[4];
    /* software tool version */
    char tool_version[2];
    /* software tool type: 0 - for MP(mass production?), 1 - for Repair, 2 - for PP */
    char tool_type[2];
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char min[2];
    char serial_number[8];
    char factory_code[4];
    char total_number[16];
    /* byte sum of above */
    uint8_t sum;
    /* 64bit UID */
    uint8_t machine_id[8];
    /* FF - Dreamcast */
    uint8_t machine_type;
    /* FF - VA0, FE - VA1, FD - VA2,
       NOTE: present in 1st factory record only, in 2nd always FF */
    uint8_t machine_version;
    /* FF filled */
    uint8_t reserved[0x40];
} flashrom_factory_record_t;

typedef struct flashrom_factory_info {
    /* 2 copies */
    flashrom_factory_record_t records[2];
    /* FF filled */ 
    uint8_t reserved_0[0x36];
    /* not clear if hardware or bios version, A0 - VA0, 9F - VA1, 9E - VA2 */
    uint8_t unknown_version;
    /* FF filled */
    uint8_t reserved_1[9];
    /* list of creators */
    char staff_roll[0xca0];
    /* FF filled */
    uint8_t reserved_2[0x420];
    /* output of RNG {static u32 seed; seed = (seed * 0x83d + 0x2439) & 0x7fff;
       return (u16)(seed + 0xc000);}, where initial seed value is serial_number[7] & 0xf */
    uint8_t random[0xdc0];
} flashrom_factory_info_t;

#endif	/* __DC_FLASHROM_H */
