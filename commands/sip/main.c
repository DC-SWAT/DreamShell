/* DreamShell ##version##

   sip.c
   Copyright (C) 2015 SWAT
*/

#include <ds.h>
#include <dc/maple.h>
#include <dc/maple/sip.h>
#include "wave_format.h"

#define SAMPLE_BUFF_SIZE 4096

static file_t fd = FILEHND_INVALID;
static uint32_t samples_size = 0, sample_buff_len = 0;
static uint8_t _sample_buff[SAMPLE_BUFF_SIZE] __attribute__((aligned(32)));
static uint8_t *sample_buff = _sample_buff;

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
		1,
		11025,
		0,
		0,
		16,
	},
	{
		{ 'd', 'a', 't', 'a' },
		0
	},
};

static void update_wav_header(uint16_t channels, uint32_t frequency, uint16_t bits, uint32_t size) {
	wave_hdr.wave_hdr.channels = channels;
	wave_hdr.wave_hdr.samplerate = frequency;
	wave_hdr.wave_hdr.bitwidth = bits;
	wave_hdr.wave_hdr.blockalign = channels * (bits / 8);
	wave_hdr.wave_hdr.bytespersec = frequency * channels * (bits / 8);
	wave_hdr.riff_hdr.filesize_minus_8 = sizeof(WAVE_HEADER_COMPLETE) + (size * 4) - 8; 
	wave_hdr.sub2.data_size = size * 4;
}

static void save_sample_buff(file_t fd) {
//	dbglog(DBG_DEBUG, "Saving sample buffer: %ld\n", sample_buff_len);
	fs_write(fd, _sample_buff, sample_buff_len);
	sample_buff_len = 0;
	sample_buff = _sample_buff;
}

static file_t create_wav_file(char *fn) {
	
	file_t fd;
	
	fd = fs_open(fn, O_CREAT | O_WRONLY | O_TRUNC);
	
	if(fd == FILEHND_INVALID) {
		return fd;
	}
	
	update_wav_header(1, 11025, 16, 0);
	
//	fs_seek(fd, sizeof(wave_hdr), SEEK_SET);
	fs_write(fd, &wave_hdr, sizeof(wave_hdr));
	return fd;
}

static void finish_wav_file(file_t fd) {
	
	if(sample_buff_len) {
		save_sample_buff(fd);
	}

	ssize_t rv;
	uint32_t size = samples_size;//fs_total(fd) - sizeof(wave_hdr);
	update_wav_header(1, 11025, 16, size);

	fs_seek(fd, 0, SEEK_SET);
	fs_write(fd, &wave_hdr, sizeof(wave_hdr));
	fs_complete(fd, &rv);
	fs_close(fd);
}


static void sip_sample_callback(maple_device_t *dev, uint8 *samples, size_t len) {
//	dbglog(DBG_DEBUG, "Sample: %d (%02x %02x %02x %02x)\n", len, samples[0], samples[1], samples[2], samples[3]);

	if(!len) {
		return;
	}

	samples_size += len;
	
	if(sample_buff_len + len > SAMPLE_BUFF_SIZE) {
		save_sample_buff(fd);
	}

	memcpy_sh4(sample_buff, samples, len);
	sample_buff_len += len;
	sample_buff += len;
}

static void wait_button_press() {
	
	ds_printf("DS_PROCESS: Recording... Press A button to stop process.\n");
	
	while(1) {
	
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);

		if(st->buttons & CONT_A) {
			return;
		}
		
		thd_sleep(500);
		MAPLE_FOREACH_END();
	}
}


int main(int argc, char *argv[]) {
    
    if(argc < 2) {
		ds_printf("Usage: %s options args...\n"
					"Options: \n"
					" -r, --record  -Start recording to file\n\n"
//					" -s, --stop    -Stop recording\n\n"
					"Arguments: \n"
					" -f, --file    -File for save audio data\n\n"
					"Example: %s -s -f /sd/audio.wav\n", argv[0], argv[0]);
		return CMD_NO_ARG; 
    } 

	/* Arguments */
	int record = 0, stop = 0;
	char *file = NULL;
	
	/* Device state */
	maple_device_t *sip;
//	sip_state_t *state;

	struct cfg_option options[] = {
		{"record", 'r', NULL, CFG_BOOL, (void *) &record, 0},
		{"stop",   's', NULL, CFG_BOOL, (void *) &stop,   0},
		{"file",   'f', NULL, CFG_STR,  (void *) &file,   0},
		CFG_END_OF_LIST
	};
  
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	sip = maple_enum_type(0, MAPLE_FUNC_MICROPHONE);

	if(!sip) {
		ds_printf("DS_ERROR: Couldn't find any attached devices, bailing out.\n");
		return CMD_ERROR;
	}

//	state = (sip_state_t *)maple_dev_status(sip);

	/* Start recording */
	if(record) {

		if(sip_set_gain(sip, SIP_DEFAULT_GAIN) < 0) {
			ds_printf("DS_ERROR: Couldn't set gain.\n");
			return CMD_ERROR;
		}
		
		if(sip_set_sample_type(sip, SIP_SAMPLE_16BIT_SIGNED) < 0) {
			ds_printf("DS_ERROR: Couldn't set sample type.\n");
			return CMD_ERROR;
		}
		
		if(sip_set_frequency(sip, SIP_SAMPLE_11KHZ) < 0) {
			ds_printf("DS_ERROR: Couldn't set frequency.\n");
			return CMD_ERROR;
		}
		
		if((fd = create_wav_file(file)) < 0) {
			ds_printf("DS_ERROR: Couldn't create file %s\n", file);
			return CMD_ERROR;
		}
		
		if(sip_start_sampling(sip, sip_sample_callback, 1) < 0) {
			ds_printf("DS_ERROR: Couldn't start sampling.\n");
			finish_wav_file(fd);
			return CMD_ERROR;
		}
		
		wait_button_press();
//		thd_sleep(10 * 1000);
		sip_stop_sampling(sip, 1);
		finish_wav_file(fd);
		
		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}

	/* Stop recording */
	if(stop) {
		sip_stop_sampling(sip, 1);
		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}

	return CMD_NO_ARG;
}
