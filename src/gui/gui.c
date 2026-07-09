/****************************
 * DreamShell ##version##   *
 * gui.c                    *
 * DreamShell GUI           *
 * (c)2006-2026 SWAT        *
 ****************************/

#include "ds.h"
#include "gui.h"
#include "console.h"
#include "sfx.h"

static Event_t *gui_input_event;
static Event_t *gui_video_event;
static Event_t *gui_tsunami_video_event;
static Event_t *gui_tsunami_input_event;
static int gui_first_open = 1;
static int gui_prev_width = 0;
static int gui_prev_height = 0;


static void GUI_DrawHandler(void *ds_event, void *param, int action) {
	switch(action) {
		case EVENT_ACTION_RENDER:
			GUI_ScreenDoUpdate(GUI_GetScreen(), 0);
			break;
		case EVENT_ACTION_UPDATE:
			GUI_ScreenDoUpdate(GUI_GetScreen(), 1);
			break;
		default:
			break;
	}
}

static void GUI_TsunamiDrawHandler(void *ds_event, void *param, int action) {

	DSApp *tsunami = (DSApp *) param;
	(void)ds_event;

	if(action == EVENT_ACTION_RENDER && tsunami != NULL) {
		TSU_AppDoFrame(tsunami);
	}
}

static void GUI_TsunamiInputHandler(void *ds_event, void *param, int action) {

	Event_t *event = (Event_t *) ds_event;
	SDL_Event *sdl_event = (SDL_Event *) param;
	DSApp *tsunami = (DSApp *) event->param;

	if(action != EVENT_ACTION_UPDATE || tsunami == NULL || sdl_event == NULL) {
		return;
	}

	if(sdl_event->type == SDL_MOUSEMOTION) {
		TSU_AppDoMouse(tsunami, sdl_event->motion.x, sdl_event->motion.y);
	}
}

static int GUI_EnableTsunami(DSApp *tsunami) {

	DisableScreen();
	TSU_AppBegin(tsunami);

	if(gui_tsunami_video_event == NULL) {
		gui_tsunami_video_event = AddEvent("GUI_Tsunami_Video", EVENT_TYPE_VIDEO, EVENT_PRIO_DEFAULT, GUI_TsunamiDrawHandler, NULL);

		if(gui_tsunami_video_event == NULL) {
			return 0;
		}
	}
	else {
		SetEventActive(gui_tsunami_video_event, 1);
	}

	if(gui_tsunami_input_event == NULL) {
		gui_tsunami_input_event = AddEvent("GUI_Tsunami_Input", EVENT_TYPE_INPUT, EVENT_PRIO_DEFAULT, GUI_TsunamiInputHandler, NULL);

		if(gui_tsunami_input_event == NULL) {
			return 0;
		}

		SetEventActive(gui_tsunami_input_event, 0);
	}
	else {
		SetEventActive(gui_tsunami_input_event, 0);
	}

	return 1;
}

static void GUI_DisableTsunami(DSApp *tsunami) {

	if(gui_tsunami_video_event != NULL) {
		if(tsunami == NULL || gui_tsunami_video_event->param == tsunami) {
			gui_tsunami_video_event->param = NULL;
			SetEventActive(gui_tsunami_video_event, 0);
		}
	}

	if(gui_tsunami_input_event != NULL) {
		if(tsunami == NULL || gui_tsunami_input_event->param == tsunami) {
			gui_tsunami_input_event->param = NULL;
			SetEventActive(gui_tsunami_input_event, 0);
		}
	}
}

static uint8_t scr_joy_state = 0;
static int scr_num = 0;

static void screenshot_callback(void)  {
	static uint32_t last_time = 0;
	uint32_t now = timer_us_gettime64() >> 10;
	
	scr_joy_state = 0;
	
	if ((now - last_time) < 1000) {
		return;
	}
	
	char path[NAME_MAX];
	char *root_dir = getenv("PATH");
	int try_cnt = 99;

	if(!strncmp(root_dir, "/cd", 3)) {
		root_dir = "/rd";
	}

	do {
		snprintf(path, NAME_MAX, "%s/screenshot/ds_scr_%03d.ppm", root_dir, ++scr_num);
		if (!FileExists(path)) {
			break;
		}
	} while(--try_cnt > 0);

	if(try_cnt == 0) {
		return;
	}

	LockVideo();
	ds_sfx_play(DS_SFX_SCRSHOT);
	dsystemf("screenshot %s ppm", path);
	UnlockVideo();
	
	last_time = now;
}


