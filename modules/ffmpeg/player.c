/* DreamShell ##version##

   player.c - ffmpeg player
   Copyright (C)2011-2014 SWAT
*/

#define USE_DIRECT_AUDIO 1
//#define USE_HW_YUV 1
            
#include "ds.h"
#ifdef USE_DIRECT_AUDIO
#include "aica.h"
#include "drivers/aica_cmd_iface.h"
#else
#include "drivers/aica_cmd_iface.h"
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"


#define ASIC_EVT_PVR_YUV_DONE 0x0006

#define PVR_YUV_STAT 0x0150
#define PVR_YUV_CONV 0x10800000

#define PVR_YUV_FORMAT_YUV420 0
#define PVR_YUV_FORMAT_YUV422 1

#define PVR_YUV_MODE_SINGLE 0
#define PVR_YUV_MODE_MULTI 1


typedef struct video_txr {
	pvr_poly_hdr_t hdr;
	int tw, th;
	int width, height;
	void (*render_cb)(void *);
	pvr_ptr_t addr;
	void *backbuf;
} video_txr_t;


#ifndef USE_HW_YUV

static void yuvtex(uint16 *tbuf,int tstride,unsigned width,unsigned height,uint8 *ybuf,int ystride,uint8 *ubuf,int ustride,uint8 *vbuf,int vstride) {
	int h = height/2;
	uint8 u, v;

	do {
		uint8 *uptr,*vptr,*yptr,*yptr2;
		uint8 *tex,*tex2;
		int w = width/2;

		tex  = (uint8*)tbuf; tbuf+=tstride;
		tex2 = (uint8*)tbuf; tbuf+=tstride;
		yptr  = ybuf; ybuf +=ystride;
		yptr2 = ybuf; ybuf +=ystride;
		uptr = ubuf; ubuf +=ustride;
		vptr = vbuf; vbuf +=vstride;
		do {
#if 1
			//uint8 u,v;
			u = *uptr++;
			v = *vptr++;
		//	asm("pref @%0"::"r"(yptr+4));
			tex[0] = u;
			tex[1] = *yptr++;
			tex[2] = v;
			tex[3] = *yptr++;
			tex2[0] = u;
			tex2[1] = *yptr2++;
			tex2[2] = v;
			tex2[3] = *yptr2++;
			tex+=4;
			tex2+=4;
#else
			//uint8 u,v;
			u = *uptr++;
			v = *vptr++;
#define	store(base,offset,data) __asm__("mov.b %1,@(" #offset ",%0)"::"r"(base),"z"(data))
			tex[0] = u;
			store(tex,1,*yptr++);
			store(tex,2,v);
			store(tex,3,*yptr++);
			tex2[0] = u;
			store(tex2,1,*yptr2++);
			store(tex2,2,v);
			store(tex2,3,*yptr2++);
			tex+=4;
			tex2+=4;
#endif
		} while(--w);
	} while(--h);

}

#else

//static uint32 yuv_inited = 0;
static semaphore_t yuv_done;
void block8x8_copy(void *src, void *dst, uint32 stride);

uint32 mblock_copy(void *lum, void *b, void *r, void *dst, int w, int h) {
	uint32 x, y;
	uint32 old_dst = (uint32)dst;

	for ( y = 0; y < h / 16; y++ ) {
		
		for ( x = 0; x < w / 2; x += 8 ) {
			
			block8x8_copy ( b + x, dst, w / 2 );
			dst += 64;
			block8x8_copy ( r + x, dst, w / 2 );
			dst += 64;

			block8x8_copy ( lum + x * 2, dst, w );
			dst += 64;
			block8x8_copy ( lum + x * 2 + 8, dst, w );
			dst += 64;
			block8x8_copy ( lum + x * 2 + w * 8, dst, w );
			dst += 64;
			block8x8_copy ( lum + x * 2 + 8 + w * 8, dst, w );
			dst += 64;
		}
		
		lum += w * 16;
		b += w * 4;
		r += w * 4;
	}
	
	return (uint32)dst - old_dst;
}


static void asic_yuv_evt_handler(uint32 code) {
	sem_signal(&yuv_done);
	thd_schedule(1, 0);
}


int yuv_conv_init() {
	asic_evt_set_handler(ASIC_EVT_PVR_YUV_DONE, asic_yuv_evt_handler);
	asic_evt_enable(ASIC_EVT_PVR_YUV_DONE, ASIC_IRQ_DEFAULT);
	sem_init(&yuv_done, 0);
	return 0;
}

