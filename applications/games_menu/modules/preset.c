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
	PATCH_VIEW
};

static struct
{
	DSApp *dsapp_ptr;
	Scene *scene_ptr;
	Form *preset_menu_form;

	int body_letter_size;
	uint body_height_size;
	char full_path_game[NAME_MAX];

	LogXYMover *menu_init_animation;
	ExpXYMover *menu_end_animation;

	Font *message_font;
	Font *menu_font;
	Font *textbox_font;

	CheckBox *dma_option;
	OptionGroup *async_option;
	CheckBox *bypass_option;
	CheckBox *cdda_option;
	CheckBox *irq_option;
	OptionGroup *os_option;
	TextBox *loader_option;
	OptionGroup *boot_option;
	CheckBox *fast_option;
	CheckBox *lowlevel_option;
	OptionGroup *memory_option;
	TextBox *custom_memory_option;
	OptionGroup *heap_option;

	OptionGroup *cddasource_option;
	OptionGroup *cddadestination_option;
	OptionGroup *cddaposition_option;
	OptionGroup *cddachannel_option;

	TextBox *patchaddress1_option;
	TextBox *patchvalue1_option;
	TextBox *patchaddress2_option;	
	TextBox *patchvalue2_option;

	PresetStruct *preset;
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

	memset(&self.full_path_game, 0, NAME_MAX);
	self.preset = NULL;
}

void DestroyPresetMenu()
{
	HidePresetMenu();

	self.dsapp_ptr = NULL;
	self.scene_ptr = NULL;
	self.message_font = NULL;
	self.menu_font = NULL;
	
	TSU_FontDestroy(&self.textbox_font);

	if (self.preset != NULL)
	{
		free(self.preset);
		self.preset = NULL;
	}
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
	}
}

