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

static const int JOYSEL_MAX_WIDGETS = 256;
static const int JOYSEL_DEFAULT_KNOB_SIZE = 20;
static const int JOYSEL_MAJOR_GAP_WEIGHT = 32;
static const int JOYSEL_MINOR_GAP_WEIGHT = 8;
static const int JOYSEL_MAJOR_DIST_WEIGHT = 2;
static const int JOYSEL_MINOR_DIST_WEIGHT = 3;
static const int JOYSEL_TARGET_MISS_PENALTY = 128;

static int joysel_axis_gap(int first_start, int first_end, int second_start, int second_end)
{
	int gap = second_start - first_end;
	int reverse_gap = first_start - second_end;

	if(reverse_gap > gap) {
		gap = reverse_gap;
	}

	return gap > 0 ? gap : 0;
}

static int joysel_axis_overlap(int first_start, int first_end, int second_start, int second_end)
{
	int start = first_start > second_start ? first_start : second_start;
	int end = first_end < second_end ? first_end : second_end;

	return end - start;
}

static void joysel_calc_widget_box(GUI_Widget *widget, SDL_Rect *rect, int use_target)
{
	GUI_Drawable *parent = NULL;
	int type;
	int x;
	int y;
	int w;
	int h;

	*rect = widget->GetArea();
	type = ((GUI_Drawable *) widget)->GetWType();

	if(type == WIDGET_TYPE_SCROLLBAR) {
		GUI_ScrollBar *sb = (GUI_ScrollBar *) widget;
		GUI_Surface *knob = sb->GetKnobImage();
		int knob_w = knob ? knob->GetWidth() : JOYSEL_DEFAULT_KNOB_SIZE;
		int knob_h = knob ? knob->GetHeight() : JOYSEL_DEFAULT_KNOB_SIZE;

		if(rect->w > rect->h) {
			rect->x += sb->GetHorizontalPosition();
			rect->w = knob_w;
		}

		if(rect->h > rect->w) {
			rect->y += sb->GetVerticalPosition();
			rect->h = knob_h;
		}
	}
	else if(use_target && type == WIDGET_TYPE_BUTTON) {
		GUI_Surface *surface = ((GUI_AbstractButton *) widget)->GetCurrentImage();

		if(surface) {
			if(rect->w > surface->GetWidth()) {
				rect->w = surface->GetWidth();
			}

			if(rect->h > surface->GetHeight()) {
				rect->h = surface->GetHeight();
			}
		}
	}

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;
	parent = widget->GetParent();

	while(parent) {
		SDL_Rect parent_area = parent->GetArea();
		int parent_type = parent->GetWType();

		if(parent_type == WIDGET_TYPE_CONTAINER || parent_type == WIDGET_TYPE_CARDSTACK) {
			GUI_Container *container = (GUI_Container *) parent;
			int x1;
			int y1;
			int x2;
			int y2;

			x -= container->GetXOffset();
			y -= container->GetYOffset();

			x1 = x > 0 ? x : 0;
			y1 = y > 0 ? y : 0;
			x2 = x + w < parent_area.w ? x + w : parent_area.w;
			y2 = y + h < parent_area.h ? y + h : parent_area.h;

			x = x1;
			y = y1;
			w = x2 > x1 ? x2 - x1 : 0;
			h = y2 > y1 ? y2 - y1 : 0;
		}

		x += parent_area.x;
		y += parent_area.y;
		parent = parent->GetParent();
	}

	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;
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
	joysel = new GUI_Widget *[JOYSEL_MAX_WIDGETS];
	joysel_size = 0;
	joysel_cur = 0;
	joysel_enabled = 1;
}

