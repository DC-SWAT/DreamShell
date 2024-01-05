/* DreamShell ##version##

   dreameye.c - dreameye driver addons
   Copyright (C) 2015, 2023 SWAT

*/          

#include <ds.h>
#include <assert.h>
#include <kos/genwait.h>
#include <drivers/dreameye.h>
#include "cis_isp_regs.h"

#define JANGGU_FRAME_HEADER_SIZE 4
#define JANGGU_FRAME_DATA_SIZE 960
#define JANGGU_FRAME_DATA_SIZE_COMPRESSED 1004

#define JANGGU_REG_FORMAT 0
#define JANGGU_REG_HEIGHT 4

#define JANGGU_FMT_COMPRESSED   0
#define JANGGU_FMT_UNCOMPRESSED 1
#define JANGGU_FMT_YUV420DE     ((0 << 1) | (1 << 6))
#define JANGGU_FMT_YUYV422      ((0 << 1) | (0 << 6))
#define JANGGU_FMT_UNK24BPP     ((1 << 1) | (1 << 6))
#define JANGGU_FMT_UNK32BPP     ((1 << 1) | (0 << 6))
#define JANGGU_FMT_UNK7         ((1 << 7))

// #define MAPLE_SPEED_2MBPS 0x0000
#define MAPLE_SPEED_1MBPS 0x0100
#define MAPLE_SPEED_4MBPS 0x0200
#define MAPLE_SPEED_8MBPS 0x0300

#define DREAMEYE_MAPLE_SPEED_2MBPS 0xD007
#define DREAMEYE_MAPLE_SPEED_4MBPS 0xA00F

#define DREAMEYE_DEVS_PER_FRAME_2MBPS 3
#define DREAMEYE_DEVS_PER_FRAME_4MBPS 5

#define DREAMEYE_DEVS_PER_FRAME DREAMEYE_DEVS_PER_FRAME_4MBPS

typedef struct dreameye_register {
    uint8_t reg;
    uint8_t param;
    uint16_t val;
} dreameye_register_t;

static dreameye_state_ext_t *first_state = NULL;
static int dreameye_send_get_video_frame(maple_device_t *dev, dreameye_state_ext_t *state);

