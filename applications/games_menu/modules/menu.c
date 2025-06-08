/* DreamShell ##version##

   menu.c - Games app module
   Copyright (C) 2024-2025 Maniac Vera

*/

#include "app_menu.h"
#include <jpeg/jpeg.h>
#include <png/png.h>
#include <kmg/kmg.h>
#include <img/SegaPVRImage.h>
#include <img/decode.h>
#include <img/load.h>
#include <img/convert.h>
#include <img/copy.h>
#include <img/utils.h>
#include <time.h>

struct MenuStructure menu_data;

void CreateMenuData(SendMessageCallBack *send_message_scan, SendMessageCallBack *send_message_optimizer, PostPVRCoverCallBack *post_pvr_cover, PostOptimizerCoverCallBack *post_optimizer_cover)
{
	menu_data.save_preset = false;
	menu_data.cover_background = true;
	menu_data.change_page_with_pad = false;
	menu_data.start_in_last_game = false;
	menu_data.rebuild_cache = false;
	menu_data.enable_cache = false;
	menu_data.last_game_played_index = -1;
	menu_data.last_device = 0;
	menu_data.cache_array_count = 0;
	menu_data.games_array_count = 0;
	menu_data.games_category_array_count = 0;
	menu_data.categories_array_count = 0;
	menu_data.firmware_array_count = 0;
	menu_data.default_cover_type = -1;
	menu_data.play_cdda_thread = NULL;
	menu_data.load_pvr_cover_thread = NULL;
	menu_data.finished_menu = false;
	menu_data.cdda_game_changed = false;
	menu_data.stop_load_pvr_cover = false;
	menu_data.firmware_array = NULL;
	menu_data.vmu_mode = 0;
	menu_data.preset = NULL;
	menu_data.games_array_ptr_count = 0;
	menu_data.games_array_ptr = NULL;

	menu_data.send_message_scan = send_message_scan;
	menu_data.send_message_optimizer = send_message_optimizer;
	menu_data.post_pvr_cover = post_pvr_cover;
	menu_data.post_optimizer_cover = post_optimizer_cover;

	menu_data.cache_array = NULL;
	menu_data.categories_array = NULL;
	menu_data.games_array = NULL;
	menu_data.games_category_array = NULL;
	memset(&menu_data.menu_option, 0, sizeof(menu_data.menu_option));

	memset(menu_data.category, 0, sizeof(menu_data.category));
	memset(menu_data.last_game, 0, sizeof(menu_data.last_game));
	memset(menu_data.default_dir, 0, sizeof(menu_data.default_dir));
	memset(menu_data.covers_path, 0, sizeof(menu_data.covers_path));
	memset(menu_data.games_path, 0, sizeof(menu_data.games_path));

	memset(menu_data.default_dir_sd, 0, sizeof(menu_data.default_dir_sd));
	memset(menu_data.covers_path_sd, 0, sizeof(menu_data.covers_path_sd));
	memset(menu_data.games_path_sd, 0, sizeof(menu_data.games_path_sd));

	menu_data.ide = (DirExists(IDE_DS_PATH) == 1);
	menu_data.sd = (DirExists(SD_DS_PATH) == 1);
	menu_data.cd = (!is_custom_bios());

	if (menu_data.ide)
	{
		strcpy(menu_data.default_dir, IDE_DS_PATH);
		strcpy(menu_data.covers_path, IDE_COVERS_PATH);
	}

	if (menu_data.sd)
	{
		if (menu_data.ide)
		{
			strcpy(menu_data.default_dir_sd, SD_DS_PATH);
			strcpy(menu_data.covers_path_sd, SD_COVERS_PATH);
		}
		else
		{
			strcpy(menu_data.default_dir, SD_DS_PATH);
			strcpy(menu_data.covers_path, SD_COVERS_PATH);
		}
	}

	if ((!menu_data.ide && !menu_data.sd) && menu_data.cd)
	{
		strcpy(menu_data.default_dir, CD_DS_PATH);
		strcpy(menu_data.covers_path, CD_COVERS_PATH);
	}

	menu_data.current_dev = GetDeviceType(menu_data.default_dir);
	menu_data.convert_pvr_to_png = (menu_data.current_dev == APP_DEVICE_IDE);

	char game_cover_path[NAME_MAX];
	memset(game_cover_path, 0, NAME_MAX);
	snprintf(game_cover_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/gd.jpg");
	menu_data.default_cover_type = (FileExists(game_cover_path) ? IT_JPG : IT_PNG);

	LoadMenuConfig();
	SetGamesPath(menu_data.games_config_path);

	menu_data.started_with_cache = menu_data.enable_cache;

	// CREATE DEFAULT MENU CONFIG FILE IF NOT EXISTS
	char config_file_name[100];
	snprintf(config_file_name, sizeof(config_file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "menu_games.cfg");
	if (!FileExists(config_file_name))
		SaveMenuConfig();

	LoadFirmwareFiles();
}

void DestroyMenuData()
{
	if (menu_data.load_pvr_cover_thread != NULL)
	{
		menu_data.stop_load_pvr_cover = true;
		thd_join(menu_data.load_pvr_cover_thread, NULL);
		menu_data.load_pvr_cover_thread = NULL;
	}

	if (menu_data.play_cdda_thread != NULL)
	{
		menu_data.cdda_game_changed = true;
		thd_join(menu_data.play_cdda_thread, NULL);
		menu_data.play_cdda_thread = NULL;
	}

	if (menu_data.firmware_array != NULL)
	{
		free(menu_data.firmware_array);
		menu_data.firmware_array_count = 0;
	}

	FreeGames();
	FreeCategories();
	FreeCache();
	UnmountAllPresetsRomdisks();

	memset(&menu_data, 0, sizeof(struct MenuStructure));
}

void SetGamesPath(const char *games_path)
{
	char path[NAME_MAX] = {0};
	const char *dev_dir = GetDeviceDir(menu_data.current_dev);
	snprintf(path, NAME_MAX, "%s/%s", dev_dir, games_path);

	memset(menu_data.games_path, 0, sizeof(menu_data.games_path));

	// IDE DEVICE
	if (menu_data.ide)
	{
		if (!DirExists(path))
		{
			strcpy(menu_data.games_path, IDE_GAMES_PATH);
		}
		else
		{
			snprintf(path, NAME_MAX, "%s/%s", IDE_PATH, games_path);
			strcpy(menu_data.games_path, path);
		}
	}

	// SD DEVICE
	if (menu_data.sd)
	{
		char sd_path[NAME_MAX] = {0};
		const char *base_folder = menu_data.ide ? SD_PATH : SD_PATH;

		snprintf(sd_path, NAME_MAX, "%s/%s", base_folder, games_path);

		if (!DirExists(path))
		{
			if (menu_data.ide)
				strcpy(menu_data.games_path_sd, SD_GAMES_PATH);
			else
				strcpy(menu_data.games_path, SD_GAMES_PATH);
		}
		else
		{
			if (menu_data.ide)
				strcpy(menu_data.games_path_sd, sd_path);
			else
				strcpy(menu_data.games_path, sd_path);
		}
	}

	// CD DEVICE
	if (!menu_data.ide && !menu_data.sd && menu_data.cd)
	{
		if (!DirExists(path))
		{
			strcpy(menu_data.games_path, CD_GAMES_PATH);
		}
		else
		{
			snprintf(path, NAME_MAX, "%s/%s", CD_PATH, games_path);
			strcpy(menu_data.games_path, path);
		}
	}
}

const char *GetDeviceDir(uint8 device)
{
	switch (device)
	{
	case APP_DEVICE_CD:
		return CD_PATH;

	case APP_DEVICE_SD:
	{
		if (menu_data.ide)
		{
			return SD_PATH;
		}
		else
		{
			return IDE_PATH;
		}
	}
	break;

	case APP_DEVICE_IDE:
		return IDE_PATH;

	case APP_DEVICE_PC:
		return "";

	default:
		return "";
	}
}

const char *GetDefaultDir(uint8 device)
{
	switch (device)
	{
	case APP_DEVICE_CD:
		return menu_data.default_dir;

	case APP_DEVICE_SD:
	{
		if (menu_data.ide)
		{
			return menu_data.default_dir_sd;
		}
		else
		{
			return menu_data.default_dir;
		}
	}
	break;

	case APP_DEVICE_IDE:
		return menu_data.default_dir;

	case APP_DEVICE_PC:
		return menu_data.default_dir;

	default:
		return menu_data.default_dir;
	}
}

const char *GetGamesPath(uint8 device)
{
	switch (device)
	{
	case APP_DEVICE_CD:
		return menu_data.games_path;

	case APP_DEVICE_SD:
	{
		if (menu_data.ide)
		{
			return menu_data.games_path_sd;
		}
		else
		{
			return menu_data.games_path;
		}
	}
	break;

	case APP_DEVICE_IDE:
		return menu_data.games_path;

	case APP_DEVICE_PC:
		return menu_data.games_path;

	default:
		return menu_data.games_path;
	}

	return NULL;
}

const char *GetCoversPath(uint8 device)
{
	switch (device)
	{
	case APP_DEVICE_CD:
		return menu_data.covers_path;

	case APP_DEVICE_SD:
	{
		if (menu_data.ide)
		{
			return menu_data.covers_path_sd;
		}
		else
		{
			return menu_data.covers_path;
		}
	}
	break;

	case APP_DEVICE_IDE:
		return menu_data.covers_path;

	case APP_DEVICE_PC:
		return menu_data.covers_path;

	default:
		return menu_data.covers_path;
	}
}

const char *GetFullGamePathByIndex(int game_index)
{
	static char full_path_game[NAME_MAX];
	memset(full_path_game, 0, sizeof(full_path_game));
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].folder)
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s/%s", GetGamesPath(menu_data.games_array[game_index].device), menu_data.games_array[game_index].folder, menu_data.games_array[game_index].game);
		}
		else
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s", GetGamesPath(menu_data.games_array[game_index].device), menu_data.games_array[game_index].game);
		}
	}

	return full_path_game;
}

void *PlayCDDAThread(void *params)
{
	int game_index = (int)params;
	if (game_index >= 0)
	{
		const char *full_path_game = GetFullGamePathByIndex(game_index);
		const uint32_t max_time = 2;
		uint32_t start_time = 0;
		uint32_t end_time = 0;
		timer_ms_gettime(&start_time, NULL);
		end_time = start_time;
		while (!menu_data.cdda_game_changed && (end_time - start_time) <= max_time)
		{
			thd_pass();
			timer_ms_gettime(&end_time, NULL);
		}

		if (!menu_data.cdda_game_changed)
		{
			if (CheckCDDA(game_index))
			{
				if (!menu_data.cdda_game_changed)
				{
					size_t track_size = 0;
					char *track_file_path = (char *)malloc(NAME_MAX);
					srand(time(NULL));					
					timer_ms_gettime(&start_time, NULL);

					do
					{
						track_size = GetCDDATrackFilename((random() % 15) + 4, full_path_game, &track_file_path);						

						// AVOID POSSIBLE CACHE STAGNATION
						if (menu_data.started_with_cache)
						{
							timer_ms_gettime(&end_time, NULL);
							if ((end_time - start_time) >= 5)
							{
								menu_data.games_array[game_index].is_cdda = CCGE_NOT_CDDA;
								break;
							}
						}

					} while (track_size == 0);

					if (menu_data.games_array[game_index].is_cdda == CCGE_CDDA)
					{						
						PlayCDDATrack(track_file_path, 3);
					}

					free(track_file_path);
				}
			}
		}
	}

	return NULL;
}

bool CheckCDDA(int game_index)
{
	bool isCDDA = false;
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].is_cdda == CCGE_CDDA)
		{
			isCDDA = true;
		}
		else
		{
			if (menu_data.games_array[game_index].is_cdda == CCGE_NOT_CHECKED)
			{
				const char *full_path_game = GetFullGamePathByIndex(game_index);
				size_t track_size = 0;

				char *track_file_path = (char *)malloc(NAME_MAX);
				track_size = GetCDDATrackFilename(4, full_path_game, &track_file_path);

				if (track_size > 0 && track_size < 30 * 1024 * 1024)
				{
					track_size = GetCDDATrackFilename(6, full_path_game, &track_file_path);
				}

				if (track_size > 0 && (MAX_SIZE_CDDA == 0 || track_size <= MAX_SIZE_CDDA))
				{
					isCDDA = true;
					menu_data.games_array[game_index].is_cdda = CCGE_CDDA;
				}
				else
				{
					isCDDA = false;
					menu_data.games_array[game_index].is_cdda = CCGE_NOT_CDDA;
				}

				free(track_file_path);
			}
		}
	}

	return isCDDA;
}

void StopCDDA()
{
	StopCDDATrack();
	if (menu_data.play_cdda_thread != NULL)
	{
		menu_data.cdda_game_changed = true;
		thd_join(menu_data.play_cdda_thread, NULL);
		menu_data.play_cdda_thread = NULL;
		menu_data.cdda_game_changed = false;
	}
}

void PlayCDDA(int game_index)
{
	StopCDDA();

	if ((menu_data.games_array[game_index].is_cdda == CCGE_NOT_CHECKED || menu_data.games_array[game_index].is_cdda == CCGE_CDDA) && (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE))
	{
		menu_data.play_cdda_thread = thd_create(0, PlayCDDAThread, (void *)game_index);
	}
}

