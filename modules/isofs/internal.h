/** 
 * \file    internal.h
 * \brief   isofs utils
 * \date    2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _ISOFS_INTERNAL_H
#define _ISOFS_INTERNAL_H

#include <arch/types.h>


#define TRACK_FLAG_PREEMPH   0x10 /* Pre-emphasis (audio only) */
#define TRACK_FLAG_COPYPERM  0x20 /* Copy permitted */
#define TRACK_FLAG_DATA      0x40 /* Data track */
#define TRACK_FLAG_FOURCHAN  0x80 /* 4-channel audio */


int read_sectors_data(file_t fd, uint32 sector_count, 
						uint16 sector_size, uint8 *buff);


#endif /* _ISOFS_INTERNAL_H */
