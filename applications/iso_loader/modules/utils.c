/* DreamShell ##version##

   utils.c - ISO Loader app utils
   Copyright (C) 2022-2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "app_utils.h"
#include "audio/wav.h"
#include "settings.h"

/* Trim begin/end spaces and copy into output buffer */
void trim_spaces(char *input, char *output, int size) {
	char *p;
	char *o;
	int s = 0;
	size--;

	p = input;
	o = output;

	if(*p == '\0') {
		*o = '\0';
		return;
	}

	while(*p == ' ' && size > 0) {
		p++;
		size--;
	}

	if(!size) {
		*o = '\0';
		return;
	}

	while(size--) { 
		*o++ = *p++;
		s++;
	}

	*o = '\0';
	o--;

	while(*o == ' ' && s > 0) {
		*o = '\0';
		o--;
		s--;
	}
}

char *trim_spaces2(char *txt)
{
	int32_t i;
	
	while(txt[0] == ' ')
	{
		txt++;
	}
	
	int32_t len = strlen(txt);
	
	for(i=len; i ; i--)
	{
		if(txt[i] > ' ') break;
		txt[i] = '\0';
	}
	
	return txt;
}

char *fix_spaces(char *str)
{
	if(!str) return NULL;
	
	int i, len = (int) strlen(str);
	
	for(i=0; i<len; i++)
	{
		if(str[i] == ' ') str[i] = '\\';
	}
	
	return str;
}

int getDeviceType(const char *dir) {

	if(!strncasecmp(dir, "/cd", 3)) {
		return APP_DEVICE_CD;
	} else if(!strncasecmp(dir, "/sd",   3)) {
		return APP_DEVICE_SD;
	} else if(!strncasecmp(dir, "/ide",  4)) {
		return APP_DEVICE_IDE;
	} else if(!strncasecmp(dir, "/pc",   3)) {
		return APP_DEVICE_PC;
//	} else if(!strncasecmp(dir, "/???", 5)) {
//		return APP_DEVICE_NET;
	} else {
		return -1;
	}
}

int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename) {
	memset(filepath, 0, NAME_MAX);
	snprintf(filepath, NAME_MAX, "%s/%s/%s.gdi", fmPath, dirname, filename);

	if(FileExists(filepath)) {
		memset(filepath, 0, NAME_MAX);
		snprintf(filepath, NAME_MAX, "%s/%s.gdi", dirname, filename);
		return 1;
	}
	
	return 0;
}


void makeGameRelativePath(char *dst, size_t dst_size, const char *base_path, const char *game_path, const char *filename) {
    char game_dir[NAME_MAX];
    getFirstPathComponent(game_path, game_dir);
    snprintf(dst, dst_size, "%s/%s/%s", base_path, game_dir, filename);
}


void getFirstPathComponent(const char *path, char *result) {
    const char *p = strchr(path, '/');
    memset(result, 0, NAME_MAX);

    if (p) {
        size_t len = p - path;
        strncpy(result, path, len);
    }
}

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

