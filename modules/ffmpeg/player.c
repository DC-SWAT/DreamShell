/* DreamShell ##version##

   player.c - ffmpeg player
   Copyright (C)2011-2014, 2024 SWAT
*/

#include "ds.h"
#include <dc/sound/sound.h>
#include "settings.h"

#include "aica.h"
#include "drivers/aica_cmd_iface.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"


typedef struct video_txr {
	pvr_poly_hdr_t hdr;
	int tw, th;
	int width, height;
	void (*render_cb)(void *);
	pvr_ptr_t addr;
	void *backbuf;
} video_txr_t;

/* TODO: Hardware YUV converter */
static void yuvtex(uint16 *tbuf, int tstride, unsigned width, unsigned height,
	uint8 *ybuf, int ystride, uint8 *ubuf, int ustride, uint8 *vbuf, int vstride) {
	int h = height / 2;
	uint8 u, v;

	do {
		uint8 *uptr, *vptr, *yptr, *yptr2;
		uint8 *tex, *tex2;
		int w = width / 2;

		tex  = (uint8*)tbuf; tbuf += tstride;
		tex2 = (uint8*)tbuf; tbuf += tstride;
		yptr  = ybuf; ybuf += ystride;
		yptr2 = ybuf; ybuf += ystride;
		uptr = ubuf; ubuf += ustride;
		vptr = vbuf; vbuf += vstride;
		do {
			u = *uptr++;
			v = *vptr++;
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
		} while(--w);
	} while(--h);
}


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
			yuvtex(txr->backbuf, txr->tw, codec->width, codec->height,
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]
			);
			pvr_txr_load(txr->backbuf, txr->addr, txr->tw * codec->height * 2);
			break;
		case PIX_FMT_UYVY422:
		default:
			pvr_txr_load(frame->data[0], txr->addr, txr->tw * codec->height * 2);
			break;
	}

	pvr_wait_ready();
	pvr_scene_begin();

	pvr_list_begin(PVR_LIST_OP_POLY);

	if (txr->addr) {
		int dispw = 640;
		int disph = 640 * txr->height / txr->width;
		RenderVideoTexture(txr, (640 - dispw) / 2, (480 - disph) / 2, dispw, disph, 0xFFFFFFFF);
	}

	pvr_list_finish();

	if(txr->render_cb != NULL) {
		txr->render_cb((void *)txr);
	}

	pvr_scene_finish();
}


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

/* sound */
static void audioinit(AVCodecContext *audio) {
	// printf("bitrate=%d sample_rate=%d channels=%d framesize=%d bits=%d\n",
	// 	audio->bit_rate,audio->sample_rate,audio->channels,audio->frame_size,audio->frame_bits
	// );

	aud->rate = audio->sample_rate;
	aud->channels = audio->channels;
	aud->bits = audio->frame_bits;
	if (aud->bits == 0) aud->bits = 16;
	aud->bytes = aud->bits / 8;
}

static void audio_end() {
	aud->playing = 0;
	aica_stop(0);
	aica_stop(1);
}