ImageDimensionStruct *GetImageDimension(const char *image_file)
{
	ImageDimensionStruct *image = NULL;
	char *image_type = strrchr(image_file, '.');

	if (strcasecmp(image_type, ".kmg") == 0)
	{
		/* Open the file */
		file_t image_handle = fs_open(image_file, O_RDONLY);
		if (image_handle == FILEHND_INVALID)
		{
			return NULL;
		}

		kmg_header_t hdr;
		/* Read the header */
		if (fs_read(image_handle, &hdr, sizeof(hdr)) != sizeof(hdr))
		{
			fs_close(image_handle);
			return NULL;
		}

		/* Verify a few things */
		if (hdr.magic != KMG_MAGIC || hdr.version != KMG_VERSION ||
			hdr.platform != KMG_PLAT_DC)
		{
			fs_close(image_handle);
			return NULL;
		}
		fs_close(image_handle);

		image = (ImageDimensionStruct *)malloc(sizeof(ImageDimensionStruct));
		memset(image, 0, sizeof(ImageDimensionStruct));
		image->width = hdr.width;
		image->height = hdr.height;
	}
	else if (strcasecmp(image_type, ".pvr") == 0)
	{
		file_t image_handle = fs_open(image_file, O_RDONLY);

		if (image_handle == FILEHND_INVALID)
			return NULL;

		size_t fsize = 512;
		uint8 *data = (uint8 *)memalign(32, fsize);
		if (data == NULL)
		{
			fs_close(image_handle);
			return NULL;
		}

		if (fs_read(image_handle, data, fsize) == -1)
		{
			fs_close(image_handle);
			return NULL;
		}
		fs_close(image_handle);

		struct PVRTHeader pvrtHeader;
		unsigned int offset = ReadPVRHeader(data, &pvrtHeader);
		if (offset == 0)
		{
			free(data);
			return NULL;
		}
		free(data);

		enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);
		if (srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444)
		{
			return NULL;
		}

		image = (ImageDimensionStruct *)malloc(sizeof(ImageDimensionStruct));
		memset(image, 0, sizeof(ImageDimensionStruct));
		image->width = pvrtHeader.width;
		image->height = pvrtHeader.height;
	}
	else if (strcasecmp(image_type, ".jpg") == 0)
	{
		file_t image_handle = fs_open(image_file, O_RDONLY);

		if (image_handle == FILEHND_INVALID)
			return NULL;

		image = (ImageDimensionStruct *)malloc(sizeof(ImageDimensionStruct));
		memset(image, 0, sizeof(ImageDimensionStruct));

		size_t file_size = fs_total(image_handle);
		unsigned char buffer[32];
		int i, j, marker;

		fs_seek(image_handle, 0, SEEK_SET);
		fs_read(image_handle, buffer, 32);
		i = j = 2;	// Start at offset of first marker
		marker = 0; // Search for SOF (start of frame) marker

		while (i < 32 && marker != 0xffc0 && j < file_size)
		{
			marker = (MOTOSHORT(&buffer[i])) & 0xfffc;
			if (marker < 0xff00) // invalid marker
			{
				i += 2;
				continue;
			}
			if (marker == 0xffc0)
				break;

			j += 2 + MOTOSHORT(&buffer[i + 2]); // Skip to next marker
			if (j < file_size)					// need to read more
			{
				fs_seek(image_handle, j, SEEK_SET); // read some more
				fs_read(image_handle, buffer, 32);
				i = 0;
			}
			else
				break;
		}

		if (marker == 0xffc0)
		{
			image->height = MOTOSHORT(&buffer[i + 5]);
			image->width = MOTOSHORT(&buffer[i + 7]);
		}

		fs_close(image_handle);
	}
	else if (strcasecmp(image_type, ".png") == 0 || strcasecmp(image_type, ".bpm") == 0)
	{
		file_t image_handle = fs_open(image_file, O_RDONLY);

		if (image_handle == FILEHND_INVALID)
			return NULL;

		image = (ImageDimensionStruct *)malloc(sizeof(ImageDimensionStruct));
		memset(image, 0, sizeof(ImageDimensionStruct));

		if (strcasecmp(image_type, ".png") == 0)
		{
			fs_seek(image_handle, 16, SEEK_SET);
			fs_read(image_handle, (char *)&image->width, sizeof(uint32));

			fs_seek(image_handle, 20, SEEK_SET);
			fs_read(image_handle, (char *)&image->height, sizeof(uint32));
		}
		else if (strcasecmp(image_type, ".bmp") == 0)
		{
			fs_seek(image_handle, 18, SEEK_SET);
			fs_read(image_handle, (char *)&image->width, sizeof(uint32));

			fs_seek(image_handle, 22, SEEK_SET);
			fs_read(image_handle, (char *)&image->height, sizeof(uint32));
		}

		image->width = ntohl(image->width);
		image->height = ntohl(image->height);

		fs_close(image_handle);
	}

	image_type = NULL;
	return image;
}

bool IsValidImage(const char *image_file)
{
	bool isValid = false;

	ImageDimensionStruct *image = GetImageDimension(image_file);
	if (image != NULL)
	{
		isValid = (image->height == 64 && image->width == 64) || (image->height == 128 && image->width == 128) || (image->height == 256 && image->width == 256);

		free(image);
	}

	return isValid;
}

bool ContainsCoverType(int game_index, int menu_type, uint16 dw_image_type)
{
	bool result = false;

	if (game_index >= 0 && dw_image_type > 0)
	{
		result = ((GetCoverType(game_index, menu_type) & (dw_image_type)) == dw_image_type);
	}

	return result;
}

uint16 GetCoverType(int game_index, int menu_type)
{
	uint16 dw_image_type = 0;

	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].cover_type <= 0)
		{
			menu_data.games_array[game_index].cover_type = 0;
		}

		dw_image_type = ((menu_data.games_array[game_index].cover_type >> ((menu_type - 1) * 16)) & IMAGE_TYPE_MASK);
	}

	return dw_image_type;
}

void CleanCoverType(int game_index, int menu_type)
{
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].cover_type <= 0)
		{
			menu_data.games_array[game_index].cover_type = 0;
			return;
		}

		switch (menu_type)
		{
		case MT_PLANE_TEXT:
			menu_data.games_array[game_index].cover_type &= 0xFFFFFFFF0000;
			break;

		case MT_IMAGE_TEXT_64_5X2:
			menu_data.games_array[game_index].cover_type &= 0xFFFF0000FFFF;
			break;

		case MT_IMAGE_128_4X3:
			menu_data.games_array[game_index].cover_type &= 0x0000FFFFFFFF;
			break;

		default:
			break;
		}
	}
}

void SetCoverType(int game_index, int menu_type, uint16 dw_image_type)
{
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].cover_type <= 0)
		{
			menu_data.games_array[game_index].cover_type = 0;
		}

		menu_data.games_array[game_index].cover_type |= (uint64)dw_image_type << ((menu_type - 1) * 16);
	}
}

const char *GetCoverExtensionFromType(uint16 image_type)
{
	switch (image_type)
	{
	case IT_PNG:
		return ".png";

	case IT_JPG:
		return ".jpg";

	case IT_BPM:
		return ".bpm";

	case IT_PVR:
		return ".pvr";

	case IT_KMG:
		return ".kmg";

	default:
		return ".jpg";
	}
}

bool CheckCoverImageType(int game_index, int menu_type, uint16 image_type)
{
	bool exists_cover = false;

	char *game_without_extension = NULL;
	if (GetCoverName(game_index, &game_without_extension))
	{
		if (menu_data.games_array[game_index].cover.menu_type & (1 << (menu_type - 1)))
		{
			char cover_file[NAME_MAX];
			memset(cover_file, 0, NAME_MAX);
			snprintf(cover_file, NAME_MAX, "%s%s", game_without_extension, GetCoverExtensionFromType(image_type));

			char game_cover_path[NAME_MAX];
			memset(game_cover_path, 0, NAME_MAX);
			snprintf(game_cover_path, NAME_MAX, "%s%s/%s%s", GetCoversPath(menu_data.current_dev), GetCoverFolder(menu_type), game_without_extension, GetCoverExtensionFromType(image_type));

			if (IsValidImage(game_cover_path))
			{
				menu_data.games_array[game_index].exists_cover[menu_type - 1] = SC_EXISTS;
				SetCoverType(game_index, menu_type, image_type);
				exists_cover = true;
			}
			else
			{
				fs_unlink(game_cover_path);
			}
		}

		free(game_without_extension);
	}

	return exists_cover;
}

int16 CheckCover(int game_index, int menu_type)
{
	int16 exists_cover = SC_WITHOUT_SEARCHING;

	if (menu_data.games_array[game_index].exists_cover[menu_type - 1] == SC_WITHOUT_SEARCHING)
	{
		uint16 image_count = 1;
		while (image_count < IMAGE_TYPE_SUPPORTED)
		{
			if (IMAGE_TYPE_SUPPORTED & image_count)
			{
				if (CheckCoverImageType(game_index, menu_type, image_count))
				{
					exists_cover = menu_data.games_array[game_index].exists_cover[menu_type - 1];
					break;
				}
			}
			image_count = image_count << 1;
		}

		if (menu_data.games_array[game_index].exists_cover[menu_type - 1] == SC_WITHOUT_SEARCHING)
		{
			SetCoverType(game_index, menu_type, menu_data.default_cover_type);
			exists_cover = menu_data.games_array[game_index].exists_cover[menu_type - 1] = SC_DEFAULT;
			menu_data.games_array[game_index].check_pvr = true;
		}
	}
	else
	{
		exists_cover = menu_data.games_array[game_index].exists_cover[menu_type - 1];
	}

	return exists_cover;
}

bool GetCoverName(int game_index, char **cover_name)
{
	if (*cover_name != NULL)
	{
		free(*cover_name);
	}

	*cover_name = NULL;
	if (game_index >= 0)
	{
		*cover_name = (char *)malloc(NAME_MAX);
		memset(*cover_name, 0, NAME_MAX);

		if (menu_data.games_array[game_index].is_folder_name)
		{
			strncpy(*cover_name, menu_data.games_array[game_index].folder_name, strlen(menu_data.games_array[game_index].folder_name));
		}
		else
		{
			strncpy(*cover_name, menu_data.games_array[game_index].game, strlen(menu_data.games_array[game_index].game) - 4);
		}
	}

	return *cover_name != NULL;
}

const char *GetCoverFolder(int menu_type)
{
	switch (menu_type)
	{
	case MT_PLANE_TEXT:
		return PLANE_TEXT_MENU_FOLDER;

	case MT_IMAGE_TEXT_64_5X2:
		return IMAGE_TEXT_64_5X2_MENU_FOLDER;

	case MT_IMAGE_128_4X3:
		return IMAGE_128_4X3_FOLDER;

	default:
		return PLANE_TEXT_MENU_FOLDER;
	}
}

bool GetGameCoverPath(int game_index, char **game_cover_path, int menu_type)
{
	if (*game_cover_path != NULL)
	{
		free(*game_cover_path);
	}

	*game_cover_path = NULL;
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].exists_cover[menu_type - 1] == SC_EXISTS)
		{
			char *game = NULL;
			const char *cover_folder = GetCoverFolder(menu_type);
			GetCoverName(game_index, &game);

			uint16 image_count = 1;
			while (image_count < IMAGE_TYPE_SUPPORTED)
			{
				if (IMAGE_TYPE_SUPPORTED & image_count)
				{
					if (ContainsCoverType(game_index, menu_type, image_count))
					{
						*game_cover_path = (char *)malloc(NAME_MAX);
						memset(*game_cover_path, 0, NAME_MAX);

						snprintf(*game_cover_path, NAME_MAX, "%s%s/%s%s", GetCoversPath(menu_data.current_dev), cover_folder, game, GetCoverExtensionFromType(image_count));
						break;
					}
				}
				image_count = image_count << 1;
			}

			if (game != NULL)
			{
				free(game);
			}
		}
		else
		{
			if (menu_data.games_array[game_index].exists_cover[menu_type - 1] == SC_DEFAULT)
			{
				*game_cover_path = (char *)malloc(NAME_MAX);
				memset(*game_cover_path, 0, NAME_MAX);

				if (ContainsCoverType(game_index, menu_type, IT_JPG))
				{
					snprintf(*game_cover_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/gd.jpg");
				}
				else
				{
					snprintf(*game_cover_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/gd.png");
				}
			}
		}
	}

	return *game_cover_path != NULL;
}

void SetMenuType(int menu_type)
{
	menu_data.menu_type = menu_type;

	switch (menu_type)
	{
	case MT_IMAGE_TEXT_64_5X2:
	{
		menu_data.menu_option.max_page_size = 10;
		menu_data.menu_option.max_columns = 2;
		menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
		menu_data.menu_option.init_position_x = 29;
		menu_data.menu_option.init_position_y = -16;
		menu_data.menu_option.padding_x = 306;
		menu_data.menu_option.padding_y = 20;
		menu_data.menu_option.image_size = 64.0f;
	}
	break;

	case MT_IMAGE_128_4X3:
	{
		menu_data.menu_option.max_page_size = 12;
		menu_data.menu_option.max_columns = 4;
		menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
		menu_data.menu_option.init_position_x = 62;
		menu_data.menu_option.init_position_y = -37;
		menu_data.menu_option.padding_x = 156;
		menu_data.menu_option.padding_y = 8;
		menu_data.menu_option.image_size = 128.0f;
	}
	break;

	case MT_IMAGE_256_3X2:
	{
		menu_data.menu_option.max_page_size = 6;
		menu_data.menu_option.max_columns = 3;
		menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
		menu_data.menu_option.init_position_x = 90;
		menu_data.menu_option.init_position_y = -82;
		menu_data.menu_option.padding_x = 212.0f;
		menu_data.menu_option.padding_y = 12;
		menu_data.menu_option.image_size = 200.0f;
	}
	break;

	case MT_PLANE_TEXT:
	{
		menu_data.menu_option.max_page_size = 14;
		menu_data.menu_option.max_columns = 1;
		menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
		menu_data.menu_option.init_position_x = 2;
		menu_data.menu_option.init_position_y = 29;
		menu_data.menu_option.padding_x = 2;
		menu_data.menu_option.padding_y = 27;
		menu_data.menu_option.image_size = 0.0f;
	}
	break;

	default:
	{
		menu_data.menu_option.max_page_size = 10;
		menu_data.menu_option.max_columns = 2;
		menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
		menu_data.menu_option.init_position_x = 20;
		menu_data.menu_option.init_position_y = 5;
		menu_data.menu_option.padding_x = 320;
		menu_data.menu_option.padding_y = 12;
		menu_data.menu_option.image_size = 64.0f;
	}
	break;
	}
}

