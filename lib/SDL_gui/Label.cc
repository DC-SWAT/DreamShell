#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C" {
	void LockVideo();
	void UnlockVideo();
}

GUI_Label::GUI_Label(const char *aname, int x, int y, int w, int h, GUI_Font *afont, const char *s)
: GUI_Widget(aname, x, y, w, h), font(afont)
{
	SetTransparent(1);
	SetAlign(WIDGET_HORIZ_CENTER | WIDGET_VERT_CENTER);
	
	textcolor.r = 0;
	textcolor.g = 0;
	textcolor.b = 0;
	text = 0;
	image = 0;
	SetText(s);

	if (font) {
		font->IncRef();
	}

	wtype = WIDGET_TYPE_OTHER;
}

GUI_Label::~GUI_Label()
{
	if (image) image->DecRef();
	if (text) delete [] text;
	if (font) font->DecRef();
}

void GUI_Label::SetFont(GUI_Font *afont)
{
	if (GUI_ObjectKeep((GUI_Object **) &font, afont)) {
		LockVideo();
		if (image) {
			image->DecRef();
			image = 0;
		}
		UnlockVideo();
		MarkChanged();
	}
}

void GUI_Label::SetTextColor(int r, int g, int b)
{
	LockVideo();
	textcolor.r = r;
	textcolor.g = g;
	textcolor.b = b;
	if (image) {
		image->DecRef();
		image = 0;
	}
	UnlockVideo();
	MarkChanged();
}

void GUI_Label::SetText(const char *s)
{
	char *ntext = NULL;
	char *otext;

	if (s) {
		ntext = new char[strlen(s) + 1];
		strcpy(ntext, s);
	}

	LockVideo();
	otext = text;
	text = ntext;
	if (image) {
		image->DecRef();
		image = 0;
	}
	UnlockVideo();

	if (otext)
		delete [] otext;

	MarkChanged();
}

char *GUI_Label::GetText()
{
	return text;
}

void GUI_Label::DrawWidget(const SDL_Rect *clip)
{
	int tw, th;
	SDL_Rect bounds;

	(void)clip;

	if (parent == 0)
		return;

	parent->Erase(&area);

	if (image == 0 && text && font)
		image = font->RenderQuality(text, textcolor);

	if (image == NULL)
		return;

	SDL_Rect dr;
	SDL_Rect sr;

	tw = image->GetWidth();
	th = image->GetHeight();
	if (tw <= 0 || th <= 0)
		return;

	sr.w = dr.w = tw;
	sr.h = dr.h = th;
	sr.x = sr.y = 0;

	switch (flags & WIDGET_HORIZ_MASK)
	{
		case WIDGET_HORIZ_LEFT:
			dr.x = 0;
			break;
		case WIDGET_HORIZ_RIGHT:
			dr.x = (int)area.w - tw;
			break;
		case WIDGET_HORIZ_CENTER:
		default:
			dr.x = ((int)area.w - tw) / 2;
			break;
	}
	switch (flags & WIDGET_VERT_MASK)
	{
		case WIDGET_VERT_TOP:
			dr.y = 0;
			break;
		case WIDGET_VERT_BOTTOM:
			dr.y = (int)area.h - th;
			break;
		case WIDGET_VERT_CENTER:
		default:
			dr.y = ((int)area.h - th) / 2;
			break;
	}

	bounds.x = 0;
	bounds.y = 0;
	bounds.w = area.w;
	bounds.h = area.h;

	if (GUI_ClipRect(&sr, &dr, &bounds)) {
		if (sr.w <= 0 || sr.h <= 0)
			return;
		if (sr.x < 0 || sr.y < 0 || sr.x + sr.w > tw || sr.y + sr.h > th)
			return;
		dr.x += area.x;
		dr.y += area.y;
		parent->Draw(image, &sr, &dr);
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
