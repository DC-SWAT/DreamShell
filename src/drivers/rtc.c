/* DreamShell ##version##

   rtc.c
   Copyright (C) 2015 SWAT
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
#define AICA_RTC_SECS_H 0xa0710000
#define AICA_RTC_SECS_L 0xa0710004


void rtc_gettimeofday(struct timeval *tv) {
	
	uint32 val1, val2;

	do {
		
		val1 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		val2 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		
	} while (val1 != val2);

	tv->tv_sec = val1 - TWENTY_YEARS;

	/* Can't get microseconds with just a seconds counter. */
	tv->tv_usec = 0;
}


int rtc_settimeofday(const struct timeval *tv) {
	
	uint32 val1, val2;
	uint32 secs = tv->tv_sec + TWENTY_YEARS;

	do {
		
		g2_write_32((secs & 0xffff0000) >> 16, AICA_RTC_SECS_H);
		g2_write_32((secs & 0xffff), AICA_RTC_SECS_L);

		val1 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		val2 = ((g2_read_32(AICA_RTC_SECS_H) & 0xffff) << 16) | (g2_read_32(AICA_RTC_SECS_L) & 0xffff);
		
	} while (val1 != val2);

	return 0;
}
