#ifndef __SDL_PRIM_H
#define __SDL_PRIM_H		1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "SDL.h"


#define SDL_TG_ALPHA			0x01
#define SDL_TG_ANTIALIAS		0x02
#define SDL_TG_FILL			0x04
#define SDL_TG_LOCK			0x08


#define __SDL_PRIM_LOCKSURFACE(screen) \
{ \
	if ( SDL_MUSTLOCK((screen)) ) { \
		if ( SDL_LockSurface((screen)) < 0 ) { \
			fprintf(stderr, "Can't lock video surface: %s\n", SDL_GetError()); \
		} \
	} \
}


#define __SDL_PRIM_UNLOCKSURFACE(screen) \
{ \
	if ( SDL_MUSTLOCK((screen)) ) { \
		SDL_UnlockSurface((screen)); \
	} \
}


inline void SDL_putPixel(SDL_Surface*, int x, int y, Uint32 clr);
inline void SDL_blendPixel(SDL_Surface*, int x, int y, Uint32 clr, Uint8 alpha);
inline Uint8* SDL_getPixel(SDL_Surface*, int x, int y);
inline void __slow_SDL_blendPixel(SDL_Surface*, int, int, Uint32, Uint8);

void SDL_drawLine_TG( SDL_Surface*, int x, int y, int x2, int y2, Uint32 clr,
                      Uint8 alpha, Uint8 flags );
/*
void SDL_drawBrokenLine_TG( SDL_Surface*, int x, int y, int x2, int y2,
                            int seglen, int breaklen, Uint32 clr, Uint8 alpha,
                            Uint32 flags );
*/
void SDL_drawCircle_TG( SDL_Surface*, int x, int y, int r, Uint32 clr,
                        Uint8 alpha, Uint8 flags );
void SDL_drawTriangle_TG( SDL_Surface*, int x, int y, int x2, int y2, int x3,
                          int y3, Uint32 clr, Uint8 alpha, Uint8 flags );

#define SDL_drawLine(surf, x1, y1, x2, y2, clr) \
	SDL_drawLine_TG((surf), (x1), (y1), (x2), (y2), (clr), 0, 0)

#define SDL_drawLine_Alpha(surf, x1, y1, x2, y2, clr, alpha) \
	SDL_drawLine_TG( (surf), (x1), (y1), (x2), (y2), (clr),\
	                 (alpha), SDL_TG_ALPHA )

#define SDL_drawLine_AA(surf, x1, y1, x2, y2, clr) \
	SDL_drawLine_TG( (surf), (x1), (y1), (x2), (y2), (clr), 0,\
	                 SDL_TG_ANTIALIAS )

#define SDL_drawLine_Alpha_AA(surf, x1, y1, x2, y2, clr, alpha) \
	SDL_drawLine_TG( (surf), (x1), (y1), (x2), (y2), (clr),\
	                 (alpha), SDL_TG_ALPHA | SDL_TG_ANTIALIAS )

#define SDL_drawCircle(surf, x, y, r, clr) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), 0, 0)

#define SDL_drawCircle_AA(surf, x, y, r, clr) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), 0, SDL_TG_ANTIALIAS)

#define SDL_drawCircle_Alpha(surf, x, y, r, clr, alpha) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), (alpha), SDL_TG_ALPHA)

#define SDL_drawCircle_Alpha_AA(surf, x, y, r, clr, alpha) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), (alpha),\
	                  SDL_TG_ALPHA | SDL_TG_ANTIALIAS)

#define SDL_fillCircle(surf, x, y, r, clr) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), 0, SDL_TG_FILL)

#define SDL_fillCircle_AA(surf, x, y, r, clr) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), 0,\
	                  SDL_TG_ANTIALIAS | SDL_TG_FILL)

#define SDL_fillCircle_Alpha(surf, x, y, r, clr, alpha) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), (alpha),\
	                  SDL_TG_ALPHA | SDL_TG_FILL)

#define SDL_fillCircle_Alpha_AA(surf, x, y, r, clr, alpha) \
	SDL_drawCircle_TG((surf), (x), (y), (r), (clr), (alpha),\
	                  SDL_TG_ALPHA | SDL_TG_ANTIALIAS | SDL_TG_FILL)

#define SDL_drawTriangle(surf, x1,y1, x2,y2, x3,y3, clr) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), 0, \
	                    0)

#define SDL_drawTriangle_AA(surf, x1,y1, x2,y2, x3,y3, clr) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), 0, \
	                    SDL_TG_ANTIALIAS)

#define SDL_drawTriangle_Alpha(surf, x1,y1, x2,y2, x3,y3, clr, alpha) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), \
	                    (alpha), SDL_TG_ALPHA)

#define SDL_drawTriangle_Alpha_AA(surf, x1,y1, x2,y2, x3,y3, clr, alpha) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), \
	                    (alpha), SDL_TG_ALPHA | SDL_TG_ANTIALIAS)

#define SDL_fillTriangle(surf, x1,y1, x2,y2, x3,y3, clr) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), 0, \
	                    SDL_TG_FILL)

#define SDL_fillTriangle_AA(surf, x1,y1, x2,y2, x3,y3, clr) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), 0, \
	                    SDL_TG_ANTIALIAS|SDL_TG_FILL)

#define SDL_fillTriangle_Alpha(surf, x1,y1, x2,y2, x3,y3, clr, alpha) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), \
	                    (alpha), SDL_TG_ALPHA|SDL_TG_FILL)

#define SDL_fillTriangle_Alpha_AA(surf, x1,y1, x2,y2, x3,y3, clr, alpha) \
	SDL_drawTriangle_TG((surf), (x1),(y1), (x2),(y2), (x3),(y3), (clr), \
	                    (alpha), SDL_TG_ALPHA|SDL_TG_ANTIALIAS|SDL_TG_FILL)


#endif
