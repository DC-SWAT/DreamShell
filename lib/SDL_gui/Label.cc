#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

//
// A label which favors memory consumption over speed.
//

GUI_Label::GUI_Label(const char *aname, int x, int y, int w, int h, GUI_Font *afont, const char *s)
: GUI_Widget(aname, x, y, w, h), font(afont)
{
	SetTransparent(1);
	SetAlign(WIDGET_HORIZ_CENTER | WIDGET_VERT_CENTER);
	
	textcolor.r = 0;
	textcolor.g = 0;
	textcolor.b = 0;
	text = 0;
	SetText(s);
	font->IncRef();
	wtype = WIDGET_TYPE_OTHER;
}

GUI_Label::~GUI_Label()
{
	if (text) delete [] text;
	font->DecRef();
}

void GUI_Label::SetFont(GUI_Font *afont)
{
	if (GUI_ObjectKeep((GUI_Object **) &font, afont))
		MarkChanged();
}

void GUI_Label::SetTextColor(int r, int g, int b)
{
	textcolor.r = r;
	textcolor.g = g;
	textcolor.b = b;
	MarkChanged();
}

void GUI_Label::SetText(const char *s)
{
	delete [] text;
	text = new char[strlen(s)+1];
	strcpy(text, s);
	MarkChanged();
}

char *GUI_Label::GetText()
{
	return text;
}

void GUI_Label::DrawWidget(const SDL_Rect *clip)
{
	if (parent == 0)
		return;
	
	if (text && font)
	{
		GUI_Surface *image = font->RenderQuality(text, textcolor);
		SDL_Rect dr;
		SDL_Rect sr;
			
		sr.w = dr.w = image->GetWidth();
		sr.h = dr.h = image->GetHeight();
		sr.x = sr.y = 0;

		switch (flags & WIDGET_HORIZ_MASK)
		{
			case WIDGET_HORIZ_LEFT:
				dr.x = area.x;
				break;
			case WIDGET_HORIZ_RIGHT:
				dr.x = area.x + area.w - dr.w;
				break;
			case WIDGET_HORIZ_CENTER:
			default:
				dr.x = area.x + (area.w - dr.w) / 2;
				break;
		}
		switch (flags & WIDGET_VERT_MASK)
		{
			case WIDGET_VERT_TOP:
				dr.y = area.y;
				break;
			case WIDGET_VERT_BOTTOM:
				dr.y = area.y + area.h - dr.h;
				break;
			case WIDGET_VERT_CENTER:
			default:
				dr.y = area.y + (area.h - dr.h) / 2;
				break;
		}
		if (GUI_ClipRect(&sr, &dr, clip)) {
			parent->Erase(&area);
			parent->Draw(image, &sr, &dr);
		}
		image->DecRef();
	}
}

extern "C"
{

GUI_Widget *GUI_LabelCreate(const char *name, int x, int y, int w, int h, GUI_Font *font, const char *text)
{
	return new GUI_Label(name, x, y, w, h, font, text);
}

int GUI_LabelCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_LabelSetFont(GUI_Widget *widget, GUI_Font *font)
{
	((GUI_Label *) widget)->SetFont(font);
}
void GUI_LabelSetTextColor(GUI_Widget *widget, int r, int g, int b)
{
	((GUI_Label *) widget)->SetTextColor(r,g,b);
}

void GUI_LabelSetText(GUI_Widget *widget, const char *text)
{
	((GUI_Label *) widget)->SetText(text);
}

char *GUI_LabelGetText(GUI_Widget *widget)
{
	return ((GUI_Label *) widget)->GetText();
}

}
