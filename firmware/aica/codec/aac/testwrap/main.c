/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: main.c,v 1.4 2005/07/05 21:08:13 ehyche Exp $ 
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
 * main.c - sample command-line test wrapper for decoder
 **************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "aacdec.h"
#include "wrapper.h"

#define READBUF_SIZE	(2 * AAC_MAINBUF_SIZE * AAC_MAX_NCHANS)	/* pick something big enough to hold a bunch of frames */

#define SKIP_FRAMES		0		/* discard first SKIP_FRAMES decoded frames */

#ifdef AAC_ENABLE_SBR
#define SBR_MUL		2
#else
#define SBR_MUL		1
#endif

#if defined (__arm) && defined (__ARMCC_VERSION)
#define ARMULATE_MUL_FACT	1
#define MAX_FRAMES	-1 
//#define MAX_FRAMES	300
#else
#define ARMULATE_MUL_FACT	1
#define MAX_FRAMES	-1
#endif

#define AACDEC_FOURCC_RIFF       0x52494646
#define AACDEC_FOURCC_WAVE       0x57415645
#define AACDEC_FOURCC_fmt        0x666d7420
#define AACDEC_FOURCC_data       0x64617461
#define AACDEC_FORMAT_PCM        0x0001

void Usage(void)
{
	printf("\nUsage:\n");
	printf(">> aacdec [-wav] infile.aac outfile\n");
        printf("     -wav:  if specified, then outfile is a .wav file.\n");
        printf("            otherwise, outfile is a raw PCM file\n");
}

static int FillReadBuffer(unsigned char *readBuf, unsigned char *readPtr, int bufSize, int bytesLeft, FILE *infile)
{
	int nRead;

	/* move last, small chunk from end of buffer to start, then fill with new data */
	memmove(readBuf, readPtr, bytesLeft);
	nRead = fread(readBuf + bytesLeft, 1, bufSize - bytesLeft, infile);

	/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
	if (nRead < bufSize - bytesLeft)
		memset(readBuf + bytesLeft + nRead, 0, bufSize - bytesLeft - nRead);	

	return nRead;
}

int TestBigEndian(void)
{
    unsigned long test = 0x000000FF;
    char *temp = (char *)&test;

    /* on big endian machines, the first byte should be 0x00, on little endian 0xFF.*/

    return (temp[0] == 0);
}

void swap16(unsigned char* pBuf, unsigned long ulLen)
{
    unsigned long i = 0;
    for (i = 0; i < ulLen; i += 2)
    {
        unsigned char ucTmp = pBuf[i];
        pBuf[i]   = pBuf[i+1];
        pBuf[i+1] = ucTmp;
    }
}

void pack32BE(unsigned long ulValue, unsigned char** ppBuf, unsigned long* pulLen)
{
    if (ppBuf && *ppBuf && pulLen && *pulLen >= 4)
    {
        unsigned char* pBuf = *ppBuf;
        pBuf[0] = (unsigned char) ((ulValue & 0xFF000000) >> 24);
        pBuf[1] = (unsigned char) ((ulValue & 0x00FF0000) >> 16);
        pBuf[2] = (unsigned char) ((ulValue & 0x0000FF00) >>  8);
        pBuf[3] = (unsigned char)  (ulValue & 0x000000FF);
        *ppBuf  += 4;
        *pulLen -= 4;
    }
}

void pack32LE(unsigned long ulValue, unsigned char** ppBuf, unsigned long* pulLen)
{
    if (ppBuf && *ppBuf && pulLen && *pulLen >= 4)
    {
        unsigned char* pBuf = *ppBuf;
        pBuf[0] = (unsigned char)  (ulValue & 0x000000FF);
        pBuf[1] = (unsigned char) ((ulValue & 0x0000FF00) >>  8);
        pBuf[2] = (unsigned char) ((ulValue & 0x00FF0000) >> 16);
        pBuf[3] = (unsigned char) ((ulValue & 0xFF000000) >> 24);
        *ppBuf  += 4;
        *pulLen -= 4;
    }
}

