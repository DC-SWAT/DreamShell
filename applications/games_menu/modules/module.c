/* DreamShell ##version##

   module.h - Games app module
   Copyright (C) 2024 Maniac Vera

*/

#include <ds.h>
#include "app_utils.h"
#include "app_module.h"
#include "app_definition.h"
#include <kos/md5.h>
#include <drivers/rtc.h>
#include <isoldr.h>
#include <tsunami/tsudefinition.h>
#include <tsunami/tsunamiutils.h>
#include <tsunami/font.h>
#include <tsunami/color.h>
#include <tsunami/drawable.h>
#include <tsunami/texture.h>
#include <tsunami/trigger.h>
#include <tsunami/vector.h>
#include <tsunami/anims/expxymover.h>
#include <tsunami/anims/logxymover.h>
#include <tsunami/triggers/death.h>
#include <tsunami/drawables/box.h>
#include <tsunami/dsmenu.h>

#define MOTOSHORT(p) ((*(p))<<8) + *(p+1)
#define IN_CACHE_GAMES
#define MAX_SIZE_CDDA 40000000

DEFAULT_MODULE_EXPORTS(app_games_menu);

static kthread_t *exit_menu_thread;
static kthread_t *play_cdda_thread;

static struct
{
	App_t *app;
	bool have_args;
	DSMenu *dsmenu_ptr;
	Scene *scene_ptr;
	int image_type;
	int sector_size;
	int current_dev;
	isoldr_info_t *isoldr;
	uint32 addr;
	uint8 md5[16];
	uint8 boot_sector[2048];
	
	bool exit_app;
	struct timeval old_time;
	struct timeval new_time;
	int menu_type;
	int pages;
	int current_page;
	int previous_page;
	int game_count;
	int menu_cursel;
	int games_array_count;
	char default_dir[20];
	char games_path[NAME_MAX];
	char covers_path[50];
	char item_value_selected[NAME_MAX];
	MenuOptionStruct menu_option;
	GameItemStruct *games_array;
	Animation *item_selector_animation;
	Rectangle *item_selector;
	Font *menu_font;
	Trigger *exit_trigger_list[MAX_SIZE_ITEMS];
	Animation *exit_animation_list[MAX_SIZE_ITEMS];
	Animation *img_button_animation[MAX_SIZE_ITEMS];
	ItemMenu *img_button[MAX_SIZE_ITEMS];
	Label *title;
	Label *title_type;
	Animation *title_animation;
	Animation *title_type_animation;
	Box *main_box;
	Rectangle *title_rectangle;
	Rectangle *title_type_rectangle;
} self;


static const char* GetFullGamePathByIndex(int game_index)
{
	static char full_path_game[NAME_MAX];
	memset(full_path_game, 0, sizeof(full_path_game));
	if (game_index >= 0)
	{
		if (self.games_array[game_index].is_folder_name)
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s/%s", self.games_path, self.games_array[game_index].folder, self.games_array[game_index].game);
		}
		else
		{
			snprintf(full_path_game, NAME_MAX, "%s/%s", self.games_path, self.games_array[game_index].game);
		}
	}

	return full_path_game;
}

static void* PlayCDDAThread(void *params)
{
	StopCDDATrack();
	if (params != NULL) {
		char track_file_path[NAME_MAX];
		const char *full_path_game = (const char*)params;
		size_t track_size = GetCDDATrackFilename(4, full_path_game, track_file_path);

		if (track_size && track_size < 30 * 1024 * 1024) {
			track_size = GetCDDATrackFilename(6, full_path_game, track_file_path);
		}

		if (track_size > 0 && track_size <= MAX_SIZE_CDDA) {
			// thd_sleep(3000);

			do
			{
				track_size = GetCDDATrackFilename((random() % 15) + 4, full_path_game, track_file_path);
			} while (track_size == 0);
			PlayCDDATrack(track_file_path, 3);
		}
	}

	return NULL;
}

static void PlayCDDA(void *params)
{
	PlayCDDAThread(params);
}

static void SetTitle(const char *text)
{
	char titleText[101];
	memset(titleText, 0, sizeof(titleText));

	if (strlen(text) > sizeof(titleText))
	{
		strncpy(titleText, text, sizeof(titleText) - 1);
	}
	else
	{
		strcpy(titleText, text);
	}

	static Vector vectorInit = {-50, 32, ML_ITEM, 1};
	static Vector vector = {10, 32, ML_ITEM, 1};
	
	if (self.title != NULL)
	{
		TSU_LabelSetText(self.title, titleText);
	}
	else
	{
		static Color color = {1, 1.0f, 1.0f, 1.0f};
		self.title = TSU_LabelCreate(self.menu_font, titleText, 26, false, true);
		TSU_LabelSetTint(self.title, &color);
		TSU_MenuSubAddLabel(self.dsmenu_ptr, self.title);
	}

	if (self.title_animation != NULL)
	{
		TSU_AnimationComplete(self.title_animation, (Drawable *)self.title);
		TSU_AnimationDestroy(&self.title_animation);
	}

	TSU_LabelSetTranslate(self.title, &vectorInit);
	self.title_animation = (Animation *)TSU_LogXYMoverCreate(vector.x, vector.y);
	TSU_DrawableAnimAdd((Drawable *)self.title, (Animation *)self.title_animation);
}

