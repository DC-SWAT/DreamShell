#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"


GUI_Widget::GUI_Widget(const char *aname, int x, int y, int w, int h)
: GUI_Drawable(aname, x, y, w, h)
{
	parent = 0;
	//wtype = WIDGET_TYPE_OTHER;
}

GUI_Widget::~GUI_Widget(void)
{
}

void GUI_Widget::SetAlign(int mask)
{
	ClearFlags(WIDGET_ALIGN_MASK);
	WriteFlags(WIDGET_ALIGN_MASK, mask);
}

void GUI_Widget::SetTransparent(int flag)
{
	if (flag)
		SetFlags(WIDGET_TRANSPARENT);
	else
		ClearFlags(WIDGET_TRANSPARENT);
}

/* set/clear the disabled flag for a widget */

void GUI_Widget::SetEnabled(int flag)
{
	if (flag)
		ClearFlags(WIDGET_DISABLED);
	else
		SetFlags(WIDGET_DISABLED);
}

/* set the state of a widget (mainly for ToggleButton) */

void GUI_Widget::SetState(int state)
{
	if (state)
		SetFlags(WIDGET_TURNED_ON);
	else
		ClearFlags(WIDGET_TURNED_ON);
}

/* get the state of a widget (mainly for ToggleButton) */

int GUI_Widget::GetState()
{
	return (flags & WIDGET_TURNED_ON) != 0;
}

void GUI_Widget::SetParent(GUI_Drawable *aparent)
{
	parent = aparent;
}

GUI_Drawable *GUI_Widget::GetParent(void)
{
	return parent;
}

void GUI_Widget::Update(int force)
{
	if (parent==0)
		return;
	
	if (force)
	{
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&area);
		SDL_Rect r = area;
		r.x = r.y = 0;
		DrawWidget(&r);
	}
}


void GUI_Widget::Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr)
{
	if (parent)
	{
		SDL_Rect dest = Adjust(dr);
		parent->Draw(image, sr, &dest);
	}
}

void GUI_Widget::Fill(const SDL_Rect *dr, SDL_Color c)
{
	if (parent)
	{
		SDL_Rect dest = Adjust(dr);
		parent->Fill(&dest, c);
	}
}

void GUI_Widget::Erase(const SDL_Rect *dr)
{
	if (parent)
	{
		SDL_Rect dest = Adjust(dr);
		if (flags & WIDGET_TRANSPARENT)
			parent->Erase(&dest);
		DrawWidget(&dest);
	}
}

void GUI_Widget::DrawWidget(const SDL_Rect *clip)
{
	
}

extern "C"
{

void GUI_WidgetUpdate(GUI_Widget *widget, int force)
{
	widget->DoUpdate(force);
}

void GUI_WidgetDraw(GUI_Widget *widget, GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr)
{
	widget->Draw(image, sr, dr);
}

void GUI_WidgetErase(GUI_Widget *widget, const SDL_Rect *dr)
{
	widget->Erase(dr);
}

void GUI_WidgetFill(GUI_Widget *widget, const SDL_Rect *dr, SDL_Color c)
{
	widget->Fill(dr, c);
}

int GUI_WidgetEvent(GUI_Widget *widget, const SDL_Event *event, int xoffset, int yoffset)
{
	return widget->Event(event, xoffset, yoffset);
}

void GUI_WidgetClicked(GUI_Widget *widget, int x, int y)
{
	widget->Clicked(x, y);
}

void GUI_WidgetContextClicked(GUI_Widget *widget, int x, int y)
{
	widget->ContextClicked(x, y);
}

void GUI_WidgetHighlighted(GUI_Widget *widget, int x, int y)
{
	widget->Highlighted(x, y);
}

void GUI_WidgetSetAlign(GUI_Widget *widget, int align)
{
	widget->SetAlign(align);
}

void GUI_WidgetMarkChanged(GUI_Widget *widget)
{
	widget->MarkChanged();
}

void GUI_WidgetSetTransparent(GUI_Widget *widget, int trans)
{
	widget->SetTransparent(trans);
}

void GUI_WidgetSetEnabled(GUI_Widget *widget, int flag)
{
	widget->SetEnabled(flag);
}

void GUI_WidgetTileImage(GUI_Widget *widget, GUI_Surface *surface, const SDL_Rect *area, int x_offset, int y_offset)
{
	widget->TileImage(surface, area, x_offset, y_offset);
}

void GUI_WidgetSetFlags(GUI_Widget *widget, int mask)
{
	widget->SetFlags(mask);
}

void GUI_WidgetClearFlags(GUI_Widget *widget, int mask)
{
	widget->ClearFlags(mask);
}

void GUI_WidgetSetState(GUI_Widget *widget, int state)
{
	widget->SetState(state);
}

int GUI_WidgetGetState(GUI_Widget *widget)
{
	return widget->GetState();
}

SDL_Rect GUI_WidgetGetArea(GUI_Widget *widget)
{
	return widget->GetArea();
}

void GUI_WidgetSetPosition(GUI_Widget *widget, int x, int y)
{
	widget->SetPosition(x, y);
}

void GUI_WidgetSetSize(GUI_Widget *widget, int w, int h)
{
	widget->SetSize(w, h);
}

int GUI_WidgetGetType(GUI_Widget *widget)
{
	return ((GUI_Drawable*)widget)->GetWType();
}

int GUI_WidgetGetFlags(GUI_Widget *widget)
{
	return ((GUI_Drawable*)widget)->GetFlags();
}

GUI_Widget *GUI_WidgetGetParent(GUI_Widget *widget)
{
	return (GUI_Widget *)((GUI_Drawable*)widget)->GetParent();
}

}
