// Modified by SWAT 
// http://www.dc-swat.net.ru 

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_Window::GUI_Window(const char *aname, GUI_Window *p, int x, int y, int w, int h)
: GUI_Object(aname), area(x,y,w,h), parent(p)
{
	// parent is a borrowed reference. do not incref or decref
}

GUI_Window::~GUI_Window(void)
{
}

void GUI_Window::UpdateAll(void)
{
	if (parent)
		parent->Update(area);
}

void GUI_Window::Update(const GUI_Rect &r)
{
	if (parent)
		parent->Update(r.Adjust(area.x, area.y));
}

GUI_Window *GUI_Window::CreateWindow(const char *aname, int x, int y, int w, int h)
{
	return new GUI_Window(aname, this, x, y, w, h);
}

void GUI_Window::DrawImage(const GUI_Surface *image, const GUI_Rect &src_r, const GUI_Rect &dst_r)
{
	if (parent)
		parent->DrawImage(image, src_r, dst_r.Adjust(area.x, area.y));
}

void GUI_Window::DrawRect(const GUI_Rect &r, const GUI_Color &c)
{
	if (parent)
		parent->DrawRect(r.Adjust(area.x, area.y), c);
}

void GUI_Window::DrawLine(int x1, int y1, int x2, int y2, const GUI_Color &c)
{
	if (parent)
		parent->DrawLine(x1+area.x, y1+area.y, x2+area.x, y2+area.y, c);
}

void GUI_Window::DrawPixel(int x, int y, const GUI_Color &c)
{
	if (parent)
		parent->DrawPixel(x+area.x, y+area.y, c);
}

void GUI_Window::FillRect(const GUI_Rect &r, const GUI_Color &c)
{
	if (parent)
		parent->FillRect(r.Adjust(area.x, area.y), c);
}



extern "C"
{
       
GUI_Window *CreateWindow(const char *aname, int x, int y, int w, int h)
{
    GUI_Window *win = NULL;
	return new GUI_Window(aname, win, x, y, w, h);
}



void GUI_WindowUpdateAll(GUI_Window *win)
{
	win->UpdateAll();
}

void GUI_WindowUpdate(GUI_Window *win, const GUI_Rect *r)
{
	win->Update(*r);
}


void GUI_WindowDrawImage(GUI_Window *win, const GUI_Surface *image, const GUI_Rect *src_r, const GUI_Rect *dst_r)
{
	win->DrawImage(image, *src_r, *dst_r);
}

void GUI_WindowDrawRect(GUI_Window *win, const GUI_Rect *r, const GUI_Color *c)
{
	win->DrawRect(*r, *c);
}

void GUI_WindowDrawLine(GUI_Window *win, int x1, int y1, int x2, int y2, const GUI_Color *c)
{
	win->DrawLine(x1, y1, x2, y2, *c);
}

void GUI_WindowDrawPixel(GUI_Window *win, int x, int y, const GUI_Color *c)
{
	win->DrawPixel(x, y, *c);
}

void GUI_WindowFillRect(GUI_Window *win, const GUI_Rect *r, const GUI_Color *c)
{
	win->FillRect(*r, *c);
}



}