/* Regs dump from Visual Park */
static dreameye_register_t ic_regs[85] = {
    // { DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_MODE_B, 0x05 },
    { DREAMEYE_COND_REG_CIS, CIS_THBU, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_THBL, 10 },
    { DREAMEYE_COND_REG_CIS, CIS_TVBU, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_TVBL, 0x0a },
    { DREAMEYE_COND_REG_CIS, CIS_TMCD, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_ARLV, 0x2D },
    { DREAMEYE_COND_REG_CIS, CIS_ARCG, 0x22 },
    { DREAMEYE_COND_REG_CIS, CIS_AGCG, 0x2D },
    { DREAMEYE_COND_REG_CIS, CIS_ABCG, 0x22 },
    { DREAMEYE_COND_REG_CIS, CIS_APBV, 0x00 },
    { DREAMEYE_COND_REG_CIS, 0x54, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_OFSR, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_OFSG, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_OFSB, 0 },
    { DREAMEYE_COND_REG_CIS, 2, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_BASE_ENB, 31 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA11, 0x45 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA12, 0xF6 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA13, 4 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA21, 0xF8 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA22, 0x5D },
    { DREAMEYE_COND_REG_ISP, ISP_CMA23, 0xEB },
    { DREAMEYE_COND_REG_ISP, ISP_CMA31, 3 },
    { DREAMEYE_COND_REG_ISP, ISP_CMA32, 0xED },
    { DREAMEYE_COND_REG_ISP, ISP_CMA33, 0x51 },
    { DREAMEYE_COND_REG_ISP, ISP_OFSR, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_OFSG, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_OFSB, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_GAIN_TOP, 0x3E },
    { DREAMEYE_COND_REG_ISP, ISP_GAIN_BOTTOM, 0x17 },
    { DREAMEYE_COND_REG_ISP, ISP_AWB_CONTROL, 0xBC },
    { DREAMEYE_COND_REG_ISP, ISP_AWB_LOCK, 0xCB },
    { DREAMEYE_COND_REG_ISP, ISP_AE_CONTROL, 0xD4 },
    { DREAMEYE_COND_REG_ISP, ISP_AE_LOCK, 0xC5 },
    { DREAMEYE_COND_REG_ISP, ISP_Y_TARGET, 0x80 },
    { DREAMEYE_COND_REG_ISP, ISP_RESET_LEVEL, 5 },
    { DREAMEYE_COND_REG_ISP, ISP_AWB_CR_TARGET, 0x80 },
    { DREAMEYE_COND_REG_ISP, ISP_AWB_CB_TARGET, 0x80 },
    { DREAMEYE_COND_REG_ISP, ISP_EDGE_CONTROL, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_HSYNC_COUNT, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_HISTO_MODE, 14 },
    { DREAMEYE_COND_REG_ISP, ISP_FIXED_FACTOR, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START0, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START1, 7 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START2, 13 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START3, 19 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START4, 0x2B },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START5, 0x3D },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START6, 0x4E },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START7, 0x8E },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_START8, 0xC8 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE0, 14 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE1, 23 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE2, 21 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE3, 19 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE4, 17 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE5, 17 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE6, 15 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE7, 14 },
    { DREAMEYE_COND_REG_ISP, ISP_GMA_SLOPE8, 13 },
    { DREAMEYE_COND_REG_CIS, CIS_FRSU, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_FRSL, 1 },
    { DREAMEYE_COND_REG_CIS, CIS_FCSU, 0 },
    { DREAMEYE_COND_REG_CIS, CIS_FCSL, 1 },
    { DREAMEYE_COND_REG_CIS, CIS_FWHU, 1 },
    { DREAMEYE_COND_REG_CIS, CIS_FWHL, 0xE2 },
    { DREAMEYE_COND_REG_CIS, CIS_FWWU, 2 },
    { DREAMEYE_COND_REG_CIS, CIS_FWWL, 128 },
    // { DREAMEYE_COND_REG_ISP, ISP_OP_MODE, 0 },
    // { DREAMEYE_COND_REG_ISP, ISP_SCALE_UPPER, 1 },
    // { DREAMEYE_COND_REG_ISP, ISP_SCALE_LOWER, 64 },
    // { DREAMEYE_COND_REG_JANGGU, 4, 0x78 },
    // { DREAMEYE_COND_REG_JANGGU, 5, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_START, 0x1E },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_SIDE, 0x78 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_CENTER, 20 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_START, 20 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_SIDE, 35 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_CENTER, 10 },
    // { DREAMEYE_COND_REG_ISP, ISP_OUT_FORM, 0x39 },
    // { DREAMEYE_COND_REG_JANGGU, 0, 0xC1 },
    // { DREAMEYE_COND_REG_JANGGU, 1, 0x33 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_UPPER, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_MIDDLE, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_LOWER, 48 },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_UPPER, 0x2D },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_MIDDLE, 0xC6 },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_LOWER, 0xC0 },
    { DREAMEYE_COND_REG_CIS, CIS_TITU, 9 },
    { DREAMEYE_COND_REG_CIS, CIS_TITM, 0x27 },
    { DREAMEYE_COND_REG_CIS, CIS_TITL, 0xC0 },
    // { DREAMEYE_COND_REG_JANGGU, 2, 1 },
    // { DREAMEYE_COND_REG_JANGGU, 2, 2 },
    // { DREAMEYE_COND_REG_JANGGU, 2, 16 },
    // { DREAMEYE_COND_REG_JANGGU, 6, 4 },
    // { DREAMEYE_COND_REG_JANGGU, 7, 0 },
    // { DREAMEYE_COND_MAPLE_BITRATE, 0x90, 0x0FA0 },
    // { DREAMEYE_COND_MAPLE_BITRATE, 0x90, 64 },
    // { DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0xB3 },
    { DREAMEYE_COND_REG_ISP, ISP_Y_TARGET, 0x80 }
};


