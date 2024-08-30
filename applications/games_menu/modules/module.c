/* DreamShell ##version##

   module.h - Games app module
   Copyright (C) 2024 Maniac Vera

*/

#include <ds.h>
#include "app_menu.h"
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
#include <tsunami/anims/fadein.h>
#include <tsunami/anims/fadeout.h>
#include <tsunami/triggers/chainanim.h>
#include <tsunami/triggers/death.h>
#include <tsunami/drawables/box.h>
#include <tsunami/dsapp.h>

DEFAULT_MODULE_EXPORTS(app_games_menu);

extern struct menu_structure menu_data;
static Event_t *do_menu_control_event;
static Event_t *do_menu_video_event;
static Event_t *do_menu_end_video_event;
static void* MenuExitHelper(void *params);
static mutex_t change_page_mutex = MUTEX_INITIALIZER;

static struct
{
	App_t *app;
	DSApp *dsapp_ptr;
	Scene *scene_ptr;

	bool have_args;
	int image_type;
	int sector_size;
	uint8 md5[16];
	uint8 boot_sector[2048];
	uint32 addr;
	isoldr_info_t *isoldr;
	
	bool exit_app;
	bool first_menu_load;
	volatile bool game_changed;
	uint32 scan_covers_start_time;
	uint32 scan_covers_end_time;
	int pages;
	int current_page;
	int previous_page;
	int game_count;
	int menu_cursel;
	int scan_count;

	char item_value_selected[NAME_MAX];
	LogXYMover *item_selector_animation;
	Rectangle *item_selector;
	Font *menu_font;
	Font *message_font;
	Trigger *exit_trigger_list[MAX_SIZE_ITEMS];
	LogXYMover *run_animation;
	ExpXYMover *exit_animation_list[MAX_SIZE_ITEMS];
	LogXYMover *item_game_animation[MAX_SIZE_ITEMS];
	ItemMenu *item_game[MAX_SIZE_ITEMS];
	ItemMenu *item_button[MAX_BUTTONS];
	Banner *img_cover_game_background;
	Banner *img_cover_game;
	FadeIn *animation_cover_game;
	FadeOut *fadeout_animation_cover;
	Texture *texture_cover_game_background;
	Texture *texture_cover_game;
	Texture *texture_info;
	Label *title, *title_back;
	Label *title_type;
	LogXYMover *title_animation;
	LogXYMover *title_back_animation;
	LogXYMover *title_type_animation;
	Box *main_box;
	Rectangle *title_rectangle;
	Rectangle *title_type_rectangle;
	Rectangle *modal_cover_scan;
	Rectangle *game_list_rectangle;
	Rectangle *img_cover_game_rectangle;
	Label *title_cover_scan;
	Label *note_cover_scan;
	Label *message_cover_scan;
	
	kthread_t *set_cover_thread;
} self;

static void HideCoverScan()
{
	if (self.modal_cover_scan != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.modal_cover_scan);
		TSU_RectangleDestroy(&self.modal_cover_scan);
	}

	if (self.title_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.title_cover_scan);
		TSU_LabelDestroy(&self.title_cover_scan);
	}

	if (self.note_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.note_cover_scan);
		TSU_LabelDestroy(&self.note_cover_scan);
	}

	if (self.message_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.message_cover_scan);
		TSU_LabelDestroy(&self.message_cover_scan);
	}
}

static void SetMessageScan(const char *fmt, const char *message)
{
	char message_scan[NAME_MAX];
	memset(message_scan, 0, sizeof(message_scan));
	snprintf(message_scan, sizeof(message_scan), fmt, message);
		
	if (self.message_cover_scan != NULL)
	{
		TSU_LabelSetText(self.message_cover_scan, message_scan);
	}
	else
	{
		Color color = {1, 1.0f, 1.0f, 1.0f};
		Vector vector_init_title = {640/2, 400/2, ML_CURSOR + 6, 1};
		vector_init_title.x = 50;
		vector_init_title.y += 40;
		
		self.message_cover_scan = TSU_LabelCreate(self.message_font, message_scan, 16, false, true);
		TSU_LabelSetTint(self.message_cover_scan, &color);
		TSU_AppSubAddLabel(self.dsapp_ptr, self.message_cover_scan);
		TSU_LabelSetTranslate(self.message_cover_scan, &vector_init_title);
	}
}

static void ShowCoverScan()
{
	self.scan_count++;
	StopCDDA();
	HideCoverScan();

	Vector vector_init_title = {640/2, 400/2 - 20, ML_CURSOR + 6, 1};
	Color modal_color = {1, 0.0f, 0.0f, 0.0f};
	Color modal_border_color = {1, 1.0f, 1.0f, 1.0f};	
	
	self.modal_cover_scan = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 40, 350, 560, 230, &modal_color, ML_CURSOR + 5, 3, &modal_border_color, 0);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.modal_cover_scan);

	static Color color = {1, 1.0f, 1.0f, 1.0f};
	self.title_cover_scan = TSU_LabelCreate(self.menu_font, "SCANNING MISSING COVER", 26, true, true);	
	TSU_LabelSetTint(self.title_cover_scan, &color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.title_cover_scan);
	TSU_LabelSetTranslate(self.title_cover_scan, &vector_init_title);

	vector_init_title.x = 640/2;
	vector_init_title.y = 400/2 + 130;
	self.note_cover_scan = TSU_LabelCreate(self.menu_font, "THE SCAN WILL APPEAR 2 TIMES", 12, true, true);
	TSU_LabelSetTint(self.note_cover_scan, &color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.note_cover_scan);
	TSU_LabelSetTranslate(self.note_cover_scan, &vector_init_title);
}

