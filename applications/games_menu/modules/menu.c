#include "app_menu.h"
#include <img/convert.h>
#include <isoldr.h>
#include <time.h>

struct menu_structure menu_data;

void CreateMenuData(SendMessageCallBack *send_message_scan, PostPVRCoverCallBack *post_pvr_cover)
{
	menu_data.games_array_count = 0;
	menu_data.play_cdda_thread = NULL;
	menu_data.load_pvr_cover_thread = NULL;
	menu_data.finished_menu = false;
    menu_data.cdda_game_changed = false;
    menu_data.stop_load_pvr_cover = false;		

	menu_data.send_message_scan = send_message_scan;
	menu_data.post_pvr_cover = post_pvr_cover;

	menu_data.games_array = NULL;
	memset(&menu_data.menu_option, 0, sizeof(menu_data.menu_option));
	menu_data.menu_type = MT_IMAGE_TEXT_64_5X2;

	memset(menu_data.default_dir, 0, sizeof(menu_data.default_dir));
	memset(menu_data.covers_path, 0, sizeof(menu_data.covers_path));
	memset(menu_data.games_path, 0, sizeof(menu_data.games_path));

	if (DirExists("/ide/games"))
	{
		strcpy(menu_data.default_dir, "/ide/DS");
		strcpy(menu_data.games_path, "/ide/games");
		strcpy(menu_data.covers_path, "/ide/DS/apps/games_menu/covers");
	}
	else if (DirExists("/sd/games"))
	{
		strcpy(menu_data.default_dir, "/sd/DS");
		strcpy(menu_data.games_path, "/sd/games");
		strcpy(menu_data.covers_path, "/sd/DS/apps/games_menu/covers");
	}
	else if (!is_custom_bios())
	{
		strcpy(menu_data.default_dir, "/cd");
		strcpy(menu_data.games_path, "/cd/games");
		strcpy(menu_data.covers_path, "/cd/apps/games_menu/covers");
	}

	menu_data.current_dev = GetDeviceType(menu_data.default_dir);
	menu_data.convert_pvr_to_png = menu_data.current_dev == APP_DEVICE_IDE;
}

void DestroyMenuData()
{
	if(menu_data.load_pvr_cover_thread != NULL)
	{
		menu_data.stop_load_pvr_cover = true;		
		thd_join(menu_data.load_pvr_cover_thread, NULL);
		menu_data.load_pvr_cover_thread = NULL;
	}

	if(menu_data.play_cdda_thread != NULL)
	{
		menu_data.cdda_game_changed = true;
		thd_join(menu_data.play_cdda_thread, NULL);
		menu_data.play_cdda_thread = NULL;
	}

	FreeGamesForce();
}

const char* GetFullGamePathByIndex(int game_index)
{
	static char full_path_game[NAME_MAX];
	memset(full_path_game, 0, sizeof(full_path_game));
	if (game_index >= 0)
	{
		if (menu_data.games_array[game_index].is_folder_name)
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s/%s", menu_data.games_path, menu_data.games_array[game_index].folder, menu_data.games_array[game_index].game);
		}
		else
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s", menu_data.games_path, menu_data.games_array[game_index].game);
		}
	}

	return full_path_game;
}