static void SetTitleType(const char *full_path_game)
{
	if (full_path_game != NULL)
	{
		const char *file_type = strrchr(full_path_game, '.');

		char title_text[4];
		memset(title_text, 0, sizeof(title_text));

		if (strcasecmp(file_type, ".cdi") == 0)
		{
			strncpy(title_text, "CDI", 3);
		}
		else if (strcasecmp(file_type, ".iso") == 0)
		{
			strncpy(title_text, "ISO", 3);
		}
		else if (strcasecmp(file_type, ".cso") == 0)
		{
			strncpy(title_text, "CSO", 3);
		}
		else if (strcasecmp(file_type, ".gdi") == 0)
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
			
			if (FileExists(result) == 1)
			{
				strncpy(title_text, "OPT", 3);
			}
			else
			{
				strncpy(title_text, "GDI", 3);
			}

			free(result);
		}

		static Vector vectorInit = {650, 32, ML_ITEM + 3, 1};
		static Vector vector = {559, 32, ML_ITEM + 3, 1};

		if (self.title_type != NULL)
		{
			TSU_LabelSetText(self.title_type, title_text);
		}
		else
		{
			static Color color = {1, 1.0f, 1.0f, 1.0f};
			self.title_type = TSU_LabelCreate(self.menu_font, title_text, 26, false, true);
			TSU_LabelSetTint(self.title_type, &color);
			TSU_MenuSubAddLabel(self.dsmenu_ptr, self.title_type);
		}

		if (self.title_type_animation != NULL)
		{
			TSU_AnimationComplete(self.title_type_animation, (Drawable *)self.title_type);
			TSU_AnimationDestroy(&self.title_type_animation);
		}

		TSU_LabelSetTranslate(self.title_type, &vectorInit);
		self.title_type_animation = (Animation *)TSU_LogXYMoverCreate(vector.x, vector.y);
		TSU_DrawableAnimAdd((Drawable *)self.title_type, (Animation *)self.title_type_animation);		
	}
}

static void SetMenuType(int menu_type)
{
	self.menu_type = menu_type;

	switch (menu_type)
	{
		case MT_IMAGE_TEXT_64_5X2:
		{
			self.menu_option.max_page_size = 10;
			self.menu_option.max_columns = 2;
			self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
			self.menu_option.init_position_x = 20;
			self.menu_option.init_position_y = -22;
			self.menu_option.padding_x = 320;
			self.menu_option.padding_y = 22;
			self.menu_option.image_size = 64.0f;
		}
		break;

		case MT_IMAGE_128_4X3:
		{
			self.menu_option.max_page_size = 12;
			self.menu_option.max_columns = 4;
			self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
			self.menu_option.init_position_x = 55;
			self.menu_option.init_position_y = -45;
			self.menu_option.padding_x = 163;
			self.menu_option.padding_y = 12;
			self.menu_option.image_size = 128.0f;
		}
		break;

		case MT_IMAGE_256_3X2:
		{
			self.menu_option.max_page_size = 6;
			self.menu_option.max_columns = 3;
			self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
			self.menu_option.init_position_x = 90;
			self.menu_option.init_position_y = -82;
			self.menu_option.padding_x = 212.0f;
			self.menu_option.padding_y = 12;
			self.menu_option.image_size = 200.0f;
		}
		break;

		case MT_PLANE_TEXT:
		{
			self.menu_option.max_page_size = 10;
			self.menu_option.max_columns = 1;
			self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
			self.menu_option.init_position_x = 20;
			self.menu_option.init_position_y = 5;
			self.menu_option.padding_x = 320;
			self.menu_option.padding_y = 12;
			self.menu_option.image_size = 64.0f;
		}
		break;

		default:
		{
			self.menu_option.max_page_size = 10;
			self.menu_option.max_columns = 2;
			self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
			self.menu_option.init_position_x = 20;
			self.menu_option.init_position_y = 5;
			self.menu_option.padding_x = 320;
			self.menu_option.padding_y = 12;
			self.menu_option.image_size = 64.0f;
		}
		break;
	}
}

static ImageDimensionStruct* GetImageDimension(const char *image_file)
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
		i = j = 2; // Start at offset of first marker
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

			j += 2 + MOTOSHORT(&buffer[i+2]); // Skip to next marker
			if (j < file_size) // need to read more
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
			image->height = MOTOSHORT(&buffer[i+5]);
			image->width = MOTOSHORT(&buffer[i+7]);
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

static void FreeGames()
{
#ifndef IN_CACHE_GAMES
	FreeGamesForce();
#endif
}

static void FreeGamesForce()
{
	if (self.games_array_count > 0)
	{
		free(self.games_array);
		self.games_array = NULL;
		self.games_array_count = 0;
	}
}

static void SetCursor()
{
	static Vector selector_translate = {0, 0, ML_CURSOR, 1};

	if (self.item_selector_animation != NULL)
	{
		TSU_AnimationComplete(self.item_selector_animation, (Drawable *)self.item_selector);
		TSU_AnimationDestroy(&self.item_selector_animation);
	}

	if (self.item_selector != NULL)
	{
		TSU_MenuSubRemoveRectangle(self.dsmenu_ptr, self.item_selector);
		TSU_RectangleDestroy(&self.item_selector);
	}

	if (self.item_selector == NULL)
	{
		if (self.menu_type == MT_IMAGE_TEXT_64_5X2)
		{
			Color color = {1, 1.0f, 1.0f, 0.1f};
			Color border_color = {1, 0.28f, 0.06f, 0.25f};
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 1, &border_color, 0);
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
			TSU_DrawableSetAlpha((Drawable *)self.item_selector, 0.8f);
		}
		else if (self.menu_type == MT_IMAGE_128_4X3)
		{
			Color color = {1, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};			
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 6, &border_color, 0);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
			TSU_DrawableSetAlpha((Drawable *)self.item_selector, 0.7f);
		}
		else
		{
			Color color = {1, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};			
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 4, &border_color, 0);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
			TSU_DrawableSetAlpha((Drawable *)self.item_selector, 0.7f);
		}

		if (selector_translate.x > 0 && selector_translate.y > 0)
		{
			TSU_DrawableSetTranslate((Drawable *)self.item_selector, &selector_translate);
		}

		TSU_MenuSubAddRectangle(self.dsmenu_ptr, self.item_selector);
	}

	static int init_position_x;
	static int init_position_y;

	if (self.menu_type == MT_IMAGE_TEXT_64_5X2)
	{

		TSU_RectangleSetSize(self.item_selector, 310, self.menu_option.image_size + 8);
		init_position_x = 5;
		init_position_y = 33;
		selector_translate.z = ML_CURSOR;
	}
	else if (self.menu_type == MT_IMAGE_128_4X3)
	{
		TSU_RectangleSetSize(self.item_selector, self.menu_option.image_size + 4, self.menu_option.image_size + 4);
		init_position_x = 9;
		init_position_y = 41;
		selector_translate.z = ML_CURSOR;		
	}
	else
	{
		TSU_RectangleSetSize(self.item_selector, self.menu_option.image_size + 4, self.menu_option.image_size + 4);
		init_position_x = 8;
		init_position_y = 40;
		selector_translate.z = ML_CURSOR;
	}

	selector_translate.x = self.menu_cursel / self.menu_option.size_items_column * self.menu_option.padding_x + init_position_x;
	selector_translate.y = ((self.menu_cursel + 1) - self.menu_option.size_items_column * (self.menu_cursel / self.menu_option.size_items_column)) * (self.menu_option.padding_y + self.menu_option.image_size) + init_position_y;

	self.item_selector_animation = (Animation *)TSU_LogXYMoverCreate(selector_translate.x, selector_translate.y);
	TSU_DrawableAnimAdd((Drawable *)self.item_selector, (Animation *)self.item_selector_animation);
}

