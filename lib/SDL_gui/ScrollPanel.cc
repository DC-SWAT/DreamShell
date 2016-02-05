// ScrollPanel
#include "SDL_gui.h"

GUI_ScrollPanel::GUI_ScrollPanel(const char *aname, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h)
{
	panel = new GUI_Panel("panel", 0, 0, w - 30, h - 30);
	AddWidget(panel);
	panel->DecRef();
	scrollbar_y = new GUI_ScrollBar("scrollbar", w - 30, 0, 30, h - 30);
	scrollbar_x = new GUI_ScrollBar("scrollbar", 0, h - 30, w - 30, 30);
	//GUI_Surface *sf = new GUI_Surface("/home/spanz/development/c/space/pics/tiny-button.png");
	//scrollbar_y->SetKnobImage(sf);
	//scrollbar_x->SetKnobImage(sf);
	//sf->DecRef();
	AddWidget(scrollbar_y);
	AddWidget(scrollbar_x);
	//printf("scrollbar_y added\n");
	scrollbar_y->DecRef();
	scrollbar_x->DecRef();

	GUI_EventHandler < GUI_ScrollPanel > *cb =
		new GUI_EventHandler < GUI_ScrollPanel > (this, &GUI_ScrollPanel::AdjustScrollbar);
	scrollbar_x->SetMovedCallback(cb);
	scrollbar_y->SetMovedCallback(cb);
	cb->DecRef();

	maxx = 0;
	maxy = 0;
	offsetx = 0;
	offsety = 0;
}

GUI_ScrollPanel::~GUI_ScrollPanel()
{
	scrollbar_y->DecRef();
	scrollbar_x->DecRef();
	panel->DecRef();
}

void GUI_ScrollPanel::AdjustScrollbar(GUI_Object * sender)
{
	if (sender == scrollbar_x)
	{
		int p = scrollbar_x->GetHorizontalPosition();

		panel->SetXOffset(p);
	}
	if (sender == scrollbar_y)
	{
		int p = scrollbar_y->GetVerticalPosition();

		panel->SetYOffset(p);
	}
}

void GUI_ScrollPanel::AddItem(GUI_Widget * w)
{
	panel->AddWidget(w);
	bool changed = false;

	if (w->GetArea().x + w->GetArea().w > maxx)
	{
		maxx = w->GetArea().x + w->GetArea().w;
		//printf("new maxx %d\n",maxx);
		changed = true;
	}
	if (w->GetArea().y + w->GetArea().w > maxy)
	{
		maxy = w->GetArea().y + w->GetArea().h;
		//    printf("new maxy %d\n",maxy);
		changed = true;
	}
	if (changed)
	{
		if (maxx < panel->GetArea().w + 1)
		{
			scrollbar_x->SetFlags(WIDGET_HIDDEN);
			//printf("hide scrollbar_x\n");
		} else
		{
			scrollbar_x->WriteFlags(~WIDGET_HIDDEN, 0);
			//printf("show scrollbar_x\n");
		}
		if (maxy < panel->GetArea().h)
		{
			scrollbar_y->SetFlags(WIDGET_HIDDEN);
			//printf("hide scrollbar_y\n");
		} else
		{
			scrollbar_y->WriteFlags(~WIDGET_HIDDEN, 0);
			//printf("show scrollbar_y\n");
		}
		Update(true);
		//MarkChanged();
	}
}

void GUI_ScrollPanel::RemoveItem(GUI_Widget * w)
{
	int n, i;

	panel->RemoveWidget(w);
	n = panel->GetWidgetCount();
	maxx = 0;
	maxy = 0;
	offsetx = 0;
	offsety = 0;
	for (i = 0; i < n; i++)
	{
		w = panel->GetWidget(i);
		if (w->GetArea().x + w->GetArea().w > maxx)
			maxx = w->GetArea().x + w->GetArea().w;
		if (w->GetArea().y + w->GetArea().w > maxy)
			maxy = w->GetArea().y + w->GetArea().h;
	}
	if (maxx < panel->GetArea().w + 1)
	{
		scrollbar_x->SetFlags(WIDGET_HIDDEN);
		//printf("hide scrollbar_x\n");
	} else
	{
		scrollbar_x->WriteFlags(~WIDGET_HIDDEN, 0);
		//printf("show scrollbar_x\n");
	}
	if (maxy < panel->GetArea().h)
	{
		scrollbar_y->SetFlags(WIDGET_HIDDEN);
		//printf("hide scrollbar_y\n");
	} else
	{
		scrollbar_y->WriteFlags(~WIDGET_HIDDEN, 0);
		//printf("show scrollbar_y\n");
	}
	Update(true);
}

void GUI_ScrollPanel::Update(int force)
{
	//GUI_Container::Update(force);
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
	for (int i = 0; i < n_widgets; i++)
		widgets[i]->DoUpdate(force);
}

int GUI_ScrollPanel::Event(const SDL_Event * event, int xoffset,
						   int yoffset)
{
	xoffset += area.x - x_offset;
	yoffset += area.y - y_offset;
	for (int i = 0; i < n_widgets; i++)
		if (widgets[i]->Event(event, xoffset, yoffset))
			return 1;
	return GUI_Drawable::Event(event, xoffset, yoffset);
}
