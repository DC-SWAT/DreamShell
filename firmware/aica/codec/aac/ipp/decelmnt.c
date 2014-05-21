/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: decelmnt.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * decelmnt.c - syntactic element decoding
 **************************************************************************************/

#include "coder.h"

/**************************************************************************************
 * Function:    GetBitsQuick
 *
 * Description: very simple bitstream reader
 *
 * Inputs:      number of bits to get (max 7 bits)
 *              double pointer to buffer
 *              pointer to offset [0, 7] into current byte
 *
 * Outputs:     updated buffer pointer and bit offset
 *
 * Return:      nBits of data from bitstream buffer
 *
 * Notes:       nBits must be in range [0, 7], nBits outside this range masked by 0x07
 *              this is restricted to reading <= 7 bits because the IPP primitives
 *                do the bulk of bitstream parsing themselves
 *              this function just gives a simple way to get a few bits such as the 
 *                for the element ID or instance tag
 **************************************************************************************/
static unsigned int GetBitsQuick(int nBits, unsigned char **buf, int *bitOffset)
{
	unsigned char *b;
	int off;
	unsigned int data;

	nBits &= 0x07;
	if (!nBits)
		return 0;

	b = *buf;
	off = *bitOffset;
	if (off + nBits < 8) {
		/* requested bits are completely within current byte */
		data = (unsigned int)(*b);
		data <<= (24 + off);
	} else {
		/* requested bits span current byte and next byte */
		data  = (unsigned int)(*b++) << 8;
		data |= (unsigned int)(*b);
		data <<= (16 + off);
	}
	data >>= (32 - nBits);
	off = (off + nBits) & 0x07;

	*buf = b;
	*bitOffset = off;
	return (unsigned int)data;
}

/**************************************************************************************
 * Function:    DecodeSingleChannelElement
 *
 * Description: decode one SCE
 *
 * Inputs:      double pointer to buffer containing SCE (14496-3, table 4.4.4) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeSingleChannelElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	PSInfoBase *psi;
	IppAACChanInfo *chanInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	chanInfo = psi->chanInfo;

	/* read instance tag */
	aacDecInfo->currInstTag = GetBitsQuick(NUM_INST_TAG_BITS, buf, bitOffset);

	chanInfo[0].tag = aacDecInfo->currInstTag;
	chanInfo[0].id = aacDecInfo->currBlockID;
	chanInfo[0].samplingRateIndex = psi->sampRateIdx;
	chanInfo[0].pIcsInfo = psi->icsInfo;

	return 0;
}

/**************************************************************************************
 * Function:    DecodeChannelPairElement
 *
 * Description: decode one CPE
 *
 * Inputs:      double pointer to buffer containing CPE (14496-3, table 4.4.5) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *              updated commonWin
 *              updated ICS info, if commonWin == 1
 *              updated mid-side stereo info, if commonWin == 1
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeChannelPairElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	int i;
	PSInfoBase *psi;
	IppStatus err;
	IppAACChanPairElt *chanPairElt;
	IppAACChanInfo *chanInfo;
	IppAACIcsInfo *icsInfo;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	chanPairElt = &(psi->chanPairElt);
	chanInfo = psi->chanInfo;
	icsInfo = psi->icsInfo;

	/* read instance tag */
	aacDecInfo->currInstTag = GetBitsQuick(NUM_INST_TAG_BITS, buf, bitOffset);
	
	/* fill inf channel info struct for both channels */
	for (i = 0; i < 2; i++) {
		chanInfo[i].tag = aacDecInfo->currInstTag;
		chanInfo[i].id = aacDecInfo->currBlockID;
		chanInfo[i].samplingRateIndex = psi->sampRateIdx;
		chanInfo[i].predSfbMax = 0;
		chanInfo[i].pChanPairElt = chanPairElt;
	}

	/* parse channel pair element */
	err = ippsDecodeChanPairElt_AAC(buf, bitOffset, icsInfo, chanPairElt, 0);
	if (err)
		return -1;

	chanInfo[0].pIcsInfo = icsInfo + 0;
	if (chanPairElt->commonWin == 1)
		chanInfo[1].pIcsInfo = icsInfo + 0;
	else
		chanInfo[1].pIcsInfo = icsInfo + 1;

	return 0;
}

/**************************************************************************************
 * Function:    DecodeLFEChannelElement
 *
 * Description: decode one LFE
 *
 * Inputs:      double pointer to buffer containing LFE (14496-3, table 4.4.9) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeLFEChannelElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;

	/* read instance tag */
	aacDecInfo->currInstTag = GetBitsQuick(NUM_INST_TAG_BITS, buf, bitOffset);

	return 0;
}

