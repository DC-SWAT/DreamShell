/**
 * DreamShell ISO Loader
 * Maple device emulation
 * (c)2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include <exception.h>
#include <dc/maple.h>

void maple_read_addr(uint8 addr, int *port, int *unit) {
    *port = (addr >> 6) & 3;
    if(addr & 0x20)
        *unit = 0;
    else if(addr & 0x10)
        *unit = 5;
    else if(addr & 0x08)
        *unit = 4;
    else if(addr & 0x04)
        *unit = 3;
    else if(addr & 0x02)
        *unit = 2;
    else if(addr & 0x01)
        *unit = 1;
    else {
        *port = -1;
        *unit = -1;
    }
}

#ifdef LOG
# ifdef DEBUG
static uint32 maple_dma_count;
# endif
static void maple_dump_frame(const char *direction, uint32 num, maple_frame_t *frame) {

    int from_port, to_port;
    int from_unit, to_unit;

    maple_read_addr(frame->from, &from_port, &from_unit);
    maple_read_addr(frame->to, &to_port, &to_unit);

    LOGF("      %s: %d cmd=%d from=%d|%d to=%d|%d len=%d",
        direction, num, frame->cmd,
        from_port, from_unit, to_port, to_unit, frame->datalen);

    if (frame->datalen) {
        LOGF(" data=");
        uint32 *dt = (uint32 *)frame->data;
        int max_len = frame->datalen > 8 ? 8 : frame->datalen;
        for(int i = 0; i < max_len; ++i) {
            LOGF("0x%08lx ", *dt++);
        }
    }
    LOGF("\n");
}
static void maple_dump_device_info(maple_devinfo_t *di) {
    LOGF(" MAPLE_DEV: %s | %s | 0x%08lx | 0x%08lx 0x%08lx 0x%08lx | 0x%02lx | 0x%02lx | 0x%04lx | 0x%04lx",
        di->product_name, di->product_license, di->func, di->function_data[0], di->function_data[1],
        di->function_data[2], di->area_code, di->connector_direction, di->standby_power, di->max_power);
}
#else
# define maple_dump_frame(a, b, c)
# define maple_dump_device_info(a)
#endif

void maple_read_frame(uint32 *buffer, maple_frame_t *frame) {
    uint8 *b = (uint8 *) buffer;

    frame->cmd = b[0];
    frame->to = b[1];
    frame->from = b[2];
    frame->datalen = b[3];
    frame->data = &b[4];
}

static void maple_vmu_device_info(maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    maple_devinfo_t *di = (maple_devinfo_t *)resp->data;
    // TODO
    (void)di;
    maple_dump_device_info((maple_devinfo_t *)CACHED_ADDR((uint32)di));
}

static void maple_vmu_memory_info(maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    (void)resp;
    // TODO
    // resp->cmd = MAPLE_RESPONSE_DATATRF;
    // resp->datalen = 25;
    LOGFF(NULL);
}

static void maple_vmu_block_read(maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    (void)resp;
    // TODO
    // resp->cmd = MAPLE_RESPONSE_DATATRF;
    LOGFF(NULL);
}

static void maple_vmu_block_write(maple_frame_t *req, maple_frame_t *resp) {
    (void)req;
    (void)resp;
    // TODO
    // resp->cmd = MAPLE_RESPONSE_OK;
    LOGFF(NULL);
}

static void maple_dma_process() {

    uint32 *data, *recv_data, addr, value;
    uint32 trans_count = 0;
    uint32 frame_count;
    uint8 len, last, port, cmd;
    maple_frame_t req_frame;
    maple_frame_t resp_frame;
    maple_frame_t *resp_frame_ptr;

    addr = *(vuint32 *)0xa05f6c04;
    data = (uint32 *)UNCACHED_ADDR(addr);

    DBGF("--- START MAPLE DMA: %ld at 0x%08lx ---\n", ++maple_dma_count, addr);

    for (trans_count = 0; trans_count < 8; ++trans_count) {

        /* First word: message length and destination port */
        value = *data++;
        len = value & 0xff;
        port = (value >> 16) & 0xff;
        last = (value >> 31) & 0x0f;

        /* Second word: receive buffer physical address */
        addr = *data++;

        if (!len || !(addr & UNCACHED_ADDR(RAM_START_ADDR)) || port > 0x04) {
            break;
        }

        recv_data = (uint32 *)UNCACHED_ADDR(addr);
        int is_filtered = 1;

        for (frame_count = 0; frame_count < 8; ++frame_count) {

            maple_read_frame(data, &req_frame);
            cmd = req_frame.cmd;

            /* Filter out controller messages */
            if (cmd == MAPLE_COMMAND_GETCOND) {
                continue;
            }
            if (is_filtered) {
                is_filtered = 0;
                LOGF("MAPLE_XFER: %d val=0x%08lx len=%d port=%d addr=0x%08lx last=%d\n",
                    trans_count, value, len, port, addr, last);
            }

            maple_dump_frame("SEND", frame_count, &req_frame);

            len -= req_frame.datalen;
            data += req_frame.datalen + 1;

            maple_read_frame(recv_data, &resp_frame);

            resp_frame_ptr = (maple_frame_t *)recv_data;
            recv_data += resp_frame.datalen + 1;

            maple_dump_frame("RECV", frame_count, &resp_frame);

            switch (cmd) {
                case MAPLE_RESPONSE_DEVINFO:
                    maple_vmu_device_info(&req_frame, resp_frame_ptr);
                    break;
                case MAPLE_COMMAND_GETMINFO:
                    maple_vmu_memory_info(&req_frame, resp_frame_ptr);
                    break;
                case MAPLE_COMMAND_BREAD:
                    maple_vmu_block_read(&req_frame, resp_frame_ptr);
                    break;
                case MAPLE_COMMAND_BWRITE:
                    maple_vmu_block_write(&req_frame, resp_frame_ptr);
                    break;
                default:
                    break;
            }
            if (!len) {
                break;
            }
        }
        if (last) {
            break;
        }
    }
    DBGF("--- END MAPLE DMA ---\n");
}


#ifdef NO_ASIC_LT
void *maple_dma_handler(void *passer, register_stack *stack, void *current_vector) {
    (void)passer;
    (void)stack;
#else
static asic_handler_f old_maple_dma_handler = NULL;
static void *maple_dma_handler(void *passer, register_stack *stack, void *current_vector) {
    if (old_maple_dma_handler) {
        current_vector = old_maple_dma_handler(passer, stack, current_vector);
    }
#endif
    maple_dma_process();
    return current_vector;
}


int maple_init_irq() {
#ifdef NO_ASIC_LT
    return 0;
#else
    asic_lookup_table_entry a_entry;
    memset(&a_entry, 0, sizeof(a_entry));
    
    a_entry.irq = EXP_CODE_ALL;
    a_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_MAPLE_DMA;
    a_entry.handler = maple_dma_handler;

    return asic_add_handler(&a_entry, &old_maple_dma_handler, 0);
#endif
}
