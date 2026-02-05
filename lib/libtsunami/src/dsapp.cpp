/*
   Tsunami for KallistiOS ##version##

   dsapp.cpp

   Copyright (C) 2024 Maniac Vera
   Copyright (C) 2026 SWAT

*/

#include "dsapp.h"
#include <kos.h>
#include "tsunamiutils.h"

DSApp::DSApp(InputEventPtr *input_event_callback)
{
    InputEventCallback = input_event_callback;
	DrawOpaquePolyEventCallback = nullptr;
	DrawTransparentPolyEventCallback = nullptr;
}

DSApp::~DSApp()
{   
    InputEventCallback = nullptr;
	DrawOpaquePolyEventCallback = nullptr;
	DrawTransparentPolyEventCallback = nullptr;
}

void DSApp::inputEvent(const Event & evt)
{
    if (InputEventCallback != NULL) {
        InputEventCallback(evt.type, evt.key);
    }
}

void DSApp::setDrawOpaquePolyEvent(DrawOpaquePolyEventPtr event_callback_ptr)
{
	DrawOpaquePolyEventCallback = event_callback_ptr;
}

void DSApp::setDrawTransparentPolyEvent(DrawTransparentPolyEventPtr event_callback_ptr)
{
	DrawTransparentPolyEventCallback = event_callback_ptr;
}

void DSApp::beginApp()
{
	// Reset our timeout
	resetTimeout();

	// Enter the main loop
	m_exiting = false;
}

void DSApp::doAppControl()
{
	controlPerFrame();
}

void DSApp::doAppFrame()
{
	if (!m_exiting) {		
		visualPerFrame();
	}
}

void DSApp::visualPerFrame()
{
	pvr_list_begin(PLX_LIST_OP_POLY);
	visualOpaqueList();

	if (DrawOpaquePolyEventCallback != nullptr) {
		DrawOpaquePolyEventCallback();
	}

	pvr_list_begin(PLX_LIST_TR_POLY);
	visualTransList();
	
	if (DrawTransparentPolyEventCallback != nullptr) {
		DrawTransparentPolyEventCallback();
	}

	m_scene->nextFrame();
}

void DSApp::doMenu() {}

bool DSApp::endApp()
{
	// Ok, we're exiting -- do the same as before, but we'll exit out
	// entirely when the scene is finished (and music faded).
	if (!m_scene->isFinished() && m_exitCount > 0.0f) {
		m_exitCount -= m_exitSpeed;
		if (m_exitCount < 0.0f)
			m_exitCount = 0.0f;

		visualPerFrame();

		return false;
	}

	// Did we get a quit-now request?
	if (!m_exitSpeed)
		return true;

	// We should be faded out now -- do three more frames to finish
	// clearing out the PVR buffered scenes.
	m_exitCount = 0;
	GenericMenu::visualPerFrame();
	GenericMenu::visualPerFrame();
	GenericMenu::visualPerFrame();

	if (m_postDelay)
		thd_sleep(m_postDelay);

	return true;
}

void DSApp::setBg(float r, float g, float b)
{
    GenericMenu::setBg(r, g, b);
}

Scene* DSApp::getScene()
{
    return m_scene;
}

void DSApp::addItemMenu(ItemMenu *item_menu)
{
    m_scene->subAdd(item_menu);
}

void DSApp::addLabel(Label *label)
{
    m_scene->subAdd(label);
}

void DSApp::addBanner(Banner *banner)
{
    m_scene->subAdd(banner);
}

void DSApp::addCircle(Circle *circle)
{
    m_scene->subAdd(circle);
}

void DSApp::addWave(Wave *wave)
{
    m_scene->subAdd(wave);
}

void DSApp::addRectangle(Rectangle *rectangle)
{
    m_scene->subAdd(rectangle);
}

void DSApp::addGradient(Gradient *gradient)
{
    m_scene->subAdd(gradient);
}

void DSApp::addBox(Box *box)
{
    m_scene->subAdd(box);
}

void DSApp::removeItemMenu(ItemMenu *item_menu)
{
    m_scene->subRemove(item_menu);
}

void DSApp::removeLabel(Label *label)
{
    m_scene->subRemove(label);
}

void DSApp::removeBanner(Banner *banner)
{
    m_scene->subRemove(banner);
}

void DSApp::removeCircle(Circle *circle)
{
    m_scene->subRemove(circle);
}

void DSApp::removeWave(Wave *wave)
{
    m_scene->subRemove(wave);
}

void DSApp::removeRectangle(Rectangle *rectangle)
{
    m_scene->subRemove(rectangle);
}

void DSApp::removeGradient(Gradient *gradient)
{
    m_scene->subRemove(gradient);
}

void DSApp::removeBox(Box *box)
{
    m_scene->subRemove(box);
}

void DSApp::StartExitApp()
{
    GenericMenu::startExit();
}

