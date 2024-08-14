#ifndef __APP_MENU_H
#define __APP_MENU_H

#include <ds.h>
#include "app_utils.h"
#include "app_definition.h"
#include "tsunami/tsudefinition.h"

#define MOTOSHORT(p) ((*(p)) << 8) + *(p + 1)
#define IN_CACHE_GAMES
#define MAX_SIZE_CDDA 0
#define MAX_TRIGGER_VALUE 255

typedef void SendMessageCallBack(const char *fmt, const char *message);
typedef void PostPVRCoverCallBack(bool new_cover);

struct menu_structure
{
	bool rescan_covers;
	bool convert_pvr_to_png;
	volatile bool finished_menu;
	volatile bool cdda_game_changed;
	volatile bool stop_load_pvr_cover;
	int menu_type;
	int games_array_count;
	int current_dev;
	char default_dir[20];
	char games_path[NAME_MAX];
	char covers_path[50];	

	CoverScannedStruct cover_scanned_app;
	MenuOptionStruct menu_option;
	GameItemStruct *games_array;

	SendMessageCallBack *send_message_scan;
	PostPVRCoverCallBack *post_pvr_cover;

	kthread_t *play_cdda_thread;
	kthread_t *load_pvr_cover_thread;
};

void CreateMenuData(SendMessageCallBack *send_message_scan, PostPVRCoverCallBack *post_pvr_cover);
void DestroyMenuData();
bool CheckGdiOptimized();
const char *GetFullGamePathByIndex(int game_index);
void StopCDDA();
bool CheckCDDA(int game_index);
void *PlayCDDAThread(void *params);
void PlayCDDA(int game_index);
ImageDimensionStruct *GetImageDimension(const char *image_file);
bool IsValidImage(const char *image_file);
int16 CheckCover(int game_index);
bool GetGameCoverPath(int game_index, char **game_cover_path);
void SetMenuType(int menu_type);
void FreeGames();
void FreeGamesForce();
bool SaveScannedCover();
bool LoadScannedCover();
void CleanIncompleteCover();
bool ExtractPVRCover(int game_index);
void *LoadPVRCoverThread(void *params);
bool IsUniqueFileGame(const char *full_path_folder);
void RetrieveGamesRecursive(const char *full_path_folder, const char *folder, int level);
bool RetrieveGames();

#endif // __APP_MENU_H