static void* SetCoverThread(void *params)
{
	int game_index = (int)params;
	if (game_index >= 0)
	{
		srand(time(NULL));
		uint time_elapsed = 0;
		if (self.img_cover_game != NULL)
		{
			if (self.animation_cover_game != NULL)
			{
				TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
				TSU_FadeInDestroy(&self.animation_cover_game);
			}
			
			if (self.fadeout_animation_cover == NULL)
			{			
				self.fadeout_animation_cover = TSU_FadeOutCreate(10.0f);
				TSU_DrawableAnimAdd((Drawable *)self.img_cover_game, (Animation *)self.fadeout_animation_cover);			
			}

			const uint max_time = 200;			
			while (!self.game_changed && menu_data.menu_type == MT_PLANE_TEXT && time_elapsed < max_time)
			{
				thd_sleep(50);
				time_elapsed += 50;
			}

			if (self.game_changed || menu_data.menu_type != MT_PLANE_TEXT)
				return NULL;

			TSU_AnimationComplete((Animation *)self.fadeout_animation_cover, (Drawable *)self.img_cover_game);
			TSU_FadeOutDestroy(&self.fadeout_animation_cover);

			TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game);
			TSU_BannerDestroy(&self.img_cover_game);
			TSU_TextureDestroy(&self.texture_cover_game);
		}

		const uint max_time = 400;
		while (!self.game_changed && menu_data.menu_type == MT_PLANE_TEXT && time_elapsed < max_time)
		{
			thd_sleep(50);
			time_elapsed += 50;
		}

		if (self.game_changed || menu_data.menu_type != MT_PLANE_TEXT)
			return NULL;

		if (CheckCover(game_index) == SC_EXISTS)
		{
			Vector vector_position = {490, 180, ML_ITEM + 2, 1};
			char *game_cover_path = NULL;
			if (GetGameCoverPath(game_index, &game_cover_path))
			{
				self.texture_cover_game = TSU_TextureCreateFromFile(game_cover_path, menu_data.games_array[game_index].cover_type == IT_PNG, false, 0);
				self.img_cover_game = TSU_BannerCreate(menu_data.games_array[game_index].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY, self.texture_cover_game);
				free(game_cover_path);
				
				TSU_DrawableTranslate((Drawable *)self.img_cover_game, &vector_position);
				
				vector_position.x = 1.0f;
				vector_position.y = 1.0f;

				TSU_DrawableSetScale((Drawable *)self.img_cover_game, &vector_position);			
				self.animation_cover_game = TSU_FadeInCreate(7.0f);
				TSU_DrawableAnimAdd((Drawable *)self.img_cover_game, (Animation *)self.animation_cover_game);

				TSU_BannerSetSize(self.img_cover_game, 0, 0);
				TSU_AppSubAddBanner(self.dsapp_ptr, self.img_cover_game);
				thd_sleep(20);
				TSU_BannerSetSize(self.img_cover_game, 256, 256);
			}
		}
	}

	return NULL;
}

static bool WaitCoverThread()
{
	if (menu_data.menu_type == MT_PLANE_TEXT && self.set_cover_thread != NULL)
	{
		thd_join(self.set_cover_thread, NULL);
		self.set_cover_thread = NULL;
	}

	return (self.set_cover_thread == NULL);
}

static void SetCover(int game_index)
{
	if (menu_data.menu_type == MT_PLANE_TEXT && game_index >= 0)
	{
		if (WaitCoverThread())
		{
			self.set_cover_thread = thd_create(0, SetCoverThread, (void *)game_index);
		}
	}
}

static bool LoadCover(ItemMenu *item_menu, int game_count)
{
	bool loaded_cover = false;

	if (item_menu != NULL)
	{
		int game_index = TSU_ItemMenuGetItemIndex(item_menu);

		if (menu_data.menu_type == MT_PLANE_TEXT)
		{
			if (self.menu_cursel == game_count)
			{
				SetCover(game_index);
			}
		}
		else
		{
			char *game_cover_path = NULL;
			if (GetGameCoverPath(game_index, &game_cover_path))
			{
				TSU_ItemMenuSetImage(item_menu, game_cover_path, menu_data.games_array[game_index].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY);
				loaded_cover = true;
				free(game_cover_path);
			}
		}
	}

	return loaded_cover;
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

	static Vector vectorInit = {-50, 34, ML_ITEM, 1};
	static Vector vector = {10, 34, ML_ITEM, 1};
	vectorInit.z = ML_ITEM + 1;
	vector.z = ML_ITEM + 1;
	
	if (self.title != NULL)
	{
		TSU_LabelSetText(self.title_back, titleText);
		TSU_LabelSetText(self.title, titleText);
	}
	else
	{
		static Color color = {1, 1.0f, 1.0f, 1.0f};
		self.title = TSU_LabelCreate(self.menu_font, titleText, 26, false, false);
		TSU_LabelSetTint(self.title, &color);
		TSU_AppSubAddLabel(self.dsapp_ptr, self.title);

		static Color color_back = {1, 0.0f, 0.0f, 0.0f};
		self.title_back = TSU_LabelCreate(self.menu_font, titleText, 26, false, false);
		TSU_LabelSetTint(self.title_back, &color_back);
		TSU_AppSubAddLabel(self.dsapp_ptr, self.title_back);
	}

	if (self.title_animation != NULL)
	{
		TSU_AnimationComplete((Animation *)self.title_animation, (Drawable *)self.title);
		TSU_LogXYMoverDestroy(&self.title_animation);

		TSU_AnimationComplete((Animation *)self.title_back_animation, (Drawable *)self.title_back);
		TSU_LogXYMoverDestroy(&self.title_back_animation);
	}

	TSU_LabelSetTranslate(self.title, &vectorInit);
	self.title_animation = TSU_LogXYMoverCreate(vector.x, vector.y);
	TSU_DrawableAnimAdd((Drawable *)self.title, (Animation *)self.title_animation);

	vectorInit.z = ML_ITEM;
	TSU_LabelSetTranslate(self.title_back, &vectorInit);
	self.title_back_animation = TSU_LogXYMoverCreate(vector.x, vector.y);
	TSU_DrawableAnimAdd((Drawable *)self.title_back, (Animation *)self.title_back_animation);
}

static void SetTitleType(const char *full_path_game, bool is_gdi_optimized)
{
	if (full_path_game != NULL)
	{
		Color title_type_color = {1, 1.0f, 0.95f, 0.0f};
		const char *file_type = strrchr(full_path_game, '.');

		char title_text[4];
		memset(title_text, 0, sizeof(title_text));

		if (is_gdi_optimized)
		{
			strncpy(title_text, "GDI", 3);			
		}
		else if (strcasecmp(file_type, ".cdi") == 0)
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
			strncpy(title_text, "GDI", 3);

			title_type_color.r = 1.0f;
			title_type_color.g = 0.54f;
			title_type_color.b = 0.0f;
		}

		TSU_DrawableSetTint((Drawable *)self.title_type_rectangle, &title_type_color);

		static Vector vectorInit = {700, 17, ML_ITEM + 6, 1};
		static Vector vector = {612, 17, ML_ITEM + 6, 1};
		vectorInit.z = ML_ITEM + 6;

		if (self.title_type != NULL)
		{
			TSU_LabelSetText(self.title_type, title_text);
		}
		else
		{
			static Color color = {1, 1.0f, 1.0f, 1.0f};
			self.title_type = TSU_LabelCreate(self.menu_font, title_text, 26, true, true);
			TSU_LabelSetTint(self.title_type, &color);
			TSU_AppSubAddLabel(self.dsapp_ptr, self.title_type);
		}

		if (self.title_type_animation != NULL)
		{
			TSU_AnimationComplete((Animation *)self.title_type_animation, (Drawable *)self.title_type);
			TSU_LogXYMoverDestroy(&self.title_type_animation);
		}

		TSU_LabelSetTranslate(self.title_type, &vectorInit);
		self.title_type_animation = TSU_LogXYMoverCreate(vector.x, vector.y);
		TSU_DrawableAnimAdd((Drawable *)self.title_type, (Animation *)self.title_type_animation);
	}
}

