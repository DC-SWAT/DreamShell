/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __APP_DEFINITION_H
#define __APP_DEFINITION_H
#define MAX_SIZE_ITEMS 12
#define MAX_SIZE_GAME_NAME 80

#include <stdbool.h>

enum SearchCoverEnum
{
    SC_WITHOUT_SEARCHING = -1,
    SC_NOT_EXIST = 0,
    SC_EXISTS = 1,
    SC_DEFAULT = 2
};

// #pragma pack(push, 1)
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
// #pragma pack(pop)

typedef struct GameItemStructure
{
    char game[MAX_SIZE_GAME_NAME];
    char folder[MAX_SIZE_GAME_NAME];
    bool is_folder_name;
    int8 exists_cover;
    uint8 cover_type;

} GameItemStruct;

typedef struct ImageDimensionStructure
{
	unsigned int width;
	unsigned int height;
} ImageDimensionStruct;

#endif //__APP_DEFINITION_H