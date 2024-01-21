/* DreamShell ##version##

   sfx.c
   DreamShell sound FX
   Copyright (C) 2024 SWAT
*/

#include <sfx.h>
#include <utils.h>

#include <kos/thread.h>
#include <dc/sound/stream.h>

#include <zlib/zlib.h>

static void *snd_stream_buf;
static int snd_stream_buf_pos;
static size_t snd_stream_buf_size;

static void *snd_stream_callback(snd_stream_hnd_t hnd, int req, int* done) {

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

static void *load_raw_gz(const char *filename, size_t size) {
	gzFile fp;
	void *data = NULL;

	if(size == 0 || size > (2 << 20)) {
		return data;
	}
	fp = gzopen(filename, "r");

	if(fp == NULL) {
		return data;
	}
	data = memalign(32, size);

	if(data == NULL) {
		gzclose(fp);
		return data;
	}
	if(gzread(fp, data, size) != size) {
		free(data);
		data = NULL;
	}
	gzclose(fp);
	return data;
}

int ds_sfx_play(ds_sfx_t sfx) {
	const char *startup_fn = "/rd/startup.raw.gz";

	if(sfx != DS_SFX_STARTUP) {
		return -1;
	}

	snd_stream_buf_pos = 0;
	snd_stream_buf_size = gzip_get_file_size(startup_fn);
	snd_stream_buf = load_raw_gz(startup_fn, snd_stream_buf_size);

	snd_stream_hnd_t snd_stream_hnd = snd_stream_alloc(snd_stream_callback, SND_STREAM_BUFFER_MAX / 2);

	if(snd_stream_hnd < 0) {
		return -1;
	}

	snd_stream_start_adpcm(snd_stream_hnd, 44100, 1);
	snd_stream_volume(snd_stream_hnd, 240);

	thd_create(1, snd_stream_thread, (void *)snd_stream_hnd);
	return 0;
}