static void GUI_EventHandler(void *ds_event, void *param, int action) {

	SDL_Event *event = (SDL_Event *) param;
	GUI_ScreenEvent(GUI_GetScreen(), event, 0, 0);

	switch(event->type) {
		case SDL_JOYBUTTONDOWN:
			switch(event->jbutton.button) {
				case SDL_DC_X: // X button
					scr_joy_state |= 1;
					break;
				
				case SDL_DC_L: // ltrig
					scr_joy_state |= 4;
					break;
				
				case SDL_DC_R: // rtrig
					scr_joy_state |= 2;
					break;
				
				default:
					scr_joy_state = 0;
					break;
			}
			break;
		
		case SDL_KEYDOWN:
			switch(event->key.keysym.sym) {
				case SDLK_PRINT:
					scr_joy_state = 7;
					break;
				
				case SDLK_F4:
					if (event->key.keysym.mod & KMOD_ALT &&
						!(event->key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT))) {
						App_t *app = GetCurApp();

						if (app && strcmp(app->name, GetMainAppName())) {
							OpenMainApp();
						}
					}
					break;
				
				case SDLK_DELETE:
					if (  event->key.keysym.mod & KMOD_CTRL &&
						  event->key.keysym.mod & KMOD_ALT  &&
						!(event->key.keysym.mod & KMOD_SHIFT)) {
						arch_menu();
					}
					break;
				
				default:
					break;
			}
			break;
		
		default:
			break;
	}

	if(scr_joy_state == 7) {
		screenshot_callback();
	}
}


int InitGUI() {

	GUI_Screen *gui = GUI_RealScreenCreate("screen", GetScreen());

	if(gui == NULL) {
		ds_printf("DS_ERROR: GUI init RealScreen error\n");
		return 0;

	} else {
		GUI_SetScreen(gui);
	}

	gui_input_event = AddEvent(
		"GUI_Input",
		EVENT_TYPE_INPUT,
		EVENT_PRIO_DEFAULT,
		GUI_EventHandler,
		NULL
	);
	gui_video_event = AddEvent(
		"GUI_Video",
		EVENT_TYPE_VIDEO,
		EVENT_PRIO_DEFAULT,
		GUI_DrawHandler,
		NULL
	);
	return 1;
}


void ShutdownGUI() {

	if(gui_tsunami_video_event != NULL) {
		RemoveEvent(gui_tsunami_video_event);
		gui_tsunami_video_event = NULL;
	}

	if(gui_tsunami_input_event != NULL) {
		RemoveEvent(gui_tsunami_input_event);
		gui_tsunami_input_event = NULL;
	}

	if(gui_input_event != NULL) {
		RemoveEvent(gui_input_event);
		gui_input_event = NULL;
	}

	if(gui_video_event != NULL) {
		RemoveEvent(gui_video_event);
		gui_video_event = NULL;
	}

	LockVideo();
	GUI_ObjectDecRef((GUI_Object *) GUI_GetScreen());
	UnlockVideo();

	return; 
}

void GUI_Disable() {
	SDL_DC_EmulateMouse(SDL_FALSE);

	if(gui_input_event != NULL) {
		SetEventActive(gui_input_event, 0);
	}

	if(gui_video_event != NULL) {
		SetEventActive(gui_video_event, 0);
	}
}

void GUI_Enable() {
	if(gui_video_event != NULL) {
		SetEventActive(gui_video_event, 1);
	}

	if(gui_input_event != NULL) {
		SetEventActive(gui_input_event, 1);
	}

	SDL_DC_EmulateMouse(SDL_TRUE);
}

int GUI_IsFirstOpen() {
	return gui_first_open;
}

void GUI_EnableTsunamiInput(void) {

	if(gui_tsunami_input_event != NULL) {
		SetEventActive(gui_tsunami_input_event, 1);
	}
}

void GUI_DisableTsunamiInput(void) {

	if(gui_tsunami_input_event != NULL) {
		SetEventActive(gui_tsunami_input_event, 0);
	}
}

