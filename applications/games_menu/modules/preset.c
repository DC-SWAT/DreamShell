/* DreamShell ##version##

   preset.h
   Copyright (C) 2024 Maniac Vera

*/

#include "app_preset.h"
#include "app_definition.h"
#include "app_menu.h"
#include "app_utils.h"
#include <isoldr.h>
#include <tsunami/dsapp.h>
#include <tsunami/font.h>
#include <tsunami/anims/logxymover.h>
#include <tsunami/anims/expxymover.h>
#include <tsunami/drawables/scene.h>
#include <tsunami/drawables/rectangle.h>
#include <tsunami/drawables/optiongroup.h>
#include <tsunami/drawables/checkbox.h>
#include <tsunami/drawables/textbox.h>

extern struct menu_structure menu_data;

enum PatchEnum
{
	PATCH_ADDRESS_TYPE = 1,
	PATCH_VALUE_TYPE = 2
};

enum ViewEnum
{
	GENERAL_VIEW = 0,
	CDDA_VIEW,
	PATCH_VIEW,
	EXTENSIONS_VIEW,
	SHORTCUT_VIEW
};

static struct
{
	DSApp *dsapp_ptr;
	Scene *scene_ptr;
	Form *preset_menu_form;

	bool save;
	int body_letter_size;
	int game_index;
	uint body_height_size;
	char full_path_game[NAME_MAX];
	char *game_cover_path;
	int changed_yflip;
	bool button_start;
	bool button_x;
	bool button_y;
	bool button_z;
	bool button_lt;
	bool button_a;
	bool button_b;
	bool button_c;
	bool button_rt;

	LogXYMover *menu_init_animation;
	ExpXYMover *menu_end_animation;

	Font *message_font;
	Font *menu_font;
	Font *textbox_font;

	CheckBox *save_preset_option;
	CheckBox *dma_option;
	OptionGroup *async_option;
	CheckBox *bypass_option;
	CheckBox *cdda_option;
	CheckBox *irq_option;
	OptionGroup *os_option;
	OptionGroup *loader_option;
	OptionGroup *boot_option;
	CheckBox *fast_option;
	CheckBox *lowlevel_option;
	OptionGroup *memory_option;
	TextBox *custom_memory_option;
	OptionGroup *heap_option;

	OptionGroup *cdda_source_option;
	OptionGroup *cdda_destination_option;
	OptionGroup *cdda_position_option;
	OptionGroup *cdda_channel_option;

	TextBox *patch_address1_option;
	TextBox *patch_value1_option;
	TextBox *patch_address2_option;	
	TextBox *patchvalue2_option;
	
	CheckBox *altboot_option;
	CheckBox *screenshot_option;
	CheckBox *button_start_option;
	CheckBox *button_x_option;
	CheckBox *button_y_option;
	CheckBox *button_z_option;
	CheckBox *button_rt_option;
	CheckBox *button_a_option;
	CheckBox *button_b_option;
	CheckBox *button_c_option;
	CheckBox *button_lt_option;
	TextBox *vmu_option;
	OptionGroup *vmu_selector_option;

	OptionGroup *shortcut_size_option;
	CheckBox *shortcut_rotate_option;
	TextBox *shortcut_name_option;
	CheckBox *shortcut_dontshowname_option;
	ItemMenu *shortcut_create_option;

	Texture *cover_texture;
	Banner *cover_banner;
} self;


void CreatePresetMenu(DSApp *dsapp_ptr, Scene *scene_ptr, Font *menu_font, Font *message_font)
{
	self.dsapp_ptr = dsapp_ptr;
	self.scene_ptr = scene_ptr;
	self.message_font = message_font;
	self.menu_font = menu_font;
	self.preset_menu_form = NULL;
	self.menu_init_animation = NULL;
	self.menu_end_animation = NULL;
	self.body_letter_size = 16;
	self.body_height_size = 22;

	char font_path[NAME_MAX];
	memset(font_path, 0, sizeof(font_path));
	snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

	self.textbox_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

	memset(self.full_path_game, 0, NAME_MAX);
	self.cover_texture = NULL;
	self.cover_banner = NULL;
	self.game_cover_path = NULL;
	self.changed_yflip = -1;
}

void DestroyPresetMenu()
{
	HidePresetMenu();

	self.dsapp_ptr = NULL;
	self.scene_ptr = NULL;
	self.message_font = NULL;
	self.menu_font = NULL;
	
	TSU_FontDestroy(&self.textbox_font);

	if (menu_data.preset != NULL)
	{
		free(menu_data.preset);
		menu_data.preset = NULL;
	}

	self.cover_texture = NULL;
}

void PresetMenuRemoveAll()
{
}

void PresetMenuInputEvent(int type, int key)
{
	TSU_FormInputEvent(self.preset_menu_form, type, key);
}

int StatePresetMenu()
{
	return (self.preset_menu_form != NULL ? 1 : 0);
}

void OnViewIndexChangedEvent(Drawable *drawable, int view_index)
{
	switch (view_index)
	{
		case GENERAL_VIEW:
			CreateGeneralView((Form *)drawable);
			break;
		
		case CDDA_VIEW:
			CreateCDDAView((Form *)drawable);
			break;

		case PATCH_VIEW:
			CreatePatchView((Form *)drawable);
			break;

		case EXTENSIONS_VIEW:
			CreateExtensionsView((Form *)drawable);
			break;

		case SHORTCUT_VIEW:
			CreateShortcutView((Form *)drawable);
			break;
	}
}

