/**
 * DreamShell ISO Loader
 * VMU
 * (c)2023 SWAT <http://www.dc-swat.ru>
 */

#include <sys/cdefs.h>
#include <arch/types.h>

#ifndef __VMU_H
#define __VMU_H

#define MAKE_STORAGE_FUNC_DATA(partition, blocksize, wp_cnt, rp_cnt, removable, crc_req) ( \
    (partition) << 24 | (((wp_cnt & 0xf) << 4) | (rp_cnt & 0xf)) << 16 \
    | ((blocksize / 32) - 1) << 8 | ((removable & 1) << 7) | ((crc_req & 1) << 6) \
)

#define STORAGE_FUNC_READ_PHASE_COUNT(d) (uint8)(d.function_data[2] >> 16 & 0x0f)
#define STORAGE_FUNC_WRITE_PHASE_COUNT(d) (uint8)(d.function_data[2] >> 20 & 0x0f)
#define STORAGE_FUNC_BLOCK_SIZE(d) (uint16)(((d.function_data[2] >> 8 & 0xff) + 1) * 32)
#define STORAGE_FUNC_PARTITION(d) (uint8)(d.function_data[2] >> 24 & 0xff)

#define MAKE_LCD_FUNC_DATA(partition, blocksize, wcnt, normally) ( \
    (partition) << 24 | ((wcnt & 0xf) << 4) << 16 \
    | ((blocksize / 32) - 1) << 8 | ((normally & 1) << 5) \
)

#define MAKE_CLOCK_FUNC_DATA(wtm, rtm, button, alarm) ( \
    (wtm << 24) | (rtm << 16) | (button << 8) | alarm \
)

/* (last_block + 1) * block_size = full capacity in bytes. */
#define STORAGE_MEMORY_LAST_BLOCK_DEFAULT 0x00ff
/* KATANA SDK support only 128/256/512/1024 KB with fixed block size (512). */
#define STORAGE_MEMORY_LAST_BLOCK_MAX 0x07ff
/* File info size in blocks for 128 KB. */
#define STORAGE_MEMORY_FILEINFO_BLOCKS_DEFAULT 13
/* Block offset for saves. */
#define STORAGE_MEMORY_SAVE_BLOCK(last_block) ( \
    (last_block + 1) - ((STORAGE_MEMORY_FILEINFO_BLOCKS_DEFAULT * \
    ((last_block + 1) / (STORAGE_MEMORY_LAST_BLOCK_DEFAULT + 1))) + 2) \
)
/* Block offset for game executable. */
#define STORAGE_MEMORY_EXE_BLOCK(last_block) ((last_block + 1) / 2)

typedef struct {
    uint32 function;
    uint16 last_block;
    uint16 partition;
    uint16 sys_block;
    uint16 fat_block;
    uint16 fat_cnt;
    uint16 file_info_block;
    uint16 file_info_cnt;
    uint8  vol_icon;
    uint8  sort_flag;
    uint16 save_block;
    uint16 save_cnt;
    uint16 exe_block;
    uint16 exe_cnt;
} maple_memory_t;


#endif
