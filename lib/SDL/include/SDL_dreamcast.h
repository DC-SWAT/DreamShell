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
	SDL_DC_C,
	SDL_DC_B,
	SDL_DC_A,
	SDL_DC_START,
	SDL_DC_Z,
	SDL_DC_Y,
	SDL_DC_X,
	SDL_DC_D,
	SDL_DC_L,
	SDL_DC_R
} SDL_DC_button;

void SDL_DC_EmulateMouse(SDL_bool value);

void SDL_DC_SetSoundBuffer(void *buffer);
void SDL_DC_RestoreSoundBuffer(void);


#ifdef __cplusplus
}
#endif

#endif
