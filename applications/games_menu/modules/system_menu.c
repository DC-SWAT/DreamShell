/* DreamShell ##version##

   system_menu.c - main menu
   Copyright (C) 2024-2025 Maniac Vera

*/

#include "app_system_menu.h"
#include "app_definition.h"
#include "app_menu.h"
#include "app_utils.h"
#include <tsunami/dsapp.h>
#include <tsunami/font.h>
#include <tsunami/drawables/scene.h>
#include <tsunami/drawables/form.h>
#include <tsunami/drawables/rectangle.h>

extern struct MenuStructure menu_data;

enum ViewEnum
{
	SYSTEM_MENU_VIEW = 0,
	STYLE_VIEW,
	CACHE_VIEW
};

static struct
{
	bool first_scan_cover;
	bool general_changes_flag;
	bool style_changes_flag;
	bool cache_changes_flag;
	ThemeStruct theme_selected;
	Color control_body_color;
	char current_category[32];
	void (*refresh_main_view)();
	void (*reload_page)();

	DSApp *dsapp_ptr;
	Scene *scene_ptr;
	Form *system_menu_form;
	Form *optimize_cover_popup;	

	Label *optimize_covers_label;
	Label *title_cover_scan;
	Label *note_cover_scan;
	Label *message_cover_scan;
	Rectangle *modal_cover_scan;

	CheckBox *save_preset_option;
	CheckBox *cover_background_option;
	CheckBox *change_page_with_page_option;
	CheckBox *start_in_last_game_option;
	OptionGroup *category_option;
	
	OptionGroup *theme_option;
	TextBox *background_color_option;
	TextBox *border_color_option;
	TextBox *title_color_option;	
	TextBox *body_color_option;
	TextBox *area_color_option;
	TextBox *control_top_color_option;
	TextBox *control_body_color_option;
	TextBox *control_bottom_color_option;

	CheckBox *cache_game_option;
	Label *cache_rebuild_option;

	Font *message_font;
	Font *menu_font;
	Font *textbox_font;
} self;

void CreateSystemMenu(DSApp *dsapp_ptr, Scene *scene_ptr, Font *menu_font, Font *message_font, void (*RefreshMainView)(), void (*ReloadPage)())
{
	self.dsapp_ptr = dsapp_ptr;
	self.scene_ptr = scene_ptr;
	self.message_font = message_font;
	self.menu_font = menu_font;
	self.system_menu_form = NULL;
	self.optimize_cover_popup = NULL;
	self.optimize_covers_label = NULL;
	self.first_scan_cover = false;
	self.general_changes_flag = false;
	self.style_changes_flag = false;
	self.cache_changes_flag = false;
	self.refresh_main_view = RefreshMainView;
	self.reload_page = ReloadPage;

	char font_path[NAME_MAX];
	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

	self.textbox_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
}

void DestroySystemMenu()
{
	HideSystemMenu();
	HideOptimizeCoverPopup();
	HideCoverScan();

	self.dsapp_ptr = NULL;
	self.scene_ptr = NULL;
	self.message_font = NULL;
	self.menu_font = NULL;
	TSU_FontDestroy(&self.textbox_font);
}

void SystemMenuRemoveAll()
{
}

void SystemMenuInputEvent(int type, int key)
{
	TSU_FormInputEvent(self.system_menu_form, type, key);
}

int StateSystemMenu()
{
	return (self.system_menu_form != NULL ? 1 : 0);
}

void OnSystemViewIndexChangedEvent(Drawable *drawable, int view_index)
{
	switch (view_index)
	{
		case SYSTEM_MENU_VIEW:
			CreateSystemMenuView((Form *)drawable);
			break;

		case STYLE_VIEW:
			CreateStyleView((Form *)drawable);
			break;

		case CACHE_VIEW:
			CreateCacheView((Form *)drawable);
			break;
	}
}

void ShowSystemMenu()
{
	HideSystemMenu();

	if (self.system_menu_form == NULL)
	{
		memset(self.current_category, 0, sizeof(self.current_category));
		strcpy(self.current_category, menu_data.category);

		CreateCategories();
		self.control_body_color = menu_data.control_body_color;
		self.general_changes_flag = false;
		self.style_changes_flag = false;
		self.cache_changes_flag = false;

		char font_path[NAME_MAX];
		memset(font_path, 0, sizeof(font_path));
		snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

		Font *form_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
		Color body_color = { 1, 0, 0, 0};
		uint width = 380;
		uint height = 440;
		self.system_menu_form = TSU_FormCreate(640 / 2 - width / 2, 480 / 2 + (height - 10) / 2, width, height, true, 3, DEFAULT_RADIUS
		, true, true, form_font, &menu_data.border_color, &menu_data.control_top_color, &body_color, &menu_data.control_bottom_color,
		&OnSystemViewIndexChangedEvent);

		{
			Label *general_label = TSU_LabelCreate(form_font, "GENERAL", 14, false, false);
			TSU_FormAddBottomLabel(self.system_menu_form, general_label);
			general_label = NULL;
		}

		{
			Label *style_label = TSU_LabelCreate(form_font, "STYLE", 14, false, false);
			TSU_FormAddBottomLabel(self.system_menu_form, style_label);
			style_label = NULL;
		}

		{
			Label *cache_label = TSU_LabelCreate(form_font, "CACHE", 14, false, false);
			TSU_FormAddBottomLabel(self.system_menu_form, cache_label);
			cache_label = NULL;
		}

		TSU_DrawableSubAdd((Drawable *)self.scene_ptr, (Drawable *)self.system_menu_form);
		TSU_FormSelectedEvent(self.system_menu_form, &SystemMenuSelectedEvent);
	}
}

