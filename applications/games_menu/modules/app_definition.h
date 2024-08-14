/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __APP_DEFINITION_H
#define __APP_DEFINITION_H
#define MAX_SIZE_ITEMS 12

#include <stdbool.h>

enum SearchCoverEnum
{
    SC_WITHOUT_SEARCHING = -1,
    SC_NOT_EXIST = 0,
    SC_EXISTS = 1,
    SC_DEFAULT = 2
};

enum CoverStatusEnum
{
    CSE_EXISTS = 1,
    CSE_PROCESSING = 2,
    CSE_COMPLETED = 3
};

enum CheckCDDAGameEnum
{
    CCGE_NOT_CHECKED = -1,
    CCGE_CDDA = 1,
    CCGE_CDDA_BIG_SIZE = 2,
    CCGE_NOT_CDDA = 3 
};

typedef struct MenuOptionStructure
{
    int max_page_size;
    int max_columns;
    int size_items_column;
    int init_position_x;
    int init_position_y;
    int padding_x;
    int padding_y;
    float image_size;
} MenuOptionStruct;

typedef struct GameItemStructure
{
    char *game;
    char *folder;
    bool is_folder_name;
    int16 exists_cover;
    uint8 cover_type;
    bool check_pvr;
    bool is_pvr_cover;
    bool is_gdi_optimized;
    int16 is_cdda;

} GameItemStruct;

typedef struct ImageDimensionStructure
{
	unsigned int width;
	unsigned int height;
} ImageDimensionStruct;

#pragma pack(push, 4)
typedef struct CoverScannedStructure
{
    char last_game_scanned[NAME_MAX];
    uint32 last_game_index;
    uint32 last_game_status;
    uint32 games_count;
} CoverScannedStruct;
#pragma pack(pop)

#endif //__APP_DEFINITION_H