void FreeGames()
{
	if (menu_data.games_array_count > 0)
	{
		for (int icount = 0; icount < menu_data.games_array_count; icount++)
		{
			if (menu_data.games_array[icount].game)
			{
				free(menu_data.games_array[icount].game);
			}

			if (menu_data.games_array[icount].folder)
			{
				free(menu_data.games_array[icount].folder);
			}

			if (menu_data.games_array[icount].folder_name)
			{
				free(menu_data.games_array[icount].folder_name);
			}

			menu_data.games_array[icount].game = NULL;
			menu_data.games_array[icount].folder = NULL;
			menu_data.games_array[icount].folder_name = NULL;
		}

		free(menu_data.games_array);
		menu_data.games_array = NULL;
		menu_data.games_array_count = 0;
	}
}

void FreeCategories()
{
	CategoryStruct *current_category, *tmp_category;
	HASH_ITER(hh, menu_data.categories_array, current_category, tmp_category)
	{
		HASH_DEL(menu_data.categories_array, current_category);
		free(current_category->games_index);
		free(current_category);
	}

	if (menu_data.games_category_array != NULL)
	{
		free(menu_data.games_category_array);
		menu_data.games_category_array = NULL;
		menu_data.games_category_array_count = 0;
	}
}

void FreeCache()
{
	if (menu_data.cache_array_count > 0)
	{
		free(menu_data.cache_array);
		menu_data.cache_array = NULL;
		menu_data.cache_array_count = 0;
	}
}

int GenerateVMUFile(const char *full_path_game, int vmu_mode, uint32 vmu_number)
{
	if (full_path_game != NULL && vmu_mode > 0)
	{
		char vmu_file_path[NAME_MAX];
		memset(vmu_file_path, 0, NAME_MAX);

		char private_path[NAME_MAX];
		snprintf(private_path, NAME_MAX, "%s/vmu%03ld.vmd", GetFolderPathFromFile(full_path_game), vmu_number);

		int private_size = FileSize(private_path);
		if (vmu_mode == 1) // SHARED
		{
			if (private_size > 0)
			{
				fs_unlink(private_path);
			}

			return 0;
		}

		switch (vmu_mode)
		{
		case 2: // 200 BLOCKS
		{
			snprintf(vmu_file_path, sizeof(vmu_file_path),
					 "%s/apps/%s/resources/empty_vmu_128kb.vmd",
					 GetDefaultDir(menu_data.current_dev), "games_menu");
		}
		break;

		case 3: // 1800 BLOCKS
		{
			snprintf(vmu_file_path, sizeof(vmu_file_path),
					 "%s/apps/%s/resources/empty_vmu_1024kb.vmd",
					 GetDefaultDir(menu_data.current_dev), "games_menu");
		}
		break;
		}

		int source_size = FileSize(vmu_file_path);
		if (source_size > 0 && private_size != source_size)
		{
			if (private_size > 0)
			{
				fs_unlink(private_path);
			}

			CopyFile(vmu_file_path, private_path, 0);
			return 0;
		}
		else
		{
			return 1;
		}
	}

	return 0;
}

static int AppCompareFirmwares(const void *a, const void *b)
{
	const FirmwareStruct *left = (const FirmwareStruct *)a;
	const FirmwareStruct *right = (const FirmwareStruct *)b;

	if (ContainsOnlyNumbers(left->file) && ContainsOnlyNumbers(right->file))
	{
		return atoi(left->file) - atoi(right->file);
	}
	else if (ContainsOnlyNumbers(left->file))
	{
		return -1;
	}
	else if (ContainsOnlyNumbers(right->file))
	{
		return 1;
	}

	return strcmp(left->file, right->file);
}

bool LoadFirmwareFiles()
{
	char fw_dir[128];
	memset(fw_dir, 0, sizeof(fw_dir));
	sprintf(fw_dir, "%s/firmware/isoldr", GetDefaultDir(menu_data.current_dev));

	if (menu_data.firmware_array != NULL)
	{
		free(menu_data.firmware_array);
		menu_data.firmware_array = NULL;
		menu_data.firmware_array_count = 0;
	}

	file_t fd = fs_open(fw_dir, O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return false;

	dirent_t *ent = NULL;
	char *file_type = NULL;

	while ((ent = fs_readdir(fd)) != NULL)
	{
		if (ent->name[0] == '.')
			continue;

		file_type = strrchr(ent->name, '.');

		if (strcasecmp(file_type, ".bin") == 0 && (strlen(ent->name) - 4) <= FIRMWARE_SIZE)
		{
			menu_data.firmware_array_count++;

			if (menu_data.firmware_array == NULL)
			{
				menu_data.firmware_array = (FirmwareStruct *)malloc(sizeof(FirmwareStruct));
			}
			else
			{
				menu_data.firmware_array = (FirmwareStruct *)realloc(menu_data.firmware_array, menu_data.firmware_array_count * sizeof(FirmwareStruct));
			}

			memset(&menu_data.firmware_array[menu_data.firmware_array_count - 1], 0, sizeof(FirmwareStruct));
			memset(menu_data.firmware_array[menu_data.firmware_array_count - 1].file, 0, FIRMWARE_SIZE + 1);
			strncpy(menu_data.firmware_array[menu_data.firmware_array_count - 1].file, ent->name, strlen(ent->name) - 4);
		}
	}
	fs_close(fd);

	if (menu_data.firmware_array_count > 0)
	{
		qsort(menu_data.firmware_array, menu_data.firmware_array_count, sizeof(FirmwareStruct), AppCompareFirmwares);
	}

	return true;
}

PresetStruct *GetDefaultPresetGame(const char *full_path_game, SectorDataStruct *sector_data)
{
	PresetStruct *preset = NULL;

	if (full_path_game)
	{
		preset = (PresetStruct *)malloc(sizeof(PresetStruct));
		memset(preset, 0, sizeof(PresetStruct));

		bool free_sector_data = false;
		if (sector_data == NULL)
		{
			if (fs_iso_mount("/iso_game", full_path_game) == 0)
			{
				sector_data = (SectorDataStruct *)malloc(sizeof(SectorDataStruct));
				memset(sector_data, 0, sizeof(SectorDataStruct));
				GetMD5HashISO("/iso_game", sector_data);
				fs_iso_unmount("/iso_game");
				free_sector_data = true;
			}
		}

		if (CanUseTrueAsyncDMA(sector_data->sector_size, GetDeviceType(full_path_game), sector_data->image_type))
		{
			preset->use_dma = 1;
			preset->emu_async = 0;
			sprintf(preset->memory, "0x%08lx", (unsigned long)ISOLDR_DEFAULT_ADDR_MIN_GINSU);
		}
		else
		{
			preset->use_dma = 0;
			preset->emu_async = 8;
			sprintf(preset->memory, "0x%08lx", (unsigned long)ISOLDR_DEFAULT_ADDR_LOW);
		}

		char title[32];
		memset(title, 0, sizeof(title));

		ipbin_meta_t *ipbin = (ipbin_meta_t *)sector_data->boot_sector;
		TrimSpaces(ipbin->title, title, sizeof(ipbin->title));

		if (free_sector_data)
		{
			free(sector_data);
		}

		preset->alt_read = 0;
		strcpy(preset->device, "auto");
		preset->emu_cdda = CDDA_MODE_DISABLED;
		preset->use_irq = 0;
		preset->emu_vmu = 0;
		preset->scr_hotkey = 0;
		preset->boot_mode = BOOT_MODE_DIRECT;
		strcpy(preset->title, title);

		// Enable CDDA if present
		if (full_path_game[0] != '\0')
		{
			char *track_file_path = NULL;
			size_t track_size = GetCDDATrackFilename(4, full_path_game, &track_file_path);

			if (track_size && track_size < 30 * 1024 * 1024)
			{
				track_size = GetCDDATrackFilename(6, full_path_game, &track_file_path);
			}

			if (track_size > 0)
			{
				preset->use_irq = 1;
				preset->emu_cdda = 1;
			}

			if (track_file_path)
			{
				free(track_file_path);
			}
		}

		preset->heap = strtoul(preset->heap_memory, NULL, 16);
		PatchParseText(preset);
	}

	return preset;
}

PresetStruct *LoadPresetGame(int game_index, bool default_preset)
{
	PresetStruct *preset = NULL;

	if (game_index >= 0)
	{
		preset = (PresetStruct *)malloc(sizeof(PresetStruct));
		memset(preset, 0, sizeof(PresetStruct));

		preset->game_index = game_index;

		const char *full_path_game = GetFullGamePathByIndex(game_index);		
		char *full_preset_file_name = NULL;
		SectorDataStruct sector_data;
		memset(&sector_data, 0, sizeof(SectorDataStruct));

		if (fs_iso_mount("/iso_game", full_path_game) == 0)
		{
			GetMD5HashISO("/iso_game", &sector_data);
			fs_iso_unmount("/iso_game");

			bool exists_preset = false;
			int game_device_type = GetDeviceType(full_path_game);
			int app_name_count = 0;
			const char *app_name_array[3] = {"games_menu", "iso_loader", NULL};
			
			while (!default_preset && app_name_array[app_name_count] != NULL)
			{
				if (game_device_type == APP_DEVICE_SD)
				{
					full_preset_file_name = MakePresetFilename(GetDefaultDir(APP_DEVICE_SD), GetDefaultDir(APP_DEVICE_SD), sector_data.md5, app_name_array[app_name_count]);
					if (!FileExists(full_preset_file_name) && menu_data.ide)
					{
						full_preset_file_name = MakePresetFilename(GetDefaultDir(APP_DEVICE_IDE), GetDefaultDir(APP_DEVICE_SD), sector_data.md5, app_name_array[app_name_count]);
					}
				}
				else if (game_device_type == APP_DEVICE_IDE)
				{
					full_preset_file_name = MakePresetFilename(GetDefaultDir(APP_DEVICE_IDE), GetDefaultDir(APP_DEVICE_IDE), sector_data.md5, app_name_array[app_name_count]);
				}
				else
				{
					full_preset_file_name = MakePresetFilename(getenv("PATH"), GetDefaultDir(menu_data.current_dev), sector_data.md5, app_name_array[app_name_count]);
				}

				if (FileSize(full_preset_file_name) >= 5)
				{
					exists_preset = true;
					break;
				}

				++app_name_count;
			}

			if (!exists_preset)
			{
				if (MountPresetsRomdisk(game_device_type) == 0)
				{
					full_preset_file_name = MakePresetFilename(GetDefaultDir(game_device_type), GetDefaultDir(game_device_type), sector_data.md5, NULL);
				}
			}
		}

		if (full_preset_file_name != NULL)
		{
			memset(preset->preset_file_name, 0, sizeof(preset->preset_file_name));
			strcpy(preset->preset_file_name, strrchr(full_preset_file_name, '/') + 1);
		}

		if (FileSize(full_preset_file_name) < 5)
		{
			full_preset_file_name = NULL;
		}

		preset->emu_async = 16;
		preset->boot_mode = BOOT_MODE_DIRECT;
		preset->bin_type = BIN_TYPE_AUTO;
		preset->cdda = CDDA_MODE_DISABLED;
		strcpy(preset->title, "");
		strcpy(preset->device, "");
		sprintf(preset->memory, "0x%08lx", (unsigned long)ISOLDR_DEFAULT_ADDR_MIN);

		strcpy(preset->heap_memory, "");
		strcpy(preset->bin_file, "");

		memset(preset->patch_a, 0, 2 * 10);
		memset(preset->patch_v, 0, 2 * 10);
		memset(preset->pa, 0, 2 * sizeof(uint32));
		memset(preset->pv, 0, 2 * sizeof(uint32));

		char scr_hotkey[4];
		memset(scr_hotkey, 0, sizeof(scr_hotkey));

		isoldr_conf options[] = {
			{"dma", CONF_INT, (void *)&preset->use_dma},
			{"altread", CONF_INT, (void *)&preset->alt_read},
			{"cdda", CONF_ULONG, (void *)&preset->emu_cdda},
			{"irq", CONF_INT, (void *)&preset->use_irq},
			{"low", CONF_INT, (void *)&preset->low},
			{"vmu", CONF_INT, (void *)&preset->emu_vmu},
			{"scrhotkey", CONF_STR, (void *)scr_hotkey},
			{"heap", CONF_STR, (void *)&preset->heap_memory},
			{"memory", CONF_STR, (void *)preset->memory},
			{"async", CONF_INT, (void *)&preset->emu_async},
			{"mode", CONF_INT, (void *)&preset->boot_mode},
			{"type", CONF_INT, (void *)&preset->bin_type},
			{"file", CONF_STR, (void *)preset->bin_file},
			{"title", CONF_STR, (void *)preset->title},
			{"device", CONF_STR, (void *)preset->device},
			{"fastboot", CONF_INT, (void *)&preset->fastboot},
			{"pa1", CONF_STR, (void *)preset->patch_a[0]},
			{"pv1", CONF_STR, (void *)preset->patch_v[0]},
			{"pa2", CONF_STR, (void *)preset->patch_a[1]},
			{"pv2", CONF_STR, (void *)preset->patch_v[1]},
			{NULL, CONF_END, NULL}};

		char preset_file_name[100];
		memset(preset_file_name, 0, sizeof(preset_file_name));
		strcpy(preset_file_name, preset->preset_file_name);

		if (full_preset_file_name != NULL)
		{
			if (ConfigParse(options, full_preset_file_name) == 0)
			{
				preset->heap = strtoul(preset->heap_memory, NULL, 16);
				PatchParseText(preset);

				if (scr_hotkey[0] != '\0' && scr_hotkey[0] != ' ')
				{
					preset->scr_hotkey = strtol(scr_hotkey, NULL, 16);
				}
			}
			else
			{
				free(preset);
				preset = GetDefaultPresetGame(full_path_game, &sector_data);
				preset->game_index = game_index;
				strcpy(preset->preset_file_name, preset_file_name);
			}

			if (preset->scr_hotkey)
			{
				preset->screenshot = 1;
				preset->use_irq = 1;
			}

			if (preset->emu_vmu)
			{
				preset->use_irq = 1;
				snprintf(preset->vmu_file, sizeof(preset->vmu_file), "vmu%03ld.vmd", preset->emu_vmu);

				char dst_path[NAME_MAX];
				snprintf(dst_path, NAME_MAX, "%s/%s", GetFolderPathFromFile(full_path_game), preset->vmu_file);

				int dst_size = FileSize(dst_path);

				menu_data.vmu_mode = 0;
				if (dst_size <= 0)
				{
					preset->vmu_mode = 1;
				}
				else if (dst_size < 256 << 10)
				{
					preset->vmu_mode = 2;
				}
				else
				{
					preset->vmu_mode = 3;
				}
				menu_data.vmu_mode = preset->vmu_mode;
			}
			else
			{
				preset->vmu_mode = 0;
			}

			if (preset->device[0] == '\0')
			{
				strcpy(preset->device, "auto");
			}
		}
		else
		{
			free(preset);
			preset = GetDefaultPresetGame(full_path_game, &sector_data);
			preset->game_index = game_index;
			strcpy(preset->preset_file_name, preset_file_name);
		}

		if (preset->emu_cdda)
		{
			preset->cdda = 1;
		}
	}

	return preset;
}

void PatchParseText(PresetStruct *preset)
{
	if (preset)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (preset->patch_a[i][1] != '0' && strlen(preset->patch_a[i]) == 8 && strlen(preset->patch_v[i]))
			{
				preset->pa[i] = strtoul(preset->patch_a[i], NULL, 16);
				preset->pv[i] = strtoul(preset->patch_v[i], NULL, 16);
			}
			else
			{
				preset->pa[i] = 0;
				preset->pv[i] = 0;
			}
		}
	}
}