static void SetCursor()
{
	static Vector selector_translate = {0, 0, ML_CURSOR, 1};

	if (self.item_selector_animation != NULL)
	{
		TSU_AnimationComplete((Animation *)self.item_selector_animation, (Drawable *)self.item_selector);
		TSU_LogXYMoverDestroy(&self.item_selector_animation);
	}

	if (self.item_selector != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.item_selector);
		TSU_RectangleDestroy(&self.item_selector);
	}

	if (self.item_selector == NULL)
	{
		if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
		{
			Color color = {0, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};

			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 3, &border_color, 0);
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}
		else if (menu_data.menu_type == MT_IMAGE_128_4X3)
		{
			Color color = {0, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};			
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 6, &border_color, 0);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}
		else if (menu_data.menu_type == MT_PLANE_TEXT)
		{
			Color color = {0.8f, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};			
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 2, &border_color, 0);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}
		else
		{
			Color color = {0, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};			
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 4, &border_color, 0);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}

		if (selector_translate.x > 0 && selector_translate.y > 0)
		{
			TSU_DrawableSetTranslate((Drawable *)self.item_selector, &selector_translate);
		}

		TSU_AppSubAddRectangle(self.dsapp_ptr, self.item_selector);
	}

	static int init_position_x;
	static int init_position_y;

	if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
	{
		TSU_RectangleSetSize(self.item_selector, 310, menu_data.menu_option.image_size + 8);
		init_position_x = 5;
		init_position_y = 34;
		selector_translate.z = ML_CURSOR;
	}
	else if (menu_data.menu_type == MT_IMAGE_128_4X3)
	{
		TSU_RectangleSetSize(self.item_selector, menu_data.menu_option.image_size + 4, menu_data.menu_option.image_size + 4);
		init_position_x = 9;
		init_position_y = 41;
		selector_translate.z = ML_CURSOR;
	}
	else if (menu_data.menu_type == MT_PLANE_TEXT)
	{
		TSU_RectangleSetSize(self.item_selector, 329, 30);
		init_position_x = 18;
		init_position_y = 52;
		selector_translate.z = ML_SELECTED - 1;
	}
	else
	{
		TSU_RectangleSetSize(self.item_selector, menu_data.menu_option.image_size + 4, menu_data.menu_option.image_size + 4);
		init_position_x = 8;
		init_position_y = 40;
		selector_translate.z = ML_CURSOR;
	}

	TSU_DrawableSetTranslate((Drawable *)self.item_selector, &selector_translate);
	selector_translate.x = self.menu_cursel / menu_data.menu_option.size_items_column * menu_data.menu_option.padding_x + init_position_x;
	selector_translate.y = ((self.menu_cursel + 1) - menu_data.menu_option.size_items_column * (self.menu_cursel / menu_data.menu_option.size_items_column)) * (menu_data.menu_option.padding_y + menu_data.menu_option.image_size) + init_position_y;

	self.item_selector_animation = TSU_LogXYMoverCreate(selector_translate.x, selector_translate.y);
	TSU_DrawableAnimAdd((Drawable *)self.item_selector, (Animation *)self.item_selector_animation);
}

static void RemoveViewTextPlane()
{
	if (self.img_cover_game_background != NULL)
	{
		TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game_background);
		TSU_BannerDestroy(&self.img_cover_game_background);
		TSU_TextureDestroy(&self.texture_cover_game_background);
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.img_cover_game_rectangle);
		TSU_RectangleDestroy(&self.img_cover_game_rectangle);
	}	

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_AppSubRemoveItemMenu(self.dsapp_ptr, self.item_button[i]);
			TSU_ItemMenuDestroy(&self.item_button[i]);
		}
	}

	if (self.img_cover_game != NULL)
	{
		if (self.animation_cover_game != NULL)
		{
			TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
			TSU_FadeInDestroy(&self.animation_cover_game);
		}

		if (self.fadeout_animation_cover != NULL)
		{
			TSU_AnimationComplete((Animation *)self.fadeout_animation_cover, (Drawable *)self.img_cover_game);
			TSU_FadeOutDestroy(&self.fadeout_animation_cover);
		}		

		TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game);
		TSU_BannerDestroy(&self.img_cover_game);
		TSU_TextureDestroy(&self.texture_cover_game);
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.game_list_rectangle);
		TSU_RectangleDestroy(&self.game_list_rectangle);
	}
}

static void CreateInfoButton(uint8 button_index, const char *button_file, const char *text, float x, float y)
{
	if(self.item_button[button_index] == NULL)
	{
		char *image_path = (char *)malloc(NAME_MAX);
		snprintf(image_path, NAME_MAX, "%s/%s/%s", menu_data.default_dir, "apps/games_menu/images", button_file);

		if (FileExists(image_path))
		{
			Vector vector_translate = { x, y, ML_ITEM + 3, 1};
			self.item_button[button_index] = TSU_ItemMenuCreate(image_path, 32, 32, PVR_LIST_TR_POLY, text, self.menu_font, 14, false, 0);
			TSU_ItemMenuSetTranslate(self.item_button[button_index], &vector_translate);			
		}

		free(image_path);
	}
}

static void AddInfoButtons()
{
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_AppSubAddItemMenu(self.dsapp_ptr, self.item_button[i]);
		}
	}
}

