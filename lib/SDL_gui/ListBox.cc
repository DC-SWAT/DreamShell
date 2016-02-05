#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_ListBox::GUI_ListBox(const char *aname, int x, int y, int w, int h, GUI_Font *afont)
: GUI_AbstractTable(aname, x, y, w, h), font(afont)
{
	item_count = 0;
	item_max = 16;
	items = new char *[item_max];
	font->IncRef();
	textcolor.r = 255;
	textcolor.g = 255;
	textcolor.b = 255;
}

GUI_ListBox::~GUI_ListBox()
{
	int i;
	for (i=0; i<item_count; i++)
		delete items[i];
	delete [] items;
	font->DecRef();
}

int GUI_ListBox::GetRowCount(void)
{
	return item_count;
}

int GUI_ListBox::GetRowSize(int row)
{
	SDL_Rect r = font->GetTextSize("X");
	return r.h;
}

int GUI_ListBox::GetColumnCount(void)
{
	return 1;
}

int GUI_ListBox::GetColumnSize(int column)
{
	return area.w;
}

void GUI_ListBox::SetFont(GUI_Font *afont)
{
	if (GUI_ObjectKeep((GUI_Object **) &font, afont))
		MarkChanged();
}

void GUI_ListBox::SetTextColor(int r, int g, int b)
{
	textcolor.r = r;
	textcolor.g = g;
	textcolor.b = b;
	MarkChanged();
}

void GUI_ListBox::DrawCell(int column, int row, const SDL_Rect *r)
{
	GUI_Surface *image = font->RenderQuality(items[row], textcolor);
	SDL_Rect sr;
	
	sr.x = sr.y = 0;
	sr.w = image->GetWidth();
	sr.h = image->GetHeight();
	SDL_Rect dr = *r;
	dr.w = sr.w;
	dr.h = sr.h;
	Draw(image, &sr, &dr);
	image->DecRef();
}

void GUI_ListBox::AddItem(const char *s)
{
	if (item_count == item_max)
	{
		item_max += 16;
		char **newitems = new char *[item_max];
		int i;
		for (i=0; i<item_count; i++)
			newitems[i] = items[i];
		delete [] items;
		items = newitems;
	}
	char *buffer = items[item_count++] = new char[strlen(s)+1];
	strcpy(buffer, s);
}

void GUI_ListBox::RemoveItem(int n)
{
	if (n >= 0 && n < item_count)
	{
		delete items[n];
		item_count--;
		while (n<item_count)
		{
			items[n] = items[n+1];
			n++;
		}
	}
}



extern "C" {
       
       
GUI_ListBox *GUI_CreateListBox(const char *aname, int x, int y, int w, int h, GUI_Font *afont)
{
    return new GUI_ListBox(aname, x, y, w, h, afont);
}


int GUI_ListBoxGetRowCount(GUI_ListBox *list)
{
	return list->GetRowCount();
}

int GUI_ListBoxGetRowSize(GUI_ListBox *list, int row)
{
   return list->GetRowSize(row);
}

int GUI_ListBoxGetColumnCount(GUI_ListBox *list)
{
	return list->GetColumnCount();
}

int GUI_ListBoxGetColumnSize(GUI_ListBox *list, int column)
{
   return list->GetColumnSize(column);
}

void GUI_ListBoxSetFont(GUI_ListBox *list, GUI_Font *afont)
{
     list->SetFont(afont);
}

void GUI_ListBoxSetTextColor(GUI_ListBox *list, int r, int g, int b)
{
     list->SetTextColor(r, g, b);
}

void GUI_ListBoxDrawCell(GUI_ListBox *list, int column, int row, const SDL_Rect *r)
{
     list->DrawCell(column, row, r);
}

void GUI_ListBoxAddItem(GUI_ListBox *list, const char *s)
{
     list->AddItem(s);
}

void GUI_ListBoxRemoveItem(GUI_ListBox *list, int n)
{
    list->RemoveItem(n);
}

}