static bool IsValidImage(const char *image_file)
{
	bool isValid = false;

	ImageDimensionStruct *image = GetImageDimension(image_file);
	if (image != NULL)
	{
		isValid = (image->height == 64 && image->width == 64) 
			|| (image->height == 128 && image->width == 128) 
			|| (image->height == 256 && image->width == 256);

		free(image);
	}

	return isValid;
}

static int AppCompareGames(const void *a, const void *b)
{
	const GameItemStruct *left = (const GameItemStruct *)a;
	const GameItemStruct *right = (const GameItemStruct *)b;
	int cmp = 0;

	char *left_compare = malloc(MAX_SIZE_GAME_NAME);
	char *right_compare = malloc(MAX_SIZE_GAME_NAME);

	memset(left_compare, 0, MAX_SIZE_GAME_NAME);
	memset(right_compare, 0, MAX_SIZE_GAME_NAME);

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

static bool IsUniqueFileGame(const char *full_path_folder)
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

static void RetrieveGamesRecursive(const char *full_path_folder, const char *folder, int level)
{
	file_t fd = fs_open(full_path_folder, O_RDONLY | O_DIR);
	if (fd == FILEHND_INVALID)
		return;

	dirent_t *ent = NULL;
	char game[MAX_SIZE_GAME_NAME];
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

			RetrieveGamesRecursive(new_folder, ent->name, level++);
			
			free(new_folder);
		}

		if (game[0] != '\0')
		{
			for (char *c = game; (*c = toupper(*c)); ++c)
			{
				if (*c == 'a')
					*c = 'A'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
			}

			self.games_array_count++;
			if (self.games_array == NULL)
			{
				self.games_array = (GameItemStruct *)malloc(sizeof(GameItemStruct));
			}
			else
			{
				self.games_array = (GameItemStruct *)realloc(self.games_array, self.games_array_count * sizeof(GameItemStruct));
			}

			memset(&self.games_array[self.games_array_count - 1], 0, sizeof(GameItemStruct));
			strncpy(self.games_array[self.games_array_count - 1].game, game, strlen(game));

			if (folder)
			{
				char *folder_game = malloc(NAME_MAX); 
				memset(folder_game, 0, NAME_MAX);
				strcpy(folder_game, full_path_folder + strlen(self.games_path) + 1);

				for (char *c = folder_game; (*c = toupper(*c)); ++c)
				{
					if (*c == 'a')
						*c = 'A'; // Maniac Vera: BUG toupper in the letter a, it does not convert it
				}

				strcpy(self.games_array[self.games_array_count - 1].folder, folder_game);
				free(folder_game);
			}
			
			self.games_array[self.games_array_count - 1].is_folder_name = is_folder_name;
			self.games_array[self.games_array_count - 1].exists_cover = SC_WITHOUT_SEARCHING;
		}
	}
	ent = NULL;
	file_type = NULL;
	fs_close(fd);
}

static bool RetrieveGames()
{
#ifdef IN_CACHE_GAMES
	if (self.games_array_count > 0)
	{
		return true;
	}
#endif

	FreeGames();
	RetrieveGamesRecursive(self.games_path, NULL, 0);

	if (self.games_array_count > 0)
	{
		qsort(self.games_array, self.games_array_count, sizeof(GameItemStruct), AppCompareGames);
		return true;
	}
	else
	{
		return false;
	}
}