static void CreateViewTextPlane()
{
	CreateInfoButton(0, "btn_x.png", "CHANGE VIEW", 359, 315);
	CreateInfoButton(1, "btn_y.png", "UNUSED", 512, 315);
	CreateInfoButton(2, "btn_a.png", "PLAY", 359, 315 + (32 * 1) + (5 * 1));
	CreateInfoButton(3, "btn_b.png", "EXIT", 512, 315 + (32 * 1) + (5 * 1));
	CreateInfoButton(4, "btn_lt.png", "PREVIOUS", 359, 315 + (32 * 2) + (5 * 2));
	CreateInfoButton(5, "btn_rt.png", "NEXT", 512, 315 + (32 * 2) + (5 * 2));
	CreateInfoButton(6, "btn_start.png", "UNUSED", 359, 315 + (32 * 3) + (5 * 3));
	AddInfoButtons();

	if (self.game_list_rectangle == NULL)
	{
		Color border_color = {1, 1.0f, 1.0f, 1.0f};
		Color background_color = {1, 0.22f, 0.06f, 0.25f};
		self.game_list_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 15, 460, 335, 405, &background_color, ML_ITEM, 3, &border_color, 0);

		TSU_AppSubAddRectangle(self.dsapp_ptr, self.game_list_rectangle);
	}

	if (self.img_cover_game_background == NULL)
	{
		char *game_cover_path = (char *)malloc(NAME_MAX);
		snprintf(game_cover_path, NAME_MAX, "%s/%s", menu_data.default_dir, "apps/games_menu/images/cover.png");

		if (FileExists(game_cover_path))
		{
			self.texture_cover_game_background = TSU_TextureCreateFromFile(game_cover_path, true, false, 0);
			self.img_cover_game_background = TSU_BannerCreate(PVR_LIST_TR_POLY, self.texture_cover_game_background);

			Vector vector_position = {490, 180, ML_ITEM + 3, 1};
			TSU_DrawableTranslate((Drawable *)self.img_cover_game_background, &vector_position);
			TSU_BannerSetSize(self.img_cover_game_background, 256, 256);
			TSU_AppSubAddBanner(self.dsapp_ptr, self.img_cover_game_background);

			Color background_color = {1, 0.88f, 0.88f, 0.88f};
			self.img_cover_game_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, vector_position.x - 128, vector_position.y + 128, 256, 256, &background_color, ML_ITEM + 1, 0);
			TSU_AppSubAddRectangle(self.dsapp_ptr, self.img_cover_game_rectangle);
		}

		free(game_cover_path);
	}
}

static bool LoadPage(bool change_view, uint8 direction)
{
	const int LENGTH_NAME_TEXT_PLANE = 26;
	const int LENGTH_NAME_TEXT_64_5X2 = 19;
	bool loaded = false;
	char name_truncated[LENGTH_NAME_TEXT_PLANE];
	char game_cover_path[NAME_MAX];
	char name[NAME_MAX];
	char *game_cover_path_tmp = NULL;	

	if (self.pages == -1)
	{
		if (RetrieveGames())
		{
			timer_ms_gettime(&self.scan_covers_start_time, NULL);
			self.pages = (menu_data.games_array_count > 0 ? ceil((float)menu_data.games_array_count / (float)menu_data.menu_option.max_page_size) : 0);
		}
	}
	else
	{
		if (change_view)
		{
			self.pages = (menu_data.games_array_count > 0 ? ceil((float)menu_data.games_array_count / (float)menu_data.menu_option.max_page_size) : 0);
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
				if (self.item_game_animation[i] != NULL)
				{
					TSU_AnimationComplete((Animation *)self.item_game_animation[i], (Drawable *)self.item_game[i]);
					TSU_LogXYMoverDestroy(&self.item_game_animation[i]);
				}

				if (self.item_game[i] != NULL)
				{
					TSU_AppSubRemoveItemMenu(self.dsapp_ptr, self.item_game[i]);
					TSU_ItemMenuDestroy(&self.item_game[i]);
				}
			}

			if (change_view || self.first_menu_load)
			{
				if (menu_data.menu_type == MT_PLANE_TEXT)
				{
					CreateViewTextPlane();
				}
				else
				{
					RemoveViewTextPlane();
				}				
			}

			int column = 0;
			int page = 0;
			self.game_count = 0;
			Vector vectorTranslate = {0, 480, ML_ITEM, 1};

			for (int icount = ((self.current_page - 1) * menu_data.menu_option.max_page_size); icount < menu_data.games_array_count; icount++)
			{
				page = ceil((float)(icount + 1) / (float)menu_data.menu_option.max_page_size);

				if (page < self.current_page)
					continue;
				else if (page > self.current_page)
					break;

				self.game_count++;

				column = floor((float)(self.game_count - 1) / (float)menu_data.menu_option.size_items_column);				
				
				if (menu_data.menu_type != MT_PLANE_TEXT)
				{
					game_cover_path_tmp = NULL;
					CheckCover(icount);
					if (GetGameCoverPath(icount, &game_cover_path_tmp))
					{
						strcpy(game_cover_path, game_cover_path_tmp);
						free(game_cover_path_tmp);
					}
				}

				memset(name, 0, sizeof(name));
				if (menu_data.games_array[icount].is_folder_name)
				{
					strcpy(name, menu_data.games_array[icount].folder_name);
				}
				else
				{
					strncpy(name, menu_data.games_array[icount].game, strlen(menu_data.games_array[icount].game) - 4);
				}

				if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
				{
					memset(name_truncated, 0, LENGTH_NAME_TEXT_PLANE);
					if (strlen(name) > LENGTH_NAME_TEXT_64_5X2 - 1)
					{
						strncpy(name_truncated, name, LENGTH_NAME_TEXT_64_5X2 - 1);
					}
					else
					{
						strcpy(name_truncated, name);
					}

					self.item_game[self.game_count - 1] = TSU_ItemMenuCreate(game_cover_path, menu_data.menu_option.image_size, menu_data.menu_option.image_size
								, menu_data.games_array[icount].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY
								, name_truncated, self.menu_font, 18, false, 0);
					
				}
				else if (menu_data.menu_type == MT_PLANE_TEXT)
				{
					memset(name_truncated, 0, LENGTH_NAME_TEXT_PLANE);
					if (strlen(name) > LENGTH_NAME_TEXT_PLANE - 1)
					{
						strncpy(name_truncated, name, LENGTH_NAME_TEXT_PLANE - 1);
					}
					else
					{
						strcpy(name_truncated, name);
					}

					self.item_game[self.game_count - 1] = TSU_ItemMenuCreateLabel(name_truncated, self.menu_font, 18);
					Color color_unselected = { 1, 0.85f, 0.85f, 0.85f };
					TSU_ItemMenuSetColorUnselected(self.item_game[self.game_count - 1], &color_unselected);
				}
				else
				{					
					self.item_game[self.game_count - 1] = TSU_ItemMenuCreateImage(game_cover_path
							, menu_data.menu_option.image_size
							, menu_data.menu_option.image_size
							, menu_data.games_array[icount].cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY
							, false
							, PVR_TXRLOAD_SQ);
				}

				ItemMenu *item_menu = self.item_game[self.game_count - 1];

				TSU_ItemMenuSetItemIndex(item_menu, icount);
				TSU_ItemMenuSetItemValue(item_menu, name);

				if (menu_data.menu_type == MT_PLANE_TEXT)
				{
					switch (direction)
					{
						case DMD_UP:
						case DMD_LEFT:
							vectorTranslate.x = 50;
							vectorTranslate.y = (menu_data.menu_option.image_size + menu_data.menu_option.padding_y) * (self.game_count - menu_data.menu_option.size_items_column * column) + menu_data.menu_option.init_position_y;
							break;

						case DMD_DOWN:
						case DMD_RIGHT:
							vectorTranslate.x = -50;
							vectorTranslate.y = (menu_data.menu_option.image_size + menu_data.menu_option.padding_y) * (self.game_count - menu_data.menu_option.size_items_column * column) + menu_data.menu_option.init_position_y;
							break;
						
						default:
							vectorTranslate.x = 50;
							break;
					}

					TSU_ItemMenuSetTranslate(item_menu, &vectorTranslate);
				}
				else
				{
					TSU_ItemMenuSetTranslate(item_menu, &vectorTranslate);
				}

				self.item_game_animation[self.game_count - 1] = TSU_LogXYMoverCreate(menu_data.menu_option.init_position_x + (column * menu_data.menu_option.padding_x),
																									(menu_data.menu_option.image_size + menu_data.menu_option.padding_y) * (self.game_count - menu_data.menu_option.size_items_column * column) + menu_data.menu_option.init_position_y);

				TSU_ItemMenuAnimAdd(item_menu, (Animation *)self.item_game_animation[self.game_count - 1]);

				if (self.first_menu_load)
				{
					if (self.game_count == 1)
					{
						TSU_ItemMenuSetSelected(item_menu, true, false);
						SetTitle(name);
						SetTitleType(GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(item_menu))
							, CheckGdiOptimized(TSU_ItemMenuGetItemIndex(item_menu)));

						SetCursor();
						SetCover(TSU_ItemMenuGetItemIndex(item_menu));
						PlayCDDA(TSU_ItemMenuGetItemIndex(item_menu));
					}
					else
					{
						TSU_ItemMenuSetSelected(item_menu, false, false);
					}
				}
				
				TSU_AppSubAddItemMenu(self.dsapp_ptr, item_menu);
			}

			loaded = true;
			self.first_menu_load = false;
			FreeGames();
		}
	}
	else
	{	
		memset(game_cover_path, 0, sizeof(game_cover_path));

		char font_path[NAME_MAX];
		memset(font_path, 0, sizeof(font_path));
		snprintf(font_path, sizeof(font_path), "%s/%s", menu_data.default_dir, "apps/games_menu/fonts/message.txf");

		TSU_LabelDestroy(&self.title);
		TSU_FontDestroy(&self.menu_font);
		self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
		
		snprintf(game_cover_path, sizeof(game_cover_path), "You need put the games here:\n%s\n\n\nand images here:\n%s", menu_data.games_path, menu_data.covers_path);
		ds_printf(game_cover_path);
		SetTitle(game_cover_path);
	}

	return loaded;
}

