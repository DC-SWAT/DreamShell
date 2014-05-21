/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: filefmt.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
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
 * bitstream.c - bitstream parsing functions
 **************************************************************************************/

#include "coder.h"

/**************************************************************************************
 * Function:    UnpackADTSHeader
 *
 * Description: parse the ADTS frame header and initialize decoder state
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer with complete ADTS frame header (byte aligned)
 *                header size = 7 bytes, plus 2 if CRC
 *
 * Outputs:     filled in ADTS struct
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * TODO:        test CRC
 *              verify that fixed fields don't change between frames
 **************************************************************************************/
int UnpackADTSHeader(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail)
{
	int bytesRead;
	PSInfoBase *psi;
	Ipp8u *pBitStream;
	IppAACADTSFrameHeader *fhADTS;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;

	/* prepare for header read */
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	pBitStream = (Ipp8u *)(*buf);
	fhADTS = &(psi->fhADTS);
	bytesRead = ADTS_HEADER_BYTES;

	/* unpack ADTS frame header (returns error if pBitStream not pointing to syncword) */
	if (ippsUnpackADTSFrameHeader_AAC(&pBitStream, fhADTS))
		return ERR_AAC_INVALID_ADTS_HEADER;

	/* skip CRC word (16 bits) */
	if (fhADTS->protectionBit == 0)
		bytesRead += 2;

	/* check validity of header */
	if (fhADTS->layer != 0 || fhADTS->profile != AAC_PROFILE_LC ||
		fhADTS->samplingRateIndex < 0 || fhADTS->samplingRateIndex >= NUM_SAMPLE_RATES ||
		fhADTS->chConfig < 0 || fhADTS->chConfig >= NUM_DEF_CHAN_MAPS)
		return ERR_AAC_INVALID_ADTS_HEADER;

#ifndef AAC_ENABLE_MPEG4
	if (fhADTS->id != 1)
		return ERR_AAC_MPEG4_UNSUPPORTED;
#endif

	/* update codec info */
	psi->sampRateIdx = fhADTS->samplingRateIndex;
	if (!psi->useImpChanMap)
		psi->nChans = channelMapTab[fhADTS->chConfig];
	
	/* syntactic element fields will be read from bitstream for each element */
	aacDecInfo->prevBlockID = AAC_ID_INVALID;
	aacDecInfo->currBlockID = AAC_ID_INVALID;
	aacDecInfo->currInstTag = -1;

	/* fill in user-accessible data (TODO - calc bitrate, handle tricky channel config cases, see section 8.5) */
	aacDecInfo->bitRate = 0;
	aacDecInfo->nChans = psi->nChans;
	aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];
	aacDecInfo->profile = fhADTS->profile;
	aacDecInfo->sbrEnabled = 0;
	aacDecInfo->adtsBlocksLeft = fhADTS->numRawBlock;

	*buf += bytesRead;
	*bitsAvail -= (bytesRead << 3);
	if (*bitsAvail < 0)
		return ERR_AAC_INDATA_UNDERFLOW;

	return ERR_AAC_NONE;
}

/**************************************************************************************
 * Function:    GetADTSChannelMapping
 *
 * Description: determine the number of channels from implicit mapping rules
 *
 * Inputs:      valid AACDecInfo struct
 *              pointer to start of raw_data_block
 *              bit offset
 *              bits available 
 *
 * Outputs:     updated number of channels
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       calculates total number of channels using rules in 14496-3, 4.5.1.2.1
 *              does not attempt to deduce speaker geometry
 **************************************************************************************/
int GetADTSChannelMapping(AACDecInfo *aacDecInfo, unsigned char *buf, int bitOffset, int bitsAvail)
{
	int ch, nChans, elementChans, err;
	PSInfoBase *psi;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

	nChans = 0;
	do {
		/* parse next syntactic element */
		err = DecodeNextElement(aacDecInfo, &buf, &bitOffset, &bitsAvail);
		if (err)
			return err;

		elementChans = elementNumChans[aacDecInfo->currBlockID];
		nChans += elementChans;

		for (ch = 0; ch < elementChans; ch++) {
			err = DecodeNoiselessData(aacDecInfo, &buf, &bitOffset, &bitsAvail, ch);
			if (err)
				return err;
		}
	} while (aacDecInfo->currBlockID != AAC_ID_END);

	if (nChans <= 0)
		return ERR_AAC_CHANNEL_MAP;

	/* update number of channels in codec state and user-accessible info structs */ 
	psi->nChans = nChans;
	aacDecInfo->nChans = psi->nChans;
	psi->useImpChanMap = 1;

	return ERR_AAC_NONE;
}

