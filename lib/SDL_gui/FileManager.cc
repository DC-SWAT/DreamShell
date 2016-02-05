#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C"
{
#include <kos.h>
int ds_printf(const char *fmt, ...); 
void LockVideo();
void UnlockVideo();
int VideoMustLock();
void UpdateActiveMouseCursor();
void ScreenWaitUpdate();
void LockInput();
void UnlockInput();
int InputMustLock();
void SDL_DC_EmulateMouse(SDL_bool value);

}


GUI_FileManager::GUI_FileManager(const char *aname, const char *path, int x, int y, int w, int h)
: GUI_Container(aname, x, y, w, h) {
	
	strncpy(cur_path, path != NULL ? path : "/", MAX_FN_LEN);
	rescan = 1;
	joy_sel = 0;
	//use_browse = browse;
	
	item_area.w = w - 20; 
	item_area.h = 20; 
	item_area.x = 0;
	item_area.y = 0;
	
	item_normal = new GUI_Surface("normal", SDL_HWSURFACE, item_area.w, item_area.h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	item_highlight = new GUI_Surface("highlight", SDL_HWSURFACE, item_area.w, item_area.h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	item_disabled = new GUI_Surface("disabled", SDL_HWSURFACE, item_area.w, item_area.h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	item_pressed = new GUI_Surface("pressed", SDL_HWSURFACE, item_area.w, item_area.h, 16, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	item_normal->Fill(NULL, 0xFF000000);
	item_highlight->Fill(NULL, 0x00FFFFFF);
	item_pressed->Fill(NULL, 0x00FFFFFF);
	item_disabled->Fill(NULL, 0xFF000000);
	
	item_click = NULL;
	item_context_click = NULL;
	item_mouseover = NULL;
	item_mouseout = NULL;
 
	item_label_font = NULL;
	item_label_clr.r = 0;
	item_label_clr.g = 0;
	item_label_clr.b = 0;
	
	Build();
//	Scan();
}

GUI_FileManager::~GUI_FileManager() {
	item_normal->DecRef();
	item_highlight->DecRef();
	item_pressed->DecRef();
	item_disabled->DecRef();
	
	if(item_label_font)
		item_label_font->DecRef();
		
	if(scrollbar)
		scrollbar->DecRef();
		
	if(button_up)
		button_up->DecRef();
		
	if(button_down)
		button_down->DecRef();
}


void GUI_FileManager::Build() {
	
	panel = new GUI_Panel("panel", 0, 0, item_area.w, area.h);
	
	if(panel) {
		AddWidget(panel);
		panel->DecRef();
	}
	
	GUI_EventHandler < GUI_FileManager > *cb;
	
	scrollbar = new GUI_ScrollBar("scrollbar", item_area.w, 20, 20, area.h - 40);
	
	if(scrollbar) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::AdjustScrollbar);
		scrollbar->SetMovedCallback(cb);
		cb->DecRef();
		AddWidget(scrollbar);
//		scrollbar->DecRef();
	}

	button_up = new GUI_Button("button_up", item_area.w, 0, 20, 20);
	
	if(button_up) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::ScrollbarButtonEvent);
		button_up->SetClick(cb);
		cb->DecRef();
		AddWidget(button_up);
//		button_up->DecRef();
	}
	
	button_down = new GUI_Button("button_down", item_area.w, area.h - 20, 20, 20);
	
	if(button_down) {
		cb = new GUI_EventHandler < GUI_FileManager > (this, &GUI_FileManager::ScrollbarButtonEvent);
		button_down->SetClick(cb);
		cb->DecRef();
		AddWidget(button_down);
//		button_down->DecRef();
	}
}


void GUI_FileManager::Resize(int w, int h)
{
	area.w = w;
	area.h = h;
	item_area.w = w - 20;
	
	if(panel)
		panel->SetSize(item_area.w, area.h);
		
	if(button_up)
		button_up->SetPosition(item_area.w, 0);
		
	if(button_down)
		button_down->SetPosition(item_area.w, area.h - 20);
	
	if(scrollbar)
		scrollbar->SetHeight(area.h - 40);
		
	MarkChanged();
}


