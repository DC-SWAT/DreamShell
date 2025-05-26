/* DreamShell ##version##

   module.c - Games app module
   Copyright (C) 2024-2025 Maniac Vera

*/

#include <ds.h>
#include "app_menu.h"
#include "app_system_menu.h"
#include "app_preset.h"
#include "app_utils.h"
#include "app_module.h"
#include "app_definition.h"
#include <kos/md5.h>
#include <isoldr.h>
#include <dc/video.h>
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
#include <tsunami/anims/fadeto.h>
#include <tsunami/triggers/chainanim.h>
#include <tsunami/triggers/death.h>
#include <tsunami/drawables/triangle.h>
#include <tsunami/drawables/box.h>
#include <tsunami/drawables/optiongroup.h>
#include <tsunami/drawables/form.h>
#include <tsunami/dsapp.h>

DEFAULT_MODULE_EXPORTS(app_games_menu);

extern struct MenuStructure menu_data;
static Event_t *do_menu_control_event;
static Event_t *do_menu_video_event;
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
	
	bool exit_app, wait_to_exit_app;
	bool first_menu_load;
	volatile bool game_changed;
	volatile bool show_cover_game;
	uint32 scan_covers_start_time;
	uint32 scan_covers_end_time;
	int pages;
	int current_page;
	int previous_page;
	int game_count;
	int menu_cursel;
	int scan_count;
	uint8 device_selected;

	int game_index_selected;
	char item_value_selected[NAME_MAX];

	LogXYMover *item_selector_animation;
	Rectangle *item_selector;
	Font *menu_font;
	Font *message_font;
	Death *exit_trigger_list[MAX_SIZE_ITEMS];
	LogXYMover *run_animation;
	ExpXYMover *exit_animation_list[MAX_SIZE_ITEMS];
	LogXYMover *item_game_animation[MAX_SIZE_ITEMS];
	ItemMenu *item_game[MAX_SIZE_ITEMS];
	ItemMenu *item_button[MAX_BUTTONS];
	Banner *img_cover_game_background;
	Banner *img_cover_game;
	FadeTo *animation_cover_game;
	FadeOut *fadeout_cover_animation;
	Texture *texture_cover_game_background;
	Texture *texture_cover_game;
	Label *title;
	Label *title_type;
	Label *page_label;
	Label *total_label;
	LogXYMover *title_animation;
	LogXYMover *title_type_animation;
	Box *main_box;
	Rectangle *area_rectangle;
	Rectangle *title_rectangle;
	Rectangle *title_background_rectangle;
	Rectangle *title_type_rectangle;
	Rectangle *menu_options_rectangle;
	Rectangle *game_list_rectangle;
	Rectangle *img_cover_game_rectangle;
	kthread_t *show_cover_thread;
} self;

static void* ShowCoverThread(void *params)
{
	int game_index = (int)params;
	if (game_index >= 0)
	{
		srand(time(NULL));
		uint64_t start_time = 0;
		uint64_t end_time = 0;
		if (self.img_cover_game != NULL)
		{
			self.show_cover_game = false;
			if (self.animation_cover_game != NULL)
			{
				TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
				thd_pass();
				TSU_FadeToDestroy(&self.animation_cover_game);
			}
			
			if (self.fadeout_cover_animation == NULL)
			{			
				self.fadeout_cover_animation = TSU_FadeOutCreate(10.0f);
				TSU_DrawableAnimAdd((Drawable *)self.img_cover_game, (Animation *)self.fadeout_cover_animation);
			}

			const uint32_t max_time = 200;
			end_time = start_time = timer_ms_gettime64();
			while (!self.game_changed && menu_data.menu_type == MT_PLANE_TEXT && (end_time - start_time) <= max_time)
			{
				thd_pass();
				end_time = timer_ms_gettime64();
			}

			if (self.game_changed || menu_data.menu_type != MT_PLANE_TEXT)
				return NULL;

			TSU_AnimationComplete((Animation *)self.fadeout_cover_animation, (Drawable *)self.img_cover_game);
			thd_pass();
			TSU_FadeOutDestroy(&self.fadeout_cover_animation);

			TSU_DrawableSetFinished((Drawable *)self.img_cover_game);
			TSU_AppSubRemoveBanner(self.dsapp_ptr, self.img_cover_game);

			thd_pass();
			TSU_BannerDestroy(&self.img_cover_game);
			TSU_TextureDestroy(&self.texture_cover_game);
		}

		const uint32_t max_time = 400;		
		if (start_time == 0)
			end_time = start_time = timer_ms_gettime64();

		while (!self.game_changed && menu_data.menu_type == MT_PLANE_TEXT && (end_time - start_time) <= max_time)
		{
			thd_pass();
			end_time = timer_ms_gettime64();
		}

		if (self.game_changed || menu_data.menu_type != MT_PLANE_TEXT)
			return NULL;

		uint16 checked_cover = CheckCover(game_index, MT_PLANE_TEXT);
		if (checked_cover == SC_EXISTS || checked_cover == SC_DEFAULT)
		{
			self.show_cover_game = true;

			Vector vector_position = {490, 180, ML_ITEM + 2, 1};
			char *game_cover_path = NULL;
			uint16 cover_type = GetCoverType(game_index, MT_PLANE_TEXT);

			if (checked_cover == SC_EXISTS)
			{
				GetGameCoverPath(game_index, &game_cover_path, MT_PLANE_TEXT);
			}
			else
			{
				game_cover_path = (char *)malloc(NAME_MAX);
				snprintf(game_cover_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/no_cover.png");
				cover_type = IT_PNG;
			}
			
			self.texture_cover_game = TSU_TextureCreateFromFile(game_cover_path, cover_type != IT_JPG, false, 0);
			self.img_cover_game = TSU_BannerCreate(cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY, self.texture_cover_game);
			free(game_cover_path);
			
			TSU_DrawableTranslate((Drawable *)self.img_cover_game, &vector_position);
			TSU_BannerSetSize(self.img_cover_game, 256, 256);

			vector_position.x = 0.0f;
			vector_position.y = 0.0f;			
			TSU_DrawableSetScale((Drawable *)self.img_cover_game, &vector_position);
			
			TSU_AppSubAddBanner(self.dsapp_ptr, self.img_cover_game);
			self.animation_cover_game = TSU_FadeToCreate(0.0f, 1.0f, 7.5f);
			TSU_DrawableAnimAdd((Drawable *)self.img_cover_game, (Animation *)self.animation_cover_game);
			
			self.show_cover_game = false;
		}
	}

	return NULL;
}

static bool StopShowCover()
{
	if (self.show_cover_thread != NULL)
	{
		thd_join(self.show_cover_thread, NULL);
		self.show_cover_thread = NULL;
	}

	return (self.show_cover_thread == NULL);
}

static void ShowCover(int game_index)
{
	if (menu_data.menu_type == MT_PLANE_TEXT && game_index >= 0)
	{
		if (StopShowCover())
		{
			self.show_cover_thread = thd_create(0, ShowCoverThread, (void *)game_index);
			thd_set_prio(self.show_cover_thread, PRIO_DEFAULT - 2);
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
				ShowCover(game_index);
				loaded_cover = true;
			}
		}
		else
		{
			char *game_cover_path = NULL;
			if (GetGameCoverPath(game_index, &game_cover_path, menu_data.menu_type))
			{	
				TSU_ItemMenuSetImage(item_menu, game_cover_path, ContainsCoverType(game_index, menu_data.menu_type, IT_JPG) ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY);				
				loaded_cover = true;
				free(game_cover_path);
			}
		}
	}

	return loaded_cover;
}

