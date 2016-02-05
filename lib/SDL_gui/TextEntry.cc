
#include <kos.h>
#include <assert.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_TextEntry::GUI_TextEntry(const char *aname, int x, int y, int w, int h, GUI_Font *afont, int size)
: GUI_Widget(aname, x, y, w, h), font(afont)
{
	SDL_Rect in;

	in.x = 4;
	in.y = 4;
	in.w = area.w-8;
	in.h = area.h-8;
	
	SetTransparent(1);
	
	normal_image =     new GUI_Surface("normal", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	highlight_image =  new GUI_Surface("highlight", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	focus_image =      new GUI_Surface("focus", SDL_HWSURFACE, w, h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	textcolor.r = 255;
	textcolor.g = 255;
	textcolor.b = 255;
	textcolor.unused = 255;

	font->IncRef();
	
	buffer_size = size;
	buffer_index = 0;
	buffer = new char[size+1];
	strcpy(buffer, "");

	normal_image->Fill(NULL, 0xFF000000);

	highlight_image->Fill(NULL, 0x00FFFFFF);
	highlight_image->Fill(&in, 0xFF000000);

	focus_image->Fill(NULL, 0x00FFFFFF);
	focus_image->Fill(&in, 0x005050C0);
	focus_callback = 0;
	unfocus_callback = 0;
	
	wtype = WIDGET_TYPE_TEXTENTRY;
}

GUI_TextEntry::~GUI_TextEntry()
{
	font->DecRef();
	normal_image->DecRef();
	highlight_image->DecRef();
	focus_image->DecRef();
	if (focus_callback) focus_callback->DecRef();
	if (unfocus_callback) unfocus_callback->DecRef();
	delete [] buffer;
}

void GUI_TextEntry::Update(int force)
{
	if (parent==0)
		return;
	
	if (force)
	{
		GUI_Surface *surface;
		
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&area);
		
		/* draw the background */
		if (flags & WIDGET_HAS_FOCUS)
		{
			surface = focus_image;
		}
		else
		{
			if (flags & WIDGET_INSIDE)
				surface = highlight_image;
			else
				surface = normal_image;
		}

		if (surface)
			parent->Draw(surface, NULL, &area);
		
		surface = font->RenderQuality(buffer, textcolor);
		if (surface != NULL)
		{			
			SDL_Rect dr;
			SDL_Rect sr;
			SDL_Rect clip = area;
			
			sr.w = dr.w = surface->GetWidth();
			sr.h = dr.h = surface->GetHeight();
			sr.x = sr.y = 0;
			
			dr.x = area.x + 4;
			dr.y = area.y + (area.h - (dr.h-4)) / 2;
		
			if (GUI_ClipRect(&sr, &dr, &clip))
				parent->Draw(surface, &sr, &dr);
		
			surface->DecRef();
		}
	}
}

void GUI_TextEntry::Clicked(int x, int y)
{
	GUI_Screen *screen = GUI_GetScreen();
	
	if (flags & WIDGET_HAS_FOCUS)
	{
		screen->ClearFocusWidget();
		if (unfocus_callback)
			unfocus_callback->Call(this);
	}
	else
	{
		if (focus_callback)
			focus_callback->Call(this);
		screen->SetFocusWidget(this);
	}
	MarkChanged();
}

void GUI_TextEntry::Highlighted(int x, int y)
{

}

void GUI_TextEntry::unHighlighted(int x, int y)
{

}

int GUI_TextEntry::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	if (event->type == SDL_KEYDOWN && flags & WIDGET_HAS_FOCUS)
	{
		int key = event->key.keysym.sym;
		int ch = event->key.keysym.unicode;
		
		if (key == SDLK_BACKSPACE)
		{
			if (buffer_index > 0)
			{
				buffer[--buffer_index] = '\0';
				MarkChanged();
			}
			return 1;
		}
		if (key == SDLK_RETURN)
		{
			GUI_Screen *screen = GUI_GetScreen();
			screen->ClearFocusWidget();
			if (unfocus_callback)
				unfocus_callback->Call(this);	
			return 1;
		}
		
		
        //ch = kbd_get_key(); 
		//if (isascii(ch)) {
		if (ch >= 32 && ch <= 126)
		{
			if (buffer_index < buffer_size)
			{

				buffer[buffer_index++] = ch;
				buffer[buffer_index] = 0;
				MarkChanged();
			}
			return 1;
		}
	}
	
	return GUI_Widget::Event(event, xoffset, yoffset);
}

void GUI_TextEntry::SetFont(GUI_Font *afont)
{
	GUI_ObjectKeep((GUI_Object **) &font, afont);
	/* FIXME: should re-draw the text in the new color */
}

void GUI_TextEntry::SetTextColor(int r, int g, int b)
{
	textcolor.r = r;
	textcolor.g = g;
	textcolor.b = b;
	/* FIXME: should re-draw the text in the new color */
}

void GUI_TextEntry::SetText(const char *text)
{
	assert(text != NULL);

	if (strlen(text) < buffer_size)
	{
		strcpy(buffer, text);
		buffer_index = strlen(text);
	}
	MarkChanged();
}

const char *GUI_TextEntry::GetText(void)
{
	return buffer;
}

void GUI_TextEntry::SetNormalImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &normal_image, surface))
		MarkChanged();
}

void GUI_TextEntry::SetHighlightImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &highlight_image, surface))
		MarkChanged();
}

void GUI_TextEntry::SetFocusImage(GUI_Surface *surface)
{
	if (GUI_ObjectKeep((GUI_Object **) &focus_image, surface))
		MarkChanged();
}

void GUI_TextEntry::SetFocusCallback(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &focus_callback, callback);
}

void GUI_TextEntry::SetUnfocusCallback(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &unfocus_callback, callback);
}

extern "C"
{

GUI_Widget *GUI_TextEntryCreate(const char *name, int x, int y, int w, int h, GUI_Font *font, int size)
{
	return new GUI_TextEntry(name, x, y, w, h, font, size);
}

int GUI_TextEntryCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_TextEntrySetFont(GUI_Widget *widget, GUI_Font *font)
{
	((GUI_TextEntry *) widget)->SetFont(font);
}

void GUI_TextEntrySetTextColor(GUI_Widget *widget, int r, int g, int b)
{
	((GUI_TextEntry *) widget)->SetTextColor(r, g, b);
}

void GUI_TextEntrySetText(GUI_Widget *widget, const char *text)
{
	((GUI_TextEntry *) widget)->SetText(text);
}

const char *GUI_TextEntryGetText(GUI_Widget *widget)
{
	return ((GUI_TextEntry *) widget)->GetText();
}

void GUI_TextEntrySetNormalImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_TextEntry *) widget)->SetNormalImage(surface);
}

void GUI_TextEntrySetHighlightImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_TextEntry *) widget)->SetHighlightImage(surface);
}

void GUI_TextEntrySetFocusImage(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_TextEntry *) widget)->SetFocusImage(surface);
}

void GUI_TextEntrySetFocusCallback(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_TextEntry *) widget)->SetFocusCallback(callback);
}

void GUI_TextEntrySetUnfocusCallback(GUI_Widget *widget, GUI_Callback *callback)
{
	((GUI_TextEntry *) widget)->SetUnfocusCallback(callback);
}

}