static bool LoadPage(bool change_view)
{
	bool loaded = false;
	char game_cover_path[NAME_MAX];
	char name[MAX_SIZE_GAME_NAME];
	char game[MAX_SIZE_GAME_NAME];
	char game_without_extension[MAX_SIZE_GAME_NAME];
	char name_truncated[17];
	int fileCount = 0;

	if (self.pages == -1)
	{
		if (RetrieveGames())
		{
			self.pages = (self.games_array_count > 0 ? ceil((float)self.games_array_count / (float)self.menu_option.max_page_size) : 0);
		}
	}
	else
	{
		if (change_view)
		{
			self.pages = (self.games_array_count > 0 ? ceil((float)self.games_array_count / (float)self.menu_option.max_page_size) : 0);
		}
	}

	if (self.pages > 0)
	{

		if (self.current_page < 1)
		{
			self.current_page = self.pages;
		}
		else if (self.current_page > self.pages)
		{
			self.current_page = 1;
		}

		if (self.current_page != self.previous_page || change_view)
		{
			self.previous_page = self.current_page;

			for (int i = 0; i < MAX_SIZE_ITEMS; i++)
			{
				if (self.img_button_animation[i] != NULL)
				{
					TSU_AnimationComplete(self.img_button_animation[i], (Drawable *)self.img_button[i]);
					TSU_AnimationDestroy(&self.img_button_animation[i]);
				}

				if (self.img_button[i] != NULL)
				{
					TSU_MenuSubRemoveItemMenu(self.dsmenu_ptr, self.img_button[i]);
					TSU_ItemMenuDestroy(&self.img_button[i]);
				}				
			}

			int column = 0;
			int page = 0;
			self.game_count = 0;
			Vector vectorTranslate = {0, 480, ML_ITEM, 1};

			for (int icount = 0; icount < self.games_array_count; icount++)
			{
				fileCount++;
				page = ceil((float)fileCount / (float)self.menu_option.max_page_size);

				if (page < self.current_page)
					continue;
				else if (page > self.current_page)
					break;

				self.game_count++;

				column = floor((float)(self.game_count - 1) / (float)self.menu_option.size_items_column);

				if (self.games_array[icount].exists_cover == SC_WITHOUT_SEARCHING)
				{
					memset(game_without_extension, 0, sizeof(game_without_extension));
					strncpy(game_without_extension, self.games_array[icount].game, strlen(self.games_array[icount].game) - 4);

					memset(game_cover_path, 0, sizeof(game_cover_path));
					snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.kmg", self.covers_path, game_without_extension);
					if (1 != 1 && IsValidImage(game_cover_path)) // DISABLE KMG
					{
						self.games_array[icount].exists_cover = SC_EXISTS;
						self.games_array[icount].cover_type = IT_KMG;
					}
					else 
					{
						memset(game_cover_path, 0, sizeof(game_cover_path));
						snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.jpg", self.covers_path, game_without_extension);
						if (IsValidImage(game_cover_path))
						{
							self.games_array[icount].exists_cover = SC_EXISTS;
							self.games_array[icount].cover_type = IT_JPG;
						}
						else
						{
							memset(game_cover_path, 0, sizeof(game_cover_path));
							snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.png", self.covers_path, game_without_extension);
							if (IsValidImage(game_cover_path))
							{
								self.games_array[icount].exists_cover = SC_EXISTS;
								self.games_array[icount].cover_type = IT_PNG;
							}
						}
					}

					if (self.games_array[icount].exists_cover == SC_WITHOUT_SEARCHING)
					{
						self.games_array[icount].exists_cover = SC_NOT_EXIST;
					}
				}

				if (self.games_array[icount].exists_cover == SC_EXISTS)
				{
					memset(game, 0, sizeof(game));
					strncpy(game, self.games_array[icount].game, strlen(self.games_array[icount].game) - 4);

					if (self.games_array[icount].cover_type == IT_KMG)
					{
						snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.kmg", self.covers_path, game);
					}
					else if (self.games_array[icount].cover_type == IT_JPG)
					{
						snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.jpg", self.covers_path, game);
					}
					else if (self.games_array[icount].cover_type == IT_PNG)
					{
						snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.png", self.covers_path, game);
					}
					else
					{
						// NEVER HERE!
					}
				}
				else
				{
					if (self.games_array[icount].exists_cover == SC_DEFAULT)
					{						
						if (self.games_array[icount].cover_type == IT_JPG)
						{
							snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s", self.default_dir, "apps/games_menu/images/gd.jpg");
						}
						else
						{
							snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s", self.default_dir, "apps/games_menu/images/gd.png");
						}
					}
					else 
					{
						snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s", self.default_dir, "apps/games_menu/images/gd.jpg");
						if (FileExists(game_cover_path) == 0) 
						{
							snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s", self.default_dir, "apps/games_menu/images/gd.png");
							self.games_array[icount].exists_cover = SC_DEFAULT;
							self.games_array[icount].cover_type = IT_PNG;
						}
						else 
						{
							self.games_array[icount].exists_cover = SC_DEFAULT;
							self.games_array[icount].cover_type = IT_JPG;
						}						
					}
				}

				memset(name, 0, sizeof(name));
				if (self.games_array[icount].is_folder_name)
				{
					strcpy(name, GetLastPart(self.games_array[icount].folder, '/', 0));
				}
				else
				{
					strncpy(name, self.games_array[icount].game, strlen(self.games_array[icount].game) - 4);
				}

				if (self.menu_type == MT_IMAGE_TEXT_64_5X2)
				{
					memset(name_truncated, 0, sizeof(name_truncated));
					if (strlen(name) > sizeof(name_truncated) - 1)
					{
						strncpy(name_truncated, name, sizeof(name_truncated) - 1);
					}
					else
					{
						strcpy(name_truncated, name);
					}

					self.img_button[self.game_count - 1] = TSU_ItemMenuCreate(game_cover_path, self.menu_option.image_size, self.menu_option.image_size
										, self.games_array[icount].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY
										, name_truncated, self.menu_font, 16);
				}
				else if (self.menu_type == MT_PLANE_TEXT)
				{
				}
				else
				{
					self.img_button[self.game_count - 1] = TSU_ItemMenuCreateImage(game_cover_path, self.menu_option.image_size
								, self.menu_option.image_size
								, self.games_array[icount].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY);
				}

				ItemMenu *item_menu = self.img_button[self.game_count - 1];

				TSU_ItemMenuSetItemIndex(item_menu, icount);
				TSU_ItemMenuSetItemValue(item_menu, name);
				TSU_ItemMenuSetTranslate(item_menu, &vectorTranslate);

				self.img_button_animation[self.game_count - 1] = (Animation *)TSU_LogXYMoverCreate(self.menu_option.init_position_x + (column * self.menu_option.padding_x),
																									(self.menu_option.image_size + self.menu_option.padding_y) * (self.game_count - self.menu_option.size_items_column * column) + self.menu_option.init_position_y);

				TSU_ItemMenuAnimAdd(item_menu, self.img_button_animation[self.game_count - 1]);

				if (self.game_count == 1)
				{
					TSU_ItemMenuSetSelected(item_menu, true);
					SetTitle(name);
					SetTitleType(GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(item_menu)));
					SetCursor();
					PlayCDDA((void *)GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(item_menu)));
				}
				else
				{
					TSU_ItemMenuSetSelected(item_menu, false);
				}

				TSU_MenuSubAddItemMenu(self.dsmenu_ptr, item_menu);
			}

			loaded = true;
			FreeGames();
		}
	}
	else
	{	
		memset(game_cover_path, 0, sizeof(game_cover_path));

		char font_path[NAME_MAX];
		memset(font_path, 0, sizeof(font_path));
		snprintf(font_path, sizeof(font_path), "%s/%s", self.default_dir, "apps/games_menu/fonts/message.txf");

		TSU_LabelDestroy(&self.title);
		TSU_FontDestroy(&self.menu_font);
		self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
		
		snprintf(game_cover_path, sizeof(game_cover_path), "You need put the games here:\n%s\n\nand images here:\n%s", self.games_path, self.covers_path);
		ds_printf(game_cover_path);
		SetTitle(game_cover_path);
	}

	return loaded;
}

