/** 
 * \file    rtc.h
 * \brief   Real time clock
 * \date    2015
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_RTC_H
#define _DS_RTC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/time.h>

/**
 * Grabs the current RTC seconds counter and adjusts it to the Unix Epoch.
 */
void rtc_gettimeofday(struct timeval *tv);

/**
 * Adjusts the given time value to the AICA Epoch and sets the RTC seconds counter.
 */
int rtc_settimeofday(const struct timeval *tv);

__END_DECLS
#endif /* _DS_RTC_H */
