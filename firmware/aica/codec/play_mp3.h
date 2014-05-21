/*
  Copyright (C) 2006 Andreas Schwarz <andreas@andreas-s.net>

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

#ifndef _PLAY_MP3_H_
#define _PLAY_MP3_H_

#include "ff.h"

int mp3_process(FIL *mp3file);
void mp3_reset();
void mp3_init(unsigned char *buffer, unsigned int buffer_size);
void mp3_alloc();
void mp3_free();

#endif /* _PLAY_MP3_H_ */
