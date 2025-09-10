#include <string.h>
#include <stdlib.h>
#include <kos/fs.h>
#include <kos/thread.h>

#include "SDL_gui.h"
#include <algorithm>

extern "C" {
	void SDL_DC_EmulateMouse(SDL_bool value);
}

GUI_FileManager::GUI_FileManager(const char *aname, const char *path, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h) {

	strncpy(cur_path, path != NULL ? path : "/", NAME_MAX);
	rescan = 1;

	scrollbar = new GUI_ScrollBar("scrollbar", 0, 20, 20, area.h - 40);
	button_up = new GUI_Button("button_up", 0, 0, 20, 20);
	button_down = new GUI_Button("button_down", 0, area.h - 20, 20, 20);

	item_area.w = w - scrollbar->GetWidth(); 
	item_area.h = 28; 
	item_area.x = 0;
	item_area.y = 0;

	scrollbar->SetPosition(item_area.w, 20);
	button_up->SetPosition(item_area.w, 0);
	button_down->SetPosition(item_area.w, area.h - 20);

	SDL_PixelFormat *format = GUI_GetScreen()->GetSurface()->GetSurface()->format;

	item_normal = new GUI_Surface("normal", SDL_HWSURFACE, item_area.w, item_area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	item_highlight = new GUI_Surface("highlight", SDL_HWSURFACE, item_area.w, item_area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	item_disabled = new GUI_Surface("disabled", SDL_HWSURFACE, item_area.w, item_area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	item_pressed = new GUI_Surface("pressed", SDL_HWSURFACE, item_area.w, item_area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	SDL_Rect rect;

	rect = {0, 0, (Uint16)item_area.w, 1};
	item_normal->Fill(&rect, SDL_MapRGB(format, 0xFF, 0xFF, 0xFF));
	item_pressed->Fill(&rect, SDL_MapRGB(format, 0xFF, 0xFF, 0xFF));
	item_disabled->Fill(&rect, SDL_MapRGB(format, 0xFF, 0xFF, 0xFF));
	rect = {0, 1, (Uint16)item_area.w, 26};
	item_normal->Fill(&rect, SDL_MapRGB(format, 0xF3, 0xF3, 0xF3));
	item_pressed->Fill(&rect, SDL_MapRGB(format, 0xF3, 0xF3, 0xF3));
	item_disabled->Fill(&rect, SDL_MapRGB(format, 0xF3, 0xF3, 0xF3));
	rect = {0, 27, (Uint16)item_area.w, 1};
	item_normal->Fill(&rect, SDL_MapRGB(format, 0xCC, 0xCC, 0xCC));
	item_pressed->Fill(&rect, SDL_MapRGB(format, 0xCC, 0xCC, 0xCC));
	item_disabled->Fill(&rect, SDL_MapRGB(format, 0xCC, 0xCC, 0xCC));

	rect = {0, 0, (Uint16)item_area.w, 1};
	item_highlight->Fill(&rect, SDL_MapRGB(format, 0xFF, 0xFF, 0xFF));
	rect = {0, 1, (Uint16)item_area.w, 26};
	item_highlight->Fill(&rect, SDL_MapRGB(format, 0xEE, 0xEE, 0xEE));
	rect = {0, 27, (Uint16)item_area.w, 1};
	item_highlight->Fill(&rect, SDL_MapRGB(format, 0xDD, 0xDD, 0xDD));

	item_selected_normal = new GUI_Surface("selected_normal", SDL_HWSURFACE, item_area.w, item_area.h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
	item_selected_normal->Fill(NULL, SDL_MapRGB(format, 0xFF, 0xBB, 0xBB));
	item_selected_highlight = NULL;
	item_selected_pressed = NULL;
	item_selected_disabled = NULL;

	item_click = NULL;
	item_context_click = NULL;
	item_mouseover = NULL;
	item_mouseout = NULL;
	item_select = NULL;
	item_select_callbacks = NULL;
 
	item_label_font = NULL;
	item_label_clr.r = 0;
	item_label_clr.g = 0;
	item_label_clr.b = 0;

	selected_item = -1;

	panel = new GUI_Panel("panel", 0, 0, item_area.w, area.h);

	if(panel) {
		AddWidget(panel);
		panel->DecRef();
	}

	GUI_EventHandler < GUI_FileManager > *cb;

	if(scrollbar) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::AdjustScrollbar);
		scrollbar->SetMovedCallback(cb);
		cb->DecRef();
		AddWidget(scrollbar);
	}

	if(button_up) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::ScrollbarButtonEvent);
		button_up->SetClick(cb);
		cb->DecRef();
		AddWidget(button_up);
	}

	if(button_down) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::ScrollbarButtonEvent);
		button_down->SetClick(cb);
		cb->DecRef();
		AddWidget(button_down);
	}
}

