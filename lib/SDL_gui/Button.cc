#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

struct GUI_Button_Defaults {
	int w, h;
	GUI_Surface *normal;
	GUI_Surface *highlight;
	GUI_Surface *pressed;
	GUI_Surface *disabled;
	GUI_Button_Defaults *next;
};

static GUI_Button_Defaults *defaults_cache = NULL;

static void on_button_stops_using_defaults(GUI_Button *button) {
	SDL_Rect area = button->GetArea();
	int w = area.w;
	int h = area.h;
	
	GUI_Button_Defaults *entry = defaults_cache;
	GUI_Button_Defaults *prev = NULL;

	while(entry) {
		if (entry->w == w && entry->h == h) {
			if (button->GetNormalImage() == entry->normal &&
				button->GetHighlightImage() == entry->highlight &&
				button->GetPressedImage() == entry->pressed &&
				button->GetDisabledImage() == entry->disabled)
			{
				if (entry->normal->GetRef() == 2) {
					if (prev) {
						prev->next = entry->next;
					} else {
						defaults_cache = entry->next;
					}
					entry->normal->DecRef();
					entry->highlight->DecRef();
					entry->pressed->DecRef();
					entry->disabled->DecRef();
					delete entry;
				}
			}
			break;
		}
		prev = entry;
		entry = entry->next;
	}
}

GUI_Button::GUI_Button(const char *aname, int x, int y, int w, int h)
: GUI_AbstractButton(aname, x, y, w, h)
{
	GUI_Button_Defaults *entry = defaults_cache;
	while (entry) {
		if (entry->w == w && entry->h == h) {
			break;
		}
		entry = entry->next;
	}

	if (entry) {
		disabled = entry->disabled;
		normal = entry->normal;
		highlight = entry->highlight;
		pressed = entry->pressed;
		disabled->IncRef();
		normal->IncRef();
		highlight->IncRef();
		pressed->IncRef();
		return;
	}

	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;

	disabled =   new GUI_Surface("disabled", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	normal =     new GUI_Surface("normal", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	highlight =  new GUI_Surface("highlight", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	pressed =    new GUI_Surface("pressed", SDL_HWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	SDL_Rect rect;
	int B;

	if(h <= 30) {
		B = 2;
	}
	else if(h >= 60) {
		B = 5;
	}
	else {
		B = (int)(2 + (h - 30.0) / 10.0 + 0.5);
	}

	const int b1 = B - 1;
	const int b2 = B;

	rect = {0, 0, (Uint16)w, (Uint16)h};
	normal->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {(Sint16)b1, (Sint16)b1, (Uint16)(w - b1 * 2), (Uint16)(h - b1 * 2)};
	normal->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {(Sint16)b2, (Sint16)b2, (Uint16)(w - b2 * 2), (Uint16)(h - b2 * 2)};
	normal->Fill(&rect, SDL_MapRGB(format, 49, 121, 159));

	rect = {0, 0, (Uint16)w, (Uint16)h};
	highlight->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {(Sint16)b1, (Sint16)b1, (Uint16)(w - b1 * 2), (Uint16)(h - b1 * 2)};
	highlight->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {(Sint16)b2, (Sint16)b2, (Uint16)(w - b2 * 2), (Uint16)(h - b2 * 2)};
	highlight->Fill(&rect, SDL_MapRGB(format, 97, 189, 236));

	rect = {0, 0, (Uint16)w, (Uint16)h};
	pressed->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {(Sint16)b1, (Sint16)b1, (Uint16)(w - b1 * 2), (Uint16)(h - b1 * 2)};
	pressed->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {(Sint16)b2, (Sint16)b2, (Uint16)(w - b2 * 2), (Uint16)(h - b2 * 2)};
	pressed->Fill(&rect, SDL_MapRGB(format, 212, 241, 21));

	rect = {0, 0, (Uint16)w, (Uint16)h};
	disabled->Fill(&rect, SDL_MapRGB(format, 238, 238, 238));
	rect = {(Sint16)b1, (Sint16)b1, (Uint16)(w - b1 * 2), (Uint16)(h - b1 * 2)};
	disabled->Fill(&rect, SDL_MapRGB(format, 187, 187, 187));
	rect = {(Sint16)b2, (Sint16)b2, (Uint16)(w - b2 * 2), (Uint16)(h - b2 * 2)};
	disabled->Fill(&rect, SDL_MapRGB(format, 204, 204, 204));

	entry = new GUI_Button_Defaults;
	entry->w = w;
	entry->h = h;
	entry->disabled = disabled;
	entry->normal = normal;
	entry->highlight = highlight;
	entry->pressed = pressed;
	
	disabled->IncRef();
	normal->IncRef();
	highlight->IncRef();
	pressed->IncRef();

	entry->next = defaults_cache;
	defaults_cache = entry;
}

GUI_Button::~GUI_Button()
{
	on_button_stops_using_defaults(this);
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
	on_button_stops_using_defaults(this);
	if (GUI_ObjectKeep((GUI_Object **) &normal, surface)) {
		MarkChanged();
	}
}

void GUI_Button::SetHighlightImage(GUI_Surface *surface)
{
	on_button_stops_using_defaults(this);
	if (GUI_ObjectKeep((GUI_Object **) &highlight, surface)) {
		MarkChanged();
	}
}

void GUI_Button::SetPressedImage(GUI_Surface *surface)
{
	on_button_stops_using_defaults(this);
	if (GUI_ObjectKeep((GUI_Object **) &pressed, surface)) {
		MarkChanged();
	}
}

void GUI_Button::SetDisabledImage(GUI_Surface *surface)
{
	on_button_stops_using_defaults(this);
	if (GUI_ObjectKeep((GUI_Object **) &disabled, surface)) {
		MarkChanged();
	}
}

GUI_Surface *GUI_Button::GetNormalImage()
{
	return normal;
}

GUI_Surface *GUI_Button::GetHighlightImage()
{
	return highlight;
}

GUI_Surface *GUI_Button::GetPressedImage()
{
	return pressed;
}

GUI_Surface *GUI_Button::GetDisabledImage()
{
	return disabled;
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

GUI_Surface *GUI_ButtonGetNormalImage(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetNormalImage();
}

GUI_Surface *GUI_ButtonGetHighlightImage(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetHighlightImage();
}

GUI_Surface *GUI_ButtonGetPressedImage(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetPressedImage();
}

GUI_Surface *GUI_ButtonGetDisabledImage(GUI_Widget *widget)
{
	return ((GUI_Button *) widget)->GetDisabledImage();
}

}
