/*
   Tsunami for KallistiOS ##version##

   itemmenu.cpp

   Copyright (C) 2024 Maniac Vera
   
*/

#include "drawables/itemmenu.h"
#include "tsudefinition.h"
#include "tsunamiutils.h"
#include <cstring>

ItemMenu::ItemMenu(const char *image_file, float width, float height, uint16 pvr_type, bool yflip, uint flags)
{
	setObjectType(ObjectTypeEnum::ITEMMENU_TYPE);
    Init();

    Vector translateVector = Vector(padding_x, padding_y, 2);

	m_width = width;
	m_height = height;
    
    image_texture = new Texture(image_file, pvr_type == PVR_LIST_TR_POLY, yflip, flags);
    image = new Banner(pvr_type, image_texture);
    image->setSize(width, height);        
    image->setTranslate(translateVector);
    this->subAdd(image);	
}

ItemMenu::ItemMenu(const char *text, Font *font, int font_size)
{
	setObjectType(ObjectTypeEnum::ITEMMENU_TYPE);
    Init();
    
    Vector translateVector = Vector(padding_x, padding_y, 2);

    if (font != nullptr) {
        this->font = font;
    }

	float w, h;
	font->getTextSize(text, &w, &h);
	m_width = w;
	m_height = h;

    this->text = new Label(font, text, font_size, false, false);       
    this->text->setTranslate(translateVector);
    this->subAdd(this->text);
}

ItemMenu::ItemMenu(const char *image_file, float width, float height, uint16 pvr_type, const char *text, Font *font, int font_size, bool yflip, uint flags)
{
	setObjectType(ObjectTypeEnum::ITEMMENU_TYPE);
    Init();

    Vector translateVector = Vector(padding_x, padding_y, 2);

    image_texture = new Texture(image_file, pvr_type == PVR_LIST_TR_POLY, yflip, flags);
    image = new Banner(pvr_type, image_texture);
    image->setSize(width, height);
    image->setTranslate(translateVector);
    this->subAdd(image);
    
    if (font != nullptr) {
        this->font = font;
    }

    translateVector.x += width/2 + 6.f;
    this->text = new Label(font, text, font_size, false, false);

	float w, h;
	font->getTextSize(text, &w, &h);

	m_width = width + w - padding_x;
	m_height = height + padding_y/2;
	
	translateVector.y +=  (h/2 - padding_y/2);
    this->text->setTranslate(translateVector);
    this->subAdd(this->text);
}

ItemMenu::~ItemMenu()
{
    FreeItem();
}

void ItemMenu::setTint(Color text_color, Color image_color)
{
    if (text != nullptr) {
        text->setTint(text_color);
    }

    if (image != nullptr) {
        image->setTint(image_color);
    }
}

void ItemMenu::FreeItem()
{
	if (image_texture != nullptr) {
		delete image_texture;
		image_texture = nullptr;
	}

    if (image != nullptr) {
        delete image;
        image = nullptr;
    }

    if (text != nullptr) {
        delete text;
        text = nullptr;
    }

    item_value.clear();
    std::string().swap(item_value);
    subRemoveAll();
}

void ItemMenu::Init() 
{
    item_value = "";
    text_color_selected = Color(1, 1.0f, 1.0f, 1.0f);
    image_color_selected = Color(1, 1.0f, 1.0f, 1.0f);
    color_unselected = Color(1, 0.7f, 0.7f, 0.7f);

    SetImageColor(image_color_selected);
    SetTextColor(text_color_selected);
    SetColorUnselected(color_unselected);
    font = nullptr;
    text = nullptr;
    image = nullptr;
    image_texture = nullptr;
}

void ItemMenu::draw(int list)
{
    Drawable::draw(list);
}

bool ItemMenu::IsSelected()
{
    return selected;
}

bool ItemMenu::HasTextAndImage()
{
    return (text != nullptr && image != nullptr);
}

void ItemMenu::SetImageColor(Color color)
{
    image_color_selected = color;
}

void ItemMenu::SetTextColor(Color color)
{
    text_color_selected = color;
}

void ItemMenu::SetColorUnselected(Color color)
{
    color_unselected = color;
}

void ItemMenu::SetSelected(bool selected, bool smear)
{
    this->selected = selected;
    
	static float z_translate = Drawable::getTranslate().z;
    Vector translate = Drawable::getTranslate();
    if (selected) {
        translate.z = z_translate + MenuLayerEnum::ML_SELECTED;
        this->setTint(text_color_selected, image_color_selected);
        text->setSmear(smear);
    }
    else {
        translate.z = z_translate + MenuLayerEnum::ML_ITEM;
        this->setTint(color_unselected, color_unselected);
        text->setSmear(false);
    }
    Drawable::setTranslate(translate);
}