GUI_FileManager::~GUI_FileManager() {
	item_normal->DecRef();
	item_highlight->DecRef();
	item_pressed->DecRef();
	item_disabled->DecRef();

	if(item_selected_normal) item_selected_normal->DecRef();
	if(item_selected_highlight) item_selected_highlight->DecRef();
	if(item_selected_pressed) item_selected_pressed->DecRef();
	if(item_selected_disabled) item_selected_disabled->DecRef();

	if(item_select_callbacks) {
		for(int i=0; i < panel->GetWidgetCount(); i++) {
			if(item_select_callbacks[i])
				item_select_callbacks[i]->DecRef();
		}
		free(item_select_callbacks);
	}

	if(item_label_font)
		item_label_font->DecRef();

	if(scrollbar)
		scrollbar->DecRef();

	if(button_up)
		button_up->DecRef();

	if(button_down)
		button_down->DecRef();
}


void GUI_FileManager::Resize(int w, int h)
{
	area.w = w;
	area.h = h;
	item_area.w = w - scrollbar->GetWidth();

	if(panel)
		panel->SetSize(item_area.w, area.h);

	if(button_up)
		button_up->SetPosition(item_area.w, 0);

	if(button_down)
		button_down->SetPosition(item_area.w, area.h - button_down->GetHeight());

	if(scrollbar) {
		scrollbar->SetPosition(item_area.w, button_up->GetHeight());
		scrollbar->SetHeight(area.h - (button_up->GetHeight() + button_down->GetHeight()));
	}

	ReScan();
}


void GUI_FileManager::AdjustScrollbar(GUI_Object * sender) {
	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	
	if(cont_height <= 0) {
		panel->SetYOffset(0);
		UpdateScrollbarButtons();
		return;
	}

	int scroll_pos = scrollbar->GetVerticalPosition();
	int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
	
	if (scroll_pos >= scroll_height) {
		panel->SetYOffset(cont_height);
	} else if(scroll_height > 0) {
		panel->SetYOffset((int)(scroll_pos * (float)cont_height / scroll_height));
	} else {
		panel->SetYOffset(0);
	}
	UpdateScrollbarButtons();
}

void GUI_FileManager::ScrollbarButtonEvent(GUI_Object * sender) {

	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	if(cont_height <= 0) {
		return;
	}

	int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
	if (scroll_height <= 0) {
		return;
	}

	int current_offset = panel->GetYOffset();
	int new_offset = current_offset;

	if(sender == button_up) {
		new_offset -= item_area.h;
	} else if(sender == button_down) {
		new_offset += item_area.h;
	}

	if (new_offset < 0) {
		new_offset = 0;
	}
	if (new_offset > cont_height) {
		new_offset = cont_height;
	}

	if (new_offset != current_offset) {
		UpdatePanelOffsetAndScrollbar(new_offset);
	}
}


void GUI_FileManager::SetPath(const char *path) {
	strncpy(cur_path, path, NAME_MAX);
	ReScan();
}

const char *GUI_FileManager::GetPath() {
	return (const char*)cur_path;
}

void GUI_FileManager::ChangeDir(const char *name, int size) {
	char *b, path[NAME_MAX];
	int s;
	file_t fd;
	
	if(name/* && size < 0*/) {
		if(size == -2 && name[0] == '.' && name[1] == '.') {

			b = strrchr(cur_path, '/');

			if(b) {

				s = b - cur_path;

				if(s > 0)
					cur_path[s] = '\0';
				else
					cur_path[1] = '\0';

				ReScan();
			}
		} else {

			memset(path, 0, NAME_MAX);

			if(strlen(cur_path) > 1) {
				snprintf(path, NAME_MAX, "%s/%s", cur_path, name);
			} else {
				snprintf(path, NAME_MAX, "%s%s", cur_path, name);
			}

			s = strlen(cur_path) + strlen(name) + 1;
			path[s > NAME_MAX-1 ? NAME_MAX-1 : s] = '\0';

			fd = fs_open(path, O_RDONLY | O_DIR);

			if(fd != FILEHND_INVALID) {
				fs_close(fd);
				strncpy(cur_path, path, NAME_MAX);
				ReScan();
			} else {
				printf("GUI_FileManager: Can't open dir: %s\n", path);
			}
		}
	}
}

