/* This header contains all info needed for encoding an ISO Mode 2 Form 1 sector. */

#ifndef __LIBEDC_H__
#define __LIBEDC_H__

// libedc includes
#include "patch.h"
#include "ecc.h"
#include "edc.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDC_LOAD_OFFSET 24
#define EDC_ENCODE_ADDRESS 0 + (75 * 2)
#define EDC_SECTOR_SIZE_DECODE (16 + L2_RAW + 12 + L2_Q + L2_P) /* Layer 2 decode */
#define EDC_SECTOR_SIZE_ENCODE L2_RAW /* Layer 2 encode */
#define EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE (LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME + (L2_RAW + L2_Q + L2_P) * FRAMES_PER_SECTOR

int edc_encode_sector(unsigned char *buf, unsigned int address);

#endif /* __LIBEDC_H__ */
