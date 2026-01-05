/****************************
 * DreamShell ##version##   *
 * console.c                *
 * DreamShell console       *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/


#include "ds.h"
#include "video.h"
#include "console.h"

//#define CONSOLE_DEBUG 1

static ConsoleInformation *DSConsole = NULL;
static Event_t *con_input_event;
static Event_t *con_video_event;
static int console_debug = 0;

static char printf_buf[1024];
static spinlock_t lock = SPINLOCK_INITIALIZER;


void SetConsoleDebug(int mode) {
	spinlock_lock(&lock);
	console_debug = mode;
	spinlock_unlock(&lock);
}

int ds_printf(const char *fmt, ...) {

	char *ptemp, *b;
	va_list args;
	int i, len;

	if(!irq_inside_int()) {
		spinlock_lock(&lock);
	}

	va_start(args, fmt);
	i = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
	va_end(args);

	if(console_debug) {
		dbglog(DBG_DEBUG, printf_buf);
	}

	if(DSConsole != NULL) {

		ptemp = printf_buf;

		if(ConsoleIsVisible()) {
			SetEventActive(con_video_event, 0);
		}

		while((b = strsep(&ptemp, "\n")) != NULL) {

			while(strlen(b) > DSConsole->VChars) {
				CON_NewLineConsole(DSConsole);
				strncpy(DSConsole->ConsoleLines[0], b, DSConsole->VChars);
				DSConsole->ConsoleLines[0][DSConsole->VChars] = '\0';
				b = &b[DSConsole->VChars];
			}

			len = strlen(b);

			if(len > 0 && b[len - 1] == '\r') {
				b[len - 1] = '\0';
				len--;
			}
			else {
				CON_NewLineConsole(DSConsole);
			}

			if(len > DSConsole->VChars) {
				len = DSConsole->VChars;
			}

			memcpy(DSConsole->ConsoleLines[0], b, len);

			if(len < DSConsole->VChars) {
				DSConsole->ConsoleLines[0][len] = '\0';
			}
			else {
				DSConsole->ConsoleLines[0][DSConsole->VChars] = '\0';
			}
		}

		if(ConsoleIsVisible()) {
			CON_UpdateConsole(DSConsole);
			SetEventActive(con_video_event, 1);
		}
	}

	if(!irq_inside_int()) {
		spinlock_unlock(&lock);
	}
	return i;
}


static void Command_Handler(ConsoleInformation *console, char* command) {
	dsystem(command);
}


static char *TabFunction(char* command) {

	if(strstr(command, " ") != NULL) {
		return command;
	}

	Item_list_t *cmds = GetCmdList();
	Cmd_t *c;
	Item_t *i;
	int internal = 0;
	
	SLIST_FOREACH(i, cmds, list) {
		c = (Cmd_t *) i->data;
		if(!strncmp(c->command, command, strlen(command))) {
			strcpy(command, c->command);
			internal = 1;
			break;
		}
	}
	
	if(!internal) {

		file_t fd;
		dirent_t *ent;
		char dir[NAME_MAX];

		snprintf(dir, NAME_MAX, "%s/cmds", getenv("PATH"));
		fd = fs_open(dir, O_RDONLY | O_DIR);

		if(fd != FILEHND_INVALID) {
			
			while ((ent = fs_readdir(fd)) != NULL) {
				if(ent->attr != O_DIR && !strncmp(ent->name, command, strlen(command))) {
					strcpy(command, ent->name);
				}
			}

			fs_close(fd);
		}
	}

	return command;
}


static void ConsoleDrawHandler(void *ds_event, void *param, int action) {

	if(!ConsoleIsVisible()) {
		return;
	}

	switch(action) {
		case EVENT_ACTION_RENDER:
			CON_DrawConsole(DSConsole);
			break;
		case EVENT_ACTION_UPDATE:
			CON_UpdateConsole(DSConsole);
			break;
		default:
			break;
	}
}


static void ConsoleEventHandler(void *ds_event, void *param, int action) {
	
	SDL_Event *event = (SDL_Event *) param;
	
	if(ConsoleIsVisible()) {
		
		CON_Events(event);

		switch(event->type) {
			
			case SDL_KEYDOWN:
				switch(event->key.keysym.sym) {
					case SDLK_F1:
					case SDLK_ESCAPE:
						HideConsole();
					default:
						break;
				}
				break;
			default:
				break;
		} 
		
	} else {
		switch(event->type) {
			case SDL_KEYDOWN:
				switch(event->key.keysym.sym) {
					case SDLK_F1:
						ShowConsole();
						break;
					case SDLK_ESCAPE:
						if (GUI_ScreenGetFocusWidget(GUI_GetScreen()) == NULL) {
							ShowConsole();
						}
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
}

int InitConsole(const char *font, const char *background, int lines, int x, int y, int w, int h, int alpha) {
	
	SDL_Rect Con_rect;

	Con_rect.x = x;
	Con_rect.y = y;
	Con_rect.w = w;
	Con_rect.h = h;

	if((DSConsole = CON_Init(font, GetScreen(), lines, Con_rect)) == NULL)
		return 0;

	CON_SetPrompt(DSConsole, "D$: ");
	
	if(background != NULL) {
		CON_Background(DSConsole, background, 0, 0); 
	}
	
	CON_SetExecuteFunction(DSConsole, Command_Handler);
	CON_SetTabCompletion(DSConsole, TabFunction);
	CON_Alpha(DSConsole, alpha);
	ds_printf("Enter 'help' for print command list.\n\n");
	
	con_input_event = AddEvent(
		"Console_Input",
		EVENT_TYPE_INPUT,
		EVENT_PRIO_DEFAULT,
		ConsoleEventHandler,
		NULL
	);
	con_video_event = AddEvent(
		"Console_Video",
		EVENT_TYPE_VIDEO,
		EVENT_PRIO_DEFAULT,
		ConsoleDrawHandler,
		NULL
	);
	return 1;
}


void ShutdownConsole() {
	RemoveEvent(con_input_event);
	RemoveEvent(con_video_event);
	CON_Destroy(DSConsole);
}


int ToggleConsole() {

	int vis = 0;

	if(DSConsole != NULL) {

		if((vis = ConsoleIsVisible())) {
			HideConsole();
		} else {
			ShowConsole();
		}
	}

	return vis;
}

void ShowConsole() {

	if(ConsoleIsVisible()) {
		return;
	}

	ScreenFadeOutEx(NULL, 1);
	GUI_Disable();

	CON_Show(DSConsole);
	CON_Topmost(DSConsole);
	CON_UpdateConsole(DSConsole);
	CON_DrawConsole(DSConsole);

	SetEventActive(con_video_event, 1);
	ScreenFadeIn();
}

void HideConsole() {

	if(!ConsoleIsVisible()) {
		return;
	}

	ScreenFadeOutEx(NULL, 1);
	SetEventActive(con_video_event, 0);

	CON_Hide(DSConsole);
	CON_Topmost(NULL);

	GUI_Enable();
	ProcessVideoEventsUpdate(NULL);
	ScreenFadeIn();
}

ConsoleInformation *GetConsole() {
	 return DSConsole;
}

int ConsoleIsVisible() {
	return CON_isVisible(DSConsole);
}
