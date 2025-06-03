/* DreamShell ##version##

   module.c - wav module
   Copyright (C) 2023 SWAT
*/

#include "ds.h"
#include "audio/wav.h"
#include "settings.h"

DEFAULT_MODULE_HEADER(wave);

static int wav_inited = 0;
static wav_stream_hnd_t wav_hnd = SND_STREAM_INVALID;

static int start_playback(const char *file, int loop) {
    wav_hnd = wav_create(file, loop);

    if(wav_hnd == SND_STREAM_INVALID) {
        ds_printf("DS_ERROR: Can't play file: %s\n", file);
        return CMD_ERROR;
    }
    
    int volume = GetVolumeFromSettings();
    if(volume >= 0) {
        wav_volume(wav_hnd, volume);
    }
    
    wav_play(wav_hnd);
    return CMD_OK;
}

static void stop_playback() {
    if(wav_hnd == SND_STREAM_INVALID) {
        return;
    }
    wav_destroy(wav_hnd);
    wav_hnd = SND_STREAM_INVALID;
}

static int builtin_wave(int argc, char *argv[]) {

    if(argc == 1) {
        ds_printf("Usage: %s option args...\n\n"
                  "Options: \n"
                  " -p, --play     -Start playing\n"
                  " -s, --stop     -Stop playing\n\n", argv[0]);
        ds_printf("Arguments: \n"
                  " -l, --loop     -Loop N times\n"
                  " -v, --volume   -Set volume\n"
                  " -f, --file     -File for playing\n\n"
                  "Examples: %s --play --file /cd/file.wav\n"
                  "          %s -s", argv[0], argv[0]);
        return CMD_NO_ARG;
    } 

    int start = 0, stop = 0, loop = 0, volume = 0;
    char *file = NULL;

    struct cfg_option options[] = {
        {"play",   'p', NULL, CFG_BOOL, (void *) &start,  0},
        {"stop",   's', NULL, CFG_BOOL, (void *) &stop,   0},
        {"loop",   'l', NULL, CFG_INT,  (void *) &loop,   0},
        {"volume", 'v', NULL, CFG_INT,  (void *) &volume, 0},
        {"file",   'f', NULL, CFG_STR,  (void *) &file,   0},
        CFG_END_OF_LIST
    };

    CMD_DEFAULT_ARGS_PARSER(options);

    if(!wav_inited) {
        wav_init();
        wav_inited = 1;
    }
    if(start) {
        if(file == NULL) {
            ds_printf("DS_ERROR: Need file for playing\n");
            return CMD_ERROR;
        }
        stop_playback();
        int rv = start_playback(file, loop);
        if (rv != CMD_OK) {
            return rv;
        }
    }
    else if(stop) {
        stop_playback();
    }
    if(volume && wav_hnd != SND_STREAM_INVALID) {
        wav_volume(wav_hnd, volume);
    }
    if(!start && !stop && !volume) {
        ds_printf("DS_ERROR: There is no option.\n");
        return CMD_NO_ARG;
    } else {
        return CMD_OK;
    }
}

int lib_open(klibrary_t *lib) {
    AddCmd(lib_get_name(), "PCM player", (CmdHandler *) builtin_wave); 
    return nmmgr_handler_add(&ds_wave_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
    RemoveCmd(GetCmdByName(lib_get_name()));
    if(wav_inited) {
        wav_shutdown();
        wav_inited = 0;
    }
    return nmmgr_handler_remove(&ds_wave_hnd.nmmgr);
}
