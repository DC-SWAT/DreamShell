/** 
 * \file    gui.h
 * \brief   DreamShell GUI
 * \date    2006-2025
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_GUI_H
#define _DS_GUI_H


#include "video.h"
#include "list.h"
#include "events.h"
#include "module.h"
#include "SDL/SDL_gui.h"

/* Virtual keyboard from vkb module */
int VirtKeyboardInit();
void VirtKeyboardShutdown();
int VirtKeyboardIsVisible();
void VirtKeyboardReDraw();
void VirtKeyboardShow();
void VirtKeyboardHide();
void VirtKeyboardToggle();

static inline int IsVirtKeyboardVisible() {
    if (GetModuleByName("vkb")) {
        uintptr_t vkb = GET_EXPORT_ADDR("VirtKeyboardIsVisible");
        if (vkb) {
            return ((int (*)(void))vkb)();
        }
    }
    return 0;
}

/* Main GUI */
int InitGUI();
void ShutdownGUI();

void GUI_Disable();
void GUI_Enable();

int GUI_Object2Trash(GUI_Object *object);
void GUI_ClearTrash();

/* GUI utils */
Uint32 colorHexToRGB(char *color, SDL_Color *clr);
SDL_Color Uint32ToColor(Uint32 c);
Uint32 ColorToUint32(SDL_Color c);
Uint32 MapHexColor(char *color, SDL_PixelFormat *format);

SDL_Surface *SDL_ImageLoad(const char *filename, SDL_Rect *selection);

#endif
