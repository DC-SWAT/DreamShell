/*
**
** LIBMPG123_SNDDRV (C) PH3NOM 2011
**
** All of the input file acces is done through the library
** by using mpg123_open(), and this allows for the use of
** mpg123_read() to get a desired number of pcm samples
** without having to worry about feeding the input buffer 
**
** The last function in this module snddrv_mp3()
** demonstrates decoding an MP2/MP3 in a seperate thread
**
*/

#include <kos.h>
#include <assert.h>

#include "config.h"
#include "compat.h"
#include "mpg123.h"
#include "snddrv.h"

static mpg123_handle *mh;
//static struct mpg123_frameinfo decinfo;
static long hz;
static int chn, enc;
static int err;
static size_t done;
static int id3;//, idx;
static int pos_t;   
static int len_t;


/* Copy the ID3v2 fields into our song info structure */
void print_v2(mpg123_id3v2 *v2)
{    
	printf("%s",        (char*)v2->title );
	printf("%s",        (char*)v2->artist );
	printf("Album: %s", (char*)v2->album );
	printf("Genre: %s", (char*)v2->genre);
}

/* Copy the ID3v1 fields into our song info structure */
void print_v1(mpg123_id3v1 *v1)
{
	printf("%s",        (char*)v1->title );
	printf("%s",        (char*)v1->artist );
	printf("Album: %s", (char*)v1->album );
	//printf("Genre: %i", (char*)v1->genre);
}

static void *pause_thd(void *p)
{
    while( snddrv.dec_status != SNDDEC_STATUS_RESUMING )
    { 
        if ( snddrv.buf_status == SNDDRV_STATUS_NEEDBUF )
        {
            memset( snddrv.pcm_buffer, 0, snddrv.pcm_needed );
            snddrv.buf_status = SNDDRV_STATUS_HAVEBUF;
        }   
        thd_sleep(20);
    }
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;
	return NULL; 
}