void CreateSystemMenuView(Form *form_ptr)
{
	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetAttributes(form_ptr, 2, 8, 200, 30);
	TSU_FormSetRowSize(form_ptr, 3, 40);
	TSU_FormSetRowSize(form_ptr, 4, 40);
	TSU_FormSetRowSize(form_ptr, 5, 40);
	TSU_FormSetRowSize(form_ptr, 6, 40);
	TSU_FormSetRowSize(form_ptr, 7, 40);
	TSU_FormSetRowSize(form_ptr, 8, 40);
	TSU_FormSetColumnSize(form_ptr, 1, 250);
	TSU_FormSetColumnSize(form_ptr, 2, 250);
	TSU_FormSetTitle(form_ptr, "SYSTEM MENU");

	int font_size = 17;

	// THE ALIGN SHOULD BE IN FORM CLASS
	{
		// SCAN MISSING COVERS
		Label *missing_covers_label = TSU_LabelCreate(form_font, "Scan missing covers", font_size, false, false);
		TSU_FormAddBodyLabel(form_ptr, missing_covers_label, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)missing_covers_label, &ScanMissingCoversClick);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)missing_covers_label);
		option_vector.x -= 167;
		TSU_DrawableSetTranslate((Drawable *)missing_covers_label, &option_vector);
		TSU_FormSetCursor(form_ptr, (Drawable *)missing_covers_label);

		missing_covers_label = NULL;
	}

	{
		// OPTIMIZE COVERS
		Label *optimize_covers_label = TSU_LabelCreate(form_font, "Optimize covers", font_size, false, false);
		TSU_FormAddBodyLabel(form_ptr, optimize_covers_label, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)optimize_covers_label, &OptimizeCoversClick);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)optimize_covers_label);
		option_vector.x -= 144;
		TSU_DrawableSetTranslate((Drawable *)optimize_covers_label, &option_vector);

		optimize_covers_label = NULL;
	}		

	{
		// DEFAULT SAVE PRESET
		Label *save_preset_label = TSU_LabelCreate(form_font, "Default save preset:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)save_preset_label, true);
		TSU_FormAddBodyLabel(form_ptr, save_preset_label, 1, 3);

		self.save_preset_option = TSU_CheckBoxCreate(form_font, font_size, 50, 16, &self.control_body_color);
		TSU_FormAddBodyCheckBox(form_ptr, self.save_preset_option, 2, 3);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.save_preset_option);
		option_vector.y -= 5;
		TSU_DrawableSetTranslate((Drawable *)self.save_preset_option, &option_vector);

		TSU_DrawableEventSetClick((Drawable *)self.save_preset_option, &DefaultSavePresetOptionClick);

		if (menu_data.save_preset)
		{
			TSU_CheckBoxSetOn(self.save_preset_option);
		}

		save_preset_label = NULL;
	}

	{
		// COVER BACKGROUND
		Label *cover_background_label = TSU_LabelCreate(form_font, "Cover background:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cover_background_label, true);
		TSU_FormAddBodyLabel(form_ptr, cover_background_label, 1, 4);

		self.cover_background_option = TSU_CheckBoxCreate(form_font, font_size, 50, 16, &self.control_body_color);
		TSU_FormAddBodyCheckBox(form_ptr, self.cover_background_option, 2, 4);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.cover_background_option);
		option_vector.y -= 5;
		TSU_DrawableSetTranslate((Drawable *)self.cover_background_option, &option_vector);

		TSU_DrawableEventSetClick((Drawable *)self.cover_background_option, &CoverBackgroundOptionClick);

		if (menu_data.cover_background)
		{
			TSU_CheckBoxSetOn(self.cover_background_option);
		}

		cover_background_label = NULL;
	}

	{
		// CHANGE PAGE WITH PAD
		Label *change_page_with_page_label = TSU_LabelCreate(form_font, "Change page with pad:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)change_page_with_page_label, true);
		TSU_FormAddBodyLabel(form_ptr, change_page_with_page_label, 1, 5);

		self.change_page_with_page_option = TSU_CheckBoxCreate(form_font, font_size, 50, 16, &self.control_body_color);
		TSU_FormAddBodyCheckBox(form_ptr, self.change_page_with_page_option, 2, 5);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.change_page_with_page_option);
		option_vector.y -= 5;
		TSU_DrawableSetTranslate((Drawable *)self.change_page_with_page_option, &option_vector);

		TSU_DrawableEventSetClick((Drawable *)self.change_page_with_page_option, &ChangePageWithPadOptionClick);

		if (menu_data.change_page_with_pad)
		{
			TSU_CheckBoxSetOn(self.change_page_with_page_option);
		}

		change_page_with_page_label = NULL;
	}

	{
		// START IN LAST GAME
		Label *start_in_last_game_label = TSU_LabelCreate(form_font, "Start in last game:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)start_in_last_game_label, true);
		TSU_FormAddBodyLabel(form_ptr, start_in_last_game_label, 1, 6);

		self.start_in_last_game_option = TSU_CheckBoxCreate(form_font, font_size, 50, 16, &self.control_body_color);
		TSU_FormAddBodyCheckBox(form_ptr, self.start_in_last_game_option, 2, 6);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.start_in_last_game_option);
		option_vector.y -= 5;
		TSU_DrawableSetTranslate((Drawable *)self.start_in_last_game_option, &option_vector);

		TSU_DrawableEventSetClick((Drawable *)self.start_in_last_game_option, &StartInLastGameOptionClick);

		if (menu_data.start_in_last_game)
		{
			TSU_CheckBoxSetOn(self.start_in_last_game_option);
		}

		start_in_last_game_label = NULL;
	}

	{
		// CATEGORY
		Label *category_label = TSU_LabelCreate(form_font, "Category folder:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)category_label, true);
		TSU_FormAddBodyLabel(form_ptr, category_label, 1, 7);

		self.category_option = TSU_OptionGroupCreate(form_font, (uint)font_size, 330, 16, &self.control_body_color);
		TSU_DrawableSetId((Drawable *)self.category_option, CATEGORY_CONTROL_ID);

		TSU_OptionGroupAdd(self.category_option, 0, "(NONE)");
		
		int count = 1;
		CategoryStruct *current_category, *tmp_category;
		HASH_ITER(hh, menu_data.categories_array, current_category, tmp_category)
		{
			TSU_OptionGroupAdd(self.category_option, ++count, current_category->category);			
		}

		TSU_OptionGroupSetStates(self.category_option, SA_CONTROL + CATEGORY_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.category_option, 2, 7);
		TSU_DrawableEventSetClick((Drawable *)self.category_option, &CategoryOptionClick);

		if (menu_data.category[0] != '\0' && menu_data.category[0] != ' ')
			TSU_OptionGroupSelectOptionByText(self.category_option, menu_data.category);
		else
			TSU_OptionGroupSelectOptionByText(self.category_option, "(NONE)");

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.category_option);
		option_vector.x -= 248;
		option_vector.y += 18;
		TSU_DrawableSetTranslate((Drawable *)self.category_option, &option_vector);

		category_label = NULL;
	}

	{
		// EXIT TO MAIN MENU
		Label *exit_covers_label = TSU_LabelCreate(form_font, "Return", font_size, false, false);
		TSU_FormAddBodyLabel(form_ptr, exit_covers_label, 1, 8);
		TSU_DrawableEventSetClick((Drawable *)exit_covers_label, &ExitSystemMenuClick);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)exit_covers_label);
		option_vector.y += 25;
		option_vector.x += 150;
		TSU_DrawableSetTranslate((Drawable *)exit_covers_label, &option_vector);

		exit_covers_label = NULL;
	}

	form_font = NULL;
}

