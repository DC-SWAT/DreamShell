/* DreamShell ##version##

   dreameye.c - dreameye driver addons
   Copyright (C) 2015, 2023 SWAT

*/          

#include <ds.h>
#include <assert.h>
#include <kos/genwait.h>
#include <drivers/dreameye.h>
#include "cis_isp_regs.h"

typedef struct dreameye_register {
    uint8_t reg;
    uint8_t param;
    uint16_t val;
} dreameye_register_t;

static dreameye_state_ext_t *first_state = NULL;
static int dreameye_send_get_video_frame(maple_device_t *dev, dreameye_state_ext_t *state);

/* Regs dump from Visual Park */
static dreameye_register_t ic_regs[102] = {
    { DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0 },
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
    { DREAMEYE_COND_REG_ISP, ISP_OP_MODE, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_SCALE_UPPER, 1 },
    { DREAMEYE_COND_REG_ISP, ISP_SCALE_LOWER, 64 },
    { DREAMEYE_COND_REG_JANGGU, 4, 0x78 },
    { DREAMEYE_COND_REG_JANGGU, 5, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_START, 0x1E },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_SIDE, 0x78 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_H_CENTER, 20 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_START, 20 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_SIDE, 35 },
    { DREAMEYE_COND_REG_ISP, ISP_WIN_V_CENTER, 10 },
    { DREAMEYE_COND_REG_ISP, ISP_OUT_FORM, 0x39 },
    { DREAMEYE_COND_REG_JANGGU, 0, 0xC1 },
    { DREAMEYE_COND_REG_JANGGU, 1, 0x33 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_UPPER, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_MIDDLE, 0 },
    { DREAMEYE_COND_REG_ISP, ISP_AF_UT_LOWER, 48 },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_UPPER, 0x2D },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_MIDDLE, 0xC6 },
    { DREAMEYE_COND_REG_ISP, ISP_EXP_LMT_LOWER, 0xC0 },
    { DREAMEYE_COND_REG_CIS, CIS_TITU, 9 },
    { DREAMEYE_COND_REG_CIS, CIS_TITM, 0x27 },
    { DREAMEYE_COND_REG_CIS, CIS_TITL, 0xC0 },
    { DREAMEYE_COND_REG_JANGGU, 2, 1 },
    { DREAMEYE_COND_REG_JANGGU, 2, 2 },
    { DREAMEYE_COND_REG_JANGGU, 2, 16 },
    { DREAMEYE_COND_REG_JANGGU, 6, 4 },
    { DREAMEYE_COND_REG_JANGGU, 7, 0 },
    { DREAMEYE_COND_MAPLE_BITRATE, 0x90, 0x0FA0 },
    { DREAMEYE_COND_MAPLE_BITRATE, 0x90, 64 },
    { DREAMEYE_COND_REG_ISP, ISP_AUTO_ENB, 0xB3 },
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

static void dreameye_get_video_frame_cb(maple_frame_t *frame) {
    maple_device_t *dev;
    maple_response_t *resp;
    uint32 *respbuf32;
    uint8 *respbuf8, *packet;
    int len, part;
    int bits, pixels, info, packet_len;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    if(frame->dev == NULL)
        return;

    dev = frame->dev;

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

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

    if(first_state->compressed) {
        // TODO
    } else {
        /* Copy only YUV data without header */
        len -= 4;
        memcpy(first_state->img_buf + first_state->img_size, packet + 4, len);
        first_state->img_size += len;
    }

    /* Check if we're done. */
    if(part == 0x40) {
        first_state->img_transferring = 0;
        return;
    }

    if(++first_state->transfer_count + dev->unit < 29) {
        dreameye_send_get_video_frame(dev, first_state);
    }
}

