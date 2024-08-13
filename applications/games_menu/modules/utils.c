/* DreamShell ##version##

   utils.c - ISO Loader app utils
   Copyright (C) 2022-2024 SWAT
   Copyright (C) 2024 Maniac Vera

*/

#include "ds.h"
#include "app_utils.h"
#include "audio/wav.h"

/* Trim begin/end spaces and copy into output buffer */
void trim_spaces(char *input, char *output, int size)
{
	char *p;
	char *o;
	int s = 0;

	p = input;
	o = output;

	if (*p == '\0')
	{
		*o = '\0';
		return;
	}

	while (*p == ' ' && s > 0)
	{
		p++;
		size--;
	}

	if (!size)
	{
		*o = '\0';
		return;
	}

	while (size--)
	{
		*o++ = *p++;
		s++;
	}

	*o = '\0';
	o--;

	while (*o == ' ' && s > 0)
	{
		*o = '\0';
		o--;
		s--;
	}
}

char *trim_spaces2(char *txt)
{
	int32_t i;

	while (txt[0] == ' ')
	{
		txt++;
	}

	int32_t len = strlen(txt);

	for (i = len; i; i--)
	{
		if (txt[i] > ' ')
			break;
		txt[i] = '\0';
	}

	return txt;
}

char *fix_spaces(char *str)
{
	if (!str)
		return NULL;

	int i, len = (int)strlen(str);

	for (i = 0; i < len; i++)
	{
		if (str[i] == ' ')
			str[i] = '\\';
	}

	return str;
}

int conf_parse(isoldr_conf *cfg, const char *filename)
{
	file_t fd;
	int i;

	fd = fs_open(filename, O_RDONLY);

	if (fd == FILEHND_INVALID)
	{
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}

	size_t size = fs_total(fd);

	char buf[512];
	char *optname = NULL, *value = NULL;

	if (fs_read(fd, buf, size) != size)
	{
		fs_close(fd);
		ds_printf("DS_ERROR: Can't read %s\n", filename);
		return -1;
	}

	fs_close(fd);

	while (1)
	{
		if (optname == NULL)
			optname = strtok(buf, "=");
		else
			optname = strtok('\0', "=");

		value = strtok('\0', "\n");

		if (optname == NULL || value == NULL)
			break;

		for (i = 0; cfg[i].conf_type; i++)
		{
			if (strncasecmp(cfg[i].name, trim_spaces2(optname), 32))
				continue;

			switch (cfg[i].conf_type)
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

bool IsGdiOptimized(const char *full_path_game)
{
	bool is_optimized = false;
	if (full_path_game)
	{
		char *result = (char *)malloc(NAME_MAX);
		char *path = (char *)malloc(NAME_MAX);

		char *game = (char *)malloc(NAME_MAX);
		memset(game, 0, NAME_MAX);
		strcpy(game, GetLastPart(full_path_game, '/', 0));	

		memset(result, 0, NAME_MAX);
		memset(path, 0, NAME_MAX);

		strncpy(path, full_path_game, strlen(full_path_game) - (strlen(game) + 1));
		snprintf(result, NAME_MAX, "%s/track01.iso", path);
		free(game);
		free(path);
		
		is_optimized = (FileExists(result) == 1);
	}
	return is_optimized;
}

const char* GetLastPart(const char *source, const char separator, int option_path)
{
	static char path[NAME_MAX];
	memset(path, 0, NAME_MAX);
	
	char *last_folder = strrchr(source, separator);
	if (last_folder != NULL)
	{
		strcpy(path, last_folder + 1);
	}
	else 
	{
		strcpy(path, source);
	}

	if (option_path == 2)
	{
		for (char *c = path; (*c = toupper(*c)); ++c)
		{
			if (*c == 'a')
				*c = 'A'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
		}
	}
	else if (option_path == 1)
	{
		for (char *c = path; (*c = tolower(*c)); ++c)
		{
			if (*c == 'A')
				*c = 'a'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
		}
	}
	
	return path;
}

int GetDeviceType(const char *dir)
{
	if (!strncasecmp(dir, "/cd", 3))
	{
		return APP_DEVICE_CD;
	}
	else if (!strncasecmp(dir, "/sd", 3))
	{
		return APP_DEVICE_SD;
	}
	else if (!strncasecmp(dir, "/ide", 4))
	{
		return APP_DEVICE_IDE;
	}
	else if (!strncasecmp(dir, "/pc", 3))
	{
		return APP_DEVICE_PC;
		//	} else if(!strncasecmp(dir, "/???", 5)) {
		//		return APP_DEVICE_NET;
	}
	else
	{
		return -1;
	}
}

// int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename) {
// 	memset(filepath, 0, NAME_MAX);
// 	snprintf(filepath, NAME_MAX, "%s/%s/%s.gdi", fmPath, dirname, filename);

// 	if(FileExists(filepath)) {
// 		memset(filepath, 0, NAME_MAX);
// 		snprintf(filepath, NAME_MAX, "%s/%s.gdi", dirname, filename);
// 		return 1;
// 	}

// 	return 0;
// }

char *MakePresetFilename(const char *dir, uint8 *md5)
{
	char dev[8];
	static char filename[NAME_MAX];

	memset(filename, 0, sizeof(filename));
	strncpy(dev, &dir[1], 3);

	if (dev[2] == '/')
	{
		dev[2] = '\0';
	}
	else
	{
		dev[3] = '\0';
	}

	snprintf(filename, sizeof(filename),
			"%s/apps/%s/presets/%s_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.cfg",
#ifndef DEBUG_MENU_GAMES_CD
			getenv("PATH"), "iso_loader", "ide", md5[0],
#else
			getenv("PATH"), "iso_loader", dev, md5[0],
#endif
			md5[1], md5[2], md5[3], md5[4], md5[5],
			md5[6], md5[7], md5[8], md5[9], md5[10],
			md5[11], md5[12], md5[13], md5[14], md5[15]);

	return filename;
}

size_t GetCDDATrackFilename(int num, const char *full_path_game, char **result)
{
	char *path = (char *)malloc(NAME_MAX);
	int size = 0;

	char *game = (char *)malloc(NAME_MAX);
	memset(game, 0, NAME_MAX);
	strcpy(game, GetLastPart(full_path_game, '/', 0));
	
	if (*result == NULL)
	{
		*result = (char *)malloc(NAME_MAX);
	}
	
	memset(*result, 0, NAME_MAX);
	memset(path, 0, NAME_MAX);

	strncpy(path, full_path_game, strlen(full_path_game) - (strlen(game) + 1));
	snprintf(*result, NAME_MAX, "%s/track%02d.raw", path, num);
	free(game);
	free(path);
	size = FileSize(*result);
	
	if (size > 0)
	{
		return size;
	}

	int len = strlen(*result);
	(*result)[len - 3] = 'w';
	(*result)[len - 1] = 'v';

	return FileSize(*result);
}

static wav_stream_hnd_t wav_hnd = SND_STREAM_INVALID;
static int wav_inited = 0;

void StopCDDATrack()
{
	if (wav_inited)
	{
		wav_shutdown();
	}
}

void PlayCDDATrack(const char *file, int loop)
{
	StopCDDATrack();
	wav_inited = wav_init();

	if (wav_inited)
	{
		wav_hnd = wav_create(file, loop);

		if (wav_hnd == SND_STREAM_INVALID)
		{
			ds_printf("DS_ERROR: Can't play file: %s\n", file);
			return;
		}
		ds_printf("DS_OK: Start playing: %s\n", file);
		wav_play(wav_hnd);
	}
}
