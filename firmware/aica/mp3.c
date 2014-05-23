
#include <stddef.h>
#include "drivers/aica_cmd_iface.h"
#include "aica.h"
#include "mp3dec.h"
	
static HMP3Decoder hMP3Decoder;
static MP3FrameInfo mp3FrameInfo;
static aica_channel_t mp3_chan[2];

extern volatile aica_channel_t *chans;

void * memcpy(void *dest, const void *src, size_t count);

int init_mp3() {
	if(!hMP3Decoder)
		hMP3Decoder = MP3InitDecoder();
	return 0;
}

void shutdown_mp3() {
	MP3FreeDecoder(hMP3Decoder);
}

int decode_mp3(aica_decoder_t *dat) {
	
	int offset = 0, i = 0;
	int bytes = dat->length;
	unsigned char *buf = (unsigned char*)dat->base;
	short *out = (short*)dat->out;
	
	offset = MP3FindSyncWord(buf, bytes);
	
	if(offset < 0) {
		return -1;
	}
	
	buf += offset;
	bytes -= offset;
	
	if(MP3Decode(hMP3Decoder, &buf, &bytes, out, 0)) {
		return -1;
	}
	
	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	
	for(i = 0; i < mp3FrameInfo.nChans; i++) {
		mp3_chan[i].cmd = AICA_CH_CMD_START | AICA_CH_START_DELAY;
		mp3_chan[i].base = dat->chan[i];
		mp3_chan[i].type = AICA_SM_16BIT;
		mp3_chan[i].length = (mp3FrameInfo.outputSamps / mp3FrameInfo.nChans);
		mp3_chan[i].loop = 1;
		mp3_chan[i].loopstart = 0;
		mp3_chan[i].loopend = (mp3FrameInfo.outputSamps / mp3FrameInfo.nChans);
		mp3_chan[i].freq = mp3FrameInfo.samprate;
		mp3_chan[i].vol = 255;
		mp3_chan[i].pan = 0;
		
		memcpy((void*)(chans + dat->chan[i]), &mp3_chan[i], sizeof(aica_channel_t));
		chans[dat->chan[i]].pos = 0;
		aica_play(dat->chan[i], 0);
	}
	
	return mp3FrameInfo.nChans;
}

