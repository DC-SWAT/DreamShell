#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "audio.h"



void writewavheader(FILE *fdest, long track_length)
{
unsigned long  wTotal_length;
unsigned long  wData_length;
unsigned long  wHeaderLength = 16;
unsigned short wFormat = 1;
unsigned short wChannels = 2;
unsigned long  wSampleRate = 44100;
unsigned long  wBitRate = 176400;
unsigned short wBlockAlign = 4;
unsigned short wBitsPerSample = 16;

      wData_length = track_length*2352;
      wTotal_length = wData_length + 8 + 16 + 12;

      fwrite("RIFF", 4, 1, fdest);
      fwrite(&wTotal_length, 4, 1, fdest);
      fwrite("WAVE", 4, 1, fdest);
      fwrite("fmt ", 4, 1, fdest);
      fwrite(&wHeaderLength, 4, 1, fdest);
      fwrite(&wFormat, 2, 1, fdest);
      fwrite(&wChannels, 2, 1, fdest);
      fwrite(&wSampleRate, 4, 1, fdest);
      fwrite(&wBitRate, 4, 1, fdest);
      fwrite(&wBlockAlign, 2, 1, fdest);
      fwrite(&wBitsPerSample, 2, 1, fdest);
      fwrite("data", 4, 1, fdest);
      fwrite(&wData_length, 4, 1, fdest);
}


void writeaiffheader(FILE *fdest, long track_length)
{
unsigned long  source_length, total_length;
unsigned char  buf[4];
unsigned long  aCommSize = 18;
unsigned short aChannels = 2;
unsigned long  aNumFrames;
unsigned short aBitsPerSample = 16;
unsigned long  aSampleRate = 44100;
unsigned long  aSsndSize;
unsigned long  aOffset = 0;
unsigned long  aBlockSize = 0;


      source_length = track_length*2352;
      total_length = source_length + 8 + 18 + 8 + 12; // COMM + SSND
      aNumFrames = source_length/4;
      aSsndSize = source_length + 8;

      fwrite("FORM", 4, 1, fdest);

      fwrite(&total_length, 4, 1, fdest);

      fwrite("AIFF", 4, 1, fdest);
      fwrite("COMM", 4, 1, fdest);

      fwrite(&aCommSize, 4, 1, fdest);
      fwrite(&aChannels, 2, 1, fdest);
      fwrite(&aNumFrames, 4, 1, fdest);
      fwrite(&aBitsPerSample, 2, 1, fdest);

      write_ieee_extended(fdest, (double)aSampleRate);

      fwrite("SSND", 4, 1, fdest);

      fwrite(&aSsndSize, 4, 1, fdest);
      fwrite(&aOffset, 4, 1, fdest);
      fwrite(&aBlockSize, 4, 1, fdest);
}


void write_ieee_extended(FILE *fdest, double x)
{
	char buf[10];
	ConvertToIeeeExtended(x, buf);
	/*
	report("converted %g to %o %o %o %o %o %o %o %o %o %o",
		x,
		buf[0], buf[1], buf[2], buf[3], buf[4],
		buf[5], buf[6], buf[7], buf[8], buf[9]);
	*/
	(void) fwrite(buf, 1, 10, fdest);
}



/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define FloatToUnsigned(f)      ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void ConvertToIeeeExtended(double num, char *bytes)
{
    int    sign;
    int expon;
    double fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);          
            fsMant = floor(fMant); 
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32); 
            fsMant = floor(fMant); 
            loMant = FloatToUnsigned(fsMant);
        }
    }
    
    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}