void hexDump(char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        dbglog(DBG_DEBUG, "%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                dbglog(DBG_DEBUG, "  %s\n", buff);

            // Output the offset.
            dbglog(DBG_DEBUG, "  %04x ", i);
        }

        // Now the hex code for the specific character.
        dbglog(DBG_DEBUG, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        dbglog(DBG_DEBUG, "   ");
        i++;
    }

    // And print the final ASCII bit.
    dbglog(DBG_DEBUG, "  %s\n", buff);
}

static void yuv420de_to_nv21(uint8_t *dest, const uint8_t *src, int width, int height) {
    int w, h;

    for (h = 0; h < height / 2; ++h) {
        memcpy(&dest[h * 2 * width], &src[h * width * 3], width);
        for (w = 0; w < width; ++w) {
            dest[(h * 2 + 1) * width + w] = src[(h * 3 + 1) * width + w * 2];
        }
        for (w = 0; w < width; ++w) {
            dest[(height + h) * width + w] = src[(h * 3 + 1) * width + w * 2 + 1];
        }
    }
}

static void yuv420de_to_yuv420p(uint8_t *dest, const uint8_t *src, int width, int height) {
    uint8_t *destY = dest;
    uint8_t *destU = dest + (width * height);
    uint8_t *destV = dest + (width * height) + (width * height / 4);
    int i, n;

    for (i = 0; i < height; i += 2) {
        memcpy(destY, src, width);
        destY += width;
        src += width;
        for (n = 0; n < width / 2; n++) {
            *destY++ = *src++;
            *destU++ = *src++;
            *destY++ = *src++;
            *destV++ = *src++;
        }
    }
}

static void convert_frame(dreameye_state_ext_t *de) {
    uint8_t *buf;

    /* TODO: Conversion when parsing packets */
    if(de->format == DREAMEYE_FRAME_FMT_YUV420P) {
        buf = (uint8_t *)memalign(32, de->frame_size);
        if (buf) {
            yuv420de_to_yuv420p(buf, de->img_buf, de->width, de->height);
            free(de->img_buf);
            de->img_buf = buf;
        }
    }
    else if(de->format == DREAMEYE_FRAME_FMT_NV21) {
        buf = (uint8_t *)memalign(32, de->frame_size);
        if (buf) {
            yuv420de_to_nv21(buf, de->img_buf, de->width, de->height);
            free(de->img_buf);
            de->img_buf = buf;
        }
    }
}

static int dreameye_get_video_frame_part(maple_device_t *dev) {
    maple_device_t *subdev;
    int i, rv, try_cnt = 0;
    const int offset = (DREAMEYE_DEVS_PER_FRAME == 5 ? 0 : 1);

    do {
        /* Send out requests to devices. */
        for(i = 0; i < DREAMEYE_DEVS_PER_FRAME; ++i) {

            subdev = maple_enum_dev(dev->port, i + offset);
            rv = dreameye_send_get_video_frame(subdev, first_state);

            if(rv == MAPLE_EOK) {
                ++first_state->img_transferring;
            }
        }

        if(first_state->img_transferring == 1) {
            thd_pass();
        }

    } while(first_state->img_transferring == 1 && ++try_cnt < 20);

    if(first_state->img_transferring == 1) {
        first_state->img_transferring = -1;
        return MAPLE_EFAIL;
    }

    return MAPLE_EOK;
}

