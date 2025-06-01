/****************************
 * DreamShell ##version##   *
 * gui.c                    *
 * DreamShell GUI           *
 * (c)2006-2023 SWAT        *
 ****************************/

#include "ds.h"
#include "gui.h"
#include "console.h"


typedef struct TrashItem {

	/* Точка входа SLIST, только для внутреннего использования. */
	SLIST_ENTRY(TrashItem) list; 
	
	/* Ссылка на объект */
	GUI_Object *object;
   
} TrashItem_t;

/*
 * Определение типа для списка, используется SLIST из библиотеки newlib
 */
typedef SLIST_HEAD(TrashItemList, TrashItem) TrashItem_list_t;

static TrashItem_list_t *trash_list;
static Event_t *gui_input_event;
static Event_t *gui_video_event;


static void GUI_DrawHandler(void *ds_event, void *param, int action) {
	switch(action) {
		case EVENT_ACTION_RENDER:
			GUI_ScreenDoUpdate(GUI_GetScreen(), 0);
			break;
		case EVENT_ACTION_UPDATE:
		{
			App_t *app = GetCurApp();
			if(app != NULL && app->body != NULL) {
				GUI_WidgetMarkChanged(app->body);
			}
			break;
		}
		default:
			break;
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
		snprintf(path, NAME_MAX, "%s/screenshot/ds_scr_%03d.png", root_dir, ++scr_num);
		if (!FileExists(path)) {
			break;
		}
	} while(--try_cnt > 0);

	if(try_cnt == 0) {
		return;
	}

	LockVideo();
	dsystemf("screenshot %s png", path);
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
					if (  event->key.keysym.mod & KMOD_ALT &&
						!(event->key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT))) {
						App_t *app = GetCurApp();
						
						if (app && strcmp(app->name, "Main") && strcmp(app->name, "Games Menu")) {
							CloseApp(app, 1);
							
							if ((app = GetAppByName("Main"))) {
								OpenApp(app, NULL);
							}
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
			scr_joy_state = 0;
			break;
	}

	if(scr_joy_state == 7) {
		screenshot_callback();
	}
}


int InitGUI() {

	trash_list = (TrashItem_list_t *) calloc(1, sizeof(TrashItem_list_t)); 

	if(trash_list == NULL) {
		return 0;
	}

	SLIST_INIT(trash_list);

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


void GUI_ClearTrash() {
	
	TrashItem_t *c, *n;
	
	c = SLIST_FIRST(trash_list); 
	
	while(c) {
		n = SLIST_NEXT(c, list); 
		EXPT_GUARD_BEGIN;
			GUI_ObjectDecRef(c->object);
		EXPT_GUARD_CATCH;
			ds_printf("DS_ERROR: Can't delete object %p from trash\n", c->object);
		EXPT_GUARD_END;
		free(c);
		c = n; 
	}
			  
	SLIST_INIT(trash_list);
}

void ShutdownGUI() {

	GUI_ClearTrash();
	free(trash_list);

	RemoveEvent(gui_input_event);
	RemoveEvent(gui_video_event);

	LockVideo();
	GUI_ObjectDecRef((GUI_Object *) GUI_GetScreen());
	UnlockVideo();

	return; 
}

void GUI_Disable() {
	SDL_DC_EmulateMouse(SDL_FALSE);
	SetEventActive(gui_input_event, 0);
	SetEventActive(gui_video_event, 0);
}

void GUI_Enable() {
	SetEventActive(gui_video_event, 1);
	SetEventActive(gui_input_event, 1);
	SDL_DC_EmulateMouse(SDL_TRUE);
}


int GUI_Object2Trash(GUI_Object *object) {

	TrashItem_t *i = (TrashItem_t *) calloc(1, sizeof(TrashItem_t)); 

	if(i == NULL) 
		return 0;
	
	i->object = object;
	ds_printf("DS_WARNING: Added GUI object to trash: %s at %p\n", GUI_ObjectGetName(object), object);

	SLIST_INSERT_HEAD(trash_list, i, list); 
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