void yuv_conv_shutdown() {
	asic_evt_disable(ASIC_EVT_PVR_YUV_DONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(ASIC_EVT_PVR_YUV_DONE, NULL);
	sem_destroy(&yuv_done);
}

void yuv_conv_setup(pvr_ptr_t addr, int mode, int format, int width, int height) {
	PVR_SET(PVR_YUV_CFG_1, ((((width / 16))-1) | ((height / 16)-1) << 8 | mode << 16 | format << 24));
	PVR_SET(PVR_YUV_ADDR, (uint32)addr);
}


void yuv_conv_frame(video_txr_t *txr, AVFrame *frame, AVCodecContext *codec, int block) {
	
	uint32 size = mblock_copy(frame->data[0], frame->data[1], frame->data[2], txr->backbuf, codec->width, codec->height);
	dcache_flush_range((uint32)txr->backbuf, size);
	
	while (!pvr_dma_ready());
	pvr_dma_transfer((void*)txr->backbuf, PVR_YUV_CONV, size, PVR_DMA_TA, 0, NULL, 0);
	
	if(block) {
		sem_wait(&yuv_done);
//		printf("PVR: YUV Macroblocks Converted: %ld\n", PVR_GET(PVR_YUV_STAT));
	}
}

#endif


static void FreeVideoTexture(video_txr_t *txr) {
	if (txr->addr) {
		pvr_mem_free(txr->addr);
		txr->addr = NULL;
	}
	if (txr->backbuf) {
		free(txr->backbuf);
		txr->backbuf = NULL;
	}
}

static void MakeVideoTexture(video_txr_t *txr, int width, int height, int format, int filler) {
	pvr_poly_cxt_t tmp;

	int tw,th;
	for(tw = 8; tw < width ; tw <<= 1);
	for(th = 8; th < height; th <<= 1);

	txr->width = width;
	txr->height = height;
	txr->tw = tw;
	txr->th = th;
	
	if(txr->addr || txr->backbuf) {
		FreeVideoTexture(txr);
	}
	
	txr->backbuf = memalign(32, txr->tw * txr->height * 2);
	txr->addr = pvr_mem_malloc(tw * th * 2);
	
	pvr_poly_cxt_txr(&tmp, PVR_LIST_OP_POLY, format, tw, th, txr->addr, filler);
	pvr_poly_compile(&txr->hdr, &tmp);
}



static void RenderVideoTexture(video_txr_t *txr, int x, int y, int w, int h, int color) {
	
	pvr_vertex_t v;
	pvr_prim(&txr->hdr, sizeof(txr->hdr));

	float u0,v0,u1,v1;
	float x0,y0,x1,y1;

	u0 = 0;
	v0 = 0;
	u1 = (float)txr->width/txr->tw;
	v1 = (float)txr->height/txr->th;

	x0 = x;
	y0 = y;
	x1 = x + w;
	y1 = y + h;

	v.flags = PVR_CMD_VERTEX;
	v.argb = color;
	v.oargb = 0;
	v.z = 512;

	v.x = x0;
	v.y = y0;
	v.u = u0;
	v.v = v0;
	pvr_prim(&v,sizeof(v));

	v.x = x1;
	v.y = y0;
	v.u = u1;
	v.v = v0;
	pvr_prim(&v,sizeof(v));

	v.x = x0;
	v.y = y1;
	v.u = u0;
	v.v = v1;
	pvr_prim(&v,sizeof(v));

	v.flags = PVR_CMD_VERTEX_EOL;
	v.x = x1;
	v.y = y1;
	v.u = u1;
	v.v = v1;
	pvr_prim(&v,sizeof(v));
}



static void RenderVideo(video_txr_t *txr, AVFrame *frame, AVCodecContext *codec) {

	switch(codec->pix_fmt) {
		case PIX_FMT_YUVJ420P:
		case PIX_FMT_YUVJ422P:
		case PIX_FMT_YUV420P:

#ifdef USE_HW_YUV
			yuv_conv_frame(txr, frame, codec, -1);
#else
			yuvtex(txr->backbuf, txr->tw, codec->width, codec->height,
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]
			);
			
//			dcache_flush_range((unsigned)txr->backbuf, txr->tw * codec->height * 2);
//			while (!pvr_dma_ready());
//			pvr_txr_load_dma(txr->backbuf, txr->addr, txr->tw * codec->height * 2, -1, NULL, 0);
			sq_cpy(txr->addr, txr->backbuf, txr->tw * codec->height * 2);
#endif
			break;
		case PIX_FMT_UYVY422:
//			dcache_flush_range((unsigned)frame->data[0], txr->tw * codec->height * 2);
//			while (!pvr_dma_ready());
//			pvr_txr_load_dma((uint8 *)(((uint32)frame->data[0] + 31) & (~31)), txr->addr, txr->tw * codec->height * 2, -1, NULL, 0);
			sq_cpy(txr->addr, frame->data[0], txr->tw * codec->height * 2);
			break;
		default:
//			dcache_flush_range((unsigned)frame->data[0], txr->tw * codec->height * 2);
//			while (!pvr_dma_ready());
//			pvr_txr_load_dma((uint8 *)(((uint32)frame->data[0] + 31) & (~31)), txr->addr, txr->tw * codec->height * 2, -1, NULL, 0);
			sq_cpy(txr->addr, frame->data[0], txr->tw * codec->height * 2);
			break;
	}
	
	pvr_wait_ready();
	pvr_scene_begin();
	
	pvr_list_begin(PVR_LIST_OP_POLY);
	
	if (txr->addr) {
		int dispw = 640;
		int disph = 640 * txr->height/txr->width;
		RenderVideoTexture(txr, (640-dispw)/2, (480-disph)/2, dispw, disph, 0xFFFFFFFF);
	}

	pvr_list_finish();
	
	if(txr->render_cb != NULL) {
		txr->render_cb((void *)txr);
	}
	
	pvr_scene_finish();
}


