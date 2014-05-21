/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: coder.h,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * Fixed-point AAC decoder using Intel IPP libraries
 * Jon Recker (jrecker@real.com)
 * February 2005
 *
 * coder.h - definitions of data structures for IPP wrapper functions
 **************************************************************************************/

#ifndef _CODER_H
#define _CODER_H

#include "aaccommon.h"

#if defined(_WIN32) && defined(_M_IX86)
#include "../tools/staticlib/ipp_px.h"	/* use static IPP libraries */
#endif

/* IPP definitions */
#include "ippAC.h"

#if defined(_WIN32) && defined(_M_IX86) && (defined (_DEBUG) || defined (REL_ENABLE_ASSERTS))
#define ASSERT(x) if (!(x)) __asm int 3;
#else
#define ASSERT(x) /* do nothing */
#endif

#define DATA_BUF_SIZE	512
#define FILL_BUF_SIZE	270

typedef struct _PSInfoBase {
	IppAACADTSFrameHeader fhADTS;
	IppAACADIFHeader      fhADIF;
	IppAACPrgCfgElt       fhPCE[MAX_NUM_PCE_ADIF];
	IppAACChanPairElt     chanPairElt;
	IppAACChanInfo        chanInfo[MAX_NCHANS_ELEM];
	IppAACIcsInfo         icsInfo[MAX_NCHANS_ELEM];
	IppAACMainHeader      icsMainHeader[MAX_NCHANS_ELEM];
	int                   dataCount;
    Ipp8u                 dataBuf[DATA_BUF_SIZE];
	IppAACPrgCfgElt       progCfgElt;
	int                   fillCount;
    Ipp8u                 fillBuf[FILL_BUF_SIZE];
	Ipp32s                workBuf[AAC_MAX_NSAMPS];
    int                   ltpFlag[IPP_AAC_MAX_LTP_SFB];
	int                   seed;

	/* state information which is the same throughout whole frame */
	int                   nChans;
	int                   useImpChanMap;
	int                   sampRateIdx;

	/* these can be reused for subsequent bitstream elements in a single data block */
	Ipp16s                scaleFactors[MAX_NCHANS_ELEM][MAX_SF_BANDS];
	Ipp8u                 sfbCodeBook[MAX_NCHANS_ELEM][MAX_SF_BANDS];
	Ipp8s                 tnsCoef[MAX_NCHANS_ELEM][MAX_TNS_COEFS];
	Ipp32s                coef[MAX_NCHANS_ELEM][AAC_MAX_NSAMPS];

	/* need one overlap buffer per channel */
	Ipp32s                overlap[AAC_MAX_NCHANS][AAC_MAX_NSAMPS];
	int                   prevWinShape[AAC_MAX_NCHANS];

} PSInfoBase;

/* private functions */
unsigned int GetBits(int nBits, unsigned char **buf, int *bitOffset);

#endif	/* _CODER_H */

