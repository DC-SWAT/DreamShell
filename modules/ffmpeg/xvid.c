
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "xvid.h"

typedef struct context {
  xvid_dec_create_t dec_param;
  xvid_dec_frame_t frame;
  xvid_dec_stats_t stats;
  xvid_gbl_init_t param;
} context_t;

static int decode_init(AVCodecContext * avctx)
{
  context_t * ctx = (context_t *) avctx->priv_data;
  int w, h;

  ctx->param.version = XVID_VERSION;
  ctx->param.cpu_flags = 0;
  ctx->param.debug = XVID_DEBUG_DEBUG;


  if(xvid_global(NULL, XVID_GBL_INIT, &ctx->param, NULL)) {
	printf("xvid_decode_init: error\n");
  	return -1;	  
  }

  ctx->dec_param.version = XVID_VERSION;
  w = ctx->dec_param.width = avctx->width;
  h = ctx->dec_param.height = avctx->height;
  
  if (xvid_decore(NULL, XVID_DEC_CREATE, &ctx->dec_param, NULL))
    return -1;

/*   w = avctx->width = ctx->dec_param.width; */
/*   h = avctx->height = ctx->dec_param.height; */
  printf("xvid_decode_init : %dx%d\n", avctx->width, avctx->height);

  ctx->frame.version = XVID_VERSION;
  ctx->frame.general = XVID_LOWDELAY; /* ? */

  ctx->stats.version = XVID_VERSION;

  ctx->frame.output.csp = XVID_CSP_INTERNAL;

/*   ctx->frame.output.csp = XVID_CSP_I420; */
/*   ctx->frame.output.plane[0] = malloc(w*h); */
/*   ctx->frame.output.stride[0] = w; */
/*   ctx->frame.output.plane[1] = malloc(w*h/4); */
/*   ctx->frame.output.stride[1] = w/4; */
/*   ctx->frame.output.plane[2] = malloc(w*h/4); */
/*   ctx->frame.output.stride[2] = w/4; */
/*   ctx->frame.output.plane[2] = NULL; */
/*   ctx->frame.output.stride[2] = 0; */

  return 0;
}

static int decode_frame(AVCodecContext * avctx,
			void *data, int *data_size,
			uint8_t * buf, int buf_size)
{
  context_t * ctx = (context_t *) avctx->priv_data;
  AVFrame * frame = (AVFrame *) data;
  int res;
  int i;

  ctx->frame.bitstream = buf;
  ctx->frame.length = buf_size;

  if (avctx->width)
    res = xvid_decore(ctx->dec_param.handle, XVID_DEC_DECODE, 
		      &ctx->frame, NULL);
  else {
    res = xvid_decore(ctx->dec_param.handle, XVID_DEC_DECODE, 
		      &ctx->frame, &ctx->stats);

    if (ctx->stats.type == XVID_TYPE_VOL) {
      printf("xvid_decode_frame: STATE %dx%d\n", 
	     ctx->stats.data.vol.width, ctx->stats.data.vol.height);
      avctx->width = ctx->stats.data.vol.width;
      avctx->height = ctx->stats.data.vol.height;
    }
  }

  if (res <= 0) {
    printf("xvid_decode_frame ERROR = %d\n", res);
    *data_size = 0;
    return res;
  }

/*   if (res != buf_size) */
/*     printf("res = %d (%d)\n", res, buf_size); */

  //printf("%x %d\n", ctx->frame.output.plane[0], ctx->frame.output.stride[0]);
  for (i=0; i<4; i++) {
    frame->data[i] = ctx->frame.output.plane[i];
    frame->linesize[i] = ctx->frame.output.stride[i];
  }

  *data_size = 1;

  return res;
}

static int decode_end(AVCodecContext * avctx)
{
  context_t * ctx = (context_t *) avctx->priv_data;

  xvid_decore(ctx->dec_param.handle, XVID_DEC_DESTROY, NULL, NULL);
/*   int i; */
/*   for (i=0; i<4; i++) { */
/*     if (ctx->frame.output.plane[i]) { */
/*       free(ctx->frame.output.plane[i]); */
/*       ctx->frame.output.plane[i] = NULL; */
/*     } */
/*   } */

  return 0;
}

	
AVCodec mpeg4_decoder =
{
    "mpeg4",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MPEG4,
    sizeof(context_t),
    decode_init,
    NULL,
    decode_end,
    decode_frame,
    (CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | 0),
    .flush= NULL,//flush,
    .long_name= "Xvid video",
};