#ifdef USE_DIRECT_AUDIO

static struct {
	int rate;
	int channels;
	int bits;
	int bytes;

	int playing;
	int writepos;
	int sampsize;
	int leftbuf,rightbuf;
} auds,*aud=&auds;

//static int16 *sep_buffer[2] = {NULL, NULL};

/* sound */
static void audioinit(AVCodecContext *audio) {
	printf("bitrate=%d sample_rate=%d channels=%d framesize=%d bits=%d\n",
		audio->bit_rate,audio->sample_rate,audio->channels,audio->frame_size,audio->frame_bits
	);

	aud->rate = audio->sample_rate;
	aud->channels = audio->channels;
	aud->bits = audio->frame_bits;
	if (aud->bits == 0) aud->bits = 16;
	aud->bytes = aud->bits / 8;
	//spu_dma_init();
	/*
	if (!sep_buffer[0]) {
		sep_buffer[0] = memalign(32, (SND_STREAM_BUFFER_MAX/2));
		sep_buffer[1] = memalign(32, (SND_STREAM_BUFFER_MAX/2));
	}*/
}

static void audio_end() {
	aud->playing = 0;
	aica_stop(0);
	aica_stop(1);
	
	/* Free global buffers */
	/*
	if (sep_buffer[0]) {
		free(sep_buffer[0]);	sep_buffer[0] = NULL;
		free(sep_buffer[1]);	sep_buffer[1] = NULL;
	}*/
}

#define G2_LOCK(OLD) \
	do { \
		if (!irq_inside_int()) \
			OLD = irq_disable(); \
		/* suspend any G2 DMA here... */ \
		while((*(volatile unsigned int *)0xa05f688c) & 0x20) \
			; \
	} while(0)

#define G2_UNLOCK(OLD) \
	do { \
		/* resume any G2 DMA here... */ \
		if (!irq_inside_int()) \
			irq_restore(OLD); \
	} while(0)

#define	SPU_RAM_BASE	0xa0800000



#if 0

/* Performs stereo seperation for the two channels; this routine
   has been optimized for the SH-4. */
static void sep_data(void *buffer, int len, int stereo) {

#if 1

	register int16	*bufsrc, *bufdst_left, *bufdst_right;
	register int cnt;
	
	cnt = len / 2;
	bufsrc = (int16*)buffer;
	bufdst_left = sep_buffer[0];
	bufdst_right = sep_buffer[1];
	
	while(cnt) {
		*bufdst_left++ = *bufsrc++;
		*bufdst_right++ = *bufsrc++;
		cnt -= 2;
	}

#else

	register int16	*bufsrc, *bufdst;
	register int	x, y, cnt;

	if (stereo) {
		bufsrc = (int16*)buffer;
		bufdst = sep_buffer[0];
		x = 0; y = 0; cnt = len / 2;
		do {
			*bufdst = *bufsrc;
			bufdst++; bufsrc+=2; cnt--;
		} while (cnt > 0);

		bufsrc = (int16*)buffer; bufsrc++;
		bufdst = sep_buffer[1];
		x = 1; y = 0; cnt = len / 2;
		do {
			*bufdst = *bufsrc;
			bufdst++; bufsrc+=2; cnt--;
			x+=2; y++;
		} while (cnt > 0);
	} else {
		memcpy(sep_buffer[0], buffer, len);
		memcpy(sep_buffer[1], buffer, len);
	}
#endif
}

#endif


#if 0
static int _spu_dma_transfer(void *from, uint32 dest, uint32 length, int block,
	g2_dma_callback_t callback, ptr_t cbdata)
{
	/* Adjust destination to SPU RAM */
	dest += 0x00800000;

	return g2_dma_transfer(from, (void *) dest, length, block, callback, cbdata, 0, 
		SPU_DMA_MODE, 1, 1);
}


static uint32 dmadest, dmacnt;

static void spu_dma_transfer2(ptr_t data) {
	spu_dma_transfer(sep_buffer[1], dmadest, dmacnt, -1, NULL, 0);
}
#endif


