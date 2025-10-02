/* DreamShell ##version##

   player.c - FFmpeg player for DreamShell

   Copyright (C) 2011-2025 Ruslan Rostovtsev (a.k.a. SWAT)
   http://www.dc-swat.ru

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following condition:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
*/

#include "ds.h"
#include "utils.h"
#include "sfx.h"
#include <dc/sound/sound.h>
#include <dc/spu.h>
#include <kos/mutex.h>
#include <kos/sem.h>
#include <errno.h>
#include "settings.h"

#include "drivers/aica_cmd_iface.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavutil/log.h"
#include "libavutil/mathematics.h"
#include "libavutil/rational.h"

#include <arch/timer.h>
#include <plx/texture.h>
#include <plx/context.h>
#include <plx/prim.h>
#include <dc/asic.h>
#include "ffmpeg.h"

#define PVR_YUV_FORMAT_YUV420 0
#define PVR_YUV_FORMAT_YUV422 1
#define PVR_YUV_MODE_SINGLE 0
#define PVR_YUV_MODE_MULTI 1

#define BYTE_SIZE_FOR_16x16_BLOCK_420 384
#define AUDIO_MIN_CHUNK_SIZE 1024
#define AUDIO_DECODE_BUFFER_GUARD (64 << 10)

typedef struct PacketBuffer {
    AVPacket pkt;
    uint8_t *buffer;
    int buffer_size;
} PacketBuffer_t;

typedef struct video_txr {
    int tw, th;
    int width, height;
    pvr_ptr_t addr;
    plx_texture_t *plx_txr;
} video_txr_t;

static struct {
    kthread_t *thread;
    AVFrame *frame[2];
    volatile int decode_frame_idx;

    AVCodecContext *codec;
    AVCodecContext *audio_codec;
    AVFormatContext *format_ctx;

    int video_stream;
    int audio_stream;

    Event_t *video_event;
    Event_t *input_event;

    volatile int playing;
    volatile int pause;
    volatile int done;
    volatile int seek_request;
    volatile int video_started;
    volatile int skip_to_keyframe;
    volatile int is_fullscreen;
    int back_to_window;
    int sdl_gui_managed;

    float frame_x, frame_y;
    float frame_x_old, frame_y_old;
    float disp_w, disp_h;
    float disp_x, disp_y;

    PacketBuffer_t vpackets[2];
    volatile int vpacket_idx;

    video_txr_t txr[2];
    volatile int txr_idx;

    semaphore_t video_packet_free;
    semaphore_t video_packet_ready;
    semaphore_t yuv_dma_done;
    uint8_t *yuv_dma_buffer;

    char filename[NAME_MAX];
    ffplay_params_t params;

    /* A/V sync */
    uint64_t start_time;
    uint64_t frame_timer;
    int64_t  frame_last_delay;
    int64_t video_current_pts;
    uint64_t video_current_pts_time;

    /* Stats */
    uint64_t fps_start_time;
    uint32_t frame_count_for_fps;
    float current_fps;
    float display_fps;
    int64_t av_diff;
    float display_av_diff;

    plx_font_t *stat_font;
    plx_fcxt_t *stat_font_cxt;
    float stat_font_base_size;

    int64_t last_callback_pts;

} vids = {0}, *vid = &vids;

static int ffplay_open_file(const char *filename, ffplay_params_t *params);
static void ffplay_close_file(void);
static AVCodecContext *findDecoder(AVFormatContext *format, int type, int *index);
static void audio_stop_playback(void);
static void load_stat_font();

static struct {
    int ch[2];
    uint32_t spu_ram_sch[2];
    size_t buffer_size;
    int type;
    int bitsize;
    int channels;
    int frequency;
    int vol;
    volatile int playing;
    uint32_t write_pos;
    uint64_t total_samples_written;

    AVRational time_base;
    int64_t clock_correction;
    uint8_t *temp_buf;
    size_t temp_buf_size;
    size_t temp_buf_rpos;
    size_t temp_buf_wpos;
    size_t buffer_size_samples;

    /* For frozen clock detection */
    int64_t last_clock_val;
    uint64_t last_clock_update_time;

} auds = {0}, *aud = &auds;

static uint32_t *sep_buffer[2] = {NULL, NULL};

static inline size_t bytes_to_samples(int bitsize, size_t bytes) {
    switch(bitsize) {
        case 4:
            return bytes << 1;
        case 8:
            return bytes;
        case 16:
        default:
            return bytes >> 1;
    }
}

static inline size_t samples_to_bytes(int bitsize, size_t samples) {
    switch(bitsize) {
        case 4:
            return samples >> 1;
        case 8:
            return samples;
        case 16:
        default:
            return samples << 1;
    }
}

static int64_t get_audio_clock() {
    if(!aud->playing) {
        return aud->clock_correction;
    }

    uint32_t play_pos = snd_get_pos(aud->ch[0]);
    uint32_t write_pos = aud->write_pos;
    size_t buffered_samples;

    if(write_pos >= play_pos) {
        buffered_samples = write_pos - play_pos;
    } else {
        buffered_samples = (aud->buffer_size_samples - play_pos) + write_pos;
    }

    uint64_t total_played_samples = (aud->total_samples_written > buffered_samples)
                                    ? aud->total_samples_written - buffered_samples
                                    : 0;

    return aud->clock_correction + (total_played_samples * 1000) / aud->frequency;
}

static int64_t get_video_clock() {
    if (vid->pause) {
        return vid->video_current_pts;
    }
    else {
        return vid->video_current_pts + (timer_ms_gettime64() - vid->video_current_pts_time);
    }
}

static int64_t get_master_clock() {
    if(!vid->playing) {
        return -1;
    }
    if(vid->audio_stream >= 0) {
        int64_t audio_clock = get_audio_clock();
        uint64_t now = timer_ms_gettime64();

        if(audio_clock < aud->last_clock_val) {
            return get_video_clock();
        }

        if(audio_clock != aud->last_clock_val) {
            aud->last_clock_val = audio_clock;
            aud->last_clock_update_time = now;
        }
        else if(now - aud->last_clock_update_time > 250) {
            return get_video_clock();
        }
        return audio_clock;
    }
    else if (vid->video_stream >= 0) {
        return get_video_clock();
    }
    return 0;
}

static int audio_dma_transfer(void *data, size_t size, int block) {
    int rs;
    size_t dma_size;
    uint32_t dest_addr, dma_offset;
    uint32_t *dma_buffer = (uint32_t *)data;

    dest_addr = aud->spu_ram_sch[0] + samples_to_bytes(aud->bitsize, aud->write_pos);
    dma_size = size / aud->channels;

    if(aud->channels == 2) {
        dma_buffer = (uint32_t *)sep_buffer[0];

        switch(aud->bitsize) {
            case 16:
                snd_pcm16_split((uint32_t *)data, sep_buffer[0], sep_buffer[1], size);
                break;
            case 8:
                snd_pcm8_split((uint32_t *)data, sep_buffer[0], sep_buffer[1], size);
                break;
            case 4:
                snd_adpcm_split((uint32_t *)data, sep_buffer[0], sep_buffer[1], size);
                break;
        }
    }
    dcache_purge_range((uintptr_t)dma_buffer, dma_size);

    do {
        rs = spu_dma_transfer(dma_buffer, dest_addr, dma_size, block, NULL, NULL);
        if(rs == 0) {
            break;
        }
        if(errno != EINPROGRESS) {
            return -1;
        }
        thd_sleep(1);
    } while(1);

    if(aud->channels == 2) {
        dma_offset = dest_addr - aud->spu_ram_sch[0];
        dest_addr = aud->spu_ram_sch[1] + dma_offset;
        dcache_purge_range((uintptr_t)sep_buffer[1], dma_size);

        do {
            rs = spu_dma_transfer(dma_buffer, dest_addr, dma_size, block, NULL, NULL);
            if(rs == 0) {
                break;
            }
            if(errno != EINPROGRESS) {
                return -1;
            }
            thd_sleep(1);
        } while(1);
    }
    return 0;
}

