/* DreamShell ##version##

   sfx.c
   DreamShell sound FX
   Copyright (C) 2024-2025 SWAT
   Copyright (C) 2025 megavolt85
*/

#include <stdlib.h>
#include <string.h>

#include <kos/thread.h>
#include <dc/sound/stream.h>
#include <dc/sound/sfxmgr.h>
#include <zlib/zlib.h>

#include <sfx.h>
#include <utils.h>
#include <settings.h>

/* Filename of raw ADPCM file in DS/sfx/ directory 
   or filename of raw ADPCM packed to gzipped file (.gz extension) in /rd directory
*/
static char *stream_sfx_name[DS_SFX_LAST_STREAM] = {
	"startup.raw.gz"
};

/* Filename of wav file (without extension) in DS/sfx directory */
static char *sys_sfx_name[DS_SFX_LAST - DS_SFX_LAST_STREAM] = {
	"click",
	"click2",
	"screenshot",
	"move",
	"chpage",
	"slide",
	"error",
	"success"
};

static sfxhnd_t sys_sfx_hnd[DS_SFX_LAST - DS_SFX_LAST_STREAM] = {
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID,
	SFXHND_INVALID
};

static void *snd_stream_buf;
static int snd_stream_buf_pos;
static size_t snd_stream_buf_size;

static void *snd_stream_callback(snd_stream_hnd_t hnd, int req, int *done) {

	if(snd_stream_buf == NULL) {
		*done = 0;
		return NULL;
	}
	void *result = snd_stream_buf + snd_stream_buf_pos;

	if(snd_stream_buf_pos + req > snd_stream_buf_size) {
		snd_stream_buf = NULL;
	}
	else {
		snd_stream_buf_pos += req;
	}
	*done = req;
	return result;
}

static void *snd_stream_thread(void *params) {
	snd_stream_hnd_t hnd = (snd_stream_hnd_t)params;

	while(snd_stream_buf != NULL) {
		if(snd_stream_poll(hnd) < 0) {
			break;
		}
		thd_sleep(50);
	}

	snd_stream_destroy(hnd);
	free(snd_stream_buf);
	return NULL;
}

static void *load_raw_gz(const char *filename, size_t *sz) {
	gzFile fp;
	void *data = NULL;
	size_t size;
	
	size = gzip_get_file_size(filename);
	
	if(size == 0 || size > (2 << 20)) {
		return data;
	}
	fp = gzopen(filename, "r");

	if(fp == NULL) {
		return data;
	}
	data = aligned_alloc(32, size);

	if(data == NULL) {
		gzclose(fp);
		return data;
	}
	if(gzread(fp, data, size) != size) {
		free(data);
		data = NULL;
	}
	gzclose(fp);
	
	*sz = size;
	
	return data;
}

static void *load_raw_adpcm(const char *filename, size_t *sz) {
	file_t fp;
	void *data = NULL;
	size_t size;
	
	size = FileSize(filename);
	
	if(size == 0 || size > (2 << 20)) {
		return data;
	}
	fp = fs_open(filename, O_RDONLY);

	if(fp == FILEHND_INVALID) {
		return data;
	}
	data = aligned_alloc(32, size);

	if(data == NULL) {
		fs_close(fp);
		return data;
	}
	if(fs_read(fp, data, size) != size) {
		free(data);
		data = NULL;
	}
	fs_close(fp);
	
	*sz = size;
	
	return data;
}

static int ds_sfx_is_enabled(ds_sfx_t sfx) {
	Settings_t *settings = GetSettings();

	if(!settings || settings->audio.volume == 0) {
		return 0;
	}

	switch(sfx) {
		case DS_SFX_STARTUP:
			return settings->audio.startup_enabled;
		case DS_SFX_CLICK:
			return settings->audio.sfx_enabled && settings->audio.click_enabled;
		case DS_SFX_CLICK2:
			return settings->audio.sfx_enabled && settings->audio.hover_enabled;
		default:
			return settings->audio.sfx_enabled;
	}
}

static int ds_sfx_get_volume() {
	Settings_t *settings = GetSettings();
	
	if(!settings) {
		return 230;
	}
	
	return settings->audio.volume;
}

static int ds_sfx_play_stream(ds_sfx_t sfx) {
	char sfx_path[NAME_MAX];
	
	if(sfx >= DS_SFX_LAST_STREAM) {
		return -1;
	}

	int pos = strlen(stream_sfx_name[sfx]) - 3;

	if(!strncmp(&stream_sfx_name[sfx][pos], ".gz", 3)) {
		snprintf(sfx_path, NAME_MAX, "/rd/%s", stream_sfx_name[sfx]);
		snd_stream_buf = load_raw_gz(sfx_path, &snd_stream_buf_size);
	}
	else {
		snprintf(sfx_path, NAME_MAX, "%s/sfx/%s", getenv("PATH"), sys_sfx_name[sfx]);
		snd_stream_buf = load_raw_adpcm(sfx_path, &snd_stream_buf_size);
	}

	if(!snd_stream_buf) {
		return -1;
	}

	snd_stream_buf_pos = 0;
	snd_stream_hnd_t snd_stream_hnd = snd_stream_alloc(snd_stream_callback, SND_STREAM_BUFFER_MAX_ADPCM);

	if(snd_stream_hnd < 0) {
		return -1;
	}

	snd_stream_start_adpcm(snd_stream_hnd, 44100, 1);
	snd_stream_volume(snd_stream_hnd, ds_sfx_get_volume());

	thd_create(1, snd_stream_thread, (void *)snd_stream_hnd);
	return 0;
}

int ds_sfx_play(ds_sfx_t sfx) {
	if(sfx >= DS_SFX_LAST) {
		return -1;
	}

	if(!ds_sfx_is_enabled(sfx)) {
		return 0;
	}

	if (sfx < DS_SFX_LAST_STREAM) {
		return ds_sfx_play_stream(sfx);
	}

	int sfx_sel = sfx - DS_SFX_LAST_STREAM;

	if (sys_sfx_hnd[sfx_sel] == SFXHND_INVALID) {
		char sfx_path[NAME_MAX];

		snprintf(sfx_path, NAME_MAX, "%s/sfx/%s.wav", getenv("PATH"), sys_sfx_name[sfx_sel]);
		sys_sfx_hnd[sfx_sel] = snd_sfx_load(sfx_path);

		if (sys_sfx_hnd[sfx_sel] == SFXHND_INVALID) {
			return -1;
		}
	}

	snd_sfx_play(sys_sfx_hnd[sfx_sel], ds_sfx_get_volume(), 128);
	return 0;
}