void CreateStyleView(Form *form_ptr)
{
	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetAttributes(form_ptr, 2, 9, 160, 32);
	TSU_FormSetColumnSize(form_ptr, 1, 200);
	TSU_FormSetColumnSize(form_ptr, 2, 180);
	TSU_FormSetRowSize(form_ptr, 7, 38);
	TSU_FormSetRowSize(form_ptr, 8, 38);
	TSU_FormSetRowSize(form_ptr, 9, 38);
	TSU_FormSetTitle(form_ptr, "STYLE");

	int font_size = 16;
	int control_height = 16;

	{
		// THEME
		Label *theme_label = TSU_LabelCreate(form_font, "Theme:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)theme_label, true);
		TSU_FormAddBodyLabel(form_ptr, theme_label, 1, 1);

		self.theme_option = TSU_OptionGroupCreate(form_font, (uint)font_size, 130, control_height, &self.control_body_color);
		TSU_DrawableSetId((Drawable *)self.theme_option, THEME_CONTROL_ID);

		TSU_OptionGroupAdd(self.theme_option, 0, DEFAULT_THEME);
		TSU_OptionGroupAdd(self.theme_option, 1, PSYCHEDELIC_THEME);
		TSU_OptionGroupAdd(self.theme_option, 2, MINT_THEME);
		TSU_OptionGroupAdd(self.theme_option, 3, CUSTOM_THEME);
		TSU_OptionGroupSetStates(self.theme_option, SA_CONTROL + THEME_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.theme_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.theme_option, &ThemeOptionClick);

		theme_label = NULL;
	}

	{
		// BACKGROUND COLOR
		Label *label = TSU_LabelCreate(form_font, "Background color:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 2);

		self.background_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.background_color_option, BACKGROUND_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.background_color_option, SA_CONTROL + BACKGROUND_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.background_color_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.background_color_option, &BackgroundColorOptionClick);

		uint32 value = plx_pack_color(menu_data.background_color.a, menu_data.background_color.r, menu_data.background_color.g, menu_data.background_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.background_color_option, text);
		}

		label = NULL;
	}

	{
		// TITLE COLOR
		Label *label = TSU_LabelCreate(form_font, "Title color:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 3);

		self.title_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.title_color_option, TITLE_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.title_color_option , SA_CONTROL + TITLE_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.title_color_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.title_color_option, &TitleColorOptionClick);

		uint32 value = plx_pack_color(menu_data.title_color.a, menu_data.title_color.r, menu_data.title_color.g, menu_data.title_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.title_color_option, text);
		}

		label = NULL;
	}

	{
		// BORDER COLOR
		Label *label = TSU_LabelCreate(form_font, "Border color:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 4);

		self.border_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.border_color_option, BORDER_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.border_color_option , SA_CONTROL + BORDER_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.border_color_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.border_color_option , &BorderColorOptionClick);

		uint32 value = plx_pack_color(menu_data.border_color.a, menu_data.border_color.r, menu_data.border_color.g, menu_data.border_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.border_color_option, text);
		}

		label = NULL;
	}

	{
		// AREA COLOR
		Label *label = TSU_LabelCreate(form_font, "Area color:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 5);

		self.area_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.area_color_option, AREA_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.area_color_option , SA_CONTROL + AREA_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.area_color_option, 2, 5);
		TSU_DrawableEventSetClick((Drawable *)self.area_color_option , &AreaColorOptionClick);

		uint32 value = plx_pack_color(menu_data.area_color.a, menu_data.area_color.r, menu_data.area_color.g, menu_data.area_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.area_color_option, text);
		}

		label = NULL;
	}

	{
		// BODY COLOR
		Label *label = TSU_LabelCreate(form_font, "Body color:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 6);

		self.body_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.body_color_option, BODY_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.body_color_option , SA_CONTROL + BODY_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.body_color_option, 2, 6);
		TSU_DrawableEventSetClick((Drawable *)self.body_color_option , &BodyColorOptionClick);

		uint32 value = plx_pack_color(menu_data.body_color.a, menu_data.body_color.r, menu_data.body_color.g, menu_data.body_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.body_color_option, text);
		}

		label = NULL;
	}

	{
		// CONTROL TOP COLOR
		Label *label = TSU_LabelCreate(form_font, "Control Top\nColor:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 7);

		self.control_top_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.control_top_color_option, CONTROL_TOP_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.control_top_color_option , SA_CONTROL + CONTROL_TOP_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.control_top_color_option, 2, 7);
		TSU_DrawableEventSetClick((Drawable *)self.control_top_color_option, &ControlTopColorOptionClick);

		uint32 value = plx_pack_color(menu_data.control_top_color.a, menu_data.control_top_color.r, menu_data.control_top_color.g, menu_data.control_top_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.control_top_color_option, text);
		}

		label = NULL;
	}

	{
		// CONTROL BODY COLOR
		Label *label = TSU_LabelCreate(form_font, "Control Body\nColor:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 8);

		self.control_body_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.control_body_color_option, CONTROL_BODY_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.control_body_color_option, SA_CONTROL + CONTROL_BODY_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.control_body_color_option, 2, 8);
		TSU_DrawableEventSetClick((Drawable *)self.control_body_color_option, &ControlBodyColorOptionClick);

		uint32 value = plx_pack_color(menu_data.control_body_color.a, menu_data.control_body_color.r, menu_data.control_body_color.g, menu_data.control_body_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.control_body_color_option, text);
		}

		label = NULL;
	}

	{
		// CONTROL BOTTOM COLOR
		Label *label = TSU_LabelCreate(form_font, "Control Bottom\nColor:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 9);

		self.control_bottom_color_option = TSU_TextBoxCreate(self.textbox_font, (uint)font_size, false, 100, control_height, &self.control_body_color, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.control_bottom_color_option, CONTROL_BOTTOM_COLOR_CONTROL_ID);

		TSU_TextBoxSetStates(self.control_bottom_color_option, SA_CONTROL + CONTROL_BOTTOM_COLOR_CONTROL_ID, SA_SYSTEM_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.control_bottom_color_option, 2, 9);
		TSU_DrawableEventSetClick((Drawable *)self.control_bottom_color_option, &ControlBottomColorOptionClick);

		uint32 value = plx_pack_color(menu_data.control_bottom_color.a, menu_data.control_bottom_color.r, menu_data.control_bottom_color.g, menu_data.control_bottom_color.b);

		if (value > 0)
		{
			char text[10] = {0};
			snprintf(text, sizeof(text), "%08lx", value);
			TSU_TextBoxSetText(self.control_bottom_color_option, text);
		}

		label = NULL;
	}

	if (menu_data.theme[0] != '\0' && menu_data.theme[0] != ' ')
	{
		TSU_OptionGroupSelectOptionByText(self.theme_option, menu_data.theme);
		SetAllThemeProperties(menu_data.theme);
	}
	else
	{
		strcpy(menu_data.theme, DEFAULT_THEME);
		SetAllThemeProperties(DEFAULT_THEME);
	}

	form_font = NULL;
}