void GUI_FileManager::AdjustScrollbar(GUI_Object * sender) {
	int cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	
	if(cont_height <= 0) {
		return;
	}

	int scroll_pos = scrollbar->GetVerticalPosition();
	int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
	panel->SetYOffset(scroll_pos * ((cont_height / scroll_height) + 1));
}

void GUI_FileManager::ScrollbarButtonEvent(GUI_Object * sender) {

	int scroll_pos, scroll_height, cont_height;
	
	cont_height = (panel->GetWidgetCount() * item_area.h) - panel->GetHeight();
	
	if(cont_height <= 0) {
		return;
	}
	
	scroll_pos = scrollbar->GetVerticalPosition();
	scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
	
	if(sender == button_up) {
		
		scroll_pos -= scroll_height / 10;
		
		if(button_down->GetFlags() & WIDGET_DISABLED) {
			button_down->SetEnabled(1);
		}
		
		if(scroll_pos < 0) {
			scroll_pos = 0;
			button_up->SetEnabled(0);
		}
		
	} else if(sender == button_down) {
		
		scroll_pos += scroll_height / 10;
		if(button_up->GetFlags() & WIDGET_DISABLED) {
			button_up->SetEnabled(1);
		}
		
		if(scroll_pos > scroll_height) {
			scroll_pos = scroll_height;
			button_down->SetEnabled(0);
		}
	}

	scrollbar->SetVerticalPosition(scroll_pos);
	panel->SetYOffset(scroll_pos * ((cont_height / scroll_height) + 1));
}


void GUI_FileManager::SetPath(const char *path) {
	strncpy(cur_path, path, MAX_FN_LEN);
//	ds_printf("GUI_FileManager::SetPath: %s\n", cur_path);
//	rescan = 1;
//	Scan();
	MarkChanged();
}

const char *GUI_FileManager::GetPath() {
	return (const char*)cur_path;
}

void GUI_FileManager::ChangeDir(const char *name, int size) {
	char *b, path[MAX_FN_LEN];
	int s;
	file_t fd;
	
	if(name/* && size < 0*/) {
		
		if(VideoMustLock()) LockVideo();

		if(size == -2) {

			b = strrchr(cur_path, '/');

			if(b) {

				s = b - cur_path;

				if(s > 0)
					cur_path[s] = '\0';
				else
					cur_path[1] = '\0';

//				rescan = 1;
//				Scan();
				MarkChanged();
//				ds_printf("strlen: %d %d\n", strlen(b), s);
			}
			
		} else {
			
			memset(path, 0, MAX_FN_LEN);

			if(strlen(cur_path) > 1) {
				snprintf(path, MAX_FN_LEN, "%s/%s", cur_path, name);
			} else {
				snprintf(path, MAX_FN_LEN, "%s%s", cur_path, name);
			}

			s = strlen(cur_path) + strlen(name) + 1;
			path[s > MAX_FN_LEN-1 ? MAX_FN_LEN-1 : s] = '\0';

//			ds_printf("%s: %s %s %s\n", __func__, name, path, cur_path);

			fd = fs_open(path, O_RDONLY | O_DIR);

			if(fd != FILEHND_INVALID) {
				fs_close(fd);
				strncpy(cur_path, path, MAX_FN_LEN);
//				rescan = 1;
//				Scan();
				MarkChanged();
			} else {
				ds_printf("GUI_FileManager: Can't open dir: %s\n", path);
			}
		}
		
		if(VideoMustLock()) {
			UnlockVideo();
			ScreenWaitUpdate();
		}
	}
}
/*
void GUI_FileManager::ItemEvent(GUI_Object * sender, GUI_CallbackFunction *func) {
	GUI_EventHandler < GUI_FileManager > *cb;
	void *data;
		
	cb = (GUI_EventHandler < GUI_FileManager > *)sender;
	data = cb->GetData();
	
	sender->IncRef();
	func(data);
	
	if(func == item_click && use_browse) {
		ChangeDir(data);
	}
	
	if(sender->GetRef() == 1) {
		sender->Trash();
	} else {
		sender->DecRef();
	}
}

void GUI_FileManager::ItemClickEvent(GUI_Object * sender) {
	if(item_click) {
		ItemEvent(sender, item_click);
	}
}

void GUI_FileManager::ItemContextClickEvent(GUI_Object * sender) {
	if(item_context_click) {
		ItemEvent(sender, item_context_click);
	}
}

void GUI_FileManager::ItemMouseoverEvent(GUI_Object * sender) {
	if(item_mouseover) {
		ItemEvent(sender, item_mouseover);
	}
}

void GUI_FileManager::ItemMouseoutEvent(GUI_Object * sender) {
	if(item_mouseout) {
		ItemEvent(sender, item_mouseout);
	}
}*/

