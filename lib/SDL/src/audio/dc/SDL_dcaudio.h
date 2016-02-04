/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    BERO <bero@geocities.co.jp>
    based on SDL_diskaudio.h by Sam Lantinga <slouken@libsdl.org>
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_dcaudio.h,v 1.2 2004/01/04 16:49:12 slouken Exp $";
#endif

#ifndef _SDL_diskaudio_h
#define _SDL_diskaudio_h

#include "SDL_sysaudio.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	/* The file descriptor for the audio device */
	Uint8 *mixbuf;
	Uint32 mixlen;
	int playing;
	int leftpos,rightpos;
	int nextbuf;
};

#endif /* _SDL_diskaudio_h */