static void InitMenu()
{
	self.scene_ptr = TSU_MenuGetScene(self.dsmenu_ptr);
	memset(self.default_dir, 0, sizeof(self.default_dir));
	memset(self.covers_path, 0, sizeof(self.covers_path));
	memset(self.games_path, 0, sizeof(self.games_path));

	if (DirExists("/ide/games"))
	{
		strcpy(self.default_dir, "/ide/DS");
		strcpy(self.games_path, "/ide/games");
		strcpy(self.covers_path, "/ide/DS/apps/games_menu/covers");
	}
	else if (DirExists("/sd/games"))
	{
		strcpy(self.default_dir, "/sd/DS");
		strcpy(self.games_path, "/sd/games");
		strcpy(self.covers_path, "/sd/DS/apps/games_menu/covers");
	}
	else if (!is_custom_bios())
	{
		strcpy(self.default_dir, "/cd");

#ifdef DEBUG_MENU_GAMES_CD
		strcpy(self.games_path, "/cd/apps/games_menu/gdis");
		strcpy(self.covers_path, "/cd/apps/games_menu/covers");
#else
		strcpy(self.games_path, "/cd/games");
		strcpy(self.covers_path, "/cd/apps/games_menu/covers");
#endif
	}

	char font_path[NAME_MAX];
	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", self.default_dir, "apps/games_menu/fonts/default.txf");

	self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	SetMenuType(MT_IMAGE_TEXT_64_5X2);
	self.exit_app = false;
	self.title = NULL;
	self.title_type = NULL;

	// Offset our scene so 0,0,0 is the screen center with Z +10
	Vector vector = {0, 0, 10, 1};
	TSU_MenSetTranslate(self.dsmenu_ptr, &vector);

	self.current_page = 1;
	LoadPage(false);

	self.menu_cursel = 0;
}

static void StartExit()
{
	StopCDDATrack();
	float y = 1.0f;

	if (self.main_box != NULL)
	{
		TSU_MenuSubRemoveBox(self.dsmenu_ptr, self.main_box);
	}

	if (self.title_rectangle != NULL)
	{
		TSU_MenuSubRemoveRectangle(self.dsmenu_ptr, self.title_rectangle);
	}

	if (self.title_type_rectangle != NULL)
	{
		TSU_MenuSubRemoveRectangle(self.dsmenu_ptr, self.title_type_rectangle);
	}

	if (self.item_selector != NULL)
	{
		TSU_MenuSubRemoveRectangle(self.dsmenu_ptr, self.item_selector);
	}

	if (self.title != NULL)
	{
		TSU_MenuSubRemoveLabel(self.dsmenu_ptr, self.title);
	}

	if (self.title_type != NULL)
	{
		TSU_MenuSubRemoveLabel(self.dsmenu_ptr, self.title_type);
	}

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.img_button[i] != NULL)
		{
			if (!self.exit_app && TSU_ItemMenuIsSelected(self.img_button[i]))
			{
				strcpy(self.item_value_selected, GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.img_button[i])));

				if (TSU_ItemMenuHasTextAndImage(self.img_button[i]))
				{
					self.exit_animation_list[i] = (Animation *)TSU_LogXYMoverCreate((640 - (84 + 64)) / 2, (480 - 84) / 2);
				}
				else
				{
					self.exit_animation_list[i] = (Animation *)TSU_LogXYMoverCreate((640 - 84) / 2, (480 - 84) / 2);
				}

				self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);
				TSU_TriggerAdd(self.exit_animation_list[i], self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.img_button[i], self.exit_animation_list[i]);
			}
			else
			{
				self.exit_animation_list[i] = (Animation *)TSU_ExpXYMoverCreate(0, y + .10, 0, 400);
				self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);

				TSU_TriggerAdd(self.exit_animation_list[i], self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.img_button[i], self.exit_animation_list[i]);
			}
		}
	}
	TSU_MenuStartExit(self.dsmenu_ptr);
}

