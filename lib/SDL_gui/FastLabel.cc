#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

//
// A label which favors speed over memory consumption.
//

GUI_FastLabel::GUI_FastLabel(const char *aname, int x, int y, int w, int h, GUI_Font *afont, const char *s)
: GUI_Picture(aname, x, y, w, h), font(afont)
{
	SetTransparent(1);
	
	textcolor.r = 255;
	textcolor.g = 255;
	textcolor.b = 255;

	font->IncRef();
	SetImage(font->RenderQuality(s, textcolor));
}

GUI_FastLabel::~GUI_FastLabel()
{
	font->DecRef();
}

void GUI_FastLabel::SetFont(GUI_Font *afont)
{
	if (GUI_ObjectKeep((GUI_Object **) &font, afont))
		MarkChanged();
	// FIXME: should re-draw the text
}

void GUI_FastLabel::SetTextColor(int r, int g, int b)
{
	textcolor.r = r;
	textcolor.g = g;
	textcolor.b = b;
	/* FIXME: should re-draw the text in the new color */
}

void GUI_FastLabel::SetText(const char *s)
{
	SetImage(font->RenderQuality(s, textcolor));
}
