#include "libedc.h"

int edc_encode_sector(unsigned char *buf, unsigned int address) {
	return do_encode_L2(buf, MODE_2_FORM_1, address);
}
