/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	6 july 2006, v0.2b
	
	File : 	YAZEDC.H
	Desc : 	This header contains all info needed for encoding an ISO Mode 2 Form 1 sector.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#ifndef __YAZEDC__H__
#define __YAZEDC__H__

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

#define LOAD_OFFSET 24
#define ENCODE_ADDRESS 0 + (75 * 2)
#define SECTOR_SIZE_DECODE (16 + L2_RAW + 12 + L2_Q + L2_P) /* Layer 2 decode/encode */
#define SECTOR_SIZE_ENCODE L2_RAW
#define MODE_2_FORM_1_DATA_SECTOR_SIZE (LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME + (L2_RAW + L2_Q + L2_P) * FRAMES_PER_SECTOR

int encode_sector(unsigned char *buf, unsigned int address);

#endif // __YAZEDC__H__
