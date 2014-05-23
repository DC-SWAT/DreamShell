/*
 * output.c
 * Copyright (C) 2006 - 2008 Josh Sutherland <OneThirty8@aol.com>
 * This file is part of VC/DC, a free MPEG-2 video player.
 *
 * VC/DC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VC/DC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/*  Audio output functions for MPEG audio data
 *  Most of this code is my own or (more likely) stolen/adapted from 
 *  La Cible's lvfdc and KOS/KOS examples, but I started with the Linux output
 *  driver from the following project:
 */
 
/*  mpadec - MPEG audio decoder
 *  Copyright (C) 2002-2004 Dmitriy Startsev (dstartsev@rambler.ru)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*  Note that the above LGPL license refers to mpadec.  On the Dreamcast, you
 *  can't link dynamically, so assume a GPL license if you're using this code
 *  for anything on the Dreamcast--
 *  I don't really care what you do with the code as long as credit is given
 *  where it's due (see the GPL), so please note that the majority of the code
 *  in this library was not written by me.  See the copyright notices for
 *  libmpeg2, liba52, mpg123/mpglib, Lame, KOS, etc...
 */

/****************************************************************************
 ****************************************************************************
 ****    TODO:
 ****    This really doesn't need to be as complicated as it is.
 ****    While it was suitable when testing a lot under Linux for the
 ****    demuxing/decoding functions which were (at the time, at least)
 ****    non-platform-specific, the Dreamcast is only going to have one
 ****    possible set of sound hardware.  Pretty this up a bit when doing
 ****    the planned major reorganization of code.
 ****    --JHS 03 January 2008.
 ****
 ****************************************************************************
 ****************************************************************************/

#ifdef _arch_dreamcast

#include <kos.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include "aica_cmd_iface.h"  /* Do we use this at all?  We might need it for ac3, but not here. */
#include "config.h"
#include "output.h"

static int sample_rate;
static int chans;

static mutex_t * audio_mut = NULL;

#define BUFFER_MAX_FILL 65536*4
static char tmpbuf[BUFFER_MAX_FILL]; /* Temporary storage space for PCM data--65534 16-bit
			samples should be plenty of room--that's about 0.5 seconds. */
static char sndbuf[BUFFER_MAX_FILL];
			
static int sndptr;

static int aud_set = 0;

static int waiting_for_data = 0;

static snd_stream_hnd_t shnd;

static int sbsize;

static int sb_min;

static int last_read, snd_ct;

static double fudge_factor;
#ifdef DEBUGFPS
extern void take_notes(char * noteid, char * fmt, ...);
#endif

/* The idea here is to keep the audio from skipping or repeating.
   If your video framerate drops too much, you're going to get
   bad sound.  So, we lie to the sound hardware and reduce the
   sample rate to something below what it's supposed to be. 
   I sorta stole this idea from xine, I think. */
/* We're not actually using this currently, but I'm leaving it here
   for use in the future, because it does work quite well and might
   allow us to add a jog/shuttle feature. That would be cool*/
#define difference(a,b) ( (a > b) ? a-b: b-a )

void fudge_audio(double fudge)
{
  int sample_fudge;
  double diff;
  if ( (fudge == fudge_factor) || (aud_set == 0) )  /* We don't want to bother recalculating */
     return;					/* the faked sample rate if it's going to be the same.
     						  Also, we don't want to get here if the streaming channels
						  have not ben initialized. */
     
  //if (fudge >= 1.0)fudge_factor = 1.0; /* This should NEVER be over 1.0. */
  diff = difference(fudge, fudge_factor);
  if (diff > 0.05)
  {
    if ( fudge_factor > fudge)
      fudge_factor -=0.05;
    else
      fudge_factor +=0.05;
  }else{
    fudge_factor = (fudge <= 0.75) ? 0.75: ((fudge >= 1.0) ? 1.0: fudge);
  }
      			
  sample_fudge = (int)((double)sample_rate * fudge_factor );
#ifdef DEBUGFPS  
    take_notes("fudge", "Audio Fudge Factor is:%.2f", fudge_factor);
    take_notes("fudge2", "Stream w/ sample rate of %dHz playing at %dHz", sample_rate, sample_fudge);
#endif    
  snd_stream_start(shnd, sample_fudge, chans-1);
      
}

/* Our audio sound stream callback function.  Nothing special here.
   snd_stream_poll will ask it for (n) samples.  If we have (n) or
   fewer samples, give it what we have.  Otherwise, give it what it
   has asked for. 
   TODO:  resample if we don't have enough samples?  We can probably
   drop in some code from xine-lib and use it as a filter. */
