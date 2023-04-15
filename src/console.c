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
#include <arch/spinlock.h>

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

	char *ptemp, *b, *cr;
	va_list args;
	int i;

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
			SetEventState(con_video_event, EVENT_STATE_SLEEP);
		}

		while((b = strsep(&ptemp, "\n")) != NULL) {

			while(strlen(b) > DSConsole->VChars) {
				CON_NewLineConsole(DSConsole);
				strncpy(DSConsole->ConsoleLines[0], b, DSConsole->VChars);
				DSConsole->ConsoleLines[0][DSConsole->VChars] = '\0';
				b = &b[DSConsole->VChars];
			}

			cr = b + strlen(b) - 1;

			if(*cr != '\r') {
				CON_NewLineConsole(DSConsole);
			}

			strncpy(DSConsole->ConsoleLines[0], b, DSConsole->VChars);
			DSConsole->ConsoleLines[0][DSConsole->VChars] = '\0';
		}

		if(ConsoleIsVisible()) {
			CON_UpdateConsole(DSConsole);
			SetEventState(con_video_event, EVENT_STATE_ACTIVE);
		}
	}

	if(!irq_inside_int()) {
		spinlock_unlock(&lock);
	}
	return i;
}


static void *CommandThread(void *command) {
	
	int argc;
	char *argv[32];
	char *str = (char *)command;

	for (argc = 0; argc < 32;) {
		if ((argv[argc] = strsep(&str, " \t\n")) == NULL)
			break;
		if (*argv[argc] != '\0')
			argc++;
	}
	
	if(CallCmd(argc, argv) == CMD_NOT_EXISTS) {
	   ds_printf("DS_ERROR: Command '%s' not found.\n", argv[0]);
	}
	
	free(command);
	return NULL;
}


static void Command_Handler(ConsoleInformation *console, char* command) {
	
	if (command[0] == '\0') {
		return;
	}
	
	int len = strlen(command);

	if(command[len - 1] == '&') {
		command[len - 2] = '\0';
		thd_create(1, CommandThread, strdup(command));
	} else {
		CommandThread(strdup(command));
	}
}


static char *TabFunction(char* command) {
	
	if(strcasestr(command, " ") != NULL) {
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
		char dir[MAX_FN_LEN];

		snprintf(dir, MAX_FN_LEN, "%s/cmds", getenv("PATH"));
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
					case SDLK_ESCAPE:
						ShowConsole();
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
}

static void SetGuiState(int state) {

	Event_t *e = NULL;
	
	e = GetEventByName("GUI_Video");
	
	if(e) {
		SetEventState(e, state);
	}
	
	e = GetEventByName("GUI_Input");
	
	if(e) {
		SetEventState(e, state);
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
	
	con_input_event = AddEvent("Console_Input", EVENT_TYPE_INPUT, ConsoleEventHandler, NULL);
	con_video_event = AddEvent("Console_Video", EVENT_TYPE_VIDEO, ConsoleDrawHandler, NULL);
//	CON_Topmost(DSConsole);
	
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

	if(!ConsoleIsVisible()) {

		CON_Show(DSConsole);
		CON_Topmost(DSConsole);
		CON_UpdateConsole(DSConsole);

		SDL_DC_EmulateMouse(SDL_FALSE);
		SetGuiState(EVENT_STATE_SLEEP);
		SetEventState(con_video_event, EVENT_STATE_ACTIVE);
	}
}

void HideConsole() {

	if(ConsoleIsVisible()) {

		ScreenFadeOut();
		CON_Hide(DSConsole);
		CON_Topmost(NULL);

		while(DSConsole->Visible != CON_CLOSED) thd_sleep(100);

		SDL_DC_EmulateMouse(SDL_TRUE);
		SetEventState(con_video_event, EVENT_STATE_SLEEP);
		SetGuiState(EVENT_STATE_ACTIVE);

		ProcessVideoEventsUpdate(NULL);
		ScreenFadeIn();
	}
}

ConsoleInformation *GetConsole() {
	 return DSConsole;
}

int ConsoleIsVisible() {
	return CON_isVisible(DSConsole);
}