static void audio_end() {
    audio_stop_playback();

    if(aud->temp_buf) {
        free(aud->temp_buf);
        aud->temp_buf = NULL;
    }

    if(aud->spu_ram_sch[0]) {
        snd_mem_free(aud->spu_ram_sch[0]);
    }
    if(aud->ch[0] > -1) {
        snd_sfx_chn_free(aud->ch[0]);
    }
    if(aud->ch[1] > -1) {
        snd_sfx_chn_free(aud->ch[1]);
    }
    if(sep_buffer[0]) {
        free(sep_buffer[0]);
        sep_buffer[0] = NULL;
        sep_buffer[1] = NULL;
    }
}

static int audio_init(AVCodecContext *codec) {
    memset(aud, 0, sizeof(auds));
    aud->frequency = codec->sample_rate;
    aud->channels = codec->channels;
    aud->bitsize = codec->bits_per_coded_sample;
    if(aud->bitsize == 0) aud->bitsize = 16;
    aud->time_base = codec->time_base;

    switch(codec->codec_id) {
        case CODEC_ID_ADPCM_YAMAHA:
            aud->type = AICA_SM_ADPCM_LS;
            break;
        case CODEC_ID_PCM_U8:
        case CODEC_ID_PCM_S8:
            aud->type = AICA_SM_8BIT;
            break;
        default:
            aud->type = AICA_SM_16BIT;
            break;
    }

    aud->buffer_size = aud->type == AICA_SM_ADPCM_LS ? 32 << 10 : 128 << 10;
    aud->buffer_size_samples = bytes_to_samples(aud->bitsize, aud->buffer_size);
    aud->spu_ram_sch[0] = snd_mem_malloc(aud->buffer_size * aud->channels);

    if(!aud->spu_ram_sch[0]) {
        ds_printf("DS_ERROR: Can't allocate sound memory\n");
        return -1;
    }
    if (aud->channels == 2) {
        aud->spu_ram_sch[1] = aud->spu_ram_sch[0] + aud->buffer_size;
    }
    spu_memset_sq(aud->spu_ram_sch[0], 0, aud->buffer_size * aud->channels);

    aud->ch[0] = snd_sfx_chn_alloc();
    aud->ch[1] = aud->channels == 2 ? snd_sfx_chn_alloc() : -1;

    if(aud->ch[0] < 0 || (aud->channels == 2 && aud->ch[1] < 0)) {
        ds_printf("DS_ERROR: Can't allocate aica channels\n");
        audio_end();
        return -1;
    }
    size_t sep_size = aud->buffer_size / 2;
    sep_buffer[0] = aligned_alloc(32, sep_size * aud->channels);

    if(sep_buffer[0] == NULL) {
        ds_printf("DS_ERROR: Can't allocate sep buffer\n");
        audio_end();
        return -1;
    }
    if(aud->channels == 2) {
        sep_buffer[1] = sep_buffer[0] + sep_size / 4;
    }
    else {
        sep_buffer[1] = NULL;
    }

    aud->temp_buf = (uint8_t *) aligned_alloc(32, AVCODEC_MAX_AUDIO_FRAME_SIZE);

    if(!aud->temp_buf) {
        ds_printf("DS_ERROR: Can't allocate audio temp buffer\n");
        audio_end();
        return -1;
    }
    
    aud->temp_buf_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    aud->temp_buf_rpos = 0;
    aud->temp_buf_wpos = 0;
    aud->write_pos = 0;
    aud->total_samples_written = 0;
    return 0;
}

static void audio_start() {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
    int vol = GetVolumeFromSettings();

    if(vol < 0) {
        vol = 240;
    }
    aud->vol = vol;

    snd_sh4_to_aica_stop();

    /* Channel 0 */
    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = aud->ch[0];
    chan->cmd = AICA_CH_CMD_START | AICA_CH_START_DELAY;
    chan->base = aud->spu_ram_sch[0];
    chan->type = aud->type;
    chan->length = bytes_to_samples(aud->bitsize, aud->buffer_size);
    chan->loop = 1;
    chan->loopstart = 0;
    chan->loopend = chan->length - 1;
    chan->freq = aud->frequency;
    chan->vol = aud->vol;
    chan->pan = aud->channels == 2 ? 0 : 128;
    snd_sh4_to_aica(tmp, cmd->size);

    if(aud->channels == 2) {
        /* Channel 1 */
        cmd->cmd_id = aud->ch[1];
        chan->base = aud->spu_ram_sch[1];
        chan->pan = 255;
        snd_sh4_to_aica(tmp, cmd->size);

        /* Start both channels simultaneously */
        cmd->cmd_id = (1ULL << aud->ch[0]) |
                      (1ULL << aud->ch[1]);
    }
    else {
        /* Start one channel */
        cmd->cmd_id = (1ULL << aud->ch[0]);
    }
    chan->cmd = AICA_CH_CMD_START | AICA_CH_START_SYNC;
    snd_sh4_to_aica(tmp, cmd->size);
    snd_sh4_to_aica_start();
    aud->playing = 1;
}

static void audio_stop_playback(void) {
    if(!aud->playing) {
        return;
    }
    aud->playing = 0;
    snd_sh4_to_aica_stop();

    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = aud->ch[0];
    chan->cmd = AICA_CH_CMD_STOP;
    snd_sh4_to_aica(tmp, cmd->size);

    if(aud->channels == 2) {
        cmd->cmd_id = aud->ch[1];
        snd_sh4_to_aica(tmp, cmd->size);
    }
    snd_sh4_to_aica_start();
}

static void update_display_geometry() {
    if(vid->is_fullscreen) {
        vid->disp_w = 640.0f;
        vid->disp_h = (float)vid->txr[0].height * (640.0f / (float)vid->txr[0].width);

        if (vid->disp_h > 480.0f) {
            vid->disp_h = 480.0f;
            vid->disp_w = (float)vid->txr[0].width * (480.0f / (float)vid->txr[0].height);
        }
        vid->disp_x = (640.0f - vid->disp_w) / 2.0f;
        vid->disp_y = (480.0f - vid->disp_h) / 2.0f;
    }
    else {
        if(vid->params.width > 0 && vid->params.height > 0) {
            float aspect_ratio = (float)vid->txr[0].width / (float)vid->txr[0].height;
            vid->disp_w = vid->params.width;
            vid->disp_h = vid->disp_w / aspect_ratio;

            if(vid->disp_h > vid->params.height) {
                vid->disp_h = vid->params.height;
                vid->disp_w = vid->disp_h * aspect_ratio;
            }
            vid->disp_x = vid->frame_x + (vid->params.width - vid->disp_w) / 2.0f;
            vid->disp_y = vid->frame_y + (vid->params.height - vid->disp_h) / 2.0f;
        }
        else {
            vid->disp_w = (float)vid->txr[0].width * vid->params.scale;
            vid->disp_h = (float)vid->txr[0].height * vid->params.scale;
            vid->disp_x = vid->frame_x;
            vid->disp_y = vid->frame_y;
        }
    }
}

