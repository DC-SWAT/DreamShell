/**
 * \file    vkb.h
 * \brief   DreamShell virtual keyboard
 * \date    2007-2026
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_VKB_H
#define _DS_VKB_H

#include "module.h"
#include "SDL/SDL.h"

#define DS_SHOW_VKB_EVENT (SDL_USEREVENT + 10)
#define DS_HIDE_VKB_EVENT (SDL_USEREVENT + 11)

int VirtKeyboardInit();
void VirtKeyboardShutdown();
int VirtKeyboardIsVisible();
void VirtKeyboardShow();
void VirtKeyboardHide();
void VirtKeyboardToggle();
void VirtKeyboardSetEnabled(int enabled);

static inline int IsVirtKeyboardVisible() {
    if (GetModuleByName("vkb")) {
        uintptr_t vkb = GET_EXPORT_ADDR("VirtKeyboardIsVisible");
        if (vkb) {
            return ((int (*)(void))vkb)();
        }
    }
    return 0;
}

static inline void SetVirtKeyboardEnabled(int enabled) {
    if (GetModuleByName("vkb")) {
        uintptr_t vkb = GET_EXPORT_ADDR("VirtKeyboardSetEnabled");
        if (vkb) {
            ((void (*)(int))vkb)(enabled);
        }
    }
}

#endif
