/*
 * @author bero <bero@geocities.co.jp>
*/

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#define NOANALYSIS 1

#include "mpglib/mpg123.h"
#include "mpglib/mpglib.h"

typedef struct MPADecodeContext {
	struct mpstr_tag mp;
	int header_parsed;
} MPADecodeContext;

static int decode_init(AVCodecContext * avctx) {
    MPADecodeContext *s = avctx->priv_data;
	InitMP3(&s->mp);
    return 0;
}

static int decode_frame(AVCodecContext * avctx, void *data, int *data_size, AVPacket *avpkt) {
	
    MPADecodeContext *s = avctx->priv_data;
	int ret;
	int outsize;
	char *outp = data;
	*data_size = 0;
	ret = decodeMP3(&s->mp, avpkt->data, avpkt->size, outp, 65536, &outsize);
	
	while(ret == MP3_OK) {
		if (!s->header_parsed) {
			struct frame *fr = &s->mp.fr;
			extern int tabsel_123[2][3][16];
			extern long freqs[9];
			s->header_parsed = 1;
			avctx->channels = fr->stereo;
			avctx->sample_rate = freqs[fr->sampling_frequency];
			avctx->bit_rate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]*1000;
			//printf("channels=%d,rate=%d\n", avctx->channels, avctx->sample_rate);
		}
//		printf("%d \n",outsize);
		outp += outsize;
		*data_size += outsize;
		ret = decodeMP3(&s->mp, NULL, 0, outp, 65536, &outsize);
	}
	//printf("datasize:%d \n",*data_size);
	return avpkt->size;
}

static int decode_close(AVCodecContext *avctx) {
    MPADecodeContext *s = avctx->priv_data;
    ExitMP3(&s->mp);
	return 0;
}

/*
AVCodec mpglib_mp2_decoder =
{
    "mp2",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MP2,
    sizeof(MPADecodeContext),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
};

AVCodec mpglib_mp3_decoder =
{
    "mp3",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MP3,
    sizeof(MPADecodeContext),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
};*/

AVCodec mp1_decoder =
{
    "mp1",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP1,
    sizeof(MPADecodeContext),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
    CODEC_CAP_PARSE_ONLY,
    .flush= NULL,//flush,
    .long_name= "MP1 (MPEG audio layer 1)",
};


AVCodec mp2_decoder =
{
    "mp2",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP2,
    sizeof(MPADecodeContext),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
    CODEC_CAP_PARSE_ONLY,
    .flush= NULL,//flush,
    .long_name= "MP2 (MPEG audio layer 2)",
};


AVCodec mp3_decoder =
{
    "mp3",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP3,
    sizeof(MPADecodeContext),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
    CODEC_CAP_PARSE_ONLY,
    .flush= NULL,//flush,
    .long_name= "MP3 (MPEG audio layer 3)",
};