/*
static void spu_memload_stereo16(int leftpos, int rightpos, void *src0, size_t size) {

	register uint16 *src   = (uint16 *)src0;
	register uint32 *left  = (uint32 *)(leftpos  + SPU_RAM_BASE);
	register uint32 *right = (uint32 *)(rightpos + SPU_RAM_BASE);
	
//	size = (size+7)/8;
	unsigned int old = 0;
	
	while(size) {

		g2_fifo_wait();
		G2_LOCK(old);
		
		left[0]  = src[0] | (src[2] << 16);
		right[0] = src[1] | (src[3] << 16);
		
		left[1]  = src[4] | (src[6] << 16);
		right[1] = src[5] | (src[7] << 16);
		
		G2_UNLOCK(old);
		
		src += 8;
		left += 2;
		right += 2;
		size -= 16;
	}
}
*/


static void spu_memload_stereo16(int leftpos, int rightpos, void *src0, size_t size)
{
	register uint16 *src = src0;

#if 0
	sep_data(src, size, 1);
	dcache_flush_range((unsigned)sep_buffer, size);
	
	dmadest = rightpos;
	dmacnt = size / 2;
	
	spu_dma_transfer(sep_buffer[0], leftpos, size / 2, -1, spu_dma_transfer2, 0);
	//spu_dma_transfer(sep_buffer[1], rightpos, size / 2, -1, NULL, 0);
#else

	register uint32 *left  = (uint32*)(leftpos  + SPU_RAM_BASE);
	register uint32 *right = (uint32*)(rightpos + SPU_RAM_BASE);
	
	size = (size+7)/8;
	unsigned lval, rval, old = 0;
	
	while(size--) {
		
		lval = *src++;
		rval = *src++;
		lval|= (*src++)<<16;
		rval|= (*src++)<<16;
		
		g2_fifo_wait();
		G2_LOCK(old);
		*left++=lval;
		*right++=rval;
		G2_UNLOCK(old);
		/*
		g2_write_32(*left++,lval);
		g2_write_32(*right++,rval);
		g2_fifo_wait();*/
	}
#endif
}

/*
static void spu_memload_mono16(int pos, void *src0, size_t size) {
	uint16 *src = (uint16 *)(((uint32)src0 + 31) & (~31));
	dcache_flush_range((unsigned)src, size);
	spu_dma_transfer(src, pos, size, -1, NULL, 0);
}*/


static void audio_write(AVCodecContext *audio, void *buf, size_t size) {
	
	if(!aud->channels) {
		return;
	}
	
	int nsamples = size/(aud->channels*aud->bytes);
	int writepos = 0;
	int curpos =0;

	if (!aud->playing) {

		aud->sampsize = (65535)/nsamples*nsamples;
		//printf("audio size=%d n=%d sampsize=%d\n",size,nsamples,aud->sampsize);
		aud->leftbuf  = 0x11000;//AICA_RAM_START;//
		aud->rightbuf = 0x11000/*AICA_RAM_START*/ + aud->sampsize*aud->bytes;
		aud->writepos = 0;
	} else {
		//curpos = aica_get_pos(0);
		//if (writepos < curpos && curpos < writepos+nsamples) {
			//printf("audio:wait");
		//}
		/*
		while(aud->writepos < curpos && curpos < aud->writepos + nsamples) {
			curpos = aica_get_pos(0);
			//timer_spin_sleep(50);
			//thd_sleep(50);
			//thd_pass();
			thd_sleep(10);
			//timer_spin_sleep(2);
		}
		*/
		writepos = aud->writepos;
		curpos = aica_get_pos(0);

		do {
			curpos = aica_get_pos(0);
		} while(writepos < curpos && curpos < writepos + nsamples);
		//audio_end();
	}
	
	writepos = aud->writepos * aud->bytes;
	
	if (aud->channels == 1) {
		spu_memload(aud->leftbuf + writepos, buf, size);
		//spu_memload_mono16(aud->leftbuf + writepos, buf, size);
	} else {
		spu_memload_stereo16(aud->leftbuf + writepos, aud->rightbuf + writepos, buf, size);
	}

	aud->writepos += nsamples;
	
	if (aud->writepos >= aud->sampsize) {
		aud->writepos = 0;
	}

	if (!aud->playing) {
		
		int mode;
		int vol = 255;
		aud->playing = 1;
		mode = (aud->bits == 16) ? SM_16BIT : SM_8BIT;
		
		if (aud->channels == 1) {
			aica_play(0,mode,aud->leftbuf,0,aud->sampsize,aud->rate,vol,128,1);
		} else {
		//	printf("%d %d\n",aud->sampsize,aud->rate);
			aica_play(0,mode,aud->leftbuf ,0,aud->sampsize,aud->rate,vol,  0,1);
			aica_play(1,mode,aud->rightbuf,0,aud->sampsize,aud->rate,vol,255,1);
		}
	}
}