int GUI_OpenApp(App_t *app) {

	if(app == NULL) {
		return 0;
	}

	if(gui_first_open && !ConsoleIsVisible()) {
		gui_first_open = 0;
		while(GetScreenOpacity() > 0.0f) {
			thd_pass();
		}
	}

	if(app->tsunami != NULL) {

		GUI_Disable();

		if(!GUI_EnableTsunami(app->tsunami)) {
			EnableScreen();
			GUI_Enable();
			return 0;
		}

		gui_tsunami_video_event->param = app->tsunami;
		gui_tsunami_input_event->param = app->tsunami;
		return 1;
	}

	if(app->body != NULL) {
		SDL_Rect rect = GUI_WidgetGetArea(app->body);

		GUI_DisableTsunami(NULL);
		GUI_Enable();

		if(rect.w != GetScreenWidth() || rect.h != GetScreenHeight()) {
			gui_prev_width = GetScreenWidth();
			gui_prev_height = GetScreenHeight();
			SetScreenMode(rect.w, rect.h, 0.0f, 0.0f, 1.0f);
		}

		LockVideo();
		GUI_ScreenSetContents(GUI_GetScreen(), app->body);
		UnlockVideo();
		return 1;
	}

	return 0;
}

int GUI_CloseApp(App_t *app) {

	if(app == NULL) {
		return 0;
	}

	if(app->tsunami != NULL) {
		GUI_DisableTsunami(app->tsunami);
		EnableScreen();
		GUI_Enable();
	}
	else if(app->body != NULL) {
		GUI_Screen *screen = GUI_GetScreen();

		LockVideo();
		if(screen != NULL && GUI_ScreenGetContents(screen) == app->body) {
			GUI_ScreenSetContents(screen, NULL);
		}
		UnlockVideo();
	}

	if(gui_prev_width && (gui_prev_width != GetScreenWidth() || gui_prev_height != GetScreenHeight())) {
		SetScreenMode(gui_prev_width, gui_prev_height, 0.0f, 0.0f, 1.0f);
	}

	return 1;
}


Uint32 colorHexToRGB(char *color, SDL_Color *clr) {

	unsigned int r = 0, g = 0, b = 0;
	sscanf(color, "#%02x%02x%02x", &r, &g, &b);

	if(clr) {
		clr->r = r;
		clr->g = g;
		clr->b = b;
	}

	return (r << 16) | (g << 8) | b;
}


Uint32 MapHexColor(char *color, SDL_PixelFormat *format) {

	SDL_PixelFormat *f;
	unsigned int r = 0, g = 0, b = 0, a = 0;

	if(format == NULL) 
		f = GetScreen()->format;
	else 
		f = format;

	if(strlen(color) >= 8) {
				   
		sscanf(color, "#%02x%02x%02x%02x", &a, &r, &g, &b);
		return SDL_MapRGBA(f, r, g, b, a);
	  
	} else {
		 
		sscanf(color, "#%02x%02x%02x", &r, &g, &b);
		return SDL_MapRGB(f, r, g, b);
	}
}



SDL_Color Uint32ToColor(Uint32 c) {
	SDL_Color clr;
	clr.r = c >> 16;
	clr.g = c >> 8;
	clr.b = c;
	return clr; 
}

Uint32 ColorToUint32(SDL_Color c) {
	return (c.r << 16) | (c.g << 8) | c.b;
}


SDL_Surface *SDL_ImageLoad(const char *filename, SDL_Rect *selection) {
	
	SDL_Surface *surface = NULL, *img = NULL;
	SDL_Surface *screen = GetScreen();
	img = IMG_Load(filename);
	   
	if(img != NULL) {
		
		if(selection && selection->w > 0 && selection->h > 0) {
			
			surface = SDL_CreateRGBSurface(screen->flags | SDL_SRCALPHA, 
											selection->w,
											selection->h, 
											screen->format->BitsPerPixel, 
											screen->format->Rmask, 
											screen->format->Gmask, 
											screen->format->Bmask, 
											screen->format->Amask);
			
			//SDL_SetAlpha(surface, SDL_SRCALPHA, 0);
			SDL_Surface *tmp = SDL_DisplayFormat(surface);
			SDL_FreeSurface(surface);
			surface = tmp;
			//SDL_FillRect(surface, NULL, SDL_MapRGBA(screen->format, 255, 255, 255, 255));
			SDL_BlitSurface(img, selection, surface, NULL);
			SDL_FreeSurface(img);

		} else {
			surface = img;
		}
	}
	return surface;
}
