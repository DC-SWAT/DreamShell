/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: aacdecdll.cpp,v 1.5 2005/09/27 20:30:02 jrecker Exp $ 
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxassert.h"
#include "hxacodec.h"
#include "gaConfig.h"
#include "racodec.h"
#include "baseobj.h"
#include "aacdecdll.h"
#include "aacconstants.h"

const char* const CAACDec::m_pszCodecName = "RealAudio 10";

CAACDec::CAACDec()
    : m_lRefCount(0)
    , m_hAACDecoder(0)
    , m_ulCoreSampleRate(0)
    , m_ulSampleRate(0)
    , m_ulChannels(0)
    , m_ulFrameLength(0)
    , m_ulMaxFrameLength(0)
    , m_ulSamplesToConceal(0)
    , m_ulSamplesToEat(0)
    , m_propertyStore(0)
{
    memset(&m_aacFrameInfo, 0, sizeof(AACFrameInfo));
}

CAACDec::~CAACDec()
{
    if (m_hAACDecoder)
        AACFreeDecoder(m_hAACDecoder);
    m_hAACDecoder = 0;
}

/****************************************************************************
 *  IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//
STDMETHODIMP CAACDec::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXAudioDecoder))
    {
        AddRef();
        *ppvObj = (IHXAudioDecoder*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CAACDec::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//
STDMETHODIMP_(ULONG32) CAACDec::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/* configure the decoder. Needs to be called before starting to decode.

cfgType: The type of configuration data. For AAC decoders, the following values
are defined:
             
eAACConfigADTS:             an ADTS-framed frame.
eAACConfigAudioSpecificCfg: an mp4 audio specific config.

hdr: a pointer to the config data.
nBytes: the number of bytes in the config data.
*/