void OnGetObjectsCurrentViewEvent(uint loop_index, int id, Drawable *drawable, uint type, uint row, uint column, int view_index)
{
	if (loop_index == 0 && view_index == CDDA_VIEW)
	{
		menu_data.preset->emu_cdda = CDDA_MODE_DISABLED;
	}

	switch (id)
	{
		case SAVE_CONTROL_ID:
		{
			self.save = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;
		
		case DMA_CONTROL_ID:
		{
			menu_data.preset->use_dma = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case ASYNC_CONTROL_ID:
		{
			menu_data.preset->emu_async = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case BYPASS_CONTROL_ID:
		{
			menu_data.preset->alt_read = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case IRQ_CONTROL_ID:
		{
			menu_data.preset->use_irq = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case OS_CONTROL_ID:
		{
			menu_data.preset->bin_type = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case LOADER_CONTROL_ID:
		{
			memset(menu_data.preset->device, 0, FIRMWARE_SIZE);
			strcpy(menu_data.preset->device, TSU_OptionGroupGetTextSelected((OptionGroup *)drawable));
		}
		break;

		case BOOT_CONTROL_ID:
		{
			menu_data.preset->boot_mode = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case FASTBOOT_CONTROL_ID:
		{
			menu_data.preset->fastboot = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case LOWLEVEL_CONTROL_ID:
		{
			menu_data.preset->low = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case MEMORY_CONTROL_ID:
		{
			memset(menu_data.preset->memory, 0, sizeof(menu_data.preset->memory));
			strcpy(menu_data.preset->memory, TSU_OptionGroupGetTextSelected((OptionGroup *)drawable));
		}
		break;

		case CUSTOM_MEMORY_CONTROL_ID:
		{
			
		}
		break;

		case HEAP_CONTROL_ID:
		{
			memset(menu_data.preset->heap_memory, 0, sizeof(menu_data.preset->heap_memory));
			strcpy(menu_data.preset->heap_memory, TSU_OptionGroupGetTextSelected((OptionGroup *)drawable));
			menu_data.preset->heap = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDA_CONTROL_ID:
		{
			menu_data.preset->cdda = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case CDDA_SOURCE_CONTROL_ID:
		{
			menu_data.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDA_DESTINATION_CONTROL_ID:
		{
			menu_data.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDA_POSITION_CONTROL_ID:
		{
			menu_data.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDA_CHANNEL_CONTROL_ID:
		{
			menu_data.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case PATCH_ADDRESS1_CONTROL_ID:
		{
		}
		break;

		case PATCH_VALUE1_CONTROL_ID:
		{
		}
		break;

		case PATCH_ADDRESS2_CONTROL_ID:
		{
		}
		break;

		case PATCH_VALUE2_CONTROL_ID:
		{
		}
		break;

		case ALTERBOOT_CONTROL_ID:
		{
			menu_data.preset->alt_boot = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case SCREENSHOT_CONTROL_ID:
		{
			menu_data.preset->screenshot = TSU_CheckBoxGetValue((CheckBox *)drawable);			
		}
		break;		

		case VMU_CONTROL_ID:
		{
		}
		break;

		case VMUSELECTOR_CONTROL_ID:
		{
			menu_data.preset->vmu_mode = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);			
		}
		break;

		case SHORTCUT_SIZE_CONTROL_ID:
		{
			menu_data.preset->icon_size = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case SHORTCUT_ROTATE_CONTROL_ID:
		{
			menu_data.preset->rotate_image = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case SHORTCUT_NAME_CONTROL_ID:
		{
		}
		break;

		case SHORTCUT_DONTSHOWNAME_CONTROL_ID:
		{
			menu_data.preset->dont_show_name = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case SHORTCUT_CREATE_CONTROL_ID:
		{
		}
		break;
	} 
}

void CreateGeneralView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 4, 9, 100, 34);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 120);
	TSU_FormSetColumnSize(form_ptr, 2, 170);
	TSU_FormSetColumnSize(form_ptr, 3, 140);
	TSU_FormSetColumnSize(form_ptr, 4, 170);
	TSU_FormSetRowSize(form_ptr, 7, 34);
	TSU_FormSetTitle(form_ptr, "SETTINGS - PRESET");

	int body_letter_size = self.body_letter_size - 2;
	uint body_height_size = self.body_height_size - 4;

	{
		// SAVE
		Label *save_label = TSU_LabelCreate(form_font, "SAVE AS PRESET:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)save_label, true);
		TSU_FormAddBodyLabel(form_ptr, save_label, 1, 1);

		self.save_preset_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.save_preset_option, SAVE_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.save_preset_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.save_preset_option, &SavePresetOptionClick);

		if (self.save)
		{
			TSU_CheckBoxSetOn(self.save_preset_option);
		}

		Vector save_vector = TSU_DrawableGetPosition((Drawable *)self.save_preset_option);
		save_vector.x += 80;
		TSU_DrawableSetTranslate((Drawable *)self.save_preset_option, &save_vector);
		TSU_FormSetCursor(form_ptr, (Drawable *)self.save_preset_option);

		save_label = NULL;
	}

	{
		// DMA
		Label *dma_label = TSU_LabelCreate(form_font, "DMA:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)dma_label, true);
		TSU_FormAddBodyLabel(form_ptr, dma_label, 1, 2);

		self.dma_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.dma_option, DMA_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.dma_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.dma_option, &DMAOptionClick);

		if (menu_data.preset->use_dma)
		{
			TSU_CheckBoxSetOn(self.dma_option);
		}

		dma_label = NULL;
	}

	{
		// ASYNC
		Label *async_label = TSU_LabelCreate(form_font, "ASYNC:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)async_label, true);
		TSU_FormAddBodyLabel(form_ptr, async_label, 1, 3);

		self.async_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 80, body_height_size);
		TSU_DrawableSetId((Drawable *)self.async_option, ASYNC_CONTROL_ID);

		if (TSU_CheckBoxGetValue(self.dma_option))
		{
			TSU_OptionGroupAdd(self.async_option, 0, "TRUE");
		}
		else
		{
			TSU_OptionGroupAdd(self.async_option, 0, "NONE");			
		}

		TSU_OptionGroupAdd(self.async_option, 1, "1");
		TSU_OptionGroupAdd(self.async_option, 2, "2");
		TSU_OptionGroupAdd(self.async_option, 3, "3");
		TSU_OptionGroupAdd(self.async_option, 4, "4");
		TSU_OptionGroupAdd(self.async_option, 5, "5");
		TSU_OptionGroupAdd(self.async_option, 6, "6");
		TSU_OptionGroupAdd(self.async_option, 7, "7");
		TSU_OptionGroupAdd(self.async_option, 8, "8");
		TSU_OptionGroupAdd(self.async_option, 16, "16");
		TSU_OptionGroupSetStates(self.async_option, SA_CONTROL + ASYNC_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.async_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.async_option, &AsyncOptionClick);

		if (menu_data.preset->emu_async)
		{
			TSU_OptionGroupSelectOptionByKey(self.async_option, menu_data.preset->emu_async);
		}

		async_label = NULL;
	}

	{
		// BY PASS
		Label *bypass_label = TSU_LabelCreate(form_font, "BY PASS:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)bypass_label, true);
		TSU_FormAddBodyLabel(form_ptr, bypass_label, 1, 4);

		self.bypass_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.bypass_option, BYPASS_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.bypass_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.bypass_option, &ByPassOptionClick);

		if (menu_data.preset->alt_read)
		{
			TSU_CheckBoxSetOn(self.bypass_option);
		}

		bypass_label = NULL;
	}

	{
		// IRQ
		Label *irq_label = TSU_LabelCreate(form_font, "IRQ:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)irq_label, true);
		TSU_FormAddBodyLabel(form_ptr, irq_label, 1, 5);

		self.irq_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.irq_option, IRQ_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.irq_option, 2, 5);
		TSU_DrawableEventSetClick((Drawable *)self.irq_option, &IRQOptionClick);

		if (menu_data.preset->use_irq)
		{
			TSU_CheckBoxSetOn(self.irq_option);
		}

		irq_label = NULL;
	}

	{
		// LOADER
		Label *loader_label = TSU_LabelCreate(form_font, "LOADER FW:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)loader_label, true);
		TSU_FormAddBodyLabel(form_ptr, loader_label, 1, 6);

		self.loader_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 130, body_height_size);
		TSU_DrawableSetId((Drawable *)self.loader_option, LOADER_CONTROL_ID);

		TSU_OptionGroupSetStates(self.loader_option, SA_CONTROL + LOADER_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_OptionGroupAdd(self.loader_option, 0, "auto");

		for (int i = 0; i < menu_data.firmware_array_count; i++)
		{
			TSU_OptionGroupAdd(self.loader_option, i+1, menu_data.firmware_array[i].file);	
		}

		TSU_FormAddBodyOptionGroup(form_ptr, self.loader_option, 2, 6);
		TSU_DrawableEventSetClick((Drawable *)self.loader_option, &LoaderOptionClick);

		if (menu_data.preset->device[0] != '\0' || menu_data.preset->device[0] == ' ')
		{
			TSU_OptionGroupSelectOptionByText(self.loader_option, menu_data.preset->device);
		}
		
		loader_label = NULL;
	}

	{
		// OS
		Label *os_label = TSU_LabelCreate(form_font, "OS:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)os_label, true);
		TSU_FormAddBodyLabel(form_ptr, os_label, 3, 2);

		self.os_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 130, body_height_size);
		TSU_DrawableSetId((Drawable *)self.os_option, OS_CONTROL_ID);

		TSU_OptionGroupAdd(self.os_option, 0, "AUTO");
		TSU_OptionGroupAdd(self.os_option, 1, "KATANA");
		TSU_OptionGroupAdd(self.os_option, 2, "HOMEBREW");
		TSU_OptionGroupAdd(self.os_option, 3, "WinCE");
		TSU_OptionGroupSetStates(self.os_option, SA_CONTROL + OS_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.os_option, 4, 2);
		TSU_DrawableEventSetClick((Drawable *)self.os_option, &OSOptionClick);

		if (menu_data.preset->bin_type)
		{
			TSU_OptionGroupSelectOptionByKey(self.os_option, menu_data.preset->bin_type);
		}

		os_label = NULL;
	}

	{
		// BOOT MODE
		Label *boot_label = TSU_LabelCreate(form_font, "BOOT MODE:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)boot_label, true);
		TSU_FormAddBodyLabel(form_ptr, boot_label, 3, 3);

		self.boot_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 120, body_height_size);
		TSU_DrawableSetId((Drawable *)self.boot_option, BOOT_CONTROL_ID);

		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_DIRECT, "DIRECT");
		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_IPBIN, "IP.BIN");
		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_IPBIN_TRUNC, "IP.BIN CUT");
		TSU_OptionGroupSetStates(self.boot_option, SA_CONTROL + BOOT_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.boot_option, 4, 3);
		TSU_DrawableEventSetClick((Drawable *)self.boot_option, &BootOptionClick);

		if (menu_data.preset->boot_mode)
		{
			TSU_OptionGroupSelectOptionByKey(self.boot_option, menu_data.preset->boot_mode);
		}

		boot_label = NULL;
	}

	{
		// FAST BOOT
		Label *fast_label = TSU_LabelCreate(form_font, "FAST BOOT:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)fast_label, true);
		TSU_FormAddBodyLabel(form_ptr, fast_label, 3, 4);

		self.fast_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.fast_option, FASTBOOT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.fast_option, 4, 4);
		TSU_DrawableEventSetClick((Drawable *)self.fast_option, &FastOptionClick);

		if (menu_data.preset->fastboot)
		{
			TSU_CheckBoxSetOn(self.fast_option);
		}

		fast_label = NULL;
	}

	{
		// LOW LEVEL
		Label *lowlevel_label = TSU_LabelCreate(form_font, "LOW LEVEL:", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)lowlevel_label, true);
		TSU_FormAddBodyLabel(form_ptr, lowlevel_label, 3, 5);

		self.lowlevel_option = TSU_CheckBoxCreate(form_font, (uint)body_letter_size, 50, body_height_size);
		TSU_DrawableSetId((Drawable *)self.lowlevel_option, LOWLEVEL_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.lowlevel_option, 4, 5);
		TSU_DrawableEventSetClick((Drawable *)self.lowlevel_option, &LowLevelOptionClick);

		if (menu_data.preset->low)
		{
			TSU_CheckBoxSetOn(self.lowlevel_option);
		}

		lowlevel_label = NULL;
	}

	{
		// MEMORY
		Label *memory_label = TSU_LabelCreate(form_font, "LOADER :\n\nMEMORY", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)memory_label, true);
		TSU_FormAddBodyLabel(form_ptr, memory_label, 1, 7);

		self.memory_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 130, body_height_size);
		TSU_DrawableSetId((Drawable *)self.memory_option, MEMORY_CONTROL_ID);

		TSU_OptionGroupAdd(self.memory_option, 1, "0x8c000100");
		TSU_OptionGroupAdd(self.memory_option, 2, "0x8c001100");
		TSU_OptionGroupAdd(self.memory_option, 3, "0x8c004000");
		TSU_OptionGroupAdd(self.memory_option, 4, "0x8c004800");
		TSU_OptionGroupAdd(self.memory_option, 5, "0x8c008000");
		TSU_OptionGroupAdd(self.memory_option, 6, "0x8ce00000");
		TSU_OptionGroupAdd(self.memory_option, 7, "0x8cef8000");
		TSU_OptionGroupAdd(self.memory_option, 8, "0x8cf80000");
		TSU_OptionGroupAdd(self.memory_option, 9, "0x8cfc0000");
		TSU_OptionGroupAdd(self.memory_option, 10, "0x8cfd0000");
		TSU_OptionGroupAdd(self.memory_option, 11, "0x8cfe0000");
		TSU_OptionGroupAdd(self.memory_option, 12, "0x8cfe8000");
		TSU_OptionGroupAdd(self.memory_option, 13, "0x8cff0000");
		TSU_OptionGroupAdd(self.memory_option, 14, "0x8cff4800");
		TSU_OptionGroupAdd(self.memory_option, 15, "0x8d000000");
		TSU_OptionGroupAdd(self.memory_option, 16, "0x8c");
		TSU_OptionGroupSetStates(self.memory_option, SA_CONTROL + MEMORY_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.memory_option, 2, 7);
		TSU_DrawableEventSetClick((Drawable *)self.memory_option, &MemoryOptionClick);

		if (menu_data.preset->memory[0] != 0)
		{
			if (TSU_OptionGroupGetOptionByText(self.memory_option, menu_data.preset->memory) != NULL)
			{
				TSU_OptionGroupSelectOptionByText(self.memory_option, menu_data.preset->memory);
			}
			else
			{
				TSU_OptionGroupSelectOptionByText(self.memory_option, "0x8c");
				memset(menu_data.preset->custom_memory, 0, sizeof(menu_data.preset->custom_memory));

				if (strlen(menu_data.preset->memory) == 10)
				{
					strcpy(menu_data.preset->custom_memory, &menu_data.preset->memory[4]);					
					memset(menu_data.preset->memory, 0, sizeof(menu_data.preset->memory));
					strcpy(menu_data.preset->memory, "0x8c");
				}
				else
				{
					TSU_OptionGroupSelectOptionByText(self.memory_option, "0x8c000100");					
				}
			}
		}

		memory_label = NULL;
	}

	{
		// CUSTOM MEMORY
		self.custom_memory_option = TSU_TextBoxCreate(self.textbox_font, (uint)body_letter_size, false, 130, body_height_size, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.custom_memory_option, CUSTOM_MEMORY_CONTROL_ID);

		TSU_TextBoxSetStates(self.custom_memory_option, SA_CONTROL + CUSTOM_MEMORY_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.custom_memory_option, 3, 7);
		TSU_DrawableEventSetClick((Drawable *)self.custom_memory_option, &CustomMemoryOptionClick);

		TSU_TextBoxSetText(self.custom_memory_option, menu_data.preset->custom_memory);
	}

	{
		// HEAP
		Label *heap_label = TSU_LabelCreate(form_font, "HEAP   :\n\nMEMORY", body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)heap_label, true);
		TSU_FormAddBodyLabel(form_ptr, heap_label, 1, 9);

		self.heap_option = TSU_OptionGroupCreate(form_font, (uint)body_letter_size, 320, body_height_size);
		TSU_DrawableSetId((Drawable *)self.heap_option, HEAP_CONTROL_ID);

		TSU_OptionGroupAdd(self.heap_option, HEAP_MODE_AUTO, "AUTO");
		TSU_OptionGroupAdd(self.heap_option, HEAP_MODE_BEHIND, "BEHIND THE LOADER");
		TSU_OptionGroupAdd(self.heap_option, HEAP_MODE_INGAME, "INGAME HEAP (KATANA ONLY)");
		TSU_OptionGroupAdd(self.heap_option, HEAP_MODE_MAPLE, "MAPPLE DMA BUFFER");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8c000100, "0x8c000100");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8c001100, "0x8c001100");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8c004000, "0x8c004000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8c004800, "0x8c004800");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8c008000, "0x8c008000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cef8000, "0x8cef8000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cf80000, "0x8cf80000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cfc0000, "0x8cfc0000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cfd0000, "0x8cfd0000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cfe0000, "0x8cfe0000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cfe8000, "0x8cfe8000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cff0000, "0x8cff0000");
		TSU_OptionGroupAdd(self.heap_option, (int32)0x8cff4800, "0x8cff4800");
		TSU_OptionGroupSetStates(self.heap_option, SA_CONTROL + HEAP_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.heap_option, 2, 9);
		TSU_DrawableEventSetClick((Drawable *)self.heap_option, &HeapOptionClick);

		if (menu_data.preset->heap)
		{
			TSU_OptionGroupSelectOptionByKey(self.heap_option, (int32)menu_data.preset->heap);
		}

		heap_label = NULL;
	}
}

void CreateCDDAView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 4, 3, 100, 42);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 110);
	TSU_FormSetColumnSize(form_ptr, 2, 165);
	TSU_FormSetColumnSize(form_ptr, 3, 150);
	TSU_FormSetColumnSize(form_ptr, 4, 165);
	TSU_FormSetTitle(form_ptr, "SETTINGS - PRESET");

	{
		// CDDA
		Label *cdda_label = TSU_LabelCreate(form_font, "CDDA:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cdda_label, true);
		TSU_FormAddBodyLabel(form_ptr, cdda_label, 1, 1);

		self.cdda_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cdda_option, CDDA_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.cdda_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.cdda_option, &CDDAOptionClick);

		if (menu_data.preset->cdda)
		{
			TSU_CheckBoxSetOn(self.cdda_option);
		}

		cdda_label = NULL;
	}

	{
		// CDDA SOURCE
		Label *cdda_source_label = TSU_LabelCreate(form_font, "Source:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cdda_source_label, true);
		TSU_FormAddBodyLabel(form_ptr, cdda_source_label, 1, 2);

		self.cdda_source_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cdda_source_option, CDDA_SOURCE_CONTROL_ID);

		TSU_OptionGroupAdd(self.cdda_source_option, CDDA_MODE_SRC_PIO, "PIO");
		TSU_OptionGroupAdd(self.cdda_source_option, CDDA_MODE_SRC_DMA, "DMA");
		TSU_OptionGroupSetStates(self.cdda_source_option, SA_CONTROL + CDDA_SOURCE_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cdda_source_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.cdda_source_option, &CDDASourceOptionClick);

		if (menu_data.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_source_option, (int32)CDDA_MODE_SRC_PIO);
		}
		else if (menu_data.preset->emu_cdda & CDDA_MODE_SRC_PIO)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_source_option, (int32)CDDA_MODE_SRC_PIO);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_source_option, (int32)CDDA_MODE_SRC_DMA);
		}

		cdda_source_label = NULL;
	}

	{
		// CDDA DESTINATION
		Label *cdda_destination_label = TSU_LabelCreate(form_font, "Destination:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cdda_destination_label, true);
		TSU_FormAddBodyLabel(form_ptr, cdda_destination_label, 3, 2);

		self.cdda_destination_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cdda_destination_option, CDDA_DESTINATION_CONTROL_ID);

		TSU_OptionGroupAdd(self.cdda_destination_option, 1, "PIO");
		TSU_OptionGroupAdd(self.cdda_destination_option, CDDA_MODE_DST_SQ, "SQ");
		TSU_OptionGroupAdd(self.cdda_destination_option, CDDA_MODE_DST_DMA, "DMA");
		TSU_OptionGroupSetStates(self.cdda_destination_option, SA_CONTROL + CDDA_DESTINATION_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cdda_destination_option, 4, 2);
		TSU_DrawableEventSetClick((Drawable *)self.cdda_destination_option, &CDDADestinationOptionClick);

		if (menu_data.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_destination_option, (int32)CDDA_MODE_DST_SQ);
		}
		else if (menu_data.preset->emu_cdda & CDDA_MODE_DST_SQ)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_destination_option, (int32)CDDA_MODE_DST_SQ);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_destination_option, (int32)CDDA_MODE_DST_DMA);
		}

		cdda_destination_label = NULL;
	}

	{
		// CDDA POSITION
		Label *cdda_position_label = TSU_LabelCreate(form_font, "Position:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cdda_position_label, true);
		TSU_FormAddBodyLabel(form_ptr, cdda_position_label, 1, 3);

		self.cdda_position_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cdda_position_option, CDDA_POSITION_CONTROL_ID);

		TSU_OptionGroupAdd(self.cdda_position_option, CDDA_MODE_POS_TMU1, "TMU1");
		TSU_OptionGroupAdd(self.cdda_position_option, CDDA_MODE_POS_TMU2, "TMU2");
		TSU_OptionGroupSetStates(self.cdda_position_option, SA_CONTROL + CDDA_POSITION_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cdda_position_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.cdda_position_option, &CDDAPositionOptionClick);

		if (menu_data.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_position_option, (int32)CDDA_MODE_POS_TMU1);
		}
		else if (menu_data.preset->emu_cdda & CDDA_MODE_POS_TMU1)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_position_option, (int32)CDDA_MODE_POS_TMU1);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_position_option, (int32)CDDA_MODE_POS_TMU2);
		}

		cdda_position_label = NULL;
	}

	{
		// CDDA CHANNEL
		Label *cdda_channel_label = TSU_LabelCreate(form_font, "Channel:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cdda_channel_label, true);
		TSU_FormAddBodyLabel(form_ptr, cdda_channel_label, 3, 3);

		self.cdda_channel_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cdda_channel_option, CDDA_CHANNEL_CONTROL_ID);

		TSU_OptionGroupAdd(self.cdda_channel_option, CDDA_MODE_CH_ADAPT, "Fixed");
		TSU_OptionGroupAdd(self.cdda_channel_option, CDDA_MODE_CH_FIXED, "Adaptive");
		TSU_OptionGroupSetStates(self.cdda_channel_option, SA_CONTROL + CDDA_CHANNEL_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cdda_channel_option, 4, 3);
		TSU_DrawableEventSetClick((Drawable *)self.cdda_channel_option, &CDDAChannelOptionClick);

		if (menu_data.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_channel_option, (int32)CDDA_MODE_CH_ADAPT);
		}
		else if (menu_data.preset->emu_cdda & CDDA_MODE_CH_ADAPT)
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_channel_option, (int32)CDDA_MODE_CH_ADAPT);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cdda_channel_option, (int32)CDDA_MODE_CH_FIXED);
		}

		cdda_channel_label = NULL;
	}
}

