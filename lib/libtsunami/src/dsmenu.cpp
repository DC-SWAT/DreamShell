/*
   Tsunami for KallistiOS ##version##

   itemmenu.cpp

   Copyright (C) 2024 Maniac Vera
   
*/

#include "dsmenu.h"
#include <kos.h>
#include "tsunamiutils.h"
// #include <cstring>
// #include <mp3/sndserver.h>

#define IN_CACHE_GAMES


DSMenu::DSMenu(InputEventPtr *input_event_callback)
{
    InputEventCallback = input_event_callback;
    ExitDoMenuCallback = NULL;
}

DSMenu::DSMenu(InputEventPtr *input_event_callback, ExitDoMenuEventPtr *exit_do_menu_callback)
{
    InputEventCallback = input_event_callback;
    ExitDoMenuCallback = exit_do_menu_callback;
}

DSMenu::~DSMenu()
{   
    InputEventCallback = nullptr;
    ExitDoMenuCallback = nullptr;
}

void DSMenu::inputEvent(const Event & evt)
{
    if (InputEventCallback != NULL) {
        InputEventCallback(evt.type, evt.key);
    }
}

void DSMenu::doMenu()
{
    GenericMenu::doMenu();
    if (ExitDoMenuCallback != NULL) {
        ExitDoMenuCallback();
    }
}

void DSMenu::setBg(float r, float g, float b)
{
    GenericMenu::setBg(r, g, b);
}

Scene* DSMenu::getScene()
{
    return m_scene;
}

void DSMenu::addItemMenu(ItemMenu *item_menu)
{
    m_scene->subAdd(item_menu);
}

void DSMenu::addLabel(Label *label)
{
    m_scene->subAdd(label);
}

void DSMenu::addBanner(Banner *banner)
{
    m_scene->subAdd(banner);
}

void DSMenu::addRectangle(Rectangle *rectangle)
{
    m_scene->subAdd(rectangle);
}

void DSMenu::addBox(Box *box)
{
    m_scene->subAdd(box);
}

void DSMenu::removeItemMenu(ItemMenu *item_menu)
{
    m_scene->subRemove(item_menu);
}

void DSMenu::removeLabel(Label *label)
{
    m_scene->subRemove(label);
}

void DSMenu::removeBanner(Banner *banner)
{
    m_scene->subRemove(banner);
}

void DSMenu::removeRectangle(Rectangle *rectangle)
{
    m_scene->subRemove(rectangle);
}

void DSMenu::removeBox(Box *box)
{
    m_scene->subRemove(box);
}

void DSMenu::StartExitMenu()
{
    GenericMenu::startExit();
}

extern "C" 
{

	DSMenu* TSU_MenuCreate(InputEventPtr *input_event_callback)
	{
		return new DSMenu(input_event_callback);
	}

	DSMenu* TSU_MenuCreateWithExit(InputEventPtr *input_event_callback, ExitDoMenuEventPtr *exit_do_menu_callback)
	{
		return new DSMenu(input_event_callback, exit_do_menu_callback);
	}

	void TSU_MenuDoMenu(DSMenu *dsMenu)
	{
		if (dsMenu != nullptr) {          
			dsMenu->doMenu();
		}
	}

	void TSU_MenuDestroy(DSMenu **dsMenu)
	{
		if (*dsMenu != nullptr) {
			delete *dsMenu;
			*dsMenu = nullptr;
		}
	}

	void TSU_MenuSetBg(DSMenu *dsMenu, float r, float g, float b)
	{
		if (dsMenu != nullptr) {
			dsMenu->setBg(r, g, b);
		}
	}

	void TSU_MenuStartExit(DSMenu *dsMenu)
	{
		if (dsMenu != nullptr) {
			dsMenu->StartExitMenu();
		}         
	}

	Scene* TSU_MenuGetScene(DSMenu *dsMenu)
	{
		if (dsMenu != nullptr) {          
			return dsMenu->getScene();
		}   
		else {
			return NULL;
		}     
	}

	void TSU_MenuSubAddLabel(DSMenu *dsMenu, Label *label_ptr)
	{
		if (dsMenu != NULL && label_ptr != NULL) {
			dsMenu->addLabel(label_ptr);
		}
	}

	void TSU_MenuSubAddBanner(DSMenu *dsMenu, Banner *banner_ptr)
	{
		if (dsMenu != NULL && banner_ptr != NULL) {
			dsMenu->addBanner(banner_ptr);
		}
	} 

	void TSU_MenuSubAddRectangle(DSMenu *dsMenu, Rectangle *rectangle_ptr)
	{
		if (dsMenu != NULL && rectangle_ptr != NULL) {
			dsMenu->addRectangle(rectangle_ptr);
		}
	}

	void TSU_MenuSubAddBox(DSMenu *dsMenu, Box *box_ptr)
	{
		if (dsMenu != NULL && box_ptr != NULL) {
			dsMenu->addBox(box_ptr);
		}
	}  

	void TSU_MenuSubAddItemMenu(DSMenu *dsMenu, ItemMenu *item_menu_ptr)
	{
		if (dsMenu != NULL && item_menu_ptr != NULL) {
			dsMenu->addItemMenu(item_menu_ptr);
		}
	}

	void TSU_MenuSubRemoveBanner(DSMenu *dsMenu, Banner *banner_ptr)
	{
		if (dsMenu != NULL && banner_ptr != NULL) {
			dsMenu->removeBanner(banner_ptr);
		}
	}

	void TSU_MenuSubRemoveRectangle(DSMenu *dsMenu, Rectangle *rectangle_ptr)
	{
		if (dsMenu != NULL && rectangle_ptr != NULL) {
			dsMenu->removeRectangle(rectangle_ptr);
		}
	}

	void TSU_MenuSubRemoveBox(DSMenu *dsMenu, Box *box_ptr)
	{
		if (dsMenu != NULL && box_ptr != NULL) {
			dsMenu->removeBox(box_ptr);
		}
	}

	void TSU_MenuSubRemoveLabel(DSMenu *dsMenu, Label *label_ptr)
	{
		if (dsMenu != NULL && label_ptr != NULL) {
			dsMenu->removeLabel(label_ptr);
		}
	}

	void TSU_MenuSubRemoveItemMenu(DSMenu *dsMenu, ItemMenu *item_menu_ptr)
	{
		if (dsMenu != NULL && item_menu_ptr != NULL) {
			dsMenu->removeItemMenu(item_menu_ptr);
		}
	}

	void TSU_MenSetTranslate(DSMenu *dsMenu, const Vector *v)
	{
		if (dsMenu != NULL) {
			dsMenu->getScene()->setTranslate(*v);
		}
	}
}