STDMETHODIMP CAACDec::OpenDecoder(UINT32 cfgType,
                                  const void* config,
                                  UINT32 nBytes)
{
    unsigned char *readPtr;
    int bytesLeft, err;
    INT16 *temp;
    UINT32 audioObjType;

    /* for MP4 bitstream parsing */
    struct BITSTREAM *pBs = 0;
    CAudioSpecificConfig ASConfig;

    readPtr = (UCHAR *)config;
    bytesLeft = nBytes;

    switch (cfgType) {
       case eAACConfigADTS:

           /* allocate temp output buffer for decoding first frame of ADTS (double in case of SBR) */
#ifdef AAC_ENABLE_SBR
           temp = new INT16[AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2];
#else
           temp = new INT16[AAC_MAX_NSAMPS * AAC_MAX_NCHANS];
#endif
           if (!temp)
               return HXR_FAIL;

           m_hAACDecoder = (HAACDecoder *)AACInitDecoder();
           if (!m_hAACDecoder) {
               delete[] temp;
               return HXR_FAIL;
           }

           err = AACDecode(m_hAACDecoder, &readPtr, &bytesLeft, temp);
           if (err) {
               AACFreeDecoder(m_hAACDecoder);
               m_hAACDecoder = 0;
               delete[] temp;
               return HXR_FAIL;
           }
           delete[] temp;

           AACGetLastFrameInfo(m_hAACDecoder, &m_aacFrameInfo);

           m_ulCoreSampleRate = m_aacFrameInfo.sampRateCore;
           m_ulSampleRate = m_aacFrameInfo.sampRateOut;
           m_ulChannels = m_aacFrameInfo.nChans;
           m_ulFrameLength = m_aacFrameInfo.outputSamps / m_aacFrameInfo.nChans;
           m_ulMaxFrameLength = m_ulFrameLength;

           /* set AAC_MAX_NCHANS in aacdec.h */
           if (m_ulChannels > AAC_MAX_NCHANS) {
               AACFreeDecoder(m_hAACDecoder);
               m_hAACDecoder = 0;
               return HXR_FAIL;
           }

           /* setup decoder to handle raw data blocks (use ADTS params) */
           AACSetRawBlockParams(m_hAACDecoder, 1, &m_aacFrameInfo);

           return HXR_OK;

       case eAACConfigAudioSpecificCfg:
           if (newBitstream(&pBs, 8*nBytes))
               return HXR_FAIL;

           feedBitstream(pBs, (const unsigned char *)config, nBytes*8);
           setAtBitstream(pBs, 0, 1);

           if (ASConfig.Read(*pBs))
               return HXR_FAIL;

           if (pBs)
               deleteBitstream(pBs);

           m_ulChannels       = ASConfig.GetNChannels();
           m_ulCoreSampleRate = ASConfig.GetCoreSampleRate();
#ifdef AAC_ENABLE_SBR
           m_ulSampleRate  = ASConfig.GetSampleRate();
#else
           m_ulSampleRate  = m_ulCoreSampleRate;
#endif

           /* set AAC_MAX_NCHANS in aacdec.h */
           if (m_ulChannels > AAC_MAX_NCHANS) {
               AACFreeDecoder(m_hAACDecoder);
               m_hAACDecoder = 0;
               return HXR_FAIL;
           }

           /* old notes:
            *     m_ulFrameLength is set to what the frame
            *     length will be initially. This might * change once
            *     the first frame is handed in (if AAC+ is signalled
            *     implicitly) * m_ulMaxFrameLength is set to what the
            *     frame length could grow to in that case
            */
           m_ulFrameLength = ASConfig.GetFrameLength();
           m_ulMaxFrameLength = 2*m_ulFrameLength;
           if (m_ulSampleRate != m_ulCoreSampleRate)
               m_ulFrameLength *= 2;

           m_hAACDecoder = (HAACDecoder *)AACInitDecoder();
           if (!m_hAACDecoder)
               return HXR_FAIL;

           /* setup decoder to handle raw data blocks (pass params from MP4 info) */
           m_aacFrameInfo.nChans =    m_ulChannels;
           m_aacFrameInfo.sampRateCore =  m_ulCoreSampleRate;      /* sample rate of BASE layer */

           /* see MPEG4 spec for index of each object type */
           audioObjType = ASConfig.GetObjectType();
           if (audioObjType == 2) {
               m_aacFrameInfo.profile = AAC_PROFILE_LC;
           } else {
               AACFreeDecoder(m_hAACDecoder);
               m_hAACDecoder = 0;
               return HXR_FAIL;
           }

           AACSetRawBlockParams(m_hAACDecoder, 0, &m_aacFrameInfo);

           return HXR_OK;

       default:
           /* unknown format */
           return HXR_FAIL;
    }
}

/* reset the decoder to the state just after OpenDecoder(). Use this
   when seeking or when decoding has returned with an error. */

STDMETHODIMP CAACDec::Reset()
{
    m_ulSamplesToConceal = 0;
    AACFlushCodec(m_hAACDecoder);

    return HXR_OK;
}

/* tell the decoder to conceal nSamples samples. The concealed samples
   will be returned with the next Decode() call. */

STDMETHODIMP CAACDec::Conceal(UINT32 nSamples)
{
#ifdef AAC_ENABLE_SBR
    m_ulSamplesToConceal = (nSamples + 2*AAC_MAX_NSAMPS - 1) / (2*AAC_MAX_NSAMPS);
#else
    m_ulSamplesToConceal = (nSamples + AAC_MAX_NSAMPS - 1) / AAC_MAX_NSAMPS;
#endif

    return HXR_OK;
}

void CAACDec::ReorderPCMChannels(INT16 *pcmBuf, int nSamps, int nChans)
{
    int i, ch, chanMap[6];
    INT16 tmpBuf[6];

    switch (nChans) {
       case 3:
           chanMap[0] = 1; /* L */
           chanMap[1] = 2; /* R */
           chanMap[2] = 0; /* C */
           break;
       case 4:
           chanMap[0] = 1; /* L */
           chanMap[1] = 2; /* R */
           chanMap[2] = 0; /* C */
           chanMap[3] = 3; /* S */
           break;
       case 5:
           chanMap[0] = 1; /* L */
           chanMap[1] = 2; /* R */
           chanMap[2] = 0; /* C */
           chanMap[3] = 3; /* LS */
           chanMap[4] = 4; /* RS */
           break;
       case 6:
           chanMap[0] = 1; /* L */
           chanMap[1] = 2; /* R */ 
           chanMap[2] = 0; /* C */
           chanMap[3] = 5; /* LFE */
           chanMap[4] = 3; /* LS */
           chanMap[5] = 4; /* RS */
           break;
       default:
           return;
    }

    for (i = 0; i < nSamps; i += nChans) {
        for (ch = 0; ch < nChans; ch++)
            tmpBuf[ch] = pcmBuf[chanMap[ch]];
        for (ch = 0; ch < nChans; ch++)
            pcmBuf[ch] = tmpBuf[ch];
        pcmBuf += nChans;
    }
}

