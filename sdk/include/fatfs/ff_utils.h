/**
 * Fatfs utils
 */
#ifndef __FF_UTILS_H__
#define __FF_UTILS_H__

#include "integer.h"

struct _time_block {
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec;
};

unsigned long rtc_secs(void);
struct _time_block *conv_gmtime(unsigned long t);
DWORD get_fattime();

#endif


