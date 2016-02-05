#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

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
	if (parent==0)
		return;

	if (force)
	{
		GUI_Surface *surface = GetCurrentImage();
		SDL_Rect src;
		src.x = src.y = 0;
		src.w = area.w;
		src.h = area.h;
	
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&area);
		if (surface)
			parent->Draw(surface, &src, &area);
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
	/*
	SDL_Rect d;
	
	d.w = dr->w;
	d.h = dr->h;
	d.x = area.x + dr->x;
	d.y = area.x + dr->y;
	
	parent->Erase(&d);
	*/
}

void GUI_AbstractButton::Notify(int mask)
{
	MarkChanged();
	GUI_Drawable::Notify(mask);
}

void GUI_AbstractButton::Clicked(int x, int y)
{
	if (click)
		click->Call(this);
}

void GUI_AbstractButton::ContextClicked(int x, int y)
{
	if (context_click)
		context_click->Call(this);
}

void GUI_AbstractButton::Highlighted(int x, int y)
{
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