static void dreameye_get_video_frame_cb(maple_frame_t *frame) {
    maple_response_t *resp;
    uint32_t *respbuf32;
    uint8_t *respbuf8, *packet;
    int len, part;
    int bits, pixels, info, packet_len;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    if(frame->dev == NULL || first_state == NULL
        || first_state->img_transferring <= 0) {
        return;
    }

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf32 = (uint32_t *)resp->data;
    respbuf8 = (uint8_t *)resp->data;

    // hexDump("resp", resp->data, resp->data_len * 4);

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
        first_state->img_transferring = -1;
        return;
    }

    if(respbuf32[0] != MAPLE_FUNC_CAMERA) {
        first_state->img_transferring = -1;
        dbglog(DBG_ERROR, "%s: bad func: 0x%08lx\n", __func__, respbuf32[0]);
        return;
    }

    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "%s: error 0x%02X 0x%02X 0x%02X\n", 
                __func__, respbuf8[5], respbuf8[6], respbuf8[7]);
        return;
    }

    len = (resp->data_len - 3) * 4;
    part = respbuf8[4];
    packet = respbuf8 + 12;

    if(!first_state->compressed) {
        /* Parsing JangGu packet header */
        bits = packet[3] + (packet[2] << 8);
        pixels = packet[1] + ((packet[0] & 0x3f) << 8);
        info = (packet[0] & 0xc0) >> 6;
        packet_len = ((bits + 47) >> 4) << 1;

        if(len != packet_len || (part == 0x80 && info != 2) ||
            (part == 0x00 && info != part) || (part == 0x40 && info != 1)) {
            dbglog(DBG_ERROR, "%s: len=%d part=0x%02X | bits=%d pixels=%d bpp=%d info=%d plen=%d\n",
                __func__, len, part, bits, pixels, bits / pixels, info, packet_len);
            first_state->img_transferring = -1;
            return;
        }

        /* Copy only YUV data without header */
        len -= JANGGU_FRAME_HEADER_SIZE;
        packet += JANGGU_FRAME_HEADER_SIZE;
    }

    if(first_state->img_size + len > first_state->frame_size) {
        dbglog(DBG_ERROR, "%s: Unexpected frame size, received %d, but expected %d\n",
                __func__, first_state->img_size + len,
                first_state->frame_size);
        first_state->img_transferring = -1;
        return;
    }

    memcpy(first_state->img_buf + first_state->img_size, packet, len);
    first_state->img_size += len;

    /* Check if we're done. */
    if(part == 0x40) {
        if(!first_state->compressed && first_state->transfer_count > 0) {
            dbglog(DBG_ERROR, "%s: Unexpected end of transfer, missing %d packets.\n",
                __func__, first_state->transfer_count);
            first_state->img_transferring = -1;
        } else {
            first_state->img_transferring = 0;
        }
        return;
    }

    /* FIXME: Very stupid things right here, but it's sync frames */
    if(first_state->width == 320)
        timer_spin_sleep(1);
    else
        timer_spin_sleep(2);
    /* End of FIXME */

    if(--first_state->img_transferring == 1) {
        dreameye_get_video_frame_part(frame->dev);
    }
}

