/*
**
** This File is a part of Dreamcast Media Center
** (C) Josh "PH3NOM" Pearson 2011
**
*/

#ifndef SNDDRV_H
#define SNDDRV_H

/* Keep track of things from the Driver side */
#define SNDDRV_STATUS_NULL         0x00
#define SNDDRV_STATUS_INITIALIZING 0x01
#define SNDDRV_STATUS_READY        0x02
#define SNDDRV_STATUS_STREAMING    0x03
#define SNDDRV_STATUS_DONE         0x04
#define SNDDRV_STATUS_ERROR        0x05

/* Keep track of things from the Decoder side */
#define SNDDEC_STATUS_NULL         0x00
#define SNDDEC_STATUS_INITIALIZING 0x01
#define SNDDEC_STATUS_READY        0x02
#define SNDDEC_STATUS_STREAMING    0x03
#define SNDDEC_STATUS_PAUSING      0x04
#define SNDDEC_STATUS_PAUSED       0x05
#define SNDDEC_STATUS_RESUMING     0x06
#define SNDDEC_STATUS_DONE         0x07
#define SNDDEC_STATUS_ERROR        0x08

/* Keep track of the buffer status from both sides*/
#define SNDDRV_STATUS_NEEDBUF      0x00
#define SNDDRV_STATUS_HAVEBUF      0x01
#define SNDDRV_STATUS_BUFEND       0x02

/* This seems to be a good number for file seeking on compressed audio */
#define SEEK_LEN 16384*48

/* SNDDRV (C) AICA Audio Driver */
struct snddrv {
       int rate;
       int channels;
       int pcm_bytes;
       int pcm_needed;
       volatile int drv_status;
       volatile int dec_status;
       volatile int buf_status;
       unsigned int pcm_buffer[65536+16384];
       unsigned int *pcm_ptr;
}snddrv;

#define SNDDRV_FREE_STRUCT() { \
        snddrv.rate = snddrv.channels = snddrv.drv_status = \
        snddrv.dec_status = snddrv.buf_status = 0; }
        
struct snddrv_song_info {
     char *artist[128];
     char * title[128];
     char * track[128];
     char * album[128];
     char * genre[128];
     char *fname;
     volatile int fpos;
     volatile float spos;
     int fsize;
     float slen;  
}snd_sinfo;

#define SNDDRV_FREE_SINFO() { \
        sq_clr( snd_sinfo.artist, 128 ); \
        sq_clr( snd_sinfo.title, 128 ); \
        sq_clr( snd_sinfo.track, 128 ); \
        sq_clr( snd_sinfo.album, 128 ); \
        sq_clr( snd_sinfo.genre, 128 ); \
        snd_sinfo.fpos = snd_sinfo.spos = snd_sinfo.fsize = snd_sinfo.slen = 0; }

#define min(a,b) ( (a) < (b) ? (a) : (b) )

#define MAX_CHANNELS 6 /* make this higher to support files with
                          more channels for LibFAAD */

/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000
        
/* SNDDRV Public Function Protocols */

/* Start the Sound Driver Thread */
int snddrv_start( int rate, int chans );

/* Exit the Sound Driver */
int snddrv_exit();

/* Increase Sound Volume */
int snddrv_volume_up();

/* Decrease Sound Volume */
int snddrv_volume_down();

#endif