static void SetTitle(int game_index, const char *text, bool limit_length)
{
	char titleText[151];
	memset(titleText, 0, sizeof(titleText));

	if (menu_data.ide && menu_data.sd && game_index >= 0)
	{
		switch (menu_data.games_array[game_index].device)
		{
			case APP_DEVICE_IDE:
				strncpy(titleText, "IDE: ", sizeof("IDE: "));
				break;

			case APP_DEVICE_SD:
				strncpy(titleText, "SD: ", sizeof("SD: "));
				break;
		}
	}

	if (strlen(text) > sizeof(titleText) - 6)
	{
		strncpy(&titleText[strlen(titleText)], text, sizeof(titleText) - 6);
	}
	else
	{
		strcpy(&titleText[strlen(titleText)], text);
	}

	if (limit_length)
	{
		titleText[100] = '\0';
	}

	static Vector vectorInit = {-50, 36, ML_ITEM, 1};
	static Vector vector = {16, 36, ML_ITEM, 1};
	vectorInit.z = ML_ITEM + 1;
	vector.z = ML_ITEM + 1;
	
	if (self.title != NULL)
	{
		TSU_LabelSetText(self.title, titleText);
	}
	else
	{
		int font_size = 20;
		Color color = { 1.0f, 1.0f, 1.0f, 0.35f };

		self.title = TSU_LabelCreate(self.menu_font, titleText, font_size, false, true);
		TSU_LabelSetTint(self.title, &color);
		TSU_AppSubAddLabel(self.dsapp_ptr, self.title);
	}

	if (self.title_animation != NULL)
	{
		TSU_AnimationComplete((Animation *)self.title_animation, (Drawable *)self.title);
		TSU_LogXYMoverDestroy(&self.title_animation);
	}

	TSU_LabelSetTranslate(self.title, &vectorInit);
	self.title_animation = TSU_LogXYMoverCreate(vector.x, vector.y);
	TSU_DrawableAnimAdd((Drawable *)self.title, (Animation *)self.title_animation);
}

static void SetTitleType(const char *full_path_game, bool is_gdi_optimized)
{
	if (full_path_game != NULL)
	{
		Color title_type_color = { 1.0f, 1.0f, 1.0f, 0.35f };
		char *file_type = strrchr(full_path_game, '.');

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
			title_type_color.g = 0.66f;
			title_type_color.b = 0.01f;
		}

		TSU_DrawableSetTint((Drawable *)self.title_type_rectangle, &title_type_color);
		TSU_DrawableSetBorderColor(self.title_type_rectangle, &title_type_color);
		TSU_DrawableSetBorderColor(self.title_rectangle, &title_type_color);

		static Vector vectorInit = {700, 36, ML_ITEM + 6, 1};
		static Vector vector = {577, 36, ML_ITEM + 6, 1};

		vectorInit.z = ML_ITEM + 6;

		if (self.title_type != NULL)
		{
			TSU_LabelSetText(self.title_type, title_text);
		}
		else
		{
			int font_size = 20;
			
			static Color color = {1, 1.0f, 1.0f, 1.0f};
			self.title_type = TSU_LabelCreate(self.menu_font, title_text, font_size, false, true);
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
			Color color = { 0.30f, 0.10f, 0.10f, 0.10f };
			Color border_color = {1, 1.0f, 1.0f, 0.1f};

			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 3, &border_color, DEFAULT_RADIUS);
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}
		else if (menu_data.menu_type == MT_IMAGE_128_4X3)
		{
			Color color = {0, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 5, &border_color, DEFAULT_RADIUS);			
			TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
		}
		else if (menu_data.menu_type == MT_PLANE_TEXT)
		{
			Color color = { 0.50f, 0.10f, 0.10f, 0.10f };
			Color border_color = {1, 1.0f, 1.0f, 0.1f};
			
			self.item_selector = TSU_RectangleCreateWithBorder(PVR_LIST_TR_POLY, 0, 0, 0, 0, &color, ML_CURSOR, 2, &border_color, DEFAULT_RADIUS);			
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
		TSU_RectangleSetSize(self.item_selector, 300, menu_data.menu_option.image_size + 8);
		init_position_x = 13;
		init_position_y = 40;
		selector_translate.z = ML_SELECTED - 1;
	}
	else if (menu_data.menu_type == MT_IMAGE_128_4X3)
	{
		TSU_RectangleSetSize(self.item_selector, menu_data.menu_option.image_size, menu_data.menu_option.image_size);
		init_position_x = 18;
		init_position_y = 47;
		selector_translate.z = ML_CURSOR;
	}
	else if (menu_data.menu_type == MT_PLANE_TEXT)
	{
		TSU_RectangleSetSize(self.item_selector, 329, 25);
		init_position_x = 18;
		init_position_y = 54;
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

static void RemoveViewTextPlane(bool remove_image_banner)
{
	self.show_cover_game = false;

	if (self.img_cover_game_background != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.img_cover_game_background);
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.img_cover_game_rectangle);
	}	

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_DrawableSetFinished((Drawable *)self.item_button[i]);
		}
	}

	if (self.img_cover_game != NULL && remove_image_banner)
	{
		if (self.animation_cover_game != NULL)
		{
			TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
		}

		if (self.fadeout_cover_animation != NULL)
		{
			TSU_AnimationComplete((Animation *)self.fadeout_cover_animation, (Drawable *)self.img_cover_game);
		}		

		TSU_DrawableSetFinished((Drawable *)self.img_cover_game);
	}
	
	if (self.menu_options_rectangle != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.menu_options_rectangle);
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.game_list_rectangle);
	}

	if (self.page_label != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.page_label);		
	}

	if (self.total_label != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.total_label);		
	}

	TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
	thd_pass();

	if (self.img_cover_game_background != NULL)
	{
		TSU_BannerDestroy(&self.img_cover_game_background);
		TSU_TextureDestroy(&self.texture_cover_game_background);
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.img_cover_game_rectangle);
	}	

	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_ItemMenuDestroy(&self.item_button[i]);
		}
	}

	if (self.img_cover_game != NULL && remove_image_banner)
	{
		if (self.animation_cover_game != NULL)
		{
			TSU_FadeToDestroy(&self.animation_cover_game);
		}

		if (self.fadeout_cover_animation != NULL)
		{
			TSU_FadeOutDestroy(&self.fadeout_cover_animation);
		}		

		TSU_BannerDestroy(&self.img_cover_game);
		TSU_TextureDestroy(&self.texture_cover_game);
	}

	if (self.menu_options_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.menu_options_rectangle);
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.game_list_rectangle);
	}

	if (self.page_label != NULL)
	{
		TSU_LabelDestroy(&self.page_label);
	}

	if (self.total_label != NULL)
	{
		TSU_LabelDestroy(&self.total_label);
	}
}

