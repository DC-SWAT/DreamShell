/* DreamShell ##version##

   audio/wav.h
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 SWAT
*/

/** \file   wav.h
    \brief  Wav file player

    \author Andy Barajas
    \author SWAT
*/
#ifndef WAV_H
#define WAV_H

#include <kos/fs.h>

#define WAVE_FORMAT_PCM                   0x0001 /* PCM */
#define WAVE_FORMAT_IEEE_FLOAT            0x0003 /* IEEE float */
#define WAVE_FORMAT_ALAW                  0x0006 /* 8-bit ITU-T G.711 A-law */
#define WAVE_FORMAT_MULAW                 0x0007 /* 8-bit ITU-T G.711 µ-law */
#define WAVE_FORMAT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */
#define WAVE_FORMAT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */
#define WAVE_FORMAT_EXTENSIBLE            0xfffe /* Determined by SubFormat */

typedef struct {
    uint32_t format;
    uint32_t channels;
    uint32_t sample_rate;
    uint32_t sample_size;
    uint32_t data_offset;
    uint32_t data_length;
} WavFileInfo;

int wav_get_info_file(file_t file, WavFileInfo *result);
int wav_get_info_cdda(file_t file, WavFileInfo *result);
int wav_get_info_adpcm(file_t file, WavFileInfo *result);
int wav_get_info_buffer(const uint8_t *buffer, WavFileInfo *result);

typedef int wav_stream_hnd_t;
typedef void (*wav_filter)(wav_stream_hnd_t hnd, void *obj, int hz, int channels, void **buffer, int *samplecnt);

int wav_init();
void wav_shutdown();
void wav_destroy(wav_stream_hnd_t hnd);

wav_stream_hnd_t wav_create(const char* filename, int loop);
wav_stream_hnd_t wav_create_fd(file_t fd, int loop);
wav_stream_hnd_t wav_create_buf(const unsigned char* buf, int loop);

void wav_play(wav_stream_hnd_t hnd);
void wav_pause(wav_stream_hnd_t hnd);
void wav_stop(wav_stream_hnd_t hnd);
void wav_volume(wav_stream_hnd_t hnd, int vol);
int wav_is_playing(wav_stream_hnd_t hnd);

void wav_add_filter(wav_stream_hnd_t hnd, wav_filter filter, void* obj);
void wav_remove_filter(wav_stream_hnd_t hnd, wav_filter filter, void* obj);

#endif
