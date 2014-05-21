/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: imdct.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * imdct.c - inverse MDCT
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    IMDCT
 *
 * Description: inverse transform and convert to 16-bit PCM
 *
 * Inputs:      valid AACDecInfo struct
 *              index of current channel (0 for SCE/LFE, 0 or 1 for CPE)
 *              output channel (range = [0, nChans-1])
 *
 * Outputs:     complete frame of decoded PCM, after inverse transform
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       interleaves samples for nChans > 1, unless SBR is enabled
 *              if SBR is enabled, do not interleave channels
 **************************************************************************************/
int IMDCT(AACDecInfo *aacDecInfo, int ch, int chOut, short *outbuf)
{
	int i, pcmMode, iFactor;
	short *outptr;
	PSInfoBase *psi;
	IppStatus err;
	IppAACChanInfo *chanInfo;
	IppAACIcsInfo *icsInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	chanInfo = &(psi->chanInfo[ch]); 
	icsInfo = chanInfo->pIcsInfo;

	iFactor = aacDecInfo->nChans;	/* interleave by total number of channels in stream */
	pcmMode = (aacDecInfo->nChans == 2 ? 2 : 1);
	outbuf += chOut;

	/* interleave LRLRLR... only if nChans == 2 (for multichannel must do separate pass) */
	outptr = (iFactor <= 2 ? outbuf : (short *)psi->workBuf);

	err = ippsMDCTInv_AAC_32s16s(psi->coef[ch], outptr, psi->overlap[chOut], 
		icsInfo->winSequence, icsInfo->winShape, psi->prevWinShape[chOut], pcmMode);
	psi->prevWinShape[chOut] = icsInfo->winShape;

	if (iFactor > 2) {
		for (i = 0; i < AAC_MAX_NSAMPS; i++) {
			*outbuf = outptr[i];
			outbuf += iFactor;
		}
		outbuf -= (iFactor*AAC_MAX_NSAMPS);
	}

#ifdef AAC_ENABLE_SBR
	/* if SBR is enabled, convert 16-bit PCM back to 32-bit ints */
	for (i = 0; i < AAC_MAX_NSAMPS; i++) {
		psi->coef[ch][i] = (int)(*outbuf);
		outbuf += iFactor;
	}

	aacDecInfo->rawSampleBuf[ch] = psi->coef[ch];
	aacDecInfo->rawSampleBytes = sizeof(int);
	aacDecInfo->rawSampleFBits = 0;
#else
	aacDecInfo->rawSampleBuf[ch] = 0;
	aacDecInfo->rawSampleBytes = 0;
	aacDecInfo->rawSampleFBits = 0;
#endif

 	if (err)
		return -1;

	return 0;
}
