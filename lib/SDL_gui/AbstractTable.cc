#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_AbstractTable::GUI_AbstractTable(const char *aname, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h)
{
	SetTransparent(1);
}

GUI_AbstractTable::~GUI_AbstractTable()
{
}

void GUI_AbstractTable::Update(int force)
{
	if (parent == 0)
		return;
	
	if (force)
	{
		if (flags & WIDGET_TRANSPARENT)
		{
			SDL_Rect r = area;
			r.x = x_offset;
			r.y = y_offset;
			Erase(&r);
		}

		int row, column;
		int x, y, w, h;
		
		y = 0;
		for (row=0; row<GetRowCount(); row++)
		{
			x = 0;
			h = GetRowSize(row);
			for (column=0; column<GetColumnCount(); column++)
			{
				w = GetColumnSize(column);
				
				if (x>=0 && x<area.w && y>=0 && y<area.h)
				{
					SDL_Rect r;
					r.x = x;
					r.y = y;
					r.w = w;
					r.h = h;
					DrawCell(column, row, &r);
				}
				x += w;
			}
			y += h;
		}			
	}
}

int GUI_AbstractTable::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	return GUI_Widget::Event(event, xoffset, yoffset);
}

int GUI_AbstractTable::GetRowCount(void)
{
	return 16;
}

int GUI_AbstractTable::GetRowSize(int row)
{
	return area.h/16;
}

int GUI_AbstractTable::GetColumnCount(void)
{
	return 16;
}

int GUI_AbstractTable::GetColumnSize(int column)
{
	return area.w/16;
}

void GUI_AbstractTable::DrawCell(int column, int row, const SDL_Rect *dr)
{
	SDL_Color c;
	c.r = column * 16;
	c.g = row * 16;
	c.b = 0;
	c.unused = 0;
	Fill(dr, c);
}