isoldr_info_t *ParsePresetToIsoldr(int game_index, PresetStruct *preset)
{
	isoldr_info_t *isoldr = NULL;
	if (game_index >= 0 && preset != NULL && preset->game_index >= 0)
	{
		const char *full_path_game = GetFullGamePathByIndex(game_index);

		if ((isoldr = isoldr_get_info(full_path_game, 0)) == NULL)
		{
			return NULL;
		}

		isoldr->use_dma = preset->use_dma;
		isoldr->alt_read = preset->alt_read;
		isoldr->emu_async = preset->emu_async;
		isoldr->emu_cdda = preset->emu_cdda;
		isoldr->use_irq = preset->use_irq;
		isoldr->scr_hotkey = preset->scr_hotkey;
		isoldr->heap = preset->heap;
		isoldr->boot_mode = preset->boot_mode;
		isoldr->fast_boot = preset->fastboot;
		isoldr->emu_vmu = preset->emu_vmu;

		if (preset->low)
		{
			isoldr->syscalls = 1;
		}

		if (preset->bin_type != BIN_TYPE_AUTO)
		{
			isoldr->exec.type = preset->bin_type;
		}

		if (strlen(preset->device) > 0)
		{
			if (strncmp(preset->device, "auto", 4) != 0)
			{
				strcpy(isoldr->fs_dev, preset->device);
			}
			else
			{
				strcpy(isoldr->fs_dev, "auto");
			}
		}
		else
		{
			strcpy(isoldr->fs_dev, "auto");
		}

		for (int i = 0; i < sizeof(isoldr->patch_addr) >> 2; ++i)
		{
			if (preset->pa[i] & 0xffffff)
			{
				isoldr->patch_addr[i] = preset->pa[i];
				isoldr->patch_value[i] = preset->pa[i];
			}
		}

		if (preset->alt_boot && menu_data.games_array[game_index].folder)
		{
			char game_path[NAME_MAX];
			memset(game_path, 0, NAME_MAX);
			snprintf(game_path, NAME_MAX, "%s/%s", GetGamesPath(menu_data.games_array[game_index].device), menu_data.games_array[game_index].folder);
			isoldr_set_boot_file(isoldr, game_path, ALT_BOOT_FILE);
		}
	}

	return isoldr;
}

bool SavePresetGame(PresetStruct *preset)
{
	bool saved = false;

	if (menu_data.current_dev != APP_DEVICE_SD && menu_data.current_dev != APP_DEVICE_IDE)
	{
		return false;
	}

	if (preset != NULL && preset->game_index >= 0)
	{
		file_t fd;
		char result[1024];
		char memory[24];
		int async = 0, type = 0, mode = 0;
		uint32 heap = HEAP_MODE_AUTO;
		uint32 cdda_mode = CDDA_MODE_DISABLED;

		char preset_file_name[NAME_MAX];
		memset(preset_file_name, 0, NAME_MAX);

		snprintf(preset_file_name, sizeof(preset_file_name),
				 "%s/apps/%s/presets/%s",
				 GetDefaultDir(menu_data.current_dev), "games_menu", preset->preset_file_name);

		fd = fs_open(preset_file_name, O_CREAT | O_TRUNC | O_WRONLY);

		if (fd == FILEHND_INVALID)
		{
			return false;
		}

		memset(result, 0, sizeof(result));
		memset(memory, 0, sizeof(memory));

		async = preset->emu_async;
		type = preset->bin_type;
		mode = preset->boot_mode;

		if (preset->cdda)
		{
			cdda_mode = preset->emu_cdda;
		}

		if (strcasecmp(preset->memory, "0x8c") == 0)
		{
			snprintf(memory, sizeof(memory), "%s%s", preset->memory, preset->custom_memory);
		}
		else
		{
			strcpy(memory, preset->memory);
		}

		heap = preset->heap;

		if (preset->device[0] == '\0' || preset->device[0] == ' ')
		{
			strcpy(preset->device, "auto");
		}

		if (preset->emu_vmu > 0 || preset->scr_hotkey)
		{
			preset->use_irq = 1;
		}

		snprintf(result, sizeof(result),
				 "title = %s\ndevice = %s\ndma = %d\nasync = %d\ncdda = %08lx\n"
				 "irq = %d\nlow = %d\nheap = %08lx\nfastboot = %d\ntype = %d\nmode = %d\nmemory = %s\n"
				 "vmu = %d\nscrhotkey = %lx\naltread = %d\n"
				 "pa1 = %08lx\npv1 = %08lx\npa2 = %08lx\npv2 = %08lx\n",
				 preset->title, preset->device, preset->use_dma, async,
				 cdda_mode, preset->use_irq, preset->low, heap,
				 preset->fastboot, type, mode, memory,
				 (int)preset->emu_vmu, (uint32)preset->scr_hotkey,
				 preset->alt_read,
				 preset->pa[0], preset->pv[0], preset->pa[1], preset->pv[1]);

		if (preset->alt_boot)
		{
			strcat(result, "file = " ALT_BOOT_FILE "\n");
		}

		fs_write(fd, result, strlen(result));
		fs_close(fd);

		char isoldr_preset_file_name[NAME_MAX];
		memset(isoldr_preset_file_name, 0, NAME_MAX);

		snprintf(isoldr_preset_file_name, sizeof(isoldr_preset_file_name),
				 "%s/apps/%s/presets/%s",
				 GetDefaultDir(menu_data.current_dev), "iso_loader", preset->preset_file_name);

		// COPY TO ISO LOADER FOLDER
		CopyFile(preset_file_name, isoldr_preset_file_name, 0);
		saved = true;
	}

	return saved;
}

void LoadDefaultMenuConfig()
{
	strcpy(menu_data.app_config.games_path, GAMES_FOLDER);
	menu_data.app_config.initial_view = menu_data.menu_type = MT_PLANE_TEXT;
	menu_data.app_config.save_preset = 0;
	menu_data.app_config.cover_background = 1;
	menu_data.app_config.change_page_with_pad = 0;
	menu_data.app_config.start_in_last_game = 0;
	menu_data.app_config.enable_cache = 0;
	menu_data.app_config.last_device = 0;

	ThemeStruct theme = GetTheme(DEFAULT_THEME);
	menu_data.app_config.background_color = theme.background_color;
	menu_data.app_config.border_color = theme.border_color;
	menu_data.app_config.title_color = theme.title_color;
	menu_data.app_config.area_color = theme.area_color;
	menu_data.app_config.body_color = theme.body_color;
	menu_data.app_config.control_top_color = theme.control_top_color;
	menu_data.app_config.control_body_color = theme.control_body_color;
	menu_data.app_config.control_bottom_color = theme.control_bottom_color;

	memset(menu_data.app_config.last_game, 0, sizeof(menu_data.app_config.last_game));
}

void ParseStringColor(uint32 *current_color, const char *color)
{
	const int COLOR_LENGTH = 8;
	if (color[0] != '\0' && color[0] != ' ' && strlen(color) == COLOR_LENGTH)
	{
		*current_color = strtoul(color, NULL, 16);
	}
}

bool LoadMenuConfig()
{
	LoadDefaultMenuConfig();
	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "menu_games.cfg");

	char background_color[10] = {0};
	char border_color[10] = {0};
	char title_color[10] = {0};
	char body_color[10] = {0};
	char area_color[10] = {0};
	char control_top_color[10] = {0};
	char control_body_color[10] = {0};
	char control_bottom_color[10] = {0};

	GenericConfigStruct options[] =
		{
			{"games_path", CONF_STR, (void *)menu_data.app_config.games_path},
			{"initial_view", CONF_INT, (void *)&menu_data.app_config.initial_view},
			{"save_preset", CONF_INT, (void *)&menu_data.app_config.save_preset},
			{"cover_background", CONF_INT, (void *)&menu_data.app_config.cover_background},
			{"change_page_with_pad", CONF_INT, (void *)&menu_data.app_config.change_page_with_pad},
			{"start_in_last_game", CONF_INT, (void *)&menu_data.app_config.start_in_last_game},
			{"last_game", CONF_STR, (void *)menu_data.app_config.last_game},
			{"last_device", CONF_INT, (void *)&menu_data.app_config.last_device},
			{"background_color", CONF_STR, (void *)background_color},
			{"border_color", CONF_STR, (void *)border_color},
			{"title_color", CONF_STR, (void *)title_color},
			{"body_color", CONF_STR, (void *)body_color},
			{"area_color", CONF_STR, (void *)area_color},
			{"control_top_color", CONF_STR, (void *)control_top_color},
			{"control_body_color", CONF_STR, (void *)control_body_color},
			{"control_bottom_color", CONF_STR, (void *)control_bottom_color},
			{"enable_cache", CONF_INT, (void *)&menu_data.app_config.enable_cache}};

	if (ConfigParse(options, file_name) == -1)
	{
		ParseMenuConfigToPresentation();
		return false;
	}

	ParseStringColor(&menu_data.app_config.background_color, background_color);
	ParseStringColor(&menu_data.app_config.border_color, border_color);
	ParseStringColor(&menu_data.app_config.title_color, title_color);
	ParseStringColor(&menu_data.app_config.body_color, body_color);
	ParseStringColor(&menu_data.app_config.area_color, area_color);
	ParseStringColor(&menu_data.app_config.control_top_color, control_top_color);
	ParseStringColor(&menu_data.app_config.control_body_color, control_body_color);
	ParseStringColor(&menu_data.app_config.control_bottom_color, control_bottom_color);

	ParseMenuConfigToPresentation();
	return true;
}

int IsHexadecimal(const char *text)
{
	if (text == NULL)
		return 0;

	for (int i = 0; text[i] != '\0'; i++)
	{
		char c = text[i];
		if (!((c >= '0' && c <= '9') ||
			  (c >= 'a' && c <= 'f') ||
			  (c >= 'A' && c <= 'F')))
			return 0;
	}

	return 1;
}

const char *GetNameCurrentTheme()
{
	static char theme_name[16];
	ThemeStruct theme;
	memset(theme_name, 0, sizeof(theme_name));

	theme.background_color = plx_pack_color(menu_data.background_color.a,
											menu_data.background_color.r, menu_data.background_color.g, menu_data.background_color.b);

	theme.border_color = plx_pack_color(menu_data.border_color.a,
										menu_data.border_color.r, menu_data.border_color.g, menu_data.border_color.b);

	theme.title_color = plx_pack_color(menu_data.title_color.a,
									   menu_data.title_color.r, menu_data.title_color.g, menu_data.title_color.b);

	theme.area_color = plx_pack_color(menu_data.area_color.a,
									  menu_data.area_color.r, menu_data.area_color.g, menu_data.area_color.b);

	theme.body_color = plx_pack_color(menu_data.body_color.a,
									  menu_data.body_color.r, menu_data.body_color.g, menu_data.body_color.b);

	theme.control_top_color = plx_pack_color(menu_data.control_top_color.a,
											 menu_data.control_top_color.r, menu_data.control_top_color.g, menu_data.control_top_color.b);

	theme.control_body_color = plx_pack_color(menu_data.control_body_color.a,
											  menu_data.control_body_color.r, menu_data.control_body_color.g, menu_data.control_body_color.b);

	theme.control_bottom_color = plx_pack_color(menu_data.control_bottom_color.a,
												menu_data.control_bottom_color.r, menu_data.control_bottom_color.g, menu_data.control_bottom_color.b);

	for (int i = 0; i < sizeof(ThemeList) / sizeof(ThemeList[0]); i++)
	{
		if (ThemeList[i].background_color == theme.background_color &&
			ThemeList[i].body_color == theme.body_color &&
			ThemeList[i].title_color == theme.title_color &&
			ThemeList[i].border_color == theme.border_color &&
			ThemeList[i].control_top_color == theme.control_top_color &&
			ThemeList[i].control_body_color == theme.control_body_color &&
			ThemeList[i].control_bottom_color == theme.control_bottom_color)
		{
			strcpy(theme_name, ThemeList[i].theme);
			break;
		}
	}

	if (theme_name[0] == '\0')
		strcpy(theme_name, CUSTOM_THEME);

	return theme_name;
}