static void GamesApp_InputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;
	
	bool skip_cursor = false;

	switch (key)
	{

		// X: CHANGE VISUALIZATION
		case KeyMiscX:
		{
			int real_cursel = (self.current_page - 1) * self.menu_option.max_page_size + self.menu_cursel + 1;
			SetMenuType(self.menu_type >= 2 ? MT_IMAGE_TEXT_64_5X2 : self.menu_type + 1);
			self.current_page = ceil((float)real_cursel / (float)self.menu_option.max_page_size);

			if (LoadPage(true))
			{
				self.menu_cursel = real_cursel - (self.current_page - 1) * self.menu_option.max_page_size - 1;
			}
		}
		break;

		case KeyCancel:
		{
			self.exit_app = true;
			skip_cursor = true;
			StartExit();
		}
		break;

		// LEFT TRIGGER
		case KeyPgup:
		{
			self.current_page--;
			if (LoadPage(false))
			{
				self.menu_cursel = 0;
			}
		}
		break;

		// RIGHT TRIGGER
		case KeyPgdn:
		{
			self.current_page++;
			if (LoadPage(false))
			{
				self.menu_cursel = 0;
				// skip_cursor = true;
			}
		}
		break;

		case KeyUp:
		{
			self.menu_cursel--;

			if (self.menu_type == MT_IMAGE_TEXT_64_5X2)
			{
				if (self.menu_cursel < 0)
				{
					self.menu_cursel += self.game_count;
				}
			}
			else
			{
				if ((self.menu_cursel + 1) % self.menu_option.size_items_column == 0)
				{
					self.menu_cursel += self.menu_option.size_items_column;
				}

				if (self.menu_cursel < 0)
				{
					self.menu_cursel = 0;
				}
				else if (self.menu_cursel >= self.game_count)
				{
					self.menu_cursel = self.game_count - 1;
				}
			}
		}

		break;

		case KeyDown:
		{
			self.menu_cursel++;

			if (self.menu_type == MT_IMAGE_TEXT_64_5X2)
			{
				if (self.menu_cursel >= self.game_count)
				{
					self.menu_cursel -= self.game_count;
				}
			}
			else
			{
				if (self.menu_cursel % self.menu_option.size_items_column == 0)
				{
					self.menu_cursel -= self.menu_option.size_items_column;
				}

				if (self.menu_cursel >= self.game_count)
				{
					self.menu_cursel = self.game_count - 1;
				}
			}
		}

		break;

		case KeyLeft:
			if (self.game_count <= self.menu_option.size_items_column)
			{
				self.menu_cursel = 0;
			}
			else
			{
				self.menu_cursel -= self.menu_option.size_items_column;

				if (self.menu_cursel < 0)
					self.menu_cursel += self.game_count;
			}

			break;

		case KeyRight:
			if (self.game_count <= self.menu_option.size_items_column)
			{
				self.menu_cursel = self.game_count - 1;
			}
			else
			{
				self.menu_cursel += self.menu_option.size_items_column;

				if (self.menu_cursel >= self.game_count)
					self.menu_cursel -= self.game_count;
			}

			break;

		case KeySelect:
			StartExit();			
			skip_cursor = true;
			break;

		default:
			{
			}
			break;
	}

	if (!skip_cursor)
	{
		for (int i = 0; i < self.game_count; i++)
		{
			if (i == self.menu_cursel)
			{
				TSU_ItemMenuSetSelected(self.img_button[i], true);
				SetTitle(TSU_ItemMenuGetItemValue(self.img_button[i]));
				SetTitleType(GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.img_button[i])));
				SetCursor();
				PlayCDDA((void *)GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.img_button[i])));
			}
			else
			{
				TSU_ItemMenuSetSelected(self.img_button[i], false);
			}
		}
	}
}

static int CanUseTrueAsyncDMA(void)
{
	return (self.sector_size == 2048 &&
			(self.current_dev == APP_DEVICE_IDE || self.current_dev == APP_DEVICE_CD) &&
			(self.image_type == ISOFS_IMAGE_TYPE_ISO || self.image_type == ISOFS_IMAGE_TYPE_GDI));
}

void DefaultPreset()
{
	if (CanUseTrueAsyncDMA())
	{
		self.isoldr->use_dma = 1;
		self.isoldr->emu_async = 0;
		self.addr = ISOLDR_DEFAULT_ADDR_MIN_GINSU;
	}
	else
	{
		self.isoldr->use_dma = 0;
		self.isoldr->emu_async = 8;
		self.addr = ISOLDR_DEFAULT_ADDR_LOW;
	}

	strcpy(self.isoldr->fs_dev, "auto");
	self.isoldr->emu_cdda = CDDA_MODE_DISABLED;
	self.isoldr->use_irq = 0;
	self.isoldr->syscalls = 0;
	self.isoldr->emu_vmu = 0;
	self.isoldr->scr_hotkey = 0;
	self.isoldr->boot_mode = BOOT_MODE_DIRECT;

	// Enable CDDA if present
	if (self.item_value_selected[0] != '\0')
	{
		char track_file_path[NAME_MAX];
		size_t track_size = GetCDDATrackFilename(4, self.item_value_selected, track_file_path);

		if (track_size && track_size < 30 * 1024 * 1024)
		{
			track_size = GetCDDATrackFilename(6, self.item_value_selected, track_file_path);
		}

		if (track_size > 0)
		{
			self.isoldr->use_irq = 1;
			self.isoldr->emu_cdda = 1;
		}
	}
}

static void GetMD5Hash(const char *file_mount_point)
{
	file_t fd;
	fd = fs_iso_first_file(file_mount_point);

	if (fd != FILEHND_INVALID)
	{
		if (fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, (int)self.boot_sector) < 0)
		{
			memset(self.md5, 0, sizeof(self.md5));
			memset(self.boot_sector, 0, sizeof(self.boot_sector));
		}
		else
		{
			kos_md5(self.boot_sector, sizeof(self.boot_sector), self.md5);
		}

		// Also get image type and sector size
		if (fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, (int)&self.image_type) < 0)
		{
			ds_printf("%s: Can't get image type\n", lib_get_name());
		}

		if (fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE, (int)&self.sector_size) < 0)
		{
			ds_printf("%s: Can't get sector size\n", lib_get_name());
		}

		fs_close(fd);
	}
}