static void InitMenu()
{
	self.scene_ptr = TSU_AppGetScene(self.dsapp_ptr);
	
	char font_path[NAME_MAX];
	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", menu_data.default_dir, "apps/games_menu/fonts/default.txf");
	self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", menu_data.default_dir, "apps/games_menu/fonts/message.txf");
	self.message_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	SetMenuType(menu_data.menu_type);
	self.exit_app = false;
	self.title = NULL;
	self.title_type = NULL;
	self.title_back = NULL;

	Vector vector = {0, 0, 10, 1};
	TSU_AppSetTranslate(self.dsapp_ptr, &vector);

	if (LoadScannedCover())
	{
		CleanIncompleteCover();
	}
	else 
	{
		memset(&menu_data.cover_scanned_app, 0, sizeof(CoverScannedStruct));
	}

	self.current_page = 1;
	LoadPage(false, DMD_NONE);

	self.menu_cursel = 0;
}

static void DoMenuEndVideoHandler(void *ds_event, void *param, int action)
{ 
	switch(action) {
		case EVENT_ACTION_RENDER:
			{
				if (TSU_AppEnd(self.dsapp_ptr))
				{
					menu_data.finished_menu = true;
				}
			}
			break;
		case EVENT_ACTION_RENDER_POST:
			break;
		case EVENT_ACTION_UPDATE:
			break;
		default:
			break;
	}
}

static void StartExit()
{
	StopCDDA();
	WaitCoverThread();
	float y = 1.0f;

	if (self.main_box != NULL)
	{
		TSU_AppSubRemoveBox(self.dsapp_ptr, self.main_box);
	}

	if (self.title_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.title_rectangle);
	}

	if (self.title_type_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.title_type_rectangle);
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.game_list_rectangle);
	}	

	if (self.item_selector != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.item_selector);
	}

	if (self.title != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.title);
	}

	if (self.title_type != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.title_type);
	}

	if (self.title_back != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.title_back);
	}

	if (self.animation_cover_game != NULL)
	{
		TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
		TSU_FadeInDestroy(&self.animation_cover_game);		
	}

	if (self.fadeout_animation_cover != NULL)
	{
		TSU_AnimationComplete((Animation *)self.fadeout_animation_cover, (Drawable *)self.img_cover_game);
		TSU_FadeOutDestroy(&self.fadeout_animation_cover);		
	}
	
	if (self.img_cover_game != NULL)
	{
		TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game);
	}

	if (self.img_cover_game_background != NULL)
	{
		TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game_background);
	}

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_AppSubRemoveItemMenu(self.dsapp_ptr, self.item_button[i]);
		}
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.img_cover_game_rectangle);
	}	

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game[i] != NULL)
		{
			if (!self.exit_app && TSU_ItemMenuIsSelected(self.item_game[i]))
			{
				strcpy(self.item_value_selected, GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.item_game[i])));

				if (menu_data.menu_type == MT_PLANE_TEXT)
				{
					float w, h;
					char *label_text = (char *)malloc(NAME_MAX);
					memset(label_text, 0, NAME_MAX);

					strcpy(label_text, TSU_ItemMenuGetLabelText(self.item_game[i]));
					label_text = Trim(label_text);

					TSU_FontGetTextSize(self.menu_font, label_text, &w, &h);
					self.run_animation = TSU_LogXYMoverCreate((640/2 - w/2), (480 - 84) / 2);

					free(label_text);
				}
				else if (TSU_ItemMenuHasTextAndImage(self.item_game[i]))
				{
					self.run_animation = TSU_LogXYMoverCreate((640 - (84 + 64)) / 2, (480 - 84) / 2);
				}
				else
				{
					self.run_animation = TSU_LogXYMoverCreate((640 - 84) / 2, (480 - 84) / 2);
				}

				self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);
				TSU_TriggerAdd((Animation *)self.run_animation, self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.item_game[i], (Animation *)self.run_animation);
			}
			else
			{				
				self.exit_animation_list[i] = TSU_ExpXYMoverCreate(0, y + .10, 0, 400);
				self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);

				TSU_TriggerAdd((Animation *)self.exit_animation_list[i], self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.item_game[i], (Animation *)self.exit_animation_list[i]);
			}
		}
	}
	TSU_AppStartExit(self.dsapp_ptr);

	RemoveEvent(do_menu_video_event);
	RemoveEvent(do_menu_control_event);

	do_menu_end_video_event = AddEvent
	(
		"GamesEndVideo_Event",
		EVENT_TYPE_VIDEO,
		EVENT_PRIO_DEFAULT,
		DoMenuEndVideoHandler,
		NULL
	);	
	
	while (!menu_data.finished_menu) { thd_pass(); }
	MenuExitHelper(NULL);
}

