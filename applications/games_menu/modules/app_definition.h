/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __APP_DEFINITION_H
#define __APP_DEFINITION_H

#define MAX_SIZE_ITEMS 12
#define MAX_BUTTONS 7
#define MAX_SCAN_COUNT 2
#define MAX_MENU 3
#define FIRMWARE_SIZE 8
#define MAX_VMU 999
#define DEFAULT_VMU_NUMBER "001"
#define SCREENSHOT_HOTKEY (CONT_START | CONT_A | CONT_B)
#define ALT_BOOT_FILE "2ND_READ.BIN"

#include <stdbool.h>

enum PresetControlEnum
{
	SAVE_CONTROL_ID = 1,
	DMA_CONTROL_ID,
	ASYNC_CONTROL_ID,
	BYPASS_CONTROL_ID,
	IRQ_CONTROL_ID,
	OS_CONTROL_ID,
	LOADER_CONTROL_ID,
	BOOT_CONTROL_ID,
	FASTBOOT_CONTROL_ID,
	LOWLEVEL_CONTROL_ID,
	MEMORY_CONTROL_ID,
	CUSTOM_MEMORY_CONTROL_ID,
	HEAP_CONTROL_ID,

	CDDA_CONTROL_ID,
	CDDA_SOURCE_CONTROL_ID,
	CDDA_DESTINATION_CONTROL_ID,
	CDDA_POSITION_CONTROL_ID,
	CDDA_CHANNEL_CONTROL_ID,

	PATCH_ADDRESS1_CONTROL_ID,
	PATCH_VALUE1_CONTROL_ID,
	PATCH_ADDRESS2_CONTROL_ID,
	PATCH_VALUE2_CONTROL_ID,

	ALTERBOOT_CONTROL_ID,
	SCREENSHOT_CONTROL_ID,
	VMU_CONTROL_ID,
	VMUSELECTOR_CONTROL_ID,

	SHORTCUT_SIZE_CONTROL_ID,
	SHORTCUT_ROTATE_CONTROL_ID,
	SHORTCUT_NAME_CONTROL_ID,
	SHORTCUT_DONTSHOWNAME_CONTROL_ID,
	SHORTCUT_CREATE_CONTROL_ID,
	SHORTCUT_COVER_CONTROL_ID,
};

enum StateAppEnum
{
	SA_NONE = 0,
	SA_GAMES_MENU,
	SA_SYSTEM_MENU,
	SA_PRESET_MENU,
	SA_SCAN_COVER,
	SA_OPTIMIZE_COVER,
	SA_CONTROL
};

enum DirectionMenuDisplacementEnum
{
	DMD_NONE = 0,
	DMD_LEFT = 1,
	DMD_RIGHT = 2,
	DMD_UP = 3,
	DMD_DOWN = 4
};

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

typedef struct FirmwareStructure
{
	char file[FIRMWARE_SIZE+1];
} FirmwareStruct;

typedef struct PresetStructure
{
	int game_index;
	char preset_file_name[50];

	int use_dma;
	int emu_async;
	int use_irq;
	int alt_read;
	int fastboot;
	int low;	
	int scr_hotkey;
	int boot_mode;
	int bin_type;
	int alt_boot;
	int screenshot;
	int cdda;
	int rotate_image;
	int dont_show_name;	
	int icon_size;
	int vmu_mode;
	
	uint32 emu_vmu;
	uint32 emu_cdda;
	uint32 heap;
	uint32 pa[2];
	uint32 pv[2];

	char title[32];
	char device[FIRMWARE_SIZE+1];	
	char memory[12];
	char custom_memory[12];
	char heap_memory[12];
	char bin_file[12];
	char patch_a[2][10];
	char patch_v[2][10];
	char vmu_file[32];
	char shortcut_name[33];
} PresetStruct;

typedef struct SectorDataStructure
{
	int image_type;
	int sector_size;
	uint8 md5[16];
	uint8 boot_sector[2048];
	uint32 *addr;
} SectorDataStruct;

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
	uint8 device;
	char *game;
	char *folder;
	char *folder_name;
	bool is_folder_name;
	int16 exists_cover[MAX_MENU];
	uint64 cover_type;
	bool check_pvr;
	bool is_pvr_cover;
	bool check_optimized;
	bool is_gdi_optimized;
	int16 is_cdda;

} GameItemStruct;

typedef struct CoverStructure
{
	char *cover;
	uint8 device;
	uint8 menu_type;
	uint8 image_type[MAX_MENU];

} CoverStruct;

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
	uint32 scan_count;
} CoverScannedStruct;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct AppConfigStructure
{
	int initial_view;
	int save_preset;
	int cover_background;
} AppConfigStruct;
#pragma pack(pop)

typedef struct isoldr_conf_structure
{
	const char *name;
	int conf_type;
	void *pointer;
} isoldr_conf;

typedef isoldr_conf GenericConfigStruct;

#endif //__APP_DEFINITION_H