static int alpha_sort(dirent_t *a, dirent_t *b)
{
	const char *s1 = a->name, *s2 = b->name;
	
	while (toupper(*s1) == toupper(*s2))
	{
		if (*s1 == 0)
			return 0;
		s1++;
		s2++;
	}
	
	return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)(s2));
}

void GUI_FileManager::Scan() 
{
	file_t f;
	dirent_t *ent;
	dirent_t *sorts = (dirent_t *) malloc(sizeof(dirent_t));
	int n = 0;

	if(sorts == NULL) {
		return;
	}

	f = fs_open(cur_path, O_RDONLY | O_DIR);
	
	if(f == FILEHND_INVALID) 
	{
		printf("GUI_FileManager: Can't open dir: %s\n", cur_path);
		free(sorts);
		return;
	}

	panel->RemoveAllWidgets();
	panel->SetYOffset(0);
	scrollbar->SetVerticalPosition(0);
	UpdateScrollbarButtons();
	selected_item = -1;

	if(strlen(cur_path) > 1) 
	{
		AddItem("..", -2, 0, O_DIR);
	}

	while ((ent = fs_readdir(f)) != NULL) 
	{
		if(ent->name[0] != '.' &&
			strncasecmp(ent->name, "RECYCLER", NAME_MAX) &&
			strncasecmp(ent->name, "$RECYCLE.BIN", NAME_MAX) &&
			strncasecmp(ent->name, "System Volume Information", NAME_MAX)
		) {
			if (n) {
				sorts = (dirent_t *) realloc((void *) sorts, (sizeof(dirent_t)*(n+1)));
			}
			memcpy(&sorts[n], ent, sizeof(dirent_t));
			n++;
		}
	}

	qsort((void *) sorts, n, sizeof(dirent_t), (int (*)(const void*, const void*)) alpha_sort);

	for (int i = 0; i < n; i++)
	{
		if (sorts[i].attr)
		{
			AddItem(sorts[i].name, sorts[i].size, sorts[i].time, sorts[i].attr);
		}
	}

	for (int i = 0; i < n; i++)
	{
		if (!sorts[i].attr)
		{
			AddItem(sorts[i].name, sorts[i].size, sorts[i].time, sorts[i].attr);
		}
	}
	rescan = 0;

	free(sorts);
	fs_close(f);
}

void GUI_FileManager::ReScan() {
	ClearSelection();
	rescan = 1;
	MarkChanged();
}

