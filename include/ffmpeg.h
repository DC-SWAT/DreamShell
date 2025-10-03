/** 
 * \file    ffmpeg.h
 * \brief   DreamShell FFmpeg module
 * \date    2025
 * \author  SWAT www.dc-swat.ru
 */

#ifndef __DS_FFMPEG_H
#define __DS_FFMPEG_H

#include <arch/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ffplay_params {
    int x;
    int y;
    int width;
    int height;
    float scale;
    int fullscreen;

    int loop;
    int verbose;
    int show_stat;
    int fade_in;

    const char *force_format;
    void (*update_callback)(int64_t current_pos, int64_t total_duration);
} ffplay_params_t;

typedef struct ffplay_info {
    int64_t duration;
    const char *format;
    const char *video_codec;
    const char *audio_codec;
    int width;
    int height;
    float fps;
    int video_bitrate;
    int audio_bitrate;
    int audio_channels;
} ffplay_info_t;

/*
    Starts video playback.
*/
int ffplay(const char *filename, ffplay_params_t *params);

/*
    Stops the currently playing video.
*/
void ffplay_shutdown();

/*
    Toggles pause for the currently playing video.
*/
void ffplay_toggle_pause();

/*
    Seek to the specified position in milliseconds.
*/
int ffplay_seek(int64_t pos_ms);

/*
    Get information about the currently playing video.
    Returns: 0 on success, -1 if not playing.
*/
int ffplay_info(ffplay_info_t *info);

/*
    Get current playback position in milliseconds.
    Returns: current position in ms, or -1 if not playing.
*/
int64_t ffplay_get_pos();

/*
    Checks if a video is currently paused.
    Returns: 1 if paused, 0 otherwise.
*/
int ffplay_is_paused();

/*
    Checks if a video is currently playing (even if paused).
    Returns: 1 if playing, 0 otherwise.
*/
int ffplay_is_playing();

#ifdef __cplusplus
}
#endif

#endif /* __DS_FFMPEG_H */
