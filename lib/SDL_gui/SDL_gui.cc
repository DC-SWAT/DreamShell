#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"
//#include <SDL/SDL_thread.h>

static GUI_Screen *screen = NULL;
/*
static int gui_running = 0;
static SDL_mutex *gui_mutex=0;
static Uint32 gui_thread=0;
*/
GUI_Screen *GUI_GetScreen()
{
	return screen;
}

void GUI_SetScreen(GUI_Screen *new_screen)
{
	// FIXME incref/decref?
	screen = new_screen;
}
#if 0
int GUI_GetRunning()
{
	return gui_running;
}

void GUI_SetRunning(int value)
{
	gui_running = value;
}

int GUI_Lock()
{
	return SDL_LockMutex(gui_mutex);
}

int GUI_Unlock()
{
	return SDL_UnlockMutex(gui_mutex);
}

int GUI_MustLock()
{
	return (SDL_ThreadID() != gui_thread);
}

int GUI_Init(void)
{
	SDL_EnableUNICODE(1);
	gui_mutex = SDL_CreateMutex();

	return 0;
}

void GUI_SetThread(Uint32 id)
{
	gui_thread = id;
}

void GUI_Run()
{
	SDL_Event event;
	
	GUI_SetThread(SDL_ThreadID());
	
	GUI_SetRunning(1);
	
	/* update the screen */
	screen->DoUpdate(1);

	/* process events */
	while (GUI_GetRunning())
	{
		SDL_WaitEvent(&event);
		do
		{
			GUI_Lock();
			screen->Event(&event, 0, 0);
			GUI_Unlock();
		} while (SDL_PollEvent(&event));

		GUI_Lock();
		screen->DrawMouse();
		screen->DoUpdate(0);
		GUI_Unlock();
	}
}





void GUI_Quit()
{
	SDL_DestroyMutex(gui_mutex);
}

#endif

int GUI_ClipRect(SDL_Rect *sr, SDL_Rect *dr, const SDL_Rect *clip)
{
	int dx = dr->x;
	int dy = dr->y;
	int dw = dr->w;
	int dh = dr->h;
	int cx = clip->x;
	int cy = clip->y;
	int cw = clip->w;
	int ch = clip->h;
#ifdef DEBUG_CLIP		
	printf(">>> ");
	if (sr != NULL)
		printf("sr: %3d %3d %3d %3d ", sr->x, sr->y, sr->w, sr->h);
	printf("dr: %3d %3d %3d %3d ", dr->x, dr->y, dr->w, dr->h);
	printf("clip: %3d %3d %3d %3d\n", clip->x, clip->y, clip->w, clip->h);
#endif
	int adj = cx - dx;
	if (adj > 0)
	{
		if (adj > dw)
			return 0;
		dx += adj;
		dw -= adj;
		if (sr != NULL)
		{
			sr->x += adj;
			sr->w -= adj;
		}
#ifdef DEBUG_CLIP		
		printf("X ADJ %d\n", adj);
#endif
	}
	adj = cy - dy;
	if (adj > 0)
	{
		if (adj > dh)
			return 0;
		dy += adj;
		dh -= adj;
		if (sr != NULL)
		{
			sr->y += adj;
			sr->h -= adj;
		}
#ifdef DEBUG_CLIP		
		printf("Y ADJ %d\n", adj);
#endif
	}
	adj = (dx + dw) - (cx + cw);
	if (adj > 0)
	{
#ifdef DEBUG_CLIP		
		printf("W ADJ %d\n", adj);
#endif
		if (adj > dw)
			return 0;
		dw -= adj;
		if (sr != NULL)
			sr->w -= adj;  	
	}
	adj = (dy + dh) - (cy + ch);
	if (adj > 0)
	{
#ifdef DEBUG_CLIP		
		printf("H ADJ %d\n", adj);
#endif
		if (adj > dh)
			return 0;
		dh -= adj;
		if (sr != NULL)
			sr->h -= adj;
	}
#ifdef DEBUG_CLIP		
	printf("<<< ");
	if (sr != NULL)
		printf("sr: %3d %3d %3d %3d ", sr->x, sr->y, sr->w, sr->h);
	printf("dr: %3d %3d %3d %3d ", dr->x, dr->y, dr->w, dr->h);
	printf("clip: %3d %3d %3d %3d\n", clip->x, clip->y, clip->w, clip->h);
#endif
	dr->x = dx;
	dr->y = dy;
	dr->w = dw;
	dr->h = dh;
	return 1;
}

void GUI_TriggerUpdate()
{
	SDL_Event e;
    
	e.type = SDL_USEREVENT;
	e.user.code = 0;
	SDL_PushEvent(&e);
}

#if defined(WIN32) && defined(BUILDING_SDLGUI_DLL)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

extern "C" int WINAPI _cygwin_dll_entry(HANDLE h, DWORD reason, void *ptr)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return 1;
}
#endif
