/* DreamShell ##version##

   utils.c - app utils
   Copyright (C) 2022-2026 SWAT
   Copyright (C) 2024-2025 Maniac Vera

*/

#include "ds.h"
#include <kos/md5.h>
#include <isoldr.h>
#include <img/copy.h>
#include "app_utils.h"
#include "audio/wav.h"
#include "settings.h"

char *StrdupSafe(const char *string)
{
	if (!string)
		return NULL;

	size_t len = strlen(string) + 1;
	char *copy = malloc(len);
	if (!copy)
	{
		return NULL;
	}

	memcpy(copy, string, len);
	return copy;
}

const char *GetFileName(const char *path)
{
	// LINUX, MAC..
	const char *slash = strrchr(path, '/');

// WINDOWS
#ifdef _WIN32
	const char *backslash = strrchr(path, '\\');
	if (!slash || (backslash && backslash > slash))
	{
		slash = backslash;
	}
#endif

	return slash ? slash + 1 : path;
}

bool EndsWith(const char *filename, const char *ext)
{
	size_t len = strlen(filename);
	size_t ext_len = strlen(ext);
	return len >= ext_len && strcasecmp(filename + len - ext_len, ext) == 0;
}

void TrimSlashes(char *path)
{
	int length = strlen(path);

	// REMOVE TRAILING '/' OR '\'
	while (length > 0 && (path[length - 1] == '/' || path[length - 1] == '\\'))
	{
		path[length - 1] = '\0';
		length--;
	}

	// REMOVE LEADING '/' OR '\' BY SHIFTING CHARACTERS TO THE LEFT
	while (*path == '/' || *path == '\\')
	{
		memmove(path, path + 1, strlen(path));
	}
}

/* Trim begin/end spaces and copy into output buffer */
void TrimSpaces(char *input, char *output, int size)
{
	char *p;
	char *o;
	int s = 0;
	size--;

	p = input;
	o = output;

	if (*p == '\0')
	{
		*o = '\0';
		return;
	}

	while (*p == ' ' && size > 0)
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
	while ((*string) == ' ')
	{
		string++;
	}

	// find end of original string
	char *c = string;
	while (*c)
	{
		c++;
	}
	c--;

	// trim suffix
	while ((*c) == ' ')
	{
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

	char buf[1024];
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
			optname = strtok(NULL, "=");

		value = strtok(NULL, "\n");

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
		char result[NAME_MAX];
		char path[NAME_MAX];

		char game[NAME_MAX];
		memset(game, 0, NAME_MAX);
		strcpy(game, GetLastPart(full_path_game, '/', 0));

		memset(result, 0, NAME_MAX);
		memset(path, 0, NAME_MAX);

		strncpy(path, full_path_game, strlen(full_path_game) - (strlen(game) + 1));
		snprintf(result, NAME_MAX, "%s/track03.iso", path);

		is_optimized = (FileExists(result) == 1);
	}
	return is_optimized;
}

void GoUpDirectory(const char *original_path, int levels, char *result)
{
	strncpy(result, original_path, NAME_MAX);
	for (int i = 0; i < levels; i++)
	{
		char *last_slash = strrchr(result, '/');
		if (last_slash != NULL && last_slash != result)
		{
			*last_slash = '\0';
		}
		else
		{
			strcpy(result, "/");
			break;
		}
	}
}

const char *GetLastPart(const char *source, const char separator, int option_path)
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
	char c;

	if (string == NULL)
		return false;

	if (*string == 0)
		return false;

	while ((c = *(string++)) != 0)
	{
		if (c < '0' || c > '9')
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

const char *GetDeviceName(int type)
{
	switch (type)
	{
		case APP_DEVICE_CD:
			return "cd";
		case APP_DEVICE_SD:
			return "sd";
		case APP_DEVICE_IDE:
			return "ide";
		case APP_DEVICE_PC:
			return "pc";
		default:
			return "";
	}
}

int device_is_auto_name(const char *device)
{
	if (device == NULL || device[0] == '\0' || device[0] == ' ')
	{
		return 1;
	}

	return strncmp(device, PRESET_DEVICE_AUTO, sizeof(PRESET_DEVICE_AUTO)) == 0;
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

		fs_close(fd);
	}
}

