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
static file_t con_pty_m = FILEHND_INVALID;


void SetConsoleDebug(int mode) {
     console_debug = mode;
}

// TODO improve speed
int ds_printf(const char *fmt, ...) {

    char buff[512];
    char *ptemp, *b, *cr;
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(buff, sizeof(buff), fmt, args);
    va_end(args);
    
	if(console_debug) {
		dbglog(DBG_DEBUG, buff);
	}
	
	if(con_pty_m != FILEHND_INVALID) {
		fs_write(con_pty_m, buff, strlen(buff));
	}

	if(DSConsole != NULL) {

		ptemp = buff;
		
		if(ConsoleIsVisible()) LockVideo();
		
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
			} else {
				*cr = '\0';
			}
			
			strncpy(DSConsole->ConsoleLines[0], b, DSConsole->VChars);
			DSConsole->ConsoleLines[0][DSConsole->VChars] = '\0';
		}
		
		if(ConsoleIsVisible()) {
			UnlockVideo();
			CON_UpdateConsole(DSConsole);
		}
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
//			ScreenChanged();
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
	DestroyConsolePTY();
}


int CreateConsolePTY() {
	
	if(con_pty_m != FILEHND_INVALID) {
		return 1;
	}
	
	int rv = 0;
	//con_pty_m = fs_open("/pty/ma00", O_WRONLY);
	file_t con_pty_s;
	rv = fs_pty_create(NULL, 0, &con_pty_m, &con_pty_s);
	
	if(!rv) {
		fs_close(con_pty_s);
	}
	
	return rv ? 0 : 1;
}

int DestroyConsolePTY() {
	if(con_pty_m != FILEHND_INVALID) {
		fs_close(con_pty_m);
		con_pty_m = FILEHND_INVALID;
		return 1;
	}
	return 0;
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
		
		if(VideoMustLock()) LockVideo();
		SetGuiState(EVENT_STATE_SLEEP);
		SetEventState(con_video_event, EVENT_STATE_ACTIVE);
		SDL_DC_EmulateMouse(SDL_FALSE);
		if(VideoMustLock()) UnlockVideo();
		
		CON_Show(DSConsole);
		CON_Topmost(DSConsole);
		
		while(DSConsole->Visible != CON_OPEN) thd_sleep(50);
	}
}

void HideConsole() {
	
	if(ConsoleIsVisible()) {
		
		ScreenFadeOut();
		CON_Hide(DSConsole);
		CON_Topmost(NULL);
		SDL_DC_EmulateMouse(SDL_TRUE);
		
		while(DSConsole->Visible != CON_CLOSED) thd_sleep(50);
		
		if(VideoMustLock()) LockVideo();
		SetEventState(con_video_event, EVENT_STATE_SLEEP);
		SetGuiState(EVENT_STATE_ACTIVE);
		if(VideoMustLock()) UnlockVideo();
		
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

