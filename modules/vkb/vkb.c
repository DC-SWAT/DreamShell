/** 
* \file    vkb.c
* \brief   DreamShell virtual keyboard
* \date    2007-2023
* \author  SWAT www.dc-swat.ru
*/

#include "ds.h"
#include "SDL/SDL_rotozoom.h"

#define VIRT_KB_SURFACE_COUNT      3
#define VIRT_KB_SURFACE_KEYS       0
#define VIRT_KB_SURFACE_KEYS_CAPS  1
#define VIRT_KB_SURFACE_NUMS       2

static int inited = 0;

typedef struct virt_kb_section {
	
  int keys[4];
  
} virt_kb_section_t;


typedef struct virt_kb_syms {
	
	virt_kb_section_t sections[9];

} virt_kb_syms_t;


typedef struct virt_kb_surface {
	
	SDL_Surface *s;
	float zoom;
	
} virt_kb_surface_t;



typedef struct virt_kb {

	int cur_surf;
	int visible;
	int redraw;

	int row;
	int col;

	SDL_Event event;
	uint8_t last_joy_state;
	uint8_t cur_joy_state;

	uint32 clr;

	virt_kb_surface_t surface[VIRT_KB_SURFACE_COUNT];

	Event_t *input;
	Event_t *video;

} virt_kb_t;


static virt_kb_t vkb;

static void VirtKeyboardEvent(void *da_event, void *param, int action);
static void VirtKeyboardDraw(void *da_event, void *param, int action);
static void VirtKeyboardUpdateWorkPlace();

#define VKB_DRAW_PADDING 10

static virt_kb_syms_t VirtKeyboardKeySyms[VIRT_KB_SURFACE_COUNT] = 
{
  // Syms
  {{
    {{ 'e', 'f', 'g', 'h' }},   // TopLeft
    {{ 'i', 'j', 'k', 'l' }},   // Top
    {{ 'm', 'n', 'o', 'p' }},   // TopRight
    {{ 'a', 'b', 'c', 'd' }},   // Left
    {{ SDLK_BACKSPACE, ' ', SDLK_RETURN, SDLK_ESCAPE }},  // Center
    {{ 'q', 'r', 's', 't' }},   // Right
    {{ '<', '[', '>', ']' }},   // BottomLeft
    {{ 'y', '.', 'z', ',' }},   // Bottom
    {{ 'u', 'v', 'w', 'x' }}    // BottomRight
  }},

  // Cap syms
  {{
    {{ 'E', 'F', 'G', 'H' }},   // TopLeft
    {{ 'I', 'J', 'K', 'L' }},   // Top
    {{ 'M', 'N', 'O', 'P' }},   // TopRight
    {{ 'A', 'B', 'C', 'D' }},   // Left
    {{ SDLK_DELETE, SDLK_TAB, SDLK_RETURN, SDLK_LCTRL }},  // Center
    {{ 'Q', 'R', 'S', 'T' }},   // Right
    {{ '(', '{', ')', '}' }},   // BottomLeft
    {{ 'Y', '.', 'Z', ',' }},   // Bottom
    {{ 'U', 'V', 'W', 'X' }}    // BottomRight
  }},

  // Nums
  {{
    {{ '1', '2', '3', '4' }},   // TopLeft    
    {{ '5', '6', '7', '8' }},   // Top        
    {{ '9', '\"', '0', '\'' }}, // TopRight   
    {{ '+', '-', '*', '\\' }},  // Left       
    {{ SDLK_DELETE, SDLK_TAB, SDLK_RETURN, '_' }},  // Center
    {{ '@', '|', '?', '/' }},   // Right      
    {{ '#', '~', '!', '`' }},   // BottomLeft 
    {{ ';', '.', ':', '$' }},   // Bottom     
    {{ '&', '^', '%', '=' }}    // BottomRight
  }}
};



