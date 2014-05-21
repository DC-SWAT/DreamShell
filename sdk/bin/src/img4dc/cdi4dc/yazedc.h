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

static int sectortype = MODE_2_FORM_1;

static const int load_offset = 24;

static const int encode_address = 0 + 75*2;

static const unsigned sect_size[1][2] = {
	/* nothing */
	//{0,0},
	
	/* Layer 1 decode/encode */
	//{ (L1_RAW+L1_Q+L1_P)*FRAMES_PER_SECTOR, L1_RAW*FRAMES_PER_SECTOR},
	
	/* Layer 2 decode/encode */
	{ 16 + L2_RAW + 12 + L2_Q + L2_P, L2_RAW},
	
	/* Layer 1 and 2 decode/encode */
/* 	{ (L1_RAW+L1_Q+L1_P)*FRAMES_PER_SECTOR, L1_RAW*FRAMES_PER_SECTOR}, */
	
	/* Subchannel decode/encode */
/* 	{ (LSUB_RAW+LSUB_Q+LSUB_P)*PACKETS_PER_SUBCHANNELFRAME,
	 LSUB_RAW*PACKETS_PER_SUBCHANNELFRAME},
	 */
	/* Layer 1 and subchannel decode/encode */
/* 	{ (L1_RAW+L1_Q+L1_P)*FRAMES_PER_SECTOR +
	   (LSUB_RAW+LSUB_Q+LSUB_P)*PACKETS_PER_SUBCHANNELFRAME,
	  LSUB_RAW*PACKETS_PER_SUBCHANNELFRAME +
	   L1_RAW*FRAMES_PER_SECTOR},
	 */
	/* Layer 2 and subchannel decode/encode */
/*	{ L2_RAW+L2_Q+L2_P+
	   (LSUB_RAW+LSUB_Q+LSUB_P)*PACKETS_PER_SUBCHANNELFRAME,
	  LSUB_RAW*PACKETS_PER_SUBCHANNELFRAME +
	   L2_RAW},
*/	
	/* Layer 1, 2 and subchannel decode/encode */
/*	{ (L1_RAW+L1_Q+L1_P)*FRAMES_PER_SECTOR +
	   (LSUB_RAW+LSUB_Q+LSUB_P)*PACKETS_PER_SUBCHANNELFRAME,
	  LSUB_RAW*PACKETS_PER_SUBCHANNELFRAME +
	   L1_RAW*FRAMES_PER_SECTOR},*/
}; 

int encode_sector(unsigned char *buf, unsigned int address);

#endif // __YAZEDC__H__
