/** 
 * \file    rtc.h
 * \brief   Real time clock
 * \date    2015-2016
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_RTC_H
#define _DS_RTC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/time.h>
#include <time.h>

/**
 * Grabs the current RTC seconds counter and adjusts it to the Unix Epoch.
 * 
 * Return:
 * On SUCCESS UNIX Time Stamp
 * On ERROR -1
 */
time_t rtc_gettime();

/**
 * Adjusts the given time value to the AICA Epoch and sets the RTC seconds counter.
 * 
 * Return:
 * On SUCCESS 0
 * On ERROR -1
 */
int rtc_settime(time_t time);

/**
 * Grabs the current RTC seconds counter and write to struct tm.
 * 
 * Return:
 * On SUCCESS 0
 * On ERROR -1
 */
int rtc_gettimeutc(struct tm *time);

/**
 * Adjusts the given time value from struct tm to the AICA Epoch and sets the RTC seconds counter.
 * 
 * Return:
 * On SUCCESS 0
 * On ERROR -1
 */
int rtc_settimeutc(const struct tm *time);

/**
 * Grabs the current RTC seconds counter and write to struct timeval.
 */
void rtc_gettimeofday(struct timeval *tv);

/**
 * Adjusts the given time value from struct timeval to the AICA Epoch and sets the RTC seconds counter.
 * 
 * Return:
 * On SUCCESS 0
 * On ERROR -1
 */
int rtc_settimeofday(const struct timeval *tv);


__END_DECLS
#endif /* _DS_RTC_H */