static void RefreshTotal()
{
	if (menu_data.menu_type == MT_PLANE_TEXT)
	{
		char label_text[20] = {0};
		snprintf(label_text, sizeof(label_text), "PAGE: %d/%d", self.current_page, self.pages);
		TSU_LabelSetText(self.page_label, label_text);

		memset(label_text, 0, sizeof(label_text)); 
		snprintf(label_text, sizeof(label_text), "GAMES: %d", menu_data.games_array_ptr_count);
		TSU_LabelSetText(self.total_label, label_text);
	}
}

static void CreateInfoButton(uint8 button_index, const char *button_file, const char *text, float x, float y)
{
	if(self.item_button[button_index] == NULL)
	{
		char *image_path = (char *)malloc(NAME_MAX);
		snprintf(image_path, NAME_MAX, "%s/%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images", button_file);

		if (FileExists(image_path))
		{
			Color color = {1, 1.0f, 1.0f, 1.0f};
			Vector vector_translate = { x, y, ML_ITEM + 3, 1};
			self.item_button[button_index] = TSU_ItemMenuCreate(image_path, 32, 32, PVR_LIST_TR_POLY, text, self.menu_font, 15, false, 0);
			TSU_ItemMenuSetTranslate(self.item_button[button_index], &vector_translate);
			TSU_LabelSetTint(TSU_ItemMenuGetLabel(self.item_button[button_index]), &color);

			const Vector *text_position = TSU_DrawableGetTranslate((Drawable *)TSU_ItemMenuGetLabel(self.item_button[button_index]));
			Vector new_position = { text_position->x, text_position->y, text_position->z, text_position->w };
			new_position.x -= 5;
			TSU_LabelSetTranslate(TSU_ItemMenuGetLabel(self.item_button[button_index]), &new_position);
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
	CreateInfoButton(0, "btn_x.png", "CHANGE VIEW", 357, 315);
	CreateInfoButton(1, "btn_y.png", "SETTINGS", 503, 315);
	CreateInfoButton(2, "btn_a.png", "PLAY", 357, 315 + (30 * 1) + (5 * 1));
	CreateInfoButton(3, "btn_b.png", "EXIT", 503, 315 + (30 * 1) + (5 * 1));
	CreateInfoButton(4, "btn_lt.png", "PREVIOUS", 357, 315 + (30 * 2) + (5 * 2));
	CreateInfoButton(5, "btn_rt.png", "NEXT", 503, 315 + (30 * 2) + (5 * 2));
	CreateInfoButton(6, "btn_start.png", "SYSTEM MENU", 357, 315 + (30 * 3) + (5 * 3));
	AddInfoButtons();

	if (self.menu_options_rectangle == NULL)
	{
		self.menu_options_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 361, 457, 254, 142, &menu_data.body_color, ML_ITEM, 3, &menu_data.border_color, DEFAULT_RADIUS);

		TSU_AppSubAddRectangle(self.dsapp_ptr, self.menu_options_rectangle);		
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.game_list_rectangle);
		TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
		thd_pass();
		
		TSU_RectangleDestroy(&self.game_list_rectangle);
	}

	if (self.page_label != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.page_label);
		TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
		thd_pass();
		
		TSU_LabelDestroy(&self.page_label);
	}

	if (self.total_label != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.total_label);
		TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
		thd_pass();
		
		TSU_LabelDestroy(&self.total_label);
	}
	
	self.game_list_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 15, 436, 335, 384, &menu_data.body_color, ML_ITEM, 3, &menu_data.border_color, DEFAULT_RADIUS);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.game_list_rectangle);

	const int font_size = 18;
	Color font_color = { 1.0f, 1.0f, 1.0f, 0.35f };	
	self.page_label = TSU_LabelCreate(self.menu_font, "", font_size, false, true);
	TSU_LabelSetTint(self.page_label, &font_color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.page_label);

	Vector page_vector = { 18, 459, ML_ITEM + 1, 1 };
	TSU_LabelSetTranslate(self.page_label, &page_vector);

	self.total_label = TSU_LabelCreate(self.menu_font, "", font_size, false, true);
	TSU_LabelSetTint(self.total_label, &font_color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.total_label);

	Vector total_vector = { 36 + 335 / 2, 459, ML_ITEM + 1, 1 };
	TSU_LabelSetTranslate(self.total_label, &total_vector);

	if (self.img_cover_game_background == NULL && menu_data.cover_background)
	{
		char *game_cover_path = (char *)malloc(NAME_MAX);
		snprintf(game_cover_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/cover.png");

		if (FileExists(game_cover_path))
		{
			self.texture_cover_game_background = TSU_TextureCreateFromFile(game_cover_path, true, false, 0);
			self.img_cover_game_background = TSU_BannerCreate(PVR_LIST_TR_POLY, self.texture_cover_game_background);

			Vector vector_position = {490, 180, ML_ITEM + 3, 1};
			TSU_DrawableTranslate((Drawable *)self.img_cover_game_background, &vector_position);
			TSU_BannerSetSize(self.img_cover_game_background, 256, 256);
			TSU_AppSubAddBanner(self.dsapp_ptr, self.img_cover_game_background);

			Color background_color = {1, 0.88f, 0.88f, 0.88f};
			self.img_cover_game_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, vector_position.x - 130, vector_position.y + 128, 256, 256, &background_color, ML_ITEM + 1, 0);
			TSU_AppSubAddRectangle(self.dsapp_ptr, self.img_cover_game_rectangle);
		}

		free(game_cover_path);
	}
}

static void CalculatePages()
{
	self.pages = (menu_data.games_array_ptr_count > 0 ? ceil((float)menu_data.games_array_ptr_count / (float)menu_data.menu_option.max_page_size) : 0);
}

