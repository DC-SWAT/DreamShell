/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	5 july 2006, v0.2b
	
	File : 	CDIBUILD.H
	Desc : 	Header for all the cdi*.* files.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#ifndef __CDIBUILD__H__
#define __CDIBUILD__H__

#include <stdio.h>
#include "cdihead.h"
#include "libedc.h"

int write_gap_tracks(FILE* cdi);
int write_data_track(FILE* cdi, FILE* iso);
void write_cdi_header_start(FILE* cdi, char* cdiname);
void write_cdi_head_end(FILE* cdi, char* volumename, uint32_t total_cdi_space_used, long cdi_end_image_tracks);

#endif // __CDIBUILD__H__