void CreatePatchView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 2, 4, 100, 42);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 220);
	TSU_FormSetColumnSize(form_ptr, 2, 170);
	TSU_FormSetTitle(form_ptr, "SETTINGS - PRESET");

	{
		// PATCH ADDRESS 1
		Label *patch_addres1_label = TSU_LabelCreate(form_font, "PATCH ADDRESS 1:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patch_addres1_label, true);
		TSU_FormAddBodyLabel(form_ptr, patch_addres1_label, 1, 1);

		self.patch_address1_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patch_address1_option, PATCH_ADDRESS1_CONTROL_ID);

		TSU_TextBoxSetStates(self.patch_address1_option, SA_CONTROL + PATCH_ADDRESS1_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patch_address1_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.patch_address1_option, &PatchAddress1OptionClick);

		if (menu_data.preset->pa[0] == 0) {
			TSU_TextBoxSetText(self.patch_address1_option, "0c000000");
		}
		else {
			TSU_TextBoxSetText(self.patch_address1_option, menu_data.preset->patch_a[0]);			
		}

		patch_addres1_label = NULL;
	}

	{
		// PATCH VALUE 1
		Label *patch_value1_label = TSU_LabelCreate(form_font, "PATCH VALUE 1:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patch_value1_label, true);
		TSU_FormAddBodyLabel(form_ptr, patch_value1_label, 1, 2);

		self.patch_value1_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patch_value1_option, PATCH_VALUE1_CONTROL_ID);

		TSU_TextBoxSetStates(self.patch_value1_option, SA_CONTROL + PATCH_VALUE1_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patch_value1_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.patch_value1_option, &PatchValue1OptionClick);
	
		if (menu_data.preset->pv[0] == 0) {
			TSU_TextBoxSetText(self.patch_value1_option, "00000000");
		}
		else {
			TSU_TextBoxSetText(self.patch_value1_option, menu_data.preset->patch_v[0]);			
		}

		patch_value1_label = NULL;
	}

	{
		// PATCH ADDRESS 2
		Label *patch_addres2_label = TSU_LabelCreate(form_font, "PATCH ADDRESS 2:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patch_addres2_label, true);
		TSU_FormAddBodyLabel(form_ptr, patch_addres2_label, 1, 3);

		self.patch_address2_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patch_address2_option, PATCH_ADDRESS2_CONTROL_ID);

		TSU_TextBoxSetStates(self.patch_address2_option, SA_CONTROL + PATCH_ADDRESS2_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patch_address2_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.patch_address2_option, &PatchAddress2OptionClick);

		if (menu_data.preset->pa[1] == 0) {
			TSU_TextBoxSetText(self.patch_address2_option, "0c000000");
		}
		else {
			TSU_TextBoxSetText(self.patch_address2_option, menu_data.preset->patch_a[1]);			
		}

		patch_addres2_label = NULL;
	}

	{
		// PATCH VALUE 2
		Label *patch_value2_label = TSU_LabelCreate(form_font, "PATCH VALUE 2:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patch_value2_label, true);
		TSU_FormAddBodyLabel(form_ptr, patch_value2_label, 1, 4);

		self.patchvalue2_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patchvalue2_option, PATCH_VALUE2_CONTROL_ID);

		TSU_TextBoxSetStates(self.patchvalue2_option, SA_CONTROL + PATCH_VALUE2_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patchvalue2_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.patchvalue2_option, &PatchValue2OptionClick);

		if (menu_data.preset->pv[1] == 0) {
			TSU_TextBoxSetText(self.patchvalue2_option, "00000000");
		}
		else {
			TSU_TextBoxSetText(self.patchvalue2_option, menu_data.preset->patch_v[1]);			
		}

		patch_value2_label = NULL;
	}
}