#else /* NOT USE_DIRECT_AUDIO*/


#define BUFFER_MAX_FILL 65536+16384
//static char tmpbuf[BUFFER_MAX_FILL]; // Temporary storage space for PCM data--65534 16-bit
									 // samples should be plenty of room--that's about 0.5 seconds. 
//static char sndbuf[BUFFER_MAX_FILL];

static uint8 *tmpbuf, *sndbuf;

static int sndptr;
static int sbsize;
static int last_read, snd_ct;

static int waiting_for_data = 0;
static snd_stream_hnd_t shnd;
static kthread_t * loop_thread;
static mutex_t * audio_mut = NULL;
static int aud_set = 0;
static int sample_rate;
static int chans;


void *aica_audio_callback(snd_stream_hnd_t hnd, int len, int * actual) {
	int wegots;

	mutex_lock(audio_mut); 

	if (len >= sndptr) {
		wegots = sndptr;
	} else {
		wegots = len;
	}

	if (wegots <= 0) {
		snd_ct = 0;
		last_read = 0;
		*actual = chans * 2; /* This seems... wrong.  But how should we handle such a case? */
		waiting_for_data = 0; /* return NULL here? We will end up with a gap if we're not at the
		start of a video.  We could say *actual = 1; or something? 
		That might eliminate the lag at the beginning of a video? */

		mutex_unlock(audio_mut);
		return NULL;
	} else {
		snd_ct = wegots;    
		*actual = wegots;
		waiting_for_data = 0;
	}

	memcpy (sndbuf, tmpbuf, snd_ct);

	last_read = 1;
	sndptr -=snd_ct;

	memcpy(tmpbuf, tmpbuf+snd_ct, sndptr);
	last_read = 0; 
	snd_ct = 0;
	mutex_unlock(audio_mut);
	
	return sndbuf;
}


/* This loops as long as the audio driver is open.
   Do we want to poll more frequently? */
static void *play_loop(void* yarr) {

	while (aud_set == 1) {  

		if (sndptr >= sbsize) { 

			while(waiting_for_data == 1) {
				snd_stream_poll(shnd);
				thd_pass();
				if (aud_set == 0) {
					return NULL;
				}
			}

			waiting_for_data = 1;
		
		} else {
			thd_pass();
		} 

	}  /* while (aud_set == 1) */
	return NULL;
}

static void start_audio() {
	aud_set = 1;
	//snd_stream_prefill(shnd);
	snd_stream_start(shnd, sample_rate, chans-1);
	loop_thread = thd_create(0, play_loop, NULL);
}

static void stop_audio() {
	aud_set = 0;
    thd_join(loop_thread, NULL);
    snd_stream_stop(shnd);
}


int aica_audio_open(int freq, int channels, uint32_t size) {
  
  
	if (audio_mut == NULL) {
		audio_mut = mutex_create();
	}

	sample_rate = freq;
	chans = channels;

	tmpbuf = (uint8*) malloc(BUFFER_MAX_FILL);
	sndbuf = (uint8*) malloc(BUFFER_MAX_FILL);

	memset (tmpbuf, 0, BUFFER_MAX_FILL);
	memset (sndbuf, 0, BUFFER_MAX_FILL);

	sbsize = size;
	sndptr = last_read = snd_ct = 0;
	waiting_for_data = 1;

	shnd = snd_stream_alloc(aica_audio_callback, sbsize);
	
	if(shnd < 0) {
		ds_printf("DS_ERROR: Can't alloc stream\n");
		return -1;
	}
	
	snd_stream_prefill(shnd);
	snd_stream_start(shnd, freq, channels-1);
	return 0;
}

int aica_audio_write(char *buffer, int len) {
 
	/*If this stuff works, get rid of that bit left from the old output system. */
	if (len== -1) { 
		return 0;
	} 
	
retry:

	mutex_lock(audio_mut);	
		
	if (sndptr + len > BUFFER_MAX_FILL) {
		
		mutex_unlock(audio_mut);
		
		if(!aud_set) {
			start_audio();
		}
		
		thd_pass();
		goto retry;
	}

	memcpy (tmpbuf + sndptr, buffer, len);
	sndptr += len;
	mutex_unlock(audio_mut);
	
	if(!aud_set && sndptr >= sbsize) {
		start_audio();
	}
	return 0;
	//return len;
}

int aica_audio_close() {
	
	aud_set = sndptr = 0;
    thd_join(loop_thread, NULL);
    snd_stream_stop(shnd);
    snd_stream_destroy(shnd);
	
	if(tmpbuf)
		free(tmpbuf);
		
	if(sndbuf)
		free(sndbuf);
	
	return 0;
}


