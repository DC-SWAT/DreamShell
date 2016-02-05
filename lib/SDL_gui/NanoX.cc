
#include <kos.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "SDL_gui.h"
#include "nano-X.h"
#include "nanowm.h"

extern "C" {
	
	char *GetWorkPath();
	SDL_Surface *GetScreen();
	void SetNanoXParams(SDL_Surface *surface, int w, int h, int bpp);
	void GetNanoXParams(SDL_Surface **surface, int *w, int *h, int *bpp);
	void SetNanoXEvent(SDL_Event *event);
}


GUI_NANOX::GUI_NANOX(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{

	SDL_Surface *screen = GetScreen();
	surface = GUI_SurfaceCreate("nanox_render", 
							screen->flags, 
							area.w, area.h, 
							screen->format->BitsPerPixel,
							screen->format->Rmask, 
							screen->format->Gmask, 
							screen->format->Bmask, 
							screen->format->Amask);
							
	SetNanoXParams(surface->GetSurface(), w, h, screen->format->BitsPerPixel);
	event_handler = NULL;
	
	if (GrOpen() < 0) {
		GUI_Exception("DS_ERROR: Cannot open graphics for NanoX\n");
	}
}


GUI_NANOX::~GUI_NANOX()
{
	GrClose();
	SDL_Surface *screen = GetScreen();
	SetNanoXParams(screen, screen->w, screen->h, screen->format->BitsPerPixel);
	surface->DecRef();
	surface = NULL;
	if (event_handler) event_handler->DecRef();
	event_handler = NULL;
}


void GUI_NANOX::DrawWidget(const SDL_Rect *clip) {
	
	if (parent == 0)
		return;
	
	if (surface) {
		
		SDL_Rect dr;
		SDL_Rect sr;
			
		sr.w = dr.w = surface->GetWidth();
		sr.h = dr.h = surface->GetHeight();
		sr.x = sr.y = 0;
		
		dr.x = area.x;
		dr.y = area.y + (area.h - dr.h) / 2;
		
		//if (GUI_ClipRect(&sr, &dr, clip))
			parent->Draw(surface, &sr, &dr);
	}
}

void GUI_NANOX::SetGEventHandler(GUI_Callback *handler) {
	GUI_ObjectKeep((GUI_Object **) &event_handler, handler);
}


int GUI_NANOX::Event(const SDL_Event *event, int xoffset, int yoffset) {
	
		SetNanoXEvent((SDL_Event *)event);
		//GR_EVENT gevent;
		//GrGetNextEvent(&gevent);
		
		if(event_handler != NULL) {
			//printf("GUI_NANOX::Event: %d\n", event->type);
			event_handler->Call(this);
		}
		/*
		switch (gevent.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				MarkChanged();
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				//GrClose();
				break;
		}*/
		return 0;
}

extern "C"
{

	GUI_Widget *GUI_NANOX_Create(const char *name, int x, int y, int w, int h) {
		return new GUI_NANOX(name, x, y, w, h);
	}
	
	void GUI_NANOX_SetGEventHandler(GUI_Widget *widget, GUI_Callback *handler) {
		((GUI_NANOX *) widget)->SetGEventHandler(handler);
	}
}
