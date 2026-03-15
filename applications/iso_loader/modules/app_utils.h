/* DreamShell ##version##

   utils.h - ISO Loader app utils
   Copyright (C) 2022-2026 SWAT

*/

/* Indexes of devices  */
enum {
	APP_DEVICE_CD = 0,
	APP_DEVICE_SD = 1,
	APP_DEVICE_IDE,
	APP_DEVICE_PC,
	APP_DEVICE_COUNT
};

void trim_spaces(char *input, char *output, int size);
char *trim_spaces2(char *txt);
char *fix_spaces(char *str);

int getDeviceType(const char *dir);
void getFirstPathComponent(const char *path, char *result);
void makeGameRelativePath(char *dst, size_t dst_size, const char *base_path, const char *game_path, const char *filename);
int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename);

size_t GetCDDATrackFilename(int num, const char *fpath, const char *filename, char *result);
void PlayCDDATrack(const char *file, int loop);
void StopCDDATrack();
void PauseCDDATrack();
void ResumeCDDATrack();
int IsCDDATrackPlaying();
void SetCDDAVolume(int vol);

char *lib_get_name();
uint32_t lib_get_version();
