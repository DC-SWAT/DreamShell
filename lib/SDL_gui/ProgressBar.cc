#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_ProgressBar::GUI_ProgressBar(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	SetTransparent(1);

	value = 0.5;	
	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;
	image1 = new GUI_Surface("1", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	image2 = new GUI_Surface("2", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	SDL_Rect rect;

	rect = {0, 0, (Uint16)w, (Uint16)h};
	image1->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {1, 1, (Uint16)(w - 2), (Uint16)(h - 2)};
	image1->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {2, 2, (Uint16)(w - 4), (Uint16)(h - 4)};
	image1->Fill(&rect, SDL_MapRGB(format, 217, 217, 217));

	rect = {0, 0, (Uint16)w, (Uint16)h};
	image2->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {1, 1, (Uint16)(w - 2), (Uint16)(h - 2)};
	image2->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {2, 2, (Uint16)(w - 4), (Uint16)(h - 4)};
	image2->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));
}

GUI_ProgressBar::~GUI_ProgressBar(void)
{
	if (image1) image1->DecRef();
	if (image2) image2->DecRef();
}

void GUI_ProgressBar::Update(int force)
{
	if (parent == 0)
		return;
	
	if (force)
	{
		int x = (int) (area.w * value);
		
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&area);
		if (image1)
		{
			SDL_Rect sr, dr;
		
			sr.w = dr.w = image1->GetWidth() - x;
			sr.h = dr.h = image1->GetHeight();
			dr.x = area.x + x;
			dr.y = area.y;
			sr.x = x;
			sr.y = 0;
			
			parent->Draw(image1, &sr, &dr);
		}
		x = area.w - x;
		if (image2)
		{
			SDL_Rect sr, dr;
		
			sr.w = dr.w = image2->GetWidth() - x;
			sr.h = dr.h = image2->GetHeight();
			dr.x = area.x;
			dr.y = area.y;
			sr.x = 0;
			sr.y = 0;
			
			parent->Draw(image2, &sr, &dr);
		}
	}
}

void GUI_ProgressBar::SetImage1(GUI_Surface *image)
{
	if (GUI_ObjectKeep((GUI_Object **) &image1, image))
		MarkChanged();
}

void GUI_ProgressBar::SetImage2(GUI_Surface *image)
{
	if (GUI_ObjectKeep((GUI_Object **) &image2, image))
		MarkChanged();
}

void GUI_ProgressBar::SetPosition(double a_value)
{
	if (a_value != value)
	{
		value = a_value;
		MarkChanged();
	}
}

extern "C"
{

GUI_Widget *GUI_ProgressBarCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_ProgressBar(name, x, y, w, h);
}

int GUI_ProgressBarCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_ProgressBarSetImage1(GUI_Widget *widget, GUI_Surface *image)
{
	((GUI_ProgressBar *) widget)->SetImage1(image);
}

void GUI_ProgressBarSetImage2(GUI_Widget *widget, GUI_Surface *image)
{
	((GUI_ProgressBar *) widget)->SetImage2(image);
}

void GUI_ProgressBarSetPosition(GUI_Widget *widget, double value)
{
	((GUI_ProgressBar *) widget)->SetPosition(value);
}

}
