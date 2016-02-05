/*
  class GUI_Mouse - used for drawing a colored mousepointer
*/
#include "SDL_gui.h"

/*
  contructor
  -call GUI_Surface-contructor
  -set x=y=0
  -create new SDL_Surface for the background
*/
GUI_Mouse::GUI_Mouse(char *aname, SDL_Surface * sf)
: GUI_Surface(aname, sf)
{
	x = 0;
	y = 0;
	bgsave =
		SDL_CreateRGBSurface(sf->flags, sf->w, sf->h,
							 sf->format->BitsPerPixel, sf->format->Rmask,
							 sf->format->Gmask, sf->format->Bmask,
							 sf->format->Amask);
}

/*
  destructor
  -free the background surface
*/
GUI_Mouse::~GUI_Mouse()
{
	SDL_FreeSurface(bgsave);
}

/*
  function Draw
*/
void GUI_Mouse::Draw(GUI_Screen * screen, int nx, int ny)
{
	//int ox=x;
	//int oy=y;
	SDL_Surface *scr;

	scr = screen->GetSurface()->GetSurface();
	x = nx;
	y = ny;
	SDL_Rect src, dst;

	src.x = 0;
	src.y = 0;
	if (x + GetWidth() <= scr->w)
		src.w = GetWidth();
	else
	{
		src.w = scr->w - x - 1;
		//printf("adjust: src.w=%d x=%d\n",src.w,x);
	}
	if (y + GetHeight() <= scr->h)
		src.h = GetHeight();
	else
		src.h = scr->h - y - 1;
	dst.x = x;
	dst.y = y;
	dst.w = src.w;
	dst.h = src.h;
	SDL_BlitSurface(scr, &dst, bgsave, &src);
	SDL_BlitSurface(GetSurface(), &src, scr, &dst);
	SDL_UpdateRect(scr, dst.x, dst.y, dst.w, dst.h);
	SDL_UpdateRect(scr, odst.x, odst.y, odst.w, odst.h);
	SDL_BlitSurface(bgsave, &src, scr, &dst);
	odst = dst;
	osrc = src;
}


extern "C"
{
       
GUI_Mouse *GUI_MouseCreate(char *name, SDL_Surface * sf)
{
	return new GUI_Mouse(name, sf);
}     
       
}
