/*

Written by rofl0r.
LICENSE: GPL v2 
 
 gcc -Wall -Wextra raw2wav.c -o raw2wav
 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "wave_format.h"


size_t getfilesize(char *filename) {
	struct stat st;
	if(!stat(filename, &st)) {
		return st.st_size;
	} else
		return 0;
}

static WAVE_HEADER_COMPLETE wave_hdr = {
	{
		{ 'R', 'I', 'F', 'F'},
		0,
		{ 'W', 'A', 'V', 'E'},
	},
	{
		{ 'f', 'm', 't', ' '},
		16,
		{1, 0},
		0,
		44100,
		0,
		0,
		16,
	},
	{
		{ 'd', 'a', 't', 'a' },
		0
	},
};

#define MUSIC_BUF_SIZE 4096
#include <assert.h>
int raw2wav(char *rawname, char* wavname, int channels, int hz, int bitrate) {
	FILE* fd, *outfd;
	int len, kb = 0;
	char *p, *artist, *title;
	unsigned int size = getfilesize(rawname);
	unsigned char in[MUSIC_BUF_SIZE];
	unsigned char out[MUSIC_BUF_SIZE];
	
	fd = fopen(rawname, "r");
	
	outfd = fopen(wavname, "w");
	
	assert(channels == 1 || channels == 2);
	assert(hz == 48000 || hz == 44100 || hz == 22050 || hz == 11025);
	assert(bitrate == 32 || bitrate == 24 || bitrate == 16 || bitrate == 8);
	wave_hdr.wave_hdr.channels = channels;
	wave_hdr.wave_hdr.samplerate = hz;
	wave_hdr.wave_hdr.bitwidth = bitrate;
	wave_hdr.wave_hdr.blockalign = channels * (wave_hdr.wave_hdr.bitwidth / 8);
	wave_hdr.wave_hdr.bytespersec = wave_hdr.wave_hdr.samplerate * channels * (wave_hdr.wave_hdr.bitwidth / 8);
	wave_hdr.riff_hdr.filesize_minus_8 = sizeof(WAVE_HEADER_COMPLETE) + (size * 4) - 8; 
	wave_hdr.sub2.data_size = size * 4;
	
	fwrite(&wave_hdr, 1, sizeof(wave_hdr), outfd);
	

	while(size && (len = fread(in, 1, MUSIC_BUF_SIZE, fd))) {
		if(size < len) len = size;
		size -= len;
		fwrite(in, 1, len, outfd);
	}
	
	fclose(outfd);
	fclose(fd);
	
	return 0;
}

int syntax(void) {
	puts("raw2wav 1.0 by rofl0r\nsyntax: raw2wav rawfile wavfile channels(1/2) frequency(44100/22050/11025) bitrate(8/16)");
	return 1;
}


int main(int argc, char** argv) {
	if(argc != 6) return syntax();
	return !!raw2wav(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
}