/*

static int spu_ch[2];
static uint32 spu_ram_inbuf;
static uint32 spu_ram_outbuf;
static uint32 spu_ram_malloc;



void snd_init_decoder(int buf_size) {
	spu_ram_inbuf = snd_mem_malloc(buf_size * 2);
	spu_ram_outbuf = snd_mem_malloc(64*1024);
	spu_ram_malloc = snd_mem_malloc(256*1024);

	// And channels
	spu_ch[0] = snd_sfx_chn_alloc();
	spu_ch[1] = snd_sfx_chn_alloc();
	
	AICA_CMDSTR_DECODER(tmp, cmd, dec);

	cmd->cmd = AICA_CMD_DECODER;
	cmd->timestamp = 0;
	cmd->size = AICA_CMDSTR_DECODER_SIZE;
	dec->cmd = AICA_DECODER_CMD_INIT;
	dec->base = spu_ram_malloc;
	dec->out = spu_ram_outbuf;
	snd_sh4_to_aica(tmp, cmd->size);
}


void snd_decode(uint8 *data, int size, uint32 codec) {
	AICA_CMDSTR_DECODER(tmp, cmd, dec);
	
	spu_dma_transfer((void *)(((uint32)data + 31) & (~31)), spu_ram_inbuf, size, -1, NULL, 0);

	cmd->cmd = AICA_CMD_DECODER;
	cmd->timestamp = 0;
	cmd->size = AICA_CMDSTR_DECODER_SIZE;
	dec->cmd = AICA_DECODER_CMD_DECODE;
	dec->codec = codec;
	dec->base = spu_ram_inbuf;
	dec->out = spu_ram_outbuf;
	dec->length = size;
	dec->chan[0] = spu_ch[0];
	dec->chan[1] = spu_ch[1];
	snd_sh4_to_aica(tmp, cmd->size);
}


void snd_cpu_clock(uint32 clock) {
	return;
	aica_cmd_t cmd; 
	cmd.cmd = AICA_CMD_CPU_CLOCK;
	cmd.cmd_id = clock;
	cmd.timestamp = 0;
	snd_sh4_to_aica(cmd, 512);
}*/
#endif

static int codecs_inited = 0;
extern AVCodec mp1_decoder;
extern AVCodec mp2_decoder;
extern AVCodec mp3_decoder;
extern AVCodec vorbis_decoder;
//extern AVCodec mpeg4_decoder;




static AVCodecContext *findDecoder(AVFormatContext *format, int type, int *index) {
	
	AVCodec *codec = NULL;
	AVCodecContext *codec_ctx = NULL;
	int i, r;
	char errbuf[256];
	int good_audio = -1;
	
	for(i = 0; i < format->nb_streams; i++) {
		if(format->streams[i]->codec->codec_type == type) {
			
			format->streams[i]->discard = AVDISCARD_ALL;
			codec_ctx = format->streams[i]->codec;
			
			if(type == AVMEDIA_TYPE_AUDIO && codec_ctx->codec_type == type && codec_ctx->codec_id == CODEC_ID_MP3) {
				good_audio = i;
			}
		}
	}
	
	codec_ctx = NULL;
	
	
	for(i = 0; i < format->nb_streams; i++) {
		
		if(format->streams[i]->codec->codec_type == type) {
			
			if(good_audio > -1 && good_audio != i) {
				continue;
			}

			codec_ctx = format->streams[i]->codec;
			
			// Find the decoder for the audio stream
			codec = avcodec_find_decoder(codec_ctx->codec_id);
			
			if(codec == NULL) {
				format->streams[i]->discard = AVDISCARD_ALL;
				codec_ctx = NULL;
				continue;
			} 
			
			ds_printf("DS_FFMPEG: Codec %s\n", codec->long_name);
			
			// Open codec
			if((r = avcodec_open(codec_ctx, codec)) < 0) {
				av_strerror(r, errbuf, 256);
				ds_printf("DS_ERROR_FFMPEG: %s\n", errbuf);
				continue;
			}
			
			format->streams[i]->discard = AVDISCARD_DEFAULT;
			*index = i;
			return codec_ctx;
		}
	}
	
	return NULL;
}




