/**
 * DreamShell ISO Loader
 * Maple sniffing and VMU emulation
 * (c)2022-2023 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include <exception.h>
#include <ubc.h>
#include <maple.h>
#include <dc/maple.h>
#include <dc/controller.h>

/**
 * Turn off maple logging by default
 * but keep it for sniffer mode.
 */
#ifdef LOG
// # define MAPLE_LOG 1
#endif

#if defined(MAPLE_SNIFFER) && defined(LOG)
# ifndef MAPLE_LOG
#  define MAPLE_LOG 1
# endif
#endif

typedef struct {
    uint32 function;
    uint16 size;
    uint16 partition;
    uint16 sys_block;
    uint16 fat_block;
    uint16 fat_cnt;
    uint16 file_info_block;
    uint16 file_info_cnt;
    uint8  vol_icon;
    uint8  reserved;
    uint16 save_block;
    uint16 save_cnt;
    uint32 reserved_exec;
} maple_memory_t;


#ifdef MAPLE_LOG
static void maple_dump_frame(const char *direction, uint32 num, maple_frame_t *frame) {
    LOGF("      %s: %d cmd=%d from=0x%02lx to=0x%02lx len=%d",
        direction, num, frame->cmd, frame->from, frame->to, frame->datalen);
    if (frame->datalen) {
        LOGF(" data=");
        uint32 *dt = (uint32 *)frame->data;
        int max_len = frame->datalen > 7 ? 7 : frame->datalen;
        for(int i = 0; i < max_len; ++i) {
            LOGF("0x%08lx ", *dt++);
        }
    }
    LOGF("\n");
}
static void maple_dump_device_info(maple_devinfo_t *di) {
    char name[sizeof(di->product_name) + 1];
    memcpy(name, di->product_name, sizeof(di->product_name));
    name[sizeof(di->product_name)] = '\0';

    LOGF("      DEVICE: %s | 0x%08lx | 0x%08lx 0x%08lx 0x%08lx | 0x%02lx | 0x%02lx | %d | %d\n",
        name, di->func, di->function_data[0], di->function_data[1], di->function_data[2],
        di->area_code, di->connector_direction, di->standby_power, di->max_power);
}
static void maple_dump_memory_info(maple_memory_t *mi) {
    LOGF("      MEMORY: 0x%08lx | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d | 0x%08lx\n",
        mi->function, mi->size, mi->partition, mi->sys_block, mi->fat_block,
        mi->fat_cnt, mi->file_info_block, mi->file_info_cnt, mi->vol_icon,
        mi->reserved, mi->save_block, mi->save_cnt, mi->reserved_exec);
}
#else
# define maple_dump_frame(a, b, c)
# define maple_dump_device_info(a)
# define maple_dump_memory_info(a)
#endif

void maple_read_frame(uint32 *buffer, maple_frame_t *frame) {
    uint32 val = *buffer++;
    uint8 *b = (uint8 *)&val;
    frame->cmd = b[0];
    frame->to = b[1];
    frame->from = b[2];
    frame->datalen = b[3];
    frame->data = buffer;
}

#ifndef MAPLE_SNIFFER
static int vmu_fd = FILEHND_INVALID;
static maple_devinfo_t device_info = {
    MAPLE_FUNC_MEMCARD | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK,
    { 0x403f7e7e, 0x00100500, 0x00410f00 },
    0xff,
    0x00,
    { 
        'V','i','s','u','a','l',' ','M','e','m',
        'o','r','y',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',' ',' '
    },
    {
        'P','r','o','d','u','c','e','d',' ','B','y',' ','o','r',' ',
        'U','n','d','e','r',' ','L','i','c','e','n','s','e',' ','F',
        'r','o','m',' ','S','E','G','A',' ','E','N','T','E','R','P',
        'R','I','S','E','S',',','L','T','D','.',' ',' ',' ',' ',' '
    },
    0x007c,
    0x0082
};
static maple_memory_t memory_info = {
    MAPLE_FUNC_MEMCARD,
    255,
    0,
    255,
    254,
    1,
    253,
    13,
    0,
    0,
    200,
    0,
    0x00800000
};