static int dreameye_send_get_video_frame(maple_device_t *dev, dreameye_state_ext_t *state) {
    uint32 *send_buf;

    if(first_state->transfer_count == 0) {
        return MAPLE_EOK;
    }

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    --first_state->transfer_count;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_SUBCOMMAND_IMAGEREQ | (state->img_number << 8);
    dev->frame.cmd = MAPLE_COMMAND_CAMCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_video_frame_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

int dreameye_get_video_frame(maple_device_t *dev, uint8_t fb_num, uint8_t **data,
                       int *img_sz) {
    dreameye_state_ext_t *de;

    assert(dev != NULL);
    assert(dev->unit == 1);

    de = (dreameye_state_ext_t *)dev->status;
    first_state = de;

    de->img_transferring = 1;
    de->img_size = 0;
    de->img_number = fb_num;

    if(de->compressed) {
        de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE_COMPRESSED;
    } else {
        de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE;
    }

    /* Allocate space for the largest possible image that could fit in that
       number of transfers. */
    de->img_buf = (uint8_t *)memalign(32, de->frame_size);

    if(!de->img_buf) {
        goto fail;
    }

    dreameye_get_video_frame_part(dev);

    while(de->img_transferring > 0) {
        thd_pass();
    }

    convert_frame(de);

    if(de->img_transferring == 0) {
        *data = de->img_buf;
        *img_sz = de->img_size;
        first_state = NULL;
        de->img_buf = NULL;
        de->img_size = 0;
        de->transfer_count = 0;
        return MAPLE_EOK;
    }

    /* If we get here, something went wrong. */
    if(de->img_buf != NULL) {
        free(de->img_buf);
    }

fail:
    *data = NULL;
    *img_sz = 0;
    first_state = NULL;
    de->img_transferring = 0;
    de->img_buf = NULL;
    de->img_size = 0;
    de->transfer_count = 0;

    return MAPLE_EFAIL;
}

static void dreameye_get_param_cb(maple_frame_t *frame) {
    dreameye_state_ext_t *de;
    maple_response_t *resp;
    uint32_t *respbuf32;
    uint8_t *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf32 = (uint32_t *)resp->data;
    respbuf8 = (uint8_t *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
//        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }

    if(respbuf32[0] != MAPLE_FUNC_CAMERA) {
        return;
    }

    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dreameye_state_ext_t *de = (dreameye_state_ext_t *)frame->dev->status;
        dbglog(DBG_ERROR, "%s: error 0x%02X 0x%02X 0x%02X value 0x%04X\n", 
                __func__, respbuf8[5], respbuf8[6], respbuf8[7], de->value);
    }

    /* Update the status that was requested. */
    if(frame->dev) {
        assert((resp->data_len) == 3);
        assert(respbuf8[4] == 0xD0);
        assert(respbuf8[5] == 0x00);
//        assert(respbuf8[8] == DREAMEYE_GETCOND_***);

        /* Update the data in the status. */
        de = (dreameye_state_ext_t *)frame->dev->status;
//        hexDump("getparam", respbuf8, 12);
        de->value = respbuf8[10] | respbuf8[11] << 8;
    }

    /* Wake up! */
    genwait_wake_all(frame);
}

int dreameye_get_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t *value) {
    uint32_t *send_buf;
    dreameye_state_ext_t *de;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = param | (arg << 8);
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_param_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the Dreameye to accept it */
    if(genwait_wait(&dev->frame, "dreameye_get_param", 500,
                    NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT)  {
            /* Something went wrong... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "dreameye_get_param: timeout to unit "
                   "%c%c\n", dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    if(value) {
        de = (dreameye_state_ext_t *)dev->status;
        *value = de->value;
    }

    return MAPLE_EOK;
}

static void dreameye_queue_param_cb(maple_frame_t *frame) {
//    dreameye_state_ext_t *de;
    maple_response_t *resp;
    uint8_t *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf8 = (uint8_t *)resp->data;

    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dreameye_state_ext_t *de = (dreameye_state_ext_t *)frame->dev->status;
        dbglog(DBG_ERROR, "%s: error 0x%02X 0x%02X 0x%02X value 0x%04X\n", 
                __func__, respbuf8[5], respbuf8[6], respbuf8[7], de->value);
        return;
    }

    if(resp->response != MAPLE_RESPONSE_OK) {
        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }

    // hexDump("setparam", respbuf8, 12);
}

int dreameye_queue_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t value) {
    uint32_t *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    dreameye_state_ext_t *de = (dreameye_state_ext_t *)dev->status;
    de->value = value;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = param | (arg << 8) | (value << 16);
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_queue_param_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}


static void dreameye_set_param_cb(maple_frame_t *frame) {

    dreameye_queue_param_cb(frame);

    /* Wake up! */
    genwait_wake_all(frame);
}


int dreameye_set_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t value) {
    uint32_t *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    dreameye_state_ext_t *de = (dreameye_state_ext_t *)dev->status;
    de->value = value;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = param | (arg << 8) | (value << 16);
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_set_param_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the Dreameye to accept it */
    if(genwait_wait(&dev->frame, "dreameye_set_param", 500,
                    NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT)  {
            /* Something went wrong... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "dreameye_set_param: timeout to unit "
                   "%c%c\n", dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

static int dreameye_set_format(maple_device_t **devs, int isp_mode, int format) {

    dreameye_state_ext_t *de = (dreameye_state_ext_t *)devs[0]->status;
    uint8_t pix_fmt = JANGGU_FMT_UNK7;

    switch(isp_mode) {
        case DREAMEYE_ISP_MODE_QSIF:
            de->width = 160;
            de->height = 120;
            break;
        case DREAMEYE_ISP_MODE_QCIF:
            de->width = 176;
            de->height = 144;
            break;
        case DREAMEYE_ISP_MODE_SIF:
            de->width = 320;
            de->height = 240;
            break;
        case DREAMEYE_ISP_MODE_CIF:
            de->width = 352;
            de->height = 288;
            break;
        case DREAMEYE_ISP_MODE_VGA:
            de->width = 640;
            de->height = 480;
            break;
        default:
            dbglog(DBG_ERROR, "%s: unknown ISP mode: %d\n", __func__, isp_mode);
            return MAPLE_EFAIL;
    }

    switch(format) {
        case DREAMEYE_FRAME_FMT_YUV420DE:
        case DREAMEYE_FRAME_FMT_YUV420P:
        case DREAMEYE_FRAME_FMT_NV21:
            pix_fmt |= (JANGGU_FMT_UNCOMPRESSED | JANGGU_FMT_YUV420DE);
            de->frame_size = de->width * de->height * 3 / 2;
            break;
        case DREAMEYE_FRAME_FMT_YUYV422:
            pix_fmt |= (JANGGU_FMT_UNCOMPRESSED | JANGGU_FMT_YUYV422);
            de->frame_size = de->width * de->height * 2;
            break;
        case DREAMEYE_FRAME_FMT_YUV420DE_COMPRESSED:
            pix_fmt |= (JANGGU_FMT_COMPRESSED | JANGGU_FMT_YUV420DE);
            de->frame_size = de->width * de->height; /* FIXME: better calc */
            break;
        case DREAMEYE_FRAME_FMT_YUYV422_COMPRESSED:
            pix_fmt |= (JANGGU_FMT_COMPRESSED | JANGGU_FMT_YUYV422);
            de->frame_size = de->width * de->height; /* FIXME: better calc */
            break;
        default:
            dbglog(DBG_ERROR, "%s: unknown format: %d\n", __func__, format);
            return MAPLE_EFAIL;
    }

    de->format = format;
    de->compressed = (pix_fmt & JANGGU_FMT_UNCOMPRESSED) ? 0 : 1;

    /* Set ISP operation mode */
    dreameye_set_param(devs[0], DREAMEYE_COND_REG_ISP, ISP_OP_MODE, isp_mode);

    if(isp_mode != DREAMEYE_ISP_MODE_VGA) {
        dreameye_queue_param(devs[1], DREAMEYE_COND_REG_ISP, ISP_SCALE_UPPER, 320 >> 8);
        dreameye_queue_param(devs[2], DREAMEYE_COND_REG_ISP, ISP_SCALE_LOWER, 320 & 0xff);
    }

    /* Set output format */
    dreameye_set_param(devs[3], DREAMEYE_COND_REG_ISP, ISP_OUT_FORM, 0x39);

    /* Setup JangGu compressor engine */
    dreameye_queue_param(devs[0], DREAMEYE_COND_REG_JANGGU, JANGGU_REG_HEIGHT, de->height);
    dreameye_queue_param(devs[1], DREAMEYE_COND_REG_JANGGU, 5, 0);
    dreameye_queue_param(devs[2], DREAMEYE_COND_REG_JANGGU, JANGGU_REG_FORMAT, pix_fmt);
    dreameye_queue_param(devs[3], DREAMEYE_COND_REG_JANGGU, 1, 0x33);
    dreameye_set_param(devs[4], DREAMEYE_COND_REG_JANGGU, 2, 16);
    dreameye_queue_param(devs[0], DREAMEYE_COND_REG_JANGGU, 6, 4);
    return dreameye_set_param(devs[1], DREAMEYE_COND_REG_JANGGU, 7, 0);
}

int dreameye_setup_video_camera(maple_device_t *dev, int isp_mode, int format) {
    const int dev_count = 5;
    maple_device_t *devs[dev_count];
    dreameye_register_t *dr;
    int i, j;

    for(i = 0; i < dev_count; ++i) {
        devs[i] = maple_enum_dev(dev->port, i + 1);
    }

    /* Disable Auto Function */
    dreameye_set_param(dev, DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0);

    /* Setup CMOS Image Sensor and Image Signal Processor */
    for (i = 0; i < 85; i += dev_count) {
        for (j = 0; j < dev_count; ++j) {

            dr = &ic_regs[i + j];

            if (j < 4) {
                dreameye_queue_param(devs[j], dr->reg, dr->param, dr->val);
            } else {
                dreameye_set_param(devs[j], dr->reg, dr->param, dr->val);
            }
        }
    }

    /* Setup frame format */
    dreameye_set_format(devs, isp_mode, format);

    /* Setup maple bus speed */
    if (DREAMEYE_DEVS_PER_FRAME > 3) {
        dreameye_set_param(dev, DREAMEYE_COND_MAPLE_BITRATE, 0x90, DREAMEYE_MAPLE_SPEED_4MBPS);
    } else {
        dreameye_set_param(dev, DREAMEYE_COND_MAPLE_BITRATE, 0x90, DREAMEYE_MAPLE_SPEED_2MBPS);
    }

#if 0
    int old = irq_disable();
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_DISABLED);
    maple_write(MAPLE_SPEED, MAPLE_SPEED_4MBPS | MAPLE_SPEED_TIMEOUT(50000));
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_ENABLED);
    irq_restore(old);
#endif
    /* Enable Auto Function */
    return dreameye_set_param(dev, DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0xB3);
}

