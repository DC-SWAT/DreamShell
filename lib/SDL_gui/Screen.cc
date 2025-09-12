#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C" {
	SDL_Surface *GetScreen();
	void LockVideo();
	void UnlockVideo();
	int VideoMustLock();
}


GUI_Screen::GUI_Screen(const char *aname, SDL_Surface *surface)
: GUI_Drawable(aname, 0, 0, surface->w, surface->h)
{
	screen_surface = new GUI_Surface("screen", surface);
	background = 0;
	contents = 0;
	focus_widget = 0;
	modal_widget = 0;
	background_color = 0;
	joysel = new GUI_Widget *[64];
	joysel_size = 0;
	joysel_cur = 0;
	joysel_enabled = 1;
}

GUI_Screen::~GUI_Screen(void)
{
	if (background) background->DecRef();
	if (focus_widget) focus_widget->DecRef();
	if (modal_widget) modal_widget->DecRef();
	if (contents) contents->DecRef();
	if (screen_surface) screen_surface->DecRef();
}

GUI_Surface *GUI_Screen::GetSurface(void)
{
	return screen_surface;
}

void GUI_Screen::FlushUpdates(void)
{
}

void GUI_Screen::UpdateRect(const SDL_Rect *r)
{
}

void GUI_Screen::Draw(GUI_Surface *image, const SDL_Rect *src_r, const SDL_Rect *dst_r)
{
	SDL_Rect sr, dr;
	SDL_Rect *srp, *drp;

	if(image == NULL) return;

	if (src_r)
	{
		sr = *src_r;
		srp = &sr;
	}
	else
		srp = NULL;
	if (dst_r)
	{
		dr = *dst_r;
		drp = &dr;
	}
	else
		drp = NULL;

	LockVideo();
	image->Blit(srp, screen_surface, drp);
	UnlockVideo();

	UpdateRect(drp);
}

void GUI_Screen::Fill(const SDL_Rect *dst_r, SDL_Color c)
{
	Uint32 color = screen_surface->MapRGB(c.r, c.g, c.b);
	SDL_Rect r = *dst_r;

	LockVideo();
	screen_surface->Fill(&r, color);
	UnlockVideo();

	UpdateRect(&r);
	
}

void GUI_Screen::Erase(const SDL_Rect *area)
{
	if (background)
		TileImage(background, area, 0, 0);
	else
	{
		SDL_Rect r;
		SDL_Rect *rp;
		if (area)
		{
			r = *area;
			rp = &r;
		}
		else
			rp = NULL;
		
		
		LockVideo();
		screen_surface->Fill(rp, background_color);
		UnlockVideo();
	}

	UpdateRect(area);
}

void GUI_Screen::Update(int force)
{

	if (force)
		Erase(&area);

	if (contents)
		contents->DoUpdate(force);

	if (modal_widget) {
		modal_widget->DoUpdate(force);
	}

	FlushUpdates();
}


void GUI_Screen::find_widget_rec(GUI_Container *p) {

	if(!p) return;
	
	int count = p->GetWidgetCount();
	int i, type;
	int j, ex;
	GUI_Widget *w;
	
	for(i = 0; i < count; i++) {
		
		w = p->GetWidget(i);
		
		if(p->IsVisibleWidget(w) && !(w->GetFlags() & WIDGET_DISABLED)) {
			type = ((GUI_Drawable*)w)->GetWType();
			
			switch(type) {
				case WIDGET_TYPE_CONTAINER:
				
					find_widget_rec((GUI_Container *)w);
					break;
					
				case WIDGET_TYPE_CARDSTACK:
				
					w = ((GUI_CardStack*)w)->GetWidget( ((GUI_CardStack*)w)->GetIndex() );
					find_widget_rec((GUI_Container *)w);
					break;
					
				case WIDGET_TYPE_BUTTON:
				case WIDGET_TYPE_SCROLLBAR:
				case WIDGET_TYPE_TEXTENTRY:
				
					if(joysel_size < 64) {
						
						ex = 0;
						
						for(j = 0; j < joysel_size; j++) {
							if(joysel[j] == w)  {
								ex = 1;
								break;
							}
						}
						
						if(!ex)
							joysel[joysel_size++] = w;
					}
				default:
					break;
			}
		}
	}
}