/**************************************************************************************
 * Function:    GetNumChannelsADIF
 *
 * Description: get number of channels from program config elements in an ADIF file
 *
 * Inputs:      array of filled-in program config element structures
 *              number of PCE's
 *
 * Outputs:     none
 *
 * Return:      total number of channels in file
 *              -1 if error (invalid number of PCE's or unsupported mode)
 **************************************************************************************/
static int GetNumChannelsADIF(IppAACPrgCfgElt *fhPCE, int nPCE)
{
	int i, j, nChans;

	if (nPCE < 1 || nPCE > MAX_NUM_PCE_ADIF)
		return -1;

	nChans = 0;
	for (i = 0; i < nPCE; i++) {
		/* for now: only support LC, no channel coupling */
		if (fhPCE[i].profile != AAC_PROFILE_LC || fhPCE[i].numValidCcElt > 0)
			return -1;

		/* add up number of channels in all channel elements (assume all single-channel) */
        nChans += fhPCE[i].numFrontElt;
        nChans += fhPCE[i].numSideElt;
        nChans += fhPCE[i].numBackElt;
        nChans += fhPCE[i].numLfeElt;

		/* add one more for every element which is a channel pair */
        for (j = 0; j < fhPCE[i].numFrontElt; j++) {
            if (fhPCE[i].pFrontIsCpe[j])
                nChans++;
        }
        for (j = 0; j < fhPCE[i].numSideElt; j++) {
            if (fhPCE[i].pSideIsCpe[j])
                nChans++;
        }
        for (j = 0; j < fhPCE[i].numBackElt; j++) {
            if (fhPCE[i].pBackIsCpe[j])
                nChans++;
        }

	}

	return nChans;
}

/**************************************************************************************
 * Function:    GetSampleRateIdxADIF
 *
 * Description: get sampling rate index from program config elements in an ADIF file
 *
 * Inputs:      array of filled-in program config element structures
 *              number of PCE's
 *
 * Outputs:     none
 *
 * Return:      sample rate of file
 *              -1 if error (invalid number of PCE's or sample rate mismatch)
 **************************************************************************************/
static int GetSampleRateIdxADIF(IppAACPrgCfgElt *fhPCE, int nPCE)
{
	int i, idx;

	if (nPCE < 1 || nPCE > MAX_NUM_PCE_ADIF)
		return -1;

	/* make sure all PCE's have the same sample rate */
	idx = fhPCE[0].samplingRateIndex;
	for (i = 1; i < nPCE; i++) {
		if (fhPCE[i].samplingRateIndex != idx)
			return -1;
	}

	return idx;
}

/**************************************************************************************
 * Function:    UnpackADIFHeader
 *
 * Description: parse the ADIF file header and initialize decoder state
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer with complete ADIF header 
 *                (starting at 'A' in 'ADIF' tag)
 *              pointer to bit offset
 *              pointer to number of valid bits remaining in inbuf
 *
 * Outputs:     filled-in ADIF struct
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int UnpackADIFHeader(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail)
{
	int bytesRead;
	PSInfoBase *psi;
	Ipp8u *pBitStream;
	IppAACADIFHeader *fhADIF;
	IppAACPrgCfgElt *fhPCE;

	/* validate pointers and sync word */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;

	/* prepare for header read */
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	pBitStream = (Ipp8u *)(*buf);
	fhADIF = &(psi->fhADIF);
	fhPCE = psi->fhPCE;

	/* unpack ADIF file header */
	if (ippsUnpackADIFHeader_AAC(&pBitStream, fhADIF, fhPCE, MAX_NUM_PCE_ADIF))
		return ERR_AAC_INVALID_ADIF_HEADER;
	bytesRead = pBitStream - *buf;

	/* update codec info */
	psi->nChans = GetNumChannelsADIF(fhPCE, fhADIF->numPrgCfgElt);
	psi->sampRateIdx = GetSampleRateIdxADIF(fhPCE, fhADIF->numPrgCfgElt);

	/* check validity of header */
	if (psi->nChans < 0 || psi->sampRateIdx < 0 || psi->sampRateIdx >= NUM_SAMPLE_RATES)
		return ERR_AAC_INVALID_ADIF_HEADER;
								
	/* syntactic element fields will be read from bitstream for each element */
	aacDecInfo->prevBlockID = AAC_ID_INVALID;
	aacDecInfo->currBlockID = AAC_ID_INVALID;
	aacDecInfo->currInstTag = -1;

	/* fill in user-accessible data */
	aacDecInfo->bitRate = 0;
	aacDecInfo->nChans = psi->nChans;
	aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];
	aacDecInfo->profile = fhPCE[0].profile;
	aacDecInfo->sbrEnabled = 0;

	*buf += bytesRead;
	*bitsAvail -= (bytesRead << 3);
	if (*bitsAvail < 0)
		return ERR_AAC_INDATA_UNDERFLOW;

	return ERR_AAC_NONE;
}

