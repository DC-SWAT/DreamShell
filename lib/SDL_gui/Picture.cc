#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_Picture::GUI_Picture(const char *aname, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h), image(NULL)
{
	SetTransparent(1);
	SetAlign(WIDGET_HORIZ_CENTER | WIDGET_VERT_CENTER);
	caption = 0;
}

GUI_Picture::GUI_Picture(const char *aname, int x, int y, int w, int h, GUI_Surface *an_image)
: GUI_Widget(aname, x, y, w, h), image(an_image)
{
	SetTransparent(1);
	SetAlign(WIDGET_HORIZ_CENTER | WIDGET_VERT_CENTER);
	image->IncRef();
	caption = 0;
}

GUI_Picture::~GUI_Picture()
{
	if (image) image->DecRef();
	if (caption) caption->DecRef();
}

void GUI_Picture::Update(int force)
{
	GUI_Widget::Update(force);
	if (caption)
		caption->DoUpdate(force);
}

void GUI_Picture::DrawWidget(const SDL_Rect *clip)
{
	if (parent == 0)
		return;
	
	if (image)
	{
		SDL_Rect dr;
		SDL_Rect sr;
			
		sr.w = dr.w = image->GetWidth();
		sr.h = dr.h = image->GetHeight();
		sr.x = sr.y = 0;

		switch (flags & WIDGET_HORIZ_MASK)
		{
			case WIDGET_HORIZ_CENTER:
				dr.x = area.x + (area.w - dr.w) / 2;
				break;
			case WIDGET_HORIZ_LEFT:
				dr.x = area.x;
				break;
			case WIDGET_HORIZ_RIGHT:
				dr.x = area.x + area.w - dr.w;
				break;
		}
		switch (flags & WIDGET_VERT_MASK)
		{
			case WIDGET_VERT_CENTER:
				dr.y = area.y + (area.h - dr.h) / 2;
				break;
			case WIDGET_VERT_TOP:
				dr.y = area.y;
				break;
			case WIDGET_VERT_BOTTOM:
				dr.y = area.y + area.h - dr.h;
				break;
		}
		if (GUI_ClipRect(&sr, &dr, clip))
			parent->Draw(image, &sr, &dr);
	}
}

int GUI_Picture::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	if (caption)
	{
		if (caption->Event(event, xoffset + area.x, yoffset + area.y))
			return 1;
	}
	return GUI_Widget::Event(event, xoffset, yoffset);
}

void GUI_Picture::SetImage(GUI_Surface *an_image)
{
	if (GUI_ObjectKeep((GUI_Object **) &image, an_image))
		MarkChanged();
}

void GUI_Picture::SetCaption(GUI_Widget *a_caption)
{
	Keep(&caption, a_caption);
}

extern "C"
{

GUI_Widget *GUI_PictureCreate(const char *name, int x, int y, int w, int h, GUI_Surface *image)
{
	return new GUI_Picture(name, x, y, w, h, image);
}

int GUI_PictureCheck(GUI_Widget *widget)
{
	// FIXME not implemented
	return 0;
}

void GUI_PictureSetImage(GUI_Widget *widget, GUI_Surface *image)
{
	((GUI_Picture *) widget)->SetImage(image);
}

void GUI_PictureSetCaption(GUI_Widget *widget, GUI_Widget *caption)
{
	((GUI_Picture *) widget)->SetCaption(caption);
}

}
