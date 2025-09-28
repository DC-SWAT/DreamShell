/* DreamShell ##version##

   preview.c - dreameye preview
   Copyright (C) 2023-2025 SWAT

*/          

#include "ds.h"
#include "sfx.h"
#include "drivers/dreameye.h"

/* u_block + v_block + y_block = 64 + 64 + 256 = 384 for YUV420 */
/* u_block + v_block + y_block = 128 + 128 + 256 = 512 for YUV422 */
#define BYTE_SIZE_FOR_16x16_BLOCK_420 384
#define BYTE_SIZE_FOR_16x16_BLOCK_422 512

#define PVR_YUV_FORMAT_YUV420 0
#define PVR_YUV_FORMAT_YUV422 1

#define PVR_YUV_MODE_SINGLE 0
#define PVR_YUV_MODE_MULTI 1

/* The frame dimensions can be different than the dimensions of the pvr
   texture BUT the dimensions have to be a multiple of 16 */
static int frame_txr_width = 320;
static int frame_txr_height = 240;
static int pvr_txr_width = 512;
static int pvr_txr_height = 256;
static float frame_scale = 1.0f;
static float frame_x = 0.0f;
static float frame_y = 0.0f;
static float frame_x_old = 0.0f;
static float frame_y_old = 0.0f;
static int frame_format = DREAMEYE_FRAME_FMT_YUV420P;

static pvr_ptr_t pvr_txr;
static plx_texture_t *plx_txr;
static uint8_t *yuv_dma_buffer = NULL;

static int capturing = 0;
static int got_frame = 0;
static int is_fullscreen;
static int back_to_window;

static kthread_t *thread;
static Event_t *input_event;
static Event_t *video_event;

static int show_stat = 0;
static uint64_t fps_start_time = 0;
static uint32_t frame_count_for_fps = 0;
static float current_fps = 0.0f;
static float display_fps = 0.0f;
static plx_font_t *stat_font;
static plx_fcxt_t *stat_font_cxt;
static float stat_font_base_size;

static maple_device_t *dreameye;
static dreameye_frame_cb frame_callback;


static void dreameye_preview_frame() {

    const uint32_t color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    const float width_ratio = (float)frame_txr_width / pvr_txr_width;
    const float height_ratio = (float)frame_txr_height / pvr_txr_height;
    const float native_width = (is_fullscreen ? 640.0f : (float)frame_txr_width * frame_scale);
    const float native_height = (is_fullscreen ? 480.0f : (float)frame_txr_height * frame_scale);
    const float z = 1.0f;

    if(!got_frame) {
        // TODO: Show waiting picture
        return;
    }
    plx_mat_identity();
    plx_mat3d_translate(0, 0, 0.1f);

	plx_cxt_texture(plx_txr);
	plx_cxt_culling(PLX_CULL_NONE);
	plx_cxt_send(PLX_LIST_TR_POLY);

	plx_vert_ifpm3(PLX_VERT, frame_x, frame_y, z, color, 0.0f, 0.0f);
	plx_vert_ifpm3(PLX_VERT, frame_x + native_width, frame_y, z, color, width_ratio, 0.0f);
	plx_vert_ifpm3(PLX_VERT, frame_x, frame_y + native_height, z, color, 0.0f, height_ratio);
	plx_vert_ifpm3(PLX_VERT_EOS, frame_x + native_width, frame_y + native_height, z, color, width_ratio, height_ratio);
}

static void load_stat_font(void) {
    if(stat_font) return;

    char fn[NAME_MAX];
    sprintf(fn, "%s/fonts/txf/axaxax.txf", getenv("PATH"));

    LockVideo();
    stat_font = plx_font_load(fn);

    if(stat_font) {
        stat_font_cxt = plx_fcxt_create(stat_font, PVR_LIST_TR_POLY);
        stat_font_base_size = plx_fcxt_getsize(stat_font_cxt);
    }
    UnlockVideo();
}