static void maple_vmu_device_info(maple_frame_t *req, maple_frame_t *resp) {
    maple_devinfo_t *di = (maple_devinfo_t *)&resp->data;

    resp->cmd = MAPLE_RESPONSE_DEVINFO;
    resp->datalen = sizeof(maple_devinfo_t) / 4;
    resp->from = req->to;
    resp->to = req->from;

    if (req->cmd == MAPLE_COMMAND_ALLINFO) {
        resp->cmd = MAPLE_RESPONSE_ALLINFO;
        resp->datalen += 20;
        memset(di + sizeof(maple_devinfo_t), 0, 20 * 4);
    }

    memcpy(di, &device_info, sizeof(maple_devinfo_t));
    maple_dump_device_info(&device_info);
}

static void maple_vmu_memory_info(maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    maple_memory_t *mi = (maple_memory_t *)&resp->data;

    resp->cmd = MAPLE_RESPONSE_DATATRF;
    resp->datalen = sizeof(maple_memory_t) / 4;
    resp->from = req->to;
    resp->to = req->from;

    memcpy(mi, &memory_info, sizeof(maple_memory_t));
    maple_dump_memory_info(&memory_info);
}

static void maple_vmu_block_read(maple_frame_t *req, maple_frame_t *resp) {
    uint32 *req_params = (uint32 *)req->data;
    uint32 *resp_params = (uint32 *)&resp->data;
    resp_params[0] = req_params[0];
    resp_params[1] = req_params[1];
    resp->from = req->to;
    resp->to = req->from;

    if (pre_read_xfer_busy()) {
        resp->cmd = MAPLE_RESPONSE_AGAIN;
        resp->datalen = 0;
        LOGF("      BREAD: device busy\n");
        return;
    }

    uint16 block = ((req_params[1] >> 24) & 0xff) | ((req_params[1] >> 16) & 0xff) << 8;
    uint8 *buff = (uint8 *)&resp_params[2];

    lseek(vmu_fd, block * 512, SEEK_SET);
    int res = read(vmu_fd, buff, 512);
    LOGF("      BREAD: block=%d buff=0x%08lx res=%d\n", block, buff, res);

    if (res < 0) {
        resp->datalen = 2;
        resp->cmd = MAPLE_RESPONSE_FILEERR;
        return;
    }
    resp->cmd = MAPLE_RESPONSE_DATATRF;
    resp->datalen = 130;

    if (block == 255) {
        memcpy((uint8 *)&memory_info.fat_block, &buff[70], 14);
    }
}

static void maple_vmu_block_write(maple_frame_t *req, maple_frame_t *resp) {
    uint32 *req_params = (uint32 *)req->data;
    resp->cmd = MAPLE_RESPONSE_OK;
    resp->from = req->to;
    resp->to = req->from;
    resp->datalen = 0;

    // Maybe it's LCD or Buzzer data
    if (req_params[0] != MAPLE_FUNC_MEMCARD) {
        return;
    }
    if (pre_read_xfer_busy()) {
        resp->cmd = MAPLE_RESPONSE_AGAIN;
        LOGF("      BWRITE: device busy\n");
        return;
    }

    uint8 phase = (req_params[1] >> 8) & 0x0f;
    uint16 block = ((req_params[1] >> 24) & 0xff) | ((req_params[1] >> 16) & 0xff) << 8;
    uint8 *buff = (uint8 *)&req_params[2];

#if _FS_READONLY == 0
    lseek(vmu_fd, (block * 512) + (128 * phase), SEEK_SET);
    int res = write(vmu_fd, buff, 128);
    LOGF("      BWRITE: block=%d phase=%d buff=0x%08lx res=%d\n", block, phase, buff, res);

    if (res < 0) {
        uint32 *resp_params = (uint32 *)&resp->data;
        resp_params[0] = req_params[0];
        resp_params[1] = req_params[1];
        resp->datalen = 2;
        resp->cmd = MAPLE_RESPONSE_FILEERR;
        return;
    }
#else
    LOGF("      BWRITE: block=%d phase=%d buff=0x%08lx disabled\n", block, phase, buff);
#endif

    if (block == 255 && phase == 0) {
        memcpy((uint8 *)&memory_info.fat_block, &buff[70], 14);
    }
}

