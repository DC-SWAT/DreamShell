#ifndef __APP_MENU_H
#define __APP_MENU_H

#include <ds.h>
#include <isoldr.h>
#include "app_utils.h"
#include "app_definition.h"
#include "tsunami/tsudefinition.h"

#define MOTOSHORT(p) ((*(p)) << 8) + *(p + 1)
#define IN_CACHE_GAMES
#define MAX_SIZE_CDDA 0
#define MAX_TRIGGER_VALUE 255
#define PLANE_TEXT_MENU_FOLDER ""
#define IMAGE_TEXT_64_5X2_MENU_FOLDER "/64"
#define IMAGE_128_4X3_FOLDER "/128"
#define IMAGE_TYPE_SUPPORTED (IT_PNG | IT_JPG | IT_PVR)

typedef void SendMessageCallBack(const char *fmt, const char *message);
typedef void PostPVRCoverCallBack(bool new_cover);
typedef void PostOptimizerCoverCallBack();

struct menu_structure
{
	bool save_preset;
	bool cover_background;
	bool change_page_with_pad;
	bool start_in_last_game;	
	bool ide;
	bool sd;
	bool cd;
	bool rescan_covers;
	bool convert_pvr_to_png;
	volatile bool finish_menu;
	volatile bool finished_menu;
	volatile bool cdda_game_changed;
	volatile bool stop_load_pvr_cover;
	volatile bool stop_optimize_game_cover;
	int default_cover_type;
	int state_app;
	int menu_type;
	int games_array_count;
	int covers_array_count;
	int firmware_array_count;
	int current_dev;
	int vmu_mode;
	int last_device;
	int last_game_played_index;

	char default_dir[20];
	char games_path[NAME_MAX];
	char covers_path[50];
	char last_game[NAME_MAX];

	char default_dir_sd[20];
	char games_path_sd[NAME_MAX];
	char covers_path_sd[50];

	PresetStruct *preset;

	AppConfigStruct app_config;
	CoverScannedStruct cover_scanned_app;
	MenuOptionStruct menu_option;
	GameItemStruct *games_array;
	CoverStruct *covers_array;
	FirmwareStruct *firmware_array;

	SendMessageCallBack *send_message_scan;
	SendMessageCallBack *send_message_optimizer;
	PostPVRCoverCallBack *post_pvr_cover;
	PostOptimizerCoverCallBack *post_optimizer_cover;

	kthread_t *play_cdda_thread;
	kthread_t *load_pvr_cover_thread;
	kthread_t *optimize_game_cover_thread;
};

const char* GetDefaultDir(uint8 device);
const char* GetGamesPath(uint8 device);
const char* GetCoversPath(uint8 device);
void CreateMenuData(SendMessageCallBack *send_message_scan, SendMessageCallBack *send_message_optimizer
	, PostPVRCoverCallBack *post_pvr_cover, PostOptimizerCoverCallBack *post_optimizer_cover);
void DestroyMenuData();
bool CheckGdiOptimized();
const char *GetFullGamePathByIndex(int game_index);
void StopCDDA();
bool CheckCDDA(int game_index);
void* PlayCDDAThread(void *params);
void PlayCDDA(int game_index);
ImageDimensionStruct *GetImageDimension(const char *image_file);
bool IsValidImage(const char *image_file);
bool ExistsCoverFile(const char *cover_file, int menu_type);
bool CheckCoverImageType(int game_index, int menu_type, uint16 image_type);
int16 CheckCover(int game_index, int menu_type);
bool GetCoverName(int game_index, char **cover_name);
const char* GetCoverFolder(int menu_type);
bool GetGameCoverPath(int game_index, char **game_cover_path, int menu_type);
void SetMenuType(int menu_type);
void FreeGames();
void FreeGamesForce();
int GenerateVMUFile(const char* full_path_game, int vmu_mode, uint32 vmu_number);
bool LoadFirmwareFiles();
void LoadDefaultMenuConfig();
bool LoadMenuConfig();
void PatchParseText(PresetStruct *preset);
PresetStruct* GetDefaultPresetGame(const char* full_path_game, SectorDataStruct *sector_data);
PresetStruct* LoadPresetGame(int game_index);
bool SavePresetGame(PresetStruct *preset);
isoldr_info_t* ParsePresetToIsoldr(int game_index, PresetStruct *preset);
void ParseMenuConfigToPresentation();
void ParsePresentationToMenuConfig();
bool SaveMenuConfig();
bool SaveScannedCover();
bool LoadScannedCover();
bool HasAnyCover();
void CleanIncompleteCover();
void OptimizeGameCovers();
void OptimizeCover(int game_index, const char *game_name, kos_img_t *img, bool is_alpha);
bool ExtractPVRCover(int game_index);
void* LoadPVRCoverThread(void *params);
void* OptimizeCoverThread(void *param);
bool IsUniqueFileGame(const char *full_path_folder);
void RetrieveCovers(uint8 device, int menu_type);
void RetrieveGamesRecursive(const char *full_path_folder, const char *folder, int level);
bool RetrieveGames();
uint16 GetCoverType(int game_index, int menu_type);
const char* GetCoverExtensionFromType(uint16 image_type);
void CleanCoverType(int game_index, int menu_type);
void SetCoverType(int game_index, int menu_type, uint16 dw_image_type);
bool ContainsCoverType(int game_index, int menu_type, uint16 dw_image_type);

#endif // __APP_MENU_H