/* Decode up to nbytes bytes of bitstream data. nBytesConsumed will be
   updated to reflect how many bytes have been read by the decoder
   (and can thus be discarded in the caller). The decoder does not
   necessarily buffer the bitstream; that is up to the caller.
   samplesOut should point to an array of INT16s, large enough to hold
   MaxSamplesOut() samples.

   nSamplesOut is updated to reflect the number of samples written by
   the decoder.

   set eof when no more data is available and keep calling the decoder
   until it does not produce any more output.
*/

STDMETHODIMP CAACDec::Decode(const UCHAR* data, UINT32 nBytes, UINT32 &nBytesConsumed, INT16 *samplesOut, UINT32& nSamplesOut, HXBOOL eof)
{
    unsigned char *readPtr;
    int bytesLeft, err;
    UINT32 maxSamplesOut;

    if (m_ulSamplesToConceal) {
        GetMaxSamplesOut(maxSamplesOut);

        nSamplesOut = maxSamplesOut;
        if (m_ulSamplesToConceal < maxSamplesOut)
            nSamplesOut = m_ulSamplesToConceal;

        /* just fill with silence */
        memset(samplesOut, 0, nSamplesOut * sizeof(INT16));
        m_ulSamplesToConceal -= nSamplesOut;
        nBytesConsumed = 0;

        AACFlushCodec(m_hAACDecoder);

        return HXR_OK;
    }

    readPtr = (unsigned char *)data;
    bytesLeft = nBytes;
    err = AACDecode(m_hAACDecoder, &readPtr, &bytesLeft, (short *)samplesOut);

    if (err) {
        nSamplesOut = 0;
        switch (err) {
           case ERR_AAC_INDATA_UNDERFLOW:
               /* need to provide more data on next call to AACDecode() (if possible) */
               return HXR_OK;
           default:
               /* other error */
               AACFreeDecoder(m_hAACDecoder);
               m_hAACDecoder = 0;
               return HXR_FAIL;
        }
    }

    /* no error */
    AACGetLastFrameInfo(m_hAACDecoder, &m_aacFrameInfo);

    /* TODO - rather than reordering for multichannel, should init the
     *        audio driver with correct channel map (it's hard coded
     *        right now - see client\audiosvc\hxaudply.cpp,
     *        updownmix.cpp)
     */
    ReorderPCMChannels(samplesOut, m_aacFrameInfo.outputSamps, m_ulChannels);

    m_ulCoreSampleRate = m_aacFrameInfo.sampRateCore;
    m_ulSampleRate = m_aacFrameInfo.sampRateOut;
    m_ulChannels = m_aacFrameInfo.nChans;

    nSamplesOut = m_aacFrameInfo.outputSamps;
    nBytesConsumed = nBytes - bytesLeft;

    return HXR_OK;
}

/* upper limit on the number of samples the decoder will produce per call */
STDMETHODIMP CAACDec::GetMaxSamplesOut(UINT32& nSamples) CONSTMETHOD
{
    nSamples = m_ulChannels * m_ulMaxFrameLength;
    return HXR_OK;
}

/* number of channels of audio in the audio stream (not valid until OpenDecoder() is called) */
STDMETHODIMP CAACDec::GetNChannels(UINT32& nChannels) CONSTMETHOD
{
    nChannels = m_ulChannels;
    return HXR_OK;
}