/**************************************************************************************
 * Function:    SetRawBlockParams
 *
 * Description: set internal state variables for decoding a stream of raw data blocks
 *
 * Inputs:      valid AACDecInfo struct
 *              flag indicating source of parameters (from previous headers or passed 
 *                explicitly by caller)
 *              number of channels
 *              sample rate
 *              profile ID
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       if copyLast == 1, then psi->nChans, psi->sampRateIdx, and 
 *                aacDecInfo->profile are not changed (it's assumed that we already 
 *                set them, such as by a previous call to UnpackADTSHeader())
 *              if copyLast == 0, then the parameters we passed in are used instead
 **************************************************************************************/
int SetRawBlockParams(AACDecInfo *aacDecInfo, int copyLast, int nChans, int sampRate, int profile)
{
	int idx;
	PSInfoBase *psi;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

	if (!copyLast) {
		aacDecInfo->profile = profile;
		psi->nChans = nChans;
		for (idx = 0; idx < NUM_SAMPLE_RATES; idx++) {
			if (sampRate == sampRateTab[idx]) {
				psi->sampRateIdx = idx;
				break;
			}
		}
		if (idx == NUM_SAMPLE_RATES)
			return ERR_AAC_INVALID_FRAME;
	}
	aacDecInfo->nChans = psi->nChans;
	aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];

	/* check validity of header */
	if (psi->sampRateIdx >= NUM_SAMPLE_RATES || psi->sampRateIdx < 0 || 
		aacDecInfo->nChans > AAC_MAX_NCHANS || aacDecInfo->nChans <= 0 || aacDecInfo->profile != AAC_PROFILE_LC)
		return ERR_AAC_INVALID_ADTS_HEADER;

	return ERR_AAC_NONE;
}

/**************************************************************************************
 * Function:    PrepareRawBlock
 *
 * Description: reset per-block state variables for raw blocks (no ADTS/ADIF headers)
 *
 * Inputs:      valid AACDecInfo struct
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int PrepareRawBlock(AACDecInfo *aacDecInfo)
{
	PSInfoBase *psi;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

	/* syntactic element fields will be read from bitstream for each element */
	aacDecInfo->prevBlockID = AAC_ID_INVALID;
	aacDecInfo->currBlockID = AAC_ID_INVALID;
	aacDecInfo->currInstTag = -1;

	/* fill in user-accessible data */
	aacDecInfo->bitRate = 0;
	aacDecInfo->sbrEnabled = 0;

	return ERR_AAC_NONE;
}

/**************************************************************************************
 * Function:    FlushCodec
 *
 * Description: flush internal codec state (after seeking, for example)
 *
 * Inputs:      valid AACDecInfo struct
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       only need to clear data which is persistent between frames 
 *                (such as overlap buffer)
 **************************************************************************************/
int FlushCodec(AACDecInfo *aacDecInfo)
{
	PSInfoBase *psi;

	/* validate pointers */
	if (!aacDecInfo || !aacDecInfo->psInfoBase)
		return ERR_AAC_NULL_POINTER;
	psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
	
	ClearBuffer(psi->overlap, AAC_MAX_NCHANS * AAC_MAX_NSAMPS*sizeof(Ipp32s));
	ClearBuffer(psi->prevWinShape, AAC_MAX_NCHANS * sizeof(int));

	return ERR_AAC_NONE;
}
