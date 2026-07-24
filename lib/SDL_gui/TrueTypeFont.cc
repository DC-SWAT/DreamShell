#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C" {
	void LockVideo();
	void UnlockVideo();
}

GUI_TrueTypeFont::GUI_TrueTypeFont(const char *fn, int size)
: GUI_Font(fn), ttf(NULL)
{
	ttf = TTF_OpenFont(fn, size);
	if (ttf == NULL) {
		GUI_Exception("TTF_OpenFont failed name='%s' size=%d", fn, size);
	}
}

GUI_TrueTypeFont::~GUI_TrueTypeFont(void)
{
	if (ttf)
		TTF_CloseFont(ttf);
}

GUI_Surface *GUI_TrueTypeFont::RenderFast(const char *s, SDL_Color fg)
{
	SDL_Surface *text;
	GUI_Surface *surface;

	assert(s != NULL);
	if (ttf == NULL || strlen(s) == 0) {
		return NULL;
	}

	LockVideo();
	text = TTF_RenderText_Solid(ttf, s, fg);
	if (text == NULL) {
		UnlockVideo();
		return NULL;
	}
	surface = new GUI_Surface("text", text);
	UnlockVideo();
	return surface;
}

GUI_Surface *GUI_TrueTypeFont::RenderQuality(const char *s, SDL_Color fg)
{
	SDL_Surface *text;
	GUI_Surface *surface;

	assert(s != NULL);
	if (ttf == NULL || strlen(s) == 0) {
		return NULL;
	}

	LockVideo();
	text = TTF_RenderText_Blended(ttf, s, fg);
	if (text == NULL) {
		UnlockVideo();
		return NULL;
	}
	surface = new GUI_Surface("text", text);
	UnlockVideo();
	return surface;
}

SDL_Rect GUI_TrueTypeFont::GetTextSize(const char *s)
{
	SDL_Rect r = { 0, 0, 0, 0 };
	int w, h;

	assert(s != NULL);
	if (ttf == NULL || strlen(s) == 0)
	{
		return r;
	}

	LockVideo();
	if (TTF_SizeText(ttf, s, &w, &h) == 0)
	{
		r.w = w;
		r.h = h;
	}
	UnlockVideo();
	return r;
}

extern "C" GUI_Font *GUI_FontLoadTrueType(char *fn, int size)
{
	GUI_TrueTypeFont *font = new GUI_TrueTypeFont(fn, size);

	if(font == NULL || font->GetTTF() == NULL) {
		delete font;
		return NULL;
	}

	return font;
}