int dreameye_stop_video_camera(maple_device_t *dev) {
    /* Disable Auto Function */
    int rv = dreameye_set_param(dev, DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0);

#if 0
    int old = irq_disable();
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_DISABLED);
    maple_write(MAPLE_SPEED, MAPLE_SPEED_2MBPS | MAPLE_SPEED_TIMEOUT(50000));
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_ENABLED);
    irq_restore(old);
#endif
    return rv;
}

int dreameye_start_capturing(maple_device_t *dev, dreameye_frame_cb cb) {
    dreameye_state_ext_t *de;

    assert(dev != NULL);
    assert(dev->unit == 1);

    de = (dreameye_state_ext_t *)dev->status;

    if(de->is_capturing || !cb) {
        return MAPLE_EFAIL;
    }

    /* Allocate space for the largest possible image that could fit in that
       number of transfers. */
    de->img_buf = (uint8_t *)memalign(32, de->frame_size);

    if(!de->img_buf) {
        return MAPLE_EFAIL;
    }

    de->img_transferring = 1;
    de->img_size = 0;
    de->img_number = 0;
    de->callback = cb;

    if(de->compressed) {
        de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE_COMPRESSED;
    } else {
        de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE;
    }

    first_state = de;

    return dreameye_get_video_frame_part(dev);
}

