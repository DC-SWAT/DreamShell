/** 
 * \file    gui.h
 * \brief   DreamShell GUI
 * \date    2006-2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_GUI_H
#define _DS_GUI_H


#include "video.h"
#include "list.h"
#include "events.h"
#include "module.h"
#include "SDL/SDL_gui.h"

struct App;

/* Main GUI */
int InitGUI();
void ShutdownGUI();

void GUI_Disable();
void GUI_Enable();
void GUI_DisableInput(void);
void GUI_EnableInput(void);
int GUI_IsFirstOpen();
int GUI_OpenApp(struct App *app);
int GUI_CloseApp(struct App *app);
void GUI_EnableTsunamiInput(void);
void GUI_DisableTsunamiInput(void);

/* GUI utils */
Uint32 colorHexToRGB(char *color, SDL_Color *clr);
SDL_Color Uint32ToColor(Uint32 c);
Uint32 ColorToUint32(SDL_Color c);
Uint32 MapHexColor(char *color, SDL_PixelFormat *format);

SDL_Surface *SDL_ImageLoad(const char *filename, SDL_Rect *selection);

#endif