static inline void render_stats() {
    if(!show_stat || !stat_font_cxt) return;

    char stat_str[64];
    snprintf(stat_str, sizeof(stat_str), "%.1f FPS", display_fps);

    const float scale = (is_fullscreen ? 480.0f : (float)frame_txr_height * frame_scale) / 480.0f;
    const float font_size = stat_font_base_size * scale;
    float text_width, text_height;

    plx_fcxt_setsize(stat_font_cxt, font_size);
    plx_fcxt_str_metrics(stat_font_cxt, stat_str, NULL, &text_height, &text_width, NULL);

    const float disp_w = is_fullscreen ? 640.0f : (float)frame_txr_width * frame_scale;
    const float margin_x = (disp_w / 640.0f) * 10.0f;

    point_t p = {
        (frame_x + disp_w) - text_width - margin_x,
        frame_y + ((is_fullscreen ? 480.0f : (float)frame_txr_height * frame_scale) * (50.0f / 480.0f)),
        20.0f
    };
    point_t p_shadow = {p.x + 2.0f * scale, p.y + 2.0f * scale, p.z - 1.0f};

    plx_fcxt_begin(stat_font_cxt);

    /* Shadow */
    plx_fcxt_setpos_pnt(stat_font_cxt, &p_shadow);
    plx_fcxt_setcolor4f(stat_font_cxt, 1.0f, 0.0f, 0.0f, 0.0f);
    plx_fcxt_draw(stat_font_cxt, stat_str);
    
    /* Text */
    plx_fcxt_setpos_pnt(stat_font_cxt, &p);
    plx_fcxt_setcolor4f(stat_font_cxt, 1.0f, 1.0f, 1.0f, 0.0f);
    plx_fcxt_draw(stat_font_cxt, stat_str);

    plx_fcxt_end(stat_font_cxt);
    plx_fcxt_setsize(stat_font_cxt, stat_font_base_size);
}

static void DrawHandler(void *ds_event, void *param, int action) {

    switch(action) {
        case EVENT_ACTION_RENDER:
            if(is_fullscreen) {
                pvr_list_begin(PVR_LIST_TR_POLY);
                dreameye_preview_frame();
                render_stats();
                pvr_list_finish();
            }
            break;
        case EVENT_ACTION_RENDER_POST:
            if(!is_fullscreen && !ScreenIsHidden() && !ConsoleIsVisible()) {
                dreameye_preview_frame();
                render_stats();
            }
            break;
        case EVENT_ACTION_UPDATE:
            break;
        default:
            break;
    }
}

static void onPreviewClick(void) {

    if(is_fullscreen) {
        ds_sfx_play(DS_SFX_CLICK);

        if(back_to_window) {
            is_fullscreen = 0;
            frame_x = frame_x_old;
            frame_y = frame_y_old;
            EnableScreen();
            GUI_Enable();
        }
        else {
            dreameye_preview_shutdown(dreameye);
        }
    }
    else {
        int mx = 0;
        int my = 0;

        SDL_GetMouseState(&mx, &my);

        if(mx >= frame_x && mx <= frame_x + (frame_txr_width * frame_scale) &&
            my >= frame_y && my <= frame_y + (frame_txr_height * frame_scale)) {
            ds_sfx_play(DS_SFX_CLICK);

            is_fullscreen = 1;
            back_to_window = 1;
            frame_x_old = frame_x;
            frame_y_old = frame_y;
            frame_x = 0.0f;
            frame_y = 0.0f;

            DisableScreen();
            GUI_Disable();
        }
    }
}

static void EventHandler(void *ds_event, void *param, int action) {

    SDL_Event *event = (SDL_Event *) param;
    static int inside_video_area = 0;

    switch(event->type) {
        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case SDL_DC_B:
                case SDL_DC_A:
                    if(is_fullscreen) {
                        onPreviewClick();
                    }
                    break;
                case SDL_DC_Y:
                    if(is_fullscreen || inside_video_area) {
                        if(!show_stat) {
                            load_stat_font();
                        }
                        show_stat = !show_stat;
                    }
                    break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(event->button.button == SDL_BUTTON_LEFT ||
                event->button.button == SDL_BUTTON_RIGHT) {
                onPreviewClick();
            }
            break;
        case SDL_MOUSEMOTION:
            int mx = event->motion.x, my = event->motion.y;
            if(mx >= frame_x && mx <= frame_x + (frame_txr_width * frame_scale) &&
                my >= frame_y && my <= frame_y + (frame_txr_height * frame_scale)) {
                if(!inside_video_area) {
                    ds_sfx_play(DS_SFX_CLICK2);
                    inside_video_area = 1;
                }
            }
            else {
                inside_video_area = 0;
            }
        default:
            break;
    }
}

