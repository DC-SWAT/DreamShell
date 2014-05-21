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

/*

This is the controller of the player.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "player.h"
#include "ff.h"
#include "fileinfo.h"
#include "dac.h"
#include "play_wav.h"
#include "play_mp3.h"
#include "play_aac.h"
#include "keys.h"
#include "ir.h"

static void next();
static void stop();
static void start();
static void do_playing();
static void do_stopped();
static void do_paused();

static unsigned char buf[2048];
//static SONGINFO songinfo;
extern FATFS fs;

enum playing_states {PLAYING, STOPPED, PAUSED};
enum user_commands {CMD_START, CMD_STOP, CMD_NEXT};
static enum playing_states state = STOPPED;
static FIL file;
static SONGLIST songlist;
static SONGINFO songinfo;
static int current_song_index = -1;

enum user_commands get_command(void)
{
  long key0 = get_key_press( 1<<KEY0 );
	long key1 = get_key_rpt( 1<<KEY1 ) || get_key_press( 1<<KEY1 );
  int ir_cmd = ir_get_cmd();
	
	if ((key0 && state == PLAYING) || ir_cmd == 0x36) {
    return CMD_STOP;
	} else if (key0 || ir_cmd == 0x35) {
    return CMD_START;
	} else if (key1 || ir_cmd == 0x34) {
    return CMD_NEXT;
	}
	
  return -1;
}

// State transition actions

static void next()
{
  current_song_index++;
  if (current_song_index >= songlist.size) {
  	current_song_index = 0;
  }
  iprintf("selected file: %.12s\n", songlist.list[current_song_index].filename);
}

static void stop()
{
	f_close( &file );
	puts("File closed.");
	
	// wait until both buffers are empty
  while(dac_busy_buffers() > 0);
	dac_reset();
	
	mp3_free();
	aac_free();
	
  state = STOPPED;
}

static void start()
{
  memset(&songinfo, 0, sizeof(SONGINFO));
  read_song_info_for_song(&(songlist.list[current_song_index]), &songinfo);
  assert(f_open( &file, get_full_filename(songlist.list[current_song_index].filename), FA_OPEN_EXISTING|FA_READ) == FR_OK);
  iprintf("title: %s\n", songinfo.title);
  iprintf("artist: %s\n", songinfo.artist);
  iprintf("album: %s\n", songinfo.album);
  iprintf("skipping: %i\n", songinfo.data_start);
  f_lseek(&file, songinfo.data_start);
  
  if (songinfo.type != UNKNOWN) {
		//assert(f_open( &file, get_full_filename(fileinfo.fname), FA_OPEN_EXISTING|FA_READ) == FR_OK);
		//puts("File opened.");
	
		switch(songinfo.type) {
  		case AAC:
  			aac_alloc();
  			aac_reset();
  		  break;
	
  		case MP4:
  			aac_alloc();
  			aac_reset();
  			aac_setup_raw();
  		  break;
		
  		case MP3:
  			mp3_alloc();
  			mp3_reset();
  		  break;
		}
		
		puts("playing");
		malloc_stats();
		state = PLAYING;
	} else {
		puts("unknown file type");
    stop();
	}
}

static void pause()
{
  // wait until both buffers are empty
  while(dac_busy_buffers() > 0);
  dac_disable_dma();
  state = PAUSED;
}

static void cont()
{
  dac_enable_dma();
  state = PLAYING;
}

// State loop actions

static void do_playing()
{
  enum user_commands cmd = get_command();
  
  switch(songinfo.type) {
  	case MP3:
  		if(mp3_process(&file) != 0) {
        stop();
  			next();
        start();
  		}
  	  break;
	
  	case MP4:
  		if (aac_process(&file, 1) != 0) {
        stop();
  		  next();
        start();
  		}
  	  break;
	
  	case AAC:
  		if (aac_process(&file, 0) != 0) {
        stop();
  		  next();
        start();
  		}
  	  break;
	
  	case WAV:
  		if (wav_process(&file) != 0) {
        stop();
  		  next();
        start();
  		}
  	  break;
	
  	default:
  		stop();
  	  break;
	}

	switch(cmd) {
	  case CMD_STOP:
      pause();
		  iprintf("underruns: %u\n", underruns);
      break;
	  case CMD_NEXT:
		  iprintf("underruns: %u\n", underruns);
      stop();
		  next();
      start();
      break;
	}
}

static void do_stopped()
{
  enum user_commands cmd = get_command();
  
  switch(cmd) {
    case CMD_START:
  		start();
      break;
  	case CMD_NEXT:
  		next();
      break;
	}
}

void do_paused()
{
  enum user_commands cmd = get_command();
  
  switch(cmd) {
    case CMD_START:
  		cont();
      break;
  	case CMD_NEXT:
      stop();
  		next();
      break;
	}
}

void player_init(void)
{
  dac_init();
	wav_init(buf, sizeof(buf));
	mp3_init(buf, sizeof(buf));
	aac_init(buf, sizeof(buf));
	
  songlist_build(&songlist);
  songlist_sort(&songlist);
  
  for (int i = 0; i < songlist.size; i++)
  {
    read_song_info_for_song(&(songlist.list[i]), &songinfo);
    iprintf("%s\n", songinfo.artist);
  }
}

void play(void)
{
  
	
	//dac_enable_dma();
	
	//iprintf("f_open: %i\n", f_open(&file, "/04TUYY~1.MP3", FA_OPEN_EXISTING|FA_READ));
	//infile_type = MP3;
	next();
	state = STOPPED;
	
	//mp3_alloc();
	
	// clear command buffer
  get_command();
	
	while(1)
	{
	
		switch(state) {
  		case STOPPED:
        do_stopped();
  		  break;
		
  		case PLAYING:
        do_playing();
        break;
      
      case PAUSED:
        do_paused();
        break;
		}
	}
	
}
