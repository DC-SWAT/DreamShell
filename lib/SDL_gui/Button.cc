#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_Button::GUI_Button(const char *aname, int x, int y, int w, int h)
: GUI_AbstractButton(aname, x, y, w, h)
{

	SDL_Rect in;
	
	in.x = 4;
	in.y = 4;
	in.w = area.w-8;
	in.h = area.h-8;
	
	disabled =   new GUI_Surface("disabled", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	normal =     new GUI_Surface("normal", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	highlight =  new GUI_Surface("highlight", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	pressed =    new GUI_Surface("pressed", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	disabled->Fill(NULL, 0xFF000000);
	
	normal->Fill(NULL, 0xFF000000);

	highlight->Fill(NULL, 0x00FFFFFF);
	highlight->Fill(&in, 0xFF000000);

	pressed->Fill(NULL, 0x00FFFFFF);
	pressed->Fill(&in, 0x005050C0);
}

GUI_Button::~GUI_Button()
{
	normal->DecRef();
	highlight->DecRef();
	pressed->DecRef();
	disabled->DecRef();
}

GUI_Surface *GUI_Button::GetCurrentImage()
{
	if (flags & WIDGET_DISABLED)
		return disabled;
	
	if (flags & WIDGET_INSIDE)
	{
		if (flags & WIDGET_PRESSED)
			return pressed;
		return highlight;
	}
	return normal;
}

void GUI_Button::SetNormalImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &normal, surface))
		MarkChanged();
}

void GUI_Button::SetHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &highlight, surface))
		MarkChanged();
}

void GUI_Button::SetPressedImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &pressed, surface))
		MarkChanged();
}

void GUI_Button::SetDisabledImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &disabled, surface))
		MarkChanged();
}

extern "C"
{

GUI_Widget *GUI_ButtonCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_Button(name, x, y, w, h);
}

int GUI_ButtonCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_ButtonSetNormalImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Button *) widget)->SetNormalImage(surface);
}

void GUI_ButtonSetHighlightImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Button *) widget)->SetHighlightImage(surface);
}

void GUI_ButtonSetPressedImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Button *) widget)->SetPressedImage(surface);
}

void GUI_ButtonSetDisabledImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Button *) widget)->SetDisabledImage(surface);
}

void GUI_ButtonSetCaption(GUI_Widget *widget, GUI_Widget *caption)
{
	((GUI_Button *) widget)->SetCaption(caption);
}

void GUI_ButtonSetCaption2(GUI_Widget *widget, GUI_Widget *caption)
{
	((GUI_Button *) widget)->SetCaption2(caption);
}

GUI_Widget *GUI_ButtonGetCaption(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetCaption();
}

GUI_Widget *GUI_ButtonGetCaption2(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetCaption();
}

void GUI_ButtonSetClick(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_Button *) widget)->SetClick(callback);
}

void GUI_ButtonSetContextClick(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_Button *) widget)->SetContextClick(callback);
}

void GUI_ButtonSetMouseover(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_Button *) widget)->SetMouseover(callback);
}

void GUI_ButtonSetMouseout(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_Button *) widget)->SetMouseout(callback);
}

}
