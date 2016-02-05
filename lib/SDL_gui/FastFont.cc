#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_FastFont::GUI_FastFont(const char *fn)
: GUI_Font(fn)
{
	image = new GUI_Surface(fn);
	char_width = image->GetWidth() / 256;
	char_height = image->GetHeight();
}

GUI_FastFont::~GUI_FastFont()
{
	image->DecRef();
}

void GUI_FastFont::DrawText(GUI_Surface *surface, const char *s, int x, int y)
{
	SDL_Rect sr, dr;
	int n, i, max;
	
	assert(s != 0);
	if (x > surface->GetWidth() || y > surface->GetHeight())
		return;

	n = strlen(s);
	max = (surface->GetWidth() - x) / char_width;
	if (n > max)
		n = max;

	dr.x = x;
	dr.y = y;
	dr.w = char_width;
	dr.h = char_height;
	sr = dr;
	sr.y = 0;

	for (i=0; i<n; i++)
	{
		sr.x = s[i] * char_width;
		image->Blit(&sr, surface, &dr);
		dr.x += char_width;
	}
}

GUI_Surface *GUI_FastFont::RenderFast(const char *s, SDL_Color fg)
{
	assert(s != 0);
	
	GUI_Surface *surface = 	new GUI_Surface("text", SDL_HWSURFACE,
		strlen(s) * char_width, char_height, 16, 
		0, 0, 0, 0);
	DrawText(surface, s, 0, 0);
	return surface;
}

GUI_Surface *GUI_FastFont::RenderQuality(const char *s, SDL_Color fg)
{
	return RenderFast(s, fg);
}

SDL_Rect GUI_FastFont::GetTextSize(const char *s)
{
	assert(s != 0);
	
	SDL_Rect r = { 0, 0, 0, 0 };
	r.w = strlen(s) * char_width;
	r.h = char_height;
	return r;	
}

GUI_Surface *GUI_FastFont::GetFontImage(void)
{
	return image;
}

extern "C" GUI_Font *GUI_FontLoadBitmap(char *fn)
{
	return new GUI_FastFont(fn);
}

