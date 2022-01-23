/* DreamShell ##version##

   utils.h - ISO Loader app utils
   Copyright (C) 2022 SWAT

*/

/* Indexes of devices  */
#define APP_DEVICE_CD    0
#define APP_DEVICE_SD    1
#define APP_DEVICE_IDE   2
#define APP_DEVICE_PC    3
//#define APP_DEVICE_NET   4
#define APP_DEVICE_COUNT 4

#define CONF_END 0
#define CONF_INT 1
#define CONF_STR 2

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
