/** 
* \file    vkb.c
* \brief   DreamShell virtual keyboard
* \date    2007-2026
* \author  SWAT www.dc-swat.ru
*/

#include "ds.h"

#include <math.h>

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


typedef struct virt_kb {

	int cur_surf;
	int visible;

	int row;
	int col;
	float zoom;

	SDL_Event event;
	uint8_t last_joy_state;
	uint8_t cur_joy_state;

	uint32 clr;
	float fade;
	float fade_from;
	int fade_target;
	int fade_run;
	int mouse_was;
	uint64 fade_start;
	uint32 fade_span;

	plx_texture_t *tex[VIRT_KB_SURFACE_COUNT];

	Event_t *input;
	Event_t *video;

} virt_kb_t;


static virt_kb_t vkb;

static void VirtKeyboardEvent(void *da_event, void *param, int action);
static void VirtKeyboardDraw(void *da_event, void *param, int action);

#define VKB_DRAW_PADDING 10
#define VKB_GRID 3
#define VKB_ZOOM_MIN 1.0f
#define VKB_ZOOM_STEP 0.2f
#define VKB_ZOOM_MAX 2.0f
#define VKB_ZOOM_EPSILON (VKB_ZOOM_STEP * 0.5f)
#define VKB_TRIG_RIGHT 2
#define VKB_TRIG_LEFT 4
#define VKB_TRIG_BOTH (VKB_TRIG_RIGHT | VKB_TRIG_LEFT)
#define VKB_Z 460.0f
#define VKB_Z_BOX 461.0f
#define VKB_Z_FRAME 462.0f
#define VKB_FRAME_PX 1.0f
#define VKB_FRAME_ALPHA 0.4f
#define VKB_FADE_MS 120
#define VKB_BLINK_MS 1000
#define VKB_BLINK_PI 3.14159265f
#define VKB_CURSOR_COLOR 0x0000007F
#define VKB_CURSOR_PRESS_COLOR 0xFD241F7F

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



static const char *const vkb_page_name[VIRT_KB_SURFACE_COUNT] = {
	"syms.png",
	"cap_syms.png",
	"nums.png"
};

static void vkb_free_pages(void) {
	int i;

	for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
		if(vkb.tex[i]) {
			plx_txr_destroy(vkb.tex[i]);
			vkb.tex[i] = NULL;
		}
	}
}

static int vkb_load_page(int index) {
	char fn[NAME_MAX];
	plx_texture_t *txr;

	snprintf(fn, sizeof(fn), "%s/gui/keyboard/%s", getenv("PATH"), vkb_page_name[index]);
	txr = plx_txr_load(fn, 0, 0);
	if(!txr) {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
		return -1;
	}

	txr->cxt_trans.gen.culling = PVR_CULLING_NONE;
	plx_txr_setfilter(txr, PVR_FILTER_BILINEAR);
	vkb.tex[index] = txr;
	return 0;
}

static int VirtKeyboardLoading(int reset) {
	int i;

	LockVideo();

	if(reset) {
		vkb_free_pages();
	}

	for(i = 0; i < VIRT_KB_SURFACE_COUNT; i++) {
		if(vkb_load_page(i) < 0) {
			vkb_free_pages();
			UnlockVideo();
			return -1;
		}
	}

	UnlockVideo();
	return 0;
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
	vkb.zoom = VKB_ZOOM_MIN;
	vkb.cur_surf = VIRT_KB_SURFACE_KEYS;
	vkb.visible = 0;
	vkb.clr = VKB_CURSOR_COLOR;
	
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
	SetEventActive(vkb.video, 0);
	return 0;
}



void VirtKeyboardShutdown() {
	SetVirtKeyboardShown(0);
	vkb_free_pages();

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
}