extern "C" 
{

	DSApp* TSU_AppCreate(InputEventPtr *input_event_callback)
	{
		return new DSApp(input_event_callback);
	}

	void TSU_AppBegin(DSApp *dsApp)
	{
		if (dsApp != nullptr) {          
			dsApp->beginApp();
		}
	}

	void TSU_AppDoControl(DSApp *dsApp)
	{
		if (dsApp != nullptr) {          
			dsApp->doAppControl();
		}
	}

	void TSU_AppDoFrame(DSApp *dsApp)
	{
		if (dsApp != nullptr) {          
			dsApp->doAppFrame();
		}
	}

	void TSU_AppSetDrawOpaquePolyEvent(DSApp *dsApp, DrawOpaquePolyEventPtr *event_callback)
	{
		if (dsApp != nullptr && event_callback != nullptr) {
			dsApp->setDrawOpaquePolyEvent(event_callback);
		}
	}

	void TSU_AppSetDrawTransparentPolyEvent(DSApp *dsApp, DrawTransparentPolyEventPtr *event_callback)
	{
		if (dsApp != nullptr && event_callback != nullptr) {
			dsApp->setDrawTransparentPolyEvent(event_callback);
		}
	}

	bool TSU_AppEnd(DSApp *dsApp)
	{
		if (dsApp != nullptr) {          
			return dsApp->endApp();
		}

		return true;
	}

	void TSU_AppDestroy(DSApp **dsApp)
	{
		if (*dsApp != nullptr) {
			delete *dsApp;
			*dsApp = nullptr;
		}
	}

	void TSU_AppSetBg(DSApp *dsApp, float r, float g, float b)
	{
		if (dsApp != nullptr) {
			dsApp->setBg(r, g, b);
		}
	}

	void TSU_AppStartExit(DSApp *dsApp)
	{
		if (dsApp != nullptr) {
			dsApp->StartExitApp();
		}         
	}

	Scene* TSU_AppGetScene(DSApp *dsApp)
	{
		if (dsApp != nullptr) {          
			return dsApp->getScene();
		}   
		else {
			return NULL;
		}     
	}

	void TSU_AppSubAddLabel(DSApp *dsApp, Label *label_ptr)
	{
		if (dsApp != NULL && label_ptr != NULL) {
			dsApp->addLabel(label_ptr);
		}
	}

	void TSU_AppSubAddBanner(DSApp *dsApp, Banner *banner_ptr)
	{
		if (dsApp != NULL && banner_ptr != NULL) {
			dsApp->addBanner(banner_ptr);
		}
	} 

	void TSU_AppSubAddCircle(DSApp *dsApp, Circle *circle_ptr)
	{
		if (dsApp != NULL && circle_ptr != NULL) {
			dsApp->addCircle(circle_ptr);
		}
	}

	void TSU_AppSubAddWave(DSApp *dsApp, Wave *wave_ptr)
	{
		if (dsApp != NULL && wave_ptr != NULL) {
			dsApp->addWave(wave_ptr);
		}
	}

	void TSU_AppSubAddRectangle(DSApp *dsApp, Rectangle *rectangle_ptr)
	{
		if (dsApp != NULL && rectangle_ptr != NULL) {
			dsApp->addRectangle(rectangle_ptr);
		}
	}

    void TSU_AppSubAddGradient(DSApp *dsApp, Gradient *gradient_ptr)
    {
        if (dsApp != NULL && gradient_ptr != NULL) {
            dsApp->addGradient(gradient_ptr);
        }
    }

	void TSU_AppSubAddBox(DSApp *dsApp, Box *box_ptr)
	{
		if (dsApp != NULL && box_ptr != NULL) {
			dsApp->addBox(box_ptr);
		}
	}  

	void TSU_AppSubAddItemMenu(DSApp *dsApp, ItemMenu *item_menu_ptr)
	{
		if (dsApp != NULL && item_menu_ptr != NULL) {
			dsApp->addItemMenu(item_menu_ptr);
		}
	}

	void TSU_AppSubRemoveBanner(DSApp *dsApp, Banner *banner_ptr)
	{
		if (dsApp != NULL && banner_ptr != NULL) {
			dsApp->removeBanner(banner_ptr);
		}
	}

	void TSU_AppSubRemoveCircle(DSApp *dsApp, Circle *circle_ptr)
	{
		if (dsApp != NULL && circle_ptr != NULL) {
			dsApp->removeCircle(circle_ptr);
		}
	}

	void TSU_AppSubRemoveWave(DSApp *dsApp, Wave *wave_ptr)
	{
		if (dsApp != NULL && wave_ptr != NULL) {
			dsApp->removeWave(wave_ptr);
		}
	}

	void TSU_AppSubRemoveRectangle(DSApp *dsApp, Rectangle *rectangle_ptr)
	{
		if (dsApp != NULL && rectangle_ptr != NULL) {
			dsApp->removeRectangle(rectangle_ptr);
		}
	}

    void TSU_AppSubRemoveGradient(DSApp *dsApp, Gradient *gradient_ptr)
    {
        if (dsApp != NULL && gradient_ptr != NULL) {
            dsApp->removeGradient(gradient_ptr);
        }
    }

	void TSU_AppSubRemoveBox(DSApp *dsApp, Box *box_ptr)
	{
		if (dsApp != NULL && box_ptr != NULL) {
			dsApp->removeBox(box_ptr);
		}
	}

	void TSU_AppSubRemoveLabel(DSApp *dsApp, Label *label_ptr)
	{
		if (dsApp != NULL && label_ptr != NULL) {
			dsApp->removeLabel(label_ptr);
		}
	}

	void TSU_AppSubRemoveItemMenu(DSApp *dsApp, ItemMenu *item_menu_ptr)
	{
		if (dsApp != NULL && item_menu_ptr != NULL) {
			dsApp->removeItemMenu(item_menu_ptr);
		}
	}

	void TSU_AppSetTranslate(DSApp *dsApp, const Vector *v)
	{
		if (dsApp != NULL) {
			dsApp->getScene()->setTranslate(*v);
		}
	}
}