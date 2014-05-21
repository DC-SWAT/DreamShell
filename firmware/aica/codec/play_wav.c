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

#include <assert.h>
#include <stdio.h>

#include "AT91SAM7S64.h"
#include "play_wav.h"
#include "dac.h"
#include "ff.h"
#include "profile.h"

void wav_init(unsigned char *buffer, unsigned int buffer_size)
{

}

int wav_process(FIL *wavfile)
{
	int writeable_buffer;
	WORD bytes_read;
	
	//puts("reading");
	
	if ((writeable_buffer = dac_get_writeable_buffer()) != -1) {
		PROFILE_START("f_read");
		f_read(wavfile, (BYTE *)dac_buffer[writeable_buffer], DAC_BUFFER_MAX_SIZE*2, &bytes_read);
		PROFILE_END();
		if (bytes_read != DAC_BUFFER_MAX_SIZE*2) {
			dac_buffer_size[writeable_buffer] = 0;
			return -1;
		} else {
			dac_buffer_size[writeable_buffer] = DAC_BUFFER_MAX_SIZE;
		}
	}
	
	dac_fill_dma();
	
	return 0;
}
