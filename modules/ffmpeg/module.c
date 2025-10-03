/* DreamShell ##version##

   module.c - ffmpeg module
   Copyright (C)2011-2014, 2025 SWAT 
*/
    
#include "ds.h"
#include "ffmpeg.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include <string.h>

DEFAULT_MODULE_HEADER(ffmpeg);

static int file_open(URLContext *h, const char *filename, int flags) {
    file_t fd;
    int _flags = 0;
    char fn[NAME_MAX];
    const char *path_start = strchr(filename, ':');

    if(path_start) {
        path_start++;
        while(*path_start == ' ') {
            path_start++;
        }
        strncpy(fn, path_start, NAME_MAX);
    }
    else {
        strncpy(fn, filename, NAME_MAX);
    }
    fn[NAME_MAX - 1] = '\0';

    if(strncmp(fn, "/sd", 3) == 0) {
        h->max_packet_size = 8 << 10;
    }
    else if(strncmp(fn, "/cd", 3) == 0) {
        h->max_packet_size = 16 << 10;
    }
    // else if(strncmp(fn, "/ide", 4) == 0) {
    //     h->max_packet_size = 64 << 10;
    // }
    // 32 KB by default for other devices

    switch(flags) {
        case URL_RDONLY:
            _flags = O_RDONLY;
            break;
        case URL_WRONLY:
            _flags = O_WRONLY;
            break;
        case URL_RDWR:
            _flags = O_RDWR;
            break;
        default:
            ds_printf("DS_ERROR: Uknown file open flag for ffmpeg: %08x\n", flags);
            return -1;
    }
    fd = fs_open(fn, _flags);

    if (fd < 0) {
        return -1;
    }
    h->priv_data = (void*)fd;
    return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size) {
    // dbglog(DBG_DEBUG, "file_read: %p %d\n", buf, size);
    return fs_read((file_t)h->priv_data, buf, size);
}

static int file_write(URLContext *h, unsigned char *buf, int size) {
    return fs_write((file_t)h->priv_data, buf, size);
}

static int64_t file_seek(URLContext *h, int64_t pos, int whence) {
    if(whence == AVSEEK_SIZE) {
        return fs_total((file_t)h->priv_data);
    }
    return fs_seek((file_t)h->priv_data, pos, whence);
}

static int file_close(URLContext *h) {
    fs_close((file_t)h->priv_data);
    return 0;
}

static int file_get_file_handle(URLContext *h) {
    return (file_t)h->priv_data;
}

static URLProtocol fs_protocol = {
    "ds",
    file_open,
    file_read,
    file_write,
    file_seek,
    file_close,
    NULL,
    NULL,
    NULL,
    file_get_file_handle
};

#define ALIGNMENT 32
#define NO_ALIGN_THRESHOLD 512

typedef struct {
    void *raw;
    unsigned int size;
} ds_mem_header_t;

void *ds_malloc(unsigned int size) {
    ds_mem_header_t *header;
    void *raw;

    if(size < NO_ALIGN_THRESHOLD) {
        raw = malloc(size + sizeof(ds_mem_header_t));
        if (!raw) return NULL;
        header = (ds_mem_header_t *)raw;
        header->raw = raw;
        header->size = size;
        return (void *)(header + 1);
    }

    raw = malloc(size + ALIGNMENT - 1 + sizeof(ds_mem_header_t));
    if(!raw) return NULL;

    uintptr_t aligned = ((uintptr_t)raw + sizeof(ds_mem_header_t) + (ALIGNMENT - 1)) & ~(uintptr_t)(ALIGNMENT - 1);
    header = (ds_mem_header_t *)aligned - 1;
    header->raw = raw;
    header->size = size;

    return (void *)aligned;
}

void ds_free(void *ptr) {
    if(ptr) {
        ds_mem_header_t *header = (ds_mem_header_t *)ptr - 1;
        free(header->raw);
    }
}

void *ds_realloc(void *ptr, unsigned int size) {
    if (!ptr) return ds_malloc(size);

    ds_mem_header_t *header = (ds_mem_header_t *)ptr - 1;
    unsigned int old_size = header->size;
    void *new_ptr = ds_malloc(size);

    if(!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, old_size < size ? old_size : size);
    ds_free(ptr);
    return new_ptr;
}