bool MakeShortcut(PresetStruct *preset, const char *device_dir, const char *full_path_game, bool show_name, const char *game_cover_path, int width, int height, bool yflip)
{
	FILE *fd;
	char save_file[NAME_MAX];
	char cmd[NAME_MAX * 2];
	int i;

	if (show_name)
	{
		snprintf(save_file, NAME_MAX, "%s/apps/main/scripts/%s.dsc", device_dir, preset->shortcut_name);
	}
	else
	{
		snprintf(save_file, NAME_MAX, "%s/apps/main/scripts/_%s.dsc", device_dir, preset->shortcut_name);
	}

	fd = fopen(save_file, "w");

	if (!fd)
	{
		ds_printf("DS_ERROR: Can't save shortcut\n");
		return false;
	}

	fprintf(fd, "module -o -f %s/modules/minilzo.klf\n", device_dir);
	fprintf(fd, "module -o -f %s/modules/isofs.klf\n", device_dir);
	fprintf(fd, "module -o -f %s/modules/isoldr.klf\n", device_dir);

	strcpy(cmd, "isoldr");
	strcat(cmd, preset->fastboot ? " -s" : " -i");

	if (preset->use_dma)
	{
		strcat(cmd, " -a");
	}

	if (preset->alt_read)
	{
		strcat(cmd, " -y");
	}

	if (preset->use_irq)
	{
		strcat(cmd, " -q");
	}

	if (preset->low)
	{
		strcat(cmd, " -l");
	}

	if (preset->emu_async)
	{
		char async[8];
		snprintf(async, sizeof(async), " -e %d", preset->emu_async);
		strcat(cmd, async);
	}

	if (!device_is_auto_name(preset->device))
	{
		strcat(cmd, " -d ");
		strcat(cmd, preset->device);
	}

	const char *memory_tmp;

	if (strlen(preset->memory) < 8)
	{
		char text[24];
		memset(text, 0, sizeof(text));
		strncpy(text, preset->memory, 10);
		memory_tmp = strncat(text, preset->custom_memory, 10);

		strcat(cmd, " -x ");
		strcat(cmd, memory_tmp);
	}
	else
	{
		strcat(cmd, " -x ");
		strcat(cmd, preset->memory);
	}

	char game[NAME_MAX];
	memset(game, 0, NAME_MAX);
	strcpy(game, full_path_game);

	strcat(cmd, " -f ");
	strcat(cmd, FixSpaces(game));

	char boot_mode[8];
	sprintf(boot_mode, "%d", preset->boot_mode);
	strcat(cmd, " -j ");
	strcat(cmd, boot_mode);

	char os[8];
	sprintf(os, "%d", preset->bin_type);
	strcat(cmd, " -o ");
	strcat(cmd, os);

	char patchstr[24];
	for (i = 0; i < 2; ++i)
	{
		if (preset->pa[i] & 0xffffff)
		{
			sprintf(patchstr, " --pa%d 0x%s", i + 1, preset->patch_a[i]);
			strcat(cmd, patchstr);
			sprintf(patchstr, " --pv%d 0x%s", i + 1, preset->patch_v[i]);
			strcat(cmd, patchstr);
		}
	}

	if (preset->cdda && preset->emu_cdda)
	{
		char cdda_mode[12];
		sprintf(cdda_mode, "0x%08lx", preset->emu_cdda);
		strcat(cmd, " -g ");
		strcat(cmd, cdda_mode);
	}

	if (preset->heap <= HEAP_MODE_MAPLE)
	{
		char mode[24];
		sprintf(mode, " -h %lu", preset->heap);
		strcat(cmd, mode);
	}
	else if (preset->heap_memory[0] != '\0')
	{
		strcat(cmd, " -h ");
		strcat(cmd, preset->heap_memory);
	}

	if (preset->vmu_mode > 0 && preset->emu_vmu > 0)
	{
		char number[12];
		sprintf(number, " -v %lu", preset->emu_vmu);
		strcat(cmd, number);
	}

	if (preset->screenshot)
	{
		char hotkey[24];
		uint32 hotkey_mask = preset->scr_hotkey ? preset->scr_hotkey : (uint32)SCREENSHOT_HOTKEY;
		sprintf(hotkey, " -k 0x%lx", hotkey_mask);
		strcat(cmd, hotkey);
	}

	if (preset->alt_boot)
	{
		char boot_file[24];
		sprintf(boot_file, " -b %s", ALT_BOOT_FILE);
		strcat(cmd, boot_file);
	}

	fprintf(fd, "%s\n", cmd);
	fprintf(fd, "console --show\n");
	fclose(fd);

	if (show_name)
	{
		snprintf(save_file, NAME_MAX, "%s/apps/main/images/%s.png", device_dir, preset->shortcut_name);
	}
	else
	{
		snprintf(save_file, NAME_MAX, "%s/apps/main/images/_%s.png", device_dir, preset->shortcut_name);
	}

	if (FileExists(save_file))
	{
		fs_unlink(save_file);
	}

	if (game_cover_path != NULL)
	{
		copy_image(game_cover_path, save_file, strcasecmp(strrchr(game_cover_path, '.'), ".png") == 0 ? true : false, true, 256, 256, width, height, yflip);
	}

	return true;
}

const char *GetFolderPathFromFile(const char *full_path_file)
{
	static char path[NAME_MAX];

	char *filename = (char *)malloc(NAME_MAX);
	memset(filename, 0, NAME_MAX);
	strcpy(filename, GetLastPart(full_path_file, '/', 0));

	memset(path, 0, NAME_MAX);

	strncpy(path, full_path_file, strlen(full_path_file) - (strlen(filename) + 1));
	free(filename);

	return path;
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
		// ds_printf("DS_OK: Start playing: %s\n", file);

		int volume = GetVolumeFromSettings();
		if(volume >= 0) {
			wav_volume(wav_hnd, volume);
		}
		wav_play(wav_hnd);
	}
}