static void GamesApp_InputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;
	
	mutex_lock(&change_page_mutex);

	bool skip_cursor = false;
	switch (key)
	{
		// X: CHANGE VISUALIZATION
		case KeyMiscX:
		{
			StopCDDA();
			WaitCoverThread();

			int real_cursel = (self.current_page - 1) * menu_data.menu_option.max_page_size + self.menu_cursel + 1;
			SetMenuType(menu_data.menu_type >= MT_IMAGE_128_4X3 ? MT_PLANE_TEXT : menu_data.menu_type + 1);
			self.current_page = ceil((float)real_cursel / (float)menu_data.menu_option.max_page_size);
			self.menu_cursel = real_cursel - (self.current_page - 1) * menu_data.menu_option.max_page_size - 1;

			if (LoadPage(true, DMD_NONE))
			{
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
			self.menu_cursel = 0;
			if (LoadPage(false, DMD_RIGHT))
			{
			}
		}
		break;

		// RIGHT TRIGGER
		case KeyPgdn:
		{
			self.current_page++;
			self.menu_cursel = 0;
			if (LoadPage(false, DMD_LEFT))
			{		
			}
		}
		break;

		case KeyUp:
		{
			self.menu_cursel--;
			
			if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
			{
				if (self.menu_cursel < 0)
				{
					self.menu_cursel += self.game_count;
				}
			}
			else if (menu_data.menu_type == MT_PLANE_TEXT)
			{
				if (self.menu_cursel < 0)
				{
					self.current_page--;					
					LoadPage(false, DMD_DOWN);
					self.menu_cursel = self.game_count - 1;
				}
			}
			else
			{
				if ((self.menu_cursel + 1) % menu_data.menu_option.size_items_column == 0)
				{
					self.menu_cursel += menu_data.menu_option.size_items_column;
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

			if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
			{
				if (self.menu_cursel >= self.game_count)
				{
					self.menu_cursel -= self.game_count;
				}
			}
			else if (menu_data.menu_type == MT_PLANE_TEXT)
			{
				if (self.menu_cursel >= self.game_count)
				{
					self.current_page++;
					self.menu_cursel = 0;
					LoadPage(false, DMD_UP);
				}
			}
			else
			{
				if (self.menu_cursel % menu_data.menu_option.size_items_column == 0)
				{
					self.menu_cursel -= menu_data.menu_option.size_items_column;
				}

				if (self.menu_cursel >= self.game_count)
				{
					self.menu_cursel = self.game_count - 1;
				}
			}
		}

		break;

		case KeyLeft:

			if (menu_data.menu_type == MT_PLANE_TEXT)
			{
				self.current_page--;
				self.menu_cursel = 0;
				if (LoadPage(false, DMD_RIGHT))
				{
				}
			}
			else
			{
				if (self.game_count <= menu_data.menu_option.size_items_column)
				{
					self.menu_cursel = 0;
				}
				else
				{
					self.menu_cursel -= menu_data.menu_option.size_items_column;

					if (self.menu_cursel < 0)
						self.menu_cursel += self.game_count;
				}
			}

			break;

		case KeyRight:
			if (menu_data.menu_type == MT_PLANE_TEXT)
			{
				self.current_page++;
				self.menu_cursel = 0;
				if (LoadPage(false, DMD_LEFT))
				{
				}
			}
			else
			{
				if (self.game_count <= menu_data.menu_option.size_items_column)
				{
					self.menu_cursel = self.game_count - 1;
				}
				else
				{
					self.menu_cursel += menu_data.menu_option.size_items_column;

					if (self.menu_cursel >= self.game_count)
						self.menu_cursel -= self.game_count;
				}
			}

			break;

		case KeySelect:
			{
				StartExit();
				skip_cursor = true;
			}
			break;

		default:
			{
			}
			break;
	}

	if (!skip_cursor)
	{
		StopCDDA();
		self.game_changed = true;
		for (int i = 0; i < self.game_count; i++)
		{
			if (self.item_game[i] != NULL)
			{
				if (i == self.menu_cursel)
				{
					TSU_ItemMenuSetSelected(self.item_game[i], true, false);
					SetTitle(TSU_ItemMenuGetItemValue(self.item_game[i]));

					SetTitleType(GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.item_game[i]))
						, CheckGdiOptimized(TSU_ItemMenuGetItemIndex(self.item_game[i])));

					SetCursor();
					SetCover(TSU_ItemMenuGetItemIndex(self.item_game[i]));
					PlayCDDA(TSU_ItemMenuGetItemIndex(self.item_game[i]));
				}
				else
				{
					TSU_ItemMenuSetSelected(self.item_game[i], false, false);
				}
			}
		}
		self.game_changed = false;
	}

	mutex_unlock(&change_page_mutex);
}

static int CanUseTrueAsyncDMA(void)
{
	return (self.sector_size == 2048 &&
			(menu_data.current_dev == APP_DEVICE_IDE || menu_data.current_dev == APP_DEVICE_CD) &&
			(self.image_type == ISOFS_IMAGE_TYPE_ISO || self.image_type == ISOFS_IMAGE_TYPE_GDI));
}