void pack16LE(unsigned short usValue, unsigned char** ppBuf, unsigned long* pulLen)
{
    if (ppBuf && *ppBuf && pulLen && *pulLen >= 2)
    {
        unsigned char* pBuf = *ppBuf;
        pBuf[0] = (unsigned char)  (usValue & 0x000000FF);
        pBuf[1] = (unsigned char) ((usValue & 0x0000FF00) >> 8);
        *ppBuf  += 2;
        *pulLen -= 2;
    }
}

void WriteWAVHeader(FILE* fp, AACFrameInfo* pFrameInfo)
{
    // Only works for numChannels <= 2 now
    if (fp && pFrameInfo && pFrameInfo->nChans <= 2)
    {
        unsigned char ucTmp[44];
        unsigned char* pTmp  = &ucTmp[0];
        unsigned long  ulTmp = sizeof(ucTmp);
        int lByteRate   = pFrameInfo->sampRateOut * pFrameInfo->nChans * 2;
        int lBlockAlign = pFrameInfo->nChans * 2;
        /* Pack the RIFF four cc */
        pack32BE(AACDEC_FOURCC_RIFF, &pTmp, &ulTmp);
        /* Pack 0 for the RIFF chunk size - update in UpdateWAVHeader. */
        pack32LE(0, &pTmp, &ulTmp);
        /* Pack the WAVE four cc */
        pack32BE(AACDEC_FOURCC_WAVE, &pTmp, &ulTmp);
        /* Pack the 'fmt ' subchunk four cc */
        pack32BE(AACDEC_FOURCC_fmt, &pTmp, &ulTmp);
        /* Pack the fmt subchunk size */
        pack32LE(16, &pTmp, &ulTmp);
        /* Pack the audio format */
        pack16LE((unsigned short) AACDEC_FORMAT_PCM, &pTmp, &ulTmp);
        /* Pack the number of channels */
        pack16LE((unsigned short) pFrameInfo->nChans, &pTmp, &ulTmp);
        /* Pack the sample rate */
        pack32LE((unsigned long) pFrameInfo->sampRateOut, &pTmp, &ulTmp);
        /* Pack the byte rate */
        pack32LE((unsigned long) lByteRate, &pTmp, &ulTmp);
        /* Pack the block align (ulChannels * 2) */
        pack16LE((unsigned short) lBlockAlign, &pTmp, &ulTmp);
        /* Pack the bits per sample */
        pack16LE((unsigned short) pFrameInfo->bitsPerSample, &pTmp, &ulTmp);
        /* Pack the 'data' subchunk four cc */
        pack32BE(AACDEC_FOURCC_data, &pTmp, &ulTmp);
        /* Pack the data subchunk size (0 for now - update in UpdateWavHeader) */
        pack32LE(0, &pTmp, &ulTmp);
        /* Write out the wav header */
        fwrite(&ucTmp[0], 1, 44, fp);
    }
}

void UpdateWAVHeader(char* pFileName)
{
    if (pFileName)
    {
        /* Re-open the file for updating */
        FILE* fp = fopen((const char*) pFileName, "r+b");
        if (fp)
        {
            unsigned char  ucTmp[4];
            unsigned long  ulFileSize = 0;
            unsigned long  ulRIFFSize = 0;
            unsigned long  ulDataSize = 0;
            unsigned char* pTmp       = NULL;
            unsigned long  ulLen      = 0;
            /*
             * Compute the RIFF chunk size and the 
             * data chunk size from the size of the file.
             */
            fseek(fp, 0, SEEK_END);
            ulFileSize = (unsigned long) ftell(fp);
            ulRIFFSize = ulFileSize - 8;
            ulDataSize = ulFileSize - 44;
            /* Seek to the RIFF chunk size */
            fseek(fp, 4, SEEK_SET);
            /* Set up the packing buffer */
            pTmp  = &ucTmp[0];
            ulLen = sizeof(ucTmp);
            /* Pack the RIFF chunk size */
            pack32LE(ulRIFFSize, &pTmp, &ulLen);
            /* Write out the buffer */
            fwrite(&ucTmp[0], 1, 4, fp);
            /* Seek to the beginning of the data chunk size */
            fseek(fp, 40, SEEK_SET);
            /* Set up the packing buffer */
            pTmp  = &ucTmp[0];
            ulLen = sizeof(ucTmp);
            /* Pack the data chunk size */
            pack32LE(ulDataSize, &pTmp, &ulLen);
            /* Write out the buffer */
            fwrite(&ucTmp[0], 1, 4, fp);
            /* Close the file */
            fclose(fp);
        }
    }
}