static float vkb_ease(float t) {
	float inv;

	if(t <= 0.0f) {
		return 0.0f;
	}
	if(t >= 1.0f) {
		return 1.0f;
	}
	inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

static void vkb_fade_begin(int show) {
	float travel;

	vkb.fade_from = vkb.fade;
	vkb.fade_target = show;
	travel = show ? (1.0f - vkb.fade_from) : vkb.fade_from;
	if(travel < 0.0f) {
		travel = 0.0f;
	}
	vkb.fade_span = (uint32)((float)VKB_FADE_MS * travel + 0.5f);
	if(vkb.fade_span < 1) {
		vkb.fade_span = 1;
	}
	vkb.fade_start = timer_ms_gettime64();
	vkb.fade_run = 1;
}

static int vkb_fade_step(void) {
	float t;
	float eased;
	uint64 now;

	if(!vkb.fade_run) {
		return 0;
	}

	now = timer_ms_gettime64();
	if(now <= vkb.fade_start) {
		t = 0.0f;
	}
	else {
		t = (float)(now - vkb.fade_start) / (float)vkb.fade_span;
	}
	eased = vkb_ease(t);
	if(vkb.fade_target) {
		vkb.fade = vkb.fade_from + (1.0f - vkb.fade_from) * eased;
	}
	else {
		vkb.fade = vkb.fade_from * (1.0f - eased);
	}
	if(t < 1.0f) {
		return 0;
	}

	vkb.fade_run = 0;
	if(vkb.fade_target) {
		vkb.fade = 1.0f;
		return 0;
	}
	vkb.fade = 0.0f;
	vkb.visible = 0;
	SetVirtKeyboardShown(0);
	return 1;
}

void VirtKeyboardShow() {
	if(vkb.tex[0] == NULL) {
		if(VirtKeyboardLoading(0) < 0) {
			return;
		}
	}
	if(vkb.visible && !vkb.fade_run && vkb.fade >= 1.0f) {
		return;
	}

	if(!vkb.visible) {
		vkb.mouse_was = SDL_DC_MouseEmulated();
	}
	vkb_fade_begin(1);
	vkb.visible = 1;
	SetVirtKeyboardShown(1);
	SDL_DC_EmulateMouse(SDL_FALSE);
	SetEventActive(vkb.video, 1);
}


void VirtKeyboardHide() {
	if(!vkb.visible || (vkb.fade_run && !vkb.fade_target)) {
		return;
	}
	vkb_fade_begin(0);
}


int VirtKeyboardIsVisible() {
	return vkb.visible;
}


void VirtKeyboardToggle() {
	if(vkb.visible && (vkb.fade_target || !vkb.fade_run)) {
		VirtKeyboardHide();
	}
	else {
		VirtKeyboardShow();
	}
}


static void VirtKeyboardResize(void) {
	vkb.zoom += VKB_ZOOM_STEP;
	if(vkb.zoom > VKB_ZOOM_MAX + VKB_ZOOM_EPSILON) {
		vkb.zoom = VKB_ZOOM_MIN;
	}
}


static void VirtKeyboardSetSurface(int index) {
	vkb.cur_surf = index;
}

static void vkb_frame(float x, float y, float w, float h, uint32 color) {
	float t = VKB_FRAME_PX;

	if(t > w * 0.5f) {
		t = w * 0.5f;
	}
	if(t > h * 0.5f) {
		t = h * 0.5f;
	}

	plx_vert_ifp(PLX_VERT, x, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + w, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x, y + t, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT_EOS, x + w, y + t, VKB_Z_FRAME, color, 0.0f, 0.0f);

	plx_vert_ifp(PLX_VERT, x, y + h - t, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + w, y + h - t, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT_EOS, x + w, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);

	plx_vert_ifp(PLX_VERT, x, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + t, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT_EOS, x + t, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);

	plx_vert_ifp(PLX_VERT, x + w - t, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + w, y, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + w - t, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT_EOS, x + w, y + h, VKB_Z_FRAME, color, 0.0f, 0.0f);
}

static void VirtKeyboardDrawHw(void) {
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	int cx = 0;
	int cy = 0;
	int cw = GetScreenWidth();
	int ch = GetScreenHeight();
	float zoom;
	float draw_w;
	float draw_h;
	float cell_w;
	float cell_h;
	float x;
	float y;
	float u;
	float v;
	float box_x;
	float box_y;
	uint32 color;
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
	plx_texture_t *txr;

	txr = vkb.tex[vkb.cur_surf];
	if(!txr || txr->w < 1 || txr->h < 1) {
		return;
	}

	zoom = vkb.zoom;
	if(zoom < VKB_ZOOM_MIN) {
		zoom = VKB_ZOOM_MIN;
	}

	if(ConsoleIsVisible()) {
		ConsoleGetRect(&cx, &cy, &cw, &ch);
	}

	draw_w = (float)txr->w * zoom;
	draw_h = (float)txr->h * zoom;
	x = (float)(cx + cw) - draw_w - (float)VKB_DRAW_PADDING;
	y = (float)(cy + VKB_DRAW_PADDING);
	u = 1.0f;
	v = 1.0f;
	color = PVR_PACK_COLOR(vkb.fade, 1.0f, 1.0f, 1.0f);

	plx_txr_send_hdr(txr, PVR_LIST_TR_POLY, 0);
	plx_vert_ifp(PLX_VERT, x, y, VKB_Z, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, x + draw_w, y, VKB_Z, color, u, 0.0f);
	plx_vert_ifp(PLX_VERT, x, y + draw_h, VKB_Z, color, 0.0f, v);
	plx_vert_ifp(PLX_VERT_EOS, x + draw_w, y + draw_h, VKB_Z, color, u, v);

	cell_w = draw_w / (float)VKB_GRID;
	cell_h = draw_h / (float)VKB_GRID;
	box_x = x + cell_w * (float)vkb.col;
	box_y = y + cell_h * (float)vkb.row;
	r = (uint8)((vkb.clr >> 24) & 0xff);
	g = (uint8)((vkb.clr >> 16) & 0xff);
	b = (uint8)((vkb.clr >> 8) & 0xff);
	a = (uint8)(vkb.clr & 0xff);
	{
		float alpha = (float)a / 255.0f;
		float phase;
		float wave;
		uint64 now;

		if(vkb.clr != VKB_CURSOR_PRESS_COLOR) {
			now = timer_ms_gettime64();
			phase = (float)(now % (uint64)VKB_BLINK_MS) / (float)VKB_BLINK_MS;
			wave = 0.5f + 0.5f * cosf(phase * 2.0f * VKB_BLINK_PI);
			alpha *= wave;
		}
		alpha *= vkb.fade;
		color = PVR_PACK_COLOR(alpha, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
	}

	pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));
	plx_vert_ifp(PLX_VERT, box_x, box_y, VKB_Z_BOX, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, box_x + cell_w, box_y, VKB_Z_BOX, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT, box_x, box_y + cell_h, VKB_Z_BOX, color, 0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT_EOS, box_x + cell_w, box_y + cell_h, VKB_Z_BOX, color, 0.0f, 0.0f);

	color = PVR_PACK_COLOR(VKB_FRAME_ALPHA * vkb.fade, 1.0f, 1.0f, 1.0f);
	vkb_frame(box_x, box_y, cell_w, cell_h, color);
}