static void audio_write(AVCodecContext *audio, void *buf, size_t size) {
	
	if(!aud->channels) {
		return;
	}
	
	int nsamples = size / (aud->channels * aud->bytes);
	int writepos = 0;
	int curpos = 0;

	if (!aud->playing) {
		aud->sampsize = (65535) / nsamples * nsamples;
		aud->leftbuf  = AICA_MEM_SIZE - 0x40000;
		aud->rightbuf = aud->leftbuf + aud->sampsize * aud->bytes;
		aud->writepos = 0;
	} else {

		writepos = aud->writepos;
		curpos = aica_get_pos(0);

		do {
			curpos = aica_get_pos(0);
		} while(writepos < curpos && curpos < writepos + nsamples);
	}

	writepos = aud->writepos * aud->bytes;

	if (aud->channels == 1) {
		spu_memload_sq(aud->leftbuf + writepos, buf, size);
	} else {
		snd_pcm16_split_sq(buf, aud->leftbuf + writepos, aud->rightbuf + writepos, size);
	}

	aud->writepos += nsamples;
	
	if (aud->writepos >= aud->sampsize) {
		aud->writepos = 0;
	}

	if (!aud->playing) {
		int mode;
		int vol = GetVolumeFromSettings();

		if(vol < 0) {
			vol = 240;
		}

		aud->playing = 1;

		switch(aud->bits) {
			case 4:
				mode = SM_ADPCM_LS;
				break;
			case 8:
				mode = SM_8BIT;
				break;
			case 16:
			default:
				mode = SM_16BIT;
				break;
		}

		if (aud->channels == 1) {
			aica_play(0, mode, aud->leftbuf, 0, aud->sampsize, aud->rate, vol, 128, 1);
		} else {
			aica_play(0, mode, aud->leftbuf, 0, aud->sampsize, aud->rate, vol, 0, 1);
			aica_play(1, mode, aud->rightbuf, 0, aud->sampsize, aud->rate, vol, 255 , 1);
		}
	}
}


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
	int16_t *audio_buf = (int16_t *) memalign(32, (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2);
	
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
	
	char fn[NAME_MAX];
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
	
	pVideoCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_VIDEO, &videoStream);
	pAudioCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_AUDIO, &audioStream);
	
	if(pVideoCodecCtx) {

		ShutdownVideoThread();
		int format = 0;

		switch(pVideoCodecCtx->pix_fmt) {
			case PIX_FMT_YUV420P:
			case PIX_FMT_YUVJ420P:
			case PIX_FMT_UYVY422:
			case PIX_FMT_YUVJ422P:
				format = PVR_TXRFMT_YUV422;
				break;
			default:
				format = PVR_TXRFMT_RGB565;
				break;
		}

		MakeVideoTexture(&movie_txr, pVideoCodecCtx->width, pVideoCodecCtx->height, format | PVR_TXRFMT_NONTWIDDLED, PVR_FILTER_BILINEAR);

	} else {
		ds_printf("DS_ERROR: Didn't find a video stream.\n");
	}

	if(pAudioCodecCtx) {
		audioinit(pAudioCodecCtx);
		
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
				if (state->buttons & (CONT_B | CONT_Y)) {
					av_free_packet(&packet);
					done = 1;
				}
				if (state->buttons & (CONT_A | CONT_X)) {
					if((framecnt - pressed) > 10) {
						pause = !pause;
						if(pause) {
							audio_end();
						}
					}
					pressed = framecnt;
				}
#if 0		
				if(state->buttons & CONT_DPAD_LEFT) {
					av_seek_frame(pFormatCtx, -1, timestamp * ( AV_TIME_BASE / 1000 ), AVSEEK_FLAG_BACKWARD);
				}
				
				if(state->buttons & CONT_DPAD_RIGHT) {
					av_seek_frame(pFormatCtx, -1, timestamp * ( AV_TIME_BASE / 1000 ), AVSEEK_FLAG_BACKWARD);
				}
#endif
			}
			
			if(pause) {
				thd_sleep(100);
			}
			
		} while(pause);
		
		// printf("Packet: size: %d data: %02x%02x%02x pst: %d\n",
		// 	packet.size, packet.data[0], packet.data[1], packet.data[2], pFrame->pts);
		
		// Is this a packet from the video stream?
		if(packet.stream_index == videoStream) {

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
			
			audio_buf_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
			if((r = avcodec_decode_audio3(pAudioCodecCtx, audio_buf, &audio_buf_size, &packet)) < 0) {
				//av_strerror(r, errbuf, 256);
				//printf("DS_ERROR_FFMPEG: %s\n", errbuf);
				//continue;
			} else {
				if(audio_buf_size > 0 && !pAudioCodecCtx->hurry_up) {
					audio_write(pAudioCodecCtx, audio_buf, audio_buf_size);
				}
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

exit_free:

	if(pFrame) {
		av_free(pFrame);
	}
	if(pFormatCtx) {
		av_close_input_file(pFormatCtx);
	}
	if(audioStream > -1) {
		if(pAudioCodecCtx) {
			avcodec_close(pAudioCodecCtx);
		}
		audio_end();
	}
	if(audio_buf) {
		free(audio_buf);
	}
	if(videoStream > -1) {
		if(pVideoCodecCtx) {
			avcodec_close(pVideoCodecCtx);
		}
		FreeVideoTexture(&movie_txr);
		InitVideoThread();
	}
	return 0;
}
