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
#include <string.h>

#include "AT91SAM7S64.h"
#include "play_mp3.h"
#include "mp3dec.h"
#include "ff.h"
#include "dac.h"
#include "profile.h"

#define debug_printf 

static HMP3Decoder hMP3Decoder;
static MP3FrameInfo mp3FrameInfo;
static unsigned char *read_ptr;
static int bytes_left=0, bytes_leftBeforeDecoding=0, err, offset;
static int nFrames = 0;
static unsigned char *mp3buf;
static unsigned int mp3buf_size;
static unsigned char allocated = 0;

void mp3_init(unsigned char *buffer, unsigned int buffer_size)
{
	mp3buf = buffer;
	mp3buf_size = buffer_size;
	mp3_reset();
}

void mp3_reset()
{
	read_ptr = NULL;
	bytes_leftBeforeDecoding = bytes_left = 0;
	nFrames = 0;
}

void mp3_alloc()
{
	if (!allocated) assert(hMP3Decoder = MP3InitDecoder());
	allocated = 1;
}

void mp3_free()
{
	if (allocated) MP3FreeDecoder(hMP3Decoder);
	allocated = 0;
}

int mp3_refill_inbuffer(FIL *mp3file)
{
  WORD bytes_read;
  int bytes_to_read;
  
  debug_printf("left: %d. refilling inbuffer...\n", bytes_left);
  if (bytes_left > 0) {
    // after fseeking backwards the FAT has to be read from the beginning -> S L O W
		//assert(f_lseek(mp3file, mp3file->fptr - bytes_leftBeforeDecoding) == FR_OK);
		// better: move unused rest of buffer to the start
		// no overlap as long as (1024 <= mp3buf_size/2), so no need to use memove
		memcpy(mp3buf, read_ptr, bytes_left);
  }
  
  bytes_to_read = mp3buf_size - bytes_left;
  
	assert(f_read(mp3file, (BYTE *)mp3buf + bytes_left, bytes_to_read, &bytes_read) == FR_OK);
	if (bytes_read == bytes_to_read) {
		read_ptr = mp3buf;
		offset = 0;
		bytes_left = mp3buf_size;
		debug_printf("ok. read: %d. left: %d\n", bytes_read, bytes_left);
		return 0;
	} else {
		puts("can't read more data");
		return -1;
	}
}

int mp3_process(FIL *mp3file)
{
	int writeable_buffer;
	
	if (read_ptr == NULL) {
		if(mp3_refill_inbuffer(mp3file) != 0)
		  return -1;
	}

	offset = MP3FindSyncWord(read_ptr, bytes_left);
	if (offset < 0) {
		puts("Error: MP3FindSyncWord returned <0");
		
		if(mp3_refill_inbuffer(mp3file) != 0)
		  return -1;
	}

	read_ptr += offset;
	bytes_left -= offset;
	bytes_leftBeforeDecoding = bytes_left;

	// check if this is really a valid frame
	// (the decoder does not seem to calculate CRC, so make some plausibility checks)
	if (MP3GetNextFrameInfo(hMP3Decoder, &mp3FrameInfo, read_ptr) == 0 &&
		mp3FrameInfo.nChans == 2 &&
		mp3FrameInfo.version == 0) {
		debug_printf("Found a frame at offset %x\n", offset + read_ptr - mp3buf + mp3file->fptr);
	} else {
		puts("this is no valid frame");
		// advance data pointer
		// TODO: handle bytes_left == 0
		assert(bytes_left > 0);
		bytes_left -= 1;
		read_ptr += 1;
		return 0;
	}

	if (bytes_left < 1024) {
		if(mp3_refill_inbuffer(mp3file) != 0)
		  return -1;
	}

	debug_printf("bytes_leftBeforeDecoding: %i\n", bytes_leftBeforeDecoding);

	while (dac_fill_dma() == 0);
	
	writeable_buffer = dac_get_writeable_buffer();
	if (writeable_buffer == -1) {
		return 0;
	}
	
	debug_printf("wb %i\n", writeable_buffer);

	//PROFILE_START("MP3Decode");
	err = MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, dac_buffer[writeable_buffer], 0);
	//PROFILE_END();

	nFrames++;
	
	if (err) {
 		switch (err) {
		case ERR_MP3_INDATA_UNDERFLOW:
			puts("ERR_MP3_INDATA_UNDERFLOW");
      bytes_left = 0;
			if(mp3_refill_inbuffer(mp3file) != 0)
  		  return -1;
			break;
			
 		case ERR_MP3_MAINDATA_UNDERFLOW:
 			/* do nothing - next call to decode will provide more mainData */
 			puts("ERR_MP3_MAINDATA_UNDERFLOW");
 			break;

 		default:
 			iprintf("unknown error: %i\n", err);
 			// skip this frame
 			if (bytes_left > 0) {
 				bytes_left --;
 				read_ptr ++;
 			} else {
 				// TODO
 				assert(0);
 			}
 			break;
 		}

		dac_buffer_size[writeable_buffer] = 0;
	} else {
		/* no error */
		MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
		debug_printf("Bitrate: %i\r\n", mp3FrameInfo.bitrate);
		debug_printf("%i samples\n", mp3FrameInfo.outputSamps);			
	
		debug_printf("Words remaining in first DMA buffer: %i\n", *AT91C_SSC_TCR);
		debug_printf("Words remaining in next DMA buffer: %i\n", *AT91C_SSC_TNCR);
	
		dac_buffer_size[writeable_buffer] = mp3FrameInfo.outputSamps;

		debug_printf("%lu Hz, %i kbps\n", mp3FrameInfo.samprate, mp3FrameInfo.bitrate/1000);
		
		if (dac_set_srate(mp3FrameInfo.samprate) != 0) {
			iprintf("unsupported sample rate: %lu\n", mp3FrameInfo.samprate);
			return -1;
		}
	}
	
	while (dac_fill_dma() == 0);

	return 0;
}