void ItemMenu::SetImage(const char *image_file, uint16 pvr_type)
{
	if (image_file) {
		Texture *new_image_texture = new Texture(image_file, pvr_type == PVR_LIST_TR_POLY);
		if (new_image_texture != nullptr) {			
			image->setTexture(new_image_texture);
			image->setTextureType(pvr_type);

			if (image_texture != nullptr) {
				delete image_texture;
				image_texture = nullptr;
			}

			image_texture = new_image_texture;
		}
	}
}

Label* ItemMenu::GetLabel()
{
    return text;
}

Banner* ItemMenu::GetBanner()
{
    return image;
}

extern "C"
{

	ItemMenu* TSU_ItemMenuCreate(const char *image_file, float width, float height, uint16 pvr_type, const char *text, Font *font_ptr, int font_size, bool yflip, uint flags)
	{
		if (font_ptr != NULL) {
			return new ItemMenu(image_file, width, height, pvr_type, text, font_ptr, font_size, yflip, flags);
		}
		else {
			return NULL;
		}
	}

	ItemMenu* TSU_ItemMenuCreateImage(const char *image_file, float width, float height, uint16 pvr_type, bool yflip, uint flags)
	{
		return new ItemMenu(image_file, width, height, pvr_type, yflip, flags);
	}

	ItemMenu* TSU_ItemMenuCreateLabel(const char *text, Font *font_ptr, int font_size)
	{
		if (font_ptr != NULL) {
			return new ItemMenu(text, font_ptr, font_size);
		}
		else {
			return NULL;
		}
	}

	void TSU_ItemMenuDestroy(ItemMenu **item_menu_ptr)
	{
		if (*item_menu_ptr != NULL) {
			delete *item_menu_ptr;
			*item_menu_ptr = NULL;
		}
	}

	void TSU_ItemMenuSetItemValue(ItemMenu *item_menu_ptr, const char *value)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->item_value = value;
		}
	}

	const char* TSU_ItemMenuGetItemValue(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->item_value.c_str();
		}
		else {
			return NULL;
		}
	}

	void TSU_ItemMenuSetItemIndex(ItemMenu *item_menu_ptr, int index)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->item_index = index;
		}
	}

	int TSU_ItemMenuGetItemIndex(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->item_index;
		}
		else {
			return -1;
		}
	}

	void TSU_ItemMenuSetTranslate(ItemMenu *item_menu_ptr, const Vector *v)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->setTranslate(*v);
		}
	}

	void TSU_ItemMenuAnimAdd(ItemMenu *item_menu_ptr, Animation *anim_ptr)
	{
		if (item_menu_ptr != NULL && anim_ptr != NULL) {
			item_menu_ptr->animAdd(anim_ptr);
		}
	}

	void TSU_ItemMenuFreeItem(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->FreeItem();
		}
	}

	void TSU_ItemMenuInit(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->Init();
		}
	}

	bool TSU_ItemMenuIsSelected(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->IsSelected();
		}
		else {
			return false;
		}
	}

	void TSU_ItemMenuSetImageColor(ItemMenu *item_menu_ptr, const Color *color)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->SetImageColor(*color);
		}
	}

	void TSU_ItemMenuSetTextColor(ItemMenu *item_menu_ptr, const Color *color)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->SetTextColor(*color);
		}
	}

	void TSU_ItemMenuSetColorUnselected(ItemMenu *item_menu_ptr, const Color *color)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->SetColorUnselected(*color);
		}
	}

	void TSU_ItemMenuSetSelected(ItemMenu *item_menu_ptr, bool selected, bool smear)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->SetSelected(selected, smear);
		}
	}

	bool TSU_ItemMenuHasTextAndImage(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->HasTextAndImage();
		}
		else {
			return false;
		}
	}

	Banner* TSU_ItemMenuGetBanner(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->GetBanner();
		}
		else {
			return NULL;
		}
	}

	Label* TSU_ItemMenuGetLabel(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {
			return item_menu_ptr->GetLabel();
		}
		else {
			return NULL;
		}
	}	

	void TSU_ItemMenuSetLabelText(ItemMenu *item_menu_ptr, const char *text)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->GetLabel()->setText(text);
		}
	}

	const char* TSU_ItemMenuGetLabelText(ItemMenu *item_menu_ptr)
	{
		if (item_menu_ptr != NULL) {  
			static char *text = NULL;
			
			if (text != NULL) {
				free(text);
				text = NULL;
			}

			text = (char *)malloc(item_menu_ptr->GetLabel()->getText().length() + 1);
			memset(text, 0, item_menu_ptr->GetLabel()->getText().length() + 1);
			strcpy(text, item_menu_ptr->GetLabel()->getText().c_str());

			return text;
		}
		else {
			return NULL;
		}
	}

	void TSU_ItemMenuSetImage(ItemMenu *item_menu_ptr, const char *image_file, uint16 pvr_type)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->SetImage(image_file, pvr_type);
		}
	}

	void TSU_ItemMenuSetTint(ItemMenu *item_menu_ptr, Color text_color, Color image_color)
	{
		if (item_menu_ptr != NULL) {
			item_menu_ptr->setTint(text_color, image_color);
		}
	}
}