static void yuv_dma_callback(void *data) {
    (void)data;
    vid->txr_idx = (vid->txr_idx + 1) & 1;
    sem_signal(&vid->yuv_dma_done);
}

static void yuv420p_to_yuv422_dma(AVFrame *frame, video_txr_t *txr) {

    const int frame_width = txr->width;
    const int frame_height = txr->height;
    const int pvr_txr_width = txr->tw;

    uint8_t *y_plane = frame->data[0];
    uint8_t *u_plane = frame->data[1];
    uint8_t *v_plane = frame->data[2];

    const int y_stride = frame->linesize[0];
    const int u_stride = frame->linesize[1];
    const int v_stride = frame->linesize[2];

    const size_t dma_buffer_size = (pvr_txr_width * frame_height * 3) / 2;
    uint8_t *dst = vid->yuv_dma_buffer;
    int x_blk, y_blk;
    const size_t padding_size = (BYTE_SIZE_FOR_16x16_BLOCK_420 * ((pvr_txr_width >> 4) - (frame_width >> 4)));

    for(y_blk = 0; y_blk < frame_height; y_blk += 16) {
        for(x_blk = 0; x_blk < frame_width; x_blk += 16) {

            uint8_t *y_src = &y_plane[y_blk * y_stride + x_blk];
            uint8_t *u_src = &u_plane[(y_blk / 2) * u_stride + (x_blk / 2)];
            uint8_t *v_src = &v_plane[(y_blk / 2) * v_stride + (x_blk / 2)];
            int i, j;

            /* U data for 16x16 pixels */
            for(i = 0; i < 8; ++i) {
                *((uint64_t *)&dst[i * 8]) = *((uint64_t *)&u_src[i * u_stride]);
            }
            dst += 64;

            /* V data for 16x16 pixels */
            for(i = 0; i < 8; ++i) {
                *((uint64_t *)&dst[i * 8]) = *((uint64_t *)&v_src[i * v_stride]);
            }
            dst += 64;

            /* Y data for 4 (8x8 pixels) */
            for(i = 0; i < 4; ++i) {
                for(j = 0; j < 8; ++j) {
                    *((uint64_t *)&dst[i * 64 + j * 8]) = *((uint64_t *)&y_src[((i >> 1) * 8 + j) * y_stride + ((i & 1) * 8)]);
                }
            }
            dst += 256;
        }
        if (padding_size > 0) {
            memset_sh4(dst, 0, padding_size);
            dst += padding_size;
        }
    }

    PVR_SET(PVR_YUV_ADDR, (((unsigned int)txr->addr) & 0xffffff));
    dcache_purge_range((uintptr_t)vid->yuv_dma_buffer, dma_buffer_size);
    pvr_dma_yuv_conv(vid->yuv_dma_buffer, dma_buffer_size, 0, yuv_dma_callback, NULL);
}

static int yuv_init(int tw, int th, int height) {
    /* Set YUV converter configuration */
    PVR_SET(PVR_YUV_CFG, (PVR_YUV_FORMAT_YUV420 << 24) |
                            (PVR_YUV_MODE_SINGLE << 16) |
                            (((th / 16) - 1) << 8) |
                            ((tw / 16) - 1));
    PVR_GET(PVR_YUV_CFG);

    /* Allocate DMA buffer */
    const size_t dma_buffer_size = (tw * height * 3) / 2;
    vid->yuv_dma_buffer = aligned_alloc(32, dma_buffer_size);

    if(!vid->yuv_dma_buffer) {
        ds_printf("DS_ERROR: Can't allocate YUV DMA buffer\n");
        return -1;
    }
    return 0;
}

static inline void render_video_frame(video_txr_t *txr) {
    const uint32 color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    const float width_ratio = (float)txr->width / txr->tw;
    const float height_ratio = (float)txr->height / txr->th;
    const float z = 1.0f;

    plx_mat_identity();
    plx_mat3d_translate(0, 0, 0.1f);

    plx_cxt_texture(txr->plx_txr);
    plx_cxt_culling(PLX_CULL_NONE);
    plx_cxt_send(PLX_LIST_TR_POLY);

    plx_vert_ifpm3(PLX_VERT, vid->disp_x, vid->disp_y, z, color, 0.0f, 0.0f);
    plx_vert_ifpm3(PLX_VERT, vid->disp_x + vid->disp_w, vid->disp_y, z, color, width_ratio, 0.0f);
    plx_vert_ifpm3(PLX_VERT, vid->disp_x, vid->disp_y + vid->disp_h, z, color, 0.0f, height_ratio);
    plx_vert_ifpm3(PLX_VERT_EOS, vid->disp_x + vid->disp_w, vid->disp_y + vid->disp_h, z, color, width_ratio, height_ratio);
}

static inline void render_stats() {
    if(!vid->params.show_stat || !vid->stat_font_cxt) return;

    char stat_str[64], diff_str[64];
    snprintf(stat_str, sizeof(stat_str), "%.1f FPS", vid->display_fps);
    snprintf(diff_str, sizeof(diff_str), "%lld A/V", (int64_t)vid->display_av_diff);

    const float scale = vid->disp_h / 480.0f;
    const float font_size = vid->stat_font_base_size * scale;
    float text_width, text_height;

    plx_fcxt_setsize(vid->stat_font_cxt, font_size);
    plx_fcxt_str_metrics(vid->stat_font_cxt, stat_str, NULL, &text_height, &text_width, NULL);

    const float margin_x = (vid->disp_w / 640.0f) * 10.0f;

    point_t p = {
        (vid->disp_x + vid->disp_w) - text_width - margin_x,
        vid->disp_y + (vid->disp_h * (50.0f / 480.0f)),
        20.0f
    };
    point_t p_shadow = {p.x + 2.0f * scale, p.y + 2.0f * scale, p.z - 1.0f};

    plx_fcxt_begin(vid->stat_font_cxt);

    /* Shadow */
    plx_fcxt_setpos_pnt(vid->stat_font_cxt, &p_shadow);
    plx_fcxt_setcolor4f(vid->stat_font_cxt, 1.0f, 0.0f, 0.0f, 0.0f);
    plx_fcxt_draw(vid->stat_font_cxt, stat_str);
    
    /* Text */
    plx_fcxt_setpos_pnt(vid->stat_font_cxt, &p);
    plx_fcxt_setcolor4f(vid->stat_font_cxt, 1.0f, 1.0f, 1.0f, 0.0f);
    plx_fcxt_draw(vid->stat_font_cxt, stat_str);

    plx_fcxt_str_metrics(vid->stat_font_cxt, diff_str, NULL, NULL, &text_width, NULL);
    p.x = (vid->disp_x + vid->disp_w) - text_width - margin_x;
    p.y += text_height + (5.0f * scale);
    p_shadow.x = p.x + 2.0f * scale;
    p_shadow.y = p.y + 2.0f * scale;

    /* Shadow */
    plx_fcxt_setpos_pnt(vid->stat_font_cxt, &p_shadow);
    plx_fcxt_setcolor4f(vid->stat_font_cxt, 1.0f, 0.0f, 0.0f, 0.0f);
    plx_fcxt_draw(vid->stat_font_cxt, diff_str);
    
    /* Text */
    plx_fcxt_setpos_pnt(vid->stat_font_cxt, &p);
    plx_fcxt_setcolor4f(vid->stat_font_cxt, 1.0f, 1.0f, 1.0f, 0.0f);
    plx_fcxt_draw(vid->stat_font_cxt, diff_str);

    plx_fcxt_end(vid->stat_font_cxt);
    plx_fcxt_setsize(vid->stat_font_cxt, vid->stat_font_base_size);
}