static short outBuf[AAC_MAX_NCHANS * AAC_MAX_NSAMPS * SBR_MUL];
static unsigned char readBuf[READBUF_SIZE];

int main(int argc, char **argv)
{
	int bytesLeft, nRead, err, outOfData, eofReached, skipFrames;
	unsigned char *readPtr;
	char *infileName, *outfileName;
	unsigned int startTime, endTime, diffTime, lastDiffTime;
	int nFramesTNSOn, nFramesTNSOff; 
	float totalDecTimeTNSOn, totalDecTimeTNSOff, audioSecs, audioFrameSecs, currMHz, peakMHz;	/* for calculating percent RT */

	FILE *infile, *outfile, *logfile;
	HAACDecoder *hAACDecoder;
	AACFrameInfo aacFrameInfo;
	uiSignal playStatus;
        int outputWAV  = 0;
        int firstWrite = 1;
        int bigEndian  = TestBigEndian();

	if (argc < 3 || argc > 4) {
		Usage();
		return -1;
	}
        infileName = argv[argc-2];
        outfileName = argv[argc-1];
        if (argc == 4)
        {
            outputWAV = 1;
        }

	/* open input file */
	infile = fopen(infileName, "rb");
	if (!infile) {
		printf(" *** Error opening input file %s ***\n", infileName);
		Usage();
		return -1;
	}
		
	/* open output file */
	if (strcmp(argv[2], "nul")) {
		outfile = fopen(outfileName, "wb");
		if (!outfile) {
			printf(" *** Error opening output file %s ***\n", outfileName);
			Usage();
			return -1;
		}
	} else {
		outfile = 0;	/* nul output */
	}

	DebugMemCheckInit();
	DebugMemCheckStartPoint();

#ifdef HELIX_CONFIG_AAC_GENERATE_TRIGTABS_FLOAT
	AACInitTrigtabsFloat();
#endif

	hAACDecoder = (HAACDecoder *)AACInitDecoder();
	if (!hAACDecoder) {
		printf(" *** Error initializing decoder ***\n");
		Usage();
		return -1;
	}

	DebugMemCheckEndPoint();

	/* initialize timing code */
	InitTimer();
	peakMHz = 0;
	lastDiffTime = 0;
	audioFrameSecs = 0;	/* init after decoding first frame */

	bytesLeft = 0;
	outOfData = 0;
	eofReached = 0;
	readPtr = readBuf;
	err = 0;
	skipFrames = SKIP_FRAMES;
	playStatus = Play;

	nFramesTNSOn = 0;
	nFramesTNSOff = 0;
	totalDecTimeTNSOn = 0;
	totalDecTimeTNSOff = 0;
	do {
		/* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
		if (bytesLeft < AAC_MAX_NCHANS * AAC_MAINBUF_SIZE && !eofReached) {
			nRead = FillReadBuffer(readBuf, readPtr, READBUF_SIZE, bytesLeft, infile);
			bytesLeft += nRead;
			readPtr = readBuf;
			if (nRead == 0)
				eofReached = 1;
		}

		startTime = ReadTimer();

		/* decode one AAC frame */
 		err = AACDecode(hAACDecoder, &readPtr, &bytesLeft, outBuf);

		endTime = ReadTimer();
		diffTime = CalcTimeDifference(startTime, endTime);

		if (err) {
			/* error occurred */
			switch (err) {
			case ERR_AAC_INDATA_UNDERFLOW:
				/* need to provide more data on next call to AACDecode() (if possible) */
				if (eofReached || bytesLeft == READBUF_SIZE)
					outOfData = 1;
				break;
			default:
				outOfData = 1;
				break;
			}
		}
		if (outOfData)
			break;

		/* no error */
		AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);

                if (firstWrite)
                {
                    if (outputWAV)
                    {
                        WriteWAVHeader(outfile, &aacFrameInfo);
                    }
                    firstWrite = 0;
                }

		if (skipFrames == 0 && outfile)
                {
                    if (bigEndian && outputWAV && aacFrameInfo.bitsPerSample == 16)
                    {
                        int lSizeBytes = aacFrameInfo.outputSamps * aacFrameInfo.bitsPerSample / 8;
                        swap16((unsigned char*) outBuf, (unsigned long) lSizeBytes);
                    }
		    fwrite(outBuf, aacFrameInfo.bitsPerSample / 8, aacFrameInfo.outputSamps, outfile); 
                }

		if (skipFrames != 0)
			skipFrames--;

		playStatus = PollHardware();
		switch (playStatus) {
		case Stop:
		case Quit:
			printf("Warning - Processing aborted");
			break;
		case Play:
			break;
		}
		
		if (!audioFrameSecs)
			audioFrameSecs = ((float)aacFrameInfo.outputSamps) / ((float)aacFrameInfo.sampRateOut * aacFrameInfo.nChans);
		currMHz = ARMULATE_MUL_FACT * (1.0f / audioFrameSecs) * diffTime / 1e6f;
		peakMHz = (currMHz > peakMHz ? currMHz : peakMHz);

		if (aacFrameInfo.tnsUsed) {
			nFramesTNSOn++;
			totalDecTimeTNSOn += diffTime;
		} else {
			nFramesTNSOff++;
			totalDecTimeTNSOff += diffTime;
		}

#if defined (__arm) && defined (__ARMCC_VERSION)
		printf("frame %5d  [%10u - %10u = %6u tks] .. ", nFramesTNSOn + nFramesTNSOff, startTime, endTime, diffTime);
		printf("curr MHz = %5.2f, peak MHz = %5.2f, delta tks = %#d (T=%c)\r", 
		currMHz, peakMHz, diffTime - lastDiffTime, aacFrameInfo.tnsUsed ? '*' : ' ');
		fflush(stdout);
#endif

		if (nFramesTNSOn + nFramesTNSOff == MAX_FRAMES)
			break;

	} while (playStatus == Play);

