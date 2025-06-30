/* DreamShell ##version##

   utils.c - app utils
   Copyright (C) 2022-2025 SWAT
   Copyright (C) 2024-2025 Maniac Vera

*/

#include "ds.h"
#include <kos/md5.h>
#include <isoldr.h>
#include <img/copy.h>
#include "app_utils.h"
#include "audio/wav.h"
#include "settings.h"

static uint8 *romdisk_data[3] = {NULL, NULL, NULL};
static const char *mount_points[] = {"/presets_cd", "/presets_sd", "/presets_ide"};

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

	if (strncmp(preset->device, "auto", 4) != 0)
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

	if (preset->emu_cdda)
	{
		char cdda_mode[12];
		sprintf(cdda_mode, "0x%08lx", preset->emu_cdda);
		strcat(cmd, " -g ");
		strcat(cmd, cdda_mode);
	}

	if (preset->heap <= HEAP_MODE_MAPLE)
	{
		char mode[24];
		sprintf(mode, " -h %d", i);
		strcat(cmd, mode);
	}
	else
	{
		char *addr = preset->heap_memory;
		strcat(cmd, " -h ");
		strcat(cmd, addr);
	}

	if (preset->vmu_mode > 0)
	{
		char number[12];
		sprintf(number, " -v %d", atoi(preset->vmu_file));
		strcat(cmd, number);
	}

	if (preset->screenshot)
	{
		char hotkey[24];
		sprintf(hotkey, " -k 0x%lx", (uint32)SCREENSHOT_HOTKEY);
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

char *MakePresetFilename(const char *default_dir, const char *device_dir, uint8 *md5, const char *app_name)
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

	char presets_dir[100] = {0};
	if (app_name == NULL)
	{
		int device_type = GetDeviceType(default_dir);
		strcpy(presets_dir, mount_points[device_type]);
	}
	else
	{
		snprintf(presets_dir, sizeof(presets_dir), "%s/apps/%s/presets", default_dir, app_name);		
	}

	snprintf(filename, sizeof(filename),
			 "%s/%s_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.cfg",
			 presets_dir, dev, md5[0],
			 md5[1], md5[2], md5[3], md5[4], md5[5],
			 md5[6], md5[7], md5[8], md5[9], md5[10],
			 md5[11], md5[12], md5[13], md5[14], md5[15]);

	return filename;
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
		ds_printf("DS_OK: Start playing: %s\n", file);

		int volume = GetVolumeFromSettings();
		if(volume >= 0) {
			wav_volume(wav_hnd, volume);
		}
		wav_play(wav_hnd);
	}
}

int MountPresetsRomdisk(int device_type)
{
	char romdisk_path[NAME_MAX];
	const char *romdisk_names[] = {"presets_cd.romfs", "presets_sd.romfs", "presets_ide.romfs"};

	if (device_type < 0 || device_type >= 3 || romdisk_data[device_type])
	{
		return 0;
	}

	snprintf(romdisk_path, NAME_MAX, "%s/apps/iso_loader/resources/%s",
			 getenv("PATH"), romdisk_names[device_type]);

	if (fs_load(romdisk_path, (void **)&romdisk_data[device_type]) <= 0)
	{
		ds_printf("DS_ERROR: Failed to load romdisk %s\n", romdisk_path);
		return -1;
	}

	if (fs_romdisk_mount(mount_points[device_type], romdisk_data[device_type], 0) < 0)
	{
		ds_printf("DS_ERROR: Failed to mount romdisk %s\n", mount_points[device_type]);
		free(romdisk_data[device_type]);
		romdisk_data[device_type] = NULL;
		return -1;
	}

	return 0;
}

void UnmountPresetsRomdisk(int device_type)
{
	if (device_type < 0 || device_type >= 3 || !romdisk_data[device_type])
	{
		return;
	}

	fs_romdisk_unmount(mount_points[device_type]);
	free(romdisk_data[device_type]);
	romdisk_data[device_type] = NULL;
}

void UnmountAllPresetsRomdisks()
{
	for (int i = 0; i < sizeof(mount_points) / sizeof(mount_points[0]); i++)
	{
		UnmountPresetsRomdisk(i);
	}
}