static void VirtKeyboardDraw(void *ds_event, void *param, int action) {
	int closed;

	(void)ds_event;
	(void)param;

	if(!vkb.visible) {
		return;
	}

	if(action == EVENT_ACTION_RENDER_POST) {
		closed = vkb_fade_step();
		if(closed) {
			SetEventActive(vkb.video, 0);
			if(!ConsoleIsVisible()) {
				SDL_DC_EmulateMouse(vkb.mouse_was ? SDL_TRUE : SDL_FALSE);
			}
			return;
		}
		if(vkb.fade <= 0.0f) {
			return;
		}
		VirtKeyboardDrawHw();
	}
	else if(action == EVENT_ACTION_UPDATE) {
		SDL_DC_EmulateMouse(SDL_FALSE);
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
	
	return VirtKeyboardKeySyms[vkb.cur_surf].sections[(vkb.row * VKB_GRID) + vkb.col].keys[key];
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
						vkb.clr = VKB_CURSOR_PRESS_COLOR;
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
				vkb.clr = VKB_CURSOR_COLOR;
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
							vkb.cur_joy_state |= VKB_TRIG_RIGHT;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS_CAPS);
						}
						else {
							vkb.cur_joy_state &= ~VKB_TRIG_RIGHT;
							if(!(vkb.cur_joy_state & VKB_TRIG_LEFT)) {
								VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS);
							}
						}

						break;
					case 3: //ltrig

						if(event->jaxis.value) {
							vkb.cur_joy_state |= VKB_TRIG_LEFT;
							VirtKeyboardSetSurface(VIRT_KB_SURFACE_NUMS);
						}
						else {
							vkb.cur_joy_state &= ~VKB_TRIG_LEFT;
							if(!(vkb.cur_joy_state & VKB_TRIG_RIGHT)) {
								VirtKeyboardSetSurface(VIRT_KB_SURFACE_KEYS);
							}
						}

						break;
				}

				break;
			default:
				break;
		}

		if(vkb.cur_joy_state != vkb.last_joy_state && vkb.last_joy_state == VKB_TRIG_BOTH) {
			VirtKeyboardResize();
		}
		vkb.last_joy_state = vkb.cur_joy_state;

	} else {
		
		if(event->type == SDL_JOYBUTTONDOWN && event->jbutton.button == SDL_DC_START) {
			VirtKeyboardToggle();
			event->type = 0;
		}
	}
}