void CreateCacheView(Form *form_ptr)
{
	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetAttributes(form_ptr, 2, 2, 200, 42);
	TSU_FormSetColumnSize(form_ptr, 2, 250);
	TSU_FormSetTitle(form_ptr, "CACHE MENU");

	int font_size = 17;

	{
		// CACHE IN GAMES
		Label *label = TSU_LabelCreate(form_font, "Enable cache:", font_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)label, true);
		TSU_FormAddBodyLabel(form_ptr, label, 1, 1);

		self.cache_game_option = TSU_CheckBoxCreate(form_font, font_size, 50, 16, &self.control_body_color);
		TSU_FormAddBodyCheckBox(form_ptr, self.cache_game_option, 2, 1);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.cache_game_option);
		option_vector.y -= 5;
		TSU_DrawableSetTranslate((Drawable *)self.cache_game_option, &option_vector);

		TSU_DrawableEventSetClick((Drawable *)self.cache_game_option, &CacheGamesOptionClick);

		if (menu_data.enable_cache)
		{
			TSU_CheckBoxSetOn(self.cache_game_option);
		}

		label = NULL;
	}

	{
		// REBUILD CACHE
		self.cache_rebuild_option = TSU_LabelCreate(form_font, "Rebuild cache", font_size, false, false);
		TSU_FormAddBodyLabel(form_ptr, self.cache_rebuild_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.cache_rebuild_option, &RebuildCacheClick);

		Vector option_vector = TSU_DrawableGetPosition((Drawable *)self.cache_rebuild_option);
		option_vector.x -= 80;
		TSU_DrawableSetTranslate((Drawable *)self.cache_rebuild_option, &option_vector);
		TSU_FormSetCursor(form_ptr, (Drawable *)self.cache_rebuild_option);
	}

	TSU_DrawableSetReadOnly((Drawable *)self.cache_rebuild_option, !menu_data.enable_cache);

	form_font = NULL;
}