#if defined (__arm) && defined (__ARMCC_VERSION)
	logfile = stdout;
#elif defined (_WIN32) && defined (_WIN32_WCE)
	logfile = fopen("\\Temp\\testwrap.log", "wb");
#else
	logfile = 0;
#endif
	
	if (logfile) {
		audioSecs = (float)nFramesTNSOff * audioFrameSecs;
		if (nFramesTNSOff) {
			fprintf(logfile, "\n\nTNS OFF:  Total clock ticks = %13.0f, MHz usage = %5.2f, nFrames = %4d\n", 
				totalDecTimeTNSOff, ARMULATE_MUL_FACT * (1.0f / audioSecs) * totalDecTimeTNSOff / 1e6f, nFramesTNSOff);
		} else {
			fprintf(logfile, "\n\nTNS OFF:   no frames\n");
		}

		audioSecs = (float)nFramesTNSOn * audioFrameSecs;
		if (nFramesTNSOn) {
			fprintf(logfile, "TNS ON:   Total clock ticks = %13.0f, MHz usage = %5.2f, nFrames = %4d\n\n", 
				totalDecTimeTNSOn,  ARMULATE_MUL_FACT * (1.0f / audioSecs) * totalDecTimeTNSOn / 1e6f, nFramesTNSOn);
		} else {
			fprintf(logfile, "TNS ON:   no frames\n\n");
		}
		
		fprintf(logfile, "nFrames total = %d, output samps = %d, sampRate = %d, nChans = %d\n", 
			nFramesTNSOn + nFramesTNSOff, aacFrameInfo.outputSamps, aacFrameInfo.sampRateOut, aacFrameInfo.nChans);

		if (logfile != stdout)
			fclose(logfile);
	}

	audioSecs = 0;
	FreeTimer();

	if (err != ERR_AAC_NONE && err != ERR_AAC_INDATA_UNDERFLOW)
		printf("Error - %d", err);
	printf("\n");

	AACFreeDecoder(hAACDecoder);

