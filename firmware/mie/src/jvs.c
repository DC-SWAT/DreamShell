/**
 * DreamShell MIE firmware
 * JVS frames, macros and node slots
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#include "hw.h"
#include "gpio.h"
#include "maple.h"
#include "jvs.h"

static uint8_t *node_slot(uint8_t node) {
    return (uint8_t *)(NODE_BASE + (uint16_t)node * NODE_STRIDE);
}

static uint8_t uart_wait(uint8_t mask) {
    uint8_t n;

    n = 0;
    for (;;) {
        n++;
        if (n == MAPLE_SPIN) {
            return 0;
        }
        if (uart_lsr & mask) {
            return 1;
        }
    }
}

static uint8_t uart_put(uint8_t byte) {
    if (!uart_wait(UART_LSR_THRE)) {
        return 0;
    }
    uart_data = byte;
    return 1;
}

static uint8_t uart_get(uint8_t *byte) {
    uint8_t n;

    n = 0;
    for (;;) {
        n++;
        if (n == JVS_RX_SPIN) {
            return 0;
        }
        if (uart_lsr & UART_LSR_DR) {
            *byte = uart_data;
            return 1;
        }
    }
}

static uint8_t jvs_tx(uint8_t dest, uint8_t count, const uint8_t *data) {
    uint8_t sum;
    uint8_t i;
    uint8_t byte;

    uart_fcr = UART_FCR_TX;
    jvs_control = 1;
    sum = 0;
    if (!uart_put(JVS_SYNC)) {
        jvs_control = 0;
        return JVS_TX_OK;
    }
    if (!uart_put(dest)) {
        jvs_control = 0;
        return JVS_TX_OK;
    }
    sum = (uint8_t)(sum + dest);
    byte = (uint8_t)(count + 1);
    if (!uart_put(byte)) {
        jvs_control = 0;
        return JVS_TX_OK;
    }
    sum = (uint8_t)(sum + byte);
    for (i = 0; i < count; i++) {
        byte = data[i];
        sum = (uint8_t)(sum + byte);
        if (byte == JVS_MARK || byte == JVS_SYNC) {
            if (!uart_put(JVS_MARK) || !uart_put((uint8_t)(byte - 1))) {
                jvs_control = 0;
                return JVS_TX_OK;
            }
        }
        else
        if (!uart_put(byte)) {
            jvs_control = 0;
            return JVS_TX_OK;
        }
    }
    if (sum == JVS_MARK || sum == JVS_SYNC) {
        if (!uart_put(JVS_MARK) || !uart_put((uint8_t)(sum - 1))) {
            jvs_control = 0;
            return JVS_TX_OK;
        }
    }
    else
    if (!uart_put(sum)) {
        jvs_control = 0;
        return JVS_TX_OK;
    }
    uart_wait(UART_LSR_TEMT);
    jvs_control = 0;
    return JVS_TX_OK;
}

static uint8_t jvs_wait_data(void) {
    uint8_t tries;
    uint8_t polls;

    tries = 0;
    while (tries != JVS_SENSE_TRIES) {
        polls = 0;
        while (polls != JVS_SENSE_POLLS) {
            if (uart_lsr & UART_LSR_DR) {
                return JVS_RX_OK;
            }
            polls++;
        }
        tries++;
    }
    return JVS_RX_NONE;
}

static uint8_t jvs_recv(uint8_t *dest, uint8_t *stored) {
    uint8_t n;
    uint8_t sum;
    uint8_t byte;
    uint8_t length;
    uint8_t left;

    n = 0;
    sum = 0;
    for (;;) {
        if (!uart_get(&byte)) {
            return JVS_RX_TIMEOUT;
        }
        if (byte == JVS_SYNC) {
            break;
        }
    }
    dest[n] = byte;
    n++;
    if (!uart_get(&byte)) {
        return JVS_RX_TIMEOUT;
    }
    dest[n] = byte;
    n++;
    if (byte != 0) {
        *stored = n;
        return JVS_RX_DEST;
    }
    sum = byte;
    if (!uart_get(&byte)) {
        return JVS_RX_TIMEOUT;
    }
    dest[n] = byte;
    n++;
    length = byte;
    sum = (uint8_t)(sum + byte);
    if (length == 0) {
        *stored = n;
        return JVS_RX_SUM;
    }
    left = (uint8_t)(length - 1);
    while (left != 0) {
        if (!uart_get(&byte)) {
            return JVS_RX_TIMEOUT;
        }
        if (byte == JVS_MARK) {
            if (!uart_get(&byte)) {
                return JVS_RX_TIMEOUT;
            }
            byte = (uint8_t)(byte + 1);
        }
        dest[n] = byte;
        n++;
        sum = (uint8_t)(sum + byte);
        left--;
    }
    if (!uart_get(&byte)) {
        return JVS_RX_TIMEOUT;
    }
    if (byte == JVS_MARK) {
        if (!uart_get(&byte)) {
            return JVS_RX_TIMEOUT;
        }
        byte = (uint8_t)(byte + 1);
    }
    dest[n] = byte;
    n++;
    *stored = n;
    if (byte != sum) {
        return JVS_RX_SUM;
    }
    return JVS_RX_OK;
}

void jvs_uart_init(void) {
    uint8_t i;

    jvs_state = 0;
    jvs_mark = 0;
    jvs_control = 1;
    uart_ier = 1;
    uart_lcr = UART_LCR_DLAB;
    uart_data = UART_DIVISOR;
    uart_ier = 0;
    uart_mcr = 0;
    uart_fcr = UART_FCR_ENABLE;
    uart_fcr = 0;
    uart_fcr = UART_FCR_ENABLE;
    uart_lcr = UART_LCR_8N1;
    (void)uart_data;
    for (i = 0; i < NODE_SLOTS; i++) {
        mie_zero(node_slot(i), NODE_STRIDE);
    }
}

static void jvs_store_result(uint8_t *slot, uint8_t status, uint8_t stored) {
    slot[JVS_SLOT_LEN] = (uint8_t)(stored + JVS_LEN_BIAS);
    slot[JVS_SLOT_TAG] = JVS_SLOT_TAG_VALUE;
    slot[JVS_SLOT_NODE] = jvs_state;
    slot[JVS_SLOT_SENSE] = 0;
    slot[JVS_SLOT_SAVED] = jvs_saved;
    slot[JVS_SLOT_STATUS] = status;
    slot[JVS_SLOT_STORED] = stored;
}

static void jvs_finish(uint8_t count) {
    uint8_t status;
    uint8_t stored;
    uint8_t *slot;

    status = jvs_tx(jvs_saved, count, jvs_frame);
    if (status != jvs_mark) {
        return;
    }
    status = jvs_wait_data();
    stored = 0;
    slot = node_slot(jvs_state);
    if (status != JVS_RX_NONE) {
        status = jvs_recv(slot + JVS_SLOT_PKT, &stored);
        jvs_control = 1;
        slot[JVS_SLOT_PKT + stored] = 0;
    }
    jvs_store_result(slot, status, stored);
    active_flag = 1;
}

static void jvs_reply_node(uint8_t sub) {
    if (active_flag == 0) {
        return;
    }
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = sub;
    reply_buf[5] = jvs_state;
    reply_buf[6] = jvs_sense;
    reply_buf[7] = 0;
    maple_reply_iy();
}

static uint16_t macro_off(uint8_t index) {
    uint8_t hi;
    uint8_t lo;
    uint8_t n;
    uint8_t carry;

    hi = 0;
    lo = index;
    for (n = 0; n < 4; n++) {
        carry = (uint8_t)(lo >> 7);
        lo = (uint8_t)(lo << 1);
        hi = (uint8_t)(hi + carry);
        hi = (uint8_t)(hi << 1);
    }
    carry = (uint8_t)(lo >> 7);
    lo = (uint8_t)(lo << 1);
    hi = (uint8_t)(hi + carry);
    return ((uint16_t)hi << 8) | lo;
}

static uint8_t *macro_at(uint8_t index) {
    return (uint8_t *)(MACRO_BASE + macro_off(index));
}

static void jvs_gap(void) {
    uint8_t n;

    n = 0;
    while (n != JVS_FRAME_GAP) {
        n++;
    }
}

static void jvs_run_batch(uint8_t count) {
    uint8_t *rec;
    uint8_t *cursor;
    uint8_t total;
    uint8_t left;

    node_slot(jvs_state)[JVS_SLOT_LEN] = 0;
    rec = jvs_frame;
    cursor = node_slot(jvs_state) + JVS_SLOT_PKT;
    total = 0;
    left = count;
    while (left != 0) {
        uint8_t dest;
        uint8_t len;
        uint8_t status;
        uint8_t stored;
        uint8_t *end;
        uint8_t *hdr;

        dest = *rec;
        rec++;
        len = *rec;
        rec++;
        jvs_saved = dest;
        jvs_tx(dest, len, rec);
        rec += len;
        stored = 0;
        status = jvs_wait_data();
        if (status != JVS_RX_NONE) {
            status = jvs_recv(cursor, &stored);
            jvs_control = 1;
        }
        end = cursor + stored;
        *end = 0;
        hdr = end - stored - 3;
        hdr[0] = dest;
        hdr[1] = status;
        hdr[2] = stored;
        cursor = end + 3;
        total = (uint8_t)(total + stored + 3);
        jvs_gap();
        left--;
    }
    {
        uint8_t *slot;

        slot = node_slot(jvs_state);
        slot[JVS_SLOT_LEN] = (uint8_t)(total + 4);
        slot[JVS_SLOT_TAG] = JVS_SLOT_TAG_VALUE;
        slot[JVS_SLOT_NODE] = jvs_state;
        slot[JVS_SLOT_SENSE] = 0;
    }
    active_flag = 1;
}

void jvs_on_irq(void) {
    uint8_t status;
    uint8_t stored;
    uint8_t *pkt;

    pkt = node_slot(jvs_state) + JVS_SLOT_PKT;
    status = jvs_recv(pkt, &stored);
    pkt[stored] = 0;
    jvs_store_result(node_slot(jvs_state), status, stored);
}

void ior_jvs_req(uint8_t *payload) {
    uint8_t node;
    uint8_t count;

    gpio_apply(payload[1], payload[2], payload[3], payload[4]);
    node = (uint8_t)(payload[5] & JVS_NODE_MASK);
    jvs_state = node;
    jvs_saved = payload[6];
    count = payload[7];
    mie_copy(jvs_frame, payload + 8, count);
    node_slot(node)[JVS_SLOT_LEN] = 0;
    jvs_reply_node(JVS_REPLY_SUB);
    jvs_finish(count);
}

static void jvs_fetch_node(uint8_t node) {
    uint8_t ready;
    uint8_t *slot;

    jvs_node_id = node;
    slot = node_slot(node);
    slot[JVS_SLOT_SENSE] = jvs_sense;
    ready = slot[JVS_SLOT_LEN];
    if (ready != 0) {
        slot[JVS_SLOT_LEN] = 0;
        gpio_sample(slot + JVS_SLOT_GPIO);
        maple_reply_long(slot + JVS_SLOT_TAG, (uint8_t)(ready + JVS_FETCH_ADD),
                         MAPLE_RESP_IO, 0, MAPLE_ADDR);
        slot[JVS_SLOT_STORED] = 0;
        return;
    }
    gpio_sample(slot + JVS_SLOT_GPIO);
    maple_reply_long(slot + JVS_SLOT_TAG, JVS_FETCH_EMPTY,
                     MAPLE_RESP_IO, 0, MAPLE_ADDR);
}

void ior_jvs_fetch(uint8_t *payload) {
    jvs_fetch_node((uint8_t)(payload[1] & JVS_NODE_MASK));
}

void ior_jvs_11(uint8_t *payload) {
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = (uint8_t)(payload[0] + 1);
    reply_buf[5] = jvs_sense;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
}

void ior_jvs_13(uint8_t *payload) {
    uint8_t index;
    uint8_t count;
    uint8_t *slot;

    index = payload[1];
    if (index == 0 || index >= MACRO_INDEX_MAX) {
        reply_buf[0] = MAPLE_RESP_IO;
        reply_buf[1] = 0;
        reply_buf[2] = MAPLE_ADDR;
        reply_buf[3] = 1;
        reply_buf[4] = (uint8_t)(payload[0] + 1);
        reply_buf[5] = jvs_sense;
        reply_buf[6] = 0;
        reply_buf[7] = 0;
        maple_reply_iy();
        return;
    }
    count = (uint8_t)(payload[2] + 1);
    if (count == 0 || count >= MACRO_COPY_MAX) {
        reply_buf[0] = MAPLE_RESP_IO;
        reply_buf[1] = 0;
        reply_buf[2] = MAPLE_ADDR;
        reply_buf[3] = 1;
        reply_buf[4] = (uint8_t)(payload[0] + 1);
        reply_buf[5] = jvs_sense;
        reply_buf[6] = 0;
        reply_buf[7] = 0;
        maple_reply_iy();
        return;
    }
    slot = macro_at(index);
    mie_copy(slot, payload + 3, count);
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = (uint8_t)(payload[0] + 1);
    reply_buf[5] = jvs_sense;
    reply_buf[6] = count;
    reply_buf[7] = 0;
    maple_reply_iy();
}

static void jvs_apply_head(uint8_t *payload) {
    gpio_apply(payload[1], payload[2], payload[3], payload[4]);
    jvs_state = (uint8_t)(payload[5] & JVS_NODE_MASK);
}

static void jvs_append_macro(uint8_t *len_at, uint8_t **dst, uint8_t index) {
    uint8_t *macro;
    uint8_t n;

    macro = macro_at(index);
    n = macro[0];
    *len_at = (uint8_t)(*len_at + n);
    mie_copy(*dst, macro + 1, n);
    *dst += n;
}

void ior_jvs_19(uint8_t *payload) {
    uint8_t *macro;
    uint8_t base;
    uint8_t extra;
    uint8_t total;

    jvs_apply_head(payload);
    jvs_saved = payload[6];
    macro = macro_at(jvs_saved);
    base = macro[0];
    mie_copy(jvs_frame, macro + 1, base);
    extra = MAPLE_PAYLOAD[7];
    if (extra != 0) {
        mie_copy(jvs_frame + base, (uint8_t *)MAPLE_PAYLOAD + 8, extra);
    }
    total = (uint8_t)(base + extra);
    node_slot(jvs_state)[JVS_SLOT_LEN] = 0;
    jvs_reply_node(JVS_REPLY_SUB);
    jvs_finish(total);
}

void ior_jvs_21(uint8_t *payload) {
    uint8_t *dst;
    uint8_t *len_at;
    uint8_t inline_n;
    uint8_t total;

    jvs_apply_head(payload);
    jvs_saved = payload[6];
    dst = jvs_frame;
    inline_n = payload[7];
    if (inline_n != 0 && payload[8] == 0xFF) {
        inline_n = *(uint8_t *)SIDE_BASE;
        mie_copy(dst, (uint8_t *)SIDE_BASE + 1, inline_n);
        dst += inline_n;
    }
    else
    if (inline_n != 0) {
        mie_copy(dst, payload + 8, inline_n);
        dst += inline_n;
    }
    len_at = &inline_n;
    jvs_append_macro(len_at, &dst, jvs_saved);
    total = *len_at;
    node_slot(jvs_state)[JVS_SLOT_LEN] = 0;
    jvs_reply_node(JVS_REPLY_SUB);
    jvs_finish(total);
}

void ior_jvs_23(uint8_t *payload) {
    uint8_t sub;

    sub = (uint8_t)(payload[0] + 1);
    mie_di();
    jvs_uart_init();
    if (payload[0] != 0) {
        jvs_mark = 1;
        irq_enable = (uint8_t)(irq_enable | IRQ_ENABLE_JVS);
        mie_ei();
        reply_buf[0] = MAPLE_RESP_IO;
        reply_buf[1] = 0;
        reply_buf[2] = MAPLE_ADDR;
        reply_buf[3] = 1;
        reply_buf[4] = sub;
        reply_buf[5] = jvs_sense;
        reply_buf[6] = 1;
        reply_buf[7] = 0;
        maple_reply_iy();
        return;
    }
    irq_enable = (uint8_t)(irq_enable & IRQ_ENABLE_JVS);
    mie_ei();
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = sub;
    reply_buf[5] = jvs_sense;
    reply_buf[6] = 0;
    reply_buf[7] = 0;
    maple_reply_iy();
}

static uint8_t jvs_pack_simple(uint8_t *src, uint8_t count) {
    uint8_t *dst;
    uint8_t n;

    dst = jvs_frame;
    n = count;
    while (n != 0) {
        uint8_t len;

        *dst = *src;
        dst++;
        src++;
        len = *src;
        *dst = len;
        dst++;
        src++;
        mie_copy(dst, src, len);
        dst += len;
        src += len;
        n--;
    }
    return count;
}

void ior_jvs_25(uint8_t *payload) {
    uint8_t count;

    jvs_apply_head(payload);
    count = payload[6];
    jvs_pack_simple(payload + 7, count);
    jvs_reply_node(JVS_BATCH_SUB);
    jvs_run_batch(count);
}

static uint8_t jvs_pack_macro(uint8_t *src, uint8_t count) {
    uint8_t *dst;
    uint8_t n;

    dst = jvs_frame;
    n = count;
    while (n != 0) {
        uint8_t dest;
        uint8_t len;
        uint8_t *len_at;

        dest = *src;
        src++;
        len = *src;
        src++;
        *dst = dest;
        dst++;
        len_at = dst;
        *dst = len;
        dst++;
        if (len != 0 && *src == 0xFF) {
            src++;
            len = *(uint8_t *)SIDE_BASE;
            *len_at = len;
            mie_copy(dst, (uint8_t *)SIDE_BASE + 1, len);
            dst += len;
        }
        else
        if (len != 0) {
            mie_copy(dst, src, len);
            dst += len;
            src += len;
        }
        jvs_append_macro(len_at, &dst, dest);
        n--;
    }
    return count;
}

void ior_jvs_27(uint8_t *payload) {
    uint8_t count;

    jvs_apply_head(payload);
    count = payload[6];
    jvs_pack_macro(payload + 7, count);
    jvs_reply_node(JVS_BATCH_SUB);
    jvs_run_batch(count);
}

void ior_jvs_33(uint8_t *payload) {
    active_flag = 0;
    mie_copy((uint8_t *)SAVED_BASE, (uint8_t *)MAPLE_BUF, MAPLE_FRAME);
    jvs_fetch_node((uint8_t)(payload[5] & JVS_NODE_MASK));
    ior_jvs_21((uint8_t *)SAVED_BASE + 4);
}

void ior_jvs_35(uint8_t *payload) {
    active_flag = 0;
    mie_copy((uint8_t *)SAVED_BASE, (uint8_t *)MAPLE_BUF, MAPLE_FRAME);
    jvs_fetch_node((uint8_t)(payload[5] & JVS_NODE_MASK));
    ior_jvs_27((uint8_t *)SAVED_BASE + 4);
}

void ior_jvs_37(uint8_t *payload) {
    uint8_t offset;
    uint8_t count;
    uint8_t *dst;

    offset = payload[1];
    count = payload[2];
    if (count == 0) {
        if (offset == 0) {
            *(uint8_t *)SIDE_BASE = 0;
            reply_buf[6] = 0;
        }
        else {
            *(uint8_t *)SIDE_BASE = offset;
            reply_buf[6] = offset;
        }
    }
    else
    if (count >= SIDE_COPY_MAX) {
        *(uint8_t *)SIDE_BASE = 0;
        reply_buf[6] = 0;
    }
    else {
        dst = (uint8_t *)SIDE_BASE + 1 + offset;
        mie_copy(dst, payload + 3, count);
        *(uint8_t *)SIDE_BASE = 0;
        reply_buf[6] = count;
    }
    reply_buf[0] = MAPLE_RESP_IO;
    reply_buf[1] = 0;
    reply_buf[2] = MAPLE_ADDR;
    reply_buf[3] = 1;
    reply_buf[4] = JVS_SIDE_SUB;
    reply_buf[5] = jvs_sense;
    reply_buf[7] = 0;
    maple_reply_iy();
}
