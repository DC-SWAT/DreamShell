/* DreamShell ##version##

   utils.h - ISO Loader app utils
   Copyright (C) 2024 Maniac Vera

*/

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

typedef struct {
	const char *name;
	int conf_type;
	void *pointer;
} isoldr_conf;

void trim_spaces(char *input, char *output, int size);
char *trim_spaces2(char *txt);
char *fix_spaces(char *str);
int conf_parse(isoldr_conf *cfg, const char *filename);

int  GetDeviceType(const char *dir);
bool ReadBootSector(const char *track_file, uint8* bootSector);
// int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename);
char *MakePresetFilename(const char *dir, uint8 *md5);

size_t GetCDDATrackFilename(int num, const char *fpath, const char *filename, char *result);
void PlayCDDATrack(const char *file, int loop);
void StopCDDATrack();

// char *lib_get_name();
// uint32 lib_get_version();