static void maple_vmu_block_sync(maple_frame_t *req, maple_frame_t *resp) {
    resp->cmd = MAPLE_RESPONSE_OK;
    resp->from = req->to;
    resp->to = req->from;
    resp->datalen = 0;

    if (pre_read_xfer_busy()) {
        resp->cmd = MAPLE_RESPONSE_AGAIN;
        LOGF("      BSYNC: device busy\n");
        return;
    }

    ioctl(vmu_fd, FS_IOCTL_SYNC, NULL);

#ifdef LOG
    uint32 *req_params = (uint32 *)req->data;
    uint16 block = ((req_params[1] >> 24) & 0xff) | ((req_params[1] >> 16) & 0xff) << 8;
    uint16 len = ((req_params[1] >> 8) & 0xff) | (req_params[1] & 0xff) << 8;
    LOGF("      BSYNC: block=%d len=%d\n", block, len);
#endif
}

static void maple_controller(maple_frame_t *req, maple_frame_t *resp) {
    uint32 *resp_params = (uint32 *)&resp->data;
    static uint32 prev_buttons = 0;
    (void)req;

    if (resp_params[0] != MAPLE_FUNC_CONTROLLER
        || resp->cmd != MAPLE_RESPONSE_DATATRF) {
        return;
    }

    cont_cond_t *cond = (cont_cond_t *)&resp_params[1];

    if (cond->buttons == 0xffff) {
        prev_buttons = 0;
        return;
    }

    uint32 buttons = (~cond->buttons & 0xffff);
    // LOGF("      CTRL: but=0x%04lx joyx=%d joyy=%d\n", buttons, cond->joyx, cond->joyy);

#ifdef HAVE_SCREENSHOT
    if (buttons == IsoInfo->scr_hotkey && buttons != prev_buttons) {
        video_screenshot();
    }
#endif

    prev_buttons = buttons;
}

static void maple_cmd_proc(int8 cmd, maple_frame_t *req, maple_frame_t *resp) {

    if (vmu_fd > FILEHND_INVALID) {

        if (resp->from & 0x20) {
            resp->from |= 0x01;
        }
        if (req->to == 0x01) {
            switch (cmd) {
                case MAPLE_COMMAND_DEVINFO:
                case MAPLE_COMMAND_ALLINFO:
                    maple_vmu_device_info(req, resp);
                    break;
                case MAPLE_COMMAND_GETMINFO:
                    maple_vmu_memory_info(req, resp);
                    break;
                case MAPLE_COMMAND_BREAD:
                    maple_vmu_block_read(req, resp);
                    break;
                case MAPLE_COMMAND_BWRITE:
                    maple_vmu_block_write(req, resp);
                    break;
                case MAPLE_COMMAND_BSYNC:
                    maple_vmu_block_sync(req, resp);
                    break;
                default:
                    break;
            }
        }
    }

    if (IsoInfo->scr_hotkey && cmd == MAPLE_COMMAND_GETCOND) {
        maple_controller(req, resp);
    }
}

#else // MAPLE_SNIFFER

static void maple_cmd_proc(int8 cmd, maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    switch (cmd) {
        case MAPLE_COMMAND_DEVINFO:
        case MAPLE_COMMAND_ALLINFO:
            maple_dump_device_info((maple_devinfo_t *)NONCACHED_ADDR((uint32)&resp->data));
            break;
        case MAPLE_COMMAND_GETMINFO:
            maple_dump_memory_info((maple_memory_t *)NONCACHED_ADDR((uint32)&resp->data));
            break;
        default:
            break;
    }
}
#endif // MAPLE_SNIFFER

