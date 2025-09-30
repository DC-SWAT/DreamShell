/**
 * @file pcm.c
 * FFmpeg PCM passthrough codecs
 * @author SWAT
 */

#include "utils.h"
#include "libavcodec/avcodec.h"

/* PCM S16LE */
static int pcm_decode_init(AVCodecContext * avctx) {
    avctx->bits_per_coded_sample = 16;
    avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}

static int pcm_decode_frame(AVCodecContext * avctx, void *data, int *data_size, AVPacket *avpkt) {
    if (avpkt->size == 0) {
        *data_size = 0;
        return 0;
    }

    int size = avpkt->size;

    if(avctx->block_align > 1 && (size % avctx->block_align) != 0) {
        size -= (size % avctx->block_align);
    }

    memcpy_sh4(data, avpkt->data, size);
    *data_size = size;
    return avpkt->size;
}

static int pcm_decode_close(AVCodecContext *avctx) {
    return 0;
}

AVCodec pcm_s16le_decoder = {
    "pcm_s16le",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_PCM_S16LE,
    0,
    pcm_decode_init,
    NULL,
    pcm_decode_close,
    pcm_decode_frame,
    CODEC_CAP_DR1,
    NULL,
    NULL,
    NULL,
    NULL,
    "PCM signed 16-bit little-endian passthrough"
};

/* ADPCM Yamaha */
static int adpcm_decode_init(AVCodecContext * avctx) {
    avctx->bits_per_coded_sample = 4;
    avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}

AVCodec yamaha_adpcm_decoder = {
    "adpcm_yamaha",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_ADPCM_YAMAHA,
    0,
    adpcm_decode_init,
    NULL,
    pcm_decode_close,
    pcm_decode_frame,
    CODEC_CAP_DR1,
    NULL,
    NULL,
    NULL,
    NULL,
    "ADPCM Yamaha passthrough"
};