void CreateExtensionsView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 5, 6, 100, 42);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 250);
	TSU_FormSetColumnSize(form_ptr, 2, 320);
	TSU_FormSetRowSize(form_ptr, 3, 34);
	TSU_FormSetRowSize(form_ptr, 4, 34);
	TSU_FormSetTitle(form_ptr, "SETTINGS - EXTENSIONS");

	{
		// ALTER BOOT
		Label *alterboot_label = TSU_LabelCreate(form_font, "Boot from 2ND_READ.BIN:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)alterboot_label, true);
		TSU_FormAddBodyLabel(form_ptr, alterboot_label, 1, 1);

		self.altboot_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size - 2, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.altboot_option, ALTERBOOT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.altboot_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.altboot_option, &AlterBootOptionClick);

		Vector altboot_vector = TSU_DrawableGetPosition((Drawable *)self.altboot_option);
		altboot_vector.x += 50;
		TSU_DrawableSetTranslate((Drawable *)self.altboot_option, &altboot_vector);

		if (menu_data.preset->alt_boot)
		{
			TSU_CheckBoxSetOn(self.altboot_option);
		}

		alterboot_label = NULL;
	}

	{
		// SCREENSHOT
		Label *screenshot_label = TSU_LabelCreate(form_font, "Enable screenshots:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)screenshot_label, true);
		TSU_FormAddBodyLabel(form_ptr, screenshot_label, 1, 2);

		self.screenshot_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size - 2, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.screenshot_option, SCREENSHOT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.screenshot_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.screenshot_option, &ScreenshotOptionClick);

		Vector screenshot_vector = TSU_DrawableGetPosition((Drawable *)self.screenshot_option);
		screenshot_vector.x += 50;
		TSU_DrawableSetTranslate((Drawable *)self.screenshot_option, &screenshot_vector);

		if (menu_data.preset->screenshot)
		{
			TSU_CheckBoxSetOn(self.screenshot_option);
		}

		screenshot_label = NULL;
	}

	Vector init_button_position, start_button_vector, a_button_vector;
	{
		self.button_start_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 3, 52, self.body_height_size - 6, "START", "START");
		TSU_DrawableSetId((Drawable *)self.button_start_option, BUTTON_START_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_start_option, 1, 3);
		TSU_DrawableEventSetClick((Drawable *)self.button_start_option, &ButtonStartOptionClick);

		init_button_position = start_button_vector = TSU_DrawableGetPosition((Drawable *)self.button_start_option);
		start_button_vector.x += 40;
		TSU_DrawableSetTranslate((Drawable *)self.button_start_option, &start_button_vector);

		if (self.button_start)
		{
			TSU_CheckBoxSetOn(self.button_start_option);
		}	
	}

	{
		self.button_x_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "x", "x");
		TSU_DrawableSetId((Drawable *)self.button_x_option, BUTTON_X_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_x_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.button_x_option, &ButtonXOptionClick);

		start_button_vector.x += 120;
		TSU_DrawableSetTranslate((Drawable *)self.button_x_option, &start_button_vector);

		if (self.button_x)
		{
			TSU_CheckBoxSetOn(self.button_x_option);
		}
	}

	{
		self.button_y_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "y", "y");
		TSU_DrawableSetId((Drawable *)self.button_y_option, BUTTON_Y_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_y_option, 3, 3);
		TSU_DrawableEventSetClick((Drawable *)self.button_y_option, &ButtonYOptionClick);

		start_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_y_option, &start_button_vector);

		if (self.button_y)
		{
			TSU_CheckBoxSetOn(self.button_y_option);
		}		
	}

	{
		self.button_z_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "z", "z");
		TSU_DrawableSetId((Drawable *)self.button_z_option, BUTTON_Z_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_z_option, 4, 3);
		TSU_DrawableEventSetClick((Drawable *)self.button_z_option, &ButtonZOptionClick);

		start_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_z_option, &start_button_vector);

		if (self.button_z)
		{
			TSU_CheckBoxSetOn(self.button_z_option);
		}	
	}

	if (1 != 1) // DISABLED
	{
		self.button_lt_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "lt", "lt");
		TSU_DrawableSetId((Drawable *)self.button_lt_option, BUTTON_LT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_lt_option, 5, 3);
		TSU_DrawableEventSetClick((Drawable *)self.button_lt_option, &ButtonLTOptionClick);

		start_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_lt_option, &start_button_vector);

		if (self.button_lt)
		{
			TSU_CheckBoxSetOn(self.button_lt_option);
		}		
	}
	
	init_button_position.x += 160;
	{
		self.button_a_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "a", "a");
		TSU_DrawableSetId((Drawable *)self.button_a_option, BUTTON_A_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_a_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.button_a_option, &ButtonAOptionClick);

		a_button_vector = TSU_DrawableGetPosition((Drawable *)self.button_a_option);
		a_button_vector.x = init_button_position.x;
		TSU_DrawableSetTranslate((Drawable *)self.button_a_option, &a_button_vector);

		if (self.button_a)
		{
			TSU_CheckBoxSetOn(self.button_a_option);
		}		
	}

	{
		self.button_b_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "b", "b");
		TSU_DrawableSetId((Drawable *)self.button_b_option, BUTTON_B_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_b_option, 3, 4);
		TSU_DrawableEventSetClick((Drawable *)self.button_b_option, &ButtonBOptionClick);

		a_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_b_option, &a_button_vector);

		if (self.button_b)
		{
			TSU_CheckBoxSetOn(self.button_b_option);
		}		
	}

	{
		self.button_c_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "c", "c");
		TSU_DrawableSetId((Drawable *)self.button_c_option, BUTTON_C_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_c_option, 4, 4);
		TSU_DrawableEventSetClick((Drawable *)self.button_c_option, &ButtonCOptionClick);

		a_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_c_option, &a_button_vector);

		if (self.button_c)
		{
			TSU_CheckBoxSetOn(self.button_c_option);
		}		
	}

	if (1 != 1) // DISABLED
	{
		self.button_rt_option = TSU_CheckBoxCreateWithCustomText(form_font, (uint)self.body_letter_size - 2, 36, self.body_height_size - 6, "rt", "rt");
		TSU_DrawableSetId((Drawable *)self.button_rt_option, BUTTON_RT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.button_rt_option, 5, 4);
		TSU_DrawableEventSetClick((Drawable *)self.button_rt_option, &ButtonRTOptionClick);

		a_button_vector.x += 100;
		TSU_DrawableSetTranslate((Drawable *)self.button_rt_option, &a_button_vector);

		if (self.button_rt)
		{
			TSU_CheckBoxSetOn(self.button_rt_option);
		}		
	}

	{
		// VMU EMULATION
		Label *vmu_label = TSU_LabelCreate(form_font, "VMU EMULATION:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)vmu_label, true);
		TSU_FormAddBodyLabel(form_ptr, vmu_label, 1, 5);

		self.vmu_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size - 2, false, 130, self.body_height_size, false, false, true, false);
		TSU_DrawableSetId((Drawable *)self.vmu_option, VMU_CONTROL_ID);

		TSU_TextBoxSetStates(self.vmu_option, SA_CONTROL + VMU_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.vmu_option, 2, 5);
		TSU_DrawableEventSetClick((Drawable *)self.vmu_option, &VMUOptionClick);

		memset(menu_data.preset->vmu_file, 0, sizeof(menu_data.preset->vmu_file));
		if (menu_data.preset->emu_vmu)
		{
			snprintf(menu_data.preset->vmu_file, sizeof(menu_data.preset->vmu_file), "%03ld", menu_data.preset->emu_vmu);
			TSU_TextBoxSetText(self.vmu_option, menu_data.preset->vmu_file);
		}
		else
		{
			menu_data.preset->emu_vmu = atoi(DEFAULT_VMU_NUMBER);
			strcpy(menu_data.preset->vmu_file, DEFAULT_VMU_NUMBER);
			TSU_TextBoxSetText(self.vmu_option, menu_data.preset->vmu_file);
		}

		vmu_label = NULL;
	}

	{
		// VMU SELECTOR MODE
		self.vmu_selector_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size - 2, 320, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.vmu_selector_option, VMUSELECTOR_CONTROL_ID);

		TSU_OptionGroupAdd(self.vmu_selector_option, 0, "Disable");
		TSU_OptionGroupAdd(self.vmu_selector_option, 1, "Shared dump from root");
		TSU_OptionGroupAdd(self.vmu_selector_option, 2, "Private dump 200 blocks (128 KB)");
		TSU_OptionGroupAdd(self.vmu_selector_option, 3, "Private dump 1800 blocks (1024 KB)");
		TSU_OptionGroupSetStates(self.vmu_selector_option, SA_CONTROL + VMUSELECTOR_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.vmu_selector_option, 2, 6);
		TSU_DrawableEventSetClick((Drawable *)self.vmu_selector_option, &VMUSelectorOptionClick);

		if (menu_data.preset->vmu_mode)
		{
			TSU_OptionGroupSelectOptionByKey(self.vmu_selector_option, menu_data.preset->vmu_mode);
		}
		else
		{
			menu_data.preset->vmu_mode = 0;
			TSU_OptionGroupSelectOptionByKey(self.vmu_selector_option, 0);			
		}
	}
}

