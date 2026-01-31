/*
   Tsunami for KallistiOS ##version##

   dsapp.h

   Copyright (C) 2024 Maniac Vera
   Copyright (C) 2026 SWAT

*/

#ifndef __DSAPP_H
#define __DSAPP_H

#include <math.h>
#include "genmenu.h"
#include "drawables/label.h"
#include "drawables/rectangle.h"
#include "drawables/box.h"
#include "drawables/banner.h"
#include "drawables/circle.h"
#include "drawables/wave.h"
#include "drawables/itemmenu.h"

typedef void InputEventPtr(int type, int key);
typedef void DrawOpaquePolyEventPtr();
typedef void DrawTransparentPolyEventPtr();


#ifdef __cplusplus

class DSApp : public GenericMenu
{
private:
    InputEventPtr *InputEventCallback;
    DrawOpaquePolyEventPtr *DrawOpaquePolyEventCallback;
    DrawTransparentPolyEventPtr *DrawTransparentPolyEventCallback;

public:
    virtual void inputEvent(const Event & evt);
    virtual void setDrawOpaquePolyEvent(DrawOpaquePolyEventPtr event_callback_ptr);
    virtual void setDrawTransparentPolyEvent(DrawTransparentPolyEventPtr event_callback_ptr);
    virtual void beginApp();
    virtual void doMenu();
    virtual void doAppFrame();
    virtual void doAppControl();
    virtual bool endApp();
    virtual void visualPerFrame();
    virtual void setBg(float r, float g, float b);
    
    Scene* getScene();
    void StartExitApp();
   
    void addItemMenu(ItemMenu *item_menu);
    void addLabel(Label *label);
    void addBanner(Banner *banner);
    void addCircle(Circle *circle);
    void addWave(Wave *wave);
    void addRectangle(Rectangle *rectangle);
    void addBox(Box *box);

    void removeItemMenu(ItemMenu *item_menu);
    void removeLabel(Label *label);
    void removeBanner(Banner *banner);
    void removeCircle(Circle *circle);
    void removeWave(Wave *wave);
    void removeRectangle(Rectangle *rectangle);
    void removeBox(Box *box);

    DSApp(InputEventPtr *input_event_callback);
    virtual ~DSApp();
};

#else

typedef struct dsApp DSApp;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

DSApp* TSU_AppCreate(InputEventPtr *input_event_callback);
void TSU_AppBegin(DSApp *dsApp);
void TSU_AppDoControl(DSApp *dsApp);
void TSU_AppDoFrame(DSApp *dsApp);
void TSU_AppSetDrawOpaquePolyEvent(DSApp *dsApp, DrawOpaquePolyEventPtr *event_callback);
void TSU_AppSetDrawTransparentPolyEvent(DSApp *dsApp, DrawTransparentPolyEventPtr *event_callback);
bool TSU_AppEnd(DSApp *dsApp);
void TSU_AppDoMenuAsync(DSApp *dsApp);
void TSU_AppDestroy(DSApp **dsApp);
void TSU_AppSetBg(DSApp *dsApp, float r, float g, float b);
Scene* TSU_AppGetScene(DSApp *dsApp);
void TSU_AppStartExit(DSApp *dsApp);
void TSU_AppSubAddLabel(DSApp *dsApp, Label *label_ptr);
void TSU_AppSubAddBanner(DSApp *dsApp, Banner *banner_ptr);
void TSU_AppSubAddCircle(DSApp *dsApp, Circle *circle_ptr);
void TSU_AppSubAddWave(DSApp *dsApp, Wave *wave_ptr);
void TSU_AppSubAddRectangle(DSApp *dsApp, Rectangle *rectangle_ptr);
void TSU_AppSubAddBox(DSApp *dsApp, Box *box_ptr);
void TSU_AppSubAddItemMenu(DSApp *dsApp, ItemMenu *item_menu_ptr);
void TSU_AppSubRemoveItemMenu(DSApp *dsApp, ItemMenu *item_menu_ptr);
void TSU_AppSubRemoveBanner(DSApp *dsApp, Banner *banner_ptr);
void TSU_AppSubRemoveCircle(DSApp *dsApp, Circle *circle_ptr);
void TSU_AppSubRemoveWave(DSApp *dsApp, Wave *wave_ptr);
void TSU_AppSubRemoveRectangle(DSApp *dsApp, Rectangle *rectangle_ptr);
void TSU_AppSubRemoveBox(DSApp *dsApp, Box *box_ptr);
void TSU_AppSubRemoveLabel(DSApp *dsApp, Label *label_ptr);
void TSU_AppSetTranslate(DSApp *dsApp, const Vector *v);

#ifdef __cplusplus
};
#endif

#endif	//__DSAPP_H