void GUI_FileManager::AddItem(const char *name, int size, int time, int attr) {
	
	GUI_Button *bt;
	GUI_Callback *cb;
//	GUI_EventHandler < GUI_FileManager > *cb;
	GUI_Label *lb;
	dirent_fm_t *prm = NULL;
	int need_free = 1;
	int cnt = panel->GetWidgetCount();

	bt = new GUI_Button(name, item_area.x, item_area.y + (cnt * item_area.h), item_area.w, item_area.h);
	
	bt->SetNormalImage(item_normal);
	bt->SetHighlightImage(item_highlight);
	bt->SetPressedImage(item_pressed);
	bt->SetDisabledImage(item_disabled);

	if(item_click || item_context_click || item_mouseover || item_mouseout || item_select) {
		
		prm = (dirent_fm_t *) malloc(sizeof(dirent_fm_t));
		
		if(prm != NULL) {
			strncpy(prm->ent.name, name, NAME_MAX);
			prm->ent.size = size;
			prm->ent.time = time;
			prm->ent.attr = attr;
			prm->obj = this;
			prm->index = cnt;
		}
	}

	if(prm) {
		if(item_click) {
//			cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::ItemClickEvent, (GUI_CallbackFunction*)free, (void*)prm);
			cb = new GUI_Callback_C(item_click, (GUI_CallbackFunction*)free, (void*)prm);
			bt->SetClick(cb);
			cb->DecRef();
			need_free = 0;
		}
		
		if(item_context_click) {
			
			if(need_free) {
				cb = new GUI_Callback_C(item_context_click, (GUI_CallbackFunction*)free, (void*)prm);
				need_free = 0;
			} else {
				cb = new GUI_Callback_C(item_context_click, NULL, (void*)prm);
			}
			
			bt->SetContextClick(cb);
			cb->DecRef();
		}
		
		if(item_mouseover) {
			
			if(need_free) {
				cb = new GUI_Callback_C(item_mouseover, (GUI_CallbackFunction*)free, (void*)prm);
				need_free = 0;
			} else {
				cb = new GUI_Callback_C(item_mouseover, NULL, (void*)prm);
			}
			
			bt->SetMouseover(cb);
			cb->DecRef();
		}
		
		if(item_mouseout) {
			if(need_free) {
				cb = new GUI_Callback_C(item_mouseout, (GUI_CallbackFunction*)free, (void*)prm);
				need_free = 0;
			} else {
				cb = new GUI_Callback_C(item_mouseout, NULL, (void*)prm);
			}
			bt->SetMouseout(cb);
			cb->DecRef();
		}

		if(item_select) {
			item_select_callbacks = (GUI_Callback **) realloc(item_select_callbacks, sizeof(GUI_Callback *) * (cnt + 1));

			if(item_select_callbacks != NULL) {
				if(need_free) {
					cb = new GUI_Callback_C(item_select, (GUI_CallbackFunction*)free, (void*)prm);
					need_free = 0;
				} else {
					cb = new GUI_Callback_C(item_select, NULL, (void*)prm);
				}
				item_select_callbacks[cnt] = cb;
			}
		}
	}

	if(item_label_font) {
//		SDL_Rect ts = item_label_font->GetTextSize(name);
		
		lb = new GUI_Label(name, 4, 0, item_area.w - 6, item_area.h, item_label_font, name);
		lb->SetTextColor(item_label_clr.r, item_label_clr.g, item_label_clr.b);
		lb->SetAlign(WIDGET_HORIZ_LEFT | WIDGET_VERT_CENTER);
		bt->SetCaption(lb);
		lb->DecRef();
		/*
		if(size > 0) {
			char inf[64];
			sprintf(inf, "%d Kb", size / 1024);
			SDL_Rect ts2 = item_label_font->GetTextSize(inf);
			
			lb = new GUI_Label(inf, (item_area.w - (ts2.w + 10)), 0, 100, item_area.h, item_label_font, inf);
			lb->SetTextColor(item_label_clr.r, item_label_clr.g, item_label_clr.b);
			lb->SetAlign(WIDGET_HORIZ_RIGHT | WIDGET_VERT_CENTER);
			bt->SetCaption2(lb);
			lb->DecRef();
		}*/
	}
	
	panel->AddWidget(bt);
	bt->DecRef();
}

GUI_Button *GUI_FileManager::GetItem(int index) {
	return (GUI_Button *)panel->GetWidget(index);
}

GUI_Panel *GUI_FileManager::GetItemPanel() {
	return panel;
}

void GUI_FileManager::SetItemLabel(GUI_Font *font, int r, int g, int b)	{
	GUI_ObjectKeep((GUI_Object **) &item_label_font, font);
	item_label_clr.r = r;
	item_label_clr.g = g;
	item_label_clr.b = b;
	ReScan();
}

void GUI_FileManager::SetItemSurfaces(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	
	if(normal) {
		item_area.w = normal->GetWidth();
		item_area.h = normal->GetHeight();

		if(item_area.w > (area.w - scrollbar->GetWidth())) {
			item_area.w = area.w - scrollbar->GetWidth();
		}
	}

	GUI_ObjectKeep((GUI_Object **) &item_normal, normal);
	GUI_ObjectKeep((GUI_Object **) &item_highlight, highlight);
	GUI_ObjectKeep((GUI_Object **) &item_pressed, pressed);
	GUI_ObjectKeep((GUI_Object **) &item_disabled, disabled);	
	ReScan();
}

void GUI_FileManager::SetItemSelectedSurfaces(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	GUI_ObjectKeep((GUI_Object **) &item_selected_normal, normal);
	GUI_ObjectKeep((GUI_Object **) &item_selected_highlight, highlight);
	GUI_ObjectKeep((GUI_Object **) &item_selected_pressed, pressed);
	GUI_ObjectKeep((GUI_Object **) &item_selected_disabled, disabled);
	ReScan();
}

void GUI_FileManager::SetItemSize(const SDL_Rect *item_r) {
	item_area.w = item_r->w;
	item_area.h = item_r->h;
	item_area.x = item_r->x;
	item_area.y = item_r->y;
	ReScan();
}

void GUI_FileManager::SetItemClick(GUI_CallbackFunction *func) {
	item_click = func;
	ReScan();
}

void GUI_FileManager::SetItemContextClick(GUI_CallbackFunction *func) {
	item_context_click = func;
	ReScan();
}

