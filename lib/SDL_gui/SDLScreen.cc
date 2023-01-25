#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

//////////////////////////////////////////////////////////////////////////////

static SDL_Rect ConvertRect(const GUI_Rect &r)
{
	SDL_Rect sdl_r = { (Sint16) r.x, (Sint16) r.y, (Uint16) r.w, (Uint16) r.h };
	return sdl_r;
}

static Uint32 MapColor(SDL_Surface *surface, const GUI_Color &c)
{
	return SDL_MapRGB(surface->format, int(c.r * 255), int(c.g * 255), int(c.b * 255));
}

//////////////////////////////////////////////////////////////////////////////

GUI_SDLScreen::GUI_SDLScreen(const char *aname, SDL_Surface *surf)
: GUI_Window(aname, NULL, 0, 0, surf->w, surf->h)
{
	surface = surf;
}

GUI_SDLScreen::~GUI_SDLScreen(void)
{
}

void GUI_SDLScreen::UpdateAll(void)
{
	SDL_UpdateRect(surface, 0, 0, area.w, area.h);
}

void GUI_SDLScreen::Update(const GUI_Rect &r)
{
	SDL_UpdateRect(surface, r.x, r.y, r.w, r.h);
}

void GUI_SDLScreen::DrawImage(const GUI_Surface *image, const GUI_Rect &src_r, const GUI_Rect &dst_r)
{
//	image->Blit(srp, surface, drp);
}

void GUI_SDLScreen::FillRect(const GUI_Rect &r, const GUI_Color &c)
{
	Uint32 sdl_c = MapColor(surface, c);
	SDL_Rect sdl_r = ConvertRect(r);

	SDL_FillRect(surface, &sdl_r, sdl_c);
}

void GUI_SDLScreen::DrawRect(const GUI_Rect &r, const GUI_Color &c)
{
}

void GUI_SDLScreen::DrawLine(int x1, int y1, int x2, int y2, const GUI_Color &c)
{
}

void GUI_SDLScreen::DrawPixel(int x, int y, const GUI_Color &c)
{
}

