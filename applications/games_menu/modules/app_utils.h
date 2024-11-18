/* DreamShell ##version##

   utils.h - ISO Loader app utils
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __APP_UTILS_H
#define __APP_UTILS_H

#include "app_definition.h"

/* Indexes of devices  */
enum {
	APP_DEVICE_CD = 0,
	APP_DEVICE_SD = 1,
	APP_DEVICE_IDE,
	APP_DEVICE_PC,
	APP_DEVICE_COUNT
};

enum {
	CONF_END = 0,
	CONF_INT = 1,
	CONF_STR,
	CONF_ULONG,
};



char *Trim(char *string);
void TrimSpaces(char *input, char *output, int size);
char *TrimSpaces2(char *txt);
char *FixSpaces(char *str);
bool MakeShortcut(PresetStruct *preset, const char* device_dir, const char* full_path_game, bool show_name, const char* game_cover_path, int width, int height, bool yflip);
int ConfigParse(isoldr_conf *cfg, const char *filename);
bool IsGdiOptimized(const char *full_path_game);
const char* GetLastPart(const char *source, const char separator, int option_path);
bool ContainsOnlyNumbers(const char *string);
int  GetDeviceType(const char *dir);
int CanUseTrueAsyncDMA(int sector_size, int current_dev, int image_type);
void GetMD5HashISO(const char *file_mount_point, SectorDataStruct *sector_data);
char* MakePresetFilename(const char *default_dir, const char *device_dir, uint8 *md5, const char *app_name);
const char *GetFolderPathFromFile(const char *full_path_file);
size_t GetCDDATrackFilename(int num, const char *full_path_game, char **result);
void PlayCDDATrack(const char *file, int loop);
void StopCDDATrack();

#endif // __APP_UTILS_H
