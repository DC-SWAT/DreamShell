/*
   This File is a part of Dreamcast Media Center
   ADXCORE (c)2012 Josh "PH3NOM" Pearson
   ph3nom.dcmc@gmail.com
  
   decoder algorithm: adv2wav(c)2001 BERO
	http://www.geocities.co.jp/Playtown/2004/
	bero@geocities.co.jp
	adx info from: http://ku-www.ss.titech.ac.jp/~yatsushi/adx.html
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kos/thread.h>

#include "libadx.h"
#include "audio/snddrv.h"

#define	BASEVOL	0x4000
#define PCM_BUF_SIZE 1024*1024

/* A few global vars, should move this into a struct/handle */
static ADX_INFO ADX_Info;
static FILE *adx_in;
static int pcm_samples, loop;
static unsigned char adx_buf[ADX_HDR_SIZE] __attribute__((aligned(32)));

/* Local function definitions */
static int  adx_parse( unsigned char *buf );
static void adx_to_pcm( short *out, unsigned char *in, PREV *prev );
static void *adx_thread(void *p);
static void *pause_thd(void *p);

static int read_be16(unsigned char *buf)     /* ADX File Format is Big Endian */
{
	return (buf[0]<<8)|buf[1];
}

static long read_be32(unsigned char *buf)
{
	return (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
}

/* Start Straming the ADX in a seperate thread */
int adx_dec( char * adx_file, int loop_enable )
{
    printf("LibADX: Checking Status\n");
    if( snddrv.dec_status!=SNDDEC_STATUS_NULL )
    {
        printf("LibADX: Already Running in another process!\n");
        return 0; 
    } 

	adx_in = fopen(adx_file,"rb");                   /* Make sure file exists */
	if(adx_in==NULL)
    {
		printf("LibADX: Can't open file %s\n", adx_file);
        return 0;
	}
	
    if(adx_parse( adx_buf ) < 1) /* Make sure we are working with an ADX file */
    {
        printf("LibADX: Invalid File Header\n");
        fclose( adx_in );
        return 0;
    } 

    printf("LibADX: Starting\n");    
    loop = loop_enable;
    thd_create(0, adx_thread, NULL ); 
    
    return 1;    
}

/* Pause Streaming the ADX (if streaming) */
int adx_pause()
{
    if( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
        return 0;

    printf("LibADX: Pausing\n");
    snddrv.dec_status = SNDDEC_STATUS_PAUSING;
    while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
       thd_pass();
    
    thd_create(0, pause_thd, NULL );
       
    return 1;
}

/* Resume Streaming the ADX (if paused) */
int adx_resume()
{
    if( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
        return 0;
        
    printf("LibADX: Resuming\n");
    snddrv.dec_status = SNDDEC_STATUS_RESUMING;
    while( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
       thd_pass();
        
    return 1;
}

/* Restart Streaming the ADX */
int adx_restart()
{
    if( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
        return 0;

    printf("LibADX: Restarting\n");
    snddrv.dec_status = SNDDEC_STATUS_PAUSING;
    while( snddrv.dec_status != SNDDEC_STATUS_PAUSED )
       thd_pass();
       
    pcm_samples = ADX_Info.samples;
    fseek( adx_in, ADX_Info.sample_offset+ADX_CRI_SIZE, SEEK_SET );
    
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;
    
    return 1;       
}

/* Stop Straming the ADX */
int adx_stop()
{
    if( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
        return 0;

    printf("LibADX: Stopping\n");    
    loop=0;     
    snddrv.dec_status = SNDDEC_STATUS_DONE;
    while( snddrv.dec_status != SNDDEC_STATUS_NULL )
        thd_pass();
         
    return 1;
}

/* Read and parse the ADX File header then seek to the begining sample offset */
static int adx_parse( unsigned char *buf )
{
    fseek( adx_in, 0, SEEK_SET );          /* Read the ADX Header into memory */
	fread( buf, 1, ADX_HDR_SIZE, adx_in );
	if(buf[0]!=ADX_HDR_SIG ) return -1;           /* Check ADX File Signature */
	
    /* Parse the ADX File header */
	ADX_Info.sample_offset = read_be16(buf+ADX_ADDR_START)-2;
	ADX_Info.chunk_size    = buf[ADX_ADDR_CHUNK];
    ADX_Info.channels      = buf[ADX_ADDR_CHAN];
	ADX_Info.rate          = read_be32(buf+ADX_ADDR_RATE);
	ADX_Info.samples       = read_be32(buf+ADX_ADDR_SAMP);
	ADX_Info.loop_type     = buf[ADX_ADDR_TYPE]; 
	
	/* Two known variations for possible loop informations: type 3 and type 4 */
    if( ADX_Info.loop_type == 3 )
	    ADX_Info.loop = read_be32(buf+ADX_ADDR_LOOP);
    else if( ADX_Info.loop_type == 4 )
	    ADX_Info.loop = read_be32(buf+ADX_ADDR_LOOP+0x0c);
    if( ADX_Info.loop > 1 || ADX_Info.loop < 0 )    /* Invalid header check */
        ADX_Info.loop = 0;      
    if( ADX_Info.loop && ADX_Info.loop_type == 3 )
    {
        ADX_Info.loop_samp_start = read_be32(buf+ADX_ADDR_SAMP_START);
        ADX_Info.loop_start      = read_be32(buf+ADX_ADDR_BYTE_START);
        ADX_Info.loop_samp_end   = read_be32(buf+ADX_ADDR_SAMP_END);
        ADX_Info.loop_end        = read_be32(buf+ADX_ADDR_BYTE_END);
    }
    else if( ADX_Info.loop && ADX_Info.loop_type == 4  )
    {
        ADX_Info.loop_samp_start = read_be32(buf+ADX_ADDR_SAMP_START+0x0c);
        ADX_Info.loop_start      = read_be32(buf+ADX_ADDR_BYTE_START+0x0c);
        ADX_Info.loop_samp_end   = read_be32(buf+ADX_ADDR_SAMP_END+0x0c);
        ADX_Info.loop_end        = read_be32(buf+ADX_ADDR_BYTE_END+0x0c);
    }
    if( ADX_Info.loop )
     ADX_Info.loop_samples = ADX_Info.loop_samp_end-ADX_Info.loop_samp_start;
    
    fseek( adx_in, ADX_Info.sample_offset, SEEK_SET ); /* CRI File Signature */
	fread( buf, 1, 6, adx_in );
       	
	if ( memcmp(buf,"(c)CRI",6) )
    {
		printf("Invalid ADX header!\n");
		return -1;
	}

	return 1;
}    

/* Convert ADX samples to PCM samples */
static void adx_to_pcm(short *out,unsigned char *in,PREV *prev)
{
	int scale = ((in[0]<<8)|(in[1]));
	int i;
	int s0,s1,s2,d;

	in+=2;
	s1 = prev->s1;
	s2 = prev->s2;
	for(i=0;i<16;i++) {
		d = in[i]>>4;
		if (d&8) d-=16;
		s0 = (BASEVOL*d*scale + 0x7298*s1 - 0x3350*s2)>>14;
		if (s0>32767) s0=32767;
		else if (s0<-32768) s0=-32768;
		*out++=s0;
		s2 = s1;
		s1 = s0;

		d = in[i]&15;
		if (d&8) d-=16;
		s0 = (BASEVOL*d*scale + 0x7298*s1 - 0x3350*s2)>>14;
		if (s0>32767) s0=32767;
		else if (s0<-32768) s0=-32768;
		*out++=s0;
		s2 = s1;
		s1 = s0;
	}
	prev->s1 = s1;
	prev->s2 = s2;

}

/* Decode the ADX in a seperate thread */
static void *adx_thread(void *p)
{
	//FILE *adx_in;
	//unsigned char buf[ADX_HDR_SIZE];
    unsigned char *pcm_buf=NULL;
	short outbuf[32*2] __attribute__((aligned(32)));
	PREV prev[2];
    //ADX_INFO ADX_Info;
    int pcm_size=0,wsize;
        
    printf("LibADX: %ikHz, %i channel\n", ADX_Info.rate, ADX_Info.channels);
    if( ADX_Info.loop && loop )         	
	    printf("LibADX: Loop Enabled\n");

    pcm_buf = memalign(32, PCM_BUF_SIZE);                    /* allocate PCM buffer */
    if( pcm_buf == NULL )
        goto exit;

	snddrv_start( ADX_Info.rate, ADX_Info.channels );/*Start AICA audio driver*/ 
    snddrv.dec_status = SNDDEC_STATUS_STREAMING;

	prev[0].s1 = 0;
	prev[0].s2 = 0;
	prev[1].s1 = 0;
	prev[1].s2 = 0;
   
    adx_dec:
    pcm_samples = ADX_Info.samples;	
	if (ADX_Info.channels==1)                          /* MONO Decode Routine */
	while(pcm_samples && snddrv.dec_status != SNDDEC_STATUS_DONE) {

        /* Check for request to pause stream */
        if( snddrv.dec_status == SNDDEC_STATUS_PAUSING )
        {
            snddrv.dec_status = SNDDEC_STATUS_PAUSED;
            while( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
               thd_pass();    
        }  

        /* If looping is enabled, check for loop point */ 
        if( loop && ADX_Info.loop )
            if( ftell( adx_in ) >= ADX_Info.loop_end  )
                   goto dec_finished;                   
        
        /* If there is room in PCM buffer, decode next chunk of ADX samples */        
	    if( pcm_size < PCM_BUF_SIZE-16384 )
	    {
		    /* Read the current chunk */
            fread(adx_buf,1,ADX_Info.chunk_size,adx_in);
		    /* Convert ADX chunk to PCM */
		    adx_to_pcm(outbuf,adx_buf,prev);
		    if (pcm_samples>32) wsize=32; else wsize = pcm_samples;
	    	pcm_samples-=wsize;
		    /* Copy the deocded samples to sample buffer */
		    memcpy(pcm_buf+pcm_size, outbuf, wsize*2*2);
		    pcm_size+=wsize*2;
        }
        /* wait for AICA Driver to request some samples */
        while( snddrv.buf_status != SNDDRV_STATUS_NEEDBUF ) thd_pass();
        
        /* Send requested samples to the AICA driver */  
		if ( snddrv.buf_status == SNDDRV_STATUS_NEEDBUF
                        && pcm_size > snddrv.pcm_needed )
        {
            /* Copy the Requested PCM Samples to the AICA Driver */ 
            memcpy( snddrv.pcm_buffer, pcm_buf, snddrv.pcm_needed );
            pcm_size -= snddrv.pcm_needed;   /* Shift the Remaining PCM Samples Back */
            memmove(pcm_buf, pcm_buf+snddrv.pcm_needed, pcm_size);
            /* Let the AICA Driver know the PCM samples are ready */ 
            snddrv.buf_status = SNDDRV_STATUS_HAVEBUF;
        }
	}
	else if (ADX_Info.channels==2)                   /* STEREO Decode Routine */
	while(pcm_samples && snddrv.dec_status != SNDDEC_STATUS_DONE) {
        
        /* Check for request to pause stream */
        if( snddrv.dec_status == SNDDEC_STATUS_PAUSING )
        {
            snddrv.dec_status = SNDDEC_STATUS_PAUSED;
            while( snddrv.dec_status != SNDDEC_STATUS_STREAMING )
               thd_pass();    
        }    
                            
        /* If looping is enabled, check for loop point */   
        if( loop && ADX_Info.loop )
            if( ftell( adx_in ) >= ADX_Info.loop_end  )
                   goto dec_finished;

        /* If there is room in the PCM buffer, decode some ADX samples */        
	    if( pcm_size < PCM_BUF_SIZE - 16384*2 )
	    {
    	    short tmpbuf[32*2];
		    int i;

		    fread(adx_buf,1,ADX_Info.chunk_size*2,adx_in);
		    adx_to_pcm(tmpbuf,adx_buf,prev);
		    adx_to_pcm(tmpbuf+32,adx_buf+ADX_Info.chunk_size,prev+1);
		    for(i=0;i<32;i++) {
			    outbuf[i*2]   = tmpbuf[i];
			    outbuf[i*2+1] = tmpbuf[i+32];
		    }
		    if (pcm_samples>32) wsize=32; else wsize = pcm_samples;
		    pcm_samples-=wsize;
		    memcpy(pcm_buf+pcm_size, outbuf, wsize*2*2);
  		    pcm_size+=wsize*2*2;
        }
        /* wait for AICA Driver to request some samples */
        while( snddrv.buf_status != SNDDRV_STATUS_NEEDBUF ) thd_pass();
              	
        /* Send requested samples to the AICA driver */  
		if ( snddrv.buf_status == SNDDRV_STATUS_NEEDBUF
                        && pcm_size > snddrv.pcm_needed )
        {
            memcpy(snddrv.pcm_buffer, pcm_buf, snddrv.pcm_needed);
            pcm_size-=snddrv.pcm_needed;
            memmove(pcm_buf, pcm_buf+snddrv.pcm_needed, pcm_size);
            snddrv.buf_status = SNDDRV_STATUS_HAVEBUF;
        }
	}
    dec_finished:         /* Indicated samples finished or loop point reached */

    /* If loop is enabled seek to loop starting offset and continue streaming */
    if( loop )
    {
        if( ADX_Info.loop )
        {
            fseek( adx_in, ADX_Info.loop_start, SEEK_SET );
            ADX_Info.samples = ADX_Info.loop_samples;
        }
        else
            fseek( adx_in, ADX_Info.sample_offset+ADX_CRI_SIZE, SEEK_SET );
        goto adx_dec;
    }

    /* At this point, we are finished decoding yet we still have some samples */
    while( pcm_size >= 16384 )
    {
        while( snddrv.buf_status != SNDDRV_STATUS_NEEDBUF )
            thd_pass();
		if ( snddrv.buf_status == SNDDRV_STATUS_NEEDBUF 
                        && pcm_size > snddrv.pcm_needed )
        {
            memcpy(snddrv.pcm_buffer, pcm_buf, snddrv.pcm_needed);
            pcm_size-=snddrv.pcm_needed;
            memmove(pcm_buf, pcm_buf+snddrv.pcm_needed, pcm_size);
            snddrv.buf_status = SNDDRV_STATUS_HAVEBUF;
        }  
    }
    
    printf("LibADX: decode finished\n" );               /* Finished streaming */

    free( pcm_buf );
    pcm_size = 0;
    snddrv_exit();                                   /* Exit the Sound Driver */
	return NULL;

    exit: 
	fclose(adx_in);
	return NULL;
}

/* This thread will handle the 'pausing' routine */
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
