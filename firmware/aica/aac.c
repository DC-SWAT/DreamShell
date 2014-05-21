
#include <stddef.h>
#include "aica_cmd_iface.h"
#include "aica.h"
#include "aacdec.h"
	
static HAACDecoder *hAACDecoder;
static AACFrameInfo aacFrameInfo;

int init_aac() {
	if(!hAACDecoder)
		hAACDecoder = (HAACDecoder *)AACInitDecoder();
	return 0;
}

void shutdown_aac() {
	AACFreeDecoder(hAACDecoder);
}

int decode_aac() {
	return 0;
}