static int setup_pvr(void) {

    plx_txr = (plx_texture_t *) malloc(sizeof(plx_texture_t));

    if(!plx_txr) {
        ds_printf("Failed to allocate memory!\n");
        return -1;
    }

    pvr_txr = pvr_mem_malloc(pvr_txr_width * pvr_txr_height * 2);

    if(!pvr_txr) {
        ds_printf("Failed to allocate PVR memory!\n");
        return -1;
    }

    /* Setup YUV converter. */
    PVR_SET(PVR_YUV_ADDR, (((unsigned int)pvr_txr) & 0xffffff));
    /* Divide PVR texture width and texture height by 16 and subtract 1. */
    uint8_t yuv_format = (frame_format == DREAMEYE_FRAME_FMT_YUYV422) ? 
                     PVR_YUV_FORMAT_YUV422 : PVR_YUV_FORMAT_YUV420;
    PVR_SET(PVR_YUV_CFG, (yuv_format << 24) |
                         (PVR_YUV_MODE_SINGLE << 16) |
                         (((pvr_txr_height / 16) - 1) << 8) |
                         ((pvr_txr_width / 16) - 1));
    /* Need to read once */
    PVR_GET(PVR_YUV_CFG);

    plx_txr->ptr = pvr_txr;
    plx_txr->w = pvr_txr_width;
    plx_txr->h = pvr_txr_height;
    plx_txr->fmt = PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED;
    plx_fill_contexts(plx_txr);
    plx_txr_setfilter(plx_txr, PVR_FILTER_BILINEAR);

    return 0;
}

// FIXME: It's works only for 160x120
static void yuv420p_to_yuv422_dma(uint8_t *src) {
    const int frame_width = frame_txr_width;
    const int frame_height = frame_txr_height;

    uint8_t *y_plane = src;
    uint8_t *u_plane = src + (frame_width * frame_height);
    uint8_t *v_plane = u_plane + (frame_width * frame_height / 4);

    const int y_stride = frame_width;
    const int u_stride = frame_width / 2;
    const int v_stride = frame_width / 2;

    const size_t dma_buffer_size = (pvr_txr_width * pvr_txr_height * 3) / 2;
    uint8_t *dst = yuv_dma_buffer;
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
    pvr_dma_yuv_conv(yuv_dma_buffer, dma_buffer_size, 1, NULL, NULL);
}

static void yuv420p_to_yuv422_sq(uint8_t *src) {
    int i, j, index, x_blk, y_blk;
    size_t dummies = (BYTE_SIZE_FOR_16x16_BLOCK_420 *
        ((pvr_txr_width >> 4) - (frame_txr_width >> 4))) >> 5;

    uint32_t *db = (uint32_t *)SQ_MASK_DEST_ADDR(PVR_TA_YUV_CONV);
    uint8_t *u_block = (uint8_t *)SQ_MASK_DEST_ADDR(PVR_TA_YUV_CONV);
    uint8_t *v_block = (uint8_t *)SQ_MASK_DEST_ADDR(PVR_TA_YUV_CONV + 64);
    uint8_t *y_block = (uint8_t *)SQ_MASK_DEST_ADDR(PVR_TA_YUV_CONV + 128);

    uint8_t *y_plane = src;
    uint8_t *u_plane = src + (frame_txr_width * frame_txr_height);
    uint8_t *v_plane = src + (frame_txr_width * frame_txr_height) +
        (frame_txr_width * frame_txr_height / 4);

    sq_lock((void *)PVR_TA_YUV_CONV);

    for(y_blk = 0; y_blk < frame_txr_height; y_blk += 16) {
        for(x_blk = 0; x_blk < frame_txr_width; x_blk += 16) {

            /* U data for 16x16 pixels */
            for(i = 0; i < 8; ++i) {
                index = (y_blk / 2 + i) * (frame_txr_width / 2) + 
                        (x_blk / 2);
                *((uint64_t *)&u_block[i * 8]) = *((uint64_t *)&u_plane[index]);
                if(((i + 1) & 3) == 0) {
                    sq_flush(&u_block[i * 8]);
                }
            }

            /* V data for 16x16 pixels */
            for(i = 0; i < 8; ++i) {
                index = (y_blk / 2 + i) * (frame_txr_width / 2) + 
                        (x_blk / 2);
                *((uint64_t *)&v_block[i * 8]) = *((uint64_t *)&v_plane[index]);
                if(((i + 1) & 3) == 0) {
                    sq_flush(&v_block[i * 8]);
                }
            }

            /* Y data for 4 (8x8 pixels) */
            for(i = 0; i < 4; ++i) {
                for(j = 0; j < 8; ++j) {
                    index = (y_blk + j + (i / 2 * 8)) * frame_txr_width + 
                             x_blk + (i % 2 * 8);
                    *((uint64_t *)&y_block[i * 64 + j * 8]) = 
                        *((uint64_t *)&y_plane[index]);
                    if(((j + 1) & 3) == 0) {
                        sq_flush(&y_block[i * 64 + j * 8]);
                    }
                }
            }
        }
        /* Send dummies if frame texture width doesn't match pvr texture width */
        for(i = 0; i < dummies; ++i) {
            db[i] = db[i + 1] = db[i + 2] = db[i + 3] = 
                db[i + 4] = db[i + 5] = db[i + 6] = db[i + 7] = 0;
            sq_flush(&db[i]);
        }
    }

    sq_unlock();
}