#ifdef HELIX_CONFIG_AAC_GENERATE_TRIGTABS_FLOAT
	AACFreeTrigtabsFloat();
#endif

	/* close files */
	fclose(infile);
	if (outfile)
        {
            fclose(outfile);
            if (outputWAV)
            {
                UpdateWAVHeader(outfileName);
            }
        }

	DebugMemCheckFree();
	
	return 0;
}

#if defined (_WIN32) && defined (_WIN32_WCE)

#include <windows.h>
#include <aygshell.h>	/* PocketPC shell functions */
#include <nled.h>		/* LED on WinCE only */
HXBOOL NLedGetDeviceInfo(int nID, void *pOutput); 
HXBOOL NLedSetDevice(int nID, void *pOutput);

static void StartLED(void)
{
	int numLeds;
	struct NLED_COUNT_INFO nLedCountInfo;
	struct NLED_SETTINGS_INFO nLedSettingsInfo;
	
	numLeds = 0;
	if( NLedGetDeviceInfo(NLED_COUNT_INFO_ID, &nLedCountInfo) ) 
		numLeds = (int)nLedCountInfo.cLeds;

	/* 0 = off, 1 = on, 2 = blink */
	if (numLeds >= 1) {
		nLedSettingsInfo.LedNum = 0; 
		nLedSettingsInfo.OffOnBlink = 1;
		NLedSetDevice(NLED_SETTINGS_INFO_ID, &nLedSettingsInfo);
	}
}

static void StopLED(void)
{
	int numLeds;
	struct NLED_COUNT_INFO nLedCountInfo;
	struct NLED_SETTINGS_INFO nLedSettingsInfo;
	
	numLeds = 0;
	if( NLedGetDeviceInfo(NLED_COUNT_INFO_ID, &nLedCountInfo) ) 
		numLeds = (int)nLedCountInfo.cLeds;

	/* 0 = off, 1 = on, 2 = blink */
	if (numLeds >= 1) {
		nLedSettingsInfo.LedNum = 0; 
		nLedSettingsInfo.OffOnBlink = 0;
		NLedSetDevice(NLED_SETTINGS_INFO_ID, &nLedSettingsInfo);
	}
}

//#define STEST
#ifdef  STEST
#define NUM_FILES		10

static const char *stestName[NUM_FILES] = {
	"aero128_44s_adts",
	"aero64_22m_adts",
	"al_sbr_gh_44_2",
	"AL05_24000",
	"al18_48000",
	"al19_11025",
	"btrav128_44s_adif",
	"L4_11025",
	"star32p_44m_adts",
	"star48p_44s_adts",
};
#else
#define NUM_FILES		1
#endif

#define LOG_TOTAL_TIME
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
	int i;
	char infileName[256], outfileName[256];
	char *testArgs[3];

#ifdef LOG_TOTAL_TIME
	int startTime, endTime, diffTime;
	FILE *logFile;
#endif

	for (i = 0; i < NUM_FILES; i++) {

#ifdef STEST
		strcpy(infileName, "\\Temp\\smoke\\");
		strcat(infileName,  stestName[i]);
		strcat(infileName, ".aac");

		strcpy(outfileName, "\\ipaq file store\\");
		strcat(outfileName, stestName[i]);
		strcat(outfileName, ".pcm");
#else
		strcpy(infileName, "\\Temp\\file.aac");
		strcpy(outfileName, "nul");
#endif
		testArgs[0] = "testwrap.exe";
		testArgs[1] = infileName;
		testArgs[2] = outfileName;

#ifdef LOG_TOTAL_TIME
		startTime = GetTickCount();
#endif
		StartLED();

		main(3, testArgs);

		StopLED();
#ifdef LOG_TOTAL_TIME
		endTime = GetTickCount();
		diffTime = endTime - startTime;

		logFile = fopen("\\Temp\\totaltime.log", "wb");
		if (logFile) {
			fprintf(logFile, "startTime = %10d, endTime = %10d, diffTime = %10d mSecs\n", startTime, endTime, diffTime);
			fclose(logFile);
		}
#endif
	}
	return 0;
}

#endif
