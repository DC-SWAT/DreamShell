/* DreamShell ##version##

   modules/wav/libwav.h
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 SWAT
*/

/** \file   libwav.h
    \brief  Wav file header parser

    \author Andy Barajas
    \author SWAT
*/
#ifndef LIBWAV_H
#define LIBWAV_H

// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
// http://soundfile.sapp.org/doc/WaveFormat/
// https://wiki.fileformat.com/audio/wav/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

#define	WAVE_FORMAT_PCM     	          0x0001 /* PCM */
#define	WAVE_FORMAT_IEEE_FLOAT            0x0003 /* IEEE float */
#define	WAVE_FORMAT_ALAW	              0x0006 /* 8-bit ITU-T G.711 A-law */
#define	WAVE_FORMAT_MULAW	              0x0007 /* 8-bit ITU-T G.711 µ-law */
#define WAVE_FORMAT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */
#define WAVE_FORMAT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */
#define	WAVE_FORMAT_EXTENSIBLE	          0xfffe /* Determined by SubFormat */

typedef struct {
    unsigned short format;
    unsigned short channels;
    unsigned short sample_size;
    unsigned int sample_rate;
    unsigned int data_offset;
    unsigned int data_length;
} WavFileInfo;

int wav_get_info_fd(file_t fd, WavFileInfo *result);
int wav_get_info_cdda_fd(file_t fd, WavFileInfo *result);
int wav_get_info_buffer(const unsigned char *buffer, WavFileInfo* result);

__END_DECLS

#endif