void GUI_FileManager::SetItemMouseover(GUI_CallbackFunction *func) {
	item_mouseover = func;
	ReScan();
}

void GUI_FileManager::SetItemMouseout(GUI_CallbackFunction *func) {
	item_mouseout = func;
	ReScan();
}

void GUI_FileManager::SetItemSelect(GUI_CallbackFunction *func) {
	item_select = func;
	ReScan();
}

void GUI_FileManager::SetScrollbar(GUI_Surface *knob, GUI_Surface *background) {
	
	if(knob) {
		scrollbar->SetKnobImage(knob);
		scrollbar->SetWidth(knob->GetWidth());
	}
		
	if(background) {
		scrollbar->SetBackgroundImage(background);
		scrollbar->SetHeight(background->GetHeight());
	}
		
	MarkChanged();
}

void GUI_FileManager::SetScrollbarButtonUp(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	button_up->SetNormalImage(normal);
	button_up->SetHighlightImage(highlight);
	button_up->SetPressedImage(pressed);
	button_up->SetDisabledImage(disabled);
	MarkChanged();
}

void GUI_FileManager::SetScrollbarButtonDown(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	button_down->SetNormalImage(normal);
	button_down->SetHighlightImage(highlight);
	button_down->SetPressedImage(pressed);
	button_down->SetDisabledImage(disabled);
	MarkChanged();
}

void GUI_FileManager::RemoveScrollbar() {
	RemoveWidget(button_down);
	RemoveWidget(button_up);
	RemoveWidget(scrollbar);
}

void GUI_FileManager::RestoreScrollbar() {
	AddWidget(button_down);
	AddWidget(button_up);
	AddWidget(scrollbar);
}

void GUI_FileManager::Update(int force) {
	
	if (flags & WIDGET_DISABLED) return;
	
	int i;
	
	if (flags & WIDGET_CHANGED)
	{
		force = 1;
		flags &= ~WIDGET_CHANGED;
	}
	
	if (force)
	{
		SDL_Rect r = area;
		r.x = x_offset;
		r.y = y_offset;
		Erase(&r);
	}

	if(rescan)  {
		if(item_select_callbacks) {
			for(int i=0; i < panel->GetWidgetCount(); i++) {
				if(item_select_callbacks[i])
					item_select_callbacks[i]->DecRef();
			}
			free(item_select_callbacks);
			item_select_callbacks = NULL;
		}

		panel->RemoveAllWidgets();
		panel->SetYOffset(0);

		for (i = 0; i < n_widgets; i++) {
			widgets[i]->DoUpdate(force);
		}

		Scan();

	} else {
		for (i = 0; i < n_widgets; i++) {
			widgets[i]->DoUpdate(force);
		}
	}
}

void GUI_FileManager::HandleMouseWheel(const SDL_Event *event) {
	if ((flags & WIDGET_PRESSED) || !event->motion.z) {
		return;
	}
	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	if (cont_height <= 0) return;

	int current_offset = panel->GetYOffset();
	int new_offset = current_offset;
	int val = event->motion.z;
	
	if (val < 0) {
		new_offset -= item_area.h;
	}
	else if(val > 0) {
		new_offset += item_area.h;
	}
	else {
		return;
	}

	if(new_offset > cont_height) new_offset = cont_height;
	if(new_offset < 0) new_offset = 0;

	if(current_offset != new_offset) {
		UpdatePanelOffsetAndScrollbar(new_offset);
	}
}

void GUI_FileManager::HandleKeyDown(const SDL_Event *event) {
	switch (event->key.keysym.sym) {
		default:
			break;

		case SDLK_HOME:
		{
			panel->SetYOffset(0);
			scrollbar->SetVerticalPosition(0);
			UpdateScrollbarButtons();
			break;
		}

		case SDLK_END:
		{
			int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
			if (cont_height > 0) {
				int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
				panel->SetYOffset(cont_height);
				scrollbar->SetVerticalPosition(scroll_height);
				UpdateScrollbarButtons();
			}
			break;
		}

		case SDLK_PAGEUP:
		case SDLK_PAGEDOWN:
		{
			int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
			if (cont_height <= 0) break;

			int current_offset = panel->GetYOffset();
			int new_offset = current_offset;

			if (event->key.keysym.sym == SDLK_PAGEUP) {
				new_offset -= panel->GetHeight();
			}
			else {
				new_offset += panel->GetHeight();
			}

			if(new_offset > cont_height) new_offset = cont_height;
			if(new_offset < 0) new_offset = 0;

			if(current_offset != new_offset) {
				UpdatePanelOffsetAndScrollbar(new_offset);
			}
		}
		break;
	}
}

