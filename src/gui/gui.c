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

static MouseCursor_t *cur_mouse = NULL;
static mutex_t gui_mutex = MUTEX_INITIALIZER;
static TrashItem_list_t *trash_list;
static Event_t *gui_input_event;
static Event_t *gui_video_event;


static void GUI_DrawHandler(void *ds_event, void *param, int action) {
	
    switch(action) {
        case EVENT_ACTION_RENDER:
            DrawActiveMouseCursor();
            break;
        case EVENT_ACTION_UPDATE:
        {
			App_t *app = GetCurApp();
			
			// TODO optimize update area
			
			if(app != NULL && app->body != NULL) {
				//GUI_WidgetErase(app->body, (SDL_Rect *)param);
				GUI_WidgetMarkChanged(app->body);
			} else {
				//GUI_ScreenErase(GUI_GetScreen(), (SDL_Rect *)param);
				GUI_ScreenDoUpdate(GUI_GetScreen(), 1); 
			}

			UpdateActiveMouseCursor();
			break;
        }
        default:
            break;
    }
}

static uint64_t last_saved_time = 0;
static uint8_t last_joy_state = 0;

static void screenshot_callback(void) 
{
	char path[MAX_FN_LEN];
	char *home_dir = getenv("PATH");
	int i;
	uint64 cur_time = timer_ms_gettime64();
	
	if(!strncmp(home_dir, "/cd", 3) || ((cur_time - last_saved_time) < 5000)) 
	{
		return;
	}
	
	last_saved_time = cur_time;
	
	for(i=0;;i++)
	{
		snprintf(path, MAX_FN_LEN, "%s/screenshot/ds_scr_%03d.png", home_dir, i);
		if(!FileExists(path)) break;
		if(i == 999)  return;
	}
	
	char *arg[3] = {"screenshot", path, "png"};

	LockVideo();
	CallCmd(3, arg);
	UnlockVideo();
}

static void GUI_EventHandler(void *ds_event, void *param, int action) {
		
	SDL_Event *event = (SDL_Event *) param;
	GUI_ScreenEvent(GUI_GetScreen(), event, 0, 0);
	
	
	switch(event->type) 
	{
		case SDL_JOYBUTTONDOWN:
			switch(event->jbutton.button) 
			{
				case 6: // X button
					last_joy_state |= 1;
					break;
			}
			break;
		case SDL_JOYAXISMOTION:
			switch(event->jaxis.axis)
			{
				case 2: // rtrig
					if(event->jaxis.value)
						last_joy_state |= 2;
					break;
				case 3: //ltrig
					if(event->jaxis.value)
						last_joy_state |= 4;
					break;
			}
			break;
		default:
			last_joy_state = 0;
			break;
	}
	
	if(last_joy_state == 7)
		screenshot_callback();
/*
	switch(event->type) {
		case SDL_KEYDOWN:
			UpdateActiveMouseCursor();
			break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			UpdateActiveMouseCursor();
			break;
		default:
			break;
	}
*/
}


int InitGUI() {

    trash_list = (TrashItem_list_t *) calloc(1, sizeof(TrashItem_list_t)); 
	
    if(trash_list == NULL) 
		return 0;

    SLIST_INIT(trash_list);

    LockVideo();
    GUI_Screen *gui = GUI_RealScreenCreate("screen", GetScreen());

    if(gui == NULL) {

        ds_printf("DS_ERROR: GUI init RealScreen error\n");
        UnlockVideo();
        return 0;
        
    } else {
        GUI_SetScreen(gui);
    }

    UnlockVideo();

    gui_input_event = AddEvent("GUI_Input", EVENT_TYPE_INPUT, GUI_EventHandler, NULL);
    gui_video_event = AddEvent("GUI_Video", EVENT_TYPE_VIDEO, GUI_DrawHandler, NULL);

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

    mutex_destroy(&gui_mutex);
    return; 
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


MouseCursor_t *CreateMouseCursor(const char *fn, SDL_Surface *surface) {

    MouseCursor_t *c;

    c = (MouseCursor_t*) calloc(1, sizeof(MouseCursor_t));

    if(c == NULL) {
        ds_printf("DS_ERROR: Malloc error\n");
        return NULL;
    }

    c->cursor = surface ? surface : IMG_Load(fn);
    c->draw = 0;

    if(c->cursor == NULL) {
        free(c);
        ds_printf("DS_ERROR: Can't create surface\n");
        return NULL; 
    }

    c->bg = SDL_CreateRGBSurface(c->cursor->flags, c->cursor->w, c->cursor->h,
                                    c->cursor->format->BitsPerPixel, c->cursor->format->Rmask,
                                    c->cursor->format->Gmask, c->cursor->format->Bmask,
                                    c->cursor->format->Amask);

    if(c->bg == NULL) {
        SDL_FreeSurface(c->cursor);
        free(c);
        ds_printf("DS_ERROR: Can't create background for mouse cursor\n");
        return NULL;
    }

    return c;
}


void DestroyMouseCursor(MouseCursor_t *c) {
    SDL_FreeSurface(c->bg);
    SDL_FreeSurface(c->cursor);
    free(c);
}

static int old_x = 0, old_y = 0;

void DrawMouseCursor(MouseCursor_t *c/*, SDL_Event *event*/) {
	
	SDL_Rect src, dst;
	SDL_Surface *scr = NULL;
	int x = 0;//c->x;//event->motion.x;
	int y = 0;//c->y;//event->motion.y;

	SDL_GetMouseState(&x, &y);
    	
    if(c && (c->draw || (old_x != x || old_y != y))) {

    	old_x = x;
    	old_y = y;
    	c->draw = 1;

    	scr = GetScreen();
		src.x = 0;
		src.y = 0;
		src.w = c->cursor->w;
		src.h = c->cursor->h;

    	
    	if (x + c->cursor->w <= scr->w) {
    		src.w = c->cursor->w;
    	} else {
    		src.w = scr->w - x - 1;
    	}
		
    	if (y + c->cursor->h <= scr->h) {
    		src.h = c->cursor->h;
		} else {
    		src.h = scr->h - y - 1;
		}

    	dst.x = x;
    	dst.y = y;
    	dst.w = src.w;
    	dst.h = src.h;
    	
    	if(c->bg) {
			SDL_BlitSurface(c->bg, &c->src, scr, &c->dst);
		}
    }

	GUI_ScreenDoUpdate(GUI_GetScreen(), 0);
    
    if(c && c->draw) {

		SDL_BlitSurface(scr, &dst, c->bg, &src);
		SDL_BlitSurface(c->cursor, &src, scr, &dst);
		ScreenChanged();
    
		c->src = src;
		c->dst = dst;
		c->draw = 0;
    }

}


void DrawActiveMouseCursor() {
	DrawMouseCursor(cur_mouse);
}

void SetActiveMouseCursor(MouseCursor_t *c) {
	if(VideoMustLock()) LockVideo();
	cur_mouse = c;
	if(VideoMustLock()) UnlockVideo();
}


MouseCursor_t *GetActiveMouseCursor() {
	return cur_mouse;
}


void WaitDrawActiveMouseCursor() {
	while(cur_mouse->draw) thd_pass();
}

void UpdateActiveMouseCursor() {
	cur_mouse->draw = 1;
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