static inline int decode_video_frame(AVPacket *pkt, video_txr_t *txr, AVFrame *frame) {
    int frameFinished = 0;
    int rs;
    char errbuf[256];
    uint64_t pts = 0;

    vid->codec->reordered_opaque = pkt->pts;
    rs = avcodec_decode_video2(vid->codec, frame, &frameFinished, pkt);

    if(rs < 0) {
        av_strerror(rs, errbuf, sizeof(errbuf));
        if(vid->params.verbose) {
            ds_printf("DS_ERROR: %s\n", errbuf);
        }
        vid->skip_to_keyframe = 1;
        return -1;
    }
    if(frameFinished) {

        if(vid->params.show_stat) {
            if(vid->fps_start_time == 0) {
                vid->fps_start_time = timer_ms_gettime64();
            }
            vid->frame_count_for_fps++;
            uint64_t now = timer_ms_gettime64();
            uint64_t diff = now - vid->fps_start_time;
            if(diff >= 100) {
                vid->current_fps = (float)vid->frame_count_for_fps * 1000.0f / (float)diff;

                if(vid->display_fps == 0.0f) {
                    vid->display_fps = vid->current_fps;
                }
                else {
                    vid->display_fps = vid->display_fps * 0.9f + vid->current_fps * 0.1f;
                }
                vid->fps_start_time = now;
                vid->frame_count_for_fps = 0;
            }
        }

        int64_t pts_int = AV_NOPTS_VALUE;

        if(frame->pts != AV_NOPTS_VALUE) {
            pts_int = frame->pts;
        }
        else if(frame->reordered_opaque != AV_NOPTS_VALUE) {
            pts_int = frame->reordered_opaque;
        }
        else if(pkt->dts != AV_NOPTS_VALUE) {
            pts_int = pkt->dts;
        }

        if(pts_int != AV_NOPTS_VALUE) {
            pts = av_rescale_q(pts_int, vid->format_ctx->streams[vid->video_stream]->time_base, (AVRational){1, 1000});
            if(vid->video_started == 0 && vid->audio_stream >= 0) {
                if(frame->key_frame) {
                    aud->clock_correction += ((pts - get_master_clock()) - (vid->frame_last_delay * 2));
                }
            }
            vid->video_current_pts = pts;
            vid->video_current_pts_time = timer_ms_gettime64();
        }
    }

    if(frameFinished) {
        switch(vid->codec->pix_fmt) {
            case PIX_FMT_YUVJ420P:
            case PIX_FMT_YUVJ422P:
            case PIX_FMT_YUV420P:
                yuv420p_to_yuv422_dma(frame, txr);
                break;
            case PIX_FMT_UYVY422:
            default:
                dcache_purge_range((uintptr_t)frame->data[0], txr->tw * vid->codec->height * 2);
                pvr_txr_load_dma(frame->data[0], txr->addr, txr->tw * vid->codec->height * 2, 0, yuv_dma_callback, NULL);
                break;
        }
        /* TODO: Better wait not here? */
        sem_wait(&vid->yuv_dma_done);

        if(vid->video_started < 2) {
            if(vid->video_started == 0) {
                if(frame->key_frame) {
                    vid->video_started++;
                }
            }
            else {
                vid->video_started++;
            }
        }
    }
    return 0;
}

static void PlayerDrawHandler(void *ds_event, void *param, int action) {
    (void)ds_event;
    (void)param;

    if(vid->playing) {
        /* Really needed only after seeking */
        if(vid->audio_stream >= 0 && !vid->pause && !aud->playing) {
            if((vid->is_fullscreen && action == EVENT_ACTION_RENDER) ||
                (!vid->is_fullscreen && action == EVENT_ACTION_RENDER_POST)) {
                thd_sleep(vid->frame_last_delay * 2);
            }
        }

        if(!vid->pause) {
            if(sem_trywait(&vid->video_packet_ready) == 0) {
                const int vpacket_idx = (vid->vpacket_idx + 1) & 1;
                const int decode_txr_idx = (vid->txr_idx + 1) & 1;
                AVPacket *pkt = &vid->vpackets[vpacket_idx].pkt;

                decode_video_frame(pkt, &vid->txr[decode_txr_idx], vid->frame[vid->decode_frame_idx]);
                vid->decode_frame_idx = (vid->decode_frame_idx + 1) & 1;
                sem_signal(&vid->video_packet_free);
            }
        }
        if(vid->video_started > 1) {
            switch(action) {
                case EVENT_ACTION_RENDER:
                    if(vid->is_fullscreen) {
                        pvr_list_begin(PVR_LIST_TR_POLY);
                        render_video_frame(&vid->txr[vid->txr_idx]);
                        render_stats();
                        pvr_list_finish();
                    }
                    else if(!vid->sdl_gui_managed) {
                        render_video_frame(&vid->txr[vid->txr_idx]);
                        render_stats();
                    }
                    break;
                case EVENT_ACTION_RENDER_POST:
                    if(!vid->is_fullscreen && !vid->pause &&
                        !ScreenIsHidden() && !ConsoleIsVisible()) {
                        render_video_frame(&vid->txr[vid->txr_idx]);
                        render_stats();
                    }
                    break;
            }
        }
    }
}

static void handle_player_click(int button) {
    if(vid->is_fullscreen) {
        if(vid->back_to_window) {
            if(button == SDL_BUTTON_LEFT) {
                vid->is_fullscreen = 0;
                vid->frame_x = vid->frame_x_old;
                vid->frame_y = vid->frame_y_old;
                ds_sfx_play(DS_SFX_CLICK);
                update_display_geometry();
                if(vid->sdl_gui_managed) {
                    EnableScreen();
                    GUI_Enable();
                }
            }
        }
        else {
            if(button == SDL_BUTTON_LEFT) {
                ffplay_toggle_pause();
            }
            else if(button == SDL_BUTTON_RIGHT) {
                if(vid->playing && !vid->done) {
                    vid->done = 2;
                }
            }
        }
    }
    else {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        if(button == SDL_BUTTON_LEFT &&
            mx >= vid->disp_x && mx <= vid->disp_x + vid->disp_w &&
            my >= vid->disp_y && my <= vid->disp_y + vid->disp_h) {
            vid->is_fullscreen = 1;
            vid->back_to_window = 1;
            vid->frame_x_old = vid->frame_x;
            vid->frame_y_old = vid->frame_y;
            ds_sfx_play(DS_SFX_CLICK);
            update_display_geometry();

            if(vid->sdl_gui_managed) {
                DisableScreen();
                GUI_Disable();
            }
        }
    }
}