void GUI_FileManager::HandleAnalogJoyMotion(const SDL_Event *event) {
	if(!(flags & WIDGET_PRESSED)) {
		return;
	}

	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	if (cont_height <= 0) return;

	int val = event->jaxis.value;

	int sleep_time = 128 - abs(val);
	if(sleep_time > 0) {
		thd_sleep(sleep_time);
	}

	int current_offset = panel->GetYOffset();
	int new_offset = current_offset;

	if (val < -12) {
		new_offset -= item_area.h;
	}
	else if(val > 12) {
		new_offset += item_area.h;
	}
	else {
		return;
	}

	if(new_offset > cont_height) new_offset = cont_height;
	if(new_offset < 0) new_offset = 0;

	if(current_offset != new_offset) {
		UpdatePanelOffsetAndScrollbar(new_offset);
	}
}

void GUI_FileManager::HandleJoyHatMotion(const SDL_Event *event) {
	if (event->jhat.hat) { // skip second d-pad
		return;
	}
	if(!(flags & WIDGET_PRESSED)) {
		return;
	}

	int item_count = panel->GetWidgetCount();
	if (item_count <= 0) return;

	int old_selection = selected_item;
	int new_selection = selected_item;

	if (new_selection < 0) {
		// If nothing is selected, any direction press selects the second item (or first if it's the only one)
		switch (event->jhat.value) {
			case SDL_HAT_UP:
			case SDL_HAT_DOWN:
			case SDL_HAT_LEFT:
			case SDL_HAT_RIGHT:
				new_selection = (item_count > 0 ? 0 : -1);
				break;
		}
	}
	else {
		// Something is selected, move from there
		int visible_items = panel->GetHeight() / item_area.h;
		switch(event->jhat.value) {
			case SDL_HAT_UP: // UP
				new_selection--;
				break;
			case SDL_HAT_DOWN: // DOWN
				new_selection++;
				break;
			case SDL_HAT_LEFT: // LEFT
				new_selection -= visible_items;
				break;
			case SDL_HAT_RIGHT: // RIGHT
				new_selection += visible_items;
				break;
			default:
				break;
		}
	}

	if (new_selection < 0) {
		new_selection = 0;
	}
	else if (new_selection >= item_count) {
		new_selection = item_count - 1;
	}

	if(old_selection != new_selection ) {
		EnsureItemVisible(new_selection);
		SetSelectedItem(new_selection);

		GUI_Button *item = GetItem(new_selection);
		if (item) {

			SDL_Event evt;
			Uint16 abs_x, abs_y;
			GetWidgetAbsolutePosition(item, &abs_x, &abs_y);

			evt.type = SDL_MOUSEMOTION;
			evt.motion.x = abs_x;
			evt.motion.y = abs_y;
			SDL_PushEvent(&evt);
			SDL_WarpMouse(evt.motion.x, evt.motion.y);

			if (item_select && item_select_callbacks && new_selection >= 0) {
				item_select_callbacks[new_selection]->Call(item);
			}
		}
	}
}

int GUI_FileManager::Event(const SDL_Event *event, int xoffset, int yoffset) {
	int i, rv;

	rv = GUI_Drawable::Event(event, xoffset, yoffset);

	if ((flags & WIDGET_DISABLED) || !(flags & WIDGET_INSIDE)) {
		return rv;
	}

	xoffset += area.x - x_offset;
	yoffset += area.y - y_offset;

	switch (event->type) {
		
		case SDL_JOYBUTTONDOWN:
		
			switch(event->jbutton.button) {
				case SDL_DC_Y:
				case SDL_DC_X:
					SetFlags(WIDGET_PRESSED);
					SDL_DC_EmulateMouse(SDL_FALSE);
					GUI_GetScreen()->SetJoySelectState(0);
					break;
				default:
					break;
			}
			break;

		case SDL_JOYBUTTONUP:

			switch(event->jbutton.button) {
				case SDL_DC_Y:
				case SDL_DC_X:
					ClearFlags(WIDGET_PRESSED);
					SDL_DC_EmulateMouse(SDL_TRUE);
					GUI_GetScreen()->SetJoySelectState(1);
					break;
				default:
					break;
			}
			break;
		
		case SDL_MOUSEMOTION:
			HandleMouseWheel(event);
			break;
		
		case SDL_KEYDOWN:
			HandleKeyDown(event);
			break;

		case SDL_JOYAXISMOTION:
			if (event->jaxis.axis == 1) { // Analog joystick
				HandleAnalogJoyMotion(event);
			}
			break;

		case SDL_JOYHATMOTION:
			HandleJoyHatMotion(event);
			break;
			
		default:
			break;
	}

	for (i = 0; i < n_widgets; i++) {
		if (widgets[i]->Event(event, xoffset, yoffset)) {
			rv = 1;
			break;
		}
	}
	return rv;
}