/* Proprietary routines for file seeking */
int sndmp3_pause() {
    if( snddrv.dec_status == SNDDEC_STATUS_STREAMING ) {
        snddrv.dec_status = SNDDEC_STATUS_PAUSING;
                
	    if( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
	        return 0;

	    printf("LibADX: Pausing\n");
	    snddrv.dec_status = SNDDEC_STATUS_PAUSING;
	    while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
	       thd_pass();
	    
	    thd_create(0, pause_thd, NULL );
    }
    return snddrv.dec_status;
}

int sndmp3_restart() {
    if( snddrv.dec_status == SNDDEC_STATUS_STREAMING ) {
        snddrv.dec_status = SNDDEC_STATUS_PAUSING;
                
        while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
            thd_pass();
                              
        mpg123_seek( mh, 0, SEEK_SET );
        snddrv.dec_status = SNDDEC_STATUS_STREAMING;
    }
    return snddrv.dec_status;    
}

int sndmp3_rewind() {
    if( snddrv.dec_status == SNDDEC_STATUS_STREAMING ) {
        snddrv.dec_status = SNDDEC_STATUS_PAUSING;
                
        while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
            thd_pass();
                              
        pos_t = mpg123_tell( mh );
        if( pos_t >= SEEK_LEN/4 ) {
            pos_t -= SEEK_LEN/4;
            mpg123_seek( mh, pos_t, SEEK_SET );
        }
                
        snddrv.dec_status = SNDDEC_STATUS_STREAMING;
    }
    return snddrv.dec_status;    
}

int sndmp3_fastforward() {
    if( snddrv.dec_status == SNDDEC_STATUS_STREAMING ) {
        snddrv.dec_status = SNDDEC_STATUS_PAUSING;
                
        while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
            thd_pass();
                    
        pos_t = mpg123_tell( mh );
        if( pos_t <= len_t - SEEK_LEN/4 ) {
            pos_t += SEEK_LEN/4;
            mpg123_seek( mh, pos_t, SEEK_SET );
        }
        
        snddrv.dec_status = SNDDEC_STATUS_STREAMING;
    }
    return snddrv.dec_status; 
}

int sndmp3_stop() {
	if(snddrv.dec_status != SNDDEC_STATUS_NULL) {
		snddrv.dec_status = SNDDEC_STATUS_DONE;
		while( snddrv.dec_status != SNDDEC_STATUS_NULL )
		       thd_pass();
	}
	return 0;
}

int mpg123_callback( ) {
    
    /* Wait for the AICA Driver to signal for more data */
    while( snddrv.buf_status != SNDDRV_STATUS_NEEDBUF ) 
       thd_pass();
    
    /* Decode the data, now that we know how many samples the driver needs */
    err = mpg123_read(mh, (uint8*)snddrv.pcm_buffer, snddrv.pcm_needed, &done); 
    snddrv.pcm_bytes = done;
    snddrv.pcm_ptr = snddrv.pcm_buffer;   

    //printf("SNDDRV REQ: %i, SNDDRV RET: %i\n", snddrv.pcm_needed, snddrv.pcm_bytes );

    /* Check for end of stream and make sure we have enough samples ready */     
    if ( (err == MPG123_DONE) || (done < snddrv.pcm_needed && snddrv.pcm_needed > 0) ){
         snddrv.dec_status = SNDDEC_STATUS_DONE;
         printf("spu_mp3_stream: return done\n");
    }
        
    /* Check for error */  
    if (err == MPG123_ERR ){
        printf("err = %s\n", mpg123_strerror(mh));
        snddrv.dec_status = SNDDEC_STATUS_ERROR;
        printf("spu_mp3_stream: return error\n");
    }
    
    /* Alert the AICA driver that the requested bytes are ready */  
    snddrv.buf_status = SNDDRV_STATUS_HAVEBUF;   
	
	return snddrv.pcm_bytes;
}

int sndmp3_start_block(const char * fname) {

    /* Initialize the Decoder */
    err = mpg123_init();
    if(!mh) {
       printf("Mh init\n");
       mh = mpg123_new(NULL, &err);
    }
    
    if(mh == NULL) {
		printf("Can't init mpg123: %s\n", mpg123_strerror(mh));
		return -1;
	}
   
    /* Open the MP3 context*/
    err = mpg123_open(mh, fname);
    //assert(err == MPG123_OK); 
    
    if(err != MPG123_OK) {
    	mpg123_exit();
    	return -1;
    }
	

    mpg123_getformat(mh, &hz, &chn, &enc);
    printf("MP3 File Info: %ld Hz, %i channels...\n", hz, chn);
	
	if(!hz && !chn) {
		printf("Can't detect format\n");
		mpg123_close(mh);
		mpg123_exit();
		return -1;
	}

    mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;

    /* Get the ID3 Tag data */	
    int meta = mpg123_meta_check(mh);
	if(meta & MPG123_ID3 && mpg123_id3(mh, &v1, &v2) == MPG123_OK)
	{
		printf("Tag data on %s:\n", fname);

		if(v1 != NULL) { 
      	   printf("\n====      ID3v1       ====\n");
           print_v1(v1);
           id3=1;
        }	
		else if(v2 != NULL) {
		   printf("\n====      ID3v2       ====\n");
           print_v2(v2);
           id3=1;
        }
	}

	
	/*
	mpg123_info(mh, &decinfo);
	
	printf("Output Sampling rate = %ld\r\n", decinfo.rate);
	printf("Output Bitrate       = %d\r\n", decinfo.bitrate);
	printf("Output Frame size    = %d\r\n", decinfo.framesize);
	*/
	len_t = mpg123_length(mh);
    snd_sinfo.slen = len_t / hz;
                 
    /* Start the AICA Driver */
    snddrv_start( hz, chn ); 
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;

    printf("spu_wave_stream: beginning\n");
    
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;

	while( snddrv.dec_status != SNDDEC_STATUS_DONE && snddrv.dec_status != SNDDEC_STATUS_ERROR ) {

        /* A request has been made to access the decoder handle */
		if( snddrv.dec_status == SNDDEC_STATUS_PAUSING )
            snddrv.dec_status = SNDDEC_STATUS_PAUSED;

        /* Wait for request to complete    */
        while( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
            thd_pass(); 
            
		mpg123_callback( );
		
		//timer_spin_sleep(10);
	}

    printf("spu_wave_stream: finished\n");

    /* Exit the AICA Driver */
    snddrv_exit();

    /* Release the audio decoder */       
    mpg123_close(mh);
    mpg123_exit();
    
    err = 0;
    hz = 0;
    chn = 0;
    done = 0;
    enc = 0;
    snddrv.pcm_ptr = 0;
    id3 = 0;
    sq_clr( snddrv.pcm_buffer, 65536 );
    
    SNDDRV_FREE_SINFO();

    snddrv.dec_status = SNDDEC_STATUS_NULL;
        
    printf("spu_mp3_stream: exited\n");
    
	return 0;
}

void *snddrv_mp3_thread() {
	while( snddrv.dec_status != SNDDEC_STATUS_DONE && snddrv.dec_status != SNDDEC_STATUS_ERROR ) {

        /* A request has been made to access the decoder handle */
		if( snddrv.dec_status == SNDDEC_STATUS_PAUSING )
            snddrv.dec_status = SNDDEC_STATUS_PAUSED;

        /* Wait for request to complete    */
        while( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
            thd_pass(); 
            
		mpg123_callback( );
		
		//timer_spin_sleep(30);
	}

    printf("spu_wave_stream: finished\n");

    /* Exit the AICA Driver */
    snddrv_exit();

    /* Release the audio decoder */       
    mpg123_close(mh);
    mpg123_exit();
    
    err = 0;
    hz = 0;
    chn = 0;
    done = 0;
    enc = 0;
    snddrv.pcm_ptr = 0;
    sq_clr( snddrv.pcm_buffer, 65536 );

    SNDDRV_FREE_SINFO();
    
    snddrv.dec_status = SNDDEC_STATUS_NULL;

    printf("spu_mp3_stream: exited\n");
    
	return NULL;
}

int sndmp3_start(const char * fname, int loop) {

    /* Initialize the Decoder */
    err = mpg123_init();
    if(!mh) {
       printf("Mh init\n");
       mh = mpg123_new(NULL, &err);
    }
    //assert(mh != NULL);
    if(mh == NULL) return -1;
   
    /* Open the MP3 context*/
    err = mpg123_open(mh, fname);
    //assert(err == MPG123_OK); 
    
    if(err != MPG123_OK) {
    	mpg123_exit();
    	return -1;
    }

    mpg123_getformat(mh, &hz, &chn, &enc);
    printf("MP3 File Info: %ld Hz, %i channels...\n", hz, chn);
	
	
	if(!hz && !chn) {
		printf("Can't detect format\n");
		mpg123_close(mh);
		mpg123_exit();
		return -1;
	}
	
	/*
	mpg123_info(mh, &decinfo);
	
	printf("Output Sampling rate = %ld\n", decinfo.rate);
	printf("Output Bitrate       = %d\n", decinfo.bitrate);
	printf("Output Frame size    = %d\n", decinfo.framesize);
	*/
	len_t = mpg123_length(mh);
    snd_sinfo.slen = len_t / hz;
                
    /* Start the AICA Driver */
    snddrv_start( hz, chn ); 
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;

    printf("spu_wave_stream: beginning\n");
    
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;

    /* Start the DECODER thread */
    thd_create(0, snddrv_mp3_thread, NULL );
    return 0;
}
