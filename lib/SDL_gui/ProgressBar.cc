#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_ProgressBar::GUI_ProgressBar(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	SDL_Rect in;

	in.x = 4;
	in.y = 4;
	in.w = area.w-8;
	in.h = area.h-8;
	
	SetTransparent(1);

	value = 0.5;	
	image1 = new GUI_Surface("1", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	image2 = new GUI_Surface("2", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	image1->Fill(NULL, 0x00FFFFFF);
	image1->Fill(&in,  0xFF000000);
	
	image2->Fill(NULL, 0x00FFFFFF);
	image2->Fill(&in,  0x004040FF);
}

GUI_ProgressBar::~GUI_ProgressBar(void)
{
	image1->DecRef();
	image2->DecRef();
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
