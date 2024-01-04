/* DreamShell ##version##

   preview.c - dreameye preview
   Copyright (C) 2023, 2024 SWAT

*/          

#include "ds.h"
#include "drivers/dreameye.h"

/* u_block + v_block + y_block = 64 + 64 + 256 = 384 */
#define BYTE_SIZE_FOR_16x16_BLOCK 384

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

static pvr_ptr_t pvr_txr;
static plx_texture_t *plx_txr;
static maple_device_t *dreameye;
// static semaphore_t yuv_done = SEM_INITIALIZER(0);

static int capturing = 0;
static int got_frame = 0;
static kthread_t *thread = NULL;
static Event_t *input_event = NULL;
static Event_t *video_event = NULL;


static void dreameye_preview_frame() {

    const uint32_t color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    const float width_ratio = (float)frame_txr_width / pvr_txr_width;
    const float height_ratio = (float)frame_txr_height / pvr_txr_height;
    const float native_width = 640.0f;
    const float native_height = 480.0f;
    const float z = 1.0f;

    if(!got_frame) {
        // TODO: Show waiting picture
        return;
    }

    plx_mat3d_identity();
    plx_mat_identity();
    plx_mat3d_apply_all();

    plx_mat3d_rotate(0.0f, 1.0f, 0.0f, 0.0f);
    plx_mat3d_rotate(0.0f, 0.0f, 1.0f, 0.0f);
    plx_mat3d_rotate(0.0f, 0.0f, 0.0f, 1.0f);
    plx_mat3d_translate(0, 0, 0);

	plx_cxt_texture(plx_txr);
	plx_cxt_culling(PLX_CULL_NONE);
	plx_cxt_send(PLX_LIST_TR_POLY);

	plx_vert_ifpm3(PLX_VERT, 0.0f, 0.0f, z, color, 0.0f, 0.0f);
	plx_vert_ifpm3(PLX_VERT, native_width, 0.0f, z, color, width_ratio, 0.0f);
	plx_vert_ifpm3(PLX_VERT, 0.0f, native_height, z, color, 0.0f, height_ratio);
	plx_vert_ifpm3(PLX_VERT_EOS, native_width, native_height, z, color, width_ratio, height_ratio);
}

static void DrawHandler(void *ds_event, void *param, int action) {

    switch(action) {
        case EVENT_ACTION_RENDER:
            break;
        case EVENT_ACTION_RENDER_HW:
            pvr_list_begin(PVR_LIST_TR_POLY);
            dreameye_preview_frame();
            pvr_list_finish();
            break;
        case EVENT_ACTION_UPDATE:
            break;
        default:
            break;
    }
}

static void EventHandler(void *ds_event, void *param, int action) {
    
    SDL_Event *event = (SDL_Event *) param;

    switch(event->type) {
        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case 1:
                case 2:
                case 5:
                case 6:
                // case 3:
                    dreameye_preview_shutdown();
                    break;
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
    PVR_SET(PVR_YUV_CFG, (PVR_YUV_FORMAT_YUV420 << 24) |
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

static void yuv420p_to_yuv422(uint8_t *src) {
    int i, j, index, x_blk, y_blk;
    size_t dummies = (BYTE_SIZE_FOR_16x16_BLOCK *
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
                *((uint64_t*)&u_block[i * 8]) = *((uint64_t*)&u_plane[index]);
                if((i + 1) % 4 == 0) {
                    sq_flush(&u_block[i * 8]);
                }
            }

            /* V data for 16x16 pixels */
            for(i = 0; i < 8; ++i) {
                index = (y_blk / 2 + i) * (frame_txr_width / 2) + 
                        (x_blk / 2);
                *((uint64_t*)&v_block[i * 8]) = *((uint64_t*)&v_plane[index]);
                if((i + 1) % 4 == 0) {
                    sq_flush(&v_block[i * 8]);
                }
            }

            /* Y data for 4 (8x8 pixels) */
            for(i = 0; i < 4; ++i) {
                for(j = 0; j < 8; ++j) {
                    index = (y_blk + j + (i / 2 * 8)) * frame_txr_width + 
                             x_blk + (i % 2 * 8);
                    *((uint64_t*)&y_block[i * 64 + j * 8]) = 
                        *((uint64_t*)&y_plane[index]);
                    if((j + 1) % 4 == 0) {
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
    // sem_wait(&yuv_done);
}


static void *capture_thread(void *param) {
    uint8_t *frame = NULL;
	int size = 0, res, num = 1;

    while(capturing) {

        do {
            num ^= 1;
            res = dreameye_get_video_frame(dreameye, num, &frame, &size);
        } while(res != MAPLE_EOK && capturing);

        if(frame) {
            LockVideo();
            yuv420p_to_yuv422(frame);
            if(!got_frame) {
                got_frame = 1;
            }
            UnlockVideo();
            free(frame);
        }
    }
    return NULL;
}
/*
static void asic_yuv_evt_handler(uint32 code) {
    (void)code;
    sem_signal(&yuv_done);
    dbglog(DBG_DEBUG, "%s: %d\n", __func__, sem_count(&yuv_done));
}*/

int dreameye_preview_init(maple_device_t *dev, int isp_mode) {
    int rs;

    if(dreameye) {
        return 0;
    }

    dreameye = dev;
    got_frame = 0;

	if(!dreameye) {
		ds_printf("DS_ERROR: Couldn't find any attached devices, bailing out.\n");
		return -1;
	}

    switch(isp_mode) {
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
            ds_printf("DS_ERROR: Unsupported ISP mode: %d\n", isp_mode);
            return -1;
    }

    rs = dreameye_setup_video_camera(dreameye, isp_mode, DREAMEYE_FRAME_FMT_YUV420P);

    if (rs != MAPLE_EOK) {
        ds_printf("DS_ERROR: Camera setup failed\n");
        return -1;
    }

    rs = setup_pvr();

    if(rs < 0) {
        ds_printf("DS_ERROR: PVR setup failed\n");
        return -1;
    }

	// asic_evt_set_handler(ASIC_EVT_PVR_YUV_DONE, asic_yuv_evt_handler);
	// asic_evt_enable(ASIC_EVT_PVR_YUV_DONE, ASIC_IRQ_DEFAULT);

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
    DisableScreen();
    GUI_Disable();

    return 0;
}

void dreameye_preview_shutdown(void) {
    if(!dreameye) {
        return;
    }
    capturing = 0;
    thd_join(thread, NULL);

	// asic_evt_disable(ASIC_EVT_PVR_YUV_DONE, ASIC_IRQ_DEFAULT);
	// asic_evt_set_handler(ASIC_EVT_PVR_YUV_DONE, NULL);

    dreameye_stop_video_camera(dreameye);
    dreameye = NULL;

    RemoveEvent(video_event);
    RemoveEvent(input_event);

    EnableScreen();
    GUI_Enable();

    pvr_mem_free(pvr_txr);
    free(plx_txr);
}