GUI_Screen::~GUI_Screen(void)
{
	delete [] joysel;
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

	if (force) {
		LockVideo();
		Erase(&area);
	}

	if (contents)
		contents->DoUpdate(force);

	if (modal_widget) {
		modal_widget->DoUpdate(force);
	}

	FlushUpdates();

	if (force) {
		UnlockVideo();
	}
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
				
					if(joysel_size < JOYSEL_MAX_WIDGETS) {
						
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
	SDL_Rect target;

	if (widget == NULL)
		return;

	calc_widget_target(widget, &target);
	*x = target.x + (target.w / 2);
	*y = target.y + (target.h / 2);
}

void GUI_Screen::calc_widget_area(GUI_Widget *widget, SDL_Rect *rect)
{
	joysel_calc_widget_box(widget, rect, 0);
}

void GUI_Screen::calc_widget_target(GUI_Widget *widget, SDL_Rect *rect)
{
	joysel_calc_widget_box(widget, rect, 1);
}

void GUI_Screen::move_joysel(int dx, int dy, int xoffset, int yoffset)
{
	GUI_Container *container = modal_widget ? (GUI_Container *) modal_widget : (GUI_Container *) contents;
	GUI_Widget *current = NULL;
	SDL_Rect cur_area;
	SDL_Rect cur_target;
	SDL_Rect area;
	SDL_Rect target;
	SDL_Event evt;
	int mouse_x = 0;
	int mouse_y = 0;
	int cur_index = -1;
	int cur_center_x = 0;
	int cur_center_y = 0;
	int best = -1;
	int best_score = 0;
	int i;

	if(!container) {
		return;
	}

	if(joysel_cur >= 0 && joysel_cur < joysel_size) {
		current = joysel[joysel_cur];
	}

	for(i = 0; i < joysel_size; i++) {
		joysel[i] = NULL;
	}
	joysel_size = 0;
	find_widget_rec(container);

	if(!joysel_size) {
		joysel_cur = -1;
		return;
	}

	SDL_GetMouseState(&mouse_x, &mouse_y);
	mouse_x += xoffset;
	mouse_y += yoffset;

	for(i = 0; i < joysel_size; i++) {
		calc_widget_target(joysel[i], &target);

		if(target.w > 0 && target.h > 0 &&
		   mouse_x >= target.x && mouse_x < target.x + target.w &&
		   mouse_y >= target.y && mouse_y < target.y + target.h) {
			cur_index = i;
			break;
		}
	}

	if(cur_index < 0) {
		for(i = 0; i < joysel_size; i++) {
			calc_widget_area(joysel[i], &area);

			if(area.w > 0 && area.h > 0 &&
			   mouse_x >= area.x && mouse_x < area.x + area.w &&
			   mouse_y >= area.y && mouse_y < area.y + area.h) {
				cur_index = i;
				break;
			}
		}
	}

	if(cur_index < 0 && current) {
		for(i = 0; i < joysel_size; i++) {
			if(joysel[i] == current) {
				cur_index = i;
				break;
			}
		}
	}

	if(cur_index >= 0) {
		calc_widget_area(joysel[cur_index], &cur_area);
		calc_widget_target(joysel[cur_index], &cur_target);
	}
	else {
		cur_area.x = mouse_x;
		cur_area.y = mouse_y;
		cur_area.w = 1;
		cur_area.h = 1;
		cur_target = cur_area;
	}

	cur_center_x = cur_target.x + cur_target.w / 2;
	cur_center_y = cur_target.y + cur_target.h / 2;

	for(i = 0; i < joysel_size; i++) {
		int center_x;
		int center_y;
		int major_gap;
		int minor_gap;
		int minor_area_gap;
		int major_center_dist;
		int minor_center_dist;
		int overlap_area_maj;
		int overlap_area_min;
		int overlap_target_min;
		int target_penalty;
		int score;

		if(i == cur_index) {
			continue;
		}

		calc_widget_area(joysel[i], &area);
		calc_widget_target(joysel[i], &target);

		if(area.w <= 0 || area.h <= 0 || target.w <= 0 || target.h <= 0) {
			continue;
		}

		center_x = target.x + target.w / 2;
		center_y = target.y + target.h / 2;

		if(dx > 0 && center_x <= cur_center_x) {
			continue;
		}

		if(dx < 0 && center_x >= cur_center_x) {
			continue;
		}

		if(dy > 0 && center_y <= cur_center_y) {
			continue;
		}

		if(dy < 0 && center_y >= cur_center_y) {
			continue;
		}

		if(dx) {
			overlap_area_maj = joysel_axis_overlap(cur_target.x, cur_target.x + cur_target.w, target.x, target.x + target.w);
			overlap_area_min = joysel_axis_overlap(cur_area.y, cur_area.y + cur_area.h, area.y, area.y + area.h);
			overlap_target_min = joysel_axis_overlap(cur_target.y, cur_target.y + cur_target.h, target.y, target.y + target.h);
			major_gap = joysel_axis_gap(cur_target.x, cur_target.x + cur_target.w, target.x, target.x + target.w);
			minor_gap = joysel_axis_gap(cur_target.y, cur_target.y + cur_target.h, area.y, area.y + area.h);
			minor_area_gap = joysel_axis_gap(cur_area.y, cur_area.y + cur_area.h, area.y, area.y + area.h);
			major_center_dist = abs(center_x - cur_center_x);
			minor_center_dist = abs(center_y - cur_center_y);
		}
		else {
			overlap_area_maj = joysel_axis_overlap(cur_target.y, cur_target.y + cur_target.h, target.y, target.y + target.h);
			overlap_area_min = joysel_axis_overlap(cur_area.x, cur_area.x + cur_area.w, area.x, area.x + area.w);
			overlap_target_min = joysel_axis_overlap(cur_target.x, cur_target.x + cur_target.w, target.x, target.x + target.w);
			major_gap = joysel_axis_gap(cur_target.y, cur_target.y + cur_target.h, target.y, target.y + target.h);
			minor_gap = joysel_axis_gap(cur_target.x, cur_target.x + cur_target.w, area.x, area.x + area.w);
			minor_area_gap = joysel_axis_gap(cur_area.x, cur_area.x + cur_area.w, area.x, area.x + area.w);
			major_center_dist = abs(center_y - cur_center_y);
			minor_center_dist = abs(center_x - cur_center_x);
		}

		if(overlap_area_maj > 0 && overlap_area_min <= 0) {
			continue;
		}

		if(overlap_area_min <= 0 && minor_area_gap > major_center_dist) {
			continue;
		}

		target_penalty = overlap_target_min > 0 ? 0 : JOYSEL_TARGET_MISS_PENALTY;
		score = major_gap * JOYSEL_MAJOR_GAP_WEIGHT
		      + minor_gap * JOYSEL_MINOR_GAP_WEIGHT
		      + major_center_dist * JOYSEL_MAJOR_DIST_WEIGHT
		      + minor_center_dist * JOYSEL_MINOR_DIST_WEIGHT
		      + target_penalty;

		if(best < 0 || score < best_score) {
			best = i;
			best_score = score;
		}
	}

	if(best < 0 && cur_index < 0) {
		for(i = 0; i < joysel_size; i++) {
			int dist_x;
			int dist_y;
			int score;

			calc_widget_target(joysel[i], &target);

			if(target.w <= 0 || target.h <= 0) {
				continue;
			}

			dist_x = target.x + target.w / 2 - cur_center_x;
			dist_y = target.y + target.h / 2 - cur_center_y;
			score = dist_x * dist_x + dist_y * dist_y;

			if(best < 0 || score < best_score) {
				best = i;
				best_score = score;
			}
		}
	}

	if(best < 0) {
		joysel_cur = cur_index;
		return;
	}

	joysel_cur = best;
	memset(&evt, 0, sizeof(evt));
	evt.type = SDL_MOUSEMOTION;
	calc_parent_offset(joysel[joysel_cur], &evt.motion.x, &evt.motion.y);
	evt.motion.x -= xoffset;
	evt.motion.y -= yoffset;
	SDL_PushEvent(&evt);
	SDL_WarpMouse(evt.motion.x, evt.motion.y);
}

void GUI_Screen::SetJoySelectState(int value) {
	joysel_enabled = value;
}

int GUI_Screen::Event(const SDL_Event *event, int xoffset, int yoffset)
{
	if(joysel_enabled) {
		switch(event->type) {
			case SDL_JOYHATMOTION:
				if(!event->jhat.hat) {
					switch(event->jhat.value) {
						case SDL_HAT_UP:
							move_joysel(0, -1, xoffset, yoffset);
							break;
						case SDL_HAT_DOWN:
							move_joysel(0, 1, xoffset, yoffset);
							break;
						case SDL_HAT_LEFT:
							move_joysel(-1, 0, xoffset, yoffset);
							break;
						case SDL_HAT_RIGHT:
							move_joysel(1, 0, xoffset, yoffset);
							break;
						default:
							break;
					}
				}
				break;
			case SDL_KEYDOWN:
				switch(event->key.keysym.sym) {
					case SDLK_UP:
						move_joysel(0, -1, xoffset, yoffset);
						break;
					case SDLK_DOWN:
						move_joysel(0, 1, xoffset, yoffset);
						break;
					case SDLK_LEFT:
						move_joysel(-1, 0, xoffset, yoffset);
						break;
					case SDLK_RIGHT:
						move_joysel(1, 0, xoffset, yoffset);
						break;
					default:
						break;
				}
				break;
			default:
				break;
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
	LockVideo();
	SetFocusWidget(NULL);
	SetModalWidget(NULL);
	Keep(&contents, widget);

	for (int i = 0; i < joysel_size; i++)
	{
		joysel[i] = NULL;
	}
	joysel_size = 0;
	joysel_cur = -1;
	UnlockVideo();

	MarkChanged();
}

GUI_Widget *GUI_Screen::GetContents(void)
{
	return contents;
}

void GUI_Screen::SetModalWidget(GUI_Widget *widget)
{
	LockVideo();
	if (widget && modal_widget == widget) {
		UnlockVideo();
		return;
	}

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
	UnlockVideo();
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
	LockVideo();
	if (widget && focus_widget == widget) {
		UnlockVideo();
		return;
	}

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
	UnlockVideo();
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