void CreateShortcutView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 3, 6, 100, 42);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 170);
	TSU_FormSetColumnSize(form_ptr, 2, 170);
	TSU_FormSetColumnSize(form_ptr, 3, 260);
	TSU_FormSetRowSize(form_ptr, 6, 60);
	TSU_FormSetTitle(form_ptr, "SETTINGS - SHORTCUT");

	{
		// SHORTCUT ICON SIZE
		Label *shortcut_size_label = TSU_LabelCreate(form_font, "Icon size:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)shortcut_size_label, true);
		TSU_FormAddBodyLabel(form_ptr, shortcut_size_label, 1, 1);

		self.shortcut_size_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size - 2, 170, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.shortcut_size_option, SHORTCUT_SIZE_CONTROL_ID);

		TSU_OptionGroupAdd(self.shortcut_size_option, 1, "48x48");
		TSU_OptionGroupAdd(self.shortcut_size_option, 2, "64x64 (as apps)");
		TSU_OptionGroupAdd(self.shortcut_size_option, 3, "96x96");
		TSU_OptionGroupAdd(self.shortcut_size_option, 4, "128x128 (Default)");
		TSU_OptionGroupAdd(self.shortcut_size_option, 5, "256x256");
		TSU_OptionGroupSetStates(self.shortcut_size_option, SA_CONTROL + SHORTCUT_SIZE_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.shortcut_size_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.shortcut_size_option, &ShortcutSizeOptionClick);

		if (menu_data.preset->icon_size)
		{
			TSU_OptionGroupSelectOptionByKey(self.shortcut_size_option, menu_data.preset->icon_size);
		}
		else
		{
			menu_data.preset->icon_size = 0;
			TSU_OptionGroupSelectOptionByKey(self.shortcut_size_option, 4);
		}

		shortcut_size_label = NULL;
	}

	{
		// SHORTCUT ROTATE IMAGE
		Label *screenshot_rotateimage_label = TSU_LabelCreate(form_font, "Rotate image 180:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)screenshot_rotateimage_label, true);
		TSU_FormAddBodyLabel(form_ptr, screenshot_rotateimage_label, 1, 2);

		self.shortcut_rotate_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size - 2, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.shortcut_rotate_option, SHORTCUT_ROTATE_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.shortcut_rotate_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.shortcut_rotate_option, &ShortcutRotateOptionClick);

		if (menu_data.preset->rotate_image)
		{
			TSU_CheckBoxSetOn(self.shortcut_rotate_option);
		}

		screenshot_rotateimage_label = NULL;
	}

	Vector name_label_vector;
	{
		// SHORTCUT NAME
		Label *shortcut_name_label = TSU_LabelCreate(form_font, "Icon name:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)shortcut_name_label, true);
		TSU_FormAddBodyLabel(form_ptr, shortcut_name_label, 1, 3);
		name_label_vector = TSU_DrawableGetPosition((Drawable *)shortcut_name_label);

		self.shortcut_name_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size - 2, false, 300, self.body_height_size, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.shortcut_name_option, SHORTCUT_NAME_CONTROL_ID);

		TSU_TextBoxSetStates(self.shortcut_name_option, SA_CONTROL + SHORTCUT_NAME_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.shortcut_name_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.shortcut_name_option, &ShortcutNameOptionClick);

		Vector shortcut_name_vector = TSU_DrawableGetPosition((Drawable *)self.shortcut_name_option);
		shortcut_name_vector.y -= 18;
		shortcut_name_vector.x = name_label_vector.x;
		TSU_DrawableSetTranslate((Drawable *)self.shortcut_name_option, &shortcut_name_vector);

		if (menu_data.preset->shortcut_name[0] == '\0' || menu_data.preset->shortcut_name[0] == ' ')
		{
			char *game_without_extension = NULL;
			GetCoverName(self.game_index, &game_without_extension);

			memset(menu_data.preset->shortcut_name, 0, sizeof(menu_data.preset->shortcut_name));
			strncpy(menu_data.preset->shortcut_name, game_without_extension, sizeof(menu_data.preset->shortcut_name) - 4);
			free(game_without_extension);
		}

		TSU_TextBoxSetText(self.shortcut_name_option, menu_data.preset->shortcut_name);
		shortcut_name_label = NULL;
	}

	{
		// SHORTCUT DONT SHOW NAME
		Label *shortcut_dontshowname_label = TSU_LabelCreate(form_font, "Don't show name:", self.body_letter_size - 2, false, false);
		TSU_DrawableSetReadOnly((Drawable*)shortcut_dontshowname_label, true);
		TSU_FormAddBodyLabel(form_ptr, shortcut_dontshowname_label, 1, 5);

		self.shortcut_dontshowname_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size - 2, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.shortcut_dontshowname_option, SHORTCUT_DONTSHOWNAME_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.shortcut_dontshowname_option, 2, 5);
		TSU_DrawableEventSetClick((Drawable *)self.shortcut_dontshowname_option, &ShortcutDontShowNameOptionClick);

		if (menu_data.preset->dont_show_name)
		{
			TSU_CheckBoxSetOn(self.shortcut_dontshowname_option);
		}

		shortcut_dontshowname_label = NULL;
	}

	{
		// CREATE SHORTCUT
		char *image_path = (char *)malloc(NAME_MAX);
		snprintf(image_path, NAME_MAX, "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/images/save.png");
		self.shortcut_create_option = TSU_ItemMenuCreate(image_path, 32, 32, PVR_LIST_TR_POLY, "Create shortcut", self.menu_font, (uint)self.body_letter_size - 2, false, 0);
		free(image_path);

		TSU_DrawableSetId((Drawable *)self.shortcut_create_option, SHORTCUT_CREATE_CONTROL_ID);
		TSU_FormAddBodyItemMenu(form_ptr, self.shortcut_create_option, 2, 6);
		TSU_DrawableEventSetClick((Drawable *)self.shortcut_create_option, &ShortcutCreateOptionClick);

		Vector shortcut_vector = TSU_DrawableGetPosition((Drawable *)self.shortcut_create_option);
		shortcut_vector.y -= 22;	
		shortcut_vector.x = name_label_vector.x;	
		TSU_DrawableSetTranslate((Drawable *)self.shortcut_create_option, &shortcut_vector);
	}

	{
		// COVER BANNER
		ShortcutLoadTextureCover();
		if (self.cover_texture != NULL)
		{
			uint16 cover_type = GetCoverType(self.game_index, MT_PLANE_TEXT);
			self.cover_banner = TSU_BannerCreate(cover_type == IT_JPG ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY, self.cover_texture);
			TSU_DrawableSetReadOnly((Drawable*)self.cover_banner, true);
			TSU_BannerSetSize(self.cover_banner, 128, 128);
			TSU_DrawableSetId((Drawable *)self.cover_banner, SHORTCUT_COVER_CONTROL_ID);
			TSU_FormAddBodyBanner(form_ptr, self.cover_banner, 3, 5);

			Vector banner_vector = TSU_DrawableGetPosition((Drawable *)self.cover_banner);
			banner_vector.x += 256/2;
			TSU_DrawableSetTranslate((Drawable *)self.cover_banner, &banner_vector);
		}
	}

	CheckIfShorcutExits();
}

