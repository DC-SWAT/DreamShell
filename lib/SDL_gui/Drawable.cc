#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C"
{
void UpdateActiveMouseCursor();
}

static int Inside(int x, int y, SDL_Rect *r)
{
	return (x >= r->x) && (x < r->x + r->w) && (y >= r->y) && (y < r->y + r->h);
}

GUI_Drawable::GUI_Drawable(const char *aname, int x, int y, int w, int h)
: GUI_Object(aname)
{
	flags = 0;
	status_callback = 0;
	focused = 0;
	area.x = x;
	area.y = y;
	area.w = w;
	area.h = h;
	wtype = WIDGET_TYPE_OTHER;
}

GUI_Drawable::~GUI_Drawable(void)
{
}

int GUI_Drawable::GetWidth(void)
{
	return area.w;
}

int GUI_Drawable::GetHeight(void)
{
	return area.h;
}

void GUI_Drawable::SetWidth(int w)
{
	area.w = w;
	MarkChanged();
}

void GUI_Drawable::SetHeight(int h)
{
	area.h = h;
	MarkChanged();
}

SDL_Rect GUI_Drawable::Adjust(const SDL_Rect *rp)
{
	SDL_Rect r;
	
	//assert(rp != NULL);
	
	if(rp != NULL) {
		r.x = rp->x + area.x;
		r.y = rp->y + area.y;
		r.w = rp->w;
		r.h = rp->h;
	} else {
		r.x = 0;
		r.y = 0;
		r.w = 0;
		r.h = 0;
	}
	return r;
}

void GUI_Drawable::Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr)
{
}

void GUI_Drawable::Fill(const SDL_Rect *dr, SDL_Color c)
{
}

void GUI_Drawable::Erase(const SDL_Rect *dr)
{
}

void GUI_Drawable::RemoveWidget(GUI_Widget *widget)
{
}

void GUI_Drawable::Notify(int mask)
{
	flag_delta = mask;
	if (status_callback)
		status_callback->Call(this);
}

void GUI_Drawable::WriteFlags(int andmask, int ormask)
{
	//ds_printf("GUI_Drawable::WriteFlags: %s\n", GetName()); 
	
	int oldflags = flags;	
	flags = (flags & andmask) | ormask;
	if (flags != oldflags)
		Notify(flags ^ oldflags);
}

void GUI_Drawable::SetFlags(int mask)
{
	WriteFlags(-1, mask);
}

void GUI_Drawable::ClearFlags(int mask)
{
	WriteFlags(~mask, 0);
}

int GUI_Drawable::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	
	GUI_Screen *screen = GUI_GetScreen(); /*  FIXME GUI_GetScreen(); */
	GUI_Drawable *focus = screen->GetFocusWidget(); /* FIXME screen->GetFocusWidget(); */

	switch (event->type)
	{
/*		
		case SDL_KEYDOWN:
			return GUI_DrawableKeyPressed(object, event->key.keysym.sym, event->key.keysym.unicode);
		case SDL_KEYUP:
			return GUI_DrawableKeyReleased(object, event->key.keysym.sym, event->key.keysym.unicode);
*/
		case SDL_MOUSEBUTTONDOWN:
		{
			int x = event->button.x - xoffset;
			int y = event->button.y - yoffset;
			if ((flags & WIDGET_DISABLED) == 0/* && (flags & WIDGET_HIDDEN) == 0*/) {
				if (Inside(x, y, &area))
					if (focus == 0 || focus == this) {
						SetFlags(WIDGET_PRESSED);
						UpdateActiveMouseCursor();
					}
			}
			break;
		}		
		case SDL_MOUSEBUTTONUP:
		{
			int x = event->button.x - xoffset;
			int y = event->button.y - yoffset;
			if ((flags & WIDGET_DISABLED) == 0/* && (flags & WIDGET_HIDDEN) == 0*/)
			{
				if (flags & WIDGET_PRESSED)
					if (Inside(x, y, &area))
						if (focus == 0 || focus == this) {
//							ds_printf("Mouse button: %d\n", event->button.button);
							if(event->button.button != 1) {
								Clicked(x, y);
							} else {
								ContextClicked(x, y);
							}
						}
			}
			if (flags & WIDGET_PRESSED) {
				ClearFlags(WIDGET_PRESSED);
				UpdateActiveMouseCursor();
			}
			break;
		}
		case SDL_MOUSEMOTION:
		{		

			int x = event->motion.x - xoffset;
			int y = event->motion.y - yoffset;
			if (focus == 0 || focus == this)
			{
				if ((flags & WIDGET_DISABLED) == 0/* && (flags & WIDGET_HIDDEN) == 0*/ && Inside(x, y, &area)) {
					
					if(!focused) {
						SetFlags(WIDGET_INSIDE);
						UpdateActiveMouseCursor();
						Highlighted(x, y);
						focused = 1;
					}

				} else {

					if((flags & WIDGET_DISABLED) == 0) {
						
						if(focused) {
							ClearFlags(WIDGET_INSIDE);
							UpdateActiveMouseCursor();
							unHighlighted(x, y);
							focused = 0;
						}
					}
				}
			}
			break;
		}
	}
	return 0;
}

void GUI_Drawable::Clicked(int x, int y)
{
}