static int dreameye_send_get_video_frame(maple_device_t *dev, dreameye_state_ext_t *state) {
    uint32 *send_buf;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
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

int dreameye_get_video_frame(maple_device_t *dev, uint8 fb_num, uint8 **data,
                       int *img_sz) {
    dreameye_state_ext_t *de;
    maple_device_t *dev2, *dev3, *dev4, *dev5;

    assert(dev != NULL);
    assert(dev->unit == 1);

    dev2 = maple_enum_dev(dev->port, 2);
    dev3 = maple_enum_dev(dev->port, 3);
    dev4 = maple_enum_dev(dev->port, 4);
    dev5 = maple_enum_dev(dev->port, 5);

    de = (dreameye_state_ext_t *)dev->status;

    first_state = de;
    de->img_transferring = 1;
    de->img_buf = NULL;
    de->img_size = 0;
    de->img_number = fb_num;
    de->transfer_count = 0;
    de->value = 0;
    de->compressed = 0;

    /* Allocate space for the largest possible image that could fit in that
       number of transfers. */
    de->img_buf = (uint8 *)malloc(964 * 30); /* Frame packet size * transfers count */

    if(!de->img_buf)
        goto fail;

    /* Send out the image requests to all sub devices. */
    dreameye_send_get_video_frame(dev, de);
    dreameye_send_get_video_frame(dev2, de);
    dreameye_send_get_video_frame(dev3, de);
    dreameye_send_get_video_frame(dev4, de);
    dreameye_send_get_video_frame(dev5, de);

    while(de->img_transferring == 1) {
        thd_pass();
    }

    if(de->img_transferring == 0) {
        *data = de->img_buf;
        *img_sz = de->img_size;

        // dbglog(DBG_DEBUG, "dreameye_get_video_frame: %d received in "
        //        "%d transfers\n", de->img_size, de->transfer_count + 1);

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
    uint32 *respbuf32;
    uint8 *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
//        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }

    if(respbuf32[0] != MAPLE_FUNC_CAMERA) {
        return;
    }

    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "%s: error 0x%02X 0x%02X 0x%02X\n", 
                __func__, respbuf8[5], respbuf8[6], respbuf8[7]);
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

int dreameye_get_param(maple_device_t *dev, uint8 param, uint8 arg, uint16 *value) {
    uint32 *send_buf;
    dreameye_state_ext_t *de;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
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
    uint8 *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf8 = (uint8 *)resp->data;

    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "%s: error 0x%02X 0x%02X 0x%02X\n", 
                __func__, respbuf8[5], respbuf8[6], respbuf8[7]);
		return;
    }

    if(resp->response != MAPLE_RESPONSE_OK) {
        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }

    // hexDump("setparam", respbuf8, 12);
}

int dreameye_queue_param(maple_device_t *dev, uint8 param, uint8 arg, uint16 value) {
    uint32 *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
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


int dreameye_set_param(maple_device_t *dev, uint8 param, uint8 arg, uint16 value) {
    uint32 *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
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

void dreameye_setup_video_camera(maple_device_t *dev) {
    const int dev_count = 5;
    maple_device_t *devs[dev_count];
    dreameye_register_t *dr;

    devs[0] = dev;
    devs[1] = maple_enum_dev(dev->port, 2);
    devs[2] = maple_enum_dev(dev->port, 3);
    devs[3] = maple_enum_dev(dev->port, 4);
    devs[4] = maple_enum_dev(dev->port, 5);

    for (int i = 0; i < 100; i += dev_count) {
        for (int j = 0; j < dev_count; ++j) {

            dr = &ic_regs[i + j];

            if (j < 4) {
                dreameye_queue_param(devs[j], dr->reg, dr->param, dr->val);
            } else {
                dreameye_set_param(devs[j], dr->reg, dr->param, dr->val);
            }
        }
    }

    dr = &ic_regs[100];
    dreameye_queue_param(devs[0], dr->reg, dr->param, dr->val);
    dr = &ic_regs[101];
    dreameye_set_param(devs[dev_count - 1], dr->reg, dr->param, dr->val);
}
