/* DreamShell
   
   media.c - Arcade app media
   Copyright (C) 2026 SWAT
   
*/

#include <audio/wav.h>
#include "app_internal.h"

size_t GetCDDATrackFilename(int num, const char *fpath, const char *filename, char *result) {
    char track_file[16];
    int size = 0, len;

    snprintf(track_file, sizeof(track_file), "track%02d.raw", num);
    makeGameRelativePath(result, NAME_MAX, fpath, filename, track_file);

    size = FileSize(result);

    if (size > 0) {
        return size;
    }

    len = strlen(result);
    result[len - 3] = 'w';
    result[len - 1] = 'v';

    size = FileSize(result);
    return size;
}

static wav_stream_hnd_t wav_hnd = SND_STREAM_INVALID;
static int wav_inited = 0;
static int cdda_paused = 0;

void PauseCDDATrack() {
    if(wav_inited && wav_hnd != SND_STREAM_INVALID && wav_is_playing(wav_hnd)) {
        wav_pause(wav_hnd);
        cdda_paused = 1;
    } else {
        cdda_paused = 0;
    }
}

void ResumeCDDATrack() {
    if(cdda_paused) {
        wav_play(wav_hnd);
        cdda_paused = 0;
    }
}

int IsCDDATrackPlaying() {
    if(wav_inited && wav_hnd != SND_STREAM_INVALID) {
        return wav_is_playing(wav_hnd);
    }
    return 0;
}

void SetCDDAVolume(int vol) {
    if(wav_inited && wav_hnd != SND_STREAM_INVALID) {
        wav_volume(wav_hnd, vol);
    }
}

void StopCDDATrack() {
    if(wav_inited) {
        wav_shutdown();
    }
}

void PlayCDDATrack(const char *file, int loop) {

    StopCDDATrack();
    wav_inited = wav_init();

    if(wav_inited) {
        wav_hnd = wav_create(file, loop);

        if(wav_hnd == SND_STREAM_INVALID) {
            ds_printf("DS_ERROR: Can't play file: %s\n", file);
            return;
        }
        // ds_printf("DS_OK: Start playing: %s\n", file);

        int volume = GetVolumeFromSettings();
        if(volume >= 0) {
            wav_volume(wav_hnd, volume);
        }
        wav_play(wav_hnd);
    }
}

static void *TrailerThread(void *param) {
    mutex_lock_scoped(&self.trailer.mutex);
    ffplay(self.trailer.path, &self.trailer.params);
    thd_sleep(50);
    self.trailer.active = false;
    return NULL;
}

void PlayTrailer(int index) {
    GameEntry *g = &self.games[index];
    mutex_lock_scoped(&self.trailer.mutex);

    if (self.trailer.active) {
        return;
    }

    strncpy(self.trailer.path, g->trailer_path, NAME_MAX);
    memset(&self.trailer.params, 0, sizeof(ffplay_params_t));

    self.trailer.params.x = self.video_x;
    self.trailer.params.y = self.video_y;
    self.trailer.params.width = self.video_w;
    self.trailer.params.height = self.video_h;
    self.trailer.params.scale = 1.0f;
    self.trailer.params.fullscreen = 0;
    self.trailer.params.loop = 0;
    self.trailer.params.fade_in = self.config.video_fade_in_ms;
    self.trailer.params.show_stat = 0;

    self.trailer.active = true;
    self.trailer.thread = thd_create(1, TrailerThread, NULL);
}

void StopTrailer() {
    mutex_lock_scoped(&self.trailer.mutex);
    ffplay_shutdown();
    self.trailer.active = false;
    if (self.video_border) {
        if (TSU_DrawableGetParent((Drawable *)self.video_border)) {
            TSU_AppSubRemoveRectangle(self.app->tsunami, self.video_border);
        }
    }
}

void StopMedia() {
    StopTrailer();
    StopCDDATrack();
}

int GetMediaVolume(void) {
    if (self.config.media_volume >= 0) {
        return self.config.media_volume;
    }

    int vol = GetVolumeFromSettings();
    if (vol < 0) return DEFAULT_MEDIA_VOLUME;
    return vol;
}