void GUI_FileManager::Scan() {

	file_t f;
	dirent_t *ent;
	
	f = fs_open(cur_path, O_RDONLY | O_DIR);
//	ds_printf("GUI_FileManager::Scan: Scaning %s\n", cur_path);
	
	if(f == FILEHND_INVALID) {
		ds_printf("GUI_FileManager: Can't open dir: %s\n", cur_path);
		return;
	}
	
//	ds_printf("GUI_FileManager: Remove all\n");
	panel->RemoveAllWidgets();
	panel->SetYOffset(0);
	
	if(strlen(cur_path) > 1) {
		const char b[] = {0x2E, 0x2E, 0};
		AddItem(b, -2, 0, O_DIR);
	}
	
	while ((ent = fs_readdir(f)) != NULL) {
		if(strncasecmp(ent->name, ".", MAX_FN_LEN) && strncasecmp(ent->name, "..", MAX_FN_LEN)) {
			AddItem(ent->name, ent->size, ent->time, ent->attr);
		}
	}
	
	fs_close(f);
	rescan = 0;
//	UpdateActiveMouseCursor();
//	MarkChanged();
}

void GUI_FileManager::ReScan() {
//	rescan = 1;
	MarkChanged();
}

void GUI_FileManager::AddItem(const char *name, int size, int time, int attr) {
	
	GUI_Button *bt;
	GUI_Callback *cb;
//	GUI_EventHandler < GUI_FileManager > *cb;
	GUI_Label *lb;
	dirent_fm_t *prm;
	int need_free = 1;
	int cnt = panel->GetWidgetCount();

	bt = new GUI_Button(name, item_area.x, item_area.y + (cnt * item_area.h), item_area.w, item_area.h);
	
	bt->SetNormalImage(item_normal);
	bt->SetHighlightImage(item_highlight);
	bt->SetPressedImage(item_pressed);
	bt->SetDisabledImage(item_disabled);
	
//	ds_printf("GUI_FileManager::AddItem: %s %d\n", name, size);

	if(item_click || item_context_click || item_mouseover || item_mouseout) {
		
		prm = (dirent_fm_t *) malloc(sizeof(dirent_fm_t));
		
		if(prm != NULL) {
			strncpy(prm->ent.name, name, MAX_FN_LEN);
			prm->ent.size = size;
			prm->ent.time = time;
			prm->ent.attr = attr;
			prm->obj = this;
			prm->index = cnt;
		}
		
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
			cb = new GUI_Callback_C(item_mouseout, (need_free ? (GUI_CallbackFunction*)free : NULL), (void*)prm);
			bt->SetMouseout(cb);
			cb->DecRef();
		}
	}
	
	if(item_label_font) {
//		SDL_Rect ts = item_label_font->GetTextSize(name);
		
		lb = new GUI_Label(name, 2, 0, item_area.w-4, item_area.h, item_label_font, name);
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
//	rescan = 1;
	MarkChanged();
}

void GUI_FileManager::SetItemSurfaces(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled) {
	
	if(normal) {
		item_area.w = normal->GetWidth(); 
		item_area.h = normal->GetHeight(); 
	}
	
	GUI_ObjectKeep((GUI_Object **) &item_normal, normal);
	GUI_ObjectKeep((GUI_Object **) &item_highlight, highlight);
	GUI_ObjectKeep((GUI_Object **) &item_pressed, pressed);
	GUI_ObjectKeep((GUI_Object **) &item_disabled, disabled);	
//	rescan = 1;
	MarkChanged();
}

void GUI_FileManager::SetItemSize(const SDL_Rect *item_r) {
	item_area.w = item_r->w;
	item_area.h = item_r->h;
	item_area.x = item_r->x;
	item_area.y = item_r->y;
//	rescan = 1;
	MarkChanged();
}

void GUI_FileManager::SetItemClick(GUI_CallbackFunction *func) {
	item_click = func;
	MarkChanged();
}

void GUI_FileManager::SetItemContextClick(GUI_CallbackFunction *func) {
	item_context_click = func;
	MarkChanged();
}

void GUI_FileManager::SetItemMouseover(GUI_CallbackFunction *func) {
	item_mouseover = func;
	MarkChanged();
}

void GUI_FileManager::SetItemMouseout(GUI_CallbackFunction *func) {
	item_mouseout = func;
	MarkChanged();
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
		rescan = 1;
	}
	
	if(rescan)  {
//		if(InputMustLock()) LockInput();
		Scan();
//		if(InputMustLock()) UnlockInput();
	}

	for (i=0; i<n_widgets; i++)
		widgets[i]->DoUpdate(force);
		
	if(force || rescan) {
		UpdateActiveMouseCursor();
	}
}


void GUI_FileManager::calc_item_offset(GUI_Widget *widget, Uint16 *x, Uint16 *y) {
	GUI_Drawable *w = NULL;
	SDL_Rect warea;
	int type = 0;

	warea = widget->GetArea();
	*x = warea.x + (warea.w / 2);
	*y = warea.y + (warea.h / 2);

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

int GUI_FileManager::Event(const SDL_Event *event, int xoffset, int yoffset) {
	
	if (flags & WIDGET_DISABLED) return 0;
	
	int i;
	
	xoffset += area.x - x_offset;
	yoffset += area.y - y_offset;
	
	switch (event->type) {
		
		case SDL_JOYBUTTONDOWN:
		
			switch(event->jbutton.button) {
				case 6:
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
				case 6:
					ClearFlags(WIDGET_PRESSED);
					thd_sleep(150);
					SDL_DC_EmulateMouse(SDL_TRUE);
					GUI_GetScreen()->SetJoySelectState(1);
					UpdateActiveMouseCursor();
					
					break;
				default:
					break;
			}
			break;
			
		case SDL_JOYAXISMOTION:
		
			switch(event->jaxis.axis) {
				case 1:
					if(flags & WIDGET_PRESSED) {
						
						int scroll_height = scrollbar->GetHeight() - scrollbar->GetKnobImage()->GetHeight();
						int sp = (scroll_height / 2) + ((event->jaxis.value / (256 / 100)) * (scroll_height / 100));
						
						if(sp > scroll_height) 
							sp = scroll_height;
							
						if(sp < 0) 
							sp = 0;
						
						scrollbar->SetVerticalPosition(sp);
						AdjustScrollbar(NULL);
						ScreenWaitUpdate();
					}
					break;
				default:
					break;
			}
			break;
			
		case SDL_JOYHATMOTION:
		
			if(flags & WIDGET_PRESSED) {
				
				if(VideoMustLock()) LockVideo();
				
//				ds_printf("JHAT: %02X\n", event->jhat.value);
				SDL_Event evt;
				int cnt = panel->GetWidgetCount() - 1;
				int inc = 1;
				
				switch(event->jhat.value) {
					case 0x0E: // UP
						if(joy_sel > 0)
							joy_sel--;
						inc = -1;
						break;
					case 0x0B: //DOWN
						if(joy_sel < cnt)
							joy_sel++;
						break;
					case 0x07: //LEFT
						joy_sel = 0;
						break;
					case 0x0D: //RIGHT
						joy_sel = cnt;
						inc = -1;
						break;
					default:
						break;
				}
				
				GUI_Widget *w = panel->GetWidget(joy_sel);
				
				while(joy_sel > 0 && joy_sel < cnt) {
					
					if(panel->IsVisibleWidget(w))
						break;
					
					joy_sel += inc;
					w = panel->GetWidget(joy_sel);
				}
				
				evt.type = SDL_MOUSEMOTION;
				calc_item_offset(w, &evt.motion.x, &evt.motion.y);
				SDL_PushEvent(&evt);
				SDL_WarpMouse(evt.motion.x, evt.motion.y);
				
				if(VideoMustLock()) UnlockVideo();
			}
			
		default:
			break;
	}
	
//	if(rescan) Scan();
		
	for (i = 0; i < n_widgets; i++) {
		if (widgets[i]->Event(event, xoffset, yoffset))
			return 1;
	}

//	UpdateActiveMouseCursor();

	return GUI_Drawable::Event(event, xoffset, yoffset);
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

}
