#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_Panel::GUI_Panel(const char *aname, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h)
{
	layout = 0;
}

GUI_Panel::~GUI_Panel()
{
	if (layout) layout->DecRef();
}

void GUI_Panel::Update(int force)
{
	int i;
	
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

	for (i=0; i<n_widgets; i++)
		widgets[i]->DoUpdate(force);
}


int GUI_Panel::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	int i;
	xoffset += area.x - x_offset;
	yoffset += area.y - y_offset;
		
	for (i = 0; i < n_widgets; i++) {
		
		if(IsVisibleWidget(widgets[i])) {
			if (widgets[i]->Event(event, xoffset, yoffset))
				return 1;
		} 
	}
	return GUI_Drawable::Event(event, xoffset, yoffset);
}

void GUI_Panel::UpdateLayout(void)
{
	if (layout != NULL)
		layout->Layout(this);
}

void GUI_Panel::SetLayout(GUI_Layout *a_layout)
{
	if (GUI_ObjectKeep((GUI_Object **) &layout, a_layout))
	{
		UpdateLayout();
		MarkChanged();
	}
}

extern "C"
{

GUI_Widget *GUI_PanelCreate(const char *name, int x, int y, int w, int h)
{
	return new GUI_Panel(name, x, y, w, h);
}

int GUI_PanelCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_PanelSetBackground(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Panel *) widget)->SetBackground(surface);
}

void GUI_PanelSetBackgroundCenter(GUI_Widget *widget, GUI_Surface *surface)
{
	((GUI_Panel *) widget)->SetBackgroundCenter(surface);
}

void GUI_PanelSetBackgroundColor(GUI_Widget *widget, SDL_Color c)
{
	((GUI_Panel *) widget)->SetBackgroundColor(c);
}

void GUI_PanelSetXOffset(GUI_Widget *widget, int value)
{
	((GUI_Panel *) widget)->SetXOffset(value);
}

void GUI_PanelSetYOffset(GUI_Widget *widget, int value)
{
	((GUI_Panel *) widget)->SetYOffset(value);
}

void GUI_PanelSetLayout(GUI_Widget *widget, GUI_Layout *layout)
{
	((GUI_Panel *) widget)->SetLayout(layout);
}


int GUI_PanelGetXOffset(GUI_Widget *widget) {
	return ((GUI_Panel *) widget)->GetXOffset();
}

int GUI_PanelGetYOffset(GUI_Widget *widget) {
	return ((GUI_Panel *) widget)->GetYOffset();
}


}
