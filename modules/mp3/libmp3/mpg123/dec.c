/*
**
** mpg123spu.c
** (C) PH3NOM 2011
**
*/

#include <kos/thread.h>
#include <dc/sound/stream.h>

#include <assert.h>
#include <string.h>
#include "stdio.h"
#include "sys/types.h"

#include "mpg123.h"

#include "mpg123spu.h"

#define BS_SIZE 65536
#define BS_MIN  16*1024      

FILE * mp3fd;
mpg123_handle *mh;
char * mp3file[128];

static unsigned int buffer[BS_SIZE], *buf_ptr;
unsigned int outbuf[BS_SIZE+16384], *outbuf_ptr;
static int pcm_count, pcm_discard;
static int err, bs_count, rsize;
unsigned int done;

long rate;
int channels, enc;

static snd_stream_hnd_t stream_hnd = -1;
static volatile int spu_status;
static kthread_t * adec_thread;

/* Exit the audio decoder. This is called from outside the decode thread. */
int libmpg123_quit() {
    spu_status = SPU_STATUS_DONE;
    //thd_wait(adec_thread);
    while( spu_status != SPU_STATUS_NULL )
      thd_sleep(10);

    printf("MPG123 Exited ok\n");
}

/* Exit the audio decoder. */
static int mpg_quit() {

    /* Release the audio decoder */       
    mpg123_close(mh);
    mpg123_exit();
                 
    /* Release the audio driver */
    fs_close(mp3fd);
    //free(outbuf);
    //free(buffer);
    sq_clr( mp3file, sizeof(mp3file));
    sq_clr( buffer, sizeof(buffer));
    sq_clr( outbuf, sizeof(outbuf));
   
    printf("LibMPG123 Decoder Finished.\n");

    return 0;
}

/* Initialize the audio decoder */
static int mp3_init( ) {
   
    int i;
    int err = 0;

    /* Open the MP3 File */
    mp3fd = fs_open( mp3file, O_RDONLY );
    assert( mp3fd!= NULL );

    buf_ptr = buffer;
    outbuf_ptr = outbuf;
    pcm_count = pcm_discard = 0;

    /* Initialize the Decoder */
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    assert(mh != NULL);
   
    /* Open the MP3 context in open_fd mode */
    err = mpg123_open_fd(mh, mp3fd);
    assert(err == MPG123_OK);   

}

/* Check to see if more input data is needed, then do so */
static int mp3_read() {

    /* Get the current bitstream offset */
    off_t mp3pos = mpg123_tell_stream (   mh );
   
    /* If the buffer is below the minimum value, pull in more data */   
    if( bs_count <= BS_MIN || err == MPG123_NEED_MORE ) {

       memcpy(buffer, buf_ptr, bs_count);
   
       long bytes_read = fs_read( mp3fd, buffer+bs_count, BS_SIZE-bs_count );
   
       bs_count+= bytes_read; buf_ptr = buffer;
       
    }

    /* Get the bitstream read size */
    int read_size = mpg123_tell_stream ( mh ) - mp3pos;
   
    return read_size;     

}

/* Taken from old K:OS MP3 decoder */
static int pcm_empty(int size) {
   if (pcm_count >= size) {
      /* Shift everything back */
      memcpy(outbuf, outbuf + size, pcm_count - size);

      /* Shift pointers back */
      pcm_count -= size;
      outbuf_ptr = outbuf + pcm_count;
   }

   return 0;
}

/* LibMPG123 Decoder callback */
static int mp3_decode(snd_stream_hnd_t hnd, int size, int * actual) {
       
        /* Update the PCM buffer */
        pcm_empty(pcm_discard);
                       
        /* Read the bitstream */
        rsize = mp3_read();
               
        /* Decode the mpeg data */
        err = mpg123_decode(mh, buf_ptr, rsize, outbuf, size, &done); 
       
        /* Adjust the pointers */
        buf_ptr += rsize;
        bs_count -= rsize;
        outbuf_ptr += done;
        pcm_count += done;
        pcm_discard = *actual = done;

        /* If this is the first frame, intitialize the output structures */
        if (err == MPG123_NEW_FORMAT ){
            mpg123_getformat(mh, &rate, &channels, &enc);
            printf("MP3 File Info: %iHz, %i channels...\n", rate, channels);
        }
       
        /* Sould we exit? */
        if (err == MPG123_ERR){
            printf("err = %s\n", mpg123_strerror(mh));
            spu_status = SPU_STATUS_DONE;
        }
       
        return outbuf;

}

static int mp3_thread() {

    printf("MP3 Decoder Thread Starting\n");

    if( spu_status != SPU_STATUS_NULL ) {
        printf("SPU ERROR: Already Running!\n");
        return -1;
    }

    while( spu_status != SPU_STATUS_DONE) {
       
    switch( spu_status ) {
         
      case SPU_STATUS_NULL:
         mp3_init();   
        if( err == MPG123_OK ) {
           stream_hnd = snd_stream_alloc(NULL, SND_STREAM_BUFFER_MAX);
           assert( stream_hnd != -1 );
            spu_status = SPU_STATUS_INITIALIZED;
         }
         break;
      case SPU_STATUS_INITIALIZED:
         snd_stream_set_callback(stream_hnd, mp3_decode);
         snd_stream_prefill( stream_hnd );
         if( spu_status != SPU_STATUS_DONE )
            spu_status = SPU_STATUS_READY;
         break;
      case SPU_STATUS_READY:   
         snd_stream_start(stream_hnd, rate, channels - 1);
         spu_status = SPU_STATUS_STREAMING;
         break;
      case SPU_STATUS_STREAMING:                   
         while( spu_status != SPU_STATUS_DONE ){
            if (snd_stream_poll(stream_hnd) < 0)
               printf("Nothing decoded\n");
            else
               thd_sleep(50);
         }
         printf("SPU_STATUS: Finish Decode\n");
         break;
      case SPU_STATUS_DONE:
         printf("Shutting Down LibMPG123 and soundstream\n");   
         mpg_quit();
         snd_stream_stop(stream_hnd);
        snd_stream_destroy(stream_hnd);
        break;
     
      default:
         mpg_quit();
         snd_stream_stop(stream_hnd);
        snd_stream_destroy(stream_hnd);
        break;   
    }
   
    }         

    spu_status = SPU_STATUS_NULL;
    printf("MP3 Decoder Thread Finished\n");
   
    return 0;
}

/* Create a new thread to decode the MP3 File */
void libmpg123_dec( char * mp3_file ) {
   
    sprintf( mp3file, "%s", mp3_file );
    adec_thread = thd_create( mp3_thread, NULL);

}