static void maple_dma_proc() {
    uint32 *data, *recv_data, addr, value;
    uint32 trans_count;
    uint8 len, last = 0, port, pattern;
    maple_frame_t req_frame;
    maple_frame_t *resp_frame_ptr;

    addr = MAPLE_REG(MAPLE_DMA_ADDR);

    data = (uint32 *)NONCACHED_ADDR(addr);

    for (trans_count = 0; trans_count < 24 && !last; ++trans_count) {

        /* First word: transfer parameters */
        value = *data++;

        len = value & 0xff;
        pattern = (value >> 8) & 0xff;
        port = (value >> 16) & 0x0f;
        last = (value >> 31) & 0x01;

        /* Second word: receive buffer physical address */
        addr = *data;

        /* Skip lightgun mode and NOP frame */
        if (pattern) {
            continue;
        }

#ifndef MAPLE_SNIFFER
        if (port) {
            data += len + 2;
            continue;
        }
#endif

        maple_read_frame(data + 1, &req_frame);
        data += len + 2;

        recv_data = (uint32 *)NONCACHED_ADDR(addr);
        resp_frame_ptr = (maple_frame_t *)recv_data;

#ifdef MAPLE_LOG
        LOGF("MAPLE_XFER: %d value=0x%08lx pattern=0x%02x len=%d port=%d addr=0x%08lx last=%d\n",
            trans_count, value, pattern, len, port, (pattern ? 0 : addr), last);

        maple_frame_t resp_frame;
        maple_read_frame(recv_data, &resp_frame);
        maple_dump_frame("SEND", trans_count, &req_frame);
        maple_dump_frame("RECV", trans_count, &resp_frame);

        maple_cmd_proc(req_frame.cmd, &req_frame, resp_frame_ptr);

        if (vmu_fd > FILEHND_INVALID && (req_frame.to == 0x01 || resp_frame.from & 0x20)) {
            maple_read_frame(recv_data, &resp_frame);
            maple_dump_frame("EMUL", trans_count, &resp_frame);
        }
#else
        maple_cmd_proc(req_frame.cmd, &req_frame, resp_frame_ptr);
#endif
    }
}


#ifdef NO_ASIC_LT
void *maple_dma_handler(void *passer, register_stack *stack, void *current_vector) {
    (void)passer;
    (void)stack;
#else
static asic_handler_f old_maple_dma_handler = NULL;
static void *maple_dma_handler(void *passer, register_stack *stack, void *current_vector) {
    if (old_maple_dma_handler && passer != current_vector) {
        current_vector = old_maple_dma_handler(passer, stack, current_vector);
    }
#endif

#ifdef HAVE_UBC
    static uint32 requested = 0;

    if (passer == current_vector) {
        // Handle UBC break on Maple register
        requested = 1;
    } else if(requested) {
        maple_dma_proc();
        requested = 0;
    }
#else
    uint32 code = *REG_INTEVT;

    if (((*ASIC_IRQ11_MASK & ASIC_NRM_AICA_DMA) && code == EXP_CODE_INT11)
        || ((*ASIC_IRQ9_MASK & ASIC_NRM_AICA_DMA) && code == EXP_CODE_INT9)
        || ((*ASIC_IRQ13_MASK & ASIC_NRM_AICA_DMA) && code == EXP_CODE_INT13)
    ) {
        maple_dma_proc();
    }
#endif
    return current_vector;
}


int maple_init_irq() {

#ifdef HAVE_UBC
    ubc_init();
    ubc_configure_channel(UBC_CHANNEL_A, MAPLE_DMA_STATUS, UBC_BBR_OPERAND | UBC_BBR_WRITE);
#endif

#ifndef NO_ASIC_LT
    asic_lookup_table_entry a_entry;
    memset(&a_entry, 0, sizeof(a_entry));
    
    a_entry.irq = EXP_CODE_INT11;
    a_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_MAPLE_DMA;
    a_entry.handler = maple_dma_handler;

    return asic_add_handler(&a_entry, &old_maple_dma_handler, 0);
#else
    return 0;
#endif
}

int maple_init_vmu(int num) {
#ifdef MAPLE_SNIFFER
    (void)num;
#else
# if _FS_READONLY == 0
    int flags = O_RDWR | O_PIO;
# else
    int flags = O_RDONLY | O_PIO;
# endif

    char *filename = relative_filename("vmu001.vmd");
    set_file_number(filename, num);

    if (vmu_fd > FILEHND_INVALID) {
        close(vmu_fd);
    }

    vmu_fd = open(filename, flags);
    free(filename);

    if (vmu_fd < 0) {

        filename = "/DS/vmu/vmu001.vmd";
        set_file_number(filename, num);

        if (vmu_fd < 0) {
            vmu_fd = open(filename + 3, flags);
            if (vmu_fd < 0) {
                LOGFF("can't find VMU dump: %d\n", num);
                return -1;
            }
        }
    }

    lseek(vmu_fd, (255 * 512) + 70, SEEK_SET);
    read(vmu_fd, (uint8 *)&memory_info.fat_block, 14);

    LOGFF("fat_blk=%d fat_cnt=%d fileinf_blk=%d fileinf_cnt=%d\n",
            memory_info.fat_block, memory_info.fat_cnt,
            memory_info.file_info_block, memory_info.file_info_cnt);

#endif
    return 0;
}