ThemeStruct GetTheme(const char *theme_name)
{
	ThemeStruct theme;

	if (strcasecmp(theme_name, CUSTOM_THEME) == 0)
	{
		if (menu_data.background_color.a == 0 && menu_data.border_color.a == 0 && menu_data.title_color.a == 0 && menu_data.title_color.a == 0 && menu_data.body_color.a == 0 && menu_data.area_color.a == 0)
		{
			theme = ThemeList[0];
		}
		else
		{
			theme.background_color = plx_pack_color(menu_data.background_color.a,
													menu_data.background_color.r, menu_data.background_color.g, menu_data.background_color.b);

			theme.border_color = plx_pack_color(menu_data.border_color.a,
												menu_data.border_color.r, menu_data.border_color.g, menu_data.border_color.b);

			theme.title_color = plx_pack_color(menu_data.title_color.a,
											   menu_data.title_color.r, menu_data.title_color.g, menu_data.title_color.b);

			theme.area_color = plx_pack_color(menu_data.area_color.a,
											  menu_data.area_color.r, menu_data.area_color.g, menu_data.area_color.b);

			theme.body_color = plx_pack_color(menu_data.body_color.a,
											  menu_data.body_color.r, menu_data.body_color.g, menu_data.body_color.b);

			theme.control_top_color = plx_pack_color(menu_data.control_top_color.a,
													 menu_data.control_top_color.r, menu_data.control_top_color.g, menu_data.control_top_color.b);

			theme.control_body_color = plx_pack_color(menu_data.control_body_color.a,
													  menu_data.control_body_color.r, menu_data.control_body_color.g, menu_data.control_body_color.b);

			theme.control_bottom_color = plx_pack_color(menu_data.control_bottom_color.a,
														menu_data.control_bottom_color.r, menu_data.control_bottom_color.g, menu_data.control_bottom_color.b);
		}
	}
	else
	{
		for (int i = 0; i < sizeof(ThemeList) / sizeof(ThemeList[0]); i++)
		{
			if (strcasecmp(theme_name, ThemeList[i].theme) == 0)
			{
				theme = ThemeList[i];
				break;
			}
		}
	}

	return theme;
}

void SetTheme(const char *theme_name)
{
	for (int i = 0; i < sizeof(ThemeList) / sizeof(ThemeList[0]); i++)
	{
		if (strcasecmp(theme_name, ThemeList[i].theme) == 0)
		{
			menu_data.background_color = ParseUIntToColor(ThemeList[i].background_color);
			menu_data.border_color = ParseUIntToColor(ThemeList[i].border_color);
			menu_data.title_color = ParseUIntToColor(ThemeList[i].title_color);
			menu_data.area_color = ParseUIntToColor(ThemeList[i].area_color);
			menu_data.body_color = ParseUIntToColor(ThemeList[i].body_color);
			menu_data.control_top_color = ParseUIntToColor(ThemeList[i].control_top_color);
			menu_data.control_body_color = ParseUIntToColor(ThemeList[i].control_body_color);
			menu_data.control_bottom_color = ParseUIntToColor(ThemeList[i].control_bottom_color);

			break;
		}
	}
}

void ParseMenuConfigToPresentation()
{
	TrimSlashes(menu_data.app_config.games_path);
	if (menu_data.app_config.games_path[0] != '\0' && menu_data.app_config.games_path[0] != ' ')
	{
		strcpy(menu_data.games_config_path, menu_data.app_config.games_path);
	}
	else
	{
		strcpy(menu_data.games_config_path, GAMES_FOLDER);
	}

	if (menu_data.app_config.initial_view > 0)
	{
		menu_data.menu_type = menu_data.app_config.initial_view;
	}
	else
	{
		menu_data.app_config.initial_view = menu_data.menu_type = MT_PLANE_TEXT;
	}

	menu_data.save_preset = (menu_data.app_config.save_preset == 1);
	menu_data.cover_background = (menu_data.app_config.cover_background == 1);
	menu_data.change_page_with_pad = (menu_data.app_config.change_page_with_pad == 1);
	menu_data.start_in_last_game = (menu_data.app_config.start_in_last_game == 1);
	strcpy(menu_data.last_game, menu_data.app_config.last_game);
	menu_data.last_device = menu_data.app_config.last_device;
	menu_data.enable_cache = (menu_data.app_config.enable_cache == 1);

	menu_data.background_color = ParseUIntToColor(menu_data.app_config.background_color);
	menu_data.border_color = ParseUIntToColor(menu_data.app_config.border_color);
	menu_data.title_color = ParseUIntToColor(menu_data.app_config.title_color);
	menu_data.area_color = ParseUIntToColor(menu_data.app_config.area_color);
	menu_data.body_color = ParseUIntToColor(menu_data.app_config.body_color);
	menu_data.control_top_color = ParseUIntToColor(menu_data.app_config.control_top_color);
	menu_data.control_body_color = ParseUIntToColor(menu_data.app_config.control_body_color);
	menu_data.control_bottom_color = ParseUIntToColor(menu_data.app_config.control_bottom_color);

	strcpy(menu_data.theme, GetNameCurrentTheme());
}

Color ParseUIntToColor(uint32 color)
{
	Color color_converted = {(float)(color >> 24 & 0xFF) / 255.0f, (float)(color >> 16 & 0xFF) / 255.0f, (float)(color >> 8 & 0xFF) / 255.0f, (float)(color >> 0 & 0xFF) / 255.0f};
	return color_converted;
}

void ParsePresentationToMenuConfig()
{
	memset(menu_data.app_config.games_path, 0, sizeof(menu_data.app_config.games_path));
	memset(menu_data.app_config.last_game, 0, sizeof(menu_data.app_config.last_game));

	TrimSlashes(menu_data.games_config_path);
	strcpy(menu_data.app_config.games_path, menu_data.games_config_path);
	menu_data.app_config.initial_view = menu_data.menu_type;
	menu_data.app_config.save_preset = (menu_data.save_preset ? 1 : 0);
	menu_data.app_config.cover_background = (menu_data.cover_background ? 1 : 0);
	menu_data.app_config.change_page_with_pad = (menu_data.change_page_with_pad ? 1 : 0);
	menu_data.app_config.start_in_last_game = (menu_data.start_in_last_game ? 1 : 0);
	menu_data.app_config.enable_cache = (menu_data.enable_cache ? 1 : 0);
	strcpy(menu_data.app_config.last_game, menu_data.last_game);
	menu_data.app_config.last_device = menu_data.last_device;

	menu_data.app_config.background_color = plx_pack_color(menu_data.background_color.a,
														   menu_data.background_color.r, menu_data.background_color.g, menu_data.background_color.b);

	menu_data.app_config.border_color = plx_pack_color(menu_data.border_color.a,
													   menu_data.border_color.r, menu_data.border_color.g, menu_data.border_color.b);

	menu_data.app_config.title_color = plx_pack_color(menu_data.title_color.a,
													  menu_data.title_color.r, menu_data.title_color.g, menu_data.title_color.b);

	menu_data.app_config.area_color = plx_pack_color(menu_data.area_color.a,
													 menu_data.area_color.r, menu_data.area_color.g, menu_data.area_color.b);

	menu_data.app_config.body_color = plx_pack_color(menu_data.body_color.a,
													 menu_data.body_color.r, menu_data.body_color.g, menu_data.body_color.b);

	menu_data.app_config.control_top_color = plx_pack_color(menu_data.control_top_color.a,
															menu_data.control_top_color.r, menu_data.control_top_color.g, menu_data.control_top_color.b);

	menu_data.app_config.control_body_color = plx_pack_color(menu_data.control_body_color.a,
															 menu_data.control_body_color.r, menu_data.control_body_color.g, menu_data.control_body_color.b);

	menu_data.app_config.control_bottom_color = plx_pack_color(menu_data.control_bottom_color.a,
															   menu_data.control_bottom_color.r, menu_data.control_bottom_color.g, menu_data.control_bottom_color.b);
}

bool SaveMenuConfig()
{
	if (menu_data.current_dev != APP_DEVICE_SD && menu_data.current_dev != APP_DEVICE_IDE)
	{
		return false;
	}

	file_t fd;
	char file_name[100];
	char result[1024];

	ParsePresentationToMenuConfig();

	snprintf(file_name, sizeof(file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "menu_games.cfg");
	fd = fs_open(file_name, O_CREAT | O_TRUNC | O_WRONLY);

	if (fd == FILEHND_INVALID)
	{
		return false;
	}

	snprintf(result, sizeof(result),
			 "games_path = %s\ninitial_view = %d\nsave_preset = %d\n"
			 "cover_background = %d\nchange_page_with_pad = %d\nstart_in_last_game = %d\nlast_game = %s\nlast_device = %d\n"
			 "background_color = %08lx\nborder_color = %08lx\ntitle_color = %08lx\nbody_color = %08lx\narea_color = %08lx\n"
			 "control_top_color = %08lx\ncontrol_body_color = %08lx\ncontrol_bottom_color = %08lx\n"
			 "enable_cache = %d",
			 menu_data.app_config.games_path, menu_data.app_config.initial_view, menu_data.app_config.save_preset,
			 menu_data.app_config.cover_background, menu_data.app_config.change_page_with_pad,
			 menu_data.app_config.start_in_last_game, menu_data.app_config.last_game,
			 menu_data.app_config.last_device, menu_data.app_config.background_color,
			 menu_data.app_config.border_color, menu_data.app_config.title_color,
			 menu_data.app_config.body_color, menu_data.app_config.area_color,
			 menu_data.app_config.control_top_color, menu_data.app_config.control_body_color,
			 menu_data.app_config.control_bottom_color, menu_data.enable_cache);

	fs_write(fd, result, strlen(result));
	fs_close(fd);

	return true;
}

bool SaveScannedCover()
{
	if (menu_data.current_dev != APP_DEVICE_SD && menu_data.current_dev != APP_DEVICE_IDE)
	{
		return false;
	}

	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "scanned_cover.log");

	file_t fd = fs_open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd == FILEHND_INVALID)
	{
		return false;
	}

	fs_write(fd, &menu_data.cover_scanned_app, sizeof(CoverScannedStruct));
	fs_close(fd);

	return true;
}

bool LoadScannedCover()
{
	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "scanned_cover.log");

	file_t fd = fs_open(file_name, O_RDONLY);
	if (fd == FILEHND_INVALID)
	{
		return false;
	}

	memset(&menu_data.cover_scanned_app, 0, sizeof(CoverScannedStruct));

	size_t file_size = fs_total(fd);
	if (file_size > 0)
	{
		unsigned char *buffer = (unsigned char *)malloc(sizeof(CoverScannedStruct));
		fs_read(fd, buffer, sizeof(CoverScannedStruct));
		memcpy(&menu_data.cover_scanned_app, buffer, sizeof(CoverScannedStruct));
		free(buffer);
	}
	fs_close(fd);

	if (file_size <= 0)
	{
		memset(&menu_data.cover_scanned_app, 0, sizeof(CoverScannedStruct));
		fs_unlink(file_name);
	}

	if (!HasAnyCover())
	{
		memset(&menu_data.cover_scanned_app, 0, sizeof(CoverScannedStruct));
		fs_unlink(file_name);
	}

	return true;
}

bool SaveCache()
{
	if ((menu_data.current_dev != APP_DEVICE_SD && menu_data.current_dev != APP_DEVICE_IDE) || menu_data.cache_array_count == 0)
	{
		return false;
	}

	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/games.cache");

	file_t fd = fs_open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd == FILEHND_INVALID)
	{
		return false;
	}

	int written = 0;
	while (written < menu_data.cache_array_count)
	{
		size_t block_to_write = (menu_data.cache_array_count - written > FILE_BLOCK_SIZE) ? FILE_BLOCK_SIZE : (menu_data.cache_array_count - written);
		size_t result = fs_write(fd, &menu_data.cache_array[written], sizeof(CacheStruct) * block_to_write);

		if (result != sizeof(CacheStruct) * block_to_write)
		{
			fs_close(fd);
			return false;
		}

		written += block_to_write;
	}

	fs_close(fd);
	return true;
}

bool LoadCache()
{
	if (menu_data.cache_array_count > 0)
		return false;

	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu", "games.cache");

	size_t total_read = 0;

	file_t fd = fs_open(file_name, O_RDONLY);
	if (fd == FILEHND_INVALID)
		return false;

	while (1)
	{
		CacheStruct *tmp = realloc(menu_data.cache_array, (total_read + FILE_BLOCK_SIZE) * sizeof(CacheStruct));
		if (tmp == NULL)
		{
			free(menu_data.cache_array);
			menu_data.cache_array = NULL;
			fs_close(fd);
			return false;
		}
		menu_data.cache_array = tmp;

		size_t num_read = fs_read(fd, menu_data.cache_array + total_read, sizeof(CacheStruct) * FILE_BLOCK_SIZE);
		if (num_read == 0)
			break;

		size_t items_read = num_read / sizeof(CacheStruct);
		total_read += items_read;

		if (items_read < FILE_BLOCK_SIZE)
			break;
	}

	fs_close(fd);

	if (total_read > 0)
	{
		CacheStruct *shrink_cache = realloc(menu_data.cache_array, total_read * sizeof(CacheStruct));
		if (shrink_cache != NULL)
			menu_data.cache_array = shrink_cache;

		menu_data.cache_array_count = (int)total_read;
	}

	return true;
}

