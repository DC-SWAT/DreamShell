/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: noiseless.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * noiseless.c - decode channel info, scalefactors, quantized coefficients, 
 *                 scalefactor band codebook, and TNS coefficients from bitstream
 **************************************************************************************/

#include "coder.h"

/**************************************************************************************
 * Function:    DecodeNoiselessData
 *
 * Description: decode noiseless data (side info and transform coefficients)
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer pointing to start of individual channel stream
 *                (14496-3, table 4.4.24)
 *              pointer to bit offset
 *              pointer to number of valid bits remaining in buf
 *              index of current channel
 *
 * Outputs:     updated global gain, section data, scale factor data, pulse data,
 *                TNS data, gain control data, and spectral data
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int DecodeNoiselessData(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail, int ch)
{
	int startOffset, commonWin;
	unsigned char *startBuf;
	PSInfoBase *psi;
	IppStatus err;
	IppAACChanInfo *chanInfo;
	IppAACIcsInfo *icsInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	startBuf = *buf;
	startOffset = *bitOffset;

	if (aacDecInfo->currBlockID == AAC_ID_CPE)
		commonWin = psi->chanPairElt.commonWin;
	else
		commonWin = 0;

	/* fill in channel info members used as inputs (see IPP docs, table 10-7) */
	chanInfo = &(psi->chanInfo[ch]);
	chanInfo->samplingRateIndex = psi->sampRateIdx;
	chanInfo->predSfbMax = 0;
	if (commonWin == 1 && chanInfo->pIcsInfo->predDataPres == 0)
		chanInfo->pIcsInfo->predReset = 0;
	icsInfo = psi->chanInfo[ch].pIcsInfo;

	err = ippsNoiselessDecoder_LC_AAC(buf, bitOffset, commonWin, &(psi->chanInfo[ch]), 
		psi->scaleFactors[ch], psi->coef[ch], psi->sfbCodeBook[ch], psi->tnsCoef[ch]);

	/* MPEG-4 AAC version: 
	 *  - this is the primitive that should be used for MPEG-4 LC AAC (i.e. PNS present)
	 *    - the other version will return error if it sees codebook 13 (meaning PNS in that SFB)
	 *    - according to the docs, this version should be okay with cb == 13, but it still gives error
	 *  - currently the new one doesn't work properly (IPP 4.1 on Windows) if pulse data is present
	 *    - examples are compliance files l4 (MPEG-2) and al04 (MPEG-4) 
	 * 
	 * err = ippsNoiselessDecode_AAC(buf, bitOffset, &(psi->icsMainHeader[ch]), psi->scaleFactors[ch], 
	 *         psi->coef[ch], psi->sfbCodeBook[ch], psi->tnsCoef[ch], &(psi->chanInfo[ch]), 
	 *         icsInfo->winSequence, icsInfo->maxSfb, commonWin, 0, 2);
	 */
	if (err)
		return ERR_AAC_INVALID_FRAME;

	/* IPP does short-block deinterleaving as separate pass, after dequant and stereo processing */
	if (psi->chanInfo[ch].pIcsInfo->winSequence == 2)
		aacDecInfo->sbDeinterleaveReqd[ch] = 1;

	*bitsAvail -= ((*buf - startBuf) << 3) + *bitOffset - startOffset;
	if (*bitsAvail < 0)
		return ERR_AAC_INDATA_UNDERFLOW;

	return ERR_AAC_NONE;
}