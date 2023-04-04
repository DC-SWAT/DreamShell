/* DreamShell ##version##

   dreameye.c - dreameye driver addons
   Copyright (C)2015 SWAT 

*/          

#include <ds.h>
#include <assert.h>
#include <kos/genwait.h>
#include <drivers/dreameye.h>

static int frame_pkg_size = 1004; // FIXME
static dreameye_state_t *first_state = NULL;
static int dreameye_send_get_video_frame(maple_device_t *dev,
                                   dreameye_state_t *state, uint8 req,
                                   uint8 cnt);
								   
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
    uint8 *respbuf8, cur_pkg;
    int len = 0, bit_exp = 0, pix_exp = 0, frame_info = 0, packet_len = 0;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    if(frame->dev == NULL)
        return;

    dev = frame->dev;

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

    hexDump("resp", resp->data, resp->data_len * 4);
	
//    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
//        dbglog(DBG_ERROR, "dreameye_get_video_frame: error 0x%02X%02X%02X\n", 
//                respbuf8[5], respbuf8[6], respbuf8[7]);
//    }

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
        first_state->img_transferring = -1;
        return;
    }

    if(respbuf32[0] != MAPLE_FUNC_CAMERA) {
        first_state->img_transferring = -1;
        dbglog(DBG_ERROR, "%s: bad func: 0x%08lx\n", __func__, respbuf32[0]);
        return;
    }

    len = (resp->data_len - 3) * 4;
    cur_pkg = respbuf8[5];
    bit_exp = respbuf8[14] + (respbuf8[13] << 8);
    pix_exp = respbuf8[12] + ((respbuf8[11] & 0x3f) << 8);
    frame_info = respbuf8[11] & 0xc0;
    packet_len = ((bit_exp + 47) >> 4) << 1;
    dbglog(DBG_DEBUG, "%s: cur=%d len=%d part=%02x bit=%d pix=%d fi=%02x pkglen=%d\n", __func__,
			cur_pkg, len, respbuf8[4], bit_exp, pix_exp, frame_info, packet_len);

    /* Copy the data. */
    memcpy(first_state->img_buf + first_state->img_size, respbuf8 + 16, len);
    first_state->img_size += len;

    /* Check if we're done. */
    if(respbuf8[4] & 0x40) {
        first_state->img_transferring = 0;
        return;
    }
	
	cur_pkg += 5;

    if(cur_pkg < first_state->transfer_count) {
        dreameye_send_get_video_frame(dev, first_state, DREAMEYE_IMAGEREQ_CONTINUE, cur_pkg);
	}
}

static int dreameye_send_get_video_frame(maple_device_t *dev,
                                   dreameye_state_t *state, uint8 req,
                                   uint8 cnt) {
    uint32 *send_buf;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_SUBCOMMAND_IMAGEREQ | (state->img_number << 8) |
                  (req << 16) | (cnt << 24);
    dev->frame.cmd = MAPLE_COMMAND_CAMCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_video_frame_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

int dreameye_get_video_frame(maple_device_t *dev, uint8 image, uint8 **data,
                       int *img_sz) {
    dreameye_state_t *de;
    maple_device_t *dev2, *dev3, *dev4, *dev5;

    assert(dev != NULL);
    assert(dev->unit == 1);

    dev2 = maple_enum_dev(dev->port, 2);
    dev3 = maple_enum_dev(dev->port, 3);
    dev4 = maple_enum_dev(dev->port, 4);
    dev5 = maple_enum_dev(dev->port, 5);

    de = (dreameye_state_t *)dev->status;

    first_state = de;
    de->img_transferring = 1;
    de->img_buf = NULL;
    de->img_size = 0;
    de->img_number = image;
    de->transfer_count = 32; // FIXME

    /* Allocate space for the largest possible image that could fit in that
       number of transfers. */
    de->img_buf = (uint8 *)malloc(frame_pkg_size * de->transfer_count);

    if(!de->img_buf)
        goto fail;

    /* Send out the image requests to all sub devices. */
    dreameye_send_get_video_frame(dev, de, DREAMEYE_IMAGEREQ_START, 0);
    dreameye_send_get_video_frame(dev2, de, DREAMEYE_IMAGEREQ_CONTINUE, 1);
    dreameye_send_get_video_frame(dev3, de, DREAMEYE_IMAGEREQ_CONTINUE, 2);
    dreameye_send_get_video_frame(dev4, de, DREAMEYE_IMAGEREQ_CONTINUE, 3);
    dreameye_send_get_video_frame(dev5, de, DREAMEYE_IMAGEREQ_CONTINUE, 4);

    while(de->img_transferring == 1) {
        thd_pass();
    }

    if(de->img_transferring == 0) {
        *data = de->img_buf;
        *img_sz = de->img_size;

        dbglog(DBG_DEBUG, "dreameye_get_image: Image of size %d received in "
               "%d transfers\n", de->img_size, de->transfer_count);

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
	
    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "dreameye_get_param: error 0x%02X%02X%02X\n", 
                respbuf8[5], respbuf8[6], respbuf8[7]);
    }

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
//        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }

    if(respbuf32[0] != MAPLE_FUNC_CAMERA)
        return;

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


static void dreameye_set_param_cb(maple_frame_t *frame) {
//    dreameye_state_ext_t *de;
    maple_response_t *resp;
    uint8 *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf8 = (uint8 *)resp->data;
	
    if(resp->response == MAPLE_COMMAND_CAMCONTROL && respbuf8[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "dreameye_set_param: error 0x%02X%02X%02X\n", 
                respbuf8[5], respbuf8[6], respbuf8[7]);
		return;
    }

    if(resp->response != MAPLE_RESPONSE_OK) {
        dbglog(DBG_ERROR, "%s: bad response: %d\n", __func__, resp->response);
        return;
    }
	
    hexDump("setparam", respbuf8, 12);

    /* Wake up! */
    genwait_wake_all(frame);
}

int dreameye_set_param(maple_device_t *dev, uint8 param, uint8 arg, uint16 value) {
    uint32 *send_buf;
//    dreameye_state_ext_t *de;

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
