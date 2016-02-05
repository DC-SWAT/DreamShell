/** 
 * \file    gui.h
 * \brief   DreamShell GUI
 * \date    2006-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_GUI_H
#define _DS_GUI_H


#include "video.h"
#include "list.h"
#include "events.h"
#include "SDL/SDL_gui.h"


typedef struct MouseCursor {
    
	SDL_Surface *bg;
	SDL_Surface *cursor;
	SDL_Rect src, dst;
	int draw;

} MouseCursor_t;


/*
  Mouse cursor
*/
MouseCursor_t *CreateMouseCursor(const char *fn, /* or */ SDL_Surface *surface);
void DestroyMouseCursor(MouseCursor_t *c);
void DrawMouseCursor(MouseCursor_t *c);
void DrawMouseCursorEvent(MouseCursor_t *c, SDL_Event *event);
void SetActiveMouseCursor(MouseCursor_t *c);
MouseCursor_t *GetActiveMouseCursor();
void UpdateActiveMouseCursor();
void DrawActiveMouseCursor();


/* Virtual keyboard from vkb module */
int VirtKeyboardInit();
void VirtKeyboardShutdown();
int VirtKeyboardIsVisible();
void VirtKeyboardReDraw();
void VirtKeyboardShow();
void VirtKeyboardHide();
void VirtKeyboardToggle();


/* Main GUI */
int InitGUI();
void ShutdownGUI();
int LockGUI();
int UnlockGUI();
int GUI_IsLocked();
int GUI_Object2Trash(GUI_Object *object);
void GUI_ClearTrash();


/* GUI utils */
Uint32 colorHexToRGB(char *color, SDL_Color *clr);
SDL_Color Uint32ToColor(Uint32 c);
Uint32 ColorToUint32(SDL_Color c);
Uint32 MapHexColor(char *color, SDL_PixelFormat *format);

SDL_Surface *SDL_ImageLoad(const char *filename, SDL_Rect *selection);

#endif