void HideSystemMenu()
{
	if (self.system_menu_form != NULL)
	{
		TSU_FormDestroy(&self.system_menu_form);
	}
}

void SystemMenuSelectedEvent(Drawable *drawable, uint bottom_index, uint column, uint row)
{
}

void ScanMissingCoversClick(Drawable *drawable)
{
	HideSystemMenu();

	if (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE)
	{
		if (menu_data.load_pvr_cover_thread == NULL && menu_data.optimize_game_cover_thread == NULL)
		{
			menu_data.stop_load_pvr_cover = false;

			if (!self.first_scan_cover)
			{
				menu_data.rescan_covers = true;
			}

			self.first_scan_cover = true;
			menu_data.load_pvr_cover_thread = thd_create(0, LoadPVRCoverThread, NULL);
			ShowCoverScan();			
		}
	}
	else
	{
		menu_data.state_app = SA_GAMES_MENU;
	}
}

void OptimizeCoversClick(Drawable *drawable)
{
	HideSystemMenu();

	if (menu_data.current_dev == APP_DEVICE_SD || menu_data.current_dev == APP_DEVICE_IDE)
	{
		if (menu_data.load_pvr_cover_thread == NULL && menu_data.optimize_game_cover_thread == NULL)
		{
			menu_data.stop_optimize_game_cover = false;
			menu_data.optimize_game_cover_thread = thd_create(0, OptimizeCoverThread, NULL);
			ShowOptimizeCoverPopup();
			menu_data.state_app = SA_OPTIMIZE_COVER;
		}
	}
	else
	{
		menu_data.state_app = SA_GAMES_MENU;
	}
}

void DefaultSavePresetOptionClick(Drawable *drawable)
{
	self.general_changes_flag = true;
	TSU_CheckBoxInputEvent(self.save_preset_option, 0, KeySelect);
	menu_data.save_preset = (TSU_CheckBoxGetValue(self.save_preset_option) ? 1 : 0);
}

void CoverBackgroundOptionClick(Drawable *drawable)
{
	self.general_changes_flag = true;
	TSU_CheckBoxInputEvent(self.cover_background_option, 0, KeySelect);
	menu_data.cover_background = (TSU_CheckBoxGetValue(self.cover_background_option) ? 1 : 0);
}

void ChangePageWithPadOptionClick(Drawable *drawable)
{
	self.general_changes_flag = true;
	TSU_CheckBoxInputEvent(self.change_page_with_page_option, 0, KeySelect);
	menu_data.change_page_with_pad = (TSU_CheckBoxGetValue(self.change_page_with_page_option) ? 1 : 0);
}

