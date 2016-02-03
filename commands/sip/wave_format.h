#ifndef WAVE_FORMAT_H
#define WAVE_FORMAT_H
#include <stdint.h>

typedef union {
	uint8_t chars[2];
	uint16_t shrt;
} u_char2;

//all values are LITTLE endian unless otherwise specified...
typedef struct {
	uint8_t text_RIFF[4];
	/*
	ChunkSize:  36 + SubChunk2Size, or more precisely:
	4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	This is the size of the rest of the chunk 
	following this number.  This is the size of the 
	entire file in bytes minus 8 bytes for the
	two fields not included in this count:
	ChunkID and ChunkSize.	 * */
	uint32_t filesize_minus_8;
	uint8_t text_WAVE[4];
} RIFF_HEADER;

typedef struct {
	uint8_t text_fmt[4];
	uint32_t formatheadersize; /* Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number. */
	u_char2 format; /* big endian, PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some 
                               form of compression. */
	uint16_t channels; //Mono = 1, Stereo = 2, etc
	uint32_t samplerate;
	uint32_t bytespersec; // ByteRate == SampleRate * NumChannels * BitsPerSample/8
	uint16_t blockalign; /* == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. */
	uint16_t bitwidth; //BitsPerSample    8 bits = 8, 16 bits = 16, etc.
} WAVE_HEADER;

typedef struct {
	uint8_t text_data[4]; //Contains the letters "data"
	uint32_t data_size; /* == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this 
                               number.*/
} RIFF_SUBCHUNK2_HEADER;

typedef struct {
	RIFF_HEADER riff_hdr;
	WAVE_HEADER wave_hdr;
	RIFF_SUBCHUNK2_HEADER sub2;
} WAVE_HEADER_COMPLETE;

static inline void* waveheader_get_data(const WAVE_HEADER_COMPLETE* hdr) {
	return (void*) (hdr+1);
}

static inline unsigned waveheader_get_datasize(const WAVE_HEADER_COMPLETE* hdr) {
	return hdr->riff_hdr.filesize_minus_8 + 8 - sizeof(*hdr);
}

#endif