void GUI_FileManager::GetWidgetAbsolutePosition(GUI_Widget *widget, Uint16 *x, Uint16 *y) {
	SDL_Rect warea = widget->GetArea();
	*x = warea.x + warea.w / 2;
	*y = warea.y + warea.h / 2;

	GUI_Drawable *p = widget->GetParent();
	
	while (p) {
		warea = p->GetArea();
		int type = p->GetWType();
		
		if (type == WIDGET_TYPE_CONTAINER || type == WIDGET_TYPE_CARDSTACK) {
			*x += warea.x - ((GUI_Container*)p)->GetXOffset();
			*y += warea.y - ((GUI_Container*)p)->GetYOffset();
		}
		else {
			*x += warea.x;
			*y += warea.y;
		}
		p = p->GetParent();
	}
}

void GUI_FileManager::UpdateScrollbarButtons() {
	int scroll_pos = scrollbar->GetVerticalPosition();
	int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
	
	if (scroll_pos <= 0) {
		button_up->SetEnabled(0);
	}
	else {
		button_up->SetEnabled(1);
	}
	if (scroll_pos >= scroll_height) {
		button_down->SetEnabled(0);
	}
	else {
		button_down->SetEnabled(1);
	}
}

void GUI_FileManager::UpdatePanelOffsetAndScrollbar(int new_offset) {
	panel->SetYOffset(new_offset);

	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	if(cont_height < 0) cont_height = 0;

	int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();

	if (new_offset >= cont_height) {
		scrollbar->SetVerticalPosition(scroll_height);
	}
	else if (cont_height > 0 && scroll_height > 0) {
		int sp = (int)(new_offset * (float)scroll_height / cont_height);
		scrollbar->SetVerticalPosition(sp);
	}
	else {
		scrollbar->SetVerticalPosition(0);
	}
	UpdateScrollbarButtons();
}

void GUI_FileManager::ClearSelection() {
	SetSelectedItem(-1);
}

void GUI_FileManager::SetSelectedItem(int index) {

	if (item_selected_normal && selected_item >= 0 && selected_item < panel->GetWidgetCount()) {
		GUI_Button *old_item = GetItem(selected_item);
		if (old_item) {
			old_item->SetNormalImage(item_normal);
			old_item->SetHighlightImage(item_highlight);
			old_item->SetPressedImage(item_pressed);
			old_item->SetDisabledImage(item_disabled);
		}
	}

	selected_item = index;

	if (selected_item >= 0 && selected_item < panel->GetWidgetCount()) {
		GUI_Button *new_item = GetItem(selected_item);
		if (new_item) {
			if(item_selected_normal) new_item->SetNormalImage(item_selected_normal);
			if(item_selected_highlight) new_item->SetHighlightImage(item_selected_highlight);
			if(item_selected_pressed) new_item->SetPressedImage(item_selected_pressed);
			if(item_selected_disabled) new_item->SetDisabledImage(item_selected_disabled);
		}
	}
}

int GUI_FileManager::GetSelectedItem() {
	return selected_item;
}

void GUI_FileManager::EnsureItemVisible(int index) {

	int item_count = panel->GetWidgetCount();
	if (item_count <= 0) return;

	int item_h = item_area.h;
	int panel_h = panel->GetHeight();

	int new_offset = (index * item_h) - (panel_h - item_h) / 2;
	int max_offset = std::max(0, (item_count * item_h) - panel_h);

	new_offset = std::max(0, new_offset + item_h);
	new_offset = std::min(new_offset, max_offset);

	if (panel->GetYOffset() != new_offset) {
		UpdatePanelOffsetAndScrollbar(new_offset);
	}
}


