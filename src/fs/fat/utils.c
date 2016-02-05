// $$$ : utils.c --

#include "ff_utils.h"
#include <drivers/rtc.h>

#define	S_P_MIN		60L
#define	S_P_HOUR	(60L * S_P_MIN)
#define	S_P_DAY		(24L * S_P_HOUR)
#define	S_P_YEAR	(365L * S_P_DAY)
#define	JAPAN_ADJ	(-9L * S_P_HOUR)

struct _time_block *conv_gmtime(uint32 t)
{
    static int day_p_m[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int day, mon, yr, cumday;
    static struct _time_block tm;

    tm.sec = t % 60L;
    t /= 60L;
    tm.min = t % 60L;
    t /= 60L;
    tm.hour = t % 24L;
    day = t / 24L;
    yr = 1950;
    cumday = 0;
    while ((cumday += (yr % 4 ? 365 : 366)) <= day) yr++;
    tm.year = yr;
    cumday -= yr % 4 ? 365 : 366;
    day -= cumday;
    cumday = 0;
    mon = 0;
    day_p_m[1] = (yr % 4) == 0 ? 29 : 28;
    while ((cumday += day_p_m[mon]) <= day) mon++;
    cumday -= day_p_m[mon];
    day -= cumday;
    tm.day = day + 1;
    tm.mon = mon + 1;
    return &tm;
}


DWORD get_fattime()
{
    struct timeval tv;
    struct _time_block *tm;
    DWORD tmr;

	rtc_gettimeofday(&tv);
    tm = conv_gmtime(tv.tv_sec);

    tmr =  (((DWORD)tm->year - 60) << 25)
		| ((DWORD)tm->mon << 21)
		| ((DWORD)tm->day << 16)
		| (WORD)(tm->hour << 11)
		| (WORD)(tm->min << 5)
		| (WORD)(tm->sec >> 1);
    return tmr;
}
