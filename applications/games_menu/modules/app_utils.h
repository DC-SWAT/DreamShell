/* DreamShell ##version##

   utils.c - app utils
   Copyright (C) 2022-2024 SWAT
   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __APP_UTILS_H
#define __APP_UTILS_H

#include "app_definition.h"

#define TO_UPPER_SAFE(c) ((c) == 'a' ? 'A' : toupper((unsigned char)(c)))

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

char *StrdupSafe(const char *string);
const char* GetFileName(const char* path);
bool EndsWith(const char *filename, const char *ext);
void TrimSlashes(char *path);
char *Trim(char *string);
void TrimSpaces(char *input, char *output, int size);
char *TrimSpaces2(char *txt);
char *FixSpaces(char *str);
bool MakeShortcut(PresetStruct *preset, const char* device_dir, const char* full_path_game, bool show_name, const char* game_cover_path, int width, int height, bool yflip);
int ConfigParse(isoldr_conf *cfg, const char *filename);
bool IsGdiOptimized(const char *full_path_game);
void GoUpDirectory(const char *original_path, int levels, char *result);
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
int MountPresetsRomdisk(int device_type);
void UnmountPresetsRomdisk(int device_type);
void UnmountAllPresetsRomdisks();

#endif // __APP_UTILS_H
