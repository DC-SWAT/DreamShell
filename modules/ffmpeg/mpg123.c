/**
 * @file mpg123.c
 * Mpeg audio 1,2,3 codec support via libmpg123.
 * @author SWAT
 */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "mpg123/config.h"
#include "mpg123/compat.h"
#include "mpg123/mpg123.h"

typedef struct MPADecodeContext {
	mpg123_handle *mh;
} MPADecodeContext;

static int decode_init(AVCodecContext * avctx) {
    MPADecodeContext *s = avctx->priv_data;
	int err = 0;
	
	mpg123_init();
	
    s->mh = mpg123_new(NULL, &err);

	if(s->mh == NULL) {
		printf("Can't init mpg123: %s\n", mpg123_strerror(s->mh));
		return -1;
	}
	
	//mpg123_param(s->mh, MPG123_VERBOSE, 2, 0);
   
    /* Open the MP3 context */
    err = mpg123_open_feed(s->mh);
 
	if(err != MPG123_OK) {
		printf("Can't open mpg123\n");
		return -1;
	}
	
    return 0;
}

static int decode_frame(AVCodecContext * avctx, void *data, int *data_size, AVPacket *avpkt) {
	
    MPADecodeContext *s = avctx->priv_data;
	int ret;
	size_t outsize = 0;
	uint8 *outp = data;

	ret = mpg123_decode(s->mh, (uint8*)avpkt->data, avpkt->size, outp, 65536, &outsize);
	*data_size = outsize;
	
    if(ret == MPG123_NEW_FORMAT) {
		
		//printf("MPG123 new format from ffmpeg\n");
		
		long rate;
		int channels, enc;
		struct mpg123_frameinfo decinfo;
		
		mpg123_getformat(s->mh, &rate, &channels, &enc);
		//mpg123_feed(s->mh, (uint8*)avpkt->data, avpkt->size);
		mpg123_info(s->mh, &decinfo);
		
		avctx->channels = channels;
		avctx->sample_rate = rate;
		avctx->bit_rate = decinfo.bitrate;
		avctx->frame_size = decinfo.framesize;
		//avctx->frame_bits = 16;
		avctx->sample_fmt = SAMPLE_FMT_U8;
		
		//printf("Output Sampling rate = %ld\r\n", decinfo.rate);
		//printf("Output Bitrate       = %d\r\n", decinfo.bitrate);
		//printf("Output Frame size    = %d\r\n", decinfo.framesize);
		
		//return avpkt->size;
	}
	
	if(ret == MPG123_ERR) {
		//printf("mpg123: %s\n", mpg123_plain_strerror(ret));
		return -1;
	}
	
	//printf("mpg123: %s\n", mpg123_plain_strerror(ret));
	
	while(ret != MPG123_ERR && ret != MPG123_NEED_MORE) {
			
		/* Get all decoded audio that is available now before feeding more input. */
		ret = mpg123_decode(s->mh, NULL, 0, outp, 65536, &outsize);
		//printf("mpg123: %s\n", mpg123_plain_strerror(ret));
		
		if(ret != MPG123_ERR && ret != MPG123_NEED_MORE) {
			*data_size += outsize;
			outp += outsize;
		}
	}
	
	return avpkt->size;
}

static int decode_close(AVCodecContext *avctx) {
	
	MPADecodeContext *s = avctx->priv_data;
	
	if(s->mh != NULL) {
		mpg123_close(s->mh);
	}

	mpg123_exit();
	return 0;
}


AVCodec mp1_decoder = {
	"mp1",
	AVMEDIA_TYPE_AUDIO,
	CODEC_ID_MP1,
	sizeof(MPADecodeContext),
	decode_init,
	NULL,
	decode_close,
	decode_frame,
	CODEC_CAP_PARSE_ONLY,
	NULL,
	NULL,
	NULL,
	NULL,
	"MP1 (MPEG audio layer 1)"
};

AVCodec mp2_decoder = {
	"mp2",
	AVMEDIA_TYPE_AUDIO,
	CODEC_ID_MP2,
	sizeof(MPADecodeContext),
	decode_init,
	NULL,
	decode_close,
	decode_frame,
	CODEC_CAP_PARSE_ONLY,
	NULL,
	NULL,
	NULL,
	NULL,
	"MP2 (MPEG audio layer 2)"
};

AVCodec mp3_decoder = {
	"mp3",
	AVMEDIA_TYPE_AUDIO,
	CODEC_ID_MP3,
	sizeof(MPADecodeContext),
	decode_init,
	NULL,
	decode_close,
	decode_frame,
	CODEC_CAP_PARSE_ONLY,
	NULL,
	NULL,
	NULL,
	NULL,
	"MP3 (MPEG audio layer 3)"
};
