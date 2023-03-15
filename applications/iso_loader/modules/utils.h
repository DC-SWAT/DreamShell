/* DreamShell ##version##

   utils.h - ISO Loader app utils
   Copyright (C) 2022-2023 SWAT

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
	CONF_STR
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

int getDeviceType(const char *dir);
int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename);
char *makePresetFilename(const char *dir, uint8 *md5);

char *lib_get_name();
uint32 lib_get_version();