// TODO: Add DMA version
static void yuyv422_to_yuv422_sq(uint8_t *src) {
    int i, j, x_blk, y_blk;

    uint8_t u_block[128] __attribute__((aligned(32)));
    uint8_t v_block[128] __attribute__((aligned(32)));
    uint8_t y_block[256] __attribute__((aligned(32)));

    for(y_blk = 0; y_blk < frame_txr_height; y_blk += 16) {
        for(x_blk = 0; x_blk < frame_txr_width; x_blk += 16) {

            for(i = 0; i < 16; ++i) {
                int row = y_blk + i;
                for(j = 0; j < 8; ++j) {
                    int x_pos = x_blk + j * 2;
                    int yuyv_idx = (row * frame_txr_width + x_pos) * 2;
                    u_block[i * 8 + j] = src[yuyv_idx + 1];
                }
            }

            for(i = 0; i < 16; ++i) {
                int row = y_blk + i;
                for(j = 0; j < 8; ++j) {
                    int x_pos = x_blk + j * 2;
                    int yuyv_idx = (row * frame_txr_width + x_pos) * 2;
                    v_block[i * 8 + j] = src[yuyv_idx + 3];
                }
            }

            for(i = 0; i < 4; ++i) {
                for(j = 0; j < 8; ++j) {
                    int row = y_blk + j + (i / 2 * 8);
                    for(int k = 0; k < 8; k += 2) {
                        int x_pos = x_blk + k + (i % 2 * 8);
                        int yuyv_idx = (row * frame_txr_width + x_pos) * 2;
                        y_block[i * 64 + j * 8 + k] = src[yuyv_idx];
                        y_block[i * 64 + j * 8 + k + 1] = src[yuyv_idx + 2];
                    }
                }
            }

            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)u_block, 64, PVR_DMA_YUV);
            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)v_block, 64, PVR_DMA_YUV);
            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)y_block, 128, PVR_DMA_YUV);
            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)(u_block + 64), 64, PVR_DMA_YUV);
            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)(v_block + 64), 64, PVR_DMA_YUV);
            pvr_sq_load((void *)PVR_TA_YUV_CONV, (void *)(y_block + 128), 128, PVR_DMA_YUV);
        }

        pvr_sq_set32((void *)PVR_TA_YUV_CONV, 0, 
                     BYTE_SIZE_FOR_16x16_BLOCK_422 * 
                     ((pvr_txr_width >> 4) - (frame_txr_width >> 4)), PVR_DMA_YUV);
    }
}

static void *capture_thread(void *param) {
    uint8_t *frame = NULL;
	int size = 0, res;

    while(capturing) {

        if(!is_fullscreen && ScreenIsHidden()) {
            thd_sleep(50);
            continue;
        }

        res = dreameye_req_video_frame(dreameye);

        if(frame_callback != NULL && frame) {
            frame_callback(dreameye, frame, size);
            free(frame);
        }

        if(res != MAPLE_EOK) {
            thd_pass();
            continue;
        }

        res = dreameye_get_video_frame(dreameye, &frame, &size);

        if(res != MAPLE_EOK) {
            thd_pass();
            continue;
        }

        if(frame) {
            LockVideo();

            if(!got_frame) {
                got_frame = 1;
            }
            switch(frame_format) {
                case DREAMEYE_FRAME_FMT_YUV420P:
                    if(frame_txr_width == 160) {
                        yuv420p_to_yuv422_dma(frame);
                    }
                    else {
                        yuv420p_to_yuv422_sq(frame);
                    }
                    break;
                case DREAMEYE_FRAME_FMT_YUYV422:
                    yuyv422_to_yuv422_sq(frame);
                    break;
                default:
                    got_frame = 0;
                    break;
            }
            if(show_stat) {
                if(fps_start_time == 0) {
                    fps_start_time = timer_ms_gettime64();
                }
                frame_count_for_fps++;
                uint64_t now = timer_ms_gettime64();
                uint64_t diff = now - fps_start_time;
                if(diff >= 100) {
                    current_fps = (float)frame_count_for_fps * 1000.0f / (float)diff;

                    if(display_fps == 0.0f) {
                        display_fps = current_fps;
                    }
                    else {
                        display_fps = display_fps * 0.9f + current_fps * 0.1f;
                    }
                    fps_start_time = now;
                    frame_count_for_fps = 0;
                }
            }
            UnlockVideo();

            if(frame_callback == NULL) {
                free(frame);
            }
        }
    }
    return NULL;
}