static void DefaultPreset()
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

	self.isoldr->alt_read = 0;
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
		char *track_file_path = NULL;
		size_t track_size = GetCDDATrackFilename(4, self.item_value_selected, &track_file_path);

		if (track_size && track_size < 30 * 1024 * 1024)
		{
			track_size = GetCDDATrackFilename(6, self.item_value_selected, &track_file_path);
		}

		if (track_size > 0)
		{
			self.isoldr->use_irq = 1;
			self.isoldr->emu_cdda = 1;
		}

		if (track_file_path)
		{
			free(track_file_path);
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
		preset_file_name = MakePresetFilename(menu_data.default_dir, self.md5);
		ds_printf("PresetFileName: %s", preset_file_name);

		if (FileSize(preset_file_name) < 5)
		{
			preset_file_name = NULL;
		}
	}

	int use_dma = 0, emu_async = 16, use_irq = 0, alt_read = 0;
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
		{"altread", CONF_INT, (void *)&alt_read},
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

	if ((self.isoldr = isoldr_get_info(self.item_value_selected, 0)) == NULL)	
	{
		return 0;
	}
	
	if (preset_file_name == NULL || ConfigParse(options, preset_file_name) == -1)
	{
		ds_printf("DS_ERROR: Can't parse preset\n");
		DefaultPreset();
		default_preset_loaded = true;
	}

	if (!default_preset_loaded)
	{
		self.isoldr->use_dma = use_dma;
		self.isoldr->alt_read = alt_read;
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
		if (!strncasecmp(menu_data.default_dir, "/cd", 3))
		{
			strcpy(self.isoldr->fs_dev, "cd");
		}
		else if (!strncasecmp(menu_data.default_dir, "/sd", 3))
		{
			strcpy(self.isoldr->fs_dev, "sd");
		}
		else if (!strncasecmp(menu_data.default_dir, "/ide", 4))
		{
			strcpy(self.isoldr->fs_dev, "ide");
		}
	}

	return 1;
}

static void FreeAppData()
{
	mutex_destroy(&change_page_mutex);
	DestroyMenuData();

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game_animation[i] != NULL)
		{
			TSU_LogXYMoverDestroy(&self.item_game_animation[i]);
		}

		if (self.item_game[i] != NULL)
		{
			TSU_ItemMenuDestroy(&self.item_game[i]);
		}

		if (self.exit_animation_list[i] != NULL)
		{
			TSU_ExpXYMoverDestroy(&self.exit_animation_list[i]);
		}

		if (self.exit_trigger_list[i] != NULL)
		{
			TSU_TriggerDestroy(&self.exit_trigger_list[i]);
		}
	}

	if (self.run_animation != NULL)
	{
		TSU_LogXYMoverDestroy(&self.run_animation);
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

	if (self.game_list_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.game_list_rectangle);
	}	

	if (self.item_selector != NULL)
	{
		TSU_RectangleDestroy(&self.item_selector);
	}

	if (self.title != NULL)
	{
		TSU_LabelDestroy(&self.title);
	}

	if (self.title_back != NULL)
	{
		TSU_LabelDestroy(&self.title_back);
	}

	if (self.title_animation != NULL)
	{
		TSU_LogXYMoverDestroy(&self.title_animation);
	}

	if (self.title_back_animation != NULL)
	{
		TSU_LogXYMoverDestroy(&self.title_back_animation);
	}

	if (self.title_type != NULL)
	{
		TSU_LabelDestroy(&self.title_type);
	}

	if (self.title_type_animation != NULL)
	{
		TSU_LogXYMoverDestroy(&self.title_type_animation);
	}

	if (self.item_selector_animation != NULL)
	{
		TSU_LogXYMoverDestroy(&self.item_selector_animation);
	}

	if (self.animation_cover_game != NULL)
	{
		TSU_FadeInDestroy(&self.animation_cover_game);		
	}

	if (self.fadeout_animation_cover != NULL)
	{
		TSU_FadeOutDestroy(&self.fadeout_animation_cover);		
	}

	if (self.img_cover_game != NULL)
	{
		TSU_BannerDestroy(&self.img_cover_game);
		TSU_TextureDestroy(&self.texture_cover_game);
	}

	if (self.img_cover_game_background != NULL)
	{
		TSU_BannerDestroy(&self.img_cover_game_background);
		TSU_TextureDestroy(&self.texture_cover_game_background);
	}

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_AppSubRemoveItemMenu(self.dsapp_ptr, self.item_button[i]);
			TSU_ItemMenuDestroy(&self.item_button[i]);
		}
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.img_cover_game_rectangle);
	}	

	TSU_FontDestroy(&self.menu_font);
	TSU_FontDestroy(&self.message_font);
	TSU_AppDestroy(&self.dsapp_ptr);
	self.scene_ptr = NULL;
}

static bool RunGame()
{
	bool is_running = false;

	if (self.item_value_selected[0] != '\0')
	{
		ds_printf("DS_GAMES: Run: %s", self.item_value_selected);

		if (LoadPreset() == 1)
		{
			ds_printf("LoadPresset: %s", "OK");
			SaveMenuConfig();
			FreeAppData();

			isoldr_exec(self.isoldr, self.addr);
			is_running = true;
		}
	}

	return is_running;
}

static void PostLoadPVRCover(bool new_cover)
{
	if (new_cover)
	{
		for (int i = 0; i < self.game_count; i++)
		{
			if (menu_data.finished_menu) break;
			
			if (self.item_game[i] != NULL)
			{
				// ONLY PVR AT THIS TIME
				if (menu_data.games_array[TSU_ItemMenuGetItemIndex(self.item_game[i])].is_pvr_cover)
				{
					if (menu_data.finished_menu) break;

					LoadCover(self.item_game[i], i);
				}
			}
		}
	}

	HideCoverScan();	
}