void* PlayCDDAThread(void *params)
{
	int game_index = (int)params;
	if (game_index >= 0)
	{
		const char *full_path_game = GetFullGamePathByIndex(game_index);
		const uint max_time = 2000;
		uint time_elapsed = 0;

		while (!menu_data.cdda_game_changed && time_elapsed < max_time)
		{
			thd_sleep(50);
			time_elapsed += 50;
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

					do
					{
						track_size = GetCDDATrackFilename((random() % 15) + 4, full_path_game, &track_file_path);
					} while (track_size == 0);
					
					PlayCDDATrack(track_file_path, 3);	
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
			const char *full_path_game = GetFullGamePathByIndex(game_index);
			size_t track_size = 0;

			if (menu_data.games_array[game_index].is_cdda == CCGE_NOT_CHECKED)
			{
				char *track_file_path = (char *)malloc(NAME_MAX);
				track_size = GetCDDATrackFilename(4, full_path_game, &track_file_path);

				if (track_size > 0 && track_size < 30 * 1024 * 1024)
				{
					track_size = GetCDDATrackFilename(6, full_path_game, &track_file_path);
				}

				if (track_size > 0 && (track_size <= MAX_SIZE_CDDA || MAX_SIZE_CDDA == 0))
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
	if (menu_data.play_cdda_thread)
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
	
	if ((menu_data.games_array[game_index].is_cdda == CCGE_NOT_CHECKED || menu_data.games_array[game_index].is_cdda == CCGE_CDDA)
		&& menu_data.current_dev == APP_DEVICE_IDE)
	{
		menu_data.play_cdda_thread = thd_create(0, PlayCDDAThread, (void *)game_index);
		thd_set_prio(menu_data.play_cdda_thread, PRIO_DEFAULT + 2);
	}
}

ImageDimensionStruct *GetImageDimension(const char *image_file)
{
	ImageDimensionStruct *image = NULL;
	char *image_type = strrchr(image_file, '.');

	if (strcasecmp(image_type, ".kmg") == 0)
	{
		file_t image_handle = fs_open(image_file, O_RDONLY);

		if (image_handle == FILEHND_INVALID)
			return NULL;

		image = (ImageDimensionStruct *)malloc(sizeof(ImageDimensionStruct));
		memset(image, 0, sizeof(ImageDimensionStruct));
		image->height = 128;
		image->width = 128;

		fs_close(image_handle);
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

int16 CheckCover(int game_index)
{
	int16 exists_cover = SC_WITHOUT_SEARCHING;

	if (menu_data.games_array[game_index].exists_cover == SC_WITHOUT_SEARCHING)
	{
		char *game_cover_path = (char *)malloc(NAME_MAX);
		char *game_without_extension = (char *)malloc(NAME_MAX);

		memset(game_without_extension, 0, NAME_MAX);
		strncpy(game_without_extension, menu_data.games_array[game_index].game, strlen(menu_data.games_array[game_index].game) - 4);

		memset(game_cover_path, 0, NAME_MAX);
		snprintf(game_cover_path, NAME_MAX, "%s/%s.kmg", menu_data.covers_path, game_without_extension);
		if (1 != 1 && IsValidImage(game_cover_path)) // DISABLE KMG
		{
			exists_cover = menu_data.games_array[game_index].exists_cover = SC_EXISTS;
			menu_data.games_array[game_index].cover_type = IT_KMG;
		}
		else
		{
			memset(game_cover_path, 0, NAME_MAX);
			snprintf(game_cover_path, NAME_MAX, "%s/%s.jpg", menu_data.covers_path, game_without_extension);
			if (IsValidImage(game_cover_path))
			{
				exists_cover = menu_data.games_array[game_index].exists_cover = SC_EXISTS;
				menu_data.games_array[game_index].cover_type = IT_JPG;
			}
			else
			{
				memset(game_cover_path, 0, NAME_MAX);
				snprintf(game_cover_path, NAME_MAX, "%s/%s.png", menu_data.covers_path, game_without_extension);
				if (IsValidImage(game_cover_path))
				{
					exists_cover = menu_data.games_array[game_index].exists_cover = SC_EXISTS;
					menu_data.games_array[game_index].cover_type = IT_PNG;
				}
			}
		}

		if (menu_data.games_array[game_index].exists_cover == SC_WITHOUT_SEARCHING)
		{
			snprintf(game_cover_path, NAME_MAX, "%s/%s", menu_data.default_dir, "apps/games_menu/images/gd.jpg");

			menu_data.games_array[game_index].cover_type = FileExists(game_cover_path) ? IT_JPG : IT_PNG;
			exists_cover = menu_data.games_array[game_index].exists_cover = SC_DEFAULT;
			menu_data.games_array[game_index].check_pvr = true;
		}

		free(game_cover_path);
		free(game_without_extension);
	}
	else
	{
		exists_cover = menu_data.games_array[game_index].exists_cover;
	}

	return exists_cover;
}

bool GetGameCoverPath(int game_index, char **game_cover_path)
{
	*game_cover_path = NULL;
	if (game_index >= 0)
	{
		*game_cover_path = (char *)malloc(NAME_MAX);
		memset(*game_cover_path, 0, NAME_MAX);

		char *game = (char *)malloc(NAME_MAX);
		if (menu_data.games_array[game_index].exists_cover == SC_EXISTS)
		{
			memset(game, 0, NAME_MAX);
			strncpy(game, menu_data.games_array[game_index].game, strlen(menu_data.games_array[game_index].game) - 4);

			if (menu_data.games_array[game_index].cover_type == IT_KMG)
			{
				snprintf(*game_cover_path, NAME_MAX, "%s/%s.kmg", menu_data.covers_path, game);
			}
			else if (menu_data.games_array[game_index].cover_type == IT_JPG)
			{
				snprintf(*game_cover_path, NAME_MAX, "%s/%s.jpg", menu_data.covers_path, game);
			}
			else if (menu_data.games_array[game_index].cover_type == IT_PNG)
			{
				snprintf(*game_cover_path, NAME_MAX, "%s/%s.png", menu_data.covers_path, game);
			}
			else
			{
				// NEVER HERE!
			}
		}
		else
		{
			if (menu_data.games_array[game_index].exists_cover == SC_DEFAULT)
			{
				if (menu_data.games_array[game_index].cover_type == IT_JPG)
				{
					snprintf(*game_cover_path, NAME_MAX, "%s/%s", menu_data.default_dir, "apps/games_menu/images/gd.jpg");
				}
				else
				{
					snprintf(*game_cover_path, NAME_MAX, "%s/%s", menu_data.default_dir, "apps/games_menu/images/gd.png");
				}
			}
		}
		free(game);
	}

	return game_cover_path != NULL;
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
			menu_data.menu_option.init_position_x = 20;
			menu_data.menu_option.init_position_y = -22;
			menu_data.menu_option.padding_x = 320;
			menu_data.menu_option.padding_y = 22;
			menu_data.menu_option.image_size = 64.0f;
		}
		break;

		case MT_IMAGE_128_4X3:
		{
			menu_data.menu_option.max_page_size = 12;
			menu_data.menu_option.max_columns = 4;
			menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
			menu_data.menu_option.init_position_x = 55;
			menu_data.menu_option.init_position_y = -45;
			menu_data.menu_option.padding_x = 163;
			menu_data.menu_option.padding_y = 12;
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
			menu_data.menu_option.max_page_size = 10;
			menu_data.menu_option.max_columns = 1;
			menu_data.menu_option.size_items_column = menu_data.menu_option.max_page_size / menu_data.menu_option.max_columns;
			menu_data.menu_option.init_position_x = 20;
			menu_data.menu_option.init_position_y = 5;
			menu_data.menu_option.padding_x = 320;
			menu_data.menu_option.padding_y = 12;
			menu_data.menu_option.image_size = 64.0f;
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
#ifndef IN_CACHE_GAMES
	FreeGamesForce();
#endif
}

void FreeGamesForce()
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
			
			menu_data.games_array[icount].game = NULL;
			menu_data.games_array[icount].folder = NULL;
		}

		free(menu_data.games_array);
		menu_data.games_array = NULL;
		menu_data.games_array_count = 0;
	}
}

bool SaveScannedCover()
{
	char file_name[100];
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", menu_data.default_dir, "apps/games_menu", "scanned_cover.txt");

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
	snprintf(file_name, sizeof(file_name), "%s/%s/%s", menu_data.default_dir, "apps/games_menu", "scanned_cover.txt");

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
		remove(file_name);
	}

	return true;
}

void CleanIncompleteCover()
{
	if (menu_data.cover_scanned_app.last_game_status == CSE_PROCESSING)
	{
		char game_cover_path[NAME_MAX];

		if (menu_data.convert_pvr_to_png)
		{
			snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.png", menu_data.covers_path, menu_data.cover_scanned_app.last_game_scanned);
		}
		else
		{
			snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.jpg", menu_data.covers_path, menu_data.cover_scanned_app.last_game_scanned);
		}

		if (FileExists(game_cover_path))
		{
			remove(game_cover_path);
		}

		strcat(game_cover_path, ".tmp");
		if (FileExists(game_cover_path))
		{
			remove(game_cover_path);
		}
	}
}

bool ExtractPVRCover(int game_index)
{
	bool extracted_cover = false;
	if (game_index >= 0 && (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE))
	{
		if (menu_data.games_array[game_index].check_pvr && menu_data.games_array[game_index].exists_cover == SC_DEFAULT)
		{
			char *game_without_extension = (char *)malloc(NAME_MAX);
			const char *full_game_path = GetFullGamePathByIndex(game_index);
			menu_data.games_array[game_index].check_pvr = false;

			memset(game_without_extension, 0, NAME_MAX);
			strncpy(game_without_extension, menu_data.games_array[game_index].game, strlen(menu_data.games_array[game_index].game) - 4);

			menu_data.send_message_scan("Mounting ISO: %s", game_without_extension);

			if(fs_iso_mount("/iso_cover", full_game_path) == 0)
			{
				char *pvr_path = (char *)malloc(30);
				memset(pvr_path, 0, 30);
				strcpy(pvr_path, "/iso_cover/0GDTEX.PVR");
				
				char *game_cover_path = (char *)malloc(NAME_MAX);				
				menu_data.send_message_scan("Extracting PVR: %s", game_without_extension);

				if (menu_data.convert_pvr_to_png)
				{
					snprintf(game_cover_path, NAME_MAX, "%s/%s.png", menu_data.covers_path, game_without_extension);
					menu_data.cover_scanned_app.last_game_status = CSE_PROCESSING;
					SaveScannedCover();

					if (pvr_to_png(pvr_path, game_cover_path, 128, 128, 10))
					{
						menu_data.games_array[game_index].exists_cover = SC_EXISTS;
						menu_data.games_array[game_index].cover_type = IT_PNG;
						menu_data.games_array[game_index].is_pvr_cover = true;
						extracted_cover = true;
						menu_data.cover_scanned_app.last_game_status = CSE_COMPLETED;
					}
				}
				else
				{
					snprintf(game_cover_path, NAME_MAX, "%s/%s.jpg", menu_data.covers_path, game_without_extension);
					menu_data.cover_scanned_app.last_game_status = CSE_PROCESSING;
					SaveScannedCover();

					if (pvr_to_jpg(pvr_path, game_cover_path, 128, 128, 80))
					{
						menu_data.games_array[game_index].exists_cover = SC_EXISTS;
						menu_data.games_array[game_index].cover_type = IT_JPG;
						menu_data.games_array[game_index].is_pvr_cover = true;
						extracted_cover = true;
						menu_data.cover_scanned_app.last_game_status = CSE_COMPLETED;
					}
				}

				free(game_cover_path);
				free(pvr_path);

				fs_iso_unmount("/iso_cover");
			}

			free(game_without_extension);
		}
	}
	return extracted_cover;
}

void* LoadPVRCoverThread(void *params)
{
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

	// char previous_game[NAME_MAX];
	for (int icount = menu_data.cover_scanned_app.last_game_index-1; icount < menu_data.games_array_count; icount++)
	{
		memset(menu_data.cover_scanned_app.last_game_scanned, 0, sizeof(menu_data.cover_scanned_app.last_game_scanned));
		strncpy(menu_data.cover_scanned_app.last_game_scanned, menu_data.games_array[icount].game, strlen(menu_data.games_array[icount].game) - 4);

		if (menu_data.stop_load_pvr_cover || menu_data.finished_menu) break;		
		
		if (CheckCover(icount) == SC_DEFAULT)
		{
			// CHECK AGAIN TO SEE IF IT WAS NOT DOWNLOADED IN PVR
			menu_data.games_array[icount].exists_cover = SC_WITHOUT_SEARCHING;
			if (CheckCover(icount) == SC_DEFAULT)
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
		strcpy(left_compare, GetLastPart(left->folder, '/', 2));
		strcpy(right_compare, GetLastPart(right->folder, '/', 2));

		cmp = strcmp(left_compare, right_compare);
	}
	else if (left->is_folder_name)
	{
		strcpy(left_compare, GetLastPart(left->folder, '/', 2));
		strncpy(right_compare, right->game, strlen(right->game) - 4);

		cmp = strcmp(left_compare, right_compare);
	}
	else if (right->is_folder_name)
	{
		strncpy(left_compare, left->game, strlen(left->game) - 4);
		strcpy(right_compare, GetLastPart(right->folder, '/', 2));
		
		cmp = strcmp(left_compare, right_compare);
	}
	else
	{
		strncpy(left_compare, left->game, strlen(left->game) - 4);
		strncpy(right_compare, right->game, strlen(right->game) - 4);

		cmp = strcmp(left_compare, right_compare);
	}

	free(left_compare);
	free(right_compare);

	return cmp > 0 ? cmp : -1;
}

bool CheckGdiOptimized(int game_index)
{
	bool optimized = false;
	
	if (game_index >= 0)
	{
		if (!menu_data.games_array[game_index].check_optimized)
		{
			menu_data.games_array[game_index].check_optimized = true;
			optimized = menu_data.games_array[game_index].is_gdi_optimized = IsGdiOptimized(GetFullGamePathByIndex(game_index));
		}
		else
		{
			optimized = menu_data.games_array[game_index].is_gdi_optimized;
		}
	}

	return optimized;
}

bool IsUniqueFileGame(const char *full_path_folder)
{
	file_t fd = fs_open(full_path_folder, O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return false;

	int file_count = 0;
	char *file_type = NULL;
	dirent_t *ent = NULL;

	while ((ent = fs_readdir(fd)) != NULL)
	{
		if (ent->name[0] == '.')
			continue;

		file_type = strrchr(ent->name, '.');
		if (strcasecmp(file_type, ".gdi") == 0
			|| strcasecmp(file_type, ".cdi") == 0
			|| (strcasecmp(file_type, ".iso") == 0 && !(strncasecmp(ent->name, "track", strlen("track")) == 0))
			|| strcasecmp(file_type, ".cso") == 0)
		{
			file_count++;

			if (file_count > 1)
			{
				break;
			}
		}
	}
	ent = NULL;
	fs_close(fd);

	return !(file_count > 1);
}

void RetrieveGamesRecursive(const char *full_path_folder, const char *folder, int level)
{
	file_t fd = fs_open(full_path_folder, O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return;

	dirent_t *ent = NULL;
	char game[NAME_MAX];
	char *file_type = NULL;
	bool is_folder_name = false;	
	bool unique_file = level > 0 ? IsUniqueFileGame(full_path_folder) : false;

	while ((ent = fs_readdir(fd)) != NULL)
	{
		if (ent->name[0] == '.')
			continue;

		// SKIP FULL NAMES WITH A LENGTH LONGER THAN NAMEMAX
		if ((full_path_folder && (strlen(full_path_folder) + strlen(ent->name)) > NAME_MAX)
			|| strlen(ent->name) > NAME_MAX)
			continue;

		is_folder_name = false;
		file_type = strrchr(ent->name, '.');
		memset(game, '\0', sizeof(game));

		if (strcasecmp(file_type, ".gdi") == 0)
		{
			strcpy(game, ent->name);
			is_folder_name = level > 0;
		}
		else if (strcasecmp(file_type, ".cdi") == 0)
		{
			strcpy(game, ent->name);
			is_folder_name = unique_file;
		}
		else if (strcasecmp(file_type, ".iso") == 0 && !(strncasecmp(ent->name, "track", strlen("track")) == 0))
		{
			strcpy(game, ent->name);
			is_folder_name = unique_file;
		}
		else if (strcasecmp(file_type, ".cso") == 0)
		{
			strcpy(game, ent->name);
			is_folder_name = unique_file;
		}
		else if (!(strncasecmp(ent->name, "track", strlen("track")) == 0))
		{
			char *new_folder = (char *)malloc(NAME_MAX);
			memset(new_folder, 0, NAME_MAX);
			snprintf(new_folder, NAME_MAX, "%s/%s", full_path_folder, ent->name);

			RetrieveGamesRecursive(new_folder, ent->name, level+1);
			
			free(new_folder);
		}

		if (game[0] != '\0')
		{
			for (char *c = game; (*c = toupper(*c)); ++c)
			{
				if (*c == 'a')
					*c = 'A'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
			}

			menu_data.games_array_count++;
			if (menu_data.games_array == NULL)
			{
				menu_data.games_array = (GameItemStruct *)malloc(sizeof(GameItemStruct));
			}
			else
			{
				menu_data.games_array = (GameItemStruct *)realloc(menu_data.games_array, menu_data.games_array_count * sizeof(GameItemStruct));
			}

			memset(&menu_data.games_array[menu_data.games_array_count - 1], 0, sizeof(GameItemStruct));
			
			menu_data.games_array[menu_data.games_array_count - 1].game = (char *)malloc(strlen(game) + 1);
			memset(menu_data.games_array[menu_data.games_array_count - 1].game, 0, strlen(game) + 1);
			strncpy(menu_data.games_array[menu_data.games_array_count - 1].game, game, strlen(game));

			if (folder)
			{
				char *folder_game = malloc(NAME_MAX); 
				memset(folder_game, 0, NAME_MAX);
				strcpy(folder_game, full_path_folder + strlen(menu_data.games_path) + 1);

				for (char *c = folder_game; (*c = toupper(*c)); ++c)
				{
					if (*c == 'a')
						*c = 'A'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
				}

				menu_data.games_array[menu_data.games_array_count - 1].folder = (char *)malloc(strlen(folder_game) + 1);
				memset(menu_data.games_array[menu_data.games_array_count - 1].folder, 0, strlen(folder_game) + 1);
				strcpy(menu_data.games_array[menu_data.games_array_count - 1].folder, folder_game);
				free(folder_game);
			}
			
			menu_data.games_array[menu_data.games_array_count - 1].is_folder_name = is_folder_name;
			menu_data.games_array[menu_data.games_array_count - 1].exists_cover = SC_WITHOUT_SEARCHING;
			menu_data.games_array[menu_data.games_array_count - 1].check_optimized = false;
			menu_data.games_array[menu_data.games_array_count - 1].is_cdda = CCGE_NOT_CHECKED;
		}
	}
	ent = NULL;
	file_type = NULL;
	fs_close(fd);
}

bool RetrieveGames()
{
#ifdef IN_CACHE_GAMES
	if (menu_data.games_array_count > 0)
	{
		return true;
	}
#endif

	FreeGames();
	RetrieveGamesRecursive(menu_data.games_path, NULL, 0);	

	if (menu_data.games_array_count > 0)
	{
		qsort(menu_data.games_array, menu_data.games_array_count, sizeof(GameItemStruct), AppCompareGames);

		menu_data.rescan_covers = false;
		if (menu_data.cover_scanned_app.games_count != menu_data.games_array_count)
		{ 
			menu_data.cover_scanned_app.games_count = menu_data.games_array_count;
			menu_data.rescan_covers = true;			
		}
		else if (menu_data.cover_scanned_app.games_count > 0 
			&& menu_data.cover_scanned_app.last_game_index > 0
			&& menu_data.cover_scanned_app.last_game_index <= menu_data.games_array_count)
		{
			char name[NAME_MAX];
			strncpy(name, menu_data.games_array[menu_data.cover_scanned_app.last_game_index-1].game, strlen(menu_data.games_array[menu_data.cover_scanned_app.last_game_index-1].game) - 4);
			if (menu_data.games_array[menu_data.cover_scanned_app.last_game_index-1].is_folder_name)
			{
				if (strcasecmp(menu_data.cover_scanned_app.last_game_scanned, menu_data.games_array[menu_data.cover_scanned_app.last_game_index-1].folder) != 0)
				{
					menu_data.rescan_covers = true;
				}
			}
			else if (strcasecmp(menu_data.cover_scanned_app.last_game_scanned, name) != 0)
			{
				menu_data.rescan_covers = true;
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}