void OnGetObjectsCurrentViewEvent(uint loop_index, int id, Drawable *drawable, uint type, uint row, uint column, int view_index)
{
	static char string_value[NAME_MAX];
	memset(string_value, 0, NAME_MAX);

	if (loop_index == 0 && view_index == CDDA_VIEW)
	{
		self.preset->emu_cdda = CDDA_MODE_DISABLED;
	}

	switch (id)
	{
		case DMA_CONTROL_ID:
		{
			self.preset->use_dma = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case ASYNC_CONTROL_ID:
		{
			self.preset->emu_async = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case BYPASS_CONTROL_ID:
		{
			self.preset->alt_read = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case IRQ_CONTROL_ID:
		{
			self.preset->use_irq = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case OS_CONTROL_ID:
		{
			self.preset->bin_type = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case LOADER_CONTROL_ID:
		{
			strcpy(string_value, TSU_TextBoxGetText((TextBox *)drawable));
			memset(self.preset->device, 0, sizeof(self.preset->device));

			if (string_value[0] != '\0' && string_value[0] != ' ')
			{
				strcpy(self.preset->device, string_value);
			}
			else
			{				
				strcpy(self.preset->device, "auto");
			}
		}
		break;

		case BOOT_CONTROL_ID:
		{
			self.preset->boot_mode = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case FASTBOOT_CONTROL_ID:
		{
			self.preset->fastboot = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case LOWLEVEL_CONTROL_ID:
		{
			self.preset->low = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case MEMORY_CONTROL_ID:
		{
			memset(self.preset->memory, 0, sizeof(self.preset->memory));
			strcpy(self.preset->memory, TSU_OptionGroupGetTextSelected((OptionGroup *)drawable));
		}
		break;

		case CUSTOMMEMORY_CONTROL_ID:
		{
			memset(self.preset->custom_memory, 0, sizeof(self.preset->custom_memory));
			strcpy(self.preset->custom_memory, TSU_TextBoxGetText((TextBox *)drawable));
		}
		break;

		case HEAP_CONTROL_ID:
		{
			memset(self.preset->heap_memory, 0, sizeof(self.preset->heap_memory));
			strcpy(self.preset->heap_memory, TSU_OptionGroupGetTextSelected((OptionGroup *)drawable));
			self.preset->heap = TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDA_CONTROL_ID:
		{
			self.preset->cdda = TSU_CheckBoxGetValue((CheckBox *)drawable);
		}
		break;

		case CDDASOURCE_CONTROL_ID:
		{
			self.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDADESTINATION_CONTROL_ID:
		{
			self.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDADPOSITION_CONTROL_ID:
		{
			self.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case CDDADCHANNEL_CONTROL_ID:
		{
			self.preset->emu_cdda |= TSU_OptionGroupGetKeySelected((OptionGroup *)drawable);
		}
		break;

		case PATCHADDRESS1_CONTROL_ID:
		{
		}
		break;

		case PATCHVALUE1_CONTROL_ID:
		{
		}
		break;

		case PATCHADDRESS2_CONTROL_ID:
		{
		}
		break;

		case PATCHVALUE2_CONTROL_ID:
		{
		}
		break;
	} 
}

void CreateGeneralView(Form *form_ptr)
{
	TSU_FormtSetAttributes(form_ptr, 4, 8, 100, 40);

	Font* form_font = TSU_FormGetTitleFont(form_ptr);
	TSU_FormSetColumnSize(form_ptr, 1, 100);
	TSU_FormSetColumnSize(form_ptr, 2, 170);
	TSU_FormSetColumnSize(form_ptr, 3, 140);
	TSU_FormSetColumnSize(form_ptr, 4, 190);
	TSU_FormSetRowSize(form_ptr, 7, 28);
	TSU_FormSetTitle(form_ptr, "SETTINGS - PRESET");

	{
		// DMA
		Label *dma_label = TSU_LabelCreate(form_font, "DMA:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)dma_label, true);
		TSU_FormAddBodyLabel(form_ptr, dma_label, 1, 1);

		self.dma_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.dma_option, DMA_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.dma_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.dma_option, &DMAOptionClick);

		if (self.preset->use_dma)
		{
			TSU_CheckBoxSetOn(self.dma_option);
		}

		dma_label = NULL;
	}

	{
		// ASYNC
		Label *async_label = TSU_LabelCreate(form_font, "ASYNC:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)async_label, true);
		TSU_FormAddBodyLabel(form_ptr, async_label, 1, 2);

		self.async_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 80, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.async_option, ASYNC_CONTROL_ID);

		TSU_OptionGroupAdd(self.async_option, 0, "NONE");
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
		TSU_FormAddBodyOptionGroup(form_ptr, self.async_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.async_option, &AsyncOptionClick);

		if (self.preset->emu_async)
		{
			TSU_OptionGroupSelectOptionByKey(self.async_option, self.preset->emu_async);
		}

		async_label = NULL;
	}

	{
		// BY PASS
		Label *bypass_label = TSU_LabelCreate(form_font, "BY PASS:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)bypass_label, true);
		TSU_FormAddBodyLabel(form_ptr, bypass_label, 1, 3);

		self.bypass_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.bypass_option, BYPASS_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.bypass_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.bypass_option, &ByPassOptionClick);

		if (self.preset->alt_read)
		{
			TSU_CheckBoxSetOn(self.bypass_option);
		}

		bypass_label = NULL;
	}

	{
		// IRQ
		Label *irq_label = TSU_LabelCreate(form_font, "IRQ:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)irq_label, true);
		TSU_FormAddBodyLabel(form_ptr, irq_label, 1, 4);

		self.irq_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.irq_option, IRQ_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.irq_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.irq_option, &IRQOptionClick);

		if (self.preset->use_irq)
		{
			TSU_CheckBoxSetOn(self.irq_option);
		}

		irq_label = NULL;
	}

	{
		// OS
		Label *os_label = TSU_LabelCreate(form_font, "OS:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)os_label, true);
		TSU_FormAddBodyLabel(form_ptr, os_label, 1, 5);

		self.os_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 130, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.os_option, OS_CONTROL_ID);

		TSU_OptionGroupAdd(self.os_option, 0, "AUTO");
		TSU_OptionGroupAdd(self.os_option, 1, "KATANA");
		TSU_OptionGroupAdd(self.os_option, 2, "HOMEBREW");
		TSU_OptionGroupAdd(self.os_option, 3, "WinCE");
		TSU_OptionGroupSetStates(self.os_option, SA_CONTROL + OS_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.os_option, 2, 5);
		TSU_DrawableEventSetClick((Drawable *)self.os_option, &OSOptionClick);

		if (self.preset->bin_type)
		{
			TSU_OptionGroupSelectOptionByKey(self.os_option, self.preset->bin_type);
		}

		os_label = NULL;
	}

	{
		// LOADER
		Label *loader_label = TSU_LabelCreate(form_font, "LOADER DEVICE:", 15, false, false);
		TSU_DrawableSetReadOnly((Drawable*)loader_label, true);
		TSU_FormAddBodyLabel(form_ptr, loader_label, 3, 1);

		self.loader_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, true, 150, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.loader_option, LOADER_CONTROL_ID);

		TSU_TextBoxSetStates(self.loader_option, SA_CONTROL + LOADER_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.loader_option, 4, 1);
		TSU_DrawableEventSetClick((Drawable *)self.loader_option, &LoaderOptionClick);

		TSU_TextBoxSetText(self.loader_option, self.preset->device);
		
		loader_label = NULL;
	}

	{
		// BOOT MODE
		Label *boot_label = TSU_LabelCreate(form_font, "BOOT MODE:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)boot_label, true);
		TSU_FormAddBodyLabel(form_ptr, boot_label, 3, 2);

		self.boot_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.boot_option, BOOT_CONTROL_ID);

		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_DIRECT, "DIRECT");
		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_IPBIN, "IP.BIN");
		TSU_OptionGroupAdd(self.boot_option, BOOT_MODE_IPBIN_TRUNC, "IP.BIN CUT");
		TSU_OptionGroupSetStates(self.boot_option, SA_CONTROL + BOOT_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.boot_option, 4, 2);
		TSU_DrawableEventSetClick((Drawable *)self.boot_option, &BootOptionClick);

		if (self.preset->boot_mode)
		{
			TSU_OptionGroupSelectOptionByKey(self.boot_option, self.preset->boot_mode);
		}

		boot_label = NULL;
	}

	{
		// FAST BOOT
		Label *fast_label = TSU_LabelCreate(form_font, "FAST BOOT:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)fast_label, true);
		TSU_FormAddBodyLabel(form_ptr, fast_label, 3, 3);

		self.fast_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.fast_option, FASTBOOT_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.fast_option, 4, 3);
		TSU_DrawableEventSetClick((Drawable *)self.fast_option, &FastOptionClick);

		if (self.preset->fastboot)
		{
			TSU_CheckBoxSetOn(self.fast_option);
		}

		fast_label = NULL;
	}

	{
		// LOW LEVEL
		Label *lowlevel_label = TSU_LabelCreate(form_font, "LOW LEVEL:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)lowlevel_label, true);
		TSU_FormAddBodyLabel(form_ptr, lowlevel_label, 3, 4);

		self.lowlevel_option = TSU_CheckBoxCreate(form_font, (uint)self.body_letter_size, 50, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.lowlevel_option, LOWLEVEL_CONTROL_ID);

		TSU_FormAddBodyCheckBox(form_ptr, self.lowlevel_option, 4, 4);
		TSU_DrawableEventSetClick((Drawable *)self.lowlevel_option, &LowLevelOptionClick);

		if (self.preset->low)
		{
			TSU_CheckBoxSetOn(self.lowlevel_option);
		}

		lowlevel_label = NULL;
	}

	{
		// MEMORY
		Label *memory_label = TSU_LabelCreate(form_font, "LOADER :\n\nMEMORY", 15, false, false);
		TSU_DrawableSetReadOnly((Drawable*)memory_label, true);
		TSU_FormAddBodyLabel(form_ptr, memory_label, 1, 6);

		self.memory_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 130, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.memory_option, MEMORY_CONTROL_ID);

		TSU_OptionGroupAdd(self.memory_option, 1, "0x8c000100");
		TSU_OptionGroupAdd(self.memory_option, 2, "0x8c001100");
		TSU_OptionGroupAdd(self.memory_option, 3, "0x8c004000");
		TSU_OptionGroupAdd(self.memory_option, 4, "0x8c004800");
		TSU_OptionGroupAdd(self.memory_option, 5, "0x8c008000");
		TSU_OptionGroupAdd(self.memory_option, 6, "0x8cef8000");
		TSU_OptionGroupAdd(self.memory_option, 7, "0x8cf80000");
		TSU_OptionGroupAdd(self.memory_option, 8, "0x8cfc0000");
		TSU_OptionGroupAdd(self.memory_option, 9, "0x8cfd0000");
		TSU_OptionGroupAdd(self.memory_option, 10, "0x8cfe0000");
		TSU_OptionGroupAdd(self.memory_option, 11, "0x8cfe8000");
		TSU_OptionGroupAdd(self.memory_option, 12, "0x8cff0000");
		TSU_OptionGroupAdd(self.memory_option, 13, "0x8cff4800");
		TSU_OptionGroupAdd(self.memory_option, 14, "0x8c");
		TSU_OptionGroupSetStates(self.memory_option, SA_CONTROL + MEMORY_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.memory_option, 2, 6);
		TSU_DrawableEventSetClick((Drawable *)self.memory_option, &MemoryOptionClick);

		if (self.preset->memory[0] != 0)
		{
			TSU_OptionGroupSelectOptionByText(self.memory_option, self.preset->memory);
		}

		memory_label = NULL;
	}

	{
		// CUSTOM MEMORY
		self.custom_memory_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 130, self.body_height_size, true, false, true, false);
		TSU_DrawableSetId((Drawable *)self.custom_memory_option, CUSTOMMEMORY_CONTROL_ID);

		TSU_TextBoxSetStates(self.custom_memory_option, SA_CONTROL + CUSTOMMEMORY_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.custom_memory_option, 3, 6);
		TSU_DrawableEventSetClick((Drawable *)self.custom_memory_option, &CustomMemoryOptionClick);

		TSU_TextBoxSetText(self.custom_memory_option, self.preset->custom_memory);
	}

	{
		// HEAP
		Label *heap_label = TSU_LabelCreate(form_font, "HEAP   :\n\nMEMORY", 15, false, false);
		TSU_DrawableSetReadOnly((Drawable*)heap_label, true);
		TSU_FormAddBodyLabel(form_ptr, heap_label, 1, 8);

		self.heap_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 320, self.body_height_size);
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
		TSU_FormAddBodyOptionGroup(form_ptr, self.heap_option, 2, 8);
		TSU_DrawableEventSetClick((Drawable *)self.heap_option, &HeapOptionClick);

		if (self.preset->heap)
		{
			TSU_OptionGroupSelectOptionByKey(self.heap_option, (int32)self.preset->heap);
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

		if (self.preset->cdda)
		{
			TSU_CheckBoxSetOn(self.cdda_option);
		}

		cdda_label = NULL;
	}

	{
		// CDDA SOURCE
		Label *cddasource_label = TSU_LabelCreate(form_font, "Source:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cddasource_label, true);
		TSU_FormAddBodyLabel(form_ptr, cddasource_label, 1, 2);

		self.cddasource_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cddasource_option, CDDASOURCE_CONTROL_ID);

		TSU_OptionGroupAdd(self.cddasource_option, CDDA_MODE_SRC_PIO, "PIO");
		TSU_OptionGroupAdd(self.cddasource_option, CDDA_MODE_SRC_DMA, "DMA");
		TSU_OptionGroupSetStates(self.cddasource_option, SA_CONTROL + CDDASOURCE_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cddasource_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.cddasource_option, &CDDASourceOptionClick);

		if (self.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddasource_option, (int32)CDDA_MODE_SRC_PIO);
		}
		else if (self.preset->emu_cdda & CDDA_MODE_SRC_PIO)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddasource_option, (int32)CDDA_MODE_SRC_PIO);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cddasource_option, (int32)CDDA_MODE_SRC_DMA);
		}

		cddasource_label = NULL;
	}

	{
		// CDDA DESTINATION
		Label *cddadestination_label = TSU_LabelCreate(form_font, "Destination:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cddadestination_label, true);
		TSU_FormAddBodyLabel(form_ptr, cddadestination_label, 3, 2);

		self.cddadestination_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cddadestination_option, CDDADESTINATION_CONTROL_ID);

		TSU_OptionGroupAdd(self.cddadestination_option, 1, "PIO");
		TSU_OptionGroupAdd(self.cddadestination_option, CDDA_MODE_DST_SQ, "SQ");
		TSU_OptionGroupAdd(self.cddadestination_option, CDDA_MODE_DST_DMA, "DMA");
		TSU_OptionGroupSetStates(self.cddadestination_option, SA_CONTROL + CDDADESTINATION_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cddadestination_option, 4, 2);
		TSU_DrawableEventSetClick((Drawable *)self.cddadestination_option, &CDDADestinationOptionClick);

		if (self.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddadestination_option, (int32)CDDA_MODE_DST_SQ);
		}
		else if (self.preset->emu_cdda & CDDA_MODE_DST_SQ)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddadestination_option, (int32)CDDA_MODE_DST_SQ);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cddadestination_option, (int32)CDDA_MODE_DST_DMA);
		}

		cddadestination_label = NULL;
	}

	{
		// CDDA POSITION
		Label *cddaposition_label = TSU_LabelCreate(form_font, "Position:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cddaposition_label, true);
		TSU_FormAddBodyLabel(form_ptr, cddaposition_label, 1, 3);

		self.cddaposition_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cddaposition_option, CDDADPOSITION_CONTROL_ID);

		TSU_OptionGroupAdd(self.cddaposition_option, CDDA_MODE_POS_TMU1, "TMU1");
		TSU_OptionGroupAdd(self.cddaposition_option, CDDA_MODE_POS_TMU2, "TMU2");
		TSU_OptionGroupSetStates(self.cddaposition_option, SA_CONTROL + CDDADPOSITION_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cddaposition_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.cddaposition_option, &CDDAPositionOptionClick);

		if (self.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddaposition_option, (int32)CDDA_MODE_POS_TMU1);
		}
		else if (self.preset->emu_cdda & CDDA_MODE_POS_TMU1)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddaposition_option, (int32)CDDA_MODE_POS_TMU1);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cddaposition_option, (int32)CDDA_MODE_POS_TMU2);
		}

		cddaposition_label = NULL;
	}

	{
		// CDDA CHANNEL
		Label *cddachannel_label = TSU_LabelCreate(form_font, "Channel:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)cddachannel_label, true);
		TSU_FormAddBodyLabel(form_ptr, cddachannel_label, 3, 3);

		self.cddachannel_option = TSU_OptionGroupCreate(form_font, (uint)self.body_letter_size, 120, self.body_height_size);
		TSU_DrawableSetId((Drawable *)self.cddachannel_option, CDDADCHANNEL_CONTROL_ID);

		TSU_OptionGroupAdd(self.cddachannel_option, CDDA_MODE_CH_ADAPT, "Fixed");
		TSU_OptionGroupAdd(self.cddachannel_option, CDDA_MODE_CH_FIXED, "Adaptive");
		TSU_OptionGroupSetStates(self.cddachannel_option, SA_CONTROL + CDDADCHANNEL_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyOptionGroup(form_ptr, self.cddachannel_option, 4, 3);
		TSU_DrawableEventSetClick((Drawable *)self.cddachannel_option, &CDDAChannelOptionClick);

		if (self.preset->emu_cdda == CDDA_MODE_DISABLED)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddachannel_option, (int32)CDDA_MODE_CH_ADAPT);
		}
		else if (self.preset->emu_cdda & CDDA_MODE_CH_ADAPT)
		{
			TSU_OptionGroupSelectOptionByKey(self.cddachannel_option, (int32)CDDA_MODE_CH_ADAPT);
		}
		else
		{
			TSU_OptionGroupSelectOptionByKey(self.cddachannel_option, (int32)CDDA_MODE_CH_FIXED);
		}

		cddachannel_label = NULL;
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
		Label *patchaddres1_label = TSU_LabelCreate(form_font, "PATCH ADDRESS 1:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patchaddres1_label, true);
		TSU_FormAddBodyLabel(form_ptr, patchaddres1_label, 1, 1);

		self.patchaddress1_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patchaddress1_option, PATCHADDRESS1_CONTROL_ID);

		TSU_TextBoxSetStates(self.patchaddress1_option, SA_CONTROL + PATCHADDRESS1_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patchaddress1_option, 2, 1);
		TSU_DrawableEventSetClick((Drawable *)self.patchaddress1_option, &PatchAddress1OptionClick);

		if (self.preset->pa[0] == 0) {
			TSU_TextBoxSetText(self.patchaddress1_option, "0c000000");
		}
		else {
			TSU_TextBoxSetText(self.patchaddress1_option, self.preset->patch_a[0]);			
		}

		patchaddres1_label = NULL;
	}

	{
		// PATCH VALUE 1
		Label *patchvalue1_label = TSU_LabelCreate(form_font, "PATCH VALUE 1:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patchvalue1_label, true);
		TSU_FormAddBodyLabel(form_ptr, patchvalue1_label, 1, 2);

		self.patchvalue1_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patchvalue1_option, PATCHVALUE1_CONTROL_ID);

		TSU_TextBoxSetStates(self.patchvalue1_option, SA_CONTROL + PATCHVALUE1_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patchvalue1_option, 2, 2);
		TSU_DrawableEventSetClick((Drawable *)self.patchvalue1_option, &PatchValue1OptionClick);
	
		if (self.preset->pv[0] == 0) {
			TSU_TextBoxSetText(self.patchvalue1_option, "00000000");
		}
		else {
			TSU_TextBoxSetText(self.patchvalue1_option, self.preset->patch_v[0]);			
		}

		patchvalue1_label = NULL;
	}

	{
		// PATCH ADDRESS 2
		Label *patchaddres2_label = TSU_LabelCreate(form_font, "PATCH ADDRESS 2:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patchaddres2_label, true);
		TSU_FormAddBodyLabel(form_ptr, patchaddres2_label, 1, 3);

		self.patchaddress2_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patchaddress2_option, PATCHADDRESS2_CONTROL_ID);

		TSU_TextBoxSetStates(self.patchaddress2_option, SA_CONTROL + PATCHADDRESS2_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patchaddress2_option, 2, 3);
		TSU_DrawableEventSetClick((Drawable *)self.patchaddress2_option, &PatchAddress2OptionClick);

		if (self.preset->pa[1] == 0) {
			TSU_TextBoxSetText(self.patchaddress2_option, "0c000000");
		}
		else {
			TSU_TextBoxSetText(self.patchaddress2_option, self.preset->patch_a[1]);			
		}

		patchaddres2_label = NULL;
	}

	{
		// PATCH VALUE 2
		Label *patchvalue2_label = TSU_LabelCreate(form_font, "PATCH VALUE 2:", self.body_letter_size, false, false);
		TSU_DrawableSetReadOnly((Drawable*)patchvalue2_label, true);
		TSU_FormAddBodyLabel(form_ptr, patchvalue2_label, 1, 4);

		self.patchvalue2_option = TSU_TextBoxCreate(self.textbox_font, (uint)self.body_letter_size, false, 155, self.body_height_size, true, false, true, true);
		TSU_DrawableSetId((Drawable *)self.patchvalue2_option, PATCHVALUE2_CONTROL_ID);

		TSU_TextBoxSetStates(self.patchvalue2_option, SA_CONTROL + PATCHVALUE2_CONTROL_ID, SA_PRESET_MENU, &menu_data.state_app);
		TSU_FormAddBodyTextBox(form_ptr, self.patchvalue2_option, 2, 4);
		TSU_DrawableEventSetClick((Drawable *)self.patchvalue2_option, &PatchValue2OptionClick);

		if (self.preset->pv[1] == 0) {
			TSU_TextBoxSetText(self.patchvalue2_option, "00000000");
		}
		else {
			TSU_TextBoxSetText(self.patchvalue2_option, self.preset->patch_v[1]);			
		}

		patchvalue2_label = NULL;
	}
}

