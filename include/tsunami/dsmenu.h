/*
   Tsunami for KallistiOS ##version##

   dsmenu.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __DSMENU_H
#define __DSMENU_H

#include <math.h>
#include "genmenu.h"
#include "drawables/label.h"
#include "drawables/rectangle.h"
#include "drawables/box.h"
#include "drawables/banner.h"
#include "drawables/itemmenu.h"

typedef void InputEventPtr(int type, int key);
typedef void ExitDoMenuEventPtr();

#ifdef __cplusplus

class DSMenu : public GenericMenu
{
private:
    InputEventPtr *InputEventCallback;
    ExitDoMenuEventPtr *ExitDoMenuCallback;

public:
    virtual void inputEvent(const Event & evt);
    virtual void doMenu();
    virtual void setBg(float r, float g, float b);
    
    Scene* getScene();
    void StartExitMenu();
   
    void addItemMenu(ItemMenu *item_menu);
    void addLabel(Label *label);
    void addBanner(Banner *banner);
    void addRectangle(Rectangle *rectangle);
    void addBox(Box *box);

    void removeItemMenu(ItemMenu *item_menu);
    void removeLabel(Label *label);
    void removeBanner(Banner *banner);
    void removeRectangle(Rectangle *rectangle);
    void removeBox(Box *box);

    DSMenu(InputEventPtr *input_event_callback);
    DSMenu(InputEventPtr *input_event_callback, ExitDoMenuEventPtr *exit_do_menu_callback);
    virtual ~DSMenu();
    
    // void DrawSquare(int x, int y, int size, uint32 color, int zIndex);
    // void DrawRectangle(int x, int y, int width, int height, uint32 color, int zIndex);
};

#else

typedef struct dsMenu DSMenu;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

DSMenu* TSU_MenuCreate(InputEventPtr *input_event_callback);
DSMenu* TSU_MenuCreateWithExit(InputEventPtr *input_event_callback, ExitDoMenuEventPtr *exit_do_menu_callback);
void TSU_MenuDoMenu(DSMenu *dsMenu);
void TSU_MenuDoMenuAsync(DSMenu *dsMenu);
void TSU_MenuDestroy(DSMenu **dsMenu);
void TSU_MenuSetBg(DSMenu *dsMenu, float r, float g, float b);
Scene* TSU_MenuGetScene(DSMenu *dsMenu);
void TSU_MenuStartExit(DSMenu *dsMenu);
void TSU_MenuSubAddLabel(DSMenu *dsMenu, Label *label_ptr);
void TSU_MenuSubAddBanner(DSMenu *dsMenu, Banner *banner_ptr);
void TSU_MenuSubAddRectangle(DSMenu *dsMenu, Rectangle *rectangle_ptr);
void TSU_MenuSubAddBox(DSMenu *dsMenu, Box *box_ptr);
void TSU_MenuSubAddItemMenu(DSMenu *dsMenu, ItemMenu *item_menu_ptr);
void TSU_MenuSubRemoveItemMenu(DSMenu *dsMenu, ItemMenu *item_menu_ptr);
void TSU_MenuSubRemoveBanner(DSMenu *dsMenu, Banner *banner_ptr);
void TSU_MenuSubRemoveRectangle(DSMenu *dsMenu, Rectangle *rectangle_ptr);
void TSU_MenuSubRemoveBox(DSMenu *dsMenu, Box *box_ptr);
void TSU_MenuSubRemoveLabel(DSMenu *dsMenu, Label *label_ptr);
void TSU_MenSetTranslate(DSMenu *dsMenu, const Vector *v);

#ifdef __cplusplus
};
#endif

#endif	//__DSMENU_H