static void PlayerInputHandler(void *ds_event, void *param, int action) {
    (void)ds_event;
    SDL_Event *event = (SDL_Event *)param;
    static uint64_t last_seek_time = 0;
    static int inside_video_area = 0;

    switch(event->type) {
        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case SDL_DC_A:
                    if(vid->is_fullscreen)
                        handle_player_click(SDL_BUTTON_LEFT);
                    break;
                case SDL_DC_B:
                    if(vid->is_fullscreen)
                        handle_player_click(SDL_BUTTON_RIGHT);
                    break;
                case SDL_DC_Y:
                    if(vid->is_fullscreen || (inside_video_area && !vid->pause)) {
                        if(vid->params.show_stat) {
                            vid->params.show_stat = 0;
                        }
                        else {
                            load_stat_font();
                            vid->params.show_stat = 1;
                        }
                    }
                    break;
                case SDL_DC_X:
                    if(vid->is_fullscreen || (inside_video_area && !vid->pause))
                        ffplay_toggle_pause();
                    break;
            }
            break;
        case SDL_JOYHATMOTION:
            if(vid->is_fullscreen && event->jhat.hat == 0) {
                uint64_t now = timer_ms_gettime64();
                if(vid->seek_request == 0 && now - last_seek_time > 250) {
                    if(event->jhat.value & SDL_HAT_RIGHT) {
                        vid->seek_request = 1;
                        last_seek_time = now;
                    }
                    else if(event->jhat.value & SDL_HAT_LEFT) {
                        vid->seek_request = -1;
                        last_seek_time = now;
                    }
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(vid->is_fullscreen || !vid->pause) {
                handle_player_click(event->button.button);
            }
            break;
        case SDL_MOUSEMOTION:
            if(!vid->is_fullscreen) {
                int mx = event->motion.x, my = event->motion.y;
                if(mx >= vid->disp_x && mx <= vid->disp_x + vid->disp_w &&
                    my >= vid->disp_y && my <= vid->disp_y + vid->disp_h) {
                    if(!inside_video_area) {
                        ds_sfx_play(DS_SFX_CLICK2);
                        inside_video_area = 1;
                    }
                }
                else {
                    inside_video_area = 0;
                }
            }
            break;
        default:
            break;
    }

    if(vid->playing && vid->is_fullscreen && !vid->done) {
        /* Free up some CPU time from the very frequent input polling. */
        thd_sleep(100);
    }
}

static void free_video_texture(video_txr_t *txr) {
    if(txr->plx_txr) {
        free(txr->plx_txr);
        txr->plx_txr = NULL;
    }
    if(txr->addr) {
        pvr_mem_free(txr->addr);
        txr->addr = NULL;
    }
}

static void create_video_texture(video_txr_t *txr, int width, int height, int format, int filler) {
    int tw, th;
    for(tw = 8; tw < width ; tw <<= 1);
    for(th = 8; th < height; th <<= 1);

    txr->width = width;
    txr->height = height;
    txr->tw = tw;
    txr->th = th;

    if(txr->addr) {
        free_video_texture(txr);
    }
    txr->addr = pvr_mem_malloc(tw * th * 2);
    txr->plx_txr = (plx_texture_t *) malloc(sizeof(plx_texture_t));

    if(!txr->plx_txr) {
        ds_printf("DS_ERROR: Can't alloc memory for plx_texture_t\n");
        return;
    }

    txr->plx_txr->ptr = txr->addr;
    txr->plx_txr->w = tw;
    txr->plx_txr->h = th;
    txr->plx_txr->fmt = format | PVR_TXRFMT_NONTWIDDLED;
    plx_fill_contexts(txr->plx_txr);
    plx_txr_setfilter(txr->plx_txr, filler);
}

static int dup_packet(PacketBuffer_t *dst_buf, AVPacket *src_pkt) {
    if(dst_buf->buffer_size < src_pkt->size) {
        uint8_t *new_buffer = av_realloc(dst_buf->buffer, src_pkt->size + FF_INPUT_BUFFER_PADDING_SIZE);
        if(!new_buffer) {
            av_free(dst_buf->buffer);
            dst_buf->buffer = NULL;
            dst_buf->buffer_size = 0;
            ds_printf("DS_ERROR: Can't allocate packet buffer\n");
            return AVERROR(ENOMEM);
        }
        dst_buf->buffer = new_buffer;
        dst_buf->buffer_size = src_pkt->size + FF_INPUT_BUFFER_PADDING_SIZE;
    }

    av_free_packet(&dst_buf->pkt);
    dst_buf->pkt = *src_pkt;
    dst_buf->pkt.data = dst_buf->buffer;

    memcpy_sh4(dst_buf->pkt.data, src_pkt->data, src_pkt->size);
    memset_sh4(dst_buf->pkt.data + src_pkt->size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    dst_buf->pkt.destruct = NULL;
    return 0;
}

static int ffplay_do_seek(int64_t pos_ms, int flags) {

    int64_t target_ts = pos_ms * (AV_TIME_BASE / 1000);
    int64_t duration = vid->format_ctx->duration;

    if(target_ts < 0) {
        target_ts = 0;
    }
    else if(duration > 0 && target_ts >= duration) {
        target_ts = duration - (1 * AV_TIME_BASE);
    }

    if(vid->params.verbose) {
        ds_printf("DS_FFMPEG: Seeking to %lld (%lld ms)\n", target_ts, pos_ms);
    }
    if(av_seek_frame(vid->format_ctx, -1, target_ts, flags) < 0) {
        ds_printf("DS_ERROR: Error seeking file\n");
        return -1;
    }

    if(vid->codec) avcodec_flush_buffers(vid->codec);

    if(vid->audio_codec) {
        audio_stop_playback();
        spu_memset_sq(aud->spu_ram_sch[0], 0, aud->buffer_size * aud->channels);
        avcodec_flush_buffers(vid->audio_codec);
        aud->write_pos = 0;
        aud->total_samples_written = 0;
        aud->clock_correction = pos_ms;
        aud->temp_buf_rpos = 0;
        aud->temp_buf_wpos = 0;
        aud->last_clock_val = 0;
        aud->last_clock_update_time = 0;
    }

    vid->video_current_pts = pos_ms;
    vid->video_current_pts_time = timer_ms_gettime64();
    vid->start_time = timer_ms_gettime64();
    vid->frame_timer = vid->start_time;
    vid->skip_to_keyframe = 1;
    vid->video_started = 0;

    vid->txr_idx = 0;
    vid->decode_frame_idx = 0;
    vid->vpacket_idx = 0;

    sem_init(&vid->video_packet_free, 1);
    sem_init(&vid->video_packet_ready, 0);
    sem_init(&vid->yuv_dma_done, 0);

    return 0;
}

static int ffplay_seek_internal(int64_t pos_ms, int flags) {
    if(!vid->playing || vid->done) {
        return -1;
    }

    LockVideo();
    if(ffplay_do_seek(pos_ms, flags) < 0) {
        UnlockVideo();
        return -1;
    }
    UnlockVideo();
    return 0;
}

static void ffplay_seek_relative(void) {
    const int64_t seek_step = 10000;
    const int64_t target_ts = get_master_clock() + vid->seek_request * seek_step;
    ffplay_seek_internal(target_ts, AVSEEK_FLAG_BACKWARD);
    vid->seek_request = 0;
}

static void send_audio_data() {
    size_t min_chunk_size_bytes = AUDIO_MIN_CHUNK_SIZE;
    size_t chunk_size_bytes;
    size_t free_bytes;
    size_t used = aud->temp_buf_wpos - aud->temp_buf_rpos;

    if(used < min_chunk_size_bytes) {
        return;
    }

    if(aud->playing) {
        uint32_t play_pos = snd_get_pos(aud->ch[0]);
        size_t used_samples;
        uint32_t write_pos = aud->write_pos;

        if(write_pos >= play_pos) {
            used_samples = write_pos - play_pos;
        }
        else {
            used_samples = (aud->buffer_size_samples - play_pos) + write_pos;
        }
        size_t free_samples = aud->buffer_size_samples - used_samples;
        free_bytes = samples_to_bytes(aud->bitsize, free_samples);
    }
    else {
        free_bytes = aud->buffer_size - samples_to_bytes(aud->bitsize, aud->write_pos);
    }

    if(free_bytes < min_chunk_size_bytes) {
        return;
    }

    chunk_size_bytes = (used < free_bytes) ? used : free_bytes;
    chunk_size_bytes = (chunk_size_bytes / min_chunk_size_bytes) * min_chunk_size_bytes;

    if(chunk_size_bytes < min_chunk_size_bytes) {
        return;
    }
    size_t nsamples_in_chunk = bytes_to_samples(aud->bitsize, chunk_size_bytes) / aud->channels;

    if(audio_dma_transfer(aud->temp_buf + aud->temp_buf_rpos, chunk_size_bytes, 1) < 0) {
        vid->done = 2;
        return;
    }

    aud->write_pos = (aud->write_pos + nsamples_in_chunk) % aud->buffer_size_samples;
    aud->total_samples_written += nsamples_in_chunk;
    
    aud->temp_buf_rpos += chunk_size_bytes;

    if(aud->temp_buf_rpos == aud->temp_buf_wpos) {
        aud->temp_buf_rpos = 0;
        aud->temp_buf_wpos = 0;
    }
}

static void ffplay_free();

static void *player_thread(void *p) {
    int r = 0;
    AVPacket packet;
    char errbuf[256];
    const int is_sd_card = strncmp(vid->filename, "/sd", 3) == 0;

    if(vid->audio_stream >= 0) {
        vid->video_current_pts = 0;
        vid->video_current_pts_time = timer_ms_gettime64();
    }
    vid->start_time = timer_ms_gettime64();
    if(vid->video_current_pts == 0) {
        vid->video_current_pts_time = vid->start_time;
    }
    vid->frame_timer = vid->start_time;
    vid->frame_last_delay = 40;

    if(vid->codec && vid->format_ctx->streams[vid->video_stream]->avg_frame_rate.den > 0) {
        AVRational frame_rate = vid->format_ctx->streams[vid->video_stream]->avg_frame_rate;
        vid->frame_last_delay = av_rescale_q(1, (AVRational){frame_rate.den, frame_rate.num}, (AVRational){1, 1000});
    }

    /* Initial A/V sync */
    if(ffplay_seek_internal(0, AVSEEK_FLAG_BACKWARD) < 0) {
        vid->done = 2;
    }

    while(!vid->done) {
        if(vid->pause) {
            thd_sleep(100);
            continue;
        }
        if(vid->params.update_callback) {
            int64_t current_ts = get_master_clock();
            if(current_ts - vid->last_callback_pts > 250) {
                int64_t duration_ms = vid->format_ctx->duration * 1000 / AV_TIME_BASE;
                vid->params.update_callback(current_ts, duration_ms);
                vid->last_callback_pts = current_ts;
            }
        }

        if(is_sd_card) {
            LockVideo();
            r = av_read_frame(vid->format_ctx, &packet);
            UnlockVideo();
        }
        else {
            r = av_read_frame(vid->format_ctx, &packet);
        }

        if(r < 0) {
            if(r == AVERROR_EOF && vid->params.loop) {
                if(ffplay_seek_internal(0, AVSEEK_FLAG_BACKWARD) < 0) {
                    vid->done = 2;
                    break;
                }
                continue;
            }
            else {
                if(r != AVERROR_EOF) {
                    av_strerror(r, errbuf, sizeof(errbuf));
                    if (vid->params.verbose) {
                        ds_printf("DS_ERROR: av_read_frame(): %s\n", errbuf);
                    }
                }
                vid->done = 2;
                break;
            }
        }
        if(vid->done) {
            av_free_packet(&packet);
            break;
        }
        if(vid->seek_request != 0) {
            ffplay_seek_relative();
            av_free_packet(&packet);
            continue;
        }
        if(aud->playing && (aud->temp_buf_wpos - aud->temp_buf_rpos) >= AUDIO_MIN_CHUNK_SIZE) {
            send_audio_data();
        }
        if(packet.stream_index == vid->video_stream) {

            if(vid->audio_stream >= 0 && !aud->playing && vid->skip_to_keyframe) {
                av_free_packet(&packet);
                continue;
            }

            int ignore_diff = 0;

            if(vid->skip_to_keyframe) {
                if(packet.flags & AV_PKT_FLAG_KEY) {
                    vid->skip_to_keyframe = 0;
                    ignore_diff = 1;
                }
                else {
                    av_free_packet(&packet);
                    continue;
                }
            }
            int64_t delay = vid->frame_last_delay;
            int64_t diff = 0;

            if(vid->audio_stream >= 0 && !ignore_diff) {
                diff = get_video_clock() - get_master_clock();

                if(diff < -1000) {
                    ffplay_seek_internal(get_master_clock() + 1000, AVSEEK_FLAG_BACKWARD);
                    av_free_packet(&packet);
                    continue;
                }

                int64_t sync_threshold = 10;

                if(diff < -sync_threshold) {
                    delay = 0;
                }
                else if(diff > sync_threshold) {
                    delay += diff / 2;
                }
            }

            if(vid->params.show_stat) {
                vid->av_diff = diff;

                if(vid->display_av_diff == 0.0f) {
                    vid->display_av_diff = (float)vid->av_diff;
                } 
                else {
                    vid->display_av_diff = vid->display_av_diff * 0.9f + (float)vid->av_diff * 0.1f;
                }
            }

            vid->frame_timer += delay;
            int64_t time_to_wait = vid->frame_timer - timer_ms_gettime64();

            // if(time_to_wait >= 5 || diff < -250) {
            //     dbglog(DBG_DEBUG, "SYNC: delay=%lld diff=%lld vclock=%lld aclock=%lld wait=%lld\n",
            //         delay, diff, get_video_clock(), get_audio_clock(), time_to_wait);
            // }

            if(time_to_wait > 500) {
                ffplay_seek_internal(get_master_clock() + 1000, AVSEEK_FLAG_BACKWARD);
                av_free_packet(&packet);
                continue;
            }

            while(time_to_wait > 1) {
                thd_sleep(time_to_wait > 5 ? 5 : time_to_wait);
                if(aud->temp_buf_wpos - aud->temp_buf_rpos >= AUDIO_MIN_CHUNK_SIZE) {
                    send_audio_data();
                }
                time_to_wait = vid->frame_timer - timer_ms_gettime64();
            }

            sem_wait(&vid->video_packet_free);

            if(vid->done) {
                av_free_packet(&packet);
                break;
            }
            if(dup_packet(&vid->vpackets[vid->vpacket_idx], &packet) < 0) {
                vid->done = 2;
                av_free_packet(&packet);
                break;
            }
            av_free_packet(&packet);

            vid->vpacket_idx = (vid->vpacket_idx + 1) & 1;
            sem_signal(&vid->video_packet_ready);
        }
        else if(packet.stream_index == vid->audio_stream) {

            if(vid->done) {
                av_free_packet(&packet);
                break;
            }

            size_t used = aud->temp_buf_wpos - aud->temp_buf_rpos;

            if(aud->temp_buf_size - aud->temp_buf_wpos < AUDIO_DECODE_BUFFER_GUARD) {
                if(aud->temp_buf_size - used >= AUDIO_DECODE_BUFFER_GUARD) {
                    memmove(aud->temp_buf, aud->temp_buf + aud->temp_buf_rpos, used);
                    aud->temp_buf_wpos -= aud->temp_buf_rpos;
                    aud->temp_buf_rpos = 0;
                }
            }
            used = aud->temp_buf_wpos - aud->temp_buf_rpos;

            /* Wait for free space in buffer */
            while(aud->temp_buf_size - used < AUDIO_DECODE_BUFFER_GUARD && !vid->done) {
                send_audio_data();
                used = aud->temp_buf_wpos - aud->temp_buf_rpos;
                if(aud->temp_buf_size - used < AUDIO_DECODE_BUFFER_GUARD) {
                    thd_sleep(10);
                }
            }

            if(vid->done) {
                av_free_packet(&packet);
                break;
            }

            int audio_data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
            r = avcodec_decode_audio3(vid->audio_codec, (int16_t *)(aud->temp_buf + aud->temp_buf_wpos), &audio_data_size, &packet);

            if(r < 0) {
                if (vid->params.verbose) {
                    av_strerror(r, errbuf, sizeof(errbuf));
                    ds_printf("DS_ERROR: %s\n", errbuf);
                }
            }
            else {

                if(audio_data_size > 0) {

                    aud->temp_buf_wpos += audio_data_size;

                    if(!aud->playing) {
                        send_audio_data();
                        /* Start playback only when we have enough data buffered */
                        if(aud->write_pos >= 8 << 10) {
                            audio_start();
                        }
                    }
                }
            }
            av_free_packet(&packet);
        }
        else {
            av_free_packet(&packet);
        }
    }

    ds_printf("DS_INFO: Playback stopped.\n");

    if(vid->done == 2) {
        ffplay_free();
    }
    return NULL;
}

static int ffplay_open_file(const char *filename, ffplay_params_t *params) {
    char errbuf[256];
    int r = 0;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pVideoCodecCtx = NULL, *pAudioCodecCtx = NULL;
    AVInputFormat *file_iformat = NULL;
    int videoStream = -1, audioStream = -1;
    char fn[NAME_MAX];

    sprintf(fn, "ds:%s", filename);

    file_iformat = params->force_format ? av_find_input_format(params->force_format) : NULL;
    ds_printf("DS_FFMPEG: Opening file: %s\n", filename);
    r = av_open_input_file((AVFormatContext**)(&pFormatCtx), fn, file_iformat, 0, NULL);

    if(r < 0) {
        av_strerror(r, errbuf, sizeof(errbuf));
        ds_printf("DS_ERROR: %s\n", errbuf);
        return -1;
    }
    r = av_find_stream_info(pFormatCtx);

    if(r < 0) {
        av_strerror(r, errbuf, sizeof(errbuf));
        ds_printf("DS_ERROR: %s\n", errbuf);
        av_close_input_file(pFormatCtx);
        return -1;
    }

    if(params->verbose) {
        dump_format(pFormatCtx, 0, filename, 0);
    }

    pVideoCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_VIDEO, &videoStream);
    pAudioCodecCtx = findDecoder(pFormatCtx, AVMEDIA_TYPE_AUDIO, &audioStream);

    if(pVideoCodecCtx) {
        vid->codec = pVideoCodecCtx;
        vid->video_stream = videoStream;
    }
    else {
        ds_printf("DS_ERROR: Didn't find a video stream or unsupported codec.\n");
    }

    if(pAudioCodecCtx) {
        if(audio_init(pAudioCodecCtx) < 0) {
            ds_printf("DS_ERROR: Can't initialize audio.\n");
            av_close_input_file(pFormatCtx);
            return -1;
        }
        vid->audio_codec = pAudioCodecCtx;
        vid->audio_stream = audioStream;
    }
    else {
        ds_printf("DS_ERROR: Didn't find a audio stream or unsupported codec.\n");
    }
    vid->format_ctx = pFormatCtx;
    return 0;
}

static void ffplay_close_file() {
    audio_end();
    if(vid->codec) {
        avcodec_close(vid->codec);
        vid->codec = NULL;
    }
    if(vid->audio_codec) {
        avcodec_close(vid->audio_codec);
        vid->audio_codec = NULL;
    }
    if(vid->format_ctx) {
        av_close_input_file(vid->format_ctx);
        vid->format_ctx = NULL;
    }
}

static AVCodecContext *findDecoder(AVFormatContext *format, int type, int *index) {
    AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    int i, r;
    char errbuf[256];
    int good_audio = -1;

    /* Discard all streams of the given type and find a preferred audio stream if applicable */
    for(i = 0; i < format->nb_streams; i++) {
        if(format->streams[i]->codec->codec_type == type) {
            format->streams[i]->discard = AVDISCARD_ALL;

            if(type == AVMEDIA_TYPE_AUDIO) {
                switch(format->streams[i]->codec->codec_id) {
                    case CODEC_ID_ADPCM_YAMAHA:
                    case CODEC_ID_PCM_S16LE:
                    // case CODEC_ID_MP3:
                    // case CODEC_ID_MP2:
                    // case CODEC_ID_MP1:
                    case CODEC_ID_AAC:
                    case CODEC_ID_AC3:
                        good_audio = i;
                        if(vid->params.verbose) {
                            ds_printf("DS_INFO: Found prefered audio stream: %d\n", i);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    /* Find and open a decoder for the selected stream */
    for(i = 0; i < format->nb_streams; i++) {
        if(format->streams[i]->codec->codec_type == type) {
            if(good_audio > -1 && good_audio != i) {
                continue;
            }
            codec_ctx = format->streams[i]->codec;
            codec = avcodec_find_decoder(codec_ctx->codec_id);

            if(codec == NULL) {
                format->streams[i]->discard = AVDISCARD_ALL;
                codec_ctx = NULL;
                continue;
            }
            if(vid->params.verbose) {
                ds_printf("DS_OK: Using codec: %s\n", codec->long_name ? codec->long_name : codec->name);
            }
            r = avcodec_open(codec_ctx, codec);
            if(r < 0) {
                if (vid->params.verbose) {
                    av_strerror(r, errbuf, sizeof(errbuf));
                    ds_printf("DS_ERROR: %s\n", errbuf);
                }
                continue;
            }
            format->streams[i]->discard = AVDISCARD_DEFAULT;
            *index = i;
            return codec_ctx;
        }
    }
    return NULL;
}

int ffplay_is_playing() {
    return vid->playing;
}

int ffplay_is_paused() {
    return vid->pause;
}

void ffplay_toggle_pause() {
    if(!vid->playing) {
        return;
    }
    vid->pause = !vid->pause;
    if(aud->playing) {
        AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
        snd_sh4_to_aica_stop();

        cmd->cmd = AICA_CMD_CHAN;
        cmd->timestamp = 0;
        cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
        cmd->cmd_id = aud->ch[0];
        chan->cmd = AICA_CH_CMD_UPDATE | AICA_CH_UPDATE_SET_VOL;
        chan->vol = vid->pause ? 0 : aud->vol;
        snd_sh4_to_aica(tmp, cmd->size);

        if(aud->channels == 2) {
            cmd->cmd_id = aud->ch[1];
            snd_sh4_to_aica(tmp, cmd->size);
        }
        snd_sh4_to_aica_start();
    }
}

int ffplay(const char *filename, ffplay_params_t *params) {
    while(vid->done) {
        thd_sleep(10);
    }
    if(vid->playing) {
        ffplay_shutdown();
    }

    memset(vid, 0, sizeof(vids));
    memset(aud, 0, sizeof(auds));
    av_log_set_level(params->verbose ? AV_LOG_INFO : AV_LOG_QUIET);
    vid->done = 3;

    if(ffplay_open_file(filename, params) < 0) {
        ffplay_close_file();
        vid->done = 0;
        return -1;
    }
    vid->frame[0] = avcodec_alloc_frame();
    vid->frame[1] = avcodec_alloc_frame();

    if(!vid->frame[0] || !vid->frame[1]) {
        ds_printf("DS_ERROR: avcodec_alloc_frame() failed\n");
        ffplay_close_file();
        vid->done = 0;
        return -1;
    }
    vid->decode_frame_idx = 0;
    sem_init(&vid->video_packet_free, 1);
    sem_init(&vid->video_packet_ready, 0);
    sem_init(&vid->yuv_dma_done, 0);

    memcpy(&vid->params, params, sizeof(ffplay_params_t));
    strncpy(vid->filename, filename, NAME_MAX);

    vid->is_fullscreen = vid->params.fullscreen;
    vid->back_to_window = !vid->params.fullscreen;
    vid->sdl_gui_managed = ScreenIsEnabled();

    if(vid->is_fullscreen && vid->sdl_gui_managed) {
        GUI_Disable();
        DisableScreen();
    }

    if(vid->codec) {
        float bbox_w, bbox_h;

        if (vid->params.width > 0 && vid->params.height > 0) {
            bbox_w = vid->params.width;
            bbox_h = vid->params.height;
        }
        else {
            bbox_w = vid->codec->width * vid->params.scale;
            bbox_h = vid->codec->height * vid->params.scale;
        }

        if(vid->params.x == -1) {
            vid->frame_x = (640 - bbox_w) / 2.0f;
        }
        else {
            vid->frame_x = (float)vid->params.x;
        }

        if(vid->params.y == -1) {
            vid->frame_y = (480 - bbox_h) / 2.0f;
        }
        else {
            vid->frame_y = (float)vid->params.y;
        }
        int format = 0;

        switch(vid->codec->pix_fmt) {
            case PIX_FMT_YUV420P:
            case PIX_FMT_YUVJ420P:
            case PIX_FMT_YUVJ422P:
            case PIX_FMT_UYVY422:
                format = PVR_TXRFMT_YUV422;
                break;
            default:
                format = PVR_TXRFMT_RGB565;
                break;
        }
        create_video_texture(&vid->txr[0], vid->codec->width, vid->codec->height, format | PVR_TXRFMT_NONTWIDDLED, PVR_FILTER_BILINEAR);
        create_video_texture(&vid->txr[1], vid->codec->width, vid->codec->height, format | PVR_TXRFMT_NONTWIDDLED, PVR_FILTER_BILINEAR);
        update_display_geometry();

        if(format & PVR_TXRFMT_YUV422) {
            if(yuv_init(vid->txr[0].tw, vid->txr[0].th, vid->txr[0].height) < 0) {
                ffplay_free();
                return -1;
            }
        }
    }

    if(vid->params.show_stat) {
        load_stat_font();
    }

    vid->video_event = AddEvent("ffmpeg_player_video", EVENT_TYPE_VIDEO, EVENT_PRIO_OVERLAY, PlayerDrawHandler, NULL);
    vid->input_event = AddEvent("ffmpeg_player_input", EVENT_TYPE_INPUT, EVENT_PRIO_DEFAULT, PlayerInputHandler, NULL);

    if(params->verbose) {
        ds_printf("DS_FFMPEG: Starting playback thread...\n");
    }
    vid->playing = 1;
    vid->done = 0;

    vid->thread = thd_create(0, player_thread, NULL);

    if(!vid->thread) {
        ds_printf("DS_ERROR: Can't create player thread!\n");
        ffplay_free();
        return -1;
    }
    return 0;
}

static void load_stat_font() {
    if(vid->stat_font) {
        return;
    }

    char fn[NAME_MAX];
    sprintf(fn, "%s/fonts/txf/axaxax.txf", getenv("PATH"));

    LockVideo();
    vid->stat_font = plx_font_load(fn);

    if(vid->stat_font) {
        vid->stat_font_cxt = plx_fcxt_create(vid->stat_font, PVR_LIST_TR_POLY);
        vid->stat_font_base_size = plx_fcxt_getsize(vid->stat_font_cxt);
    }
    UnlockVideo();
}

static void ffplay_free() {
    if(vid->stat_font_cxt) {
        plx_fcxt_destroy(vid->stat_font_cxt);
    }
    if(vid->stat_font) {
        plx_font_destroy(vid->stat_font);
    }
    if(vid->video_event) {
        RemoveEvent(vid->video_event);
    }
    if(vid->input_event) {
        RemoveEvent(vid->input_event);
    }
    if(vid->vpackets[0].buffer) {
        av_free(vid->vpackets[0].buffer);
    }
    av_free_packet(&vid->vpackets[0].pkt);
    if(vid->vpackets[1].buffer) {
        av_free(vid->vpackets[1].buffer);
    }
    av_free_packet(&vid->vpackets[1].pkt);
    ffplay_close_file();
    free_video_texture(&vid->txr[0]);
    free_video_texture(&vid->txr[1]);

    sem_destroy(&vid->video_packet_free);
    sem_destroy(&vid->video_packet_ready);
    sem_destroy(&vid->yuv_dma_done);

    if(vid->yuv_dma_buffer) {
        free(vid->yuv_dma_buffer);
    }
    if(vid->frame[0]) {
        av_free(vid->frame[0]);
    }
    if(vid->frame[1]) {
        av_free(vid->frame[1]);
    }
    vid->playing = 0;
    vid->done = 0;

    if(vid->is_fullscreen && vid->sdl_gui_managed) {
        EnableScreen();
        GUI_Enable();
    }
}

void ffplay_shutdown() {
    if(!vid->playing || vid->done) {
        return;
    }
    vid->done = 1;

    if(vid->input_event) {
        RemoveEvent(vid->input_event);
        vid->input_event = NULL;
    }
    if(vid->video_event) {
        RemoveEvent(vid->video_event);
        vid->video_event = NULL;
    }
    sem_signal(&vid->video_packet_free);

    if(vid->thread) {
        thd_join(vid->thread, NULL);
        vid->thread = NULL;
    }
    ffplay_free();
}

int ffplay_seek(int64_t pos_ms) {
    return ffplay_seek_internal(pos_ms, AVSEEK_FLAG_BACKWARD);
}

int ffplay_info(ffplay_info_t *info) {
    if(!vid->playing || !info || vid->done) {
        return -1;
    }
    info->duration = vid->format_ctx->duration * 1000 / AV_TIME_BASE;
    info->format = vid->format_ctx->iformat->name;

    if(vid->codec) {
        info->video_codec = vid->codec->codec->name;
        info->width = vid->codec->width;
        info->height = vid->codec->height;
        info->video_bitrate = vid->codec->bit_rate / 1000;

        AVStream *st = vid->format_ctx->streams[vid->video_stream];
        if(st->avg_frame_rate.num && st->avg_frame_rate.den) {
            info->fps = av_q2d(st->avg_frame_rate);
        }
        else {
            info->fps = 0.0f;
        }
    }
    else {
        info->video_codec = "none";
        info->width = 0;
        info->height = 0;
        info->video_bitrate = 0;
        info->fps = 0.0f;
    }

    if(vid->audio_codec) {
        info->audio_codec = vid->audio_codec->codec->name;
        info->audio_bitrate = vid->audio_codec->bit_rate / 1000;
        info->audio_channels = vid->audio_codec->channels;
    }
    else {
        info->audio_codec = "none";
        info->audio_bitrate = 0;
        info->audio_channels = 0;
    }
    return 0;
}

int64_t ffplay_get_pos() {
    return get_master_clock();
}