void GUI_Screen::calc_parent_offset(GUI_Widget *widget, Uint16 *x, Uint16 *y) {
	GUI_Drawable *w = NULL;
	SDL_Rect warea;
	int type = 0, p = 0;

	warea = widget->GetArea();
	type = ((GUI_Drawable *)widget)->GetWType();
	
	if(type == WIDGET_TYPE_SCROLLBAR) {
		
		if(warea.w > warea.h) {
			p = ((GUI_ScrollBar*)widget)->GetHorizontalPosition();
			*x = warea.x + p + 10;
		} else {
			*x = warea.x + (warea.w / 2);
		}
		
		if(warea.h > warea.w) {
			p = ((GUI_ScrollBar*)widget)->GetVerticalPosition();
			*y = warea.y + p + 10;
		} else {
			*y = warea.y + (warea.h / 2);
		}

	} else {
		*x = warea.x + (warea.w / 2);
		*y = warea.y + (warea.h / 2);
	}
	w = widget->GetParent();
	
	while(w) {
		warea = w->GetArea();
		type = w->GetWType();
		
		if(type == WIDGET_TYPE_CONTAINER) {
			*x += warea.x - ((GUI_Container*)w)->GetXOffset();
			*y += warea.y - ((GUI_Container*)w)->GetYOffset();
		} else {
			*x += warea.x;
			*y += warea.y;
		}
		w = w->GetParent();
	}
}

void GUI_Screen::SetJoySelectState(int value) {
	joysel_enabled = value;
}

int GUI_Screen::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	if(joysel_enabled) {

		SDL_Event evt;
		GUI_Container *container = modal_widget ? (GUI_Container *)modal_widget : (GUI_Container *)contents;
		
		switch(event->type) {
			case SDL_JOYHATMOTION:
			{
				if (event->jhat.hat) { // skip second d-pad
					break;
				}
				if (container) {
					switch(event->jhat.value) {
						case SDL_HAT_UP: //UP
						case SDL_HAT_DOWN: //DOWN
							for(int i = 0; i < joysel_size; i++) {
								joysel[i] = NULL;
							}
							joysel_size = 0;
							find_widget_rec(container);

							if(joysel_size) {

								if(event->jhat.value == SDL_HAT_UP) {
									if(joysel_cur <= 0) joysel_cur = joysel_size - 1;
									else joysel_cur--;
								} else {
									if(joysel_cur >= joysel_size-1) joysel_cur = 0;
									else joysel_cur++;
								}

								if(joysel[joysel_cur]) {
									evt.type = SDL_MOUSEMOTION;
									calc_parent_offset(joysel[joysel_cur], &evt.motion.x, &evt.motion.y);
									evt.motion.x -= xoffset;
									evt.motion.y -= yoffset;
									SDL_PushEvent(&evt);
									SDL_WarpMouse(evt.motion.x, evt.motion.y);
								}
							}
							break;

						case SDL_HAT_LEFT: //LEFT
						case SDL_HAT_RIGHT: //RIGHT
							for(int i = 0; i < joysel_size; i++) {
								joysel[i] = NULL;
							}
							joysel_size = 0;
							find_widget_rec(container);
							joysel_cur = event->jhat.value == SDL_HAT_LEFT ? 0 : joysel_size - 1;

							if(joysel_size && joysel[joysel_cur]) {
								evt.type = SDL_MOUSEMOTION;
								calc_parent_offset(joysel[joysel_cur], &evt.motion.x, &evt.motion.y);
								evt.motion.x -= xoffset;
								evt.motion.y -= yoffset;
								SDL_PushEvent(&evt);
								SDL_WarpMouse(evt.motion.x, evt.motion.y);
							}
							break;
					}
				}
				break;
			}

			case SDL_KEYDOWN:
			{
				if (container) {
					switch (event->key.keysym.sym) {
						default:
							break;

						case SDLK_UP: //UP
						case SDLK_DOWN: //DOWN
							for(int i = 0; i < joysel_size; i++) {
								joysel[i] = NULL;
							}
							joysel_size = 0;
							find_widget_rec(container);

							if(joysel_size) {

								if(event->key.keysym.sym == SDLK_UP) {
									if(joysel_cur <= 0) joysel_cur = joysel_size - 1;
									else joysel_cur--;
								} else {
									if(joysel_cur >= joysel_size-1) joysel_cur = 0;
									else joysel_cur++;
								}

								if(joysel[joysel_cur]) {
									evt.type = SDL_MOUSEMOTION;
									calc_parent_offset(joysel[joysel_cur], &evt.motion.x, &evt.motion.y);
									evt.motion.x -= xoffset;
									evt.motion.y -= yoffset;
									SDL_PushEvent(&evt);
									SDL_WarpMouse(evt.motion.x, evt.motion.y);
								}
							}

							break;

						case SDLK_RIGHT: //RIGHT
						case SDLK_LEFT: //LEFT
							for(int i = 0; i < joysel_size; i++) {
								joysel[i] = NULL;
							}
							joysel_size = 0;
							find_widget_rec(container);
							joysel_cur = event->key.keysym.sym == SDLK_LEFT ? 0 : joysel_size - 1;

							if(joysel_size && joysel[joysel_cur]) {
								evt.type = SDL_MOUSEMOTION;
								calc_parent_offset(joysel[joysel_cur], &evt.motion.x, &evt.motion.y);
								evt.motion.x -= xoffset;
								evt.motion.y -= yoffset;
								SDL_PushEvent(&evt);
								SDL_WarpMouse(evt.motion.x, evt.motion.y);
							}

							break;
					}
				}
				break;
			}
		}
	}

	if(modal_widget)
	{
		if(modal_widget->Event(event, xoffset, yoffset))
			return 1;
	}
	else if(contents)
	{
		if(contents->Event(event, xoffset, yoffset))
			return 1;
	}
	return GUI_Drawable::Event(event, xoffset, yoffset);
}

