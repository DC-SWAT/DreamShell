/* DreamShell ##version##

   rtc.c
   Copyright (C) 2015-2016 SWAT
   Copyright (C) 2016 Megavolt85
*/

#include <arch/types.h>
#include <dc/g2bus.h>
#include <drivers/rtc.h>

/* The AICA RTC has an Epoch of 1/1/1950, so we must subtract 20 years (in
   seconds to get the standard Unix Epoch when getting the time, and add 20
   years when setting the time. */
#define TWENTY_YEARS ((20 * 365LU + 5) * 86400)

/* The AICA RTC is represented by a 32-bit seconds counter stored in 2 16-bit
   registers.*/
#define AICA_RTC_SECS_H   0xa0710000
#define AICA_RTC_SECS_L   0xa0710004
#define AICA_RTC_WRITE_EN 0xa0710008

#define AICA_RTC_ATTEMPTS_COUNT 10

time_t rtc_gettime() {

	time_t val1, val2;
	int i = 0;

	do {

		val1 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		val2 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);

		if(i++ > AICA_RTC_ATTEMPTS_COUNT) {
			return (-1);
		}

	} while (val1 != val2);

	return (val1 - TWENTY_YEARS);
}


int rtc_settime(time_t time) {

	time_t val1, val2;
	time_t secs = time + TWENTY_YEARS;
	int i = 0;

	do {

		g2_write_32(1, AICA_RTC_WRITE_EN);
		g2_write_32((secs & 0xffff0000) >> 16, AICA_RTC_SECS_H);
		g2_write_32((secs & 0xffff), AICA_RTC_SECS_L);

		val1 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		val2 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);

		if(i++ > AICA_RTC_ATTEMPTS_COUNT) {
			return (-1);
		}

	} while ((val1 != val2) && ((val1 & 0xffffff80) != (secs & 0xffffff80)));

	return 0;
}


int rtc_gettimeutc(struct tm *time) {

	time_t timestamp = rtc_gettime();

	if(time == NULL || timestamp == -1) {
		return -1;
	}

	localtime_r(&timestamp, time);
	return 0;
}


int rtc_settimeutc(const struct tm *time) {

	time_t timestamp = mktime((struct tm *)time);

	if(time == NULL || timestamp == -1) {
		return -1;
	}

	return rtc_settime(timestamp);
}


void rtc_gettimeofday(struct timeval *tv) {
	tv->tv_sec = rtc_gettime();

	/* Can't get microseconds with just a seconds counter. */
	tv->tv_usec = 0;
}

int rtc_settimeofday(const struct timeval *tv) {
	return rtc_settime(tv->tv_sec);
}
