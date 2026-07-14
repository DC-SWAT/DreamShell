/**
 * DreamShell HollySH BIOS loader
 * Logging
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <limits.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <dc/video.h>

#ifdef LOG_SCI
#include <dc/sci.h>
#else
#include <dc/scif.h>
#endif

int printf(const char *fmt, ...) {
    static int print_y = 1;
    int i = 0;
    uint16 *vram = (uint16*)(VIDEO_VRAM_START);

    if(fmt == NULL) {
        print_y = 1;

        for(i = 0; i < 0x4B000; i++) {
            vram[i] = 0;
        }
        return 0;
    }

    char buff[64];
    va_list args;

    va_start(args, fmt);
    i = vsnprintf(buff, sizeof(buff), fmt, args);
    va_end(args);

#ifndef LOG_SCREEN
    LOGF(buff);
#endif

    bfont_draw_str(vram + ((print_y * 24 + 4) * 640) + 12, 0xffff, 0x0000, buff);

    if(buff[strlen(buff)-1] == '\n') {
        if (print_y++ > 15) {
            print_y = 0;
            memset((uint32 *)vram, 0, 2 * 1024 * 1024);
        }
    }

    return i;
}

size_t strnlen(const char *s, size_t maxlen) {
    const char *e;
    size_t n;

    for (e = s, n = 0; *e && n < maxlen; e++, n++)
        ;
    return n;
}

int check_digit (char c) {
    if((c>='0') && (c<='9')) {
        return 1;
    }
    return 0;
}

#ifdef LOG

static int log_open = 0;
static char log_buff[256];

int OpenLog() {
    memset(log_buff, 0, sizeof(log_buff));
#ifdef LOG_SCI
    sci_init(SCI_UART_BAUD_115200, SCI_MODE_UART, SCI_CLK_INT);
#elif !defined(LOG_SCREEN)
    scif_init();
#endif
    log_open = 1;
    return 1;
}

void CloseLog() {
    log_open = 0;
}

static inline void PutLog(const char *str, size_t len) {
#ifdef LOG_SCI
    sci_write_data((uint8 *)str, len);
#elif defined(LOG_SCREEN)
    (void)len;
    printf(str);
#else
    scif_write_buffer((uint8 *)str, len, 1);
#endif
}

int WriteLog(const char *fmt, ...) {
    va_list args;
    int i;

    if(!log_open) {
        return 0;
    }

    va_start(args, fmt);
    i = vsnprintf(log_buff, sizeof(log_buff), fmt, args);
    va_end(args);

    if(i > 0) {
        PutLog(log_buff, i);
    }
    return i;
}

int WriteLogFunc(const char *func, const char *fmt, ...) {

    if(!log_open) {
        return 0;
    }

    PutLog(func, strlen(func));

    if(fmt == NULL) {
        const char str[] = "\n";
        PutLog(str, 1);
        return 0;
    }

    const char str[] = ": ";
    PutLog(str, 2);

    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(log_buff, sizeof(log_buff), fmt, args);
    va_end(args);

    if(i > 0) {
        PutLog(log_buff, i);
    }
    return i;
}

#endif
