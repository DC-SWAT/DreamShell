/* DreamShell ##version##

   modules/wav/libwav.c
   Copyright (C) 2023 Andy Barajas
*/

/** \file   libwav.c
    \brief  Wav file header parser

    \author Andy Barajas
*/

#include "libwav.h"

#include <string.h>

int wav_get_info_file(FILE* file, WavFileInfo* result) {
    char read_buf[8];
    unsigned int fmt_size;
    memset(result, 0, sizeof(WavFileInfo));

    fseek(file, 0, SEEK_SET);
    fread(read_buf, 4, 1, file);
    if(strncmp(read_buf, "RIFF", 4)) {
        fclose(file);
        return 0;
    }

    // Skip total filesize
    fseek(file, 4, SEEK_CUR); // Skip

    // File Type Header "WAVE"
    fread(read_buf, 4, 1, file);
    if(strncmp(read_buf, "WAVE", 4)) {
        fclose(file);
        return 0;
    }

    // Format chunk marker "fmt " (including null terminator)
    fread(read_buf, 4, 1, file);
    if(strncmp(read_buf, "fmt ", 4)) {
        fclose(file);
        return 0;
    }

    // Length of format section
    fread(&fmt_size, 4, 1, file); // Used to seek to 'data' chunk

    // Get wav format
    fread(&(result->wav_format), 2, 1, file);

    // Get num of channels
    fread(&(result->channels), 2, 1, file);

    // Get Sample Rate
    fread(&(result->sample_rate), 4, 1, file);

    fseek(file, 20+fmt_size, SEEK_SET); // Skip to 'data' chunk

    fread(read_buf, 4, 1, file);
    if(strncmp(read_buf, "data", 4)) {
        fclose(file);
        return 0;
    }

    fread(&(result->data_length), 4, 1, file);
    result->data_offset = ftell(file);

    return 1;
}

#define WAV_ADDR_TYPE 8
#define WAV_ADDR_FORMAT_HDR 12
#define WAV_ADDR_FORMAT_LENGTH 16
#define WAV_ADDR_FORMAT 20
#define WAV_ADDR_CHAN 22
#define WAV_ADDR_RATE 24

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
    memcpy(&(result->wav_format), buffer + WAV_ADDR_FORMAT, 2);

    // Get num of channels
    memcpy(&(result->channels), buffer + WAV_ADDR_CHAN, 2);

    // Get Sample Rate
    memcpy(&(result->sample_rate), buffer + WAV_ADDR_RATE, 4);

    memcpy(read_buf, buffer + WAV_ADDR_FORMAT + fmt_size, 4);
    if(strncmp(read_buf, "data", 4)) {
        return 0;
    }

    memcpy(&(result->data_length), buffer + WAV_ADDR_FORMAT + fmt_size + 4, 4);
    result->data_offset = WAV_ADDR_FORMAT + fmt_size + 8;

    return 1;
}
