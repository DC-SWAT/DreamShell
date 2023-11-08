/* DreamShell ##version##

   modules/wav/libwav.c
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 SWAT
*/

/** \file   libwav.c
    \brief  Wav file header parser

    \author Andy Barajas
    \author SWAT
*/

#include "libwav.h"
#include <string.h>

int wav_get_info_cdda_fd(file_t fd, WavFileInfo *result) {
    result->format = WAVE_FORMAT_PCM;
    result->sample_size = 16;
    result->channels = 2;
    result->sample_rate = 44100;
    result->data_offset = 0;
    result->data_length = fs_total(fd);
    return 1;
}

int wav_get_info_fd(file_t fd, WavFileInfo *result) {
    int offset = 0;
    unsigned int value = 0;

    memset(result, 0, sizeof(WavFileInfo));

    fs_seek(fd, 0x08, SEEK_SET);
    fs_read(fd, &value, 4);

    /* Check file magic */
    if(!memcmp(&value, "WAVE", 4)) {

        /* Read WAV header info */
        fs_seek(fd, 0x14, SEEK_SET);
        fs_read(fd, &result->format, 2);
        fs_read(fd, &result->channels, 2);
        fs_read(fd, &result->sample_rate, 4);
        fs_seek(fd, 0x22, SEEK_SET);
        fs_read(fd, &result->sample_size, 2);
        fs_read(fd, &value, 4);

        if(value != 0x61746164) {

            offset += 0x32;
            fs_seek(fd, offset, SEEK_SET);
            fs_read(fd, &value, 4);

            if(value != 0x61746164) {

                offset += 0x14;
                fs_seek(fd, offset, SEEK_SET);
                fs_read(fd, &value, 4);
            }
        } else {
            offset += 0x2c;
        }

        fs_read(fd, &value, 4);
        result->data_length = value;
        result->data_offset = offset;
        return 1;
    }
    return 0;
}

#define WAV_ADDR_TYPE 8
#define WAV_ADDR_FORMAT_HDR 12
#define WAV_ADDR_FORMAT_LENGTH 16
#define WAV_ADDR_FORMAT 20
#define WAV_ADDR_CHAN 22
#define WAV_ADDR_RATE 24
#define WAV_ADDR_SIZE 34

int wav_get_info_buffer(const unsigned char* buffer, WavFileInfo* result) {
    char read_buf[8];
    unsigned int fmt_size;
    memset(result, 0, sizeof(WavFileInfo));

    memcpy(read_buf, buffer, 4);
    if(strncmp(read_buf, "RIFF", 4)) {
        return 0;
    }

    // File Type Header "WAVE"
    memcpy(read_buf, buffer + WAV_ADDR_TYPE, 4);
    if(strncmp(read_buf, "WAVE", 4)) {
        return 0;
    }

    // Format chunk marker "fmt " (including null terminator)
    memcpy(read_buf, buffer + WAV_ADDR_FORMAT_HDR, 4);
    if(strncmp(read_buf, "fmt ", 4)) {
        return 0;
    }

    // Length of format section
    memcpy(&fmt_size, buffer + WAV_ADDR_FORMAT_LENGTH, 4); // Used to seek to 'data' chunk

    // Get wav format
    memcpy(&(result->format), buffer + WAV_ADDR_FORMAT, 2);

    // Get num of channels
    memcpy(&(result->channels), buffer + WAV_ADDR_CHAN, 2);

    // Get Sample Rate
    memcpy(&(result->sample_rate), buffer + WAV_ADDR_RATE, 4);

    // Get Sample Size
    memcpy(&(result->sample_size), buffer + WAV_ADDR_SIZE, 4);

    memcpy(read_buf, buffer + WAV_ADDR_FORMAT + fmt_size, 4);
    if(strncmp(read_buf, "data", 4)) {
        return 0;
    }

    memcpy(&(result->data_length), buffer + WAV_ADDR_FORMAT + fmt_size + 4, 4);
    result->data_offset = WAV_ADDR_FORMAT + fmt_size + 8;

    return 1;
}
