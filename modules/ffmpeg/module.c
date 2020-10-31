/* DreamShell ##version##

   module.c - ffmpeg module
   Copyright (C)2011-2014 SWAT 
*/
            
#include "ds.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

DEFAULT_MODULE_HEADER(ffmpeg);

static int file_open(URLContext *h, const char *filename, int flags) {
	file_t fd;
	int _flags = 0;
	char fn[MAX_FN_LEN];
	char proto[32];
	
	sscanf(filename, "%[a-zA-Z0-9_]:%s", proto, fn);
	
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
			ds_printf("DS_ERROR_FFMPEG: Uknown file open flag: %08x\n", flags);
			return -1;
	}

	if ((fd = fs_open(fn, _flags)) < 0) {
		return -1;
	}
	h->priv_data = (void*)fd;
	return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size) {
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

/*
static int file_read_pause(URLContext *h, int pause) {
	return 0;
}

static int64_t file_read_seek(URLContext *h, int stream_index, int64_t timestamp, int flags) {
	return 0;
}
*/

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


int ffplay(const char *filename, const char *force_format);
//int sdl_ffplay(const char *filename);

int builtin_ffplay_cmd(int argc, char *argv[]) { 

	if(argc == 1) {
		ds_printf("Usage: %s option args...\n\n"
					"Options: \n"
					" -v, --version  -Show ffmpeg version\n"
					//" -s, --sdl      -Use SDL_ffmpeg (slow)\n"
					" -p, --play     -Start playing\n\n"
					"Arguments: \n"
					" -i  --format   -Force format detection\n"
					" -f, --file     -File for playing\n\n"
					"Examples: %s -p -f /cd/file.avi\n", argv[0], argv[0]);
		return CMD_NO_ARG; 
	} 

	int play = 0, /*use_sdl = 0, */ver = 0;
	char *file = NULL, *format = NULL;

	struct cfg_option options[] = {
		{"play",   		'p', NULL, CFG_BOOL, (void *) &play,  	0},
		//{"sdl",   		's', NULL, CFG_BOOL, (void *) &use_sdl, 0},
		{"version",   	'v', NULL, CFG_BOOL, (void *) &ver,  	0},
		{"file",   		'f', NULL, CFG_STR,  (void *) &file,   	0},
		{"format",   	'i', NULL, CFG_STR,  (void *) &format,  0},
		CFG_END_OF_LIST
	};
  
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	if(ver) {
		ds_printf("DS: ffmpeg v%d.%d.%d build %02x\n", VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
	}

	if(play) {
		//if(use_sdl) {
		//	sdl_ffplay(file);
		//} else {
			ffplay(file, format);
		//}
	}
	
	return CMD_OK; 
} 

int lib_open(klibrary_t *lib) {
	av_register_all();
	av_register_protocol(&fs_protocol);
	AddCmd("ffplay", "Video/Audio player via ffmpeg", (CmdHandler *) builtin_ffplay_cmd); 
	return nmmgr_handler_add(&ds_ffmpeg_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
	RemoveCmd(GetCmdByName("ffplay"));
	return nmmgr_handler_remove(&ds_ffmpeg_hnd.nmmgr); 
}
