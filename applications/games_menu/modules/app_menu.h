/* DreamShell ##version##

   app_menu.h - Games app module
   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __APP_MENU_H
#define __APP_MENU_H

#include <ds.h>
#include <isoldr.h>
#include "app_utils.h"
#include "app_definition.h"
#include "tsunami/tsudefinition.h"
#include "tsunami/color.h"

#define GAMES_FOLDER "games"

#define IDE_PATH "/ide"
#define IDE_DS_PATH "/ide/DS"
#define IDE_GAMES_PATH "/ide/games"
#define IDE_COVERS_PATH "/ide/DS/apps/games_menu/covers"

#define SD_PATH "/sd"
#define SD_DS_PATH "/sd/DS"
#define SD_GAMES_PATH "/sd/games"
#define SD_COVERS_PATH "/sd/DS/apps/games_menu/covers"

#define CD_PATH "/cd"
#define CD_DS_PATH "/cd"
#define CD_GAMES_PATH "/cd/games"
#define CD_COVERS_PATH "/cd/apps/games_menu/covers"

#define MOTOSHORT(p) ((*(p)) << 8) + *(p + 1)
#define MAX_SIZE_CDDA 0
#define MAX_TRIGGER_VALUE 255
#define PLANE_TEXT_MENU_FOLDER ""
#define IMAGE_TEXT_64_5X2_MENU_FOLDER "/64"
#define IMAGE_128_4X3_FOLDER "/128"
#define IMAGE_TYPE_SUPPORTED (IT_PNG | IT_JPG | IT_PVR)
#define DEFAULT_RADIUS 2

typedef void SendMessageCallBack(const char *fmt, const char *message);
typedef void PostPVRCoverCallBack(bool new_cover);
typedef void PostOptimizerCoverCallBack();

struct MenuStructure
{
	bool rebuild_cache;
	bool started_with_cache;
	bool enable_cache;

	bool save_preset;
	bool cover_background;
	bool change_page_with_pad;
	bool start_in_last_game;	
	bool ide;
	bool sd;
	bool cd;
	bool rescan_covers;
	bool cover_to_pvr;
	bool convert_pvr_to_png;
	volatile bool finish_menu;
	volatile bool finished_menu;
	volatile bool cdda_game_changed;
	volatile bool stop_load_pvr_cover;
	volatile bool stop_optimize_game_cover;
	int default_cover_type;
	int state_app;
	int menu_type;
	int cache_array_count;
	int categories_array_count;
	int games_array_count;
	int games_category_array_count;
	int firmware_array_count;
	int current_dev;
	int vmu_mode;
	int last_device;
	int last_game_played_index;

	char default_dir[20];
	char games_config_path[100];
	char games_path[NAME_MAX];
	char covers_path[50];
	char last_game[NAME_MAX];
	char category[32];
	char theme[16];

	char default_dir_sd[20];
	char games_path_sd[NAME_MAX];
	char covers_path_sd[50];

	int games_array_ptr_count;
	GameItemStruct **games_array_ptr;

	Color background_color;
	Color border_color;
	Color title_color;
	Color body_color;
	Color area_color;

	Color control_top_color;
	Color control_body_color;
	Color control_bottom_color;

	PresetStruct *preset;

	AppConfigStruct app_config;
	CoverScannedStruct cover_scanned_app;
	MenuOptionStruct menu_option;
	GameItemStruct *games_array;
	GameItemStruct **games_category_array;
	FirmwareStruct *firmware_array;
	CategoryStruct *categories_array;
	CacheStruct *cache_array;

	SendMessageCallBack *send_message_scan;
	SendMessageCallBack *send_message_optimizer;
	PostPVRCoverCallBack *post_pvr_cover;
	PostOptimizerCoverCallBack *post_optimizer_cover;

	kthread_t *play_cdda_thread;
	kthread_t *load_pvr_cover_thread;
	kthread_t *optimize_game_cover_thread;
};

GameItemStruct *GetGamePtrByIndex(int game_index);
void SetGamesPath(const char* games_path);
const char* GetDeviceDir(uint8 device);
const char* GetDefaultDir(uint8 device);
const char* GetGamesPath(uint8 device);
const char* GetCoversPath(uint8 device);
const char* GetDefaultCoverName(int menu_type);
void CreateMenuData(SendMessageCallBack *send_message_scan, SendMessageCallBack *send_message_optimizer
	, PostPVRCoverCallBack *post_pvr_cover, PostOptimizerCoverCallBack *post_optimizer_cover);
void DestroyMenuData();
bool CheckGdiOptimized(int game_index);
const char *GetFullGamePathByIndex(int game_index);
void StopCDDA();
bool CheckCDDA(int game_index);
void* PlayCDDAThread(void *params);
void PlayCDDA(int game_index);
ImageDimensionStruct *GetImageDimension(const char *image_file);
bool IsValidImage(const char *image_file);
bool CheckCoverImageType(int game_index, int menu_type, uint16 image_type);
int16 CheckCover(int game_index, int menu_type);
bool GetCoverName(int game_index, char **cover_name);
const char* GetCoverFolder(int menu_type);
bool GetGameCoverPath(int game_index, char **game_cover_path, int menu_type);
void SetMenuType(int menu_type);
void FreeCache();
void FreeGames();
void FreeCategories();
int GenerateVMUFile(const char* full_path_game, int vmu_mode, uint32 vmu_number);
bool LoadFirmwareFiles();
void LoadDefaultMenuConfig();
bool LoadMenuConfig();
bool LoadCache();
bool SaveCache();
void PatchParseText(PresetStruct *preset);
PresetStruct* GetDefaultPresetGame(const char* full_path_game, SectorDataStruct *sector_data);
PresetStruct* LoadPresetGame(int game_index, bool default_preset);
bool SavePresetGame(PresetStruct *preset);
isoldr_info_t* ParsePresetToIsoldr(int game_index, PresetStruct *preset);
int IsHexadecimal(const char *text);
const char* GetNameCurrentTheme();
ThemeStruct GetTheme(const char *theme);
void SetTheme(const char *theme);
void ParseStringColor(uint32 *current_color, const char *color);
Color ParseUIntToColor(uint32 color);
void ParseMenuConfigToPresentation();
void ParsePresentationToMenuConfig();
bool SaveMenuConfig();
bool SaveScannedCover();
bool LoadScannedCover();
bool HasAnyCover();
void CleanIncompleteCover();
bool DecodeImage(const char *path, kos_img_t *img);
void OptimizeGameCovers();
void OptimizeCover(int game_index, const char *game_name, kos_img_t *img, bool is_alpha);
void CreateOptimizedCover(int game_index, const char *game_name, kos_img_t *img, bool is_alpha, int menu_type, int image_size);
bool ExtractPVRCover(int game_index);
void* LoadPVRCoverThread(void *params);
void* OptimizeCoverThread(void *param);
uint8 GetImageType(const char *filename);
void UpdateCover(CoverStruct *cover, int menu_type, uint8 device, uint8 image_type);
void SetLastGamePlayed();
void CreateCategories();
void AddGameToCache(int game_index);
void CopyDeepGame(const GameItemStruct *src, GameItemStruct *dest);
GameItemStruct* FindInCache(const char *game_path);
void RemoveStaleCache();
void PopulateCache();
void RetrieveGamesByCategory(const char *category_name);
void RetrieveCovers(uint8 device, int menu_type);
void RetrieveGamesRecursive();
bool RetrieveGames();
uint16 GetCoverType(int game_index, int menu_type);
const char* GetCoverExtensionFromType(uint16 image_type);
void CleanCoverType(int game_index, int menu_type);
void SetCoverType(int game_index, int menu_type, uint16 dw_image_type);
bool ContainsCoverType(int game_index, int menu_type, uint16 dw_image_type);

#endif // __APP_MENU_H