void CleanIncompleteCover()
{
	if (menu_data.cover_scanned_app.last_game_status == CSE_PROCESSING)
	{
		char game_cover_path[NAME_MAX];
		uint16 image_count = 1;

		while (image_count < IMAGE_TYPE_SUPPORTED)
		{
			if (IMAGE_TYPE_SUPPORTED & image_count)
			{
				memset(game_cover_path, 0, sizeof(game_cover_path));
				snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s%s", GetCoversPath(menu_data.current_dev), menu_data.cover_scanned_app.last_game_scanned, GetCoverExtensionFromType(image_count));
				if (FileExists(game_cover_path))
				{
					fs_unlink(game_cover_path);
				}

				strcat(game_cover_path, ".tmp");
				if (FileExists(game_cover_path))
				{
					fs_unlink(game_cover_path);
				}
			}

			image_count = image_count << 1;
		}
	}
}

void OptimizeGameCovers()
{
	char *game_cover_path = NULL;
	char *image_type = NULL;
	char *game_without_extension = NULL;
	kos_img_t kimg;

	for (int icount = 0; icount < menu_data.games_array_count; icount++)
	{
		if (menu_data.stop_optimize_game_cover || menu_data.finished_menu)
			break;

		if (CheckCover(icount, MT_PLANE_TEXT) == SC_EXISTS)
		{
			GetCoverName(icount, &game_without_extension);
			menu_data.send_message_optimizer("Check COVER: %s", game_without_extension);

			if (CheckCover(icount, MT_IMAGE_TEXT_64_5X2) != SC_EXISTS || CheckCover(icount, MT_IMAGE_128_4X3) != SC_EXISTS)
			{
				if (GetGameCoverPath(icount, &game_cover_path, MT_PLANE_TEXT))
				{
					image_type = strrchr(game_cover_path, '.');

					if ((strcasecmp(image_type, ".png") == 0 && png_decode(game_cover_path, &kimg) == 0) || (strcasecmp(image_type, ".jpg") == 0 && jpg_decode(game_cover_path, &kimg) == 0))
					{
						menu_data.send_message_optimizer("Optimizing cover: %s", game_without_extension);
						OptimizeCover(icount, game_without_extension, &kimg, strcasecmp(image_type, ".png") == 0 ? true : false);
						kos_img_free(&kimg, 0);
					}

					free(game_cover_path);
					game_cover_path = NULL;
				}
			}

			menu_data.send_message_optimizer("%s", "");
			if (game_without_extension != NULL)
			{
				free(game_without_extension);
				game_without_extension = NULL;
			}
		}
	}

	image_type = NULL;
}

void OptimizeCover(int game_index, const char *game_name, kos_img_t *img, bool is_alpha)
{
	if (img != NULL && img->data)
	{
		bool optimize_to_128 = img->w > 128;
		bool optimize_to_64 = img->w > 64;
		char new_cover_path[NAME_MAX];
		char cover_folder[NAME_MAX];
		int image_size = 0;

		for (int16 imenu = 2; imenu <= MAX_MENU; imenu++)
		{
			if ((optimize_to_64 && imenu == MT_IMAGE_TEXT_64_5X2) || (optimize_to_128 && imenu == MT_IMAGE_128_4X3))
			{
				memset(cover_folder, 0, NAME_MAX);
				memset(new_cover_path, 0, NAME_MAX);

				image_size = (imenu == MT_IMAGE_TEXT_64_5X2 ? 64 : 128);

				snprintf(cover_folder, NAME_MAX, "%s%s", GetCoversPath(menu_data.current_dev), GetCoverFolder(imenu));
				if (DirExists(cover_folder) == 0)
				{
					fs_mkdir(cover_folder);
				}

				snprintf(new_cover_path, NAME_MAX, "%s/%s%s", cover_folder, game_name, is_alpha ? GetCoverExtensionFromType(IT_PNG) : GetCoverExtensionFromType(IT_JPG));
				if (copy_image_memory_to_file(img, new_cover_path, true, image_size, image_size))
				{
					CleanCoverType(game_index, imenu);
					SetCoverType(game_index, imenu, is_alpha ? IT_PNG : IT_JPG);
					menu_data.games_array[game_index].exists_cover[imenu - 1] = SC_EXISTS;
				}
			}
		}
	}
}

bool ExtractPVRCover(int game_index)
{
	bool extracted_cover = false;
	if (game_index >= 0 && (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE))
	{
		if (!menu_data.games_array[game_index].check_pvr || menu_data.games_array[game_index].exists_cover[MT_PLANE_TEXT - 1] != SC_DEFAULT)
		{
			return false;
		}

		char *game_without_extension = NULL;
		const char *full_game_path = GetFullGamePathByIndex(game_index);
		menu_data.games_array[game_index].check_pvr = false;

		GetCoverName(game_index, &game_without_extension);
		menu_data.send_message_scan("Mounting GAME: %s", game_without_extension);

		if (fs_iso_mount("/iso_cover", full_game_path) == 0)
		{
			char pvr_path[30];
			memset(pvr_path, 0, 30);
			strcpy(pvr_path, "/iso_cover/0GDTEX.PVR");

			menu_data.send_message_scan("Extracting PVR: %s", game_without_extension);
			menu_data.cover_scanned_app.last_game_status = CSE_PROCESSING;
			SaveScannedCover();

			kos_img_t kimg;
			if (pvr_decode(pvr_path, &kimg) == 0)
			{
				uint16 image_type = 0;
				if (kimg.fmt != TFM_RGB565 && menu_data.convert_pvr_to_png)
				{
					image_type = IT_PNG;
				}
				else
				{
					image_type = IT_JPG;
				}

				if (image_type > 0)
				{
					char game_cover_path[NAME_MAX];
					memset(game_cover_path, 0, NAME_MAX);
					snprintf(game_cover_path, NAME_MAX, "%s/%s%s", GetCoversPath(menu_data.current_dev), game_without_extension, GetCoverExtensionFromType(image_type));

					if ((image_type == IT_PNG && img_to_png(&kimg, game_cover_path, 0, 0)) || (image_type == IT_JPG && img_to_jpg(&kimg, game_cover_path, 0, 0, 100)))
					{
						menu_data.games_array[game_index].exists_cover[MT_PLANE_TEXT - 1] = SC_EXISTS;
						SetCoverType(game_index, MT_PLANE_TEXT, image_type);

						menu_data.send_message_scan("Optimizing cover: %s", game_without_extension);
						OptimizeCover(game_index, game_without_extension, &kimg, (image_type == IT_PNG));

						menu_data.games_array[game_index].is_pvr_cover = true;
						extracted_cover = true;
					}
				}

				kos_img_free(&kimg, 0);
				menu_data.cover_scanned_app.last_game_status = CSE_COMPLETED;
			}

			fs_iso_unmount("/iso_cover");
		}

		if (game_without_extension)
		{
			free(game_without_extension);
			game_without_extension = NULL;
		}
	}

	return extracted_cover;
}

void *OptimizeCoverThread(void *param)
{
	OptimizeGameCovers();
	menu_data.post_optimizer_cover();

	return NULL;
}

void *LoadPVRCoverThread(void *params)
{
	menu_data.cover_scanned_app.scan_count++;

	bool new_cover = false;
	if (menu_data.rescan_covers)
	{
		menu_data.rescan_covers = false;
		menu_data.cover_scanned_app.last_game_index = 1;
	}

	if (menu_data.cover_scanned_app.last_game_index == 0)
	{
		menu_data.cover_scanned_app.last_game_index = 1;
	}

	char *game_without_extension = NULL;
	for (int icount = menu_data.cover_scanned_app.last_game_index - 1; icount < menu_data.games_array_count; icount++)
	{
		if (menu_data.stop_load_pvr_cover || menu_data.finished_menu)
			break;

		GetCoverName(icount, &game_without_extension);

		memset(menu_data.cover_scanned_app.last_game_scanned, 0, sizeof(menu_data.cover_scanned_app.last_game_scanned));
		strncpy(menu_data.cover_scanned_app.last_game_scanned, game_without_extension, strlen(game_without_extension));

		menu_data.send_message_scan("Check game: %s", game_without_extension);

		if (CheckCover(icount, MT_PLANE_TEXT) == SC_DEFAULT)
		{
			// CHECK AGAIN TO SEE IF IT WAS NOT DOWNLOADED IN PVR
			menu_data.games_array[icount].exists_cover[MT_PLANE_TEXT - 1] = SC_WITHOUT_SEARCHING;
			if (CheckCover(icount, MT_PLANE_TEXT) == SC_DEFAULT)
			{
				ExtractPVRCover(icount);
				new_cover = true;
			}
			else
			{
				menu_data.games_array[icount].is_pvr_cover = true;
				menu_data.cover_scanned_app.last_game_status = CSE_EXISTS;
			}
		}
		else
		{
			menu_data.cover_scanned_app.last_game_status = CSE_EXISTS;
		}

		menu_data.cover_scanned_app.last_game_index = (uint32)icount + 1;
		SaveScannedCover();
	}

	if (game_without_extension != NULL)
	{
		free(game_without_extension);
		game_without_extension = NULL;
	}

	menu_data.post_pvr_cover(new_cover);

	return NULL;
}

static int AppCompareGames(const void *a, const void *b)
{
	const GameItemStruct *left = (const GameItemStruct *)a;
	const GameItemStruct *right = (const GameItemStruct *)b;
	int cmp = 0;

	char *left_compare = malloc(NAME_MAX);
	char *right_compare = malloc(NAME_MAX);

	memset(left_compare, 0, NAME_MAX);
	memset(right_compare, 0, NAME_MAX);

	if (left->is_folder_name && right->is_folder_name)
	{
		cmp = strcmp(left->folder_name, right->folder_name);
	}
	else if (left->is_folder_name)
	{
		strncpy(right_compare, right->game, strlen(right->game) - 4);
		cmp = strcmp(left->folder_name, right_compare);
	}
	else if (right->is_folder_name)
	{
		strncpy(left_compare, left->game, strlen(left->game) - 4);
		cmp = strcmp(left_compare, right->folder_name);
	}
	else
	{
		strncpy(left_compare, left->game, strlen(left->game) - 4);
		strncpy(right_compare, right->game, strlen(right->game) - 4);

		cmp = strcmp(left_compare, right_compare);
	}

	if (cmp == 0)
	{
		int left_device = (left->device == APP_DEVICE_IDE ? 1 : 2);
		int right_device = (right->device == APP_DEVICE_IDE ? 1 : 2);
		cmp = left_device - right_device;
	}

	free(left_compare);
	free(right_compare);

	return cmp;
}

bool CheckGdiOptimized(int game_index)
{
	bool optimized = false;

	if (game_index >= 0)
	{
		if (!menu_data.games_array[game_index].checked_optimized)
		{
			menu_data.games_array[game_index].checked_optimized = true;
			optimized = menu_data.games_array[game_index].is_gdi_optimized = IsGdiOptimized(GetFullGamePathByIndex(game_index));
		}
		else
		{
			optimized = menu_data.games_array[game_index].is_gdi_optimized;
		}
	}

	return optimized;
}

