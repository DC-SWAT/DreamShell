/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: debug.c,v 1.2 2005/07/05 21:08:13 ehyche Exp $ 
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
 * debug.c - implementations of memory testing functions
 **************************************************************************************/

#include "wrapper.h"

#if !defined (_DEBUG)

void DebugMemCheckInit(void)
{
}

void DebugMemCheckStartPoint(void)
{
}

void DebugMemCheckEndPoint(void)
{
}

void DebugMemCheckFree(void)
{
}

#elif defined (_WIN32) && !defined (_WIN32_WCE)

#include <stdio.h>
#include <crtdbg.h>

#ifdef FORTIFY
#include "fortify.h"
#else
static 	_CrtMemState oldState, newState, stateDiff;
#endif

void DebugMemCheckInit(void)
{
	int tmpDbgFlag;

	/* Send all reports to STDOUT */
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );	

	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
}

void DebugMemCheckStartPoint(void)
{
#ifdef FORTIFY
	Fortify_EnterScope();
#else
	_CrtMemCheckpoint(&oldState);
#endif
}

void DebugMemCheckEndPoint(void)
{
#ifdef FORTIFY
		Fortify_LeaveScope();
#else
		_CrtMemCheckpoint(&newState);
		_CrtMemDifference(&stateDiff, &oldState, &newState);
		_CrtMemDumpStatistics(&stateDiff);
#endif
}

void DebugMemCheckFree(void)
{
	printf("\n");
	if (!_CrtDumpMemoryLeaks())
		printf("Memory leak test:      no leaks\n");

	if (!_CrtCheckMemory())
		printf("Memory integrity test: error!\n");
	else 
		printf("Memory integrity test: okay\n");
}

#elif defined (__arm) && defined (__ARMCC_VERSION)

void DebugMemCheckInit(void)
{
}

void DebugMemCheckStartPoint(void)
{
}

void DebugMemCheckEndPoint(void)
{
}

void DebugMemCheckFree(void)
{
}

#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__powerpc__))

void DebugMemCheckInit(void)
{
}

void DebugMemCheckStartPoint(void)
{
}

void DebugMemCheckEndPoint(void)
{
}

void DebugMemCheckFree(void)
{
}

#endif
