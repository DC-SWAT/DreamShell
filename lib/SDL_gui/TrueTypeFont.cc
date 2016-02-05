#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_TrueTypeFont::GUI_TrueTypeFont(const char *fn, int size)
: GUI_Font(fn)
{
	/* FIXME include the size in the name for caching */

	ttf = TTF_OpenFont(fn, size);
	if (ttf == NULL)
		/*throw*/ GUI_Exception("TTF_OpenFont failed name='%s' size=%d", fn, size);
}

GUI_TrueTypeFont::~GUI_TrueTypeFont(void)
{
	if (ttf)
		TTF_CloseFont(ttf);
}

GUI_Surface *GUI_TrueTypeFont::RenderFast(const char *s, SDL_Color fg)
{
	assert(s != NULL);
	if (strlen(s) == 0)
		return NULL;

	return new GUI_Surface("text", TTF_RenderText_Solid(ttf, s, fg));
}

GUI_Surface *GUI_TrueTypeFont::RenderQuality(const char *s, SDL_Color fg)
{
	assert(s != NULL);
	if (strlen(s) == 0)
		return NULL;

	return new GUI_Surface("text", TTF_RenderText_Blended(ttf, s, fg));
}

SDL_Rect GUI_TrueTypeFont::GetTextSize(const char *s)
{
	SDL_Rect r = { 0, 0, 0, 0 };
	int w, h;
			
	assert(s != NULL);
	if (strlen(s) != 0)
	{
		if (TTF_SizeText(ttf, s, &w, &h) == 0)
		{
			r.w = w;
			r.h = h;
		}
	}
	return r;	
}

extern "C" GUI_Font *GUI_FontLoadTrueType(char *fn, int size)
{
	return new GUI_TrueTypeFont(fn, size);
}

