/*
	SDL_console: An easy to use drop-down console based on the SDL library
	Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Clemens Wacha
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WHITOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library Generla Public
	License along with this library; if not, write to the Free
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
	Clemens Wacha
	reflex-2000@gmx.net
*/

#ifndef Drawtext_h
#define Drawtext_h

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif


#define TRANS_FONT 1


#ifdef __cplusplus
extern "C" {
#endif

	typedef struct BitFont_td {
		SDL_Surface		*FontSurface;
		int			CharWidth;
		int			CharHeight;
		int			FontNumber;
		struct BitFont_td	*NextFont;
	}
	BitFont;


	void	DT_DrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y );
	int	DT_LoadFont(const char *BitmapName, int flags );
	int	DT_FontHeight( int FontNumber );
	int	DT_FontWidth( int FontNumber );
	BitFont*	DT_FontPointer(int FontNumber );
	void	DT_DestroyDrawText();


#ifdef __cplusplus
};
#endif

#endif



