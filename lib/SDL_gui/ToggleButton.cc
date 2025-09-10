#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

struct GUI_ToggleButton_Defaults {
	int size;
	GUI_Surface *on_normal;
	GUI_Surface *on_highlight;
	GUI_Surface *off_normal;
	GUI_Surface *off_highlight;
	GUI_ToggleButton_Defaults *next;
};

static GUI_ToggleButton_Defaults *defaults_cache = NULL;

static void on_button_stops_using_defaults(GUI_ToggleButton *button) {
	SDL_Rect area = button->GetArea();
	int box_size = area.h - 4;
	
	GUI_ToggleButton_Defaults *entry = defaults_cache;
	GUI_ToggleButton_Defaults *prev = NULL;

	while(entry) {
		if (entry->size == box_size) {
			if (button->GetOnNormalImage() == entry->on_normal &&
				button->GetOnHighlightImage() == entry->on_highlight &&
				button->GetOffNormalImage() == entry->off_normal &&
				button->GetOffHighlightImage() == entry->off_highlight)
			{
				if (entry->on_normal->GetRef() == 2) {
					if (prev) {
						prev->next = entry->next;
					} else {
						defaults_cache = entry->next;
					}
					entry->on_normal->DecRef();
					entry->on_highlight->DecRef();
					entry->off_normal->DecRef();
					entry->off_highlight->DecRef();
					delete entry;
				}
			}
			break;
		}
		prev = entry;
		entry = entry->next;
	}
}


GUI_ToggleButton::GUI_ToggleButton(const char *aname, int x, int y, int w, int h)
: GUI_AbstractButton(aname, x, y, w, h)
{
	int box_size = h - 4;
	GUI_ToggleButton_Defaults *entry = defaults_cache;
	while (entry) {
		if (entry->size == box_size) {
			break;
		}
		entry = entry->next;
	}

	if (entry) {
		on_normal = entry->on_normal;
		on_highlight = entry->on_highlight;
		off_normal = entry->off_normal;
		off_highlight = entry->off_highlight;
		on_normal->IncRef();
		on_highlight->IncRef();
		off_normal->IncRef();
		off_highlight->IncRef();
		return;
	}

	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;

	on_normal =     new GUI_Surface("on0",  SDL_HWSURFACE, box_size, box_size, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	on_highlight =  new GUI_Surface("on1",  SDL_HWSURFACE, box_size, box_size, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	off_normal =    new GUI_Surface("off0", SDL_HWSURFACE, box_size, box_size, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	off_highlight = new GUI_Surface("off1", SDL_HWSURFACE, box_size, box_size, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	SDL_Rect rect;

	rect = {0, 0, (Uint16)box_size, (Uint16)box_size};
	off_normal->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {2, 2, (Uint16)(box_size - 4), (Uint16)(box_size - 4)};
	off_normal->Fill(&rect, SDL_MapRGB(format, 255, 255, 255));

	rect = {0, 0, (Uint16)box_size, (Uint16)box_size};
	off_highlight->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {2, 2, (Uint16)(box_size - 4), (Uint16)(box_size - 4)};
	off_highlight->Fill(&rect, SDL_MapRGB(format, 212, 241, 41));

	rect = {0, 0, (Uint16)box_size, (Uint16)box_size};
	on_normal->Fill(&rect, SDL_MapRGB(format, 97, 189, 236));
	rect = {2, 2, (Uint16)(box_size - 4), (Uint16)(box_size - 4)};
	on_normal->Fill(&rect, SDL_MapRGB(format, 255, 255, 255));

	if(box_size > 14) {
		rect = {7, 7, (Uint16)(box_size - 14), (Uint16)(box_size - 14)};
		on_normal->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));
	}

	rect = {0, 0, (Uint16)box_size, (Uint16)box_size};
	on_highlight->Fill(&rect, SDL_MapRGB(format, 97, 189, 236));
	rect = {2, 2, (Uint16)(box_size - 4), (Uint16)(box_size - 4)};
	on_highlight->Fill(&rect, SDL_MapRGB(format, 212, 241, 41));

	if(box_size > 14) {
		rect = {7, 7, (Uint16)(box_size - 14), (Uint16)(box_size - 14)};
		on_highlight->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));
	}

	entry = new GUI_ToggleButton_Defaults;
	entry->size = box_size;
	entry->on_normal = on_normal;
	entry->on_highlight = on_highlight;
	entry->off_normal = off_normal;
	entry->off_highlight = off_highlight;
	on_normal->IncRef();
	on_highlight->IncRef();
	off_normal->IncRef();
	off_highlight->IncRef();
	entry->next = defaults_cache;
	defaults_cache = entry;
}

GUI_ToggleButton::~GUI_ToggleButton()
{
	on_button_stops_using_defaults(this);
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
	if (GUI_ObjectKeep((GUI_Object **) &on_normal, surface)) {
		on_button_stops_using_defaults(this);
		MarkChanged();
	}
}

void GUI_ToggleButton::SetOffNormalImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &off_normal, surface)) {
		on_button_stops_using_defaults(this);
		MarkChanged();
	}
}

void GUI_ToggleButton::SetOnHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &on_highlight, surface)) {
		on_button_stops_using_defaults(this);
		MarkChanged();
	}
}

void GUI_ToggleButton::SetOffHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &off_highlight, surface)) {
		on_button_stops_using_defaults(this);
		MarkChanged();
	}
}

GUI_Surface *GUI_ToggleButton::GetOnNormalImage() {
	return on_normal;
}

GUI_Surface *GUI_ToggleButton::GetOnHighlightImage() {
	return on_highlight;
}

GUI_Surface *GUI_ToggleButton::GetOffNormalImage() {
	return off_normal;
}

GUI_Surface *GUI_ToggleButton::GetOffHighlightImage() {
	return off_highlight;
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

GUI_Surface *GUI_ToggleButtonGetOnNormalImage(GUI_Widget *widget)
{
	return ((GUI_ToggleButton *) widget)->GetOnNormalImage();
}

GUI_Surface *GUI_ToggleButtonGetOnHighlightImage(GUI_Widget *widget)
{
	return ((GUI_ToggleButton *) widget)->GetOnHighlightImage();
}

GUI_Surface *GUI_ToggleButtonGetOffNormalImage(GUI_Widget *widget)
{
	return ((GUI_ToggleButton *) widget)->GetOffNormalImage();
}

GUI_Surface *GUI_ToggleButtonGetOffHighlightImage(GUI_Widget *widget)
{
	return ((GUI_ToggleButton *) widget)->GetOffHighlightImage();
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