void ShortcutLoadTextureCover()
{
	if (TSU_CheckBoxGetValue(self.shortcut_rotate_option) != self.changed_yflip)
	{
		self.changed_yflip = TSU_CheckBoxGetValue(self.shortcut_rotate_option);
		uint16 cover_type = GetCoverType(self.game_index, MT_PLANE_TEXT);

		if (self.game_cover_path != NULL)
		{
			Texture *texture_tmp = TSU_TextureCreateFromFile(self.game_cover_path, cover_type != IT_JPG, self.changed_yflip, 0);
			if (texture_tmp != NULL)
			{
				if (self.cover_banner != NULL)
				{
					TSU_BannerSetTexture(self.cover_banner, texture_tmp);
				}

				if (self.cover_texture != NULL)
				{
					TSU_TextureDestroy(&self.cover_texture);
				}

				self.cover_texture = texture_tmp;
				texture_tmp = NULL;
			}
		}
		else
		{
			// READ PVR
		}
	}
}

void ShowPresetMenu(int game_index)
{
	if (game_index >= 0)
	{
		self.changed_yflip = -1;
		self.game_index = game_index;
		memset(self.full_path_game, 0, sizeof(NAME_MAX));
		strcpy(self.full_path_game, GetFullGamePathByIndex(self.game_index));

		HidePresetMenu();
		
		CheckCover(self.game_index, MT_PLANE_TEXT);
		GetGameCoverPath(self.game_index, &self.game_cover_path, MT_PLANE_TEXT);

		if (self.preset_menu_form == NULL)
		{
			self.save = menu_data.save_preset;
			if (menu_data.preset == NULL || menu_data.preset->game_index != self.game_index)
			{
				if (menu_data.preset != NULL)
				{
					free(menu_data.preset);
				}	
				
				menu_data.preset = LoadPresetGame(self.game_index);
			}

			SetModeScreenshot();

			char font_path[NAME_MAX];
			memset(font_path, 0, sizeof(font_path));
			snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

			Font *form_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

			self.preset_menu_form = TSU_FormCreate(640/2 - 618/2, 480/2 + 450/2, 610, 455, true, 3, true, true, form_font, &OnViewIndexChangedEvent);
			TSU_FormSelectedEvent(self.preset_menu_form, &PresetMenuSelectedEvent);
			TSU_FormGetObjectsCurrentViewEvent(self.preset_menu_form, &OnGetObjectsCurrentViewEvent);

			{
				Label *general_label = TSU_LabelCreate(form_font, "GRAL", 12, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, general_label);
				general_label = NULL;
			}

			{
				Label *cdda_label = TSU_LabelCreate(form_font, "CDDA", 12, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, cdda_label);
				cdda_label = NULL;
			}

			{
				Label *patch_label = TSU_LabelCreate(form_font, "PATCH", 12, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, patch_label);
				patch_label = NULL;
			}

			{
				Label *extensions_label = TSU_LabelCreate(form_font, "EXTS.", 12, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, extensions_label);
				extensions_label = NULL;
			}

			{
				Label *shortcut_label = TSU_LabelCreate(form_font, "SHORTCUT", 11, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, shortcut_label);
				shortcut_label = NULL;
			}

			TSU_DrawableSubAdd((Drawable *)self.scene_ptr, (Drawable *)self.preset_menu_form);
			
			Vector menu_position = TSU_DrawableGetPosition((Drawable *)self.preset_menu_form);
			menu_position.y -= 200;
			TSU_DrawableSetTranslate((Drawable *)self.preset_menu_form, &menu_position);

			self.menu_init_animation = TSU_LogXYMoverCreate(0, 0);
			TSU_DrawableAnimAdd((Drawable *)self.preset_menu_form, (Animation *)self.menu_init_animation);
						
			form_font = NULL;		
		}
	}
}

void HidePresetMenu()
{
	if (self.preset_menu_form != NULL)
	{
		TSU_FormOnGetObjectsCurrentView(self.preset_menu_form);

		if (menu_data.preset->vmu_mode == 0)
		{
			menu_data.preset->emu_vmu = 0;
		}

		if (menu_data.preset->cdda == 0)
		{
			menu_data.preset->emu_cdda = CDDA_MODE_DISABLED;
		}

		if (menu_data.preset->vmu_mode || menu_data.preset->screenshot)
		{
			menu_data.preset->use_irq = 1;			
		}

		if (strlen(menu_data.preset->memory) == 10)
		{
			memset(menu_data.preset->custom_memory, 0, sizeof(menu_data.preset->custom_memory));
		}

		if (self.save)
		{
			SavePresetGame(menu_data.preset);
		}

		if (self.menu_init_animation != NULL)
		{
			TSU_AnimationComplete((Animation *)self.menu_init_animation, (Drawable *)self.preset_menu_form);
			TSU_LogXYMoverDestroy(&self.menu_init_animation);
		}

		TSU_FormDestroy(&self.preset_menu_form);
		self.cover_banner = NULL;

		if (self.cover_texture != NULL)
		{
			TSU_TextureDestroy(&self.cover_texture);
		}		

		if (self.game_cover_path != NULL)
		{
			free(self.game_cover_path);
			self.game_cover_path = NULL;
		}
	}
}