static void DoMenuControlHandler(void *ds_event, void *param, int action)
{
	SDL_Event *event = (SDL_Event *) param;

	if (menu_data.load_pvr_cover_thread != NULL)
	{
		bool skip_return = self.modal_cover_scan == NULL;
		menu_data.stop_load_pvr_cover = true;		
		thd_join(menu_data.load_pvr_cover_thread, NULL);
		menu_data.load_pvr_cover_thread = NULL;

		if (!skip_return)
			return;
	}

	timer_ms_gettime(&self.scan_covers_start_time, NULL);
	
	switch(event->type) {
		case SDL_JOYBUTTONDOWN: {
			switch(event->jbutton.button) {
				case 1: // B
					GamesApp_InputEvent(EvtKeypress, KeyCancel);
					break;

				case 2: // A				
					GamesApp_InputEvent(EvtKeypress, KeySelect);
					break;

				case 5: // Y
					break;

				case 6: // X
					GamesApp_InputEvent(EvtKeypress, KeyMiscX);
					break;

				case 3: // START
					break;

				default:
					break;
			}
		}
		break;

		case SDL_JOYAXISMOTION: {
			switch(event->jaxis.axis) {
				case 2: // RIGHT TRIGGER
					{
						static bool right_trigger_down = false;
						if (!right_trigger_down && (event->jaxis.value >= MAX_TRIGGER_VALUE/3)) {
							right_trigger_down = true;
							GamesApp_InputEvent(EvtKeypress, KeyPgdn);
						}
						else if (event->jaxis.value < MAX_TRIGGER_VALUE/3) {
							right_trigger_down = false;
						}
					}
					break;

				case 3: // LEFT TRIGGER
					{
						static bool left_trigger_down = false;
						if (!left_trigger_down && (event->jaxis.value >= MAX_TRIGGER_VALUE/3)) {
							left_trigger_down = true;
							GamesApp_InputEvent(EvtKeypress, KeyPgup);
						}
						else if (event->jaxis.value < MAX_TRIGGER_VALUE/3) {
							left_trigger_down = false;
						}
					}					
					break;
			}
		}

		case SDL_JOYHATMOTION: {
			switch(event->jhat.value) {
				case 0x0E: // KEY UP
					GamesApp_InputEvent(EvtKeypress, KeyUp);
					break;

				case 0x0B: // KEY DOWN
					GamesApp_InputEvent(EvtKeypress, KeyDown);
					break;

				case 0x07: // KEY LEFT
					GamesApp_InputEvent(EvtKeypress, KeyLeft);
					break;

				case 0x0D: // KEY RIGHT
					GamesApp_InputEvent(EvtKeypress, KeyRight);
					break;

				default:
					break;
			}
		}
		break;

		default:
			break;
	}
}

static void DoMenuVideoHandler(void *ds_event, void *param, int action)
{
	if (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE)
	{
		if (self.scan_count < MAX_SCAN_COUNT
			&& menu_data.cover_scanned_app.last_game_index < menu_data.games_array_count
			&& menu_data.load_pvr_cover_thread == NULL)
		{
			timer_ms_gettime(&self.scan_covers_end_time, NULL);
			if ((self.scan_covers_end_time - self.scan_covers_start_time) >= 5)
			{
				self.scan_covers_start_time = self.scan_covers_end_time;
				menu_data.stop_load_pvr_cover = false;
				menu_data.load_pvr_cover_thread = thd_create(0, LoadPVRCoverThread, NULL);
				ShowCoverScan();
			}
		}
	}

	switch(action)
	{
		case EVENT_ACTION_RENDER:
			{
				TSU_AppDoFrame(self.dsapp_ptr);
			}
			break;
		case EVENT_ACTION_RENDER_POST:
			break;
		case EVENT_ACTION_UPDATE:
			break;
		default:
			break;
	}
}

static void* MenuExitHelper(void *params)
{
	if (self.dsapp_ptr != NULL)
	{
		RemoveEvent(do_menu_end_video_event);
		if (!self.exit_app)
		{
			if (RunGame())
			{
				SDL_DC_EmulateMouse(SDL_TRUE);
				EnableScreen();
				GUI_Enable();
				ShutdownDS();
			}
			else
			{
				FreeAppData();
				SDL_DC_EmulateMouse(SDL_TRUE);
				EnableScreen();
				GUI_Enable();

				GamesApp_Exit(NULL);
			}
		}
		else
		{
			FreeAppData();
			SDL_DC_EmulateMouse(SDL_TRUE);
			EnableScreen();
			GUI_Enable();

			GamesApp_Exit(NULL);
		}
	}

	return NULL;
}

void GamesApp_Init(App_t *app)
{
	srand(time(NULL));
	mutex_init((mutex_t *)&change_page_mutex, MUTEX_TYPE_NORMAL);

	memset(&self, 0, sizeof(self));

	if (app->args != NULL)
	{
		self.have_args = true;
	}
	else
	{
		self.have_args = false;
	}
	
	self.app = app;
	self.app->thd = NULL;
	self.sector_size = 2048;
	self.pages = -1;
	self.first_menu_load = true;

	memset(self.item_value_selected, 0, sizeof(self.item_value_selected));

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		self.item_button[i] = NULL;
	}
	
	self.run_animation = NULL;
	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		self.item_game[i] = NULL;
		self.item_game_animation[i] = NULL;
		self.exit_animation_list[i] = NULL;
		self.exit_trigger_list[i] = NULL;
	}

	if ((self.dsapp_ptr = TSU_AppCreate(GamesApp_InputEvent)) != NULL)
	{
		Color main_color = {1, 0.22f, 0.06f, 0.25f};
		self.main_box = TSU_BoxCreate(PVR_LIST_OP_POLY, 3, 480 - 12, 640 - 6, 480 - 9, 12, &main_color, ML_BACKGROUND, 0);
		
		Color title_color = {1, 0.22f, 0.06f, 0.25f};
		self.title_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 0, 40, 640, 40, &title_color, ML_BACKGROUND, 0);

		Color title_type_color = {1, 1.0f, 1.0f, 0.1f};
		self.title_type_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 584, 37, 60, 34, &title_type_color, ML_ITEM + 2, 3, &title_color, 0);

		TSU_AppSubAddBox(self.dsapp_ptr, self.main_box);
		TSU_AppSubAddRectangle(self.dsapp_ptr, self.title_rectangle);
		TSU_AppSubAddRectangle(self.dsapp_ptr, self.title_type_rectangle);

		CreateMenuData(&SetMessageScan, &PostLoadPVRCover);
		InitMenu();
	}
}

void GamesApp_Open(App_t *app)
{
	(void)app;

	if (self.dsapp_ptr != NULL)
	{
		int mx = 0;
		int my = 0;
		SDL_GetMouseState(&mx, &my);
		SDL_DC_EmulateMouse(SDL_FALSE);
		SDL_DC_EmulateKeyboard(SDL_FALSE);

		DisableScreen();
		GUI_Disable();

		TSU_AppBegin(self.dsapp_ptr);

		do_menu_control_event = AddEvent
		(
			"GamesControl_Event",
			EVENT_TYPE_INPUT,
			EVENT_PRIO_DEFAULT,
			DoMenuControlHandler,
			NULL
		);

		do_menu_video_event = AddEvent
		(
			"GamesVideo_Event",
			EVENT_TYPE_VIDEO,
			EVENT_PRIO_DEFAULT,
			DoMenuVideoHandler,
			NULL
		);
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
	App_t *app = GetAppByName("Main");
	OpenApp(app, NULL);
}