static bool LoadPage(bool change_view, uint8 direction)
{
	const int LENGTH_NAME_TEXT_PLANE = 27;
	const int LENGTH_NAME_TEXT_64_5X2 = 18;
	bool loaded = false;
	char name_truncated[LENGTH_NAME_TEXT_PLANE];
	char game_cover_path[NAME_MAX];
	char name[NAME_MAX];
	char *game_cover_path_tmp = NULL;

	if (self.pages == -1)
	{
		if (RetrieveGames())
		{
			menu_data.games_array_ptr_count = menu_data.games_array_count;
			menu_data.games_array_ptr = (GameItemStruct **)&menu_data.games_array;
			timer_ms_gettime(&self.scan_covers_start_time, NULL);
			CalculatePages();
		}
		
		for (int imenu = 1; imenu <= MAX_MENU; imenu++)
		{
			RetrieveCovers(menu_data.current_dev, imenu);
		}
	}
	else
	{
		if (change_view)
		{
			CalculatePages();
		}		
	}

	if (self.pages > 0)
	{
		if (menu_data.last_game_played_index >= 0)
		{
			self.current_page = ceil((float)(menu_data.last_game_played_index + 1) / (float)menu_data.menu_option.max_page_size);
		}
		
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
			for (int i = 0; i < MAX_SIZE_ITEMS; i++)
			{
				if (self.item_game_animation[i] != NULL)
				{
					TSU_AnimationComplete((Animation *)self.item_game_animation[i], (Drawable *)self.item_game[i]);
				}

				if (self.item_game[i] != NULL)
				{
					TSU_DrawableSetFinished((Drawable *)self.item_game[i]);
				}
			}

			TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
			thd_pass();

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
			}

			if (change_view || self.first_menu_load)
			{				
				RemoveViewTextPlane(true);
				if (menu_data.menu_type == MT_PLANE_TEXT)
				{					
					CreateViewTextPlane();
				}
				else
				{
					self.game_list_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 10, 480 - 18, 640 - 28, 415, &menu_data.body_color, ML_BACKGROUND + 1, DEFAULT_RADIUS);
					TSU_AppSubAddRectangle(self.dsapp_ptr, self.game_list_rectangle);
				}
			}

			RefreshTotal();
			int cover_menu_type = 0;
			int column = 0;
			int page = 0;
			int game_index = 0;
			GameItemStruct *game_ptr = NULL;
			ItemMenu *item_selected = NULL;
			self.game_count = 0;
			Vector vectorTranslate = {0, 480, ML_ITEM, 1};

			for (int icount = ((self.current_page - 1) * menu_data.menu_option.max_page_size); icount < menu_data.games_array_ptr_count; icount++)
			{
				page = ceil((float)(icount + 1) / (float)menu_data.menu_option.max_page_size);

				if (page < self.current_page)
					continue;
				else if (page > self.current_page)
					break;

				self.game_count++;
				game_ptr = GetGamePtrByIndex(icount);
				game_index = game_ptr->game_index_tmp >= 0 ? game_ptr->game_index_tmp : icount;

				column = floor((float)(self.game_count - 1) / (float)menu_data.menu_option.size_items_column);				
				
				if (menu_data.menu_type != MT_PLANE_TEXT)
				{
					game_cover_path_tmp = NULL;
					if (CheckCover(game_index, menu_data.menu_type) == SC_EXISTS)
					{
						cover_menu_type = menu_data.menu_type;
					}
					else
					{
						CheckCover(game_index, MT_PLANE_TEXT);
						cover_menu_type = MT_PLANE_TEXT;
					}

					if (GetGameCoverPath(game_index, &game_cover_path_tmp, cover_menu_type))
					{
						strcpy(game_cover_path, game_cover_path_tmp);
						free(game_cover_path_tmp);
					}
				}

				memset(name, 0, sizeof(name));
				if (game_ptr->is_folder_name)
				{
					strcpy(name, game_ptr->folder_name);
				}
				else
				{
					strncpy(name, game_ptr->game, strlen(game_ptr->game) - 4);
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
								, ContainsCoverType(game_index, cover_menu_type, IT_JPG) ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY								
								, name_truncated, self.menu_font, 19, false, PVR_TXRLOAD_DMA);

					Color color_unselected = { 1, 1.0f, 1.0f, 1.0f };
					TSU_ItemMenuSetColorUnselected(self.item_game[self.game_count - 1], &color_unselected);
					
					Color color_text = { 1.0f, 1.0f, 1.0f, 0.35f };
					TSU_ItemMenuSetTextColor(self.item_game[self.game_count - 1], &color_text);
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
					Color color_unselected = { 1, 1.0f, 1.0f, 1.0f };
					TSU_ItemMenuSetColorUnselected(self.item_game[self.game_count - 1], &color_unselected);

					Color color_text = { 1.0f, 1.0f, 1.0f, 0.35f };
					TSU_ItemMenuSetTextColor(self.item_game[self.game_count - 1], &color_text);
				}
				else
				{				
					self.item_game[self.game_count - 1] = TSU_ItemMenuCreateImage(game_cover_path
							, menu_data.menu_option.image_size
							, menu_data.menu_option.image_size
							, ContainsCoverType(game_index, cover_menu_type, IT_JPG) ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY							
							, false
							, PVR_TXRLOAD_DMA);
				}

				ItemMenu *item_menu = self.item_game[self.game_count - 1];

				TSU_ItemMenuSetItemIndex(item_menu, game_index);
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

				if (menu_data.menu_type != MT_PLANE_TEXT)
				{
					if (menu_data.menu_type == MT_IMAGE_TEXT_64_5X2)
					{
						TSU_LogXYMoverSetFactor(self.item_game_animation[self.game_count - 1], 6.5f);
					}
					else
					{
						TSU_LogXYMoverSetFactor(self.item_game_animation[self.game_count - 1], 6.0f);
					}
				}

				TSU_ItemMenuAnimAdd(item_menu, (Animation *)self.item_game_animation[self.game_count - 1]);

				if (self.first_menu_load)
				{
					if ((self.game_count == 1 && menu_data.last_game_played_index == -1) 
						|| menu_data.last_game_played_index == icount)
					{
						item_selected = self.item_game[self.game_count - 1];
						menu_data.last_game_played_index = -1;
						self.menu_cursel = self.game_count - 1;
					}
					else
					{
						TSU_ItemMenuSetSelected(item_menu, false, false);
					}
				}
				
				TSU_AppSubAddItemMenu(self.dsapp_ptr, item_menu);
			}

			if (self.first_menu_load)
			{
				if (self.game_count > 0)
				{
					if (item_selected == NULL)
					{
						self.menu_cursel = 0;
						item_selected = self.item_game[0];
					}
				}
				
				if (item_selected)
				{
					int game_index = TSU_ItemMenuGetItemIndex(item_selected);
					memset(name, 0, sizeof(name));

					if (menu_data.games_array[game_index].is_folder_name)
					{
						strcpy(name, menu_data.games_array[game_index].folder_name);
					}
					else
					{
						strncpy(name, menu_data.games_array[game_index].game, strlen(menu_data.games_array[game_index].game) - 4);
					}

					TSU_ItemMenuSetSelected(item_selected, true, true);
					SetTitle(game_index, name, true);
					SetTitleType(GetFullGamePathByIndex(game_index)
						, CheckGdiOptimized(game_index));

					SetCursor();
					ShowCover(game_index);
					PlayCDDA(game_index);
				}
			}

			menu_data.last_game_played_index = -1;
			loaded = true;
			self.first_menu_load = false;
		}
	}
	else
	{	
		static bool draw_message = true;
		if (draw_message)
		{
			draw_message = false;
			memset(game_cover_path, 0, sizeof(game_cover_path));

			char font_path[NAME_MAX];
			memset(font_path, 0, sizeof(font_path));
			snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/message.txf");

			TSU_LabelDestroy(&self.title);
			TSU_FontDestroy(&self.menu_font);
			self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
			
			snprintf(game_cover_path, sizeof(game_cover_path), "\n\nYou need put the games here:\n%s\n\nand images here:\n%s\n\nor change games_path in\napps/games_menu/menu_games.cfg", GetGamesPath(menu_data.current_dev), GetCoversPath(menu_data.current_dev));			
			SetTitle(-1, game_cover_path, false);
		}
	}

	return loaded;
}