bool HasAnyCover()
{
	bool any_cover = false;
	file_t fd = fs_open(GetCoversPath(menu_data.current_dev), O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return false;

	char *file_type = NULL;
	dirent_t *ent = NULL;

	while ((ent = fs_readdir(fd)) != NULL)
	{
		if (ent->name[0] == '.')
			continue;

		file_type = strrchr(ent->name, '.');
		if (strcasecmp(file_type, ".jpg") == 0 || strcasecmp(file_type, ".png") == 0 || strcasecmp(file_type, ".pvr") == 0)
		{
			any_cover = true;
			break;
		}
	}
	ent = NULL;
	fs_close(fd);

	return any_cover;
}

static int AppCompareCovers(const void *a, const void *b)
{
	const CoverStruct *left = (const CoverStruct *)a;
	const CoverStruct *right = (const CoverStruct *)b;
	char right_without_extension[NAME_MAX];
	char left_without_extension[NAME_MAX];

	strncpy(left_without_extension, left->cover, strlen(left->cover) - 4);
	left_without_extension[strlen(left->cover) - 4] = '\0';

	strncpy(right_without_extension, right->cover, strlen(right->cover) - 4);
	right_without_extension[strlen(right->cover) - 4] = '\0';

	return strcasecmp(left_without_extension, right_without_extension);
}

uint8 GetImageType(const char *filename)
{
	const char *file_type = strrchr(filename, '.');
	if (!file_type)
		return 0;

	static const struct
	{
		const char *ext;
		uint8 type;
	} ext_map[] = {
		{".png", IT_PNG},
		{".jpg", IT_JPG},
		{".bpm", IT_BPM},
		{".pvr", IT_PVR},
		{".kmg", IT_KMG},
	};

	for (int i = 0; i < sizeof(ext_map) / sizeof(ext_map[0]); i++)
	{
		if (strcasecmp(file_type, ext_map[i].ext) == 0)
			return ext_map[i].type;
	}

	return 0;
}

void UpdateCover(CoverStruct *cover, int menu_type, uint8 device, uint8 image_type)
{
	cover->menu_type |= (1 << (menu_type - 1));
	cover->image_type[menu_type - 1] |= image_type;
	cover->device |= (1 << (device - 1));
}

static int CompareCoversWithoutExtension(const void *a, const void *b)
{
	const char *key = (const char *)a;
	const CoverStruct *cover = (const CoverStruct *)b;
	char name_without_extension[NAME_MAX];

	strncpy(name_without_extension, cover->cover, strlen(cover->cover) - 4);
	name_without_extension[strlen(cover->cover) - 4] = '\0';

	return strcasecmp(key, name_without_extension);
}

void RetrieveCovers(uint8 device, int menu_type)
{
	if (menu_type <= 0)
		return;

	char game_cover_path[NAME_MAX];
	snprintf(game_cover_path, NAME_MAX, "%s%s", GetCoversPath(device), GetCoverFolder(menu_type));

	file_t fd = fs_open(game_cover_path, O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return;

	dirent_t *ent = NULL;
	char cover[NAME_MAX];
	CoverStruct *covers_array = NULL;
	int covers_array_count = 0;
	int covers_array_capacity = 256;

	covers_array = malloc(covers_array_capacity * sizeof(CoverStruct));

	// GET ALL COVERS BY MENU TYPE
	while ((ent = fs_readdir(fd)) != NULL)
	{
		if (ent->name[0] == '.')
			continue;

		uint8 image_type = GetImageType(ent->name);
		if (image_type == 0)
			continue;

		strncpy(cover, ent->name, NAME_MAX - 1);
		cover[NAME_MAX - 1] = '\0';

		if (covers_array_count >= covers_array_capacity)
		{
			covers_array_capacity *= 2;
			covers_array = realloc(covers_array, covers_array_capacity * sizeof(CoverStruct));
		}

		CoverStruct *new_cover = &covers_array[covers_array_count++];
		memset(new_cover, 0, sizeof(CoverStruct));

		strcpy(new_cover->cover, cover);
		UpdateCover(new_cover, menu_type, device, image_type);
	}
	fs_close(fd);

	if (covers_array_count > 0 && menu_data.games_array_count > 0)
	{
		qsort(covers_array, covers_array_count, sizeof(CoverStruct), AppCompareCovers);

		for (int icount = 0; icount < menu_data.games_array_count; icount++)
		{
			GameItemStruct *game_item = &menu_data.games_array[icount];
			char game_without_extension[NAME_MAX] = {0};

			if (game_item->is_folder_name)
				strcpy(game_without_extension, game_item->folder_name);
			else
				strncpy(game_without_extension, game_item->game, strlen(game_item->game) - 4);

			CoverStruct *cover_found = bsearch(game_without_extension, covers_array, covers_array_count,
											   sizeof(CoverStruct), CompareCoversWithoutExtension);

			if (cover_found)
			{
				if (game_item->cover.cover[0] == '\0')
					memcpy(&game_item->cover, cover_found, sizeof(CoverStruct));

				UpdateCover(&game_item->cover, menu_type, device, GetImageType(cover_found->cover));
			}
		}
	}

	free(covers_array);
}

static int CategoryCompare(CategoryStruct *a, CategoryStruct *b)
{
    return strcmp(a->category, b->category);
}

void CreateCategories()
{
	static bool category_flag = false;

	if (category_flag)
		return;

	category_flag = true;
	char parent_folder[NAME_MAX];

	for (int i = 0; i < menu_data.games_array_count; i++)
	{
		GameItemStruct *game_item = &menu_data.games_array[i];
		const char *category_name;

		if (game_item->folder == NULL)
		{
			continue;
		}

		if (game_item->is_folder_name)
		{
			memset(parent_folder, 0, NAME_MAX);
			GoUpDirectory(game_item->folder, 1, parent_folder);

			if (strlen(parent_folder) == 1)
			{
				continue;
			}

			category_name = GetLastPart(parent_folder, '/', 0);
		}
		else
			category_name = GetLastPart(game_item->folder, '/', 0);

		if (!category_name)
		{
			continue;
		}

		CategoryStruct *category = NULL;
		HASH_FIND_STR(menu_data.categories_array, category_name, category);

		if (category == NULL)
		{
			category = (CategoryStruct *)calloc(1, sizeof(CategoryStruct));
			if (!category)
			{
				continue; // YOU MUST NOT PASS THIS WAY
			}

			strncpy(category->category, category_name, sizeof(category->category) - 1);
			category->category[sizeof(category->category) - 1] = '\0';

			category->games_index = NULL;
			HASH_ADD_STR(menu_data.categories_array, category, category);
			menu_data.categories_array_count++;
		}

		int needed_count = category->games_count + 1;
		int current_capacity = category->games_index
								   ? ((category->games_count + CATEGORY_BLOCK_SIZE - 1) / CATEGORY_BLOCK_SIZE) * CATEGORY_BLOCK_SIZE
								   : 0;

		if (needed_count > current_capacity)
		{
			int new_capacity = current_capacity + CATEGORY_BLOCK_SIZE;
			int *new_array = realloc(category->games_index, sizeof(int) * new_capacity);
			if (!new_array)
			{
				continue; // YOU MUST NOT PASS THIS WAY
			}
			category->games_index = new_array;
		}

		category->games_index[category->games_count++] = i;
	}

	CategoryStruct *current_category, *tmp_category;
	HASH_ITER(hh, menu_data.categories_array, current_category, tmp_category)
	{
		if (current_category->games_count > 0)
		{
			int *shrunk_array = realloc(current_category->games_index, sizeof(int) * current_category->games_count);
			if (shrunk_array)
				current_category->games_index = shrunk_array;
		}
	}

	if (menu_data.categories_array_count > 0)
	{
		HASH_SORT(menu_data.categories_array, CategoryCompare);
	}
}

void RetrieveGamesByCategory(const char *category_name)
{
	if (menu_data.games_category_array != NULL)
	{
		free(menu_data.games_category_array);
		menu_data.games_category_array = NULL;
		menu_data.games_category_array_count = 0;
	}

	CategoryStruct *category = NULL;
	HASH_FIND_STR(menu_data.categories_array, category_name, category);

	if (category)
	{
		GameItemStruct **game_tmp = realloc(menu_data.games_category_array, category->games_count * sizeof(GameItemStruct *));
		if (game_tmp == NULL)
		{
			return;
		}

		menu_data.games_category_array = game_tmp;
		for (int i = 0; i < category->games_count; i++)
		{
			int game_index = category->games_index[i];
			menu_data.games_array[game_index].game_index_tmp = game_index;
			menu_data.games_category_array[i] = &menu_data.games_array[game_index];
		}

		menu_data.games_category_array_count = category->games_count;
	}
}

static int CompareGameNames(const char *left, const CacheStruct *right)
{
	if (right->attributes & GAE_IS_FOLDER_NAME)
	{
		char folder_game[NAME_MAX] = {0};
		GoUpDirectory(right->game, 1, folder_game);

		return strcasecmp(left, folder_game);
	}
	else
	{
		return strcasecmp(left, right->game);
	}
}

static int CompareGamePaths(const void *a, const void *b)
{
	const char *string = (const char *)a;
	const CacheStruct *cache = (const CacheStruct *)b;

	return CompareGameNames(string, cache);
}

static int CompareCacheStructs(const void *a, const void *b)
{
	const CacheStruct *cache_left = (const CacheStruct *)a;
	const CacheStruct *cache_right = (const CacheStruct *)b;

	char game_left[NAME_MAX] = {0};
	if (cache_left->attributes & GAE_IS_FOLDER_NAME)
	{
		GoUpDirectory(cache_left->game, 1, game_left);
	}
	else
	{
		strcpy(game_left, cache_left->game);
	}

	return CompareGameNames(game_left, cache_right);
}

static int ComparePaths(const void *a, const void *b)
{
	const char *path_a = *(const char **)a;
	const char *path_b = *(const char **)b;
	return strcmp(path_a, path_b);
}

void RemoveStaleCache()
{
	if (menu_data.games_array_count == 0)
		return;

	char **valid_paths = malloc(menu_data.games_array_count * sizeof(char *));
	if (!valid_paths)
		return;

	for (int i = 0; i < menu_data.games_array_count; i++)
	{
		const char *path = GetFullGamePathByIndex(i);
		valid_paths[i] = StrdupSafe(path);
	}

	qsort(valid_paths, menu_data.games_array_count, sizeof(char *), ComparePaths);

	int write_index = 0;
	for (int i = 0; i < menu_data.cache_array_count; i++)
	{
		char *game_path = menu_data.cache_array[i].game;
		char **found = (char **)bsearch(&game_path, valid_paths, menu_data.games_array_count, sizeof(char *), ComparePaths);

		if (found)
		{
			if (write_index != i)
				menu_data.cache_array[write_index] = menu_data.cache_array[i];
			write_index++;
		}
	}

	menu_data.cache_array_count = write_index;

	if (menu_data.cache_array_count > 0)
	{
		CacheStruct *shrink_ptr = realloc(menu_data.cache_array, menu_data.cache_array_count * sizeof(CacheStruct));
		if (shrink_ptr)
			menu_data.cache_array = shrink_ptr;
	}

	for (int i = 0; i < menu_data.games_array_count; i++)
	{
		free(valid_paths[i]);
	}

	free(valid_paths);
}

void AddGameToCache(int game_index)
{
	if (game_index < 0)
		return;

	GameItemStruct *game_item = &menu_data.games_array[game_index];
	int required_count = menu_data.cache_array_count + 1;
	int required_blocks = (required_count + CACHE_BLOCK_SIZE - 1) / CACHE_BLOCK_SIZE;
	int new_alloc_size = required_blocks * CACHE_BLOCK_SIZE;

	if (menu_data.cache_array == NULL)
	{
		menu_data.cache_array = malloc(new_alloc_size * sizeof(CacheStruct));
		if (menu_data.cache_array == NULL)
			return;
	}
	else if (required_count > (menu_data.cache_array_count / CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE)
	{
		void *temp = realloc(menu_data.cache_array, new_alloc_size * sizeof(CacheStruct));
		if (temp == NULL)
			return;
		menu_data.cache_array = temp;
	}

	CacheStruct cache;
	memset(&cache, 0, sizeof(CacheStruct));

	const char *game_path = GetFullGamePathByIndex(game_index);
	strncpy(cache.game, game_path, sizeof(cache.game) - 1);
	cache.game[sizeof(cache.game) - 1] = '\0';

	if (game_item->is_folder_name)
		cache.attributes |= GAE_IS_FOLDER_NAME;

	if (game_item->is_gdi_optimized)
		cache.attributes |= GAE_IS_GDI_OPTIMIZED;

	if (game_item->is_cdda == CCGE_CDDA)
		cache.attributes |= GAE_IS_CDDA;

	menu_data.cache_array[menu_data.cache_array_count] = cache;
	menu_data.cache_array_count++;
}

void PopulateCache()
{
	FreeCache();

	for (int icount = 0; icount < menu_data.games_array_count; icount++)
	{
		GameItemStruct *game_item = &menu_data.games_array[icount];

		if (menu_data.cache_array_count % CACHE_BLOCK_SIZE == 0)
		{
			menu_data.cache_array = realloc(menu_data.cache_array, (menu_data.cache_array_count + CACHE_BLOCK_SIZE) * sizeof(CacheStruct));
		}

		CacheStruct *cache = &menu_data.cache_array[menu_data.cache_array_count];
		menu_data.cache_array_count++;
		memset(cache, 0, sizeof(CacheStruct));

		strcpy(cache->game, GetFullGamePathByIndex(icount));

		if (game_item->is_folder_name)
			cache->attributes |= GAE_IS_FOLDER_NAME;

		if (game_item->is_gdi_optimized)
			cache->attributes |= GAE_IS_GDI_OPTIMIZED;

		if (game_item->is_cdda == CCGE_CDDA)
			cache->attributes |= GAE_IS_CDDA;
	}

	if (menu_data.cache_array_count > 0)
	{
		CacheStruct *shrink_ptr = realloc(menu_data.cache_array, menu_data.cache_array_count * sizeof(CacheStruct));
		if (shrink_ptr)
			menu_data.cache_array = shrink_ptr;

		qsort(menu_data.cache_array, menu_data.cache_array_count, sizeof(CacheStruct), CompareCacheStructs);
	}
}

GameItemStruct *FindInCache(const char *game_path)
{
	GameItemStruct *game_item = NULL;
	CacheStruct *cache_item = (CacheStruct *)bsearch(game_path, menu_data.cache_array, menu_data.cache_array_count, sizeof(CacheStruct), CompareGamePaths);

	if (cache_item != NULL)
	{
		game_item = malloc(sizeof(GameItemStruct));
		memset(game_item, 0, sizeof(GameItemStruct));
		game_item->device = GetDeviceType(cache_item->game);
		game_item->exists_cover[MT_PLANE_TEXT - 1] = SC_WITHOUT_SEARCHING;
		game_item->exists_cover[MT_IMAGE_TEXT_64_5X2 - 1] = SC_WITHOUT_SEARCHING;
		game_item->exists_cover[MT_IMAGE_128_4X3 - 1] = SC_WITHOUT_SEARCHING;
		game_item->checked_optimized = true;
		game_item->game_index_tmp = -1;
		game_item->is_folder_name = (cache_item->attributes & GAE_IS_FOLDER_NAME ? true : false);
		game_item->is_gdi_optimized = (cache_item->attributes & GAE_IS_GDI_OPTIMIZED ? true : false);
		game_item->is_cdda = (cache_item->attributes & GAE_IS_CDDA ? CCGE_CDDA : CCGE_NOT_CDDA);
		memset(&game_item->cover, 0, sizeof(CoverStruct));

		const char *game_file = GetFileName(cache_item->game);

		char game_path_withouth_base[NAME_MAX] = {0};
		const char *base_path = GetGamesPath(game_item->device);
		snprintf(game_path_withouth_base, NAME_MAX, "%s", cache_item->game + strlen(base_path) + 1);

		if (game_item->is_folder_name)
		{
			game_path_withouth_base[strlen(game_path_withouth_base) - strlen(game_file) - 1] = '\0';
			game_item->folder = StrdupSafe(game_path_withouth_base);
		}
		else if (strcasecmp(game_path_withouth_base, game_file) == 0)
		{
			memset(game_item->folder, 0, NAME_MAX);
		}
		else
		{
			game_path_withouth_base[strlen(game_path_withouth_base) - strlen(game_file) - 1] = '\0';
			game_item->folder = StrdupSafe(game_path_withouth_base);
		}

		if (game_item->is_folder_name)
		{
			char folder_game[NAME_MAX] = {0};
			GoUpDirectory(cache_item->game, 1, folder_game);

			const char *last = GetLastPart(folder_game, '/', 0);
			game_item->folder_name = StrdupSafe(last);
		}

		game_item->game = StrdupSafe(GetFileName(game_file));
	}

	return game_item;
}

void CopyDeepGame(const GameItemStruct *src, GameItemStruct *dest)
{
	memset(dest, 0, sizeof(GameItemStruct));
	dest->game_index_tmp = src->game_index_tmp;
	dest->device = src->device;
	dest->game = StrdupSafe(src->game);
	dest->folder = StrdupSafe(src->folder);
	dest->folder_name = StrdupSafe(src->folder_name);
	dest->is_folder_name = src->is_folder_name;
	memcpy(dest->exists_cover, src->exists_cover, sizeof(src->exists_cover));
	dest->cover_type = src->cover_type;
	dest->check_pvr = src->check_pvr;
	dest->is_pvr_cover = src->is_pvr_cover;
	dest->checked_optimized = src->checked_optimized;
	dest->is_gdi_optimized = src->is_gdi_optimized;
	dest->is_cdda = src->is_cdda;
	dest->cover = src->cover;
}

void RetrieveGamesRecursive()
{
	if (menu_data.cache_array_count > 0 && menu_data.enable_cache)
	{
		qsort(menu_data.cache_array, menu_data.cache_array_count, sizeof(CacheStruct), CompareCacheStructs);
	}

	char temp_folder_path[NAME_MAX];
	char folder_game[NAME_MAX];
	char upper_game_name[NAME_MAX];

	int *indexes_to_cache = NULL;
	int cache_count = 0;
	int cache_capacity = 0;

	if (menu_data.enable_cache)
	{
		indexes_to_cache = malloc(CACHE_BLOCK_SIZE * sizeof(int));
		cache_capacity = CACHE_BLOCK_SIZE;
	}

	int stack_capacity = TRAMPOLINE_BLOCK_SIZE;
	int stack_top = 0;
	
	const char *initial_paths[2] = { NULL, NULL };
	int path_count = 0;

	initial_paths[path_count++] = GetGamesPath(menu_data.current_dev);

	if (menu_data.ide && menu_data.sd && menu_data.current_dev != APP_DEVICE_SD)
	{
		initial_paths[path_count++] = GetGamesPath(APP_DEVICE_SD);
	}

	for (int i = 0; i < path_count; i++)
	{
		const char *initial_path = initial_paths[i];

		if (!initial_path || !initial_path[0])
			continue;

		stack_top = 0;
		GameTrampolineStruct *stack_trampoline = malloc(stack_capacity * sizeof(GameTrampolineStruct));

		snprintf(stack_trampoline[stack_top].full_path_folder, NAME_MAX, "%s", initial_path);
		memset(stack_trampoline[stack_top].folder, '\0', sizeof(stack_trampoline[stack_top].folder));
		stack_trampoline[stack_top].level = 0;
		stack_top++;

		while (stack_top > 0)
		{
			stack_top--;
			GameTrampolineStruct frame = stack_trampoline[stack_top];

			file_t fd = fs_open(frame.full_path_folder, O_RDONLY | O_DIR);
			if (fd == FILEHND_INVALID)
				continue;

			dirent_t *ent = NULL;
			bool checked_unique_file = false;
			int gdi_index = -1;
			int unique_file = 0;
			int unique_file_index = -1;

			bool checked_optimized = false, gdi_optimized = false;
			int gdi_cdda = CCGE_NOT_CHECKED;

			GameItemStruct *game_cache = NULL;
			bool exists_cache = false;

			while ((ent = fs_readdir(fd)) != NULL)
			{
				if (ent->name[0] == '.')
					continue;

				if (strlen(frame.full_path_folder) + strlen(ent->name) >= NAME_MAX)
					continue;

				memset(temp_folder_path, 0, sizeof(temp_folder_path));
				snprintf(temp_folder_path, NAME_MAX, "%s/%s", frame.full_path_folder, ent->name);

				const char *file_type = strrchr(ent->name, '.');
				bool is_valid_game = false;
				bool is_folder_name = false;
				exists_cache = false;
				upper_game_name[0] = '\0';

				if (menu_data.enable_cache &&
					(ent->attr == O_DIR || (file_type && (EndsWith(ent->name, ".gdi") || EndsWith(ent->name, ".cdi") || EndsWith(ent->name, ".cso") || (EndsWith(ent->name, ".iso") && strncasecmp(ent->name, "track", 5) != 0)))))
				{
					game_cache = FindInCache(temp_folder_path);

					if (game_cache != NULL)
					{
						exists_cache = true;
						is_valid_game = true;
					}
				}

				if (!exists_cache)
				{
					// GDI
					if (file_type && EndsWith(ent->name, ".gdi"))
					{
						strcpy(upper_game_name, ent->name);
						is_folder_name = frame.level > 0;
						is_valid_game = true;
					}
					else if (file_type && (EndsWith(ent->name, ".cdi") ||
										(EndsWith(ent->name, ".iso") && strncasecmp(ent->name, "track", 5) != 0) ||
										EndsWith(ent->name, ".cso")))
					{
						strcpy(upper_game_name, ent->name);
						is_folder_name = (frame.level > 0 && unique_file == 0);
						is_valid_game = true;
					}
					else if (strncasecmp(ent->name, "track", 5) == 0)
					{
						if (!checked_optimized && strncasecmp(ent->name, "track03", 7))
						{
							checked_optimized = true;
							if (strcasecmp(file_type, ".iso") == 0)
								gdi_optimized = true;
						}

						if (gdi_cdda != CCGE_CDDA)
						{
							if (strncasecmp(ent->name, "track06.wav", 11) == 0 || strncasecmp(ent->name, "track06.raw", 11) == 0)
								gdi_cdda = CCGE_CDDA;
							else if (strncasecmp(ent->name, "track04.wav", 11) == 0 || strncasecmp(ent->name, "track04.raw", 11) == 0)
								gdi_cdda = CCGE_CANDIDATE;
						}

						continue;
					}
					else if (ent->attr == O_DIR)
					{
						if (stack_top >= stack_capacity)
						{
							stack_capacity += TRAMPOLINE_BLOCK_SIZE;
							GameTrampolineStruct *new_stack = realloc(stack_trampoline, stack_capacity * sizeof(GameTrampolineStruct));
							stack_trampoline = new_stack;
						}

						snprintf(stack_trampoline[stack_top].full_path_folder, NAME_MAX, "%s", temp_folder_path);
						snprintf(stack_trampoline[stack_top].folder, NAME_MAX, "%s", ent->name);
						stack_trampoline[stack_top].level = frame.level + 1;
						stack_top++;
						continue;
					}
				}

				if (is_valid_game)
				{
					for (char *c = upper_game_name; *c; ++c)
						*c = TO_UPPER_SAFE(*c);

					if (menu_data.games_array_count % GAME_BLOCK_SIZE == 0)
					{
						GameItemStruct *tmp = realloc(menu_data.games_array, (menu_data.games_array_count + GAME_BLOCK_SIZE) * sizeof(GameItemStruct));
						menu_data.games_array = tmp;
					}

					GameItemStruct *game_item = &menu_data.games_array[menu_data.games_array_count++];

					if (game_cache != NULL)
					{
						CopyDeepGame(game_cache, game_item);

						if (game_cache->game)
							free(game_cache->game);
						if (game_cache->folder)
							free(game_cache->folder);
						if (game_cache->folder_name)
							free(game_cache->folder_name);

						free(game_cache);
						game_cache = NULL;

						continue;
					}

					memset(game_item, 0, sizeof(GameItemStruct));
					game_item->game = StrdupSafe(upper_game_name);
					game_item->is_folder_name = is_folder_name;
					game_item->exists_cover[MT_PLANE_TEXT - 1] = SC_WITHOUT_SEARCHING;
					game_item->exists_cover[MT_IMAGE_TEXT_64_5X2 - 1] = SC_WITHOUT_SEARCHING;
					game_item->exists_cover[MT_IMAGE_128_4X3 - 1] = SC_WITHOUT_SEARCHING;
					game_item->checked_optimized = false;
					game_item->is_cdda = CCGE_NOT_CHECKED;
					game_item->game_index_tmp = -1;
					game_item->device = GetDeviceType(frame.full_path_folder);
					memset(&game_item->cover, 0, sizeof(CoverStruct));

					if (frame.folder[0])
					{
						const char *base_path = GetGamesPath(game_item->device);
						snprintf(folder_game, NAME_MAX, "%s", frame.full_path_folder + strlen(base_path) + 1);

						for (char *c = folder_game; *c; ++c)
							*c = TO_UPPER_SAFE(*c);

						game_item->folder = StrdupSafe(folder_game);

						if (is_folder_name)
						{
							const char *last = GetLastPart(folder_game, '/', 0);
							game_item->folder_name = StrdupSafe(last);
						}
					}

					if (file_type && strcasecmp(file_type, ".gdi") == 0)
					{
						gdi_index = menu_data.games_array_count - 1;
					}
					else
					{
						if (unique_file_index == -1)
							unique_file_index = menu_data.games_array_count - 1;

						if (!checked_unique_file && unique_file_index >= 0 && unique_file > 0)
						{
							checked_unique_file = true;
							menu_data.games_array[unique_file_index].is_folder_name = false;
							free(menu_data.games_array[unique_file_index].folder_name);
							menu_data.games_array[unique_file_index].folder_name = NULL;
						}

						game_item->is_cdda = CCGE_NOT_CDDA;
					}

					if (menu_data.enable_cache && !exists_cache)
					{
						if (cache_count >= cache_capacity)
						{
							cache_capacity += CACHE_BLOCK_SIZE;
							int *new_indexes = realloc(indexes_to_cache, cache_capacity * sizeof(int));
							indexes_to_cache = new_indexes;
						}

						if (indexes_to_cache)
						{
							indexes_to_cache[cache_count++] = menu_data.games_array_count - 1;
						}
					}

					unique_file++;
				}
			}
			fs_close(fd);

			// GDI FINAL HANDLING
			if (gdi_index >= 0)
			{
				GameItemStruct *gdi_item = &menu_data.games_array[gdi_index];
				gdi_item->checked_optimized = true;
				gdi_item->is_gdi_optimized = gdi_optimized;

				if (gdi_cdda == CCGE_CANDIDATE)
				{
					char *track_file_path = malloc(NAME_MAX);
					const char *full_path_game = GetFullGamePathByIndex(gdi_index);
					size_t track_size = GetCDDATrackFilename(4, full_path_game, &track_file_path);
					free(track_file_path);

					if (track_size < 30 * 1024 * 1024 && track_size > 0)
						gdi_cdda = CCGE_NOT_CDDA;
					else
						gdi_cdda = CCGE_CDDA;
				}

				gdi_item->is_cdda = (gdi_cdda == CCGE_CDDA || gdi_cdda == CCGE_CANDIDATE) ? CCGE_CDDA : CCGE_NOT_CDDA;
			}
		}

		free(stack_trampoline);
	}

	if (indexes_to_cache != NULL)
	{
		for (int i = 0; i < cache_count; i++)
		{
			AddGameToCache(indexes_to_cache[i]);
		}
		free(indexes_to_cache);

		if (menu_data.cache_array_count > 0)
		{
			void *shrink_ptr = realloc(menu_data.cache_array, menu_data.cache_array_count * sizeof(CacheStruct));
			if (shrink_ptr)
				menu_data.cache_array = shrink_ptr;
		}

		qsort(menu_data.cache_array, menu_data.cache_array_count, sizeof(CacheStruct), CompareCacheStructs);
	}

	if (menu_data.games_array && menu_data.games_array_count > 0)
	{
		GameItemStruct *final_array = realloc(menu_data.games_array, menu_data.games_array_count * sizeof(GameItemStruct));
		if (final_array)
			menu_data.games_array = final_array;
	}
}

GameItemStruct *GetGamePtrByIndex(int game_index)
{
	GameItemStruct *game_ptr = NULL;

	if (game_index >= 0)
	{
		game_ptr = menu_data.category[0] != '\0' ? menu_data.games_array_ptr[game_index] : (*menu_data.games_array_ptr + game_index);
	}

	return game_ptr;
}

void SetLastGamePlayed()
{
	if (menu_data.start_in_last_game)
	{
		for (int icount = 0; icount < menu_data.games_array_count; icount++)
		{
			if (menu_data.app_config.last_device == menu_data.games_array[icount].device)
			{
				if ((menu_data.games_array[icount].is_folder_name && strcasecmp(menu_data.app_config.last_game, menu_data.games_array[icount].folder_name) == 0) ||
					(!menu_data.games_array[icount].is_folder_name && strcasecmp(menu_data.app_config.last_game, menu_data.games_array[icount].game) == 0))
				{
					menu_data.last_game_played_index = icount;
					break;
				}
			}
		}
	}
}

bool RetrieveGames()
{
	FreeGames();
	FreeCache();

	// LOAD CACHE FROM FILE
	if (menu_data.enable_cache)
	{
		LoadCache();
	}

	RetrieveGamesRecursive();

	if (menu_data.last_game_played_index > menu_data.games_array_count - 1)
	{
		menu_data.last_game_played_index = -1;
		menu_data.last_device = 0;
		memset(menu_data.last_game, 0, sizeof(menu_data.last_game));
	}

	if (menu_data.cache_array_count > 0 && menu_data.enable_cache)
	{
		RemoveStaleCache();
		SaveCache();
	}

	if (menu_data.games_array_count > 0)
	{
		qsort(menu_data.games_array, menu_data.games_array_count, sizeof(GameItemStruct), AppCompareGames);
		SetLastGamePlayed();

		menu_data.rescan_covers = false;
		if (menu_data.cover_scanned_app.games_count != menu_data.games_array_count)
		{
			menu_data.cover_scanned_app.games_count = menu_data.games_array_count;
			menu_data.rescan_covers = true;
		}
		else if (menu_data.cover_scanned_app.games_count > 0 && menu_data.cover_scanned_app.last_game_index > 0 && menu_data.cover_scanned_app.last_game_index <= menu_data.games_array_count)
		{
			char *name = NULL;
			if (GetCoverName(menu_data.cover_scanned_app.last_game_index - 1, &name))
			{
				if (strcasecmp(menu_data.cover_scanned_app.last_game_scanned, name) != 0)
				{
					menu_data.rescan_covers = true;
				}

				free(name);
			}
		}

		return true;
	}

	return false;
}