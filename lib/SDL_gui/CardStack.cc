#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

 extern "C"
 {
	void UpdateActiveMouseCursor();
 }

GUI_CardStack::GUI_CardStack(const char *aname, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h)
{
	visible_index = 0;
	wtype = WIDGET_TYPE_CARDSTACK;
}

GUI_CardStack::~GUI_CardStack(void)
{
}

void GUI_CardStack::Update(int force)
{
	if (flags & WIDGET_CHANGED)
	{
		force = 1;
		flags &= ~WIDGET_CHANGED;
	}
	
	if (force)
	{
		SDL_Rect r = area;
		r.x = x_offset;
		r.y = y_offset;
		Erase(&r);
	}

	if (n_widgets)
	{
		if (visible_index < 0 || visible_index >= n_widgets)
			visible_index = 0;
		
		widgets[visible_index]->DoUpdate(force);
	}
}

int GUI_CardStack::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	if (n_widgets)
	{
		if (visible_index < 0 || visible_index >= n_widgets)
			visible_index = 0;
		if (widgets[visible_index]->Event(event, xoffset + area.x - y_offset, yoffset + area.y - y_offset))
			return 1;
	}
	return GUI_Drawable::Event(event, xoffset, yoffset);
}

void GUI_CardStack::Next()
{
	if (n_widgets)
	{
		if (++visible_index >= n_widgets)
			visible_index = 0;
		
		MarkChanged();
	}
}

void GUI_CardStack::Prev()
{
	if (n_widgets)
	{
		if (--visible_index < 0)
			visible_index = n_widgets-1;

		MarkChanged();
	}
}

void GUI_CardStack::ShowIndex(int index)
{
	if (n_widgets)
	{
		if (index >= 0 && index < n_widgets) {
			visible_index = index;
			MarkChanged();
			UpdateActiveMouseCursor();
		}
	}
}

void GUI_CardStack::Show(const char *aname)
{
	int i;
	
	for (i=0; i<n_widgets; i++)
		if (widgets[i]->CheckName(aname) == 0)
		{	
			visible_index = i;
			MarkChanged();
			UpdateActiveMouseCursor();
			break;
		}
}

int GUI_CardStack::GetIndex()
{
	return visible_index;	
}

int GUI_CardStack::IsVisibleWidget(GUI_Widget *widget) {
	if (!widget)
		return 0;
		
	int i;
	for (i=0; i < n_widgets; i++)
		if (widgets[i] == widget && i == visible_index) {
			return 1;
		}
			
	return 0;
}

extern "C"
{

GUI_Widget *GUI_CardStackCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_CardStack(name, x, y, w, h);
}

int GUI_CardStackCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_CardStackSetBackground(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_CardStack *) widget)->SetBackground(surface);
}

void GUI_CardStackSetBackgroundColor(GUI_Widget *widget, SDL_Color c)
{
	((GUI_CardStack *) widget)->SetBackgroundColor(c);
}

void GUI_CardStackNext(GUI_Widget *widget)
{
	((GUI_CardStack *) widget)->Next();
}

void GUI_CardStackPrev(GUI_Widget *widget)
{
	((GUI_CardStack *) widget)->Prev();
}

void GUI_CardStackShowIndex(GUI_Widget *widget, int index)
{
	((GUI_CardStack *) widget)->ShowIndex(index);
}

void GUI_CardStackShow(GUI_Widget *widget, const char *name)
{
	((GUI_CardStack *) widget)->Show(name);
}

int GUI_CardStackGetIndex(GUI_Widget *widget)
{
	return ((GUI_CardStack *) widget)->GetIndex();
}

}