static void *mpv_callback(snd_stream_hnd_t hnd, int len, int * actual)
{
  int wegots;
  
   //mutex_lock(audio_mut); 
     
    if (len >= sndptr)
    {
       wegots = sndptr;
    } else {
       wegots = len;
    }
    
    if (wegots<=0)
    {
      snd_ct = 0;
      last_read = 0;
      *actual = chans * 2; /* This seems... wrong.  But how should we handle such a case? */
      waiting_for_data = 0; /* return NULL here? We will end up with a gap if we're not at the
      				start of a video.  We could say *actual = 1; or something? 
				That might eliminate the lag at the beginning of a video? */
      				
      //mutex_unlock(audio_mut);
     return NULL;
    } else {
     snd_ct = wegots;    
     *actual = wegots;
     waiting_for_data = 0;
    }
   
    memcpy (sndbuf, tmpbuf, snd_ct);

    last_read = 1;  sndptr -=snd_ct;
    
    memcpy(tmpbuf, tmpbuf+snd_ct, sndptr);
    last_read = 0; snd_ct = 0;
    //mutex_unlock(audio_mut);
     return sndbuf;
}

static kthread_t * loop_thread;

condvar_t *audio_cond = NULL;
/* This loops as long as the audio driver is open.
   Do we want to poll more frequently? */
static void play_loop(void* yarr)
{

  while (aud_set == 1)
  {  

  mutex_lock(audio_mut);
   if (sndptr >= sbsize)
   //if (sndptr >= sb_min)
   { 
     
     while(waiting_for_data == 1)	
     {
        snd_stream_poll(shnd);
	
	 if (aud_set == 0)
	{
	  mutex_unlock(audio_mut);
	  return;
	} 
     }

      waiting_for_data = 1;
    }else{
     if(aud_set) /* Important to check this--if we haven't got here when we set aud_set to 0, we'll get here and be stuck! */
      cond_wait(audio_cond, audio_mut);
    } 
    mutex_unlock(audio_mut);
  }  /* while (aud_set == 1) */
 
}

static void start_audio()
{
  aud_set = 1;
  //snd_stream_prefill(shnd);
  snd_stream_start(shnd, sample_rate, chans-1);
  loop_thread = thd_create(play_loop, NULL);
}

/* Allocate streaming channel(s), set our sampling frequency, and say "GO!" */
static void *oss_open(char *filename, int freq, int channels, int format, int endian, uint64_t size)
{
  struct oss_out *oss_out = (struct oss_out *)malloc(sizeof(struct oss_out) + (filename ? (strlen(filename) + 1) : 0));
  int sample_fudge;
  
  chans = channels;
  if ( (fudge_factor == 0.0)|| (aud_set==0) )
  {
     fudge_factor = 1.0;
  }
  
  if (audio_mut == NULL)
  {
    audio_mut = mutex_create();
  }
  
  memset (tmpbuf, 0, BUFFER_MAX_FILL);
  memset (sndbuf, 0, BUFFER_MAX_FILL);
  sample_rate = freq;
  sample_fudge = (int)((double)freq * fudge_factor ); /*Fudge this a tad... */
  sbsize = size;
    snd_init();
   sndptr = last_read = snd_ct = 0;
   waiting_for_data = 1;
   sb_min = (freq*2*channels/4 > sbsize) ? freq*2*channels/4 : sbsize; /* 1/4 second or size, whichever is bigger. */
	snd_init();
	
	snd_stream_init();
    	
	shnd = snd_stream_alloc(mpv_callback, sbsize);
	//snd_stream_prefill(shnd);
	if (audio_cond == NULL) audio_cond = cond_create();
  return oss_out;
}


static int oss_write(void *handle, char *buffer, int len)
{

 if (len== -1){
   mutex_lock(audio_mut);
   if (loop_thread->state == STATE_WAIT)
   {
     cond_broadcast(audio_cond);
     thd_schedule_next(loop_thread);
   }
   mutex_unlock(audio_mut);
    if(!aud_set)
        start_audio();
    return 0;
   } /*If this stuff works, try to get it to only call this function once per demuxed audio packet?. */
retry:

  mutex_lock(audio_mut);			
 if (sndptr+len > BUFFER_MAX_FILL)
 {
     /* write what we can before bailing out. */
   /* int new_len;
   new_len = 65534*2 - sndptr;
   if(new_len)
   {
    memcpy (tmpbuf+sndptr, buffer, new_len);
    sndptr+=new_len;
    len -= new_len;
   } */
     if (loop_thread->state == STATE_WAIT)
     {
      cond_broadcast(audio_cond);
      thd_schedule_next(loop_thread);
     }
      mutex_unlock(audio_mut);
      if(!aud_set)
        start_audio();
      //thd_pass();
      goto retry;
 }
 
 memcpy (tmpbuf+sndptr, buffer, len);
 sndptr+= len;
  if (sndptr >= sbsize)
  {
   
   if (loop_thread->state == STATE_WAIT)
     {
      cond_broadcast(audio_cond);
      thd_schedule_next(loop_thread);
     }
    if(!aud_set) start_audio();
  }
 mutex_unlock(audio_mut);
/*  if(!aud_set)
   if (sndptr >= sbsize)
      start_audio(); */

  return 0;
  //return len;
}