static int LoadPreset()
{
	char *preset_file_name = NULL;
	if (fs_iso_mount("/iso_game", self.item_value_selected) == 0)
	{
		GetMD5Hash("/iso_game");
		fs_iso_unmount("/iso_game");
		preset_file_name = MakePresetFilename(self.default_dir, self.md5);
		ds_printf("PresetFileName: %s", preset_file_name);

		if (FileSize(preset_file_name) < 5)
		{
			preset_file_name = NULL;
		}
	}

	self.current_dev = GetDeviceType(self.default_dir);

	int use_dma = 0, emu_async = 16, use_irq = 0;
	int fastboot = 0, low = 0, emu_vmu = 0, scr_hotkey = 0;
	int boot_mode = BOOT_MODE_DIRECT;
	int bin_type = BIN_TYPE_AUTO;
	bool default_preset_loaded = false;
	char title[32] = "";
	char device[8] = "";
	char cdda[12] = "";
	char memory[12] = "0x8c000100";
	char heap_memory[12] = "";
	char bin_file[12] = "";
	char patch_a[2][10];
	char patch_v[2][10];
	memset(patch_a, 0, 2 * 10);
	memset(patch_v, 0, 2 * 10);

	isoldr_conf options[] = {
		{"dma", CONF_INT, (void *)&use_dma},
		{"cdda", CONF_STR, (void *)cdda},
		{"irq", CONF_INT, (void *)&use_irq},
		{"low", CONF_INT, (void *)&low},
		{"vmu", CONF_INT, (void *)&emu_vmu},
		{"scrhotkey", CONF_INT, (void *)&scr_hotkey},
		{"heap", CONF_STR, (void *)&heap_memory},
		{"memory", CONF_STR, (void *)memory},
		{"async", CONF_INT, (void *)&emu_async},
		{"mode", CONF_INT, (void *)&boot_mode},
		{"type", CONF_INT, (void *)&bin_type},
		{"file", CONF_STR, (void *)bin_file},
		{"title", CONF_STR, (void *)title},
		{"device", CONF_STR, (void *)device},
		{"fastboot", CONF_INT, (void *)&fastboot},
		{"pa1", CONF_STR, (void *)patch_a[0]},
		{"pv1", CONF_STR, (void *)patch_v[0]},
		{"pa2", CONF_STR, (void *)patch_a[1]},
		{"pv2", CONF_STR, (void *)patch_v[1]},
		{NULL, CONF_END, NULL}};

	if ((self.isoldr = isoldr_get_info(self.item_value_selected, 0)) != NULL)
	{
		if (preset_file_name == NULL || conf_parse(options, preset_file_name) == -1)
		{
			ds_printf("DS_ERROR: Can't parse preset\n");
			DefaultPreset();
			default_preset_loaded = true;
		}

		if (!default_preset_loaded)
		{
			self.isoldr->use_dma = use_dma;
			self.isoldr->emu_async = emu_async;
			self.isoldr->emu_cdda = strtoul(cdda, NULL, 16);
			self.isoldr->use_irq = use_irq;
			self.isoldr->scr_hotkey = scr_hotkey;

			if (strtoul(heap_memory, NULL, 10) <= HEAP_MODE_MAPLE)
			{
				self.isoldr->heap = strtoul(heap_memory, NULL, 10);
			}
			else
			{
				self.isoldr->heap = strtoul(heap_memory, NULL, 16);
			}

			self.isoldr->boot_mode = boot_mode;
			self.isoldr->fast_boot = fastboot;

			if (strlen(device) > 0)
			{
				if (strncmp(device, "auto", 4) != 0)
				{
					strcpy(self.isoldr->fs_dev, device);
				}
				else
				{
					strcpy(self.isoldr->fs_dev, "auto");
				}
			}
			else
			{
				strcpy(self.isoldr->fs_dev, "auto");
			}

			if (bin_type != BIN_TYPE_AUTO)
			{
				self.isoldr->exec.type = bin_type;
			}

			if (low)
			{
				self.isoldr->syscalls = 1;
			}

			self.addr = strtoul(memory, NULL, 16);
		}

		if (strncmp(self.isoldr->fs_dev, "auto", 4) == 0)
		{
			if (!strncasecmp(self.default_dir, "/cd", 3))
			{
				strcpy(self.isoldr->fs_dev, "cd");
			}
			else if (!strncasecmp(self.default_dir, "/sd", 3))
			{
				strcpy(self.isoldr->fs_dev, "sd");
			}
			else if (!strncasecmp(self.default_dir, "/ide", 4))
			{
				strcpy(self.isoldr->fs_dev, "ide");
			}
		}

		return 1;
	}
	else
	{
		return 0;
	}
}

void FreeAppData()
{
	if(play_cdda_thread != NULL)
	{
		thd_destroy(play_cdda_thread);
		play_cdda_thread = NULL;
	}

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.img_button_animation[i] != NULL)
		{
			TSU_AnimationDestroy(&self.img_button_animation[i]);
		}

		if (self.img_button[i] != NULL)
		{
			TSU_ItemMenuDestroy(&self.img_button[i]);
		}

		if (self.exit_animation_list[i] != NULL)
		{
			TSU_AnimationDestroy(&self.exit_animation_list[i]);
		}

		if (self.exit_trigger_list[i] != NULL)
		{
			TSU_TriggerDestroy(&self.exit_trigger_list[i]);
		}
	}	

	if (self.main_box != NULL)
	{
		TSU_BoxDestroy(&self.main_box);
	}

	if (self.title_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.title_rectangle);
	}

	if (self.title_type_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.title_type_rectangle);
	}

	if (self.item_selector != NULL)
	{
		TSU_RectangleDestroy(&self.item_selector);
	}

	if (self.title != NULL)
	{
		TSU_LabelDestroy(&self.title);
	}

	if (self.title_animation != NULL)
	{
		TSU_AnimationDestroy(&self.title_animation);
	}

	if (self.title_type != NULL)
	{
		TSU_LabelDestroy(&self.title_type);
	}

	if (self.title_type_animation != NULL)
	{
		TSU_AnimationDestroy(&self.title_type_animation);
	}

	if (self.item_selector_animation != NULL)
	{
		TSU_AnimationDestroy(&self.item_selector_animation);
	}

	TSU_FontDestroy(&self.menu_font);
	TSU_MenuDestroy(&self.dsmenu_ptr);
	FreeGamesForce();

	self.scene_ptr = NULL;
}

