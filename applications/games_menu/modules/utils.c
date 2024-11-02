/* DreamShell ##version##

   utils.c - ISO Loader app utils
   Copyright (C) 2022-2024 SWAT
   Copyright (C) 2024 Maniac Vera

*/

#include "ds.h"
#include <kos/md5.h>
#include <isoldr.h>
#include "app_utils.h"
#include "audio/wav.h"

/* Trim begin/end spaces and copy into output buffer */
void TrimSpaces(char *input, char *output, int size)
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

char *Trim(char *string)
{
    // trim prefix
    while ((*string) == ' ' ) {
         string ++;
    }

    // find end of original string
    char *c = string;
    while (*c) {
         c ++;
    }
    c--;

    // trim suffix
    while ((*c) == ' ' ) {
        *c = '\0';
        c--;
    }
    return string;
}

char *TrimSpaces2(char *txt)
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

char *FixSpaces(char *str)
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

int ConfigParse(isoldr_conf *cfg, const char *filename)
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
			if (strncasecmp(cfg[i].name, TrimSpaces2(optname), 32))
				continue;

			switch (cfg[i].conf_type)
			{
			case CONF_INT:
				*(int *)cfg[i].pointer = atoi(value);
				break;

			case CONF_STR:
				strcpy((char *)cfg[i].pointer, TrimSpaces2(value));
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

bool ContainsOnlyNumbers(const char *string)
{
	const int string_len = strlen(string);

	for(int i = 0; i < string_len; ++i)
	{
		if(!isdigit((unsigned char)string[i])) 
			return false;
	}

	return true;
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

int CanUseTrueAsyncDMA(int sector_size, int current_dev, int image_type)
{
	return (sector_size == 2048 &&
			(current_dev == APP_DEVICE_IDE || current_dev == APP_DEVICE_CD) &&
			(image_type == ISOFS_IMAGE_TYPE_ISO || image_type == ISOFS_IMAGE_TYPE_GDI));
}

void GetMD5HashISO(const char *file_mount_point, SectorDataStruct *sector_data)
{
	file_t fd;
	fd = fs_iso_first_file(file_mount_point);

	if (fd != FILEHND_INVALID)
	{
		if (fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, (int)sector_data->boot_sector) < 0)
		{
			memset(sector_data->md5, 0, sizeof(sector_data->md5));
			memset(sector_data->boot_sector, 0, sizeof(sector_data->boot_sector));
		}
		else
		{
			kos_md5(sector_data->boot_sector, sizeof(sector_data->boot_sector), sector_data->md5);
		}

		// Also get image type and sector size
		if (fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, (int)&sector_data->image_type) < 0)
		{
			ds_printf("Can't get image type\n");
		}

		if (fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE, (int)&sector_data->sector_size) < 0)
		{
			ds_printf("Can't get sector size\n");
		}

		fs_close(fd);
	}
}

char *MakePresetFilename(const char *default_dir, const char *device_dir, uint8 *md5)
{
	char dev[8];
	static char filename[NAME_MAX];

	memset(filename, 0, sizeof(filename));
	strncpy(dev, &device_dir[1], 3);

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
			default_dir, "iso_loader", dev, md5[0],
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
		wav_inited = 0;
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