static void ReloadPage()
{
	if (menu_data.games_array_ptr_count > 0)
	{
		CalculatePages();
		self.first_menu_load = true;
		int game_index = TSU_ItemMenuGetItemIndex(self.item_game[self.menu_cursel]);
		menu_data.last_game_played_index = -1;
		
		if (menu_data.category[0] != '\0')
		{
			menu_data.last_game_played_index = 0;
			GameItemStruct *game_ptr = NULL;
			for (int icount = 0; icount < menu_data.games_array_ptr_count; icount++)
			{
				game_ptr = GetGamePtrByIndex(icount);

				if (game_ptr->game_index_tmp == game_index)
				{
					menu_data.last_game_played_index = icount;
					break;
				}
			}
		}
		else
		{
			menu_data.last_game_played_index = game_index;
		}

		LoadPage(false, DMD_NONE);
	}
}

static void InitMenu()
{
	menu_data.state_app = SA_GAMES_MENU;
	self.scene_ptr = TSU_AppGetScene(self.dsapp_ptr);
	
	char font_path[NAME_MAX];
	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");
	self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/message.txf");
	self.message_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	SetMenuType(menu_data.menu_type);
	self.exit_app = false;
	self.title = NULL;
	self.title_type = NULL;
	self.game_index_selected = -1;

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
}

static void RemoveAll()
{
	TSU_AnimationComplete((Animation *)self.title_animation, (Drawable *)self.title);
	TSU_AnimationComplete((Animation *)self.title_type_animation, (Drawable *)self.title_type);
	TSU_AnimationComplete((Animation *)self.item_selector_animation, (Drawable *)self.item_selector);

	if (self.animation_cover_game != NULL && self.img_cover_game != NULL)
	{
		TSU_AnimationComplete((Animation *)self.animation_cover_game, (Drawable *)self.img_cover_game);
	}


	TSU_DrawableSetFinished((Drawable *)self.main_box);
	TSU_DrawableSetFinished((Drawable *)self.area_rectangle);	
	TSU_DrawableSetFinished((Drawable *)self.title_rectangle);
	TSU_DrawableSetFinished((Drawable *)self.title_background_rectangle);	
	TSU_DrawableSetFinished((Drawable *)self.title_type_rectangle);	
	TSU_DrawableSetFinished((Drawable *)self.menu_options_rectangle);
	TSU_DrawableSetFinished((Drawable *)self.game_list_rectangle);
	TSU_DrawableSetFinished((Drawable *)self.page_label);
	TSU_DrawableSetFinished((Drawable *)self.total_label);
	TSU_DrawableSetFinished((Drawable *)self.item_selector);
	TSU_DrawableSetFinished((Drawable *)self.title);
	TSU_DrawableSetFinished((Drawable *)self.title_type);

	if (self.img_cover_game != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.img_cover_game);
	}	
	
	if (self.img_cover_game_background != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.img_cover_game_background);
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_DrawableSetFinished((Drawable *)self.img_cover_game_rectangle);
	}
	
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		if (self.item_button[i] != NULL)
		{
			TSU_DrawableSetFinished((Drawable *)self.item_button[i]);
		}
	}

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game_animation[i] != NULL)
		{
			TSU_AnimationComplete((Animation *)self.item_game_animation[i], (Drawable *)self.item_game[i]);
		}
	}

	SystemMenuRemoveAll();
	TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
	thd_pass();
}

static void StartExit()
{
	StopCDDA();
	StopShowCover();
	RemoveAll();

	float y = 1.0f;
	self.game_index_selected = -1;

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game[i] != NULL)
		{
			if (!self.exit_app && TSU_ItemMenuIsSelected(self.item_game[i]))
			{
				self.game_index_selected = TSU_ItemMenuGetItemIndex(self.item_game[i]);
				strcpy(self.item_value_selected, GetFullGamePathByIndex(self.game_index_selected));
				self.device_selected = menu_data.games_array[TSU_ItemMenuGetItemIndex(self.item_game[i])].device;

				if (menu_data.menu_type == MT_PLANE_TEXT)
				{
					float w, h;
					char *label_text = (char *)malloc(NAME_MAX);
					memset(label_text, 0, NAME_MAX);

					strcpy(label_text, TSU_ItemMenuGetLabelText(self.item_game[i]));
					label_text = Trim(label_text);

					TSU_FontSetSize(self.menu_font, 18);
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

				self.exit_trigger_list[i] = TSU_DeathCreate(NULL);
				TSU_TriggerAdd((Animation *)self.run_animation, (Trigger *)self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.item_game[i], (Animation *)self.run_animation);
			}
			else
			{				
				self.exit_animation_list[i] = TSU_ExpXYMoverCreate(0, y + .10, 0, 500);
				self.exit_trigger_list[i] = TSU_DeathCreate(NULL);

				TSU_TriggerAdd((Animation *)self.exit_animation_list[i], (Trigger *)self.exit_trigger_list[i]);
				TSU_DrawableAnimAdd((Drawable *)self.item_game[i], (Animation *)self.exit_animation_list[i]);
			}
		}
	}
	TSU_AppStartExit(self.dsapp_ptr);

	self.wait_to_exit_app = true;
	while (!menu_data.finished_menu) { thd_pass(); }

	RemoveEvent(do_menu_video_event);
	RemoveEvent(do_menu_control_event);
	
	MenuExitHelper(NULL);
}

static void GamesApp_SystemMenuInputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;

	mutex_lock(&change_page_mutex);

	switch (key)
	{
		case KeyStart:
		{	
			if (StateSystemMenu())
			{
				menu_data.state_app = SA_GAMES_MENU;
				ExitSystemMenuClick(NULL);
			}
		}
		break;

		default:
		{			
			if (StateSystemMenu())
			{
				SystemMenuInputEvent(type, key);
			}
		}
		break;
	}

	mutex_unlock(&change_page_mutex);
}

static void GamesApp_PresetMenuInputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;

	mutex_lock(&change_page_mutex);

	switch (key)
	{
		case KeyStart:
		case KeyMiscY:
		{	
			if (StatePresetMenu())
			{
				HidePresetMenu();
				menu_data.state_app = SA_GAMES_MENU;
			}
		}
		break;

		default:
		{			
			if (StatePresetMenu())
			{
				PresetMenuInputEvent(type, key);
			}
		}
		break;
	}

	mutex_unlock(&change_page_mutex);
}

static void GamesApp_ScanCoverInputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;
	
	mutex_lock(&change_page_mutex);

	StopScanCovers();
	HideCoverScan();
	menu_data.state_app = SA_GAMES_MENU;
	timer_ms_gettime(&self.scan_covers_start_time, NULL);

	mutex_unlock(&change_page_mutex);
}

static void GamesApp_OptimizeCoverInputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;
	
	mutex_lock(&change_page_mutex);

	StopOptimizeCovers();
	HideOptimizeCoverPopup();
	menu_data.state_app = SA_GAMES_MENU;
	timer_ms_gettime(&self.scan_covers_start_time, NULL);
	
	mutex_unlock(&change_page_mutex);
}

