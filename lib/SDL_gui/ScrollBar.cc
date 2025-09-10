#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_ScrollBar::GUI_ScrollBar(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	SetTransparent(1);

	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;
	background = NULL;
	knob = new GUI_Surface("knob", SDL_HWSURFACE, w, w * 2, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	position_x = 0;
	position_y = 0;
	tracking_on = 0;
	tracking_start_x = 0;
	tracking_start_y = 0;
	tracking_pos_x = 0;
	tracking_pos_y = 0;

	SDL_Rect rect;

	knob->Fill(NULL, SDL_MapRGB(format, 204, 204, 204));
	rect = {1, 1, (Uint16)(w - 2), (Uint16)(w * 2 - 2)};
	knob->Fill(&rect,  SDL_MapRGB(format, 243, 243, 243));

	CreateBackground();
	moved_callback = 0;
	wtype = WIDGET_TYPE_SCROLLBAR;
}

GUI_ScrollBar::~GUI_ScrollBar(void)
{
	knob->DecRef();
	background->DecRef();
	if (moved_callback) moved_callback->DecRef();
}

void GUI_ScrollBar::CreateBackground(void) {
	if (background) {
		background->DecRef();
	}
	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;
	background = new GUI_Surface("bg", SDL_HWSURFACE, area.w, area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	background->Fill(NULL, SDL_MapRGB(format, 238, 238, 238));
	SDL_Rect rect = {0, 0, 1, (Uint16)area.h};
	background->Fill(&rect,  SDL_MapRGB(format, 204, 204, 204));
}

void GUI_ScrollBar::Update(int force)
{
	if (parent == 0)
		return;
	
	if (force)
	{
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&area);
		
		if (background)
			parent->Draw(background, NULL, &area);
		
		if (knob)
		{
			SDL_Rect sr, dr;
		
			sr.w = dr.w = knob->GetWidth();
			sr.h = dr.h = knob->GetHeight();
			
			dr.x = area.x + position_x;
			dr.y = area.y + position_y;
			sr.x = 0;
			sr.y = 0;
			
			parent->Draw(knob, &sr, &dr);
		}
	}
}

void GUI_ScrollBar::Erase(const SDL_Rect *rp)
{
	SDL_Rect dest;
	
	//assert(parent != NULL);
	//assert(rp != NULL);
	
	if(parent != NULL && rp != NULL) {
	
		dest = Adjust(rp);
		
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&dest);
		if (background)
			parent->TileImage(background, &dest, 0, 0);
	}
}

int GUI_ScrollBar::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	switch (event->type)
	{
		case SDL_MOUSEMOTION:
		{

			//SDL_GetMouseState(&event->motion.x, &event->motion.y);

			int x = event->motion.x - area.x - xoffset;
			int y = event->motion.y - area.y - yoffset;

			if (tracking_on)
			{
				position_x = tracking_pos_x + x - tracking_start_x;
				position_y = tracking_pos_y + y - tracking_start_y;
				if (position_x < 0)	position_x = 0;
				if (position_x > area.w - knob->GetWidth()) position_x = area.w - knob->GetWidth();
				if (position_y < 0)	position_y = 0;
				if (position_y > area.h - knob->GetHeight()) position_y = area.h - knob->GetHeight();
				MarkChanged();
				if (moved_callback) {
					moved_callback->Call(this);
				}
				return 1;
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			if (flags & WIDGET_INSIDE)
			{
				int x = event->button.x - area.x - xoffset;
				int y = event->button.y - area.y - yoffset;

				/* if the cursor is inside the knob, start tracking */
				if (y >= position_y && y < position_y + knob->GetHeight() &&
				    x >= position_x && x < position_x + knob->GetWidth())
				{
					tracking_on = 1;
					tracking_start_x = x;
					tracking_pos_x = position_x;
					tracking_start_y = y;
					tracking_pos_y = position_y;
				}
				return 1;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			if (tracking_on)
			{
				tracking_on = 0;
			}
			break;
		}
	}
	return GUI_Drawable::Event(event, xoffset, yoffset);
}

void GUI_ScrollBar::SetKnobImage(GUI_Surface *image)
{
	if (GUI_ObjectKeep((GUI_Object **) &knob, image))
		MarkChanged();
}

GUI_Surface *GUI_ScrollBar::GetKnobImage()
{
	return knob;
}

void GUI_ScrollBar::SetBackgroundImage(GUI_Surface *image)
{
	if (GUI_ObjectKeep((GUI_Object **) &background, image))
		MarkChanged();
}

int GUI_ScrollBar::GetHorizontalPosition(void)
{
	return position_x;
}

int GUI_ScrollBar::GetVerticalPosition(void)
{
	return position_y;
}

void GUI_ScrollBar::SetHorizontalPosition(int value)
{
	position_x = value;
	MarkChanged();
}

void GUI_ScrollBar::SetVerticalPosition(int value)
{
	position_y = value;
	MarkChanged();
}

void GUI_ScrollBar::SetMovedCallback(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &moved_callback, callback);
}

void GUI_ScrollBar::SetWidth(int w)
{
	if (area.w == w)
		return;

	Resize(w, area.h);
}

void GUI_ScrollBar::SetHeight(int h)
{
	if (area.h == h)
		return;

	Resize(area.w, h);
}

void GUI_ScrollBar::Resize(int w, int h)
{
	if (area.w == w && area.h == h)
		return;

	GUI_Widget::SetSize(w, h);
	CreateBackground();
	MarkChanged();
}

extern "C"
{

GUI_Widget *GUI_ScrollBarCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_ScrollBar(name, x, y, w, h);
}

int GUI_ScrollBarCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_ScrollBarSetKnobImage(GUI_Widget *widget, GUI_Surface *image)
{
	((GUI_ScrollBar *) widget)->SetKnobImage(image);
}

GUI_Surface *GUI_ScrollBarGetKnobImage(GUI_Widget *widget)
{
	return ((GUI_ScrollBar *) widget)->GetKnobImage();
}

void GUI_ScrollBarSetBackgroundImage(GUI_Widget *widget, GUI_Surface *image)
{
	((GUI_ScrollBar *) widget)->SetBackgroundImage(image);
}

void GUI_ScrollBarSetPosition(GUI_Widget *widget, int value)
{
	((GUI_ScrollBar *) widget)->SetVerticalPosition(value);
}


void GUI_ScrollBarSetMovedCallback(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_ScrollBar *) widget)->SetMovedCallback(callback);
}

int GUI_ScrollBarGetPosition(GUI_Widget *widget)
{
	return ((GUI_ScrollBar *) widget)->GetVerticalPosition();
}

int GUI_ScrollBarGetHorizontalPosition(GUI_Widget *widget)
{
	return ((GUI_ScrollBar *) widget)->GetHorizontalPosition();
}

int GUI_ScrollBarGetVerticalPosition(GUI_Widget *widget)
{
	return ((GUI_ScrollBar *) widget)->GetVerticalPosition();
}

void GUI_ScrollBarSetHorizontalPosition(GUI_Widget *widget, int value)
{
	((GUI_ScrollBar *) widget)->SetHorizontalPosition(value);
}

void GUI_ScrollBarSetVerticalPosition(GUI_Widget *widget, int value) 
{
	((GUI_ScrollBar *) widget)->SetVerticalPosition(value);
}

void GUI_ScrollBarResize(GUI_Widget *widget, int w, int h)
{
	((GUI_ScrollBar *) widget)->Resize(w, h);
}

}