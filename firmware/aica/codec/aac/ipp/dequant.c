/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dequant.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * dequant.c - wrapper for IPP primitives for dequantization and short block 
 *               deinterleaving
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    Dequantize
 *
 * Description: dequantize all transform coefficients for one channel
 *
 * Inputs:      valid AACDecInfo struct (including unpacked, quantized coefficients)
 *              index of current channel
 *
 * Outputs:     dequantized coefficients
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
int Dequantize(AACDecInfo *aacDecInfo, int ch)
{
	PSInfoBase *psi;
	IppStatus err;
	IppAACIcsInfo *icsInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	icsInfo = psi->chanInfo[ch].pIcsInfo;

	err = ippsQuantInv_AAC_32s_I(psi->coef[ch], psi->scaleFactors[ch], icsInfo->numWinGrp, icsInfo->pWinGrpLen, 
			icsInfo->maxSfb, psi->sfbCodeBook[ch], psi->sampRateIdx, psi->chanInfo[ch].winLen);
	if (err)
		return -1;

	return 0;
}

/**************************************************************************************
 * Function:    DeinterleaveShortBlocks
 *
 * Description: deinterleave transform coefficients in short blocks for one channel
 *
 * Inputs:      valid AACDecInfo struct (including unpacked, quantized coefficients)
 *              index of current channel
 *
 * Outputs:     deinterleaved coefficients (window groups into 8 separate windows)
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
int DeinterleaveShortBlocks(AACDecInfo *aacDecInfo, int ch)
{
	int i;
	PSInfoBase *psi;
	IppStatus err;
	IppAACIcsInfo *icsInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	icsInfo = psi->chanInfo[ch].pIcsInfo;

	if (icsInfo->winSequence == 2) {
		err = ippsDeinterleaveSpectrum_AAC_32s(psi->coef[ch], psi->workBuf, icsInfo->numWinGrp, icsInfo->pWinGrpLen, 
				icsInfo->maxSfb, psi->sampRateIdx, psi->chanInfo[ch].winLen);
 		if (err)
			return -1;
		
		/* copy back from temp buffer into coef buffer */
		for (i = 0; i < AAC_MAX_NSAMPS; i++)
			psi->coef[ch][i] = psi->workBuf[i];
	}

	return 0;
}