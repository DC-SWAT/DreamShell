/*
   Tsunami for KallistiOS ##version##

   itemmenu.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __ITEMMENU_H
#define __ITEMMENU_H

#include "../font.h"
#include "../animation.h"
#include "../texture.h"
#include "banner.h"
#include "label.h"

#ifdef __cplusplus

#include <memory>

class ItemMenu : public Drawable
{
private:    
    const float padding_x = 20.0f;
    const float padding_y = 20.0f;
    float width = 0;
    float height = 0;
    bool selected = false;
    Color text_color_selected;
    Color image_color_selected;
    Color color_unselected;    

    void setTint(Color text_color, Color image_color);
    Font *font = nullptr;
    Texture *image_texture;

public:
    std::string item_value;
    int item_index;
    Banner *image;
    Label *text;

    ItemMenu(const char *image_file, float width, float height, uint16 pvr_type);
    ItemMenu(const char *text, Font* font, int font_size = 24);
    ItemMenu(const char *image_file, float width, float height, uint16 pvr_type, const char *text, Font* font, int font_size = 24);
    virtual ~ItemMenu();

    virtual void draw(int list);
    void FreeItem();
    void Init();    
    bool IsSelected();
    void SetImageColor(Color color);
    void SetTextColor(Color color);
    void SetColorUnselected(Color color);
    void SetSelected(bool selected = true);    
    bool HasTextAndImage(); 

    Label* GetLabel();
    Banner* GetBanner();
};

#else

typedef struct itemMenu ItemMenu;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

ItemMenu* TSU_ItemMenuCreate(const char *image_file, float width, float height, uint16 pvr_type, const char *text, Font *font_ptr, int font_size);
ItemMenu* TSU_ItemMenuCreateImage(const char *image_file, float width, float height, uint16 pvr_type);
ItemMenu* TSU_ItemMenuCreateLabel(const char *text, Font *font_ptr, int font_size);
void TSU_ItemMenuDestroy(ItemMenu **item_menu_ptr);
void TSU_ItemMenuSetItemValue(ItemMenu *item_menu_ptr, const char *value);
const char* TSU_ItemMenuGetItemValue(ItemMenu *item_menu_ptr);
void TSU_ItemMenuSetTranslate(ItemMenu *item_menu_ptr, const Vector *v);
void TSU_ItemMenuSetItemIndex(ItemMenu *item_menu_ptr, int index);
int TSU_ItemMenuGetItemIndex(ItemMenu *item_menu_ptr);
void TSU_ItemMenuAnimAdd(ItemMenu *item_menu_ptr, Animation *anim_ptr);
void TSU_ItemMenuFreeItem(ItemMenu *item_menu_ptr);
void TSU_ItemMenuInit(ItemMenu *item_menu_ptr);    
bool TSU_ItemMenuIsSelected(ItemMenu *item_menu_ptr);
void TSU_ItemMenuSetImageColor(ItemMenu *item_menu_ptr, const Color *color);
void TSU_ItemMenuSetTextColor(ItemMenu *item_menu_ptr, const Color *color);
void TSU_ItemMenuSetColorUnselected(ItemMenu *item_menu_ptr, const Color *color);
void TSU_ItemMenuSetSelected(ItemMenu *item_menu_ptr, bool selected);
bool TSU_ItemMenuHasTextAndImage(ItemMenu *item_menu_ptr);
Banner* TSU_ItemMenuGetBanner(ItemMenu *item_menu_ptr);
Label* TSU_ItemMenuGetLabel(ItemMenu *item_menu_ptr);
void TSU_ItemMenuSetLabelText(ItemMenu *item_menu_ptr, const char *text);
const char* TSU_ItemMenuGetLabelText(ItemMenu *item_menu_ptr);

#ifdef __cplusplus
};
#endif

#endif	// __ITEMMENU_H