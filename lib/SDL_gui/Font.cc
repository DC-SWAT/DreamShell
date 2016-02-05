#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

GUI_Font::GUI_Font(const char *aname)
: GUI_Object(aname)
{
}

GUI_Font::~GUI_Font(void)
{
}

GUI_Surface *GUI_Font::RenderFast(const char *s, SDL_Color fg)
{
	/*throw*/ GUI_Exception("RenderFast not implemented");
	return NULL;
}

GUI_Surface *GUI_Font::RenderQuality(const char *s, SDL_Color fg)
{
	/*throw*/ GUI_Exception("RenderQuality not implemented");
	return NULL;
}

void GUI_Font::DrawText(GUI_Surface *surface, const char *s, int x, int y)
{
	/*throw*/ GUI_Exception("DrawText not implemented");
}

SDL_Rect GUI_Font::GetTextSize(const char *s)
{
	/*throw*/ GUI_Exception("GetTextSize not implemented");
	SDL_Rect r;
	return r;
}

extern "C"
{

GUI_Surface *GUI_FontRenderFast(GUI_Font *font, const char *s, SDL_Color fg)
{
	return font->RenderFast(s, fg);
}

GUI_Surface *GUI_FontRenderQuality(GUI_Font *font, const char *s, SDL_Color fg)
{
	return font->RenderQuality(s, fg);
}

void GUI_FontDrawText(GUI_Font *font, GUI_Surface *surface, const char *s, int x, int y)
{
	font->DrawText(surface, s, x, y);
}

SDL_Rect GUI_FontGetTextSize(GUI_Font *font, const char *s)
{
	return font->GetTextSize(s);
}

};
