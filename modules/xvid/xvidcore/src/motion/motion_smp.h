/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - multithreaded Motion Estimation header -
 *
 *  Copyright(C) 2005 Radoslaw Czyz <xvid@syskin.cjb.net>
 *
 *  significant portions derived from x264 project,
 *  original authors: Trax, Gianluigi Tiesi, Eric Petit
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: motion_smp.h 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

#ifndef SMP_MOTION_H
#define SMP_MOTION_H

typedef struct
{
	pthread_t handle;		/* thread's handle */
	const FRAMEINFO *current;
	uint8_t * RefQ;
	int y_row;
	int y_step;
	int start_y;
	int stop_y;
	int * complete_count_self;
	int * complete_count_above;
	
	int MVmax, mvSum, mvCount;		/* out */

	uint32_t minfcode, minbcode;

	uint8_t *tmp_buffer;
	Bitstream *bs;

	Statistics *sStat;
	void *pEnc;
} SMPData;


void MotionEstimateSMP(SMPData * h);
void SMPMotionEstimationBVOP(SMPData * h);

#endif /* SMP_MOTION_H */