void PresetMenuSelectedEvent(Drawable *drawable, uint bottom_index, uint column, uint row)
{
}

void SavePresetOptionClick(Drawable *drawable)
{
	SavePresetInputEvent(0, KeySelect);	
}

void DMAOptionClick(Drawable *drawable)
{
	DMAInputEvent(0, KeySelect);

	if (TSU_CheckBoxGetValue(self.dma_option))
	{
		TSU_OptionGroupSetOptionByKey(self.async_option, 0, "TRUE");

		if (TSU_OptionGroupGetKeySelected(self.async_option) == 0)
		{
			TSU_OptionGroupSetDisplayText(self.async_option, "TRUE");
		}
	}
	else
	{
		TSU_OptionGroupSetOptionByKey(self.async_option, 0, "NONE");

		if (TSU_OptionGroupGetKeySelected(self.async_option) == 0)
		{
			TSU_OptionGroupSetDisplayText(self.async_option, "NONE");
		}
	}
}

void AsyncOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.async_option, true);
}

void ByPassOptionClick(Drawable *drawable)
{
	ByPassInputEvent(0, KeySelect);
}

void CDDAOptionClick(Drawable *drawable)
{
	CDDAInputEvent(0, KeySelect);
}

void IRQOptionClick(Drawable *drawable)
{
	IRQInputEvent(0, KeySelect);
}

void OSOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.os_option, true);
}

void LoaderOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.loader_option, true);
}

void BootOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.boot_option, true);
}

void FastOptionClick(Drawable *drawable)
{
	FastInputEvent(0, KeySelect);
}

void LowLevelOptionClick(Drawable *drawable)
{
	LowLevelInputEvent(0, KeySelect);
}

void MemoryOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.memory_option, true);
}

void CustomMemoryOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.custom_memory_option, true);
}

void HeapOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.heap_option, true);
}

void CDDASourceOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cdda_source_option, true);
}

void CDDADestinationOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cdda_destination_option, true);
}

void CDDAPositionOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cdda_position_option, true);
}

void CDDAChannelOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cdda_channel_option, true);
}

void PatchAddress1OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patch_address1_option, true);
}

void PatchValue1OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patch_value1_option, true);
}

void PatchAddress2OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patch_address2_option, true);
}

void PatchValue2OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patchvalue2_option, true);
}

void AlterBootOptionClick(Drawable *drawable)
{
	AlterBootInputEvent(0, KeySelect);
}

bool AllButtonsDisabled()
{
	return (!self.button_x && !self.button_y && !self.button_z && !self.button_rt 
			&& !self.button_a && !self.button_b && !self.button_c && !self.button_lt 
			&& !self.button_start);
}

void ScreenshotOptionClick(Drawable *drawable)
{
	ScreenshotInputEvent(0, KeySelect);

	if (TSU_CheckBoxGetValue(self.screenshot_option)) 
	{
		if (AllButtonsDisabled())
		{
			TSU_CheckBoxSetOn(self.button_start_option);
			TSU_CheckBoxSetOn(self.button_a_option);
			TSU_CheckBoxSetOn(self.button_b_option);

			self.button_a = self.button_b = self.button_start = true;
			menu_data.preset->scr_hotkey = GetModeScreenshot();
		}
	}
	else
	{
		TSU_CheckBoxSetOff(self.button_start_option);
		TSU_CheckBoxSetOff(self.button_x_option);
		TSU_CheckBoxSetOff(self.button_y_option);
		TSU_CheckBoxSetOff(self.button_z_option);
		TSU_CheckBoxSetOff(self.button_lt_option);
		TSU_CheckBoxSetOff(self.button_a_option);
		TSU_CheckBoxSetOff(self.button_b_option);
		TSU_CheckBoxSetOff(self.button_c_option);
		TSU_CheckBoxSetOff(self.button_rt_option);

		self.button_x = self.button_y = self.button_z = self.button_rt = self.button_a = self.button_b = self.button_c = self.button_lt = self.button_start = false;
		menu_data.preset->scr_hotkey = GetModeScreenshot();
	}
}

void SetModeScreenshot()
{
	self.button_x = (menu_data.preset->scr_hotkey & CONT_X);
	self.button_y = (menu_data.preset->scr_hotkey & CONT_Y);
	self.button_z = (menu_data.preset->scr_hotkey & CONT_Z);
	self.button_lt = false;
	self.button_a = (menu_data.preset->scr_hotkey & CONT_A);
	self.button_b = (menu_data.preset->scr_hotkey & CONT_B);
	self.button_c = (menu_data.preset->scr_hotkey & CONT_C);
	self.button_start = (menu_data.preset->scr_hotkey & CONT_START);
	self.button_rt = false;	
}

int GetModeScreenshot()
{
	int screenshot_mode = 0;

	if (self.button_x)
	{
		screenshot_mode |= CONT_X;
	}

	if (self.button_y)
	{
		screenshot_mode |= CONT_Y;
	}

	if (self.button_z)
	{
		screenshot_mode |= CONT_Z;
	}

	if (self.button_rt)
	{
	}

	if (self.button_a)
	{
		screenshot_mode |= CONT_A;
	}

	if (self.button_b)
	{
		screenshot_mode |= CONT_B;
	}

	if (self.button_c)
	{
		screenshot_mode |= CONT_C;
	}

	if (self.button_lt)
	{
	}

	if (self.button_start)
	{
		screenshot_mode |= CONT_START;
	}

	return screenshot_mode;
}

void CheckScreenshot()
{
	if (AllButtonsDisabled())
	{
		TSU_CheckBoxSetOff(self.screenshot_option);
		menu_data.preset->scr_hotkey = menu_data.preset->screenshot = 0;
	}
	else
	{
		TSU_CheckBoxSetOn(self.screenshot_option);
		menu_data.preset->screenshot = 1;
		menu_data.preset->scr_hotkey = GetModeScreenshot();
	}
}

void ButtonStartOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_start_option, 0, KeySelect);
	self.button_start = TSU_CheckBoxGetValue(self.button_start_option);
	CheckScreenshot();
}

void ButtonXOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_x_option, 0, KeySelect);
	self.button_x = TSU_CheckBoxGetValue(self.button_x_option);
	CheckScreenshot();
}

void ButtonYOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_y_option, 0, KeySelect);
	self.button_y = TSU_CheckBoxGetValue(self.button_y_option);
	CheckScreenshot();
}

void ButtonZOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_z_option, 0, KeySelect);
	self.button_z = TSU_CheckBoxGetValue(self.button_z_option);
	CheckScreenshot();
}

void ButtonLTOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_lt_option, 0, KeySelect);
	self.button_lt = TSU_CheckBoxGetValue(self.button_lt_option);
	CheckScreenshot();
}

void ButtonAOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_a_option, 0, KeySelect);
	self.button_a = TSU_CheckBoxGetValue(self.button_a_option);
	CheckScreenshot();
}

void ButtonBOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_b_option, 0, KeySelect);
	self.button_b = TSU_CheckBoxGetValue(self.button_b_option);
	CheckScreenshot();
}

void ButtonCOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_c_option, 0, KeySelect);
	self.button_c = TSU_CheckBoxGetValue(self.button_c_option);
	CheckScreenshot();
}

void ButtonRTOptionClick(Drawable *drawable)
{
	TSU_CheckBoxInputEvent(self.button_rt_option, 0, KeySelect);
	self.button_rt = TSU_CheckBoxGetValue(self.button_rt_option);
	CheckScreenshot();
}

void VMUOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.vmu_option, true);
}

void VMUSelectorOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.vmu_selector_option, true);
}

void ShortcutSizeOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.shortcut_size_option, true);
}

void ShortcutRotateOptionClick(Drawable *drawable)
{
	ShortcutRotateInputEvent(0, KeySelect);
}

void ShortcutNameOptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.shortcut_name_option, true);
}

void ShortcutDontShowNameOptionClick(Drawable *drawable)
{
	ShortcutDontShowNameInputEvent(0, KeySelect);
}

