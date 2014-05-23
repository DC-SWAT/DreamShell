/**
 * @file oggvorbis.c
 * Ogg Vorbis codec support via libvorbisenc.
 * @author Mark Hills and SWAT
 */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define OGGVORBIS_FRAME_SIZE 1024


typedef struct OggVorbisContext {
    vorbis_info vi;
    vorbis_dsp_state vd;
    vorbis_block vb;

    /* decoder */
    vorbis_comment vc;
} OggVorbisContext;


static int oggvorbis_decode_init(AVCodecContext *avccontext) {
	OggVorbisContext *context = avccontext->priv_data;

	vorbis_info_init(&context->vi);
	vorbis_comment_init(&context->vc);

	return 0;
}


static inline int conv(int samples, float **pcm, char *buf, int channels) {
	int i, j, val;
	ogg_int16_t *ptr, *data = (ogg_int16_t*)buf;
	float *mono;

	for(i = 0 ; i < channels ; i++){
		ptr = &data[i];
		mono = pcm[i];

		for(j = 0 ; j < samples ; j++) {

			val = mono[j] * 32767.f;

			if(val > 32767) val = 32767;
			if(val < -32768) val = -32768;

			*ptr = val;
			ptr += channels;
		}
	}

	return 0;
}

static int oggvorbis_decode_frame(AVCodecContext * avccontext, void *data, 
										int *data_size, AVPacket *avpkt) {

    OggVorbisContext *context = avccontext->priv_data;
    ogg_packet *op = (ogg_packet*)avpkt->data;
    float **pcm;
    int samples, total_samples, total_bytes;
 
    op->packet = (uint8*)op + sizeof(ogg_packet) ; /* correct data pointer */

    if(op->packetno < 3) {
		vorbis_synthesis_headerin(&context->vi, &context->vc, op);
		return avpkt->size;
    }

    if(op->packetno == 3) {
//		printf("vorbis_decode: %d channel, %ldHz, encoder `%s'\n",
//			context->vi.channels, context->vi.rate, context->vc.vendor);

		avccontext->channels = context->vi.channels;
		avccontext->sample_rate = context->vi.rate;
		
		vorbis_synthesis_init(&context->vd, &context->vi);
		vorbis_block_init(&context->vd, &context->vb);
    }

    if(vorbis_synthesis(&context->vb, op) == 0)
		vorbis_synthesis_blockin(&context->vd, &context->vb);
    
    total_samples = 0;
    total_bytes = 0;

    while((samples = vorbis_synthesis_pcmout(&context->vd, &pcm)) > 0) {
		conv(samples, pcm, (char*)data + total_bytes, context->vi.channels);
		total_bytes += samples * 2 * context->vi.channels;
		total_samples += samples;
		vorbis_synthesis_read(&context->vd, samples);
    }

    *data_size = total_bytes;   
    return avpkt->size;
}


static int oggvorbis_decode_close(AVCodecContext *avccontext) {
    OggVorbisContext *context = avccontext->priv_data;
   
    vorbis_info_clear(&context->vi);
    vorbis_comment_clear(&context->vc);

    return 0;
}


AVCodec vorbis_decoder = {
	"vorbis",
	AVMEDIA_TYPE_AUDIO,
	CODEC_ID_VORBIS,
	sizeof(OggVorbisContext),
	oggvorbis_decode_init,
	NULL,
	oggvorbis_decode_close,
	oggvorbis_decode_frame,
	CODEC_CAP_PARSE_ONLY,
	NULL,
	NULL,
	NULL,
	NULL,
	"Vorbis",
};

