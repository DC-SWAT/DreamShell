/* DreamShell ##version##

   utils.c - ISO Loader app utils
   Copyright (C) 2022-2025 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "app_utils.h"
#include "audio/wav.h"
#include "settings.h"

/* Trim begin/end spaces and copy into output buffer */
void trim_spaces(char *input, char *output, int size) {
	char *p;
	char *o;
	int s = 0;
	size--;

	p = input;
	o = output;

	if(*p == '\0') {
		*o = '\0';
		return;
	}

	while(*p == ' ' && size > 0) {
		p++;
		size--;
	}

	if(!size) {
		*o = '\0';
		return;
	}

	while(size--) { 
		*o++ = *p++;
		s++;
	}

	*o = '\0';
	o--;

	while(*o == ' ' && s > 0) {
		*o = '\0';
		o--;
		s--;
	}
}

char *trim_spaces2(char *txt)
{
	int32_t i;
	
	while(txt[0] == ' ')
	{
		txt++;
	}
	
	int32_t len = strlen(txt);
	
	for(i=len; i ; i--)
	{
		if(txt[i] > ' ') break;
		txt[i] = '\0';
	}
	
	return txt;
}

char *fix_spaces(char *str)
{
	if(!str) return NULL;
	
	int i, len = (int) strlen(str);
	
	for(i=0; i<len; i++)
	{
		if(str[i] == ' ') str[i] = '\\';
	}
	
	return str;
}

int conf_parse(isoldr_conf *cfg, const char *filename)
{
	file_t fd;
	int i;
	
	fd = fs_open(filename, O_RDONLY);
	
	if(fd == FILEHND_INVALID) 
	{
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}
	
	size_t size = fs_total(fd);
	
	char buf[512];
	char *optname = NULL, *value = NULL;
	
	if(fs_read(fd, buf, size) != size)
	{
		fs_close(fd);
		ds_printf("DS_ERROR: Can't read %s\n", filename);
		return -1;
	}
	
	fs_close(fd);
	
	while(1)
	{
		if(optname == NULL)
			optname = strtok(buf, "=");
		else
			optname = strtok('\0', "=");
		
		value = strtok('\0', "\n");
		
		if(optname == NULL || value == NULL) break;
		
		for(i=0; cfg[i].conf_type; i++)
		{
			if(strncasecmp(cfg[i].name, trim_spaces2(optname), 32)) continue;
			
			switch(cfg[i].conf_type)
			{
				case CONF_INT:
					*(int *)cfg[i].pointer = atoi(value);
					break;
					
				case CONF_STR:
					strcpy((char *)cfg[i].pointer, trim_spaces2(value));
					break;

				case CONF_ULONG:
					*(uint32 *)cfg[i].pointer = strtoul(value, NULL, 16);
			}
			break;
		}
	}
	
	return 0;
}

int getDeviceType(const char *dir) {

	if(!strncasecmp(dir, "/cd", 3)) {
		return APP_DEVICE_CD;
	} else if(!strncasecmp(dir, "/sd",   3)) {
		return APP_DEVICE_SD;
	} else if(!strncasecmp(dir, "/ide",  4)) {
		return APP_DEVICE_IDE;
	} else if(!strncasecmp(dir, "/pc",   3)) {
		return APP_DEVICE_PC;
//	} else if(!strncasecmp(dir, "/???", 5)) {
//		return APP_DEVICE_NET;
	} else {
		return -1;
	}
}

int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename) {
	memset(filepath, 0, NAME_MAX);
	snprintf(filepath, NAME_MAX, "%s/%s/%s.gdi", fmPath, dirname, filename);

	if(FileExists(filepath)) {
		memset(filepath, 0, NAME_MAX);
		snprintf(filepath, NAME_MAX, "%s/%s.gdi", dirname, filename);
		return 1;
	}
	
	return 0;
}

size_t GetCDDATrackFilename(int num, const char *fpath, const char *filename, char *result) {
	char path[NAME_MAX];
	int len = 0;
	int size = 0;

	if (strchr(filename, '/')) {
		len = strlen(strchr(filename, '/'));
	} else {
		len = strlen(filename);
	}

	memset(result, 0, NAME_MAX);
	memset(path, 0, NAME_MAX);
	strncpy(path, filename, strlen(filename) - len);
	snprintf(result, NAME_MAX, "%s/%s/track%02d.raw", fpath, path, num);

	size = FileSize(result);

	if (size > 0) {
		return size;
	}

	len = strlen(result);
	result[len - 3] = 'w';
	result[len - 1] = 'v';

	size = FileSize(result);
	return size;
}