void GUI_Drawable::ContextClicked(int x, int y)
{
}

void GUI_Drawable::Highlighted(int x, int y)
{
}

void GUI_Drawable::unHighlighted(int x, int y)
{
}

void GUI_Drawable::Update(int force)
{
}

GUI_Drawable *GUI_Drawable::GetParent(void) {
	return NULL;
}

int GUI_Drawable::GetWType(void) {
	return wtype;
}

void GUI_Drawable::DoUpdate(int force)
{
	if (flags & WIDGET_CHANGED) {
		force = 1;
	}
	Update(force);
	flags &= ~WIDGET_CHANGED;
}

/* mark as changed so it will be drawn in the next update */

void GUI_Drawable::MarkChanged()
{
	flags |= WIDGET_CHANGED;
}

/* tile an image over an area on a widget */

void GUI_Drawable::TileImage(GUI_Surface *surface, const SDL_Rect *rp, int x_offset, int y_offset)
{
	SDL_Rect sr, dr;
	int xp, yp, bw, bh;

	//assert(surface != NULL);
	//assert(rp != NULL);
	
	if(surface == NULL || rp == NULL) return;
	
	bw = surface->GetWidth();
	bh = surface->GetHeight();
	
	//ds_printf("GUI_Drawable::TileImage: xo=%d yo=%d\n", x_offset, y_offset);
	
	for (xp=0; xp < rp->w; xp += sr.w)
	{
		dr.x = rp->x+xp;
		sr.x = (dr.x + x_offset) % bw;
		sr.w = dr.w = bw - sr.x;
		if (dr.x + dr.w > rp->x + rp->w)
			sr.w = dr.w = rp->x + rp->w - dr.x;
		for (yp=0; yp < rp->h; yp += sr.h)
		{
			dr.y = rp->y+yp;
			sr.y = (dr.y + y_offset) % bh;
			sr.h = dr.h = bh - sr.y;
			if (dr.y + dr.h > rp->y + rp->h)
				sr.h = dr.h = rp->y + rp->h - dr.y;
			Draw(surface, &sr, &dr);
		}
	}
}

void GUI_Drawable::CenterImage(GUI_Surface *surface, const SDL_Rect *rp, int x_offset, int y_offset)
{
	SDL_Rect sr, dr;
	int bw, bh;//, cx, cy;
	
	if(surface == NULL) return;
	
//	ds_printf("%s: %d %d %d %d\n", __func__, rp->x, rp->y, rp->w, rp->h);
	
	bw = surface->GetWidth();
	bh = surface->GetHeight();
	
	sr.x = 0;
	sr.y = 0;
	sr.w = bw;
	sr.h = bh;
	
	dr.x = (area.w / 2) - (bw / 2) + x_offset;
	dr.y = (area.h / 2) - (bh / 2) + y_offset;
	dr.w = bw;
	dr.h = bh;
	
	Draw(surface, &sr, &dr);

//	dr.x = rp->x;
//	sr.x = (dr.x + x_offset) % bw;
//	sr.w = dr.w = bw - sr.x;
//	
//	if (dr.x + dr.w > rp->x + rp->w)
//		sr.w = dr.w = rp->x + rp->w - dr.x;
//
//	dr.y = rp->y;
//	sr.y = (dr.y + y_offset) % bh;
//	sr.h = dr.h = bh - sr.y;
//	
//	if (dr.y + dr.h > rp->y + rp->h)
//		sr.h = dr.h = rp->y + rp->h - dr.y;
//	
//	cx = (area.w / 2) - (bw / 2) + x_offset;
//	cy = (area.h / 2) - (bh / 2) + y_offset;
	
//	sr.x += cx;
//	sr.y += cy;
//	dr.x += cx;
//	dr.y += cy;
	
	
//	if(sr.x < cx + bw && sr.x >= cx + bw) {
//		if(sr.y < cy + bh && sr.y >= cy + bh) {
//			ds_printf("%s: %d %d %d %d -> %d %d %d %d\n", __func__, sr.x, sr.y, sr.w, sr.h, dr.x, dr.y, dr.w, dr.h);
//			Draw(surface, &sr, &dr);
//		}
//	}
}


void GUI_Drawable::Keep(GUI_Widget **target, GUI_Widget *source)
{
	if (*target != source)
	{
		if (source)
			source->IncRef();
		if (*target)
		{
			(*target)->SetParent(0);
			(*target)->DecRef();
		}
		if (source)
			source->SetParent(this);
			
		(*target) = source;
		MarkChanged();
	}
}

SDL_Rect GUI_Drawable::GetArea()
{
	return area;
}

void GUI_Drawable::SetPosition(int x, int y)
{
	area.x = x;
	area.y = y;
	MarkChanged();
}

void GUI_Drawable::SetSize(int w, int h)
{
	area.w = w;
	area.h = h;
	MarkChanged();
}

void GUI_Drawable::SetStatusCallback(GUI_Callback *callback)
{
	GUI_ObjectKeep((GUI_Object **) &status_callback, callback);
}

int GUI_Drawable::GetFlagDelta(void)
{
	return flag_delta;
}

int GUI_Drawable::GetFlags(void)
{
	return flags;
}