bool RunGame()
{
	bool is_running = false;

	if (self.item_value_selected[0] != '\0')
	{
		ds_printf("DS_GAMES: Run: %s", self.item_value_selected);

		if (LoadPreset() == 1)
		{
			ds_printf("LoadPresset: %s", "OK");
			FreeAppData();

			isoldr_exec(self.isoldr, self.addr);
			is_running = true;
		}
	}

	return is_running;
}

static void* MenuExitHelper(void *params)
{
	// NEED IT TO SLEEP BECAUSE MARKS ERROR IN OPEN FUNCTION
	thd_sleep(1000);

	if (self.dsmenu_ptr != NULL)
	{
		if (!self.exit_app)
		{
			if (RunGame())
			{
				InitVideoThread();
				SDL_DC_EmulateMouse(SDL_TRUE);
				SDL_DC_EmulateKeyboard(SDL_TRUE);
				EnableScreen();
				GUI_Enable();
				ShutdownDS();
			}
			else
			{
				FreeAppData();
				InitVideoThread();
				SDL_DC_EmulateMouse(SDL_TRUE);
				SDL_DC_EmulateKeyboard(SDL_TRUE);
				EnableScreen();
				GUI_Enable();

				GamesApp_Exit(NULL);
			}
		}
		else
		{
			FreeAppData();
			InitVideoThread();
			SDL_DC_EmulateMouse(SDL_TRUE);
			SDL_DC_EmulateKeyboard(SDL_TRUE);
			EnableScreen();
			GUI_Enable();

			GamesApp_Exit(NULL);
		}
	}

	return NULL;
}

void GamesApp_ExitMenuEvent()
{
	exit_menu_thread = thd_create(0, &MenuExitHelper, NULL);
}

void GamesApp_Init(App_t *app)
{
	if (exit_menu_thread)
	{
		thd_destroy(exit_menu_thread);
	}
	exit_menu_thread = NULL;

	if (play_cdda_thread)
	{
		thd_destroy(play_cdda_thread);
	}
	play_cdda_thread = NULL;

	if (app->args != NULL)
	{
		self.have_args = true;
	}
	else
	{
		self.have_args = false;
	}

	self.isoldr = NULL;
	self.sector_size = 2048;
	self.exit_app = false;
	self.pages = -1;
	self.current_page = 0;
	self.previous_page = 0;
	self.game_count = 0;
	self.menu_cursel = 0;
	self.games_array_count = 0;
	self.games_array = NULL;
	self.item_selector = NULL;
	self.menu_font = NULL;
	self.item_selector_animation = NULL;
	self.title_animation = NULL;
	self.title_type_animation = NULL;
	self.title = NULL;	
	self.title_type = NULL;
	self.main_box = NULL;
	self.title_rectangle = NULL;
	self.title_type_rectangle = NULL;

	memset(&self.menu_option, 0, sizeof(self.menu_option));
	self.menu_type = MT_IMAGE_TEXT_64_5X2;

	memset(self.item_value_selected, 0, sizeof(self.item_value_selected));
	
	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		self.img_button[i] = NULL;
		self.img_button_animation[i] = NULL;
		self.exit_animation_list[i] = NULL;
		self.exit_trigger_list[i] = NULL;
	}

	if ((self.dsmenu_ptr = TSU_MenuCreateWithExit(GamesApp_InputEvent, GamesApp_ExitMenuEvent)) != NULL)
	{
		Color main_color = {1, 0.28f, 0.06f, 0.25f};
		self.main_box = TSU_BoxCreate(PVR_LIST_OP_POLY, 3, 480 - 12, 640 - 6, 480 - 9, 12, &main_color, ML_BACKGROUND, 0);
		
		Color title_color = {1, 0.28f, 0.06f, 0.25f};
		self.title_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 0, 40, 640, 40, &title_color, ML_BACKGROUND, 0);

		Color title_type_color = {1, 1.0f, 1.0f, 0.1f};
		self.title_type_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 552, 37, 640 - 555, 34, &title_type_color, ML_ITEM + 2, 3, &title_color, 0);

		TSU_MenuSubAddRectangle(self.dsmenu_ptr, self.title_rectangle);
		TSU_MenuSubAddRectangle(self.dsmenu_ptr, self.title_type_rectangle);
		TSU_MenuSubAddBox(self.dsmenu_ptr, self.main_box);
		
		InitMenu();		
	}
}

void GamesApp_Open(App_t *app)
{
	(void)app;

	if (self.dsmenu_ptr != NULL)
	{
		int mx = 0;
		int my = 0;
		SDL_GetMouseState(&mx, &my);
		SDL_DC_EmulateMouse(SDL_FALSE);
		SDL_DC_EmulateKeyboard(SDL_FALSE);

		DisableScreen();
		GUI_Disable();
		ShutdownVideoThread();
		TSU_MenuDoMenu(self.dsmenu_ptr);
	}
}

void GamesApp_Shutdown(App_t *app)
{
	(void)app;

	if (self.isoldr)
	{
		free(self.isoldr);
	}
}

void GamesApp_Exit(GUI_Widget *widget)
{
	(void)widget;

	App_t *app = NULL;
	if (self.have_args == true)
	{

		app = GetAppByName("File Manager");

		if (!app || !(app->state & APP_STATE_LOADED))
		{
			app = NULL;
		}
	}
	if (!app)
	{
		app = GetAppByName("Main");
	}

	OpenApp(app, NULL);
}