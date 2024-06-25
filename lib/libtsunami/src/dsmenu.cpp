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

void DSMenu::StartExitMenu()
{
    GenericMenu::startExit();
}

// void DSMenu::DrawSquare(int x, int y, int size, uint32 color, int zIndex = 1)
// {
//     DrawRectangle(x, y, size, size, color, zIndex);
// }

// void DSMenu::DrawRectangle(int x, int y, int width, int height, uint32 color, int zIndex = 1)
// {
//     pvr_vertex_t vert;

//     vert.flags = PVR_CMD_VERTEX;
//     vert.x = x;
//     vert.y = y;
//     vert.z = 1.0f;
//     vert.u = vert.v = 0.0f;
//     // vert.argb = PVR_PACK_COLOR(1.0f, color / 255.0f, color / 255.0f, color / 255.0f);
//     vert.argb = color;
//     vert.oargb = 0;
//     pvr_prim(&vert, sizeof(vert));
    
//     vert.x = x;
//     vert.y = y - height;
//     pvr_prim(&vert, sizeof(vert));

//     vert.x = x + width;
//     vert.y = y;
//     pvr_prim(&vert, sizeof(vert));

//     vert.flags = PVR_CMD_VERTEX_EOL;
//     vert.x = x + width;
//     vert.y = y - height;
//     pvr_prim(&vert, sizeof(vert));
// }

extern "C" 
{

	DSMenu* TSU_MenuCreate(InputEventPtr *input_event_callback)
	{
		// // Guard against an untoward exit during testing.
		// cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y, (cont_btn_callback_t)arch_exit);

		// // Get 3D going
		// pvr_init_defaults();

		return new DSMenu(input_event_callback);
	}

	DSMenu* TSU_MenuCreateWithExit(InputEventPtr *input_event_callback, ExitDoMenuEventPtr *exit_do_menu_callback)
	{
		// cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y, (cont_btn_callback_t)arch_exit);
		return new DSMenu(input_event_callback, exit_do_menu_callback);
	}

	void TSU_MenuDoMenu(DSMenu *dsMenu)
	{
		if (dsMenu != nullptr) {          
			dsMenu->doMenu();
		}
	}

	void TSU_MenuDestroy(DSMenu *dsMenu)
	{
		if (dsMenu != nullptr) {
			delete dsMenu;
			dsMenu = nullptr;
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
			delete banner_ptr;
			banner_ptr = NULL;
		}
	}

	void TSU_MenuSubRemoveLabel(DSMenu *dsMenu, Label *label_ptr)
	{
		if (dsMenu != NULL && label_ptr != NULL) {
			dsMenu->removeLabel(label_ptr);
			delete label_ptr;
			label_ptr = NULL;
		}
	}

	void TSU_MenuSubRemoveItemMenu(DSMenu *dsMenu, ItemMenu *item_menu_ptr)
	{
		if (dsMenu != NULL && item_menu_ptr != NULL) {
			dsMenu->removeItemMenu(item_menu_ptr);
			delete item_menu_ptr;
			item_menu_ptr = NULL;
		}
	}

	void TSU_MenSetTranslate(DSMenu *dsMenu, const Vector *v)
	{
		if (dsMenu != NULL) {
			dsMenu->getScene()->setTranslate(*v);
		}
	}

	// auto scene_shared = dsMenu->getScene();

	// // CONVERTIR shared_ptr to void*
	// void *scene_ptr = (void *)(&scene_shared);
	// TSU_SendMessage(((Drawable *)scene_shared.get())->dummy);
		
	// // CONVERTIR void* to shared_ptr
	// auto scene_shared_aux = *(std::shared_ptr<Drawable>*)(scene_ptr);
	// TSU_SendMessage(scene_shared_aux->dummy);

	// // CONVERTIR shared_ptr to herencia
	// auto scene_shared_aux = std::dynamic_pointer_cast<Drawable>(scene_shared);        

	// TSU_SendMessage("OK");
	// strcpy(scene_shared_aux->dummy, "MUNDO!");
	// auto scene_shared_aux_2 = *(std::shared_ptr<Scene>*)(scene_ptr);
	// TSU_SendMessage(scene_shared_aux_2->dummy);    
}