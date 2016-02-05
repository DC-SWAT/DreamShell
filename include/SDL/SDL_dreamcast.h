#ifndef SDL_DREAMCAST_H
#define SDL_DREAMCAST_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    SDL_DC_DMA_VIDEO,
    SDL_DC_TEXTURED_VIDEO,
    SDL_DC_DIRECT_VIDEO
} SDL_DC_driver;

void SDL_DC_SetVideoDriver(SDL_DC_driver value);
void SDL_DC_SetWindow(int width, int height);
void SDL_DC_VerticalWait(SDL_bool value);
void SDL_DC_ShowAskHz(SDL_bool value);
void SDL_DC_Default60Hz(SDL_bool value);

typedef enum {
    SDL_DC_START=3,
    SDL_DC_A=2,
    SDL_DC_B=1,
    SDL_DC_X=5,
    SDL_DC_Y=6,
    SDL_DC_L=7,
    SDL_DC_R=8,
    SDL_DC_LEFT=11,
    SDL_DC_RIGHT=12,
    SDL_DC_UP=9,
    SDL_DC_DOWN=10
} SDL_DC_button;

void SDL_DC_MapKey(int joy, SDL_DC_button button, SDLKey key);
void SDL_DC_EmulateKeyboard(SDL_bool value);
void SDL_DC_EmulateMouse(SDL_bool value);

void SDL_DC_SetSoundBuffer(void *buffer);
void SDL_DC_RestoreSoundBuffer(void);


#ifdef __cplusplus
}
#endif

#endif