void GUI_Screen::RemoveWidget(GUI_Widget *widget)
{
	if (widget == contents)
		Keep(&contents, NULL);
}

void GUI_Screen::SetContents(GUI_Widget *widget)
{
	SetFocusWidget(NULL);
	SetModalWidget(NULL);
	Keep(&contents, widget);

	for (int i = 0; i < joysel_size; i++)
	{
		joysel[i] = NULL;
	}
	joysel_size = 0;
	joysel_cur = -1;

	MarkChanged();
}

GUI_Widget *GUI_Screen::GetContents(void)
{
	return contents;
}

void GUI_Screen::SetModalWidget(GUI_Widget *widget)
{
	if (widget && modal_widget == widget)
		return;

	for (int i = 0; i < joysel_size; i++)
	{
		joysel[i] = NULL;
	}
	joysel_size = 0;
	joysel_cur = -1;

	if (modal_widget)
	{
		modal_widget->DecRef();
		modal_widget = 0;
	}

	if (widget)
	{
		modal_widget = widget;
		modal_widget->IncRef();
	}
}

void GUI_Screen::SetBackground(GUI_Surface *image)
{
	if (GUI_ObjectKeep((GUI_Object **) &background, image))
		MarkChanged();
}

void GUI_Screen::SetBackgroundColor(SDL_Color c)
{
	Uint32 color;
	
	color = screen_surface->MapRGB(c.r, c.g, c.b);
	if (color != background_color)
	{
		background_color = color;
		MarkChanged();
	}
}

void GUI_Screen::SetFocusWidget(GUI_Widget *widget)
{
	if (widget && focus_widget == widget)
		return;

	if (focus_widget)
	{
		focus_widget->ClearFlags(WIDGET_HAS_FOCUS);
		focus_widget->DecRef();
		focus_widget = 0;
	}

	if (widget)
	{
		focus_widget = widget;
		focus_widget->IncRef();
		focus_widget->SetFlags(WIDGET_HAS_FOCUS);
	}
}

GUI_Widget *GUI_Screen::GetFocusWidget()
{
	return focus_widget;
}

GUI_Widget *GUI_Screen::GetModalWidget()
{
	return modal_widget;
}

extern "C"
{

GUI_Screen *GUI_ScreenCreate(int w, int h, int d, int f)
{
	return new GUI_RealScreen("screen", GetScreen());
}

void GUI_ScreenSetContents(GUI_Screen *screen, GUI_Widget *widget)
{
	screen->SetContents(widget);
}

GUI_Widget *GUI_ScreenGetContents(GUI_Screen *screen)
{
	return screen->GetContents();
}

void GUI_ScreenSetBackground(GUI_Screen *screen, GUI_Surface *image)
{
	screen->SetBackground(image);
}

void GUI_ScreenSetFocusWidget(GUI_Screen *screen, GUI_Widget *widget)
{
	screen->SetFocusWidget(widget);
}

void GUI_ScreenSetModalWidget(GUI_Screen *screen, GUI_Widget *widget)
{
	screen->SetModalWidget(widget);
}

GUI_Widget *GUI_ScreenGetModalWidget(GUI_Screen *screen)
{
	return screen->GetModalWidget();
}

void GUI_ScreenSetBackgroundColor(GUI_Screen *screen, SDL_Color c)
{
	screen->SetBackgroundColor(c);
}

GUI_Widget *GUI_ScreenGetFocusWidget(GUI_Screen *screen)
{
	return screen->GetFocusWidget();
}

void GUI_ScreenEvent(GUI_Screen *screen, const SDL_Event *event, int xoffset, int yoffset)
{
	screen->Event(event, xoffset, yoffset);
}


void GUI_ScreenDoUpdate(GUI_Screen *screen, int force) {
	if(screen)
		screen->DoUpdate(force);
}


void GUI_ScreenDraw(GUI_Screen *screen, GUI_Surface *image, const SDL_Rect *src_r, const SDL_Rect *dst_r)
{
	if(screen)
		screen->Draw(image, src_r, dst_r);
}


void GUI_ScreenFill(GUI_Screen *screen, const SDL_Rect *dst_r, SDL_Color c)
{
	if(screen)
		screen->Fill(dst_r, c);
}

void GUI_ScreenErase(GUI_Screen *screen, const SDL_Rect *area)
{
	if(screen)
		screen->Erase(area);
}

void GUI_ScreenSetJoySelectState(GUI_Screen *screen, int value) {
	screen->SetJoySelectState(value);
}

GUI_Surface *GUI_ScreenGetSurface(GUI_Screen *screen)
{
	if(screen)
		return screen->GetSurface();
		
	return NULL;
}

}