int dreameye_stop_capturing(maple_device_t *dev) {
    dreameye_state_ext_t *de;

    assert(dev != NULL);

    de = (dreameye_state_ext_t *)dev->status;

    if(!de->is_capturing) {
        return MAPLE_EFAIL;
    }

    de->is_capturing = 0;
    de->img_transferring = 0;
    de->transfer_count = 0;
    de->callback = NULL;

    free(de->img_buf);
    de->img_buf = NULL;

    first_state = NULL;

    return MAPLE_EOK;
}

int dreameye_poll(maple_device_t *dev) {
    dreameye_state_ext_t *de = first_state;

    if(!de || !de->is_capturing || !de->callback) {
        dev->status_valid = 1;
        return 0;
    }

    if(de->img_transferring == 0) {
        /* Send out complete frame */
        convert_frame(de);
        de->callback(dev, de->img_buf, de->img_size);
        /* Request next frame */
        if(de->compressed) {
            de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE_COMPRESSED;
        } else {
            de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE;
        }
        de->img_transferring = 1;
        de->img_size = 0;
        de->img_number ^= 1;
    }
    else if(de->img_transferring == -1) {
        /* Retry the same frame */
        if(de->compressed) {
            de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE_COMPRESSED;
        } else {
            de->transfer_count = de->frame_size / JANGGU_FRAME_DATA_SIZE;
        }
        de->img_transferring = 1;
    }

    if(de->transfer_count > 0) {
        dreameye_get_video_frame_part(dev);
    }

    dev->status_valid = 1;
    return 0;
}

#if 0
static void dreameye_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, dreameye_poll);
}

static int dreameye_attach(maple_driver_t *drv, maple_device_t *dev) {
    dreameye_state_ext_t *de;

    (void)drv;

    de = (dreameye_state_ext_t *)dev->status;
    de->image_count = 0;
    de->image_count_valid = 0;
    de->transfer_count = 0;
    de->img_transferring = 0;
    de->img_buf = NULL;
    de->img_size = 0;
    de->img_number = 0;
    de->is_capturing = 0;
    de->callback = NULL;

    dev->status_valid = 1;
    return 0;
}
#endif
