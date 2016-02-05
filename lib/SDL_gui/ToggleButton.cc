#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_ToggleButton::GUI_ToggleButton(const char *aname, int x, int y, int w, int h)
: GUI_AbstractButton(aname, x, y, w, h)
{
	SDL_Rect in;
	
	in.x = 4;
	in.y = 4;
	in.w = area.w-8;
	in.h = area.h-8;
	
	on_normal =     new GUI_Surface("on0", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	on_highlight =  new GUI_Surface("on1", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	off_normal =    new GUI_Surface("off0", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	off_highlight = new GUI_Surface("off1", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	off_normal->Fill(NULL, 0xFF000000);
	off_normal->Fill(&in,  0x007F0000);

	off_highlight->Fill(NULL, 0x00FFFFFF);
	off_highlight->Fill(&in, 0x007F0000);

	on_normal->Fill(NULL, 0xFF000000);
	on_normal->Fill(&in,  0x00007F00);

	on_highlight->Fill(NULL, 0x00FFFFFF);
	on_highlight->Fill(&in, 0x00007F00);
}

GUI_ToggleButton::~GUI_ToggleButton()
{
	off_normal->DecRef();
	off_highlight->DecRef();
	on_normal->DecRef();
	on_highlight->DecRef();
}

void GUI_ToggleButton::Clicked(int x, int y)
{
	flags ^= WIDGET_TURNED_ON;
	MarkChanged();
	GUI_AbstractButton::Clicked(x,y);
}

void GUI_ToggleButton::Highlighted(int x, int y)
{
	GUI_AbstractButton::Highlighted(x,y);
}

void GUI_ToggleButton::unHighlighted(int x, int y)
{
	GUI_AbstractButton::unHighlighted(x,y);
}


GUI_Surface *GUI_ToggleButton::GetCurrentImage()
{
	if (flags & WIDGET_INSIDE)
	{
		if (flags & WIDGET_TURNED_ON)
			return on_highlight;
		return off_highlight;
	}
	if (flags & WIDGET_TURNED_ON)
		return  on_normal;
	return off_normal;
}

void GUI_ToggleButton::SetOnNormalImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &on_normal, surface))
		MarkChanged();
}

void GUI_ToggleButton::SetOffNormalImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &off_normal, surface))
		MarkChanged();
}

void GUI_ToggleButton::SetOnHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &on_highlight, surface))
		MarkChanged();
}

void GUI_ToggleButton::SetOffHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &off_highlight, surface))
		MarkChanged();
}

extern "C"
{

GUI_Widget *GUI_ToggleButtonCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_ToggleButton(name, x, y, w, h);
}

int GUI_ToggleButtonCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_ToggleButtonSetOnNormalImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_ToggleButton *) widget)->SetOnNormalImage(surface);
}

void GUI_ToggleButtonSetOnHighlightImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_ToggleButton *) widget)->SetOnHighlightImage(surface);
}

void GUI_ToggleButtonSetOffNormalImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_ToggleButton *) widget)->SetOffNormalImage(surface);
}

void GUI_ToggleButtonSetOffHighlightImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_ToggleButton *) widget)->SetOffHighlightImage(surface);
}

void GUI_ToggleButtonSetCaption(GUI_Widget *widget, GUI_Widget *caption)
{
	((GUI_ToggleButton *) widget)->SetCaption(caption);
}

GUI_Widget *GUI_ToggleButtonGetCaption(GUI_Widget *widget)
{
	return ((GUI_ToggleButton *) widget)->GetCaption();
}

void GUI_ToggleButtonSetClick(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_ToggleButton *) widget)->SetClick(callback);
}

void GUI_ToggleButtonSetContextClick(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_ToggleButton *) widget)->SetContextClick(callback);
}

void GUI_ToggleButtonSetMouseover(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_ToggleButton *) widget)->SetMouseover(callback);
}

void GUI_ToggleButtonSetMouseout(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_ToggleButton *) widget)->SetMouseout(callback);
}

}