static wav_stream_hnd_t wav_hnd = SND_STREAM_INVALID;
static int wav_inited = 0;

void StopCDDATrack() {
	if(wav_inited) {
		wav_shutdown();
	}
}

void PlayCDDATrack(const char *file, int loop) {

	StopCDDATrack();
	wav_inited = wav_init();

	if(wav_inited) {
		wav_hnd = wav_create(file, loop);

		if(wav_hnd == SND_STREAM_INVALID) {
			ds_printf("DS_ERROR: Can't play file: %s\n", file);
			return;
		}
		ds_printf("DS_OK: Start playing: %s\n", file);

		int volume = GetVolumeFromSettings();
		if(volume >= 0) {
			wav_volume(wav_hnd, volume);
		}
		wav_play(wav_hnd);
	}
}

static uint8 *romdisk_data[3] = {NULL, NULL, NULL};
static const char *mount_points[] = {"/presets_cd", "/presets_sd", "/presets_ide"};

int mountPresetsRomdisk(int device_type) {
	char romdisk_path[NAME_MAX];
	const char *romdisk_names[] = {"presets_cd.romfs", "presets_sd.romfs", "presets_ide.romfs"};

	if (device_type < 0 || device_type >= 3 || romdisk_data[device_type]) {
		return 0;
	}

	snprintf(romdisk_path, NAME_MAX, "%s/apps/iso_loader/resources/%s", 
		getenv("PATH"), romdisk_names[device_type]);

	if (fs_load(romdisk_path, (void**)&romdisk_data[device_type]) <= 0) {
		ds_printf("DS_ERROR: Failed to load romdisk %s\n", romdisk_path);
		return -1;
	}

	if (fs_romdisk_mount(mount_points[device_type], romdisk_data[device_type], 0) < 0) {
		ds_printf("DS_ERROR: Failed to mount romdisk %s\n", mount_points[device_type]);
		free(romdisk_data[device_type]);
		romdisk_data[device_type] = NULL;
		return -1;
	}

	return 0;
}

void unmountPresetsRomdisk(int device_type) {
	if (device_type < 0 || device_type >= 3 || !romdisk_data[device_type]) {
		return;
	}

	fs_romdisk_unmount(mount_points[device_type]);
	free(romdisk_data[device_type]);
	romdisk_data[device_type] = NULL;
}

void unmountAllPresetsRomdisks() {
	for (int i = 0; i < sizeof(mount_points) / sizeof(mount_points[0]); ++i) {
		unmountPresetsRomdisk(i);
	}
}

static void formatPresetFilename(const char *base_path, const char *dir, uint8 *md5, char *output) {
	char dev[8];

	memset(dev, 0, sizeof(dev));
	strncpy(dev, &dir[1], 3);
	if (dev[2] == '/') {
		dev[2] = '\0';
	}
	else {
		dev[3] = '\0';
	}

	snprintf(output, NAME_MAX,
		"%s/%s_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.cfg",
		base_path, dev, md5[0], md5[1], md5[2], md5[3], 
		md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], 
		md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
}

static char preset_filename[NAME_MAX];

char *makePresetFilename(const char *dir, uint8 *md5) {
	char base_path[NAME_MAX];

	memset(preset_filename, 0, sizeof(preset_filename));
	snprintf(base_path, NAME_MAX, "%s/apps/%s/presets", getenv("PATH"), lib_get_name() + 4);
	formatPresetFilename(base_path, dir, md5, preset_filename);
	return preset_filename;
}

char *findPresetFile(const char *dir, uint8 *md5) {
	int device_type;
	char *preset_file = makePresetFilename(dir, md5);

	if (FileSize(preset_file) > 0) {
		return preset_file;
	}

	device_type = getDeviceType(dir);
	if (device_type < 0 || device_type >= 3) {
		return NULL;
	}

	if (mountPresetsRomdisk(device_type) < 0) {
		return NULL;
	}

	formatPresetFilename(mount_points[device_type], dir, md5, preset_filename);

	if (FileSize(preset_filename) > 0) {
		return preset_filename;
	}

	return NULL;
}
