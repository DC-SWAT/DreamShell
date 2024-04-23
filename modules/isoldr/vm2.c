/* DreamShell ##version##

   DreamShell ISO Loader
   VM2 and USB4Maple support

   Copyright (C)2024 megavolt85
   Copyright (C)2024 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include <dc/maple.h>

typedef struct maple_alldevinfo {
    uint32  functions;              /**< \brief Function codes supported */
    uint32  function_data[3];       /**< \brief Additional data per function */
    uint8   area_code;              /**< \brief Region code */
    uint8   connector_direction;    /**< \brief ? */
    char    product_name[30];       /**< \brief Name of device */
    char    product_license[60];    /**< \brief License statement */
    uint16  standby_power;          /**< \brief Power consumption (standby) */
    uint16  max_power;              /**< \brief Power consumption (max) */
    char    extended[40];
    char    free[40];
} maple_alldevinfo_t;

static uint8_t recv_buff[196];

static void vbl_allinfo_callback(maple_frame_t * frm) {
    maple_response_t *resp;

    /* So.. did we get a response? */
    resp = (maple_response_t *)frm->recv_buf;

    if (resp->response == MAPLE_RESPONSE_ALLINFO) {
        /* Copy in the new buff */
        memcpy(recv_buff, resp, 196);
    }

    maple_frame_unlock(frm);
    genwait_wake_all(frm);
}

/* Send a ALLINFO command for the given port/unit */
static int send_allinfo(maple_device_t * dev) {
    // Reserve access; if we don't get it, forget about it 
    if (maple_frame_lock(&dev->frame) < 0) {
        return -1;
    }

    // Setup our autodetect frame to probe at a new device 
    maple_frame_init(&dev->frame);
    dev->frame.cmd = MAPLE_COMMAND_ALLINFO;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.callback = vbl_allinfo_callback;
    maple_queue_frame(&dev->frame);

    /* Wait for the VM2 to accept it */
    if(genwait_wait(&dev->frame, "vmu_get_allinfo", 200, NULL) < 0)  {
        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "send_allinfo: timeout to unit %c%c\n", dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }
    return MAPLE_EOK;
}

static void vm2_reply(maple_frame_t * frm) {
    maple_response_t *resp;

    /* So.. did we get a response? */
    resp = (maple_response_t *)frm->recv_buf;

    if (resp->response != MAPLE_RESPONSE_OK) {
        dbglog(DBG_ERROR, "maple: bad response %d on device, wait ACK\n",resp->response); 
    }

    memcpy(recv_buff, resp, 4);
    maple_frame_unlock(frm);
    genwait_wake_all(frm);
}

static int vm2_set_id(maple_device_t * dev, const char *ID, const char *name) {
    maple_response_t *resp;
    uint32_t *send_buf;

wait_vm2:

    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MEMCARD;
    strncpy((char *) &dev->frame.recv_buf[4], ID, 12);

    if (name) {
        strncpy((char *) &dev->frame.recv_buf[16], name, 128);
    }

    dev->frame.cmd = 33;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = name ? 36 : 4;
    dev->frame.callback = vm2_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);
    
    /* Wait for the VM2 to accept it */
    if(genwait_wait(&dev->frame, "vm2_set_id", 200, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vm2_set_id: timeout to unit %c%c\n", dev->port + 'A', dev->unit + '0');
            return -1;
        }
    }

    resp = (maple_response_t *) recv_buff;

    if (resp->response == MAPLE_RESPONSE_OK) {
        return 0;
    }
    else if (resp->response == MAPLE_RESPONSE_AGAIN) {
        goto wait_vm2;
    }

    return -1;
}

static int check_vm2_present(maple_device_t * dev) {

    memset(recv_buff, 0, 196);

    if (send_allinfo(dev) != MAPLE_EOK) {
        return 0;
    }

    maple_alldevinfo_t *info = (maple_alldevinfo_t *) &recv_buff[4];

    if (!strncasecmp(info->extended, "VM2 by Dreamware", 16) ||
        !strncasecmp(info->extended, "USB RP2040 EMU  ", 16)) {
        return 1;
    }
    return 0;
}

void isoldr_vm2_bank_switch(const char *ipbin_info_sec) {

    int i;
    char pid[12];
    char name[128];

    strncpy(pid, ipbin_info_sec + 64, sizeof(pid));
    strncpy(name, ipbin_info_sec + 128, sizeof(name));

    pid[10] = '\0';
    for (i = 9; i > 0; i--) {
        if (pid[i] == ' ') {
            pid[i] = '\0';
        }
        else if (pid[i]) {
            break;
        }
    }

    name[127] = '\0';
    for (i = 126; i > 0; i--) {
        if (name[i] == ' ') {
            name[i] = '\0';
        }
        else if (name[i]) {
            break;
        }
    }

    for (i = 0; i < 8; i++) {
        maple_device_t * vmu = maple_enum_type(i, MAPLE_FUNC_MEMCARD);

        if (vmu && check_vm2_present(vmu)) {

            vm2_set_id(vmu, pid, name);
            thd_sleep(200);

            while (!maple_enum_dev(vmu->port, vmu->unit)) {
                thd_pass();
            }
            break;
        }
    }
}