int dreameye_preview_init(maple_device_t *dev, dreameye_preview_t *params) {
    int rs;

    if(dreameye) {
        if(!capturing) {
            return -1;
        }
        dreameye_preview_shutdown(dreameye);
    }
	if(!dev) {
		ds_printf("DS_ERROR: Couldn't find any attached devices.\n");
		return -1;
	}
    if(!params) {
        ds_printf("DS_ERROR: Params is required.\n");
        return -1;
    }

    dreameye = dev;
    frame_callback = params->callback;
    got_frame = 0;
    back_to_window = 0;
    is_fullscreen = params->fullscreen;
    frame_scale = (float)params->scale;
    frame_x = (float)params->x;
    frame_y = (float)params->y;

    switch(params->isp_mode) {
        case DREAMEYE_ISP_MODE_QSIF:
            frame_txr_width = 160;
            frame_txr_height = 120;
            pvr_txr_width = 256;
            pvr_txr_height = 128;
            break;
        case DREAMEYE_ISP_MODE_SIF:
            frame_txr_width = 320;
            frame_txr_height = 240;
            pvr_txr_width = 512;
            pvr_txr_height = 256;
            break;
        case DREAMEYE_ISP_MODE_VGA:
            frame_txr_width = 640;
            frame_txr_height = 480;
            pvr_txr_width = 1024;
            pvr_txr_height = 512;
            break;
        default:
            ds_printf("DS_ERROR: Unsupported ISP mode: %d\n", params->isp_mode);
            return -1;
    }

    switch(params->bpp) {
        case 12:
            frame_format = DREAMEYE_FRAME_FMT_YUV420P;
            break;
        case 16:
            frame_format = DREAMEYE_FRAME_FMT_YUYV422;
            break;
        default:
            ds_printf("DS_ERROR: Unsupported bits per pixel: %d\n", params->bpp);
            return -1;
    }

    rs = dreameye_setup_video_camera(dreameye, params->isp_mode, frame_format);

    if (rs != MAPLE_EOK) {
        ds_printf("DS_ERROR: Camera setup failed\n");
        return -1;
    }

    rs = setup_pvr();

    if(rs < 0) {
        ds_printf("DS_ERROR: PVR setup failed\n");
        return -1;
    }

    const size_t dma_buffer_size = (pvr_txr_width * pvr_txr_height * 3) / 2;
    yuv_dma_buffer = aligned_alloc(32, dma_buffer_size);

    if(!yuv_dma_buffer) {
        ds_printf("DS_ERROR: Can't allocate YUV DMA buffer\n");
        return -1;
    }

    input_event = AddEvent(
        "DreamEye_Input",
        EVENT_TYPE_INPUT,
        EVENT_PRIO_DEFAULT,
        EventHandler,
        NULL
    );
    video_event = AddEvent(
        "DreamEye_Video",
        EVENT_TYPE_VIDEO,
        EVENT_PRIO_DEFAULT,
        DrawHandler,
        NULL
    );

    capturing = 1;
    thread = thd_create(0, capture_thread, NULL);

    if(is_fullscreen) {
        DisableScreen();
        GUI_Disable();
    }

    return 0;
}

void dreameye_preview_shutdown(maple_device_t *dev) {
    if(!dreameye || !capturing) {
        return;
    }
    capturing = 0;

    RemoveEvent(input_event);
    RemoveEvent(video_event);

    thd_join(thread, NULL);

    dreameye_stop_video_camera(dev ? dev : dreameye);

    if(stat_font_cxt) {
        plx_fcxt_destroy(stat_font_cxt);
        stat_font_cxt = NULL;
    }
    if(stat_font) {
        plx_font_destroy(stat_font);
        stat_font = NULL;
    }
    if(is_fullscreen) {
        EnableScreen();
        GUI_Enable();
    }

    pvr_mem_free(pvr_txr);
    free(plx_txr);

    if(yuv_dma_buffer) {
        free(yuv_dma_buffer);
        yuv_dma_buffer = NULL;
    }

    dreameye = NULL;
}

