/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: timing.c,v 1.1 2005/02/26 01:47:37 jrecker Exp $ 
 *   
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

/**************************************************************************************
 * Fixed-point HE-AAC decoder
 * Jon Recker (jrecker@real.com)
 * February 2005
 *
 * timing.c - implementations of CPU timing functions
 **************************************************************************************/

#include "wrapper.h"

#if (defined (_WIN32) && !defined (_WIN32_WCE)) || defined (__GNUC__) || (defined (__arm) && defined (__ARMCC_VERSION))

#include <time.h>
#define CLOCKS_PER_MSEC		(CLOCKS_PER_SEC / 1000)

int InitTimer(void)
{
    return 0;
}

unsigned int ReadTimer(void)
{
    return clock();
}

int FreeTimer(void)
{
    return 0;
}

unsigned int GetClockFrequency(void)
{
    return CLOCKS_PER_SEC;
}

unsigned int CalcTimeDifference(unsigned int startTime, unsigned int endTime)
{
    /* timer counts up on x86, 32-bit counter */
    if (endTime < startTime)
		return (0xffffffff - (startTime - endTime) );
    else
		return (endTime - startTime);
}

#elif defined (_WIN32) && defined (_WIN32_WCE) && defined (ARM)

#include <windows.h>

int InitTimer(void)
{
    return 0;
}

unsigned int ReadTimer(void)
{
    return GetTickCount();
}

int FreeTimer(void)
{
    return 0;
}

unsigned int GetClockFrequency(void)
{
    return 1000;
}

unsigned int CalcTimeDifference(unsigned int startTime, unsigned int endTime)
{
    /* timer counts up on x86, 32-bit counter */
    if (endTime < startTime)
		return (0xffffffff - (startTime - endTime) );
    else
		return (endTime - startTime);
}

#elif 0
/*#elif defined (__arm) && defined (__ARMCC_VERSION)*/

/* see definitions in ADSv1_2/bin/peripherals.ami */
#define TIMER_BASE		0x0a800000
#define TIMER_VALUE_1	(TIMER_BASE + 0x04)
#define TIMER_CONTROL_1	(TIMER_BASE + 0x08)

/* make sure this matches your ARMulator setting and the control word in InitTimer()!!! */	
#define CLOCKS_PER_SEC	(120000000 / 256)		

int InitTimer(void)
{
	volatile unsigned int *timerControl1 = (volatile unsigned int *)TIMER_CONTROL_1;
	unsigned int control1;
	
	/* see ARMulator Reference, pg 4-78 
	 * bits [3:2] = clock divisor factor (00 = 1, 01 = 16, 10 = 256, 11 = undef)
	 * bit  [6]   = free-running mode (0) or periodic mode (1)
	 * bit  [7]   = timer disabled (0) or enabled (1)
	 */
	control1 = 0x00000088;
	*timerControl1 = control1;
	
	return 0;
}

unsigned int ReadTimer(void)
{
	volatile unsigned int *timerValue1 = (volatile unsigned int *)TIMER_VALUE_1;
	unsigned int value;
	
	value = *timerValue1 & 0x0000ffff;
	
	return (unsigned int)value;
}

int FreeTimer(void)
{
    return 0;
}

unsigned int GetClockFrequency(void)
{
    return CLOCKS_PER_SEC;
}

unsigned int CalcTimeDifference(unsigned int startTime, unsigned int endTime)
{
	
    /* timer counts down int ARMulator, 16-bit counter */
    if (endTime > startTime)
		return (startTime + 0x00010000 - endTime);
    else
		return (startTime - endTime);
}

#else

int InitTimer(void)
{
	return -1;
}

unsigned int ReadTimer(void)
{
	return 0;
}

int FreeTimer(void)
{
    return 0;
}

unsigned int GetClockFrequency(void)
{
    return 0;
}

unsigned int CalcTimeDifference(unsigned int startTime, unsigned int endTime)
{
	
	return 0;
}

#endif

