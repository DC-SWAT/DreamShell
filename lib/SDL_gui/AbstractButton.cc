#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C"
{
	#include "sfx.h"
}

GUI_AbstractButton::GUI_AbstractButton(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	SetTransparent(1);
	caption = 0;
	caption2 = 0;
	click = 0;
	context_click = 0;
	hover = 0;
	unhover = 0;
	wtype = WIDGET_TYPE_BUTTON;
}

GUI_AbstractButton::~GUI_AbstractButton()
{
	if (caption) caption->DecRef();
	if (caption2) caption2->DecRef();
	if (click) click->DecRef();
	if (context_click) context_click->DecRef();
	if (hover) hover->DecRef();
	if (unhover) unhover->DecRef();
}

GUI_Surface *GUI_AbstractButton::GetCurrentImage()
{
	return 0;
}

void GUI_AbstractButton::Update(int force)
{
	GUI_Surface *surface;
	SDL_Rect src;

	if (parent==0)
		return;

	if (force)
	{
		surface = GetCurrentImage();
		src.x = src.y = 0;
		src.w = area.w;
		src.h = area.h;
		if (surface) {
			if (src.w > surface->GetWidth())
				src.w = surface->GetWidth();
			if (src.h > surface->GetHeight())
				src.h = surface->GetHeight();

			SDL_Rect dest = area;
			dest.w = src.w;
			dest.h = src.h;
			parent->Draw(surface, &src, &dest);
		} else if (flags & WIDGET_TRANSPARENT) {
			parent->Erase(&area);
		}
	}

	if (caption)
		caption->DoUpdate(force);

	if (caption2)
		caption2->DoUpdate(force);
}

void GUI_AbstractButton::Fill(const SDL_Rect *dr, SDL_Color c)
{
}

void GUI_AbstractButton::Erase(const SDL_Rect *dr)
{
	if (parent == 0 || dr == NULL)
		return;

	SDL_Rect local = *dr;
	SDL_Rect bounds;
	SDL_Rect dest;
	SDL_Rect src;
	GUI_Surface *surface;
	int sw, sh;

	bounds.x = 0;
	bounds.y = 0;
	bounds.w = area.w;
	bounds.h = area.h;

	if (!GUI_ClipRect(NULL, &local, &bounds))
		return;

	dest.x = area.x + local.x;
	dest.y = area.y + local.y;
	dest.w = local.w;
	dest.h = local.h;
	parent->Erase(&dest);

	surface = GetCurrentImage();
	if (surface == 0)
		return;

	sw = surface->GetWidth();
	sh = surface->GetHeight();
	if (sw > area.w)
		sw = area.w;
	if (sh > area.h)
		sh = area.h;

	src = local;
	if (src.x >= sw || src.y >= sh)
		return;
	if (src.x + src.w > sw)
		src.w = sw - src.x;
	if (src.y + src.h > sh)
		src.h = sh - src.y;
	if (src.w <= 0 || src.h <= 0)
		return;

	dest.x = area.x + src.x;
	dest.y = area.y + src.y;
	dest.w = src.w;
	dest.h = src.h;
	parent->Draw(surface, &src, &dest);
}

void GUI_AbstractButton::DrawWidget(const SDL_Rect *clip)
{
	GUI_Surface *surface = GetCurrentImage();
	SDL_Rect src;
	SDL_Rect dest;

	if (parent == 0 || surface == 0)
		return;

	src.x = src.y = 0;
	src.w = surface->GetWidth();
	src.h = surface->GetHeight();
	if (src.w > area.w)
		src.w = area.w;
	if (src.h > area.h)
		src.h = area.h;

	dest = area;
	dest.w = src.w;
	dest.h = src.h;
	parent->Draw(surface, &src, &dest);
}

void GUI_AbstractButton::Notify(int mask)
{
	MarkChanged();
	GUI_Drawable::Notify(mask);
}

void GUI_AbstractButton::Clicked(int x, int y)
{
	ds_sfx_play(DS_SFX_CLICK);
	if (click) {
		click->Call(this);
	}
}

void GUI_AbstractButton::ContextClicked(int x, int y)
{
	ds_sfx_play(DS_SFX_CLICK);
	if (context_click) {
		context_click->Call(this);
	}
}

void GUI_AbstractButton::Highlighted(int x, int y)
{
	ds_sfx_play(DS_SFX_CLICK2);
	if (hover)
		hover->Call(this);
}

void GUI_AbstractButton::unHighlighted(int x, int y)
{
	if (unhover)
		unhover->Call(this);
}

void GUI_AbstractButton::RemoveWidget(GUI_Widget *widget)
{
	if (widget == caption)
		Keep(&caption, NULL);
		
	if (widget == caption2)
		Keep(&caption2, NULL);
}

void GUI_AbstractButton::SetCaption(GUI_Widget *widget)
{
	Keep(&caption, widget);
}

void GUI_AbstractButton::SetCaption2(GUI_Widget *widget)
{
	Keep(&caption2, widget);
}

GUI_Widget *GUI_AbstractButton::GetCaption()
{
	return caption;
}

GUI_Widget *GUI_AbstractButton::GetCaption2()
{
	return caption2;
}

void GUI_AbstractButton::SetClick(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &click, callback);
}

void GUI_AbstractButton::SetContextClick(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &context_click, callback);
}

void GUI_AbstractButton::SetMouseover(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &hover, callback);
}

void GUI_AbstractButton::SetMouseout(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &unhover, callback);
}