static void GamesApp_ControlInputEvent(int type, int key, int state_app)
{
	if (type != EvtKeypress)
		return;

	switch (state_app)
	{
		case SA_CONTROL + ASYNC_CONTROL_ID: //ASYNC
		{
			AsyncInputEvent(type, key);
			break;
		}

		case SA_CONTROL + OS_CONTROL_ID: //OS
		{
			OSInputEvent(type, key);
			break;
		}

		case SA_CONTROL + LOADER_CONTROL_ID: //LOADER
		{
			LoaderInputEvent(type, key);
			break;
		}

		case SA_CONTROL + BOOT_CONTROL_ID: //BOOT
		{
			BootInputEvent(type, key);
			break;
		}
		
		case SA_CONTROL + MEMORY_CONTROL_ID: //MEMORY
		{
			MemoryInputEvent(type, key);
			break;
		}

		case SA_CONTROL + CUSTOM_MEMORY_CONTROL_ID: //CUSTOM MEMORY
		{
			CustomMemoryInputEvent(type, key);
			break;
		}

		case SA_CONTROL + HEAP_CONTROL_ID: //HEAP
		{
			HeapInputEvent(type, key);
			break;
		}	

		case SA_CONTROL + CDDA_SOURCE_CONTROL_ID: //CDDA SOURCE
		{
			CDDASourceInputEvent(type, key);
			break;
		}	

		case SA_CONTROL + CDDA_DESTINATION_CONTROL_ID: //CDDA DESTINATION
		{
			CDDADestinationInputEvent(type, key);
			break;
		}	

		case SA_CONTROL + CDDA_POSITION_CONTROL_ID: //CDDA POSITION
		{
			CDDAPositionInputEvent(type, key);
			break;
		}	

		case SA_CONTROL + CDDA_CHANNEL_CONTROL_ID: //CDDA CHANNEL
		{
			CDDAChannelInputEvent(type, key);
			break;
		}

		case SA_CONTROL + PATCH_ADDRESS1_CONTROL_ID: //PATCH ADDRESS 1
		{
			PatchAddress1InputEvent(type, key);
			break;
		}

		case SA_CONTROL + PATCH_VALUE1_CONTROL_ID: //PATCH VALUE 1
		{
			PatchValue1InputEvent(type, key);
			break;
		}

		case SA_CONTROL + PATCH_ADDRESS2_CONTROL_ID: //PATCH ADDRESS 2
		{
			PatchAddress2InputEvent(type, key);
			break;
		}

		case SA_CONTROL + PATCH_VALUE2_CONTROL_ID: //PATCH VALUE 2
		{
			PatchValue2InputEvent(type, key);
			break;
		}

		case SA_CONTROL + ALTERBOOT_CONTROL_ID: //ALTER BOOT
		{
			AlterBootInputEvent(type, key);
			break;
		}

		case SA_CONTROL + SCREENSHOT_CONTROL_ID: //SCREENSHOT
		{
			ScreenshotInputEvent(type, key);
			break;
		}

		case SA_CONTROL + VMU_CONTROL_ID: //VMU
		{
			VMUInputEvent(type, key);
			break;
		}

		case SA_CONTROL + VMUSELECTOR_CONTROL_ID: //VMU OPTION
		{
			VMUSelectorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + SHORTCUT_SIZE_CONTROL_ID: //SHORTCUT SIZE
		{
			ShortcutSizeInputEvent(type, key);
			break;
		}

		case SA_CONTROL + SHORTCUT_ROTATE_CONTROL_ID: //SHORTCUT ROTATE
		{
			ShortcutRotateInputEvent(type, key);
			break;
		}

		case SA_CONTROL + SHORTCUT_NAME_CONTROL_ID: //SHORTCUT NAME
		{
			ShortcutNameInputEvent(type, key);
			break;
		}

		case SA_CONTROL + SHORTCUT_DONTSHOWNAME_CONTROL_ID: //SHORTCUT DONT SHOW NAME
		{
			ShortcutDontShowNameInputEvent(type, key);
			break;
		}

		case SA_CONTROL + CATEGORY_CONTROL_ID: //SYSTE MENU CATEGORY
		{
			CategoryInputEvent(type, key);
			break;			
		}

		case SA_CONTROL + THEME_CONTROL_ID: //SYSTEM MENU THEME
		{
			ThemeInputEvent(type, key);
			break;
		}

		case SA_CONTROL + BACKGROUND_COLOR_CONTROL_ID: //SYSTEM MENU BACKGROUND COLOR
		{
			BackgroundColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + BORDER_COLOR_CONTROL_ID: //SYSTEM MENU CATEGORY BORDER COLOR
		{
			BorderColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + TITLE_COLOR_CONTROL_ID: //SYSTEM MENU TITLE BACKGROUND COLOR
		{
			TitleColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + BODY_COLOR_CONTROL_ID: //SYSTEM MENU BODY COLOR
		{
			BodyColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + AREA_COLOR_CONTROL_ID: //SYSTEM MENU AREA COLOR
		{
			AreaColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + CONTROL_TOP_COLOR_CONTROL_ID: //SYSTEM MENU TOP CONTROL COLOR
		{
			ControlTopColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + CONTROL_BODY_COLOR_CONTROL_ID: //SYSTEM MENU BODY CONTROL COLOR
		{
			ControlBodyColorInputEvent(type, key);
			break;
		}

		case SA_CONTROL + CONTROL_BOTTOM_COLOR_CONTROL_ID: //SYSTEM MENU BOTTOM CONTROL COLOR
		{
			ControlBottomColorInputEvent(type, key);
			break;
		}
		
		default:
			break;
	}
}

static void GamesApp_InputEvent(int type, int key)
{
	if (type != EvtKeypress)
		return;
	
	mutex_lock(&change_page_mutex);

	bool skip_cursor = false;
	float speed_cursor = 7.0f;
	timer_ms_gettime(&self.scan_covers_start_time, NULL);

	if (menu_data.menu_type == MT_PLANE_TEXT)
	{
		speed_cursor = 6.5;
	}

	switch (key)
	{
		case KeyStart:
		{	
			if (menu_data.games_array_count > 0)
			{
				menu_data.state_app = SA_SYSTEM_MENU;
				ShowSystemMenu();
			}
			skip_cursor = true;			
		}
		break;

		case KeyMiscY:
		{
			if (menu_data.games_array_count > 0)
			{
				menu_data.state_app = SA_PRESET_MENU;
				ShowPresetMenu(TSU_ItemMenuGetItemIndex(self.item_game[self.menu_cursel]));
			}
			skip_cursor = true;
		}
		break;

		// X: CHANGE VISUALIZATION
		case KeyMiscX:
		{
			StopShowCover();

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

					if (menu_data.change_page_with_pad)
					{
						self.current_page--;
						self.menu_cursel = 0;
						if (LoadPage(false, DMD_RIGHT))
						{
						}
					}
				}
				else
				{
					self.menu_cursel -= menu_data.menu_option.size_items_column;

					if (self.menu_cursel < 0)
					{
						if (menu_data.change_page_with_pad)
						{
							self.current_page--;
							self.menu_cursel = 0;
							if (LoadPage(false, DMD_RIGHT))
							{
							}
						}
						else
						{
							self.menu_cursel += self.game_count;
						}
					}
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
					
					if (menu_data.change_page_with_pad)
					{
						self.current_page++;
						self.menu_cursel = 0;
						if (LoadPage(false, DMD_LEFT))
						{
						}
					}
				}
				else
				{
					self.menu_cursel += menu_data.menu_option.size_items_column;

					if (self.menu_cursel >= self.game_count)
					{
						if (menu_data.change_page_with_pad)
						{
							self.current_page++;
							self.menu_cursel = 0;
							if (LoadPage(false, DMD_LEFT))
							{
							}
						}
						else
						{
							self.menu_cursel -= self.game_count;
						}
					}
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
		self.game_changed = true;		
		
		if (key != KeyMiscX)
			StopCDDA();			
		
		for (int i = 0; i < self.game_count; i++)
		{
			if (self.item_game[i] != NULL)
			{
				if (i == self.menu_cursel)
				{
					TSU_ItemMenuSetSelected(self.item_game[i], true, true);
					SetTitle(TSU_ItemMenuGetItemIndex(self.item_game[i]), TSU_ItemMenuGetItemValue(self.item_game[i]), true);

					SetTitleType(GetFullGamePathByIndex(TSU_ItemMenuGetItemIndex(self.item_game[i]))
						, CheckGdiOptimized(TSU_ItemMenuGetItemIndex(self.item_game[i])));

					SetCursor();

					TSU_LogXYMoverSetFactor(self.item_selector_animation, speed_cursor);

					ShowCover(TSU_ItemMenuGetItemIndex(self.item_game[i]));

					if (key != KeyMiscX)
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

static int LoadPreset()
{
	if (menu_data.preset == NULL || menu_data.preset->game_index != self.game_index_selected)
	{
		if (menu_data.preset != NULL)
		{
			free(menu_data.preset);
		}

		menu_data.preset = LoadPresetGame(self.game_index_selected);
	}

	if ((self.isoldr = ParsePresetToIsoldr(self.game_index_selected, menu_data.preset)) == NULL)
	{
		return 0;
	}

	char memory[12];
	memset(memory, 0, sizeof(memory));

	if (strcasecmp(menu_data.preset->memory, "0x8c") == 0)
	{
		snprintf(memory, sizeof(memory), "%s%s", menu_data.preset->memory, menu_data.preset->custom_memory);
	}
	else
	{
		strcpy(memory, menu_data.preset->memory);
	}

	self.addr = strtoul(memory, NULL, 16);

	if (menu_data.preset->emu_vmu)
	{
		GenerateVMUFile(self.item_value_selected, menu_data.preset->vmu_mode, menu_data.preset->emu_vmu);
	}

	if (strncmp(self.isoldr->fs_dev, "auto", 4) == 0)
	{
		if (self.device_selected == APP_DEVICE_SD)
		{
			strcpy(self.isoldr->fs_dev, "sd");
		}
		else
		{
			if (!strncasecmp(GetDefaultDir(menu_data.current_dev), "/cd", 3))
			{
				strcpy(self.isoldr->fs_dev, "cd");
			}
			else if (!strncasecmp(GetDefaultDir(menu_data.current_dev), "/sd", 3))
			{
				strcpy(self.isoldr->fs_dev, "sd");
			}
			else if (!strncasecmp(GetDefaultDir(menu_data.current_dev), "/ide", 4))
			{
				strcpy(self.isoldr->fs_dev, "ide");
			}
		}
	}

	return 1;
}

static void FreeAppData()
{
	mutex_destroy(&change_page_mutex);	

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game[i] != NULL)
		{
			if (!self.exit_app && TSU_ItemMenuIsSelected(self.item_game[i]) && self.run_animation != NULL)
			{
				TSU_AnimationComplete((Animation *)self.run_animation, (Drawable *)self.item_game[i]);			
			}
			else if (self.exit_animation_list[i] != NULL)
			{
				TSU_AnimationComplete((Animation *)self.exit_animation_list[i], (Drawable *)self.item_game[i]);
			}

			TSU_DrawableSetFinished((Drawable *)self.item_game[i]);
		}
	}	

	TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
	thd_pass();

	for (int i = 0; i < MAX_SIZE_ITEMS; i++)
	{
		if (self.item_game[i] != NULL)
		{
			if (TSU_ItemMenuIsSelected(self.item_game[i]) && self.run_animation != NULL)
			{
				TSU_LogXYMoverDestroy(&self.run_animation);
			}
			else if (self.exit_animation_list[i] != NULL)
			{
				TSU_ExpXYMoverDestroy(&self.exit_animation_list[i]);
			}

			if (self.item_game_animation[i] != NULL)
			{
				TSU_LogXYMoverDestroy(&self.item_game_animation[i]);
			}

			if (self.exit_trigger_list[i] != NULL)
			{
				TSU_DeathDestroy(&self.exit_trigger_list[i]);
			}

			if (self.item_game[i] != NULL)
			{
				TSU_ItemMenuDestroy(&self.item_game[i]);
			}
		}
	}

	if (self.main_box != NULL)
	{
		TSU_BoxDestroy(&self.main_box);
	}

	if (self.area_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.area_rectangle);
	}

	if (self.title_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.title_rectangle);
	}

	if (self.title_background_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.title_background_rectangle);
	}

	if (self.title_type_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.title_type_rectangle);
	}

	if (self.menu_options_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.menu_options_rectangle);
	}

	if (self.game_list_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.game_list_rectangle);
	}

	if (self.page_label != NULL)
	{
		TSU_LabelDestroy(&self.page_label);
	}

	if (self.total_label != NULL)
	{
		TSU_LabelDestroy(&self.total_label);		
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
		TSU_LogXYMoverDestroy(&self.title_animation);
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
		TSU_FadeToDestroy(&self.animation_cover_game);		
	}

	if (self.fadeout_cover_animation != NULL)
	{
		TSU_FadeOutDestroy(&self.fadeout_cover_animation);		
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
			TSU_ItemMenuDestroy(&self.item_button[i]);
		}
	}

	if (self.img_cover_game_rectangle != NULL)
	{
		TSU_RectangleDestroy(&self.img_cover_game_rectangle);
	}

	DestroySystemMenu();
	DestroyPresetMenu();

	TSU_FontDestroy(&self.menu_font);
	TSU_FontDestroy(&self.message_font);
	TSU_AppDestroy(&self.dsapp_ptr);
	self.scene_ptr = NULL;

	DestroyMenuData();
}

static bool PlayGame()
{
	bool is_running = false;

	if (self.item_value_selected[0] != '\0')
	{
		menu_data.last_device = self.device_selected;
		if (menu_data.games_array[self.game_index_selected].is_folder_name)
		{
			strcpy(menu_data.last_game, menu_data.games_array[self.game_index_selected].folder_name);
		}
		else
		{
			strcpy(menu_data.last_game, menu_data.games_array[self.game_index_selected].game);
		}

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

static void PostOptimizer()
{
	HideOptimizeCoverPopup();
	menu_data.state_app = SA_GAMES_MENU;
	menu_data.optimize_game_cover_thread = NULL;
}

static void PostLoadPVRCover(bool new_cover)
{
	if (new_cover)
	{
		int game_index = 0;
		for (int i = 0; i < self.game_count; i++)
		{
			if (menu_data.finished_menu) break;
			
			if (self.item_game[i] != NULL)
			{
				game_index = TSU_ItemMenuGetItemIndex(self.item_game[i]);

				// ONLY PVR AT THIS TIME
				if (menu_data.games_array[game_index].is_pvr_cover)
				{
					if (menu_data.finished_menu) break;

					thd_pass();
					LoadCover(self.item_game[i], i);
				}
			}
		}
	}

	HideCoverScan();
	menu_data.state_app = SA_GAMES_MENU;
	menu_data.load_pvr_cover_thread  = NULL;
}

static void StateAppInpuEvent(int state_app, int type, int key)
{
	switch (state_app)
	{
		case SA_GAMES_MENU:
			GamesApp_InputEvent(type, key);
			break;
		
		case SA_SYSTEM_MENU:
			GamesApp_SystemMenuInputEvent(type, key);
			break;

		case SA_PRESET_MENU:
			GamesApp_PresetMenuInputEvent(type, key);
			break;

		case SA_SCAN_COVER:
			GamesApp_ScanCoverInputEvent(type, key);
			break;

		case SA_OPTIMIZE_COVER:
			GamesApp_OptimizeCoverInputEvent(type, key);
			break;

		default:
			if (state_app >= SA_CONTROL) {				
				GamesApp_ControlInputEvent(type, key, state_app);
			}
			break;
	}
}

static void DoMenuControlHandler(void *ds_event, void *param, int action)
{
	if (self.show_cover_game) thd_pass();

	SDL_Event *event = (SDL_Event *) param;
	
	switch(event->type) {
		case SDL_JOYBUTTONDOWN: {
			switch(event->jbutton.button) {
				case 1: // B
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyCancel);
					break;

				case 2: // A				
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeySelect);
					break;

				case 5: // Y
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyMiscY);
					break;

				case 6: // X
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyMiscX);
					break;

				case 4: // Z
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyPgup);
					break;

				case 0: // C
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyPgdn);
					break;

				case 3: // START
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyStart);			
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
							StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyPgdn);
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
							StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyPgup);
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
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyUp);
					break;

				case 0x0B: // KEY DOWN
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyDown);
					break;

				case 0x07: // KEY LEFT
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyLeft);
					break;

				case 0x0D: // KEY RIGHT
					StateAppInpuEvent(menu_data.state_app, EvtKeypress, KeyRight);
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
	if (!self.wait_to_exit_app)
		if (self.show_cover_game) thd_pass();

	switch(action)
	{
		case EVENT_ACTION_RENDER:
			{
				if (!self.wait_to_exit_app)
				{
					TSU_AppDoFrame(self.dsapp_ptr);
				}
				else
				{
					if (!menu_data.finished_menu && TSU_AppEnd(self.dsapp_ptr))
					{
						menu_data.finished_menu = true;
					}
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

static void* MenuExitHelper(void *params)
{
	if (self.dsapp_ptr != NULL)
	{
		if (!self.exit_app)
		{
			if (PlayGame())
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

static void CreateMainView()
{
	if (self.main_box != NULL)
		TSU_DrawableSetFinished((Drawable *)self.main_box);

	if (self.area_rectangle != NULL)
		TSU_DrawableSetFinished((Drawable *)self.area_rectangle);

	if (self.title_rectangle != NULL)
		TSU_DrawableSetFinished((Drawable *)self.title_rectangle);

	if (self.title_background_rectangle != NULL)
		TSU_DrawableSetFinished((Drawable *)self.title_background_rectangle);

	if (self.title_type_rectangle != NULL)
		TSU_DrawableSetFinished((Drawable *)self.title_type_rectangle);	

	TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
	thd_pass();

	if (self.main_box != NULL)
		TSU_BoxDestroy(&self.main_box);

	if (self.area_rectangle != NULL)
		TSU_RectangleDestroy(&self.area_rectangle);

	if (self.title_rectangle != NULL)
		TSU_RectangleDestroy(&self.title_rectangle);

	if (self.title_background_rectangle != NULL)
		TSU_RectangleDestroy(&self.title_background_rectangle);

	if (self.title_type_rectangle != NULL)
		TSU_RectangleDestroy(&self.title_type_rectangle);

	self.main_box = TSU_BoxCreate(PVR_LIST_OP_POLY, 10, 480 - 18, 640 - 28, 480 - 30, 5, &menu_data.background_color, ML_BACKGROUND + 2, DEFAULT_RADIUS);
	Color title_type_color = {1, 1.0f, 1.0f, 0.1f};

	self.area_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 10, 480 - 18, 640 - 28, 415, &menu_data.area_color, ML_BACKGROUND + 1, DEFAULT_RADIUS);
	self.title_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 12, 40, 640 - 33, 24, &menu_data.title_color, ML_BACKGROUND + 3, 3, &title_type_color, DEFAULT_RADIUS);
	self.title_background_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 5, 54, 640 - 18, 46, &menu_data.background_color, ML_BACKGROUND, DEFAULT_RADIUS);
	self.title_type_rectangle = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 570, 39, 46, 20, &title_type_color, ML_ITEM + 2, 3, &title_type_color, 0);

	TSU_AppSubAddBox(self.dsapp_ptr, self.main_box);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.area_rectangle);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.title_rectangle);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.title_background_rectangle);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.title_type_rectangle);
}