int ffplay(const char *filename, const char *force_format) {

	char errbuf[256];
	int r = 0;
	
	int frameFinished;
	AVPacket packet;
	int audio_buf_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	int16_t *audio_buf = (int16_t *) malloc((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
	
	if(!audio_buf) {
		ds_printf("DS_ERROR: No free memory\n");
		return -1;
	}
	
	memset(audio_buf, 0, (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
	
	AVFormatContext *pFormatCtx = NULL;
	AVFrame *pFrame = NULL;
	AVCodecContext *pVideoCodecCtx = NULL, *pAudioCodecCtx = NULL;
	AVInputFormat *file_iformat = NULL;
	
	video_txr_t movie_txr;
	int videoStream = -1, audioStream = -1;
	
	maple_device_t *cont = NULL;
	cont_state_t *state = NULL;
	int pause = 0, done = 0;
	
	char fn[MAX_FN_LEN];
	sprintf(fn, "ds:%s", filename);

	memset(&movie_txr, 0, sizeof(movie_txr));
	
	if(!codecs_inited) {
		avcodec_register_all();
		avcodec_register(&mp1_decoder);
		avcodec_register(&mp2_decoder);
		avcodec_register(&mp3_decoder);
		avcodec_register(&vorbis_decoder);
		//avcodec_register(&mpeg4_decoder);
		codecs_inited = 1;
	}
	
	if(force_format)
      file_iformat = av_find_input_format(force_format);
    else
      file_iformat = NULL;


	// Open video file
	ds_printf("DS_PROCESS_FFMPEG: Opening file: %s\n", filename);
	if((r = av_open_input_file((AVFormatContext**)(&pFormatCtx), fn, file_iformat, /*FFM_PACKET_SIZE*/0, NULL)) != 0) {
		av_strerror(r, errbuf, 256);
		ds_printf("DS_ERROR_FFMPEG: %s\n", errbuf);
		free(audio_buf);
		return -1; // Couldn't open file
	}
	
	// Retrieve stream information
	ds_printf("DS_PROCESS_FFMPEG: Retrieve stream information...\n");
	if((r = av_find_stream_info(pFormatCtx)) < 0) {
		av_strerror(r, errbuf, 256);
		ds_printf("DS_ERROR_FFMPEG: %s\n", errbuf);
		av_close_input_file(pFormatCtx);
		free(audio_buf);
		return -1; // Couldn't find stream information
	}

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, filename, 0);
	//thd_sleep(5000);
	
	pVideoCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_VIDEO, &videoStream);
	pAudioCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_AUDIO, &audioStream);
	
	//LockInput();
	
	if(pVideoCodecCtx) {
		
		//LockVideo();
		ShutdownVideoThread();
		SDL_DS_FreeScreenTexture(0);
		int format = 0;
		
		switch(pVideoCodecCtx->pix_fmt) {
			case PIX_FMT_YUV420P:
			case PIX_FMT_YUVJ420P:
			
				format = PVR_TXRFMT_YUV422;
#ifdef USE_HW_YUV				
				yuv_conv_init();
#endif
				break;
				
			case PIX_FMT_UYVY422:
			case PIX_FMT_YUVJ422P:
			
				format = PVR_TXRFMT_YUV422;
				break;
				
			default:
				format = PVR_TXRFMT_RGB565;
				break;
		}
		
		MakeVideoTexture(&movie_txr, pVideoCodecCtx->width, pVideoCodecCtx->height, format | PVR_TXRFMT_NONTWIDDLED, PVR_FILTER_BILINEAR);
		
#ifdef USE_HW_YUV				
		yuv_conv_setup(movie_txr.addr, PVR_YUV_MODE_MULTI, PVR_YUV_FORMAT_YUV420, movie_txr.width, movie_txr.height);
		pvr_dma_init();
#endif

	} else {
		ds_printf("DS_ERROR: Didn't find a video stream.\n");
	}
	
	
	if(pAudioCodecCtx) {
		
#ifdef USE_DIRECT_AUDIO
		audioinit(pAudioCodecCtx);
#else

		sprintf(fn, "%s/firmware/aica/ds_stream.drv", getenv("PATH"));
		
		if(snd_init_fw(fn) < 0) {
			goto exit_free;
		}
	
		if(aica_audio_open(pAudioCodecCtx->sample_rate, pAudioCodecCtx->channels, 8192) < 0) {
			goto exit_free;
		}
		//snd_cpu_clock(0x19);
		//snd_init_decoder(8192);
#endif
		
	} else {
		ds_printf("DS_ERROR: Didn't find a audio stream.\n");
	}
	
	//ds_printf("FORMAT: %d\n", pVideoCodecCtx->pix_fmt);

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	if(pFrame == NULL) {
		ds_printf("DS_ERROR: Can't alloc memory\n");
		goto exit_free;
	}
	
	int pressed = 0, framecnt = 0;
	uint32 fa = 0;
	
	fa = GET_EXPORT_ADDR("ffplay_format_handler");
	
	if(fa > 0 && fa != 0xffffffff) {
		EXPT_GUARD_BEGIN;
			void (*ff_format_func)(AVFormatContext *, AVCodecContext *, AVCodecContext *) = 
				(void (*)(AVFormatContext *, AVCodecContext *, AVCodecContext *))fa;
			ff_format_func(pFormatCtx, pVideoCodecCtx, pAudioCodecCtx);
		EXPT_GUARD_CATCH;
		EXPT_GUARD_END;
	}
	
	fa = GET_EXPORT_ADDR("ffplay_frame_handler");
	void (*ff_frame_func)(AVFrame *) = NULL;
	
	if(fa > 0 && fa != 0xffffffff) {
		EXPT_GUARD_BEGIN;
			ff_frame_func = (void (*)(AVFrame *))fa;
			// Test call
			ff_frame_func(NULL);
		EXPT_GUARD_CATCH;
			ff_frame_func = NULL;
		EXPT_GUARD_END;
	}
	
	fa = GET_EXPORT_ADDR("ffplay_render_handler");
	
	if(fa > 0 && fa != 0xffffffff) {
		EXPT_GUARD_BEGIN;
			movie_txr.render_cb = (void (*)(void *))fa;
			// Test call
			movie_txr.render_cb(NULL);
		EXPT_GUARD_CATCH;
			movie_txr.render_cb = NULL;
		EXPT_GUARD_END;
	}
	
	while(av_read_frame(pFormatCtx, &packet) >= 0 && !done) {
		
		do {
			if(ff_frame_func) 
				ff_frame_func(pFrame);
					
			cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
			framecnt++;

			if(cont) {
				state = (cont_state_t *)maple_dev_status(cont);
				
				if (!state) {
					break;
				}
				if (state->buttons & CONT_START || state->buttons & CONT_B) {
					av_free_packet(&packet);
					done = 1;
				}
				if (state->buttons & CONT_A) {
					if((framecnt - pressed) > 10) {
						pause = pause ? 0 : 1;
						if(pause) {
#ifdef USE_DIRECT_AUDIO
							audio_end();
#else
							stop_audio();
#endif
						} else {
#ifndef USE_DIRECT_AUDIO
							start_audio();
#endif
						}
					}
					pressed = framecnt;
				}
				
				if(state->buttons & CONT_DPAD_LEFT) {
					//av_seek_frame(pFormatCtx, -1, timestamp * ( AV_TIME_BASE / 1000 ), AVSEEK_FLAG_BACKWARD);
				}
				
				if(state->buttons & CONT_DPAD_RIGHT) {
					//av_seek_frame(pFormatCtx, -1, timestamp * ( AV_TIME_BASE / 1000 ), AVSEEK_FLAG_BACKWARD);
				}
			}
			
			if(pause) thd_sleep(100);
			
		} while(pause);
		
		//printf("Packet: size: %d data: %02x%02x%02x pst: %d\n", packet.size, packet.data[0], packet.data[1], packet.data[2], pFrame->pts);
		
		// Is this a packet from the video stream?
		if(packet.stream_index == videoStream) {
			//printf("video\n");
			// Decode video frame
			if((r = avcodec_decode_video2(pVideoCodecCtx, pFrame, &frameFinished, &packet)) < 0) {
				//av_strerror(r, errbuf, 256);
				//printf("DS_ERROR_FFMPEG: %s\n", errbuf);
			} else {
				
				// Did we get a video frame?
				if(frameFinished && !pVideoCodecCtx->hurry_up) {
					RenderVideo(&movie_txr, pFrame, pVideoCodecCtx);
				}
			}

		} else if(packet.stream_index == audioStream) {
			//printf("audio\n");
			//snd_decode((uint8*)audio_buf, audio_buf_size, AICA_CODEC_MP3);
			
			audio_buf_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
			if((r = avcodec_decode_audio3(pAudioCodecCtx, audio_buf, &audio_buf_size, &packet)) < 0) {
				//av_strerror(r, errbuf, 256);
				//printf("DS_ERROR_FFMPEG: %s\n", errbuf);
				//continue;
			} else {
				
				if(audio_buf_size > 0 && !pAudioCodecCtx->hurry_up) {

#ifdef USE_DIRECT_AUDIO
					audio_write(pAudioCodecCtx, audio_buf, audio_buf_size);
#else
					aica_audio_write((char*)audio_buf, audio_buf_size);
#endif
				}
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	
	goto exit_free;
	
exit_free:

	if(pFrame)
		av_free(pFrame);
	
	if(pFormatCtx)
		av_close_input_file(pFormatCtx);
	
	if(audioStream > -1) {
		if(pAudioCodecCtx)
			avcodec_close(pAudioCodecCtx);
#ifdef USE_DIRECT_AUDIO
		audio_end();
#else
		aica_audio_close();
		sprintf(fn, "%s/firmware/aica/kos_stream.drv", getenv("PATH"));
		snd_init_fw(fn);
#endif
	}
	
	if(audio_buf) {
		free(audio_buf);
	}
	
	if(videoStream > -1) {
		if(pVideoCodecCtx)
			avcodec_close(pVideoCodecCtx);
		FreeVideoTexture(&movie_txr);
		SDL_DS_AllocScreenTexture(GetScreen());
		InitVideoThread();
		//UnlockVideo();
	}
	
	//UnlockInput();
	ProcessVideoEventsUpdate(NULL);
	return 0;
}
