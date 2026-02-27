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
    Font *font = nullptr;
    Texture *image_texture;

public:
    std::string item_value;
    int item_index;
    Banner *image;
    Label *text;

    ItemMenu(const char *image_file, float width, float height, pvr_list_type_t pvr_type, bool yflip = false, uint flags = 0);
    ItemMenu(const char *text, Font* font, int font_size = 24, float width = 0, float height = 0);
    ItemMenu(const char *image_file, float width, float height, pvr_list_type_t pvr_type, const char *text, Font* font, int font_size = 24, float label_width = 0, float label_height = 0, bool yflip = false, uint flags = 0);
    virtual ~ItemMenu();

    virtual void draw(pvr_list_type_t list);
    void FreeItem();
    void Init();    
    bool IsSelected();
    void SetImageColor(Color color);
    void SetTextColor(Color color);
    void SetColorUnselected(Color color);
    void SetSelected(bool selected = true, bool smear = true);
    bool HasTextAndImage(); 
    void SetImage(const char *image_file, pvr_list_type_t pvr_type);
    void setTint(Color text_color, Color image_color);

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

ItemMenu* TSU_ItemMenuCreate(const char *image_file, float width, float height, pvr_list_type_t pvr_type, const char *text, Font *font_ptr, int font_size, float label_width, float label_height, bool yflip, uint flags);
ItemMenu* TSU_ItemMenuCreateImage(const char *image_file, float width, float height, pvr_list_type_t pvr_type, bool yflip, uint flags);
ItemMenu* TSU_ItemMenuCreateLabel(const char *text, Font *font_ptr, int font_size, float width, float height);
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
void TSU_ItemMenuSetSelected(ItemMenu *item_menu_ptr, bool selected, bool smear);
bool TSU_ItemMenuHasTextAndImage(ItemMenu *item_menu_ptr);
Banner* TSU_ItemMenuGetBanner(ItemMenu *item_menu_ptr);
Label* TSU_ItemMenuGetLabel(ItemMenu *item_menu_ptr);
void TSU_ItemMenuSetLabelText(ItemMenu *item_menu_ptr, const char *text);
const char* TSU_ItemMenuGetLabelText(ItemMenu *item_menu_ptr);
void TSU_ItemMenuSetImage(ItemMenu *item_menu_ptr, const char *image_file, pvr_list_type_t pvr_type);
void TSU_ItemMenuSetTint(ItemMenu *item_menu_ptr, Color text_color, Color image_color);
void TSU_ItemMenuSetWindowState(ItemMenu *itemmenu_ptr, int window_state);
int TSU_ItemMenuGetWindowState(ItemMenu *itemmenu_ptr);

#ifdef __cplusplus
};
#endif

#endif	// __ITEMMENU_H