/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: aacdecdll.h,v 1.3 2005/05/19 20:43:21 jrecker Exp $ 
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

#ifndef AACDECDLL_H
#define AACDECDLL_H

#include "aacdec.h"

class CAACDec : public IHXAudioDecoder,
                public CHXBaseCountingObject
{
public:
    CAACDec();
    virtual ~CAACDec();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXAudioDecoder methods
    STDMETHOD(OpenDecoder)      (THIS_ UINT32 cfgType, const void* config, UINT32 nBytes);
    STDMETHOD(Reset)            (THIS);
    STDMETHOD(Conceal)          (THIS_ UINT32 nSamples);
    STDMETHOD(Decode)           (THIS_ const UCHAR* data, UINT32 nBytes, UINT32 &nBytesConsumed,
                                       INT16 *samplesOut, UINT32& nSamplesOut, HXBOOL eof);
    STDMETHOD(GetMaxSamplesOut) (THIS_ UINT32& nSamples) CONSTMETHOD;
    STDMETHOD(GetNChannels)     (THIS_ UINT32& nChannels) CONSTMETHOD ;
    STDMETHOD(GetSampleRate)    (THIS_ UINT32& sampleRate) CONSTMETHOD;
    STDMETHOD(GetDelay)         (THIS_ UINT32& nSamples) CONSTMETHOD;

    // helper functions for old interface / compatibility shim layer.
    void*     GetFlavorProperty(UINT16 flvIndex, UINT16 propIndex, UINT16* pSize);
    HX_RESULT _RAInitDecoder(RADECODER_INIT_PARAMS* pParam);
    HX_RESULT _RADecode(const UCHAR* in, UINT32 inLength, INT16* out, UINT32* pOutLength, UINT32 userData);
protected:
    INT32              m_lRefCount;

	HAACDecoder        *m_hAACDecoder;
	AACFrameInfo       m_aacFrameInfo;

    UINT32             m_ulCoreSampleRate;
    UINT32             m_ulSampleRate;
    UINT32             m_ulChannels;
    UINT32             m_ulFrameLength;
    UINT32             m_ulMaxFrameLength;
    // the next decode() call will conceal up to this many samples.
    UINT32             m_ulSamplesToConceal;
    // the next decode() call will throw away up to this many samples.
    // this is needed in the compatibility layer, because the old API
    // assumed that the decoder eats the delay.
    UINT32             m_ulSamplesToEat;
    // use this to store a property returned through GetFlavorProperty
    UINT32             m_propertyStore;

    static const char* const m_pszCodecName;

private:
	void ReorderPCMChannels(INT16 *pcmBuf, int nSamps, int nChans);
};

#endif /* #ifndef AACDECDLL */