void StartInLastGameOptionClick(Drawable *drawable)
{
	self.general_changes_flag = true;
	TSU_CheckBoxInputEvent(self.start_in_last_game_option, 0, KeySelect);
	menu_data.start_in_last_game = (TSU_CheckBoxGetValue(self.start_in_last_game_option) ? 1 : 0);
}

void SaveSystemMenuConfig()
{
	int menu_type = menu_data.menu_type;
	menu_data.menu_type = menu_data.app_config.initial_view;
	SaveMenuConfig();
	menu_data.menu_type = menu_type;
}

void ExitSystemMenuClick(Drawable *drawable)
{
	HideSystemMenu();

	bool config_saved = false;
	if (self.general_changes_flag || self.style_changes_flag)
	{
		SaveSystemMenuConfig();

		if (self.style_changes_flag)
			self.refresh_main_view();
		
		config_saved = true;
	}

	if (self.cache_changes_flag && !config_saved)
	{
		if (!menu_data.rebuild_cache && menu_data.games_array_count > 0 && menu_data.cache_array_count == 0)
		{
			PopulateCache();
			SaveCache();
		}

		SaveSystemMenuConfig();
	}

	menu_data.state_app = SA_GAMES_MENU;

	if (menu_data.games_array_count > 0)
	{
		if (menu_data.category[0] != '\0')
		{
			RetrieveGamesByCategory(menu_data.category);

			if (menu_data.games_category_array_count > 0)
			{
				menu_data.games_array_ptr_count = menu_data.games_category_array_count;
				menu_data.games_array_ptr = menu_data.games_category_array;
			}
			else
			{
				memset(menu_data.category, 0, sizeof(menu_data.category));
				menu_data.games_array_ptr_count = menu_data.games_array_count;
				menu_data.games_array_ptr = (GameItemStruct **)&menu_data.games_array;
			}
		}
		else
		{
			menu_data.games_array_ptr_count = menu_data.games_array_count;
			menu_data.games_array_ptr = (GameItemStruct **)&menu_data.games_array;
		}

		if (strcasecmp(self.current_category, menu_data.category) != 0)
			self.reload_page();
	}
}

void EnableColorControls(bool enable)
{
	TSU_DrawableSetReadOnly((Drawable *)self.background_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.border_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.title_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.body_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.area_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.control_top_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.control_body_color_option, !enable);
	TSU_DrawableSetReadOnly((Drawable *)self.control_bottom_color_option, !enable);
}

void SetAllThemeProperties(const char *theme)
{
	self.theme_selected = GetTheme(theme);
	SetTheme(theme);

	char text[10] = {0};

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.background_color);
	TSU_TextBoxSetText(self.background_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.border_color);
	TSU_TextBoxSetText(self.border_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.title_color);
	TSU_TextBoxSetText(self.title_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.body_color);
	TSU_TextBoxSetText(self.body_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.area_color);
	TSU_TextBoxSetText(self.area_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.control_top_color);
	TSU_TextBoxSetText(self.control_top_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.control_body_color);
	TSU_TextBoxSetText(self.control_body_color_option, text);

	snprintf(text, sizeof(text), "%08lx", self.theme_selected.control_bottom_color);
	TSU_TextBoxSetText(self.control_bottom_color_option, text);

	if (strcasecmp(theme, CUSTOM_THEME) == 0)
		EnableColorControls(true);
	else
		EnableColorControls(false);
}

void CategoryOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.category_option, true);
}

void CategoryInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.category_option, type, key);

	if (key == KeySelect)
	{
		memset(menu_data.category, 0, sizeof(menu_data.category));
		strcpy(menu_data.category, TSU_OptionGroupGetTextSelected(self.category_option));

		if (strcasecmp(menu_data.category, "(NONE)") == 0)
			memset(menu_data.category, 0, sizeof(menu_data.category));
	}
}

void ThemeOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.theme_option, true);
}

void ThemeInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.theme_option, type, key);

	if (key == KeySelect)
	{
		self.style_changes_flag = true;
		memset(menu_data.theme, 0, sizeof(menu_data.theme));
		strcpy(menu_data.theme, TSU_OptionGroupGetTextSelected(self.theme_option));		
		SetAllThemeProperties(menu_data.theme);
	}
}

void SetColorPropertyValue(TextBox *textbox, Color *property_color, uint32 current_color)
{
	char text[10] = {0};	

	if (strlen(TSU_TextBoxGetText(textbox)) == 8 && IsHexadecimal(text))
	{
		strncpy(text, TSU_TextBoxGetText(textbox), 8);
		*property_color = ParseUIntToColor(strtoul(text, NULL, 16));
	}
	else
	{
		*property_color = ParseUIntToColor(current_color);
		snprintf(text, sizeof(text), "%08lx", current_color);
		TSU_TextBoxSetText(textbox, text);
	}
}

void BackgroundColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.background_color_option, true);
}

void BackgroundColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.background_color_option, &menu_data.background_color, self.theme_selected.background_color);
	}

	TSU_TextBoxInputEvent(self.background_color_option, type, key);
}

void BorderColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.border_color_option, true);
}

void BorderColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.border_color_option, &menu_data.border_color, self.theme_selected.border_color);
	}

	TSU_TextBoxInputEvent(self.border_color_option, type, key);
}

void TitleColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.title_color_option, true);
}

void TitleColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.title_color_option, &menu_data.title_color, self.theme_selected.title_color);
	}

	TSU_TextBoxInputEvent(self.title_color_option, type, key);
}

void BodyColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.body_color_option, true);
}

void BodyColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.body_color_option, &menu_data.body_color, self.theme_selected.body_color);
	}

	TSU_TextBoxInputEvent(self.body_color_option, type, key);
}

void AreaColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.area_color_option, true);
}

void AreaColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.area_color_option, &menu_data.area_color, self.theme_selected.area_color);
	}

	TSU_TextBoxInputEvent(self.area_color_option, type, key);
}

void ControlTopColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.control_top_color_option, true);
}

void ControlTopColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.control_top_color_option, &menu_data.control_top_color, self.theme_selected.control_top_color);
	}

	TSU_TextBoxInputEvent(self.control_top_color_option, type, key);
}

void ControlBodyColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.control_body_color_option, true);
}

void ControlBodyColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.control_body_color_option, &menu_data.control_body_color, self.theme_selected.control_body_color);
	}

	TSU_TextBoxInputEvent(self.control_body_color_option, type, key);
}

void ControlBottomColorOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.control_bottom_color_option, true);
}

void ControlBottomColorInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		self.style_changes_flag = true;
		SetColorPropertyValue(self.control_bottom_color_option, &menu_data.control_bottom_color, self.theme_selected.control_bottom_color);
	}

	TSU_TextBoxInputEvent(self.control_bottom_color_option, type, key);
}

void CacheGamesOptionClick(Drawable *drawable)
{
	self.cache_changes_flag = true;
	TSU_CheckBoxInputEvent(self.cache_game_option , 0, KeySelect);
	menu_data.enable_cache = (TSU_CheckBoxGetValue(self.cache_game_option) ? 1 : 0);

	TSU_DrawableSetReadOnly((Drawable *)self.cache_rebuild_option, !menu_data.enable_cache);
}

void RebuildCacheClick(Drawable *drawable)
{
	if (!menu_data.enable_cache || menu_data.rebuild_cache)
		return;

	menu_data.rebuild_cache = true;

	Color press_color = {1, 1.0f, 1.0f, 0.1f};
	Color drop_color = {1, 1.0f, 1.0f, 1.0f};

	TSU_LabelSetTint(self.cache_rebuild_option, &press_color);
	TSU_LabelSetText(self.cache_rebuild_option, "Rebuilding ...");

	bool enable_cache = menu_data.enable_cache;
	menu_data.enable_cache = false;

	if (RetrieveGames())
	{
		menu_data.games_array_ptr_count = menu_data.games_array_count;
		menu_data.games_array_ptr = (GameItemStruct **)&menu_data.games_array;
		memset(menu_data.category, 0, sizeof(CategoryStruct));
	}
	
	for (int imenu = 1; imenu <= MAX_MENU; imenu++)
	{
		RetrieveCovers(menu_data.current_dev, imenu);
	}

	menu_data.enable_cache = enable_cache;
	PopulateCache();
	SaveCache();
	
	self.reload_page();

	TSU_LabelSetTint(self.cache_rebuild_option, &drop_color);
	TSU_LabelSetText(self.cache_rebuild_option, "Rebuild cache");
}

void ShowOptimizeCoverPopup()
{
	StopCDDA();
	HideOptimizeCoverPopup();

	if (self.optimize_cover_popup == NULL)
	{
		char font_path[NAME_MAX];
		memset(font_path, 0, sizeof(font_path));
		snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

		Font *form_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
		Color body_color = { 1, 0, 0, 0 };

		self.optimize_cover_popup = TSU_FormCreate(640 / 2 - 500 / 2, 480 / 2 + 230 / 2 - 30, 500, 230, true, 3, DEFAULT_RADIUS, true, false, form_font, 
			&menu_data.border_color, &menu_data.control_top_color, &body_color, &menu_data.control_bottom_color, NULL);

		TSU_FormSetAttributes(self.optimize_cover_popup, 1, 2, 500, 32);

		self.optimize_covers_label = TSU_LabelCreate(form_font, "", 16, false, false);
		Label *exit_covers_label = TSU_LabelCreate(form_font, "                Press any key to exit", 16, false, false);

		TSU_DrawableSetReadOnly((Drawable *)self.optimize_covers_label, true);
		TSU_DrawableSetReadOnly((Drawable *)exit_covers_label, true);

		TSU_FormSetRowSize(self.optimize_cover_popup, 1, 50);
		TSU_FormSetRowSize(self.optimize_cover_popup, 2, 100);
		TSU_FormSetTitle(self.optimize_cover_popup, "OPTIMIZING COVERS");

		TSU_FormAddBodyLabel(self.optimize_cover_popup, self.optimize_covers_label, 1, 1);
		TSU_FormAddBodyLabel(self.optimize_cover_popup, exit_covers_label, 1, 2);

		TSU_DrawableSubAdd((Drawable *)self.scene_ptr, (Drawable *)self.optimize_cover_popup);

		form_font = NULL;
		exit_covers_label = NULL;
	}
}

