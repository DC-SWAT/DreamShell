/** 
 * \file    video.h
 * \brief   DreamShell video rendering
 * \date    2004-2024
 * \author  SWAT www.dc-swat.ru
 */
     
#ifndef _DS_VIDEO_H
#define _DS_VIDEO_H

#include <kos.h>
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_dreamcast.h"
#include "SDL/SDL_gfxPrimitives.h"
#include "SDL/SDL_gfxBlitFunc.h"
#include "SDL/SDL_rotozoom.h"
#include "SDL/SDL_console.h"
#include "SDL/DT_drawtext.h"

#include "plx/font.h"
#include "plx/sprite.h"
#include "plx/list.h"
#include "plx/dr.h"
#include "plx/context.h"
#include "plx/texture.h"

#include "gui.h"
#include "list.h"

// Pixel packing macro
#define PACK_PIXEL(r, g, b) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) )


SDL_Surface *GetScreen();
void SetScreen(SDL_Surface *new_screen);

int GetScreenWidth();
int GetScreenHeight();

/* Always RGB565 */
int GetVideoMode();
void SetVideoMode(int mode); 

void SDL_DS_SetWindow(int width, int height);
void SetScreenMode(int w, int h, float x, float y, float z);

void SDL_DS_AllocScreenTexture(SDL_Surface *screen);
void SDL_DS_FreeScreenTexture(int reset_pvr_memory);

pvr_ptr_t GetScreenTexture();
void SetScreenTexture(pvr_ptr_t *txr);

void SetScreenOpacity(float opacity);
float GetScreenOpacity();
void SetScreenFilter(int filter);
void SetScreenVertex(float u1, float v1, float u2, float v2);

void ScreenRotate(float x, float y, float z);
void ScreenTranslate(float x, float y, float z);

void ScreenFadeIn();
void ScreenFadeOut();
void ScreenFadeOutEx(const char *text, int wait);
void ScreenFadeStop();
int ScreenIsHidden();

void DisableScreen();
void EnableScreen();
int ScreenIsEnabled();
void ScreenChanged();
int ScreenUpdated();
void ScreenWaitUpdate();

void LockVideo();
void UnlockVideo();
int VideoIsLocked();
int VideoMustLock();

void InitVideoHardware();
int InitVideo(int w, int h, int bpp);
void ShutdownVideo();

void InitVideoThread();
void ShutdownVideoThread();

void ShowLogo();
void HideLogo();


/* Load PVR to a KOS Platform Independent Image */
int pvr_to_img(const char *filename, kos_img_t *rv);

/* Load zlib compressed KMG to a KOS Platform Independent Image */
int gzip_kmg_to_img(const char * filename, kos_img_t *rv);

/* Utility function to fill out the initial poly contexts */
void plx_fill_contexts(plx_texture_t * txr);

static inline void plx_vert_ifpm3(int flags, float x, float y, float z, uint32 color, float u, float v) {       
	plx_mat_tfip_3d(x, y, z);
	plx_vert_ifp(flags, x, y, z, color, u, v);
}

#endif