void ShowPresetMenu(const char *full_path_game)
{
	if (full_path_game != NULL)
	{
		memset(&self.full_path_game, 0, sizeof(NAME_MAX));
		strcpy(self.full_path_game, full_path_game);

		StopCDDA();
		HidePresetMenu();

		if (self.preset_menu_form == NULL)
		{
			if (self.preset != NULL)
			{
				free(self.preset);
				self.preset = NULL;
			}

			self.preset = LoadPresetGame(full_path_game);

			char font_path[NAME_MAX];
			memset(font_path, 0, sizeof(font_path));
			snprintf(font_path, sizeof(font_path), "%s/%s", GetDefaultDir(menu_data.current_dev), "apps/games_menu/fonts/default.txf");

			Font *form_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);

			self.preset_menu_form = TSU_FormCreate(640/2 - 618/2, 480/2 + 450/2, 610, 455, true, 3, true, true, form_font, &OnViewIndexChangedEvent);
			TSU_FormSelectedEvent(self.preset_menu_form, &PresetMenuSelectedEvent);
			TSU_FormGetObjectsCurrentViewEvent(self.preset_menu_form, &OnGetObjectsCurrentViewEvent);

			{
				Label *general_label = TSU_LabelCreate(form_font, "GRAL", 14, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, general_label);
				general_label = NULL;
			}

			{
				Label *cdda_label = TSU_LabelCreate(form_font, "CDDA", 14, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, cdda_label);
				cdda_label = NULL;
			}

			{
				Label *patch_label = TSU_LabelCreate(form_font, "PATCH", 14, false, false);
				TSU_FormAddBottomLabel(self.preset_menu_form, patch_label);
				patch_label = NULL;
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
		SavePresetGame(self.full_path_game, self.preset);

		if (self.menu_init_animation != NULL)
		{
			TSU_AnimationComplete((Animation *)self.menu_init_animation, (Drawable *)self.preset_menu_form);
			TSU_LogXYMoverDestroy(&self.menu_init_animation);
		}

		TSU_FormDestroy(&self.preset_menu_form);
		self.dma_option = NULL;
	}
}

void PresetMenuSelectedEvent(Drawable *drawable, uint bottom_index, uint column, uint row)
{
}

void DMAOptionClick(Drawable *drawable)
{
	DMAInputEvent(0, KeySelect);
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
	TSU_TextBoxSetFocus(self.loader_option, true);
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
	TSU_OptionGroupSetFocus(self.cddasource_option, true);
}

void CDDADestinationOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cddadestination_option, true);
}

void CDDAPositionOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cddaposition_option, true);
}

void CDDAChannelOptionClick(Drawable *drawable)
{
	TSU_OptionGroupSetFocus(self.cddachannel_option, true);
}

void PatchAddress1OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patchaddress1_option, true);
}

void PatchValue1OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patchvalue1_option, true);
}

void PatchAddress2OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patchaddress2_option, true);
}

void PatchValue2OptionClick(Drawable *drawable)
{
	TSU_TextBoxSetFocus(self.patchvalue2_option, true);
}

void ExitPresetMenuClick(Drawable *drawable)
{
	HidePresetMenu();
	menu_data.state_app = SA_GAMES_MENU;
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
	if (key == KeyCancel)
	{
		char text[20];
		memset(text, 0, sizeof(text));
		strcpy(text, TSU_TextBoxGetText(self.loader_option));
		
		if (text[0] == '\0' || text[0] == ' ')
		{
			memset(self.preset->device, 0, sizeof(self.preset->device));
			strcpy(self.preset->device, "auto");
			TSU_TextBoxSetText(self.loader_option, "auto");
		}
	}

	TSU_TextBoxInputEvent(self.loader_option, type, key);
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
	TSU_TextBoxInputEvent(self.custom_memory_option, type, key);
}

void HeapInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.heap_option, type, key);
}

void CDDASourceInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cddasource_option, type, key);
}

void CDDADestinationInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cddadestination_option, type, key);
}

void CDDAPositionInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cddaposition_option, type, key);
}

void CDDAChannelInputEvent(int type, int key)
{
	TSU_OptionGroupInputEvent(self.cddachannel_option, type, key);
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
		SetPatchByType(self.patchaddress1_option, PATCH_ADDRESS_TYPE, self.preset->patch_a[0], &self.preset->pa[0]);
	}

	TSU_TextBoxInputEvent(self.patchaddress1_option , type, key);
}

void PatchValue1InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patchvalue1_option, PATCH_VALUE_TYPE, self.preset->patch_v[0], &self.preset->pv[0]);
	}

	TSU_TextBoxInputEvent(self.patchvalue1_option, type, key);
}

void PatchAddress2InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patchaddress2_option, PATCH_ADDRESS_TYPE, self.preset->patch_a[1], &self.preset->pa[1]);
	}

	TSU_TextBoxInputEvent(self.patchaddress2_option, type, key);
}

void PatchValue2InputEvent(int type, int key)
{
	if (key == KeyCancel)
	{
		SetPatchByType(self.patchvalue2_option, PATCH_VALUE_TYPE, self.preset->patch_v[1], &self.preset->pv[1]);
	}

	TSU_TextBoxInputEvent(self.patchvalue2_option, type, key);
}