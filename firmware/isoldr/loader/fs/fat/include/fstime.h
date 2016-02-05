/* $$$ : fstime.h --
*/
#ifndef __FSTIME_H__
#define __FSTIME_H__

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

#endif
// end of : fstime.h