void ShortcutCreateOptionClick(Drawable *drawable)
{
	Color press_color = {1, 1.0f, 1.0f, 0.1f};
	Color drop_color = {1, 1.0f, 1.0f, 1.0f};

	TSU_ItemMenuSetTint(self.shortcut_create_option, press_color, press_color);
	TSU_ItemMenuSetLabelText(self.shortcut_create_option, "Creating...");

	int width = 0;
	int height = 0;
	if (self.cover_banner != NULL)
	{
		switch (TSU_OptionGroupGetKeySelected(self.shortcut_size_option))
		{
			case 1:
				width = height = 48;
				break;

			case 2:
				width = height = 64;
				break;

			case 3:
				width = height = 96;
				break;

			case 5:
				width = height = 256;
				break;
		
			default:
				width = height = 128;
				break;
		}
	}

	MakeShortcut(menu_data.preset, GetDefaultDir(menu_data.current_dev), self.full_path_game
	, TSU_CheckBoxGetValue(self.shortcut_dontshowname_option) ? false : true, self.game_cover_path
	, width, height, TSU_CheckBoxGetValue(self.shortcut_rotate_option) ? true : false);

	TSU_ItemMenuSetTint(self.shortcut_create_option, drop_color, drop_color);
	TSU_ItemMenuSetLabelText(self.shortcut_create_option, "Rewrite shortcut");
}

void ExitPresetMenuClick(Drawable *drawable)
{
	HidePresetMenu();
	menu_data.state_app = SA_GAMES_MENU;
}

void SavePresetInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.save_preset_option, type, key);
}

void DMAInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.dma_option, type, key);
}

void AsyncInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.async_option, type, key);
}

void ByPassInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.bypass_option, type, key);
}

void CDDAInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.cdda_option, type, key);
}

void IRQInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.irq_option, type, key);
}

void OSInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.os_option, type, key);
}

void LoaderInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.loader_option, type, key);
}

void BootInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.boot_option, type, key);
}

void FastInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.fast_option, type, key);
}

void LowLevelInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.lowlevel_option, type, key);
}

void MemoryInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.memory_option, type, key);
}

void CustomMemoryInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		memset(menu_data.preset->custom_memory, 0, sizeof(menu_data.preset->custom_memory));
		strcpy(menu_data.preset->custom_memory, TSU_TextBoxGetText(self.custom_memory_option));

		if (strlen(menu_data.preset->custom_memory) != 6)
		{
			memset(menu_data.preset->custom_memory, 0, sizeof(menu_data.preset->custom_memory));
			TSU_TextBoxSetText(self.custom_memory_option, menu_data.preset->custom_memory);
		}
	}

	TSU_TextBoxInputEvent(self.custom_memory_option, type, key);
}

void HeapInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.heap_option, type, key);
}

void CDDASourceInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cdda_source_option, type, key);
}

void CDDADestinationInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cdda_destination_option, type, key);
}

void CDDAPositionInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cdda_position_option, type, key);
}

void CDDAChannelInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cdda_channel_option, type, key);
}

void SetPatchByType(TextBox *patch_textbox, int control_type, char *patch_text_variable_ptr, uint32 *patch_variable_ptr)
{
	char text[10];
	memset(text, 0, sizeof(text));
	strcpy(text, TSU_TextBoxGetText(patch_textbox));
	uint32_t text_value = strtoul(text, NULL, 16);

	if( (strlen(text) != 8) || (control_type == PATCH_ADDRESS_TYPE && (!(text_value & 0xffffff) 
									|| (
											(text_value >> 24 != 0x0c) 
										&&  (text_value >> 24 != 0x8c) 
										&&  (text_value >> 24 != 0xac)
										)
								))
							|| (control_type == PATCH_VALUE_TYPE && !text_value)) 
	{		
		TSU_TextBoxSetText(patch_textbox, (control_type ==  PATCH_ADDRESS_TYPE ? "0c000000" : "00000000"));

		memset(patch_text_variable_ptr, 0, 10);
		strcpy(patch_text_variable_ptr, (control_type ==  PATCH_ADDRESS_TYPE ? "0c000000" : "00000000"));
		*patch_variable_ptr = 0;
	}
	else
	{
		memset(patch_text_variable_ptr, 0, 10);
		strcpy(patch_text_variable_ptr, text);
		*patch_variable_ptr = strtoul(text, NULL, 16);
	}
}

void PatchAddress1InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patch_address1_option, PATCH_ADDRESS_TYPE, menu_data.preset->patch_a[0], &menu_data.preset->pa[0]);
	}

	TSU_TextBoxInputEvent(self.patch_address1_option , type, key);
}

void PatchValue1InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patch_value1_option, PATCH_VALUE_TYPE, menu_data.preset->patch_v[0], &menu_data.preset->pv[0]);
	}

	TSU_TextBoxInputEvent(self.patch_value1_option, type, key);
}

void PatchAddress2InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patch_address2_option, PATCH_ADDRESS_TYPE, menu_data.preset->patch_a[1], &menu_data.preset->pa[1]);
	}

	TSU_TextBoxInputEvent(self.patch_address2_option, type, key);
}

void PatchValue2InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patchvalue2_option, PATCH_VALUE_TYPE, menu_data.preset->patch_v[1], &menu_data.preset->pv[1]);
	}

	TSU_TextBoxInputEvent(self.patchvalue2_option, type, key);
}

void AlterBootInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.altboot_option, type, key);
}

void ScreenshotInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.screenshot_option, type, key);
}

void VMUInputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		memset(menu_data.preset->vmu_file, 0, sizeof(menu_data.preset->vmu_file));
		strcpy(menu_data.preset->vmu_file, TSU_TextBoxGetText(self.vmu_option));

		if (menu_data.preset->vmu_file[0] != '\0' && ContainsOnlyNumbers(menu_data.preset->vmu_file))
		{
			menu_data.preset->emu_vmu = atoi(menu_data.preset->vmu_file);
			if (menu_data.preset->emu_vmu > MAX_VMU)
			{
				memset(menu_data.preset->vmu_file, 0, sizeof(menu_data.preset->vmu_file));
				snprintf(menu_data.preset->vmu_file, sizeof(menu_data.preset->vmu_file), "%d", MAX_VMU);
				menu_data.preset->emu_vmu = MAX_VMU;								
			}
		}
		else
		{
			strcpy(menu_data.preset->vmu_file, DEFAULT_VMU_NUMBER);
			menu_data.preset->emu_vmu = 1;	
		}

		TSU_TextBoxSetText(self.vmu_option, menu_data.preset->vmu_file);
	}

	TSU_TextBoxInputEvent(self.vmu_option, type, key);
}

void VMUSelectorInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.vmu_selector_option, type, key);
}

void ShortcutChangeCoverSize()
{
	if (self.cover_banner != NULL)
	{
		switch (TSU_OptionGroupGetKeySelected(self.shortcut_size_option))
		{
			case 1:
				TSU_BannerSetSize(self.cover_banner, 48, 48);
				break;

			case 2:
				TSU_BannerSetSize(self.cover_banner, 64, 64);
				break;

			case 3:
				TSU_BannerSetSize(self.cover_banner, 96, 96);
				break;

			case 5:
				TSU_BannerSetSize(self.cover_banner, 256, 256);
				break;
		
			default:
				TSU_BannerSetSize(self.cover_banner, 128, 128);
				break;
		}
	}
}

void ShortcutSizeInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.shortcut_size_option, type, key);
	ShortcutChangeCoverSize();
}

void ShortcutRotateInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.shortcut_rotate_option, type, key);
	ShortcutLoadTextureCover();
	ShortcutChangeCoverSize();
}

void ShortcutNameInputEvent(int type, int key)
{
	TSU_TextBoxInputEvent(self.shortcut_name_option, type, key);

	if (key == KeyCancel)
	{
		memset(menu_data.preset->shortcut_name, 0, sizeof(menu_data.preset->shortcut_name));
		strncpy(menu_data.preset->shortcut_name, TSU_TextBoxGetText(self.shortcut_name_option), sizeof(menu_data.preset->shortcut_name) - 4);

		CheckIfShorcutExits();
	}
}

void ShortcutDontShowNameInputEvent(int type, int key)
{
	TSU_CheckBoxInputEvent(self.shortcut_dontshowname_option, type, key);
	CheckIfShorcutExits();
}

void ShortcutCreateInputEvent(int type, int key)
{
}

void CheckIfShorcutExits()
{
	char shortcut_file[NAME_MAX];

	if (TSU_CheckBoxGetValue(self.shortcut_dontshowname_option) == 0)
	{
		snprintf(shortcut_file, NAME_MAX, "%s/apps/main/scripts/%s.dsc", GetDefaultDir(menu_data.current_dev), menu_data.preset->shortcut_name);
	}
	else
	{
		snprintf(shortcut_file, NAME_MAX, "%s/apps/main/scripts/_%s.dsc", GetDefaultDir(menu_data.current_dev), menu_data.preset->shortcut_name);		
	}

	if (FileExists(shortcut_file))
	{
		TSU_ItemMenuSetLabelText(self.shortcut_create_option, "Rewrite shortcut");
	}
	else
	{
		TSU_ItemMenuSetLabelText(self.shortcut_create_option, "Create shortcut");
	}
}