int builtin_ffplay_cmd(int argc, char *argv[]) {
    if(argc == 1) {
        ds_printf("Usage: %s option args...\n\n"
                    "Options: \n"
                    " -v, --version     - Show ffmpeg library version\n"
                    " -p, --play        - Start playing\n"
                    " -s, --stop        - Stop playing\n"
                    " -a, --pause       - Toggle pause\n"
                    " -l, --loop        - Loop playback\n"
                    " -e, --verbose     - Verbose output\n"
                    " -t, --stat        - Show statistics\n\n"
                    "Arguments: \n"
                    " -d  --fade-in     - Fade in duration in ms (default: 0)\n"
                    " -i, --format      - Force format detection\n"
                    " -f, --file        - File for playing\n"
                    " -m, --mode        - Video mode (0: Windowed, 1: Fullscreen)\n"
                    " -x, --x           - X coordinate for windowed mode (default: 0)\n"
                    " -y, --y           - Y coordinate for windowed mode (default: 0)\n"
                    " -c, --scale       - Scale for windowed mode (default: 1.0)\n\n"
                    "Examples: %s -p -f /cd/file.avi\n"
                    "          %s -p -f /cd/file.avi -m 0 -x 100 -y 100 -c 0.5\n", 
                    argv[0], argv[0], argv[0]);
        return CMD_NO_ARG; 
    }
    int play = 0, stop = 0, ver = 0, pause = 0;
    char *file = NULL;
    ffplay_params_t params;

    memset(&params, 0, sizeof(ffplay_params_t));
    params.fullscreen = 1;
    params.x = -1;
    params.y = -1;
    params.scale = 1.0f;

    struct cfg_option options[] = {
        {"play",   		'p', NULL, CFG_BOOL,  (void *) &play,  	             0},
        {"stop",   		's', NULL, CFG_BOOL,  (void *) &stop,  	             0},
        {"pause",   	'a', NULL, CFG_BOOL,  (void *) &pause,  	         0},
        {"version",   	'v', NULL, CFG_BOOL,  (void *) &ver,  	             0},
        {"file",   		'f', NULL, CFG_STR,   (void *) &file,   	         0},
        {"format",   	'i', NULL, CFG_STR,   (void *) &params.force_format, 0},
        {"mode",        'm', NULL, CFG_INT,   (void *) &params.fullscreen,   0},
        {"x",   		'x', NULL, CFG_INT,   (void *) &params.x,   		 0},
        {"y",   		'y', NULL, CFG_INT,   (void *) &params.y,   		 0},
        {"scale",   	'c', NULL, CFG_FLOAT, (void *) &params.scale, 	     0},
        {"loop",        'l', NULL, CFG_BOOL,  (void *) &params.loop,         0},
        {"verbose",     'e', NULL, CFG_BOOL,  (void *) &params.verbose,      0},
        {"stat",        't', NULL, CFG_BOOL,  (void *) &params.show_stat,    0},
        {"fade-in",     'd', NULL, CFG_INT,   (void *) &params.fade_in,      0},
        CFG_END_OF_LIST
    };
    CMD_DEFAULT_ARGS_PARSER(options);

    if(stop) {
        ffplay_shutdown();
        return CMD_OK;
    }
    if(pause) {
        ffplay_toggle_pause();
        return CMD_OK;
    }
    if(ver) {
        ds_printf("DS_INFO: ffmpeg v%d.%d.%d build %02x\n", VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
    }
    if(play) {
        if(!file) {
            ds_printf("Error: -f or --file is required with -p\n");
            return CMD_ERROR;
        }
        ffplay(file, &params);
    }
    return CMD_OK; 
}

// extern AVCodec mp1_decoder;
// extern AVCodec mp2_decoder;
// extern AVCodec mp3_decoder;
extern AVCodec yamaha_adpcm_decoder;
extern AVCodec pcm_s16le_decoder;

int lib_open(klibrary_t *lib) {
    av_register_all();
    avcodec_register_all();
    av_register_protocol(&fs_protocol);

    // avcodec_register(&mp1_decoder);
    // avcodec_register(&mp2_decoder);
    // avcodec_register(&mp3_decoder);
    avcodec_register(&yamaha_adpcm_decoder);
    avcodec_register(&pcm_s16le_decoder);

    AddCmd("ffplay", "Multimedia player", (CmdHandler *) builtin_ffplay_cmd); 
    return nmmgr_handler_add(&ds_ffmpeg_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
    RemoveCmd(GetCmdByName("ffplay"));
    return nmmgr_handler_remove(&ds_ffmpeg_hnd.nmmgr); 
}
