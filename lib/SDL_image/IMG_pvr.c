/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2013-2014 SWAT <swat@211.ru>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "SDL_endian.h"
#include "SDL_image.h"
#include "SegaPVRImage.h"

#if defined(__DREAMCAST__)
#include <malloc.h>
#endif

#if defined(HAVE_STDIO_H) && !defined(__DREAMCAST__)
int fileno(FILE *f);
#endif

#ifdef LOAD_PVR

// unsigned long int GetUntwiddledTexelPosition(unsigned long int x, unsigned long int y);

// int MipMapsCountFromWidth(unsigned long int width);

// RGBA Utils
// void TexelToRGBA( unsigned short int srcTexel, enum TextureFormatMasks srcFormat, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);

// unsigned int ToUint16(unsigned char* value)
// {
//     return (value[0] | (value[1] << 8));
// }


// // Twiddle
// static const unsigned int kTwiddleTableSize = 1024;
// unsigned long int gTwiddledTable[1024];
static bool bTwiddleTable = false;


/* See if an image is contained in a data source */
int IMG_isPVR(SDL_RWops *src)
{
	int start;
	int is_PVR = 0;
	char magic[4];

	if ( !src )
		return 0;
		
	start = SDL_RWtell(src);

	if ( SDL_RWread(src, magic, 1, sizeof(magic)) == sizeof(magic) ) {
		if (!strncasecmp(magic, "GBIX", 4) || !strncasecmp(magic, "PVRT", 4)) {
			is_PVR = 1;
		}
	}
	SDL_RWseek(src, start, RW_SEEK_SET);
	
#ifdef DEBUG_IMGLIB
	fprintf(stderr, "IMG_isPVR: %d (%c%c%c%c)\n", is_PVR, magic[0], magic[1], magic[2], magic[3]);
#endif
	
	return(is_PVR);
}

/* Load a PVR type image from an SDL datasource */
SDL_Surface *IMG_LoadPVR_RW(SDL_RWops *src)
{
	int start = 0, len = 0;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Surface *surface = NULL;
	char *error = NULL;
	uint8 *data;

	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	start = SDL_RWtell(src);
	
	SDL_RWseek(src, 0, RW_SEEK_END);
	len = SDL_RWtell(src);
	SDL_RWseek(src, 0, RW_SEEK_SET);
	
#ifdef DEBUG_IMGLIB
	fprintf(stderr, "IMG_LoadPVR_RW: %d bytes\n", len);
#endif
	
	data = (uint8*) memalign(32, len);
	
	if(data == NULL) {
		error = "no free memory";
		goto done;
	}
	
#if defined(HAVE_STDIO_H) && !defined(__DREAMCAST__)

	int fd = fileno(src->hidden.stdio.fp);
	
	if(fd > -1) {
		if(read(fd, data, len) < 0) {
			error = "file truncated";
			goto done;
		}
	} else {
		if (!SDL_RWread(src, data, len, 1)) {
			error = "file truncated";
			goto done;
		}
	}

#else
	if (!SDL_RWread(src, data, len, 1)) {
		error = "file truncated";
		goto done;
	}
#endif
	
    struct PVRTHeader pvrtHeader;
    unsigned int offset = ReadPVRHeader(data, &pvrtHeader);
	
    if (offset == 0)
    {
		error = "bad PVR header";
		goto done;
    }
	
	enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader.textureAttributes & 0xFF);
	
	if(srcFormat != TFM_ARGB1555 && srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB4444) {
		error = "unsupported format";
		goto done;
	}
	
	/* Create the surface of the appropriate type */
	Rmask = Gmask = Bmask = Amask = 0;

	if ( SDL_BYTEORDER == SDL_LIL_ENDIAN ) {
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		if(srcFormat != TFM_RGB565)
			Amask = 0xFF000000;
	} else {
		int so = srcFormat != TFM_RGB565 ? 0 : 8;
		Rmask = 0xFF000000 >> so;
		Gmask = 0x00FF0000 >> so;
		Bmask = 0x0000FF00 >> so;
		Amask = 0x000000FF >> so;
	}
	
	surface = SDL_AllocSurface(SDL_SWSURFACE, pvrtHeader.width, pvrtHeader.height,
				   32, Rmask, Gmask, Bmask, Amask);
				   
	if ( surface == NULL ) {
		error = "can't allocate surface";
		goto done;
	}
	
	if(bTwiddleTable == false) {
		BuildTwiddleTable();
		bTwiddleTable = true;
	}

	
	memset_sh4(surface->pixels, 0, pvrtHeader.width * pvrtHeader.height * 4);
    
	if(!DecodePVR(data + offset, &pvrtHeader, surface->pixels)) {
		error = "can't decode image";
		goto done;
	}
	

done:
	if ( error ) {
		SDL_RWseek(src, start, RW_SEEK_SET);
		if ( surface ) {
			SDL_FreeSurface(surface);
			surface = NULL;
		}
		IMG_SetError(error);
	}
	
	if(data) {
		free(data);
	}
	
	return(surface);
}

#else

/* See if an image is contained in a data source */
int IMG_isPVR(SDL_RWops *src)
{
	return(0);
}

/* Load a PCX type image from an SDL datasource */
SDL_Surface *IMG_LoadPVR_RW(SDL_RWops *src)
{
	return(NULL);
}


#endif /* LOAD_PVR */
