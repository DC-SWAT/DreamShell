#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_VBoxLayout::GUI_VBoxLayout(const char *aname)
: GUI_Layout(aname)
{
}

GUI_VBoxLayout::~GUI_VBoxLayout(void)
{
}

void GUI_VBoxLayout::Layout(GUI_Container *container)
{
	SDL_Rect container_area = container->GetArea();
	int n = container->GetWidgetCount();
	int y, i;

	/* sum the heights of all children, and center it */
	y = container_area.h;
	for (i=0; i<n; i++)
	{
		GUI_Widget *temp = container->GetWidget(i);
		y -= temp->GetArea().h;
	}
	y /= 2;
	
	/* position the children */
	for (i=0; i<n; i++)
	{
		GUI_Widget *temp = container->GetWidget(i);
		SDL_Rect r = temp->GetArea();
		
		int x = (container_area.w - r.w) / 2;
		temp->SetPosition(x, y);
		y = y + r.h;
	}
}

extern "C" GUI_Layout *GUI_VBoxLayoutCreate(void)
{
	return new GUI_VBoxLayout("vbox");
}