extern "C"
{

GUI_Widget *GUI_FileManagerCreate(const char *name, const char *path, int x, int y, int w, int h)
{
	return new GUI_FileManager(name, path, x, y, w, h);
}

void GUI_FileManagerResize(GUI_Widget *widget, int w, int h) {
	((GUI_FileManager *) widget)->Resize(w, h);
}

void GUI_FileManagerSetPath(GUI_Widget *widget, const char *path) {
	((GUI_FileManager *) widget)->SetPath(path);
}

const char *GUI_FileManagerGetPath(GUI_Widget *widget) {
	return ((GUI_FileManager *) widget)->GetPath();
}

void GUI_FileManagerChangeDir(GUI_Widget *widget, const char *name, int size) {
	((GUI_FileManager *) widget)->ChangeDir(name, size);
}

void GUI_FileManagerScan(GUI_Widget *widget) {
	((GUI_FileManager *) widget)->ReScan();
}

void GUI_FileManagerAddItem(GUI_Widget *widget, const char *name, int size, int time, int attr) {
	((GUI_FileManager *) widget)->AddItem(name, size, time, attr);
}

GUI_Widget *GUI_FileManagerGetItem(GUI_Widget *widget, int index) {
	return (GUI_Widget *)((GUI_FileManager *) widget)->GetItem(index);
}

GUI_Widget *GUI_FileManagerGetItemPanel(GUI_Widget *widget) {
	return (GUI_Widget *)((GUI_FileManager *) widget)->GetItemPanel();
}

void GUI_FileManagerSetItemSurfaces(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	((GUI_FileManager *) widget)->SetItemSurfaces(normal, highlight, pressed, disabled);
}

void GUI_FileManagerSetItemSelectedSurfaces(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	((GUI_FileManager *) widget)->SetItemSelectedSurfaces(normal, highlight, pressed, disabled);
}

void GUI_FileManagerSetItemLabel(GUI_Widget *widget, GUI_Font *font, int r, int g, int b) {
	((GUI_FileManager *) widget)->SetItemLabel(font, r, g, b);
}

void GUI_FileManagerSetItemSize(GUI_Widget *widget, const SDL_Rect *item_r) {
	((GUI_FileManager *) widget)->SetItemSize(item_r);
}

void GUI_FileManagerSetItemClick(GUI_Widget *widget, GUI_CallbackFunction *func) {
	((GUI_FileManager *) widget)->SetItemClick(func);
}

void GUI_FileManagerSetItemContextClick(GUI_Widget *widget, GUI_CallbackFunction *func) {
	((GUI_FileManager *) widget)->SetItemContextClick(func);
}

void GUI_FileManagerSetItemMouseover(GUI_Widget *widget, GUI_CallbackFunction *func) {
	((GUI_FileManager *) widget)->SetItemMouseover(func);
}

void GUI_FileManagerSetItemMouseout(GUI_Widget *widget, GUI_CallbackFunction *func) {
	((GUI_FileManager *) widget)->SetItemMouseout(func);
}

void GUI_FileManagerSetItemSelect(GUI_Widget *widget, GUI_CallbackFunction *func) {
	((GUI_FileManager *) widget)->SetItemSelect(func);
}

void GUI_FileManagerSetScrollbar(GUI_Widget *widget, GUI_Surface *knob, GUI_Surface *background) {
	((GUI_FileManager *) widget)->SetScrollbar(knob, background);
}

void GUI_FileManagerSetScrollbarButtonUp(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	((GUI_FileManager *) widget)->SetScrollbarButtonUp(normal, highlight, pressed, disabled);
}

void GUI_FileManagerSetScrollbarButtonDown(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled)  {
	((GUI_FileManager *) widget)->SetScrollbarButtonDown(normal, highlight, pressed, disabled);
}

int GUI_FileManagerEvent(GUI_Widget *widget, const SDL_Event *event, int xoffset, int yoffset) {
	return ((GUI_FileManager *) widget)->Event(event, xoffset, yoffset);
}

void GUI_FileManagerUpdate(GUI_Widget *widget, int force) {
	((GUI_FileManager *) widget)->Update(force);
}

void GUI_FileManagerRemoveScrollbar(GUI_Widget *widget) {
	((GUI_FileManager *) widget)->RemoveScrollbar();
}

void GUI_FileManagerRestoreScrollbar(GUI_Widget *widget) {
	((GUI_FileManager *) widget)->RestoreScrollbar();
}

int GUI_FileManagerGetSelectedItem(GUI_Widget *widget) {
	return ((GUI_FileManager *) widget)->GetSelectedItem();
}

void GUI_FileManagerSetSelectedItem(GUI_Widget *widget, int index) {
	((GUI_FileManager *) widget)->SetSelectedItem(index);
}

void GUI_FileManagerClearSelection(GUI_Widget *widget) {
	((GUI_FileManager *) widget)->ClearSelection();
}

}