static void RefreshMainView()
{
	CreateMainView();
	
	if (menu_data.menu_type == MT_PLANE_TEXT)
	{
		RemoveViewTextPlane(false);
		CreateViewTextPlane();
		RefreshTotal();
	}
	else
	{
		if (self.game_list_rectangle != NULL)
			TSU_DrawableSetFinished((Drawable *)self.game_list_rectangle);

		TSU_DrawableSubRemoveFinished((Drawable *)self.scene_ptr);
		thd_pass();

		if (self.game_list_rectangle != NULL)
			TSU_RectangleDestroy(&self.game_list_rectangle);
		
		self.game_list_rectangle = TSU_RectangleCreate(PVR_LIST_OP_POLY, 10, 480 - 18, 640 - 28, 415, &menu_data.body_color, ML_BACKGROUND + 1, DEFAULT_RADIUS);
		TSU_AppSubAddRectangle(self.dsapp_ptr, self.game_list_rectangle);
	}
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
		CreateMenuData(&SetMessageScan, &SetMessageOptimizer, &PostLoadPVRCover, &PostOptimizer);
		
		CreateMainView();
		
		InitMenu();

		CreateSystemMenu(self.dsapp_ptr, self.scene_ptr, self.menu_font, self.message_font, &RefreshMainView, &ReloadPage);
		CreatePresetMenu(self.dsapp_ptr, self.scene_ptr, self.menu_font, self.message_font);
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