/**************************************************************************************
 * Function:    DecodeDataStreamElement
 *
 * Description: decode one DSE
 *
 * Inputs:      double pointer to buffer containing DSE (14496-3, table 4.4.10) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *              filled in data stream buffer
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
static int DecodeDataStreamElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	PSInfoBase *psi;
	IppStatus err;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

	/* parse data stream element (including instance tag) */
	err = ippsDecodeDatStrElt_AAC(buf, bitOffset, &(aacDecInfo->currInstTag), &(psi->dataCount), psi->dataBuf);
	if (err)
		return -1;

	return 0;
}

/**************************************************************************************
 * Function:    DecodeProgramConfigElement
 *
 * Description: decode one PCE
 *
 * Inputs:      double pointer to buffer containing PCE (14496-3, table 4.4.2) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *              filled-in ProgConfigElement struct
 *              updated BitStreamInfo struct
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
static int DecodeProgramConfigElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	PSInfoBase *psi;
	IppStatus err;
	IppAACPrgCfgElt *progCfgElt;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	progCfgElt = &(psi->progCfgElt);

	/* parse data stream element (including instance tag) */
	err = ippsDecodePrgCfgElt_AAC(buf, bitOffset, progCfgElt);
	if (err)
		return -1;
	aacDecInfo->currInstTag = progCfgElt->eltInsTag;

	return 0;
}

/**************************************************************************************
 * Function:    DecodeFillElement
 *
 * Description: decode one fill element
 *
 * Inputs:      double pointer to buffer containing fill element (14496-3, table 4.4.2) 
 *              pointer to bit offset
 *
 * Outputs:     updated element instance tag
 *              updated buffer pointer
 *              updated bit offset
 *              unpacked extension payload
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
static int DecodeFillElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset)
{
	PSInfoBase *psi;
	IppStatus err;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return -1;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

	/* parse fill element */
	err = ippsDecodeFillElt_AAC(buf, bitOffset, &(psi->fillCount), psi->fillBuf);
	if (err)
		return -1;

	aacDecInfo->currInstTag = -1;	/* fill elements don't have instance tag */
	aacDecInfo->fillExtType = 0;

#ifdef AAC_ENABLE_SBR
	/* check for SBR 
	 * aacDecInfo->sbrEnabled is sticky (reset each raw_data_block), so for multichannel 
	 *    need to verify that all SCE/CPE/ICCE have valid SBR fill element following, and 
	 *    must upsample by 2 for LFE
	 */
	if (psi->fillCount > 0) {
		aacDecInfo->fillExtType = (int)((psi->fillBuf[0] >> 4) & 0x0f);
		if (aacDecInfo->fillExtType == EXT_SBR_DATA || aacDecInfo->fillExtType == EXT_SBR_DATA_CRC)
			aacDecInfo->sbrEnabled = 1;
	}
#endif

	aacDecInfo->fillBuf = psi->fillBuf;
	aacDecInfo->fillCount = psi->fillCount;

	return 0;
}

/**************************************************************************************
 * Function:    DecodeNextElement
 *
 * Description: decode next syntactic element in AAC frame
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer containing next element
 *              pointer to bit offset
 *              pointer to number of valid bits remaining in buf
 *
 * Outputs:     type of element decoded (aacDecInfo->currBlockID)
 *              type of element decoded last time (aacDecInfo->prevBlockID)
 *              updated aacDecInfo state, depending on which element was decoded
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int DecodeNextElement(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail)
{
	int err, startOffset;
	unsigned char *startBuf;
	PSInfoBase *psi;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	startBuf = *buf;
	startOffset = *bitOffset;

	/* read element ID (save last ID for SBR purposes) */
	aacDecInfo->prevBlockID = aacDecInfo->currBlockID;
	aacDecInfo->currBlockID = GetBitsQuick(NUM_SYN_ID_BITS, buf, bitOffset);

	err = 0;
	switch (aacDecInfo->currBlockID) {
	case AAC_ID_SCE:
		err = DecodeSingleChannelElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_CPE:
		err = DecodeChannelPairElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_CCE:
		/* TODO - implement CCE decoding */
		break;
	case AAC_ID_LFE:
		err = DecodeLFEChannelElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_DSE:
		err = DecodeDataStreamElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_PCE:
		err = DecodeProgramConfigElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_FIL:
		err = DecodeFillElement(aacDecInfo, buf, bitOffset);
		break;
	case AAC_ID_END:
		break;
	}
	if (err)
		return ERR_AAC_SYNTAX_ELEMENT;

	*bitsAvail -= ((*buf - startBuf) << 3) + *bitOffset - startOffset;
	if (*bitsAvail < 0)
		return ERR_AAC_INDATA_UNDERFLOW;

	return ERR_AAC_NONE;
}

