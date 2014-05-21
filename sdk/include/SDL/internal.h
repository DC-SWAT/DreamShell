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

#ifndef _internal_h_
#define _internal_h_


#define PRINT_ERROR(X) fprintf(stderr, "ERROR in %s:%s(): %s\n", __FILE__, __FUNCTION__, X)

Uint32	DT_GetPixel(SDL_Surface *surface, int x, int y);
void	DT_PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);


#endif /* _internal_h_ */