/* sample rate of the audio stream (not valid until OpenDecoder() is called) */
STDMETHODIMP CAACDec::GetSampleRate(UINT32& sampleRate) CONSTMETHOD
{
    sampleRate = m_ulSampleRate;
    return HXR_OK;
}

/* codec delay in samples */
STDMETHODIMP CAACDec::GetDelay(UINT32& nSamples) CONSTMETHOD
{
    nSamples = m_ulChannels * m_ulFrameLength;
    return HXR_OK;
}

/* query basic codec properties */
void* CAACDec::GetFlavorProperty(UINT16 flvIndex, UINT16 propIndex, UINT16* pSize)
{
    *pSize = 0;
    switch (propIndex) {
       case FLV_PROP_SAMPLES_IN:
           m_propertyStore = m_ulChannels * m_ulFrameLength;
           *pSize = sizeof(m_propertyStore);
           return &m_propertyStore;
                
       case FLV_PROP_NAME:
           *pSize = strlen(m_pszCodecName) + 1;
           return (void*)m_pszCodecName;
        
       default:
           return 0;
    }
}

HX_RESULT CAACDec::_RAInitDecoder(RADECODER_INIT_PARAMS* pParam)
{
    HX_RESULT res = HXR_OK;

    if (pParam->opaqueDataLength < 1)
        return HXR_FAIL;

    if (SUCCEEDED(res))
        res = OpenDecoder(pParam->opaqueData[0], pParam->opaqueData+1, pParam->opaqueDataLength-1);

    if (SUCCEEDED(res)) {
        if (pParam->channels && pParam->channels != m_ulChannels)
            res = HXR_FAIL;
        else
            pParam->channels = (UINT16)m_ulChannels;
    }

    if (SUCCEEDED(res)) {
        pParam->sampleRate = (UINT32)m_ulSampleRate;
        res = GetDelay(m_ulSamplesToEat);
    }

    return res;
}

HX_RESULT CAACDec::_RADecode(const UCHAR* in, UINT32 inLength, INT16* out, UINT32* pOutLength, UINT32 userData)
{
    UINT32 nSamplesPerFrame, nBytesConsumed, nSamplesOut, ulEatSamples;
    HX_RESULT res = HXR_OK;

    *pOutLength = 0;
    if (!userData) {
        /* conceal one frame */
        GetMaxSamplesOut(nSamplesPerFrame);
        Conceal(nSamplesPerFrame);
    }

    nBytesConsumed = 0;
    nSamplesOut = 0;
    res = Decode(in, inLength, nBytesConsumed, out, nSamplesOut, 0);
    inLength -= nBytesConsumed;

    /* if we were not concealing, test if the AAC decoder consumed all data */
    HX_ASSERT(!userData || 0 == inLength);

    /* eat delay (if any) */
    if (m_ulSamplesToEat) {
        ulEatSamples = (m_ulSamplesToEat > nSamplesOut) ? nSamplesOut : m_ulSamplesToEat;
        nSamplesOut -= ulEatSamples;
        memmove(out, out+ulEatSamples, nSamplesOut*sizeof(INT16));
        m_ulSamplesToEat -= ulEatSamples;
    }

    /* return output count in bytes */
    *pOutLength = nSamplesOut * sizeof(INT16);

    return res;
}

#ifdef HELIX_CONFIG_AAC_GENERATE_TRIGTABS_FLOAT
/* create a dummy object to call init and free functions for
 * dynamically generated codec tables a static global object will be
 * constructed before the entrypoint for the DLL is called, so this
 * guarantees that the tables will be initialized before any instances
 * of the codec (CAACDec) are created order of construction and
 * destruction does not matter, since this object only calls the C
 * functions to init and free the codec tables
 */
class CAACGenTrigtabsFloat
{
public:
    CAACGenTrigtabsFloat();
    ~CAACGenTrigtabsFloat();
};

CAACGenTrigtabsFloat::CAACGenTrigtabsFloat()
{
    AACInitTrigtabsFloat();
}

CAACGenTrigtabsFloat::~CAACGenTrigtabsFloat()
{
    AACFreeTrigtabsFloat();
}

static const CAACGenTrigtabsFloat g_cAACGenTrigtabsFloat;

#endif