void HideOptimizeCoverPopup()
{
	if (self.optimize_cover_popup != NULL)
	{
		TSU_FormDestroy(&self.optimize_cover_popup);
		self.optimize_covers_label = NULL;
	}
}

void SetMessageOptimizer(const char *fmt, const char *message)
{
	char message_optimizer[NAME_MAX];
	memset(message_optimizer, 0, sizeof(message_optimizer));
	snprintf(message_optimizer, sizeof(message_optimizer), fmt, message);
	message_optimizer[44] = '\0';
		
	if (self.optimize_covers_label != NULL)
	{
		TSU_LabelSetText(self.optimize_covers_label, message_optimizer);
	}
}

int StopOptimizeCovers()
{
	if (menu_data.optimize_game_cover_thread != NULL)
	{
		bool skip_return = self.optimize_cover_popup == NULL;
		menu_data.stop_optimize_game_cover = true;
		thd_join(menu_data.optimize_game_cover_thread, NULL);
		menu_data.optimize_game_cover_thread = NULL;
		HideOptimizeCoverPopup();

		if (!skip_return)
			return 1;
	}

	return (menu_data.optimize_game_cover_thread == NULL ? 0 : -1);
}

void ShowCoverScan()
{
	menu_data.state_app = SA_SCAN_COVER;

	StopCDDA();
	HideCoverScan();

	Vector vector_init_title = {640 / 2, 400 / 2 - 50, ML_CURSOR + 6, 1};
	Color modal_color = {1, 0.0f, 0.0f, 0.0f};

	self.modal_cover_scan = TSU_RectangleCreateWithBorder(PVR_LIST_OP_POLY, 40, 350, 560, 230, &modal_color, ML_CURSOR + 5, 3, &menu_data.border_color, DEFAULT_RADIUS);
	TSU_AppSubAddRectangle(self.dsapp_ptr, self.modal_cover_scan);

	static Color color = {1, 1.0f, 1.0f, 1.0f};
	self.title_cover_scan = TSU_LabelCreate(self.menu_font, "SCANNING MISSING COVER", 26, true, true);
	TSU_LabelSetTint(self.title_cover_scan, &color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.title_cover_scan);
	TSU_LabelSetTranslate(self.title_cover_scan, &vector_init_title);

	vector_init_title.x = 640 / 2;
	vector_init_title.y = 400 / 2 + 100;
	self.note_cover_scan = TSU_LabelCreate(self.menu_font, "\n\nPress any key to exit", 14, true, true);
	TSU_LabelSetTint(self.note_cover_scan, &color);
	TSU_AppSubAddLabel(self.dsapp_ptr, self.note_cover_scan);
	TSU_LabelSetTranslate(self.note_cover_scan, &vector_init_title);
}

void HideCoverScan()
{
	if (self.modal_cover_scan != NULL)
	{
		TSU_AppSubRemoveRectangle(self.dsapp_ptr, self.modal_cover_scan);
		thd_pass();
		TSU_RectangleDestroy(&self.modal_cover_scan);
	}

	if (self.title_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.title_cover_scan);
		thd_pass();
		TSU_LabelDestroy(&self.title_cover_scan);
	}

	if (self.note_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.note_cover_scan);
		thd_pass();
		TSU_LabelDestroy(&self.note_cover_scan);
	}

	if (self.message_cover_scan != NULL)
	{
		TSU_AppSubRemoveLabel(self.dsapp_ptr, self.message_cover_scan);
		thd_pass();
		TSU_LabelDestroy(&self.message_cover_scan);
	}
}

void SetMessageScan(const char *fmt, const char *message)
{
	char message_scan[NAME_MAX];
	memset(message_scan, 0, sizeof(message_scan));
	snprintf(message_scan, sizeof(message_scan), fmt, message);
	message_scan[49] = '\0';
		
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

int StopScanCovers()
{
	if (menu_data.load_pvr_cover_thread != NULL)
	{	
		bool skip_return = self.modal_cover_scan == NULL;
		menu_data.stop_load_pvr_cover = true;		
		thd_join(menu_data.load_pvr_cover_thread, NULL);
		menu_data.load_pvr_cover_thread = NULL;

		if (!skip_return)
			return 1;
	}

	return (menu_data.load_pvr_cover_thread == NULL ? 0 : -1);
}