static int VirtKeyboardLoading(int reset) {

	char fn[NAME_MAX];
	int i = 0;
	LockVideo();

	if(reset) {

		for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
			if(vkb.surface[i].s != NULL) {
				SDL_FreeSurface(vkb.surface[i].s);
			}
		}
	}

	sprintf(fn, "%s/gui/keyboard/syms.bmp", getenv("PATH"));
	vkb.surface[VIRT_KB_SURFACE_KEYS].s = IMG_Load(fn);

	if(vkb.surface[VIRT_KB_SURFACE_KEYS].s == NULL) {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
		goto error;
	}

	vkb.surface[VIRT_KB_SURFACE_KEYS].zoom = 1.0f;

	sprintf(fn, "%s/gui/keyboard/cap_syms.bmp", getenv("PATH"));
	vkb.surface[VIRT_KB_SURFACE_KEYS_CAPS].s = IMG_Load(fn);

	if(vkb.surface[VIRT_KB_SURFACE_KEYS_CAPS].s == NULL) {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
		goto error;
	}

	vkb.surface[VIRT_KB_SURFACE_KEYS_CAPS].zoom = 1.0f;

	sprintf(fn, "%s/gui/keyboard/nums.bmp", getenv("PATH"));
	vkb.surface[VIRT_KB_SURFACE_NUMS].s = IMG_Load(fn);

	if(vkb.surface[VIRT_KB_SURFACE_NUMS].s == NULL) {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
		goto error;
	}

	vkb.surface[VIRT_KB_SURFACE_NUMS].zoom = 1.0f;
	UnlockVideo();
	return 0;

error:
	for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
		if(vkb.surface[i].s != NULL) {
			SDL_FreeSurface(vkb.surface[i].s);
		}
	}
	UnlockVideo();
	return -1;
}


int VirtKeyboardInit() {
	
	if (inited) {
		return 0;
	}
	
	memset(&vkb, 0, sizeof(vkb));

//	if(VirtKeyboardLoading(0) < 0) {
//		return -1;
//	}

	vkb.col = 1;
	vkb.row = 1;
	vkb.cur_surf = VIRT_KB_SURFACE_KEYS;
	vkb.visible = 0;
	vkb.redraw = 0;
	vkb.clr = 0x0000007F;
	
	vkb.input = AddEvent(
		"VirtKeyboardInput",
		EVENT_TYPE_INPUT,
		EVENT_PRIO_OVERLAY,
		VirtKeyboardEvent,
		NULL
	);
	
	if(!vkb.input) {
		VirtKeyboardShutdown();
		return -1;
	}
	
	vkb.video = AddEvent(
		"VirtKeyboardVideo",
		EVENT_TYPE_VIDEO,
		EVENT_PRIO_OVERLAY,
		VirtKeyboardDraw,
		NULL
	);
	
	if(!vkb.video) {
		VirtKeyboardShutdown();
		return -1;
	}
	
	vkb.event.type = SDL_KEYDOWN;
	vkb.event.key.type = SDL_KEYDOWN;
	return 0;
}



void VirtKeyboardShutdown() {
	int i;
	
	if (!inited) {
		return;
	}
	
	if(VirtKeyboardIsVisible()) {
		VirtKeyboardHide();
	}
	
	if(vkb.input)
		RemoveEvent(vkb.input);
		
	if(vkb.video)
		RemoveEvent(vkb.video);
	
	for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
		if(vkb.surface[i].s != NULL) {
			SDL_FreeSurface(vkb.surface[i].s);
		}
	}
}


void VirtKeyboardShow() {
	
    if(vkb.surface[0].s == NULL) {
        if(VirtKeyboardLoading(0) < 0) {
            return;
        }
    }
	
    vkb.visible = 1;
    vkb.redraw = 1;
    SDL_DC_EmulateMouse(SDL_FALSE);
    SetEventActive(vkb.video, 1);
}


void VirtKeyboardHide() {
    vkb.visible = 0;
    vkb.redraw = 0;
    SetEventActive(vkb.video, 0);
    VirtKeyboardUpdateWorkPlace();

    if(!ConsoleIsVisible()) {
        SDL_DC_EmulateMouse(SDL_TRUE);
    }
}


int VirtKeyboardIsVisible() {
	return vkb.visible;
}


void VirtKeyboardToggle() {
	if(!vkb.visible)
		VirtKeyboardShow();
	else 
		VirtKeyboardHide();
}


static virt_kb_surface_t *VirtKeyboardGetSurface() {
	return &vkb.surface[vkb.cur_surf];
}


static void VirtKeyboardUpdateWorkPlace() {

	VideoEventUpdate_t area;
	virt_kb_surface_t *surf = VirtKeyboardGetSurface();
	SDL_Surface *DScreen = GetScreen();

	area.x = DScreen->w - surf->s->w - VKB_DRAW_PADDING;
	area.y = VKB_DRAW_PADDING;
	area.w = surf->s->w;
	area.h = surf->s->h;

	ProcessVideoEventsUpdate(&area);
}


static void VirtKeyboardResize() {
	int i;

	for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
		if(vkb.surface[i].s != NULL) {

			if(vkb.surface[i].zoom <= 1.8f) {
				vkb.surface[i].zoom += 0.2f;
			} else {
				VirtKeyboardLoading(1);
				VirtKeyboardUpdateWorkPlace();
				VirtKeyboardReDraw();
				return;
			}

			SDL_Surface *s = zoomSurface(vkb.surface[i].s, 1.2f, 1.2f, 1);

			if(s == NULL) {
				ds_printf("DS_ERROR: Virtual keyboard zoom error\n");
			} else {
				SDL_FreeSurface(vkb.surface[i].s);
				vkb.surface[i].s = s;
			}
		}
	}

	VirtKeyboardReDraw();
}


static void VirtKeyboardSetSurface(int index) {
	vkb.cur_surf = index;
}

void VirtKeyboardReDraw() {
	if(vkb.visible) {
		vkb.redraw = 1;
	}
}

static void VirtKeyboardBlit() {

	ConsoleInformation *DSConsole = GetConsole();
	SDL_Surface *DScreen = GetScreen();

	SDL_Surface *dst = ConsoleIsVisible() ? DSConsole->ConsoleSurface : DScreen;
	virt_kb_surface_t *surf = VirtKeyboardGetSurface();
	
	SDL_Rect dest;
	dest.x = dst->w - surf->s->w - VKB_DRAW_PADDING;
	dest.y = VKB_DRAW_PADDING;
	
	int box_w = (surf->s->h / 3) - 1;
	int box_h = (surf->s->w / 3) - 1;
	
	if(surf->zoom > 1.0f) {
		box_w += surf->zoom;
		box_h += surf->zoom;
	}
	
	//SDL_FillRect(scr, 0, SDL_MapRGBA(scr->format, 250, 250, 250, 0));
	SDL_BlitSurface(surf->s, NULL, dst, &dest); 

	boxRGBA(dst, dest.x + (box_w * vkb.col) + vkb.col, 
				dest.y + (box_h * vkb.row) + vkb.row, 
				dest.x + (box_w * vkb.col) + box_w, 
				dest.y + (box_h * vkb.row) + box_h,
				(vkb.clr >> 24) & 0xff, 
				(vkb.clr >> 16) & 0xff, 
				(vkb.clr >> 8) & 0xff, 
				(vkb.clr >> 0) & 0xff);

	vkb.redraw = 0;
	
	if(ConsoleIsVisible())
		// FIXME: WasUnicode used now as async update
		DSConsole->WasUnicode = 1; // CON_UpdateConsole(DSConsole);
	else
		ScreenChanged();
}

static void VirtKeyboardDraw(void *ds_event, void *param, int action) {

	if(action == EVENT_ACTION_RENDER && vkb.redraw) {

		VirtKeyboardBlit();

	} else if(action == EVENT_ACTION_UPDATE) {
		if(vkb.visible) {
			vkb.redraw = 1;
			// GUI_Enable() can switch on this feature.
			SDL_DC_EmulateMouse(SDL_FALSE);
			// VirtKeyboardBlit();
		}
	}
}

static int ButtonToKey(int but) {
	
	int key = 0;
	
	switch(but) {
		case SDL_DC_B:
			key = 2;
			break;
		
		case SDL_DC_A:
			key = 3;
			break;
		
		case SDL_DC_Y:
			key = 1;
			break;
		
		case SDL_DC_X:
			key = 0;
			break;
		
		default:
			break;
	}
	
	return VirtKeyboardKeySyms[vkb.cur_surf].sections[(vkb.row*3) + vkb.col].keys[key];
}



static void VirtKeyboardEvent(void *ds_event, void *param, int action) {
	
	SDL_Event *event = (SDL_Event *) param;
	
	/* Handle text entry focus events */
	switch(event->type) {
		case DS_SHOW_VKB_EVENT:
			if(!VirtKeyboardIsVisible()) {
				VirtKeyboardShow();
			}
			/* Don't process this event further inside vkb */
			event->type = 0;
			return;
		case DS_HIDE_VKB_EVENT:
			if(VirtKeyboardIsVisible()) {
				VirtKeyboardHide();
			}
			/* Don't process this event further inside vkb */
			event->type = 0;
			return;
	}

	if(VirtKeyboardIsVisible()) {

		switch(event->type) {
			
			case SDL_KEYDOWN:
    			switch(event->key.keysym.sym) {
					case SDLK_F1:
					case SDLK_ESCAPE:
						VirtKeyboardReDraw();
					default:
						break;
				}
				break;

			case SDL_JOYBUTTONDOWN:
				
				switch(event->jbutton.button) {
					case SDL_DC_B: 
					case SDL_DC_A:
					case SDL_DC_Y:
					case SDL_DC_X:
					
						vkb.event.key.keysym.sym = ButtonToKey(event->jbutton.button);
						
						switch(vkb.event.key.keysym.sym) {
							case SDLK_LCTRL:
								vkb.event.key.keysym.mod |= KMOD_CTRL;
								vkb.event.key.keysym.sym = SDLK_c;
								break;
							case SDLK_BACKSPACE:
							case SDLK_RETURN:
							case SDLK_ESCAPE:
							case SDLK_DELETE:
								vkb.event.key.keysym.mod = 0;
								break;
							case SDLK_TAB:
								vkb.event.key.keysym.mod = 0;
								vkb.event.key.keysym.unicode = '_';
								vkb.event.key.keysym.sym = SDLK_UNDERSCORE;
								break;
							default:
								vkb.event.key.keysym.unicode = vkb.event.key.keysym.sym;
								vkb.event.key.keysym.sym = 0;
								vkb.event.key.keysym.mod = 0;
								break;
						}
						
						SDL_PushEvent(&vkb.event);
						vkb.clr = 0xFD241F7F;
						VirtKeyboardReDraw();
						break;
					case 3:
						VirtKeyboardHide();
						break;
				}
				/*
				 * 1 -  B
				 * 6 -  X
				 * 5 -  Y
				 * 2 -  A
				 * 3 -  START
				 */
				break; 
			
			case SDL_JOYBUTTONUP:
				vkb.clr = 0x0000007F;
				VirtKeyboardReDraw();
				break;
			
			case SDL_JOYHATMOTION:
				if (event->jhat.hat) { // skip second d-pad
					break;
				}
				
				switch(event->jhat.value) {
					case SDL_HAT_UP:
						vkb.event.key.keysym.sym = SDLK_UP;
						break;
					case SDL_HAT_DOWN:
						vkb.event.key.keysym.sym = SDLK_DOWN;
						break;
					case SDL_HAT_LEFT:
						vkb.event.key.keysym.sym = SDLK_LEFT;
						break;
					case SDL_HAT_RIGHT:    
						vkb.event.key.keysym.sym = SDLK_RIGHT;
						break;
					default:
						break;
				}
				
				vkb.event.key.keysym.unicode = 0;
				vkb.event.key.keysym.mod = 0;
				
				//vkb.event->motion.x = 0;
				//vkb.event->motion.y = 0;
				
				SDL_PushEvent(&vkb.event);
				
				break;
			case SDL_MOUSEMOTION:
				event->type = 0;
				break;
			case SDL_JOYAXISMOTION:
				
				switch(event->jaxis.axis) {
					case 0:
						
						if(event->jaxis.value < -64) {
							vkb.col = 0;
						} else if(event->jaxis.value > 64) {
							vkb.col = 2;
						} else {
							vkb.col = 1;
						}
						
						break;
					case 1:

						if(event->jaxis.value < -64) {
							vkb.row = 0;
						} else if(event->jaxis.value > 64) {
							vkb.row = 2;
						} else {
							vkb.row = 1;
						}
						
						break;
					case 2: // rtrig
					
						if(event->jaxis.value) {
							vkb.cur_joy_state |= 2;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS_CAPS);
						} else {
							vkb.cur_joy_state ^= 2;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS);
						}
						
						break;
					case 3: //ltrig

						if(event->jaxis.value) {
							vkb.cur_joy_state |= 4;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_NUMS);
						} else {
							vkb.cur_joy_state ^= 4;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS);
						}
						
						break;
				}

				VirtKeyboardReDraw();
				break;
			default:
				break;
		}

		if(vkb.cur_joy_state != vkb.last_joy_state && vkb.last_joy_state == 6) {
			VirtKeyboardResize();
		}
		vkb.last_joy_state = vkb.cur_joy_state;

	} else {
		
		if(event->type == SDL_JOYBUTTONDOWN && event->jbutton.button == SDL_DC_START && ScreenIsEnabled()) {
			VirtKeyboardToggle();
		}
	}
}