static int oss_close(void *handle)
{
  struct oss_out *oss_out = (struct oss_out *)handle;
   
  if (oss_out) {
    free(oss_out);
  }
    aud_set = sndptr = 0;
    
    /* Do this before waiting for the thread to end--if we are waiting on
    this condvar, it'll never exit!*/
    if (audio_cond)
    {
     cond_destroy(audio_cond);
     audio_cond = NULL;
    }
    
    thd_wait(loop_thread);
    fudge_factor = 0.0;
    snd_stream_stop(shnd);
    snd_stream_destroy(shnd);
    snd_shutdown();
    
  return 0;
}

struct audio_out oss_out = { oss_open, oss_write, oss_close };



void *mpg_callback(snd_stream_hnd_t hnd, int len, int * actual)
{
  int wegots;

    if (len >= sndptr)
    {
       wegots = sndptr;
    } else {
       wegots = len;
    }
    
    if (wegots<=0)
    {
      snd_ct = 0;
      last_read = 0;
      *actual = len;
      waiting_for_data = 0;
    } else {
     snd_ct = wegots;    
     *actual = wegots;
     waiting_for_data = 0;
    }
     return tmpbuf;
}

static void *mp3_open(char *filename, int freq, int channels, int format, int endian, uint64_t size)
{
  struct oss_out *oss_out = (struct oss_out *)malloc(sizeof(struct oss_out) + (filename ? (strlen(filename) + 1) : 0));

  chans = channels;
  sample_rate = freq;
  sbsize = size;
    snd_init();
   sndptr = last_read = snd_ct = 0;
	
	memset (tmpbuf, 0, 65534*2);
	
	snd_init();
	
	snd_stream_init();
	
    	
	shnd = snd_stream_alloc(mpg_callback, sbsize);
	snd_stream_prefill(shnd);
	
	snd_stream_start(shnd, sample_rate, chans-1);	
	
  return oss_out;
}

static int mp3_write(void *handle, char *buffer, int len)
{
 
 if(last_read == 1)
 {
   if(sndptr > 0)
    memcpy(tmpbuf, tmpbuf+snd_ct, sndptr);
    last_read = 0; snd_ct = 0;
 }
 memcpy (tmpbuf+sndptr, buffer, len);
 sndptr+= len;

     if(sndptr >= sbsize)
     {
	while(waiting_for_data == 1)	
	{
	  snd_stream_poll(shnd);
	  
	}
	last_read = 1; sndptr -=snd_ct;
	waiting_for_data = 1;
	if (sndptr < 0) sndptr = 0;
    }
    
  return len;
}

static int mp3_close(void *handle)
{
  struct oss_out *oss_out = (struct oss_out *)handle;

  if (oss_out) {
    free(oss_out);
  }

    snd_stream_stop(shnd);
    snd_stream_destroy(shnd);
    snd_shutdown();

  return 0;
}

struct mp3_audio_out mp3_out = { mp3_open, mp3_write, mp3_close };
#else

#include "config.h"
#include "output.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

static void *oss_open(char *filename, int freq, int channels, int format, int endian, uint64_t size)
{
  struct oss_out *oss_out = (struct oss_out *)malloc(sizeof(struct oss_out) + (filename ? (strlen(filename) + 1) : 0));
  int tmp;

  if (!oss_out) {
    fputs("Not enough memory\n", stderr);
    return NULL;
  }
  /*oss_out->fd = -1;
  if (filename) {
    oss_out->filename = (char *)oss_out + sizeof(struct oss_out);
    strcpy(oss_out->filename, filename);
  } else */oss_out->filename = "/dev/dsp";
  if ((oss_out->fd = open(oss_out->filename, O_WRONLY | O_BINARY, S_IREAD | S_IWRITE)) < 0) {
    perror(oss_out->filename);
    free(oss_out);
    return NULL;
  }
  if (endian == 1) {
    format = AFMT_S16_BE;
  } else {
    format = AFMT_S16_LE;
  }
  tmp = format;
  if (ioctl(oss_out->fd, SNDCTL_DSP_SETFMT, &tmp) < 0) {
    perror(oss_out->filename);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  if (tmp != format) {
    fputs("Cannot set output format\n", stderr);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  tmp = channels;
  if (ioctl(oss_out->fd, SNDCTL_DSP_CHANNELS, &tmp) < 0) {
    perror(oss_out->filename);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  if (tmp != channels) {
    fputs("Cannot set output format\n", stderr);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  tmp = freq;
  if (ioctl(oss_out->fd, SNDCTL_DSP_SPEED, &tmp) < 0) {
    perror(oss_out->filename);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  if (tmp != freq) {
    fputs("Cannot set output format\n", stderr);
    close(oss_out->fd);
    free(oss_out);
    exit(1);
    return NULL;
  }
  return oss_out;
}

static int oss_write(void *handle, void *buffer, int size)
{
  struct oss_out *oss_out = (struct oss_out *)handle;
  int r = 0;

  if (oss_out) {
    r = write(oss_out->fd, buffer, size);
    if ((r < 0)) perror(oss_out->filename);
  }
  return r;
}

static int oss_close(void *handle)
{
  int i;
  struct oss_out *oss_out = (struct oss_out *)handle;

  if (oss_out) {
    free(oss_out);
  }
    
  return 0;
}

struct audio_out oss_out = { oss_open, oss_write, oss_close };

#endif
