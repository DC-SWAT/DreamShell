/* mp3junk.c
 * Copyright (C) 2006 - 2009 Josh Sutherland <OneThirty8@aol.com>
 *
 * This file is part of VC/DC, a free MPEG-2 video player.
 * VC/DC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VC/DC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <kos.h>
#include "mpg123.h"

/* This is mainly for displaying the stuff found in ID3 tags on the screen
   during mp3 file playback.  And a simple and crappy EQ.  Nothing too extraordinary.
*/

real equalizer[32];
int enable_equalizer = 0;

void mp3_enable_eq (int enable)
{
  static int first_init = 0;
  int i;
  enable_equalizer = enable;
  if (!first_init)
  for (i=0; i< 32; i++)
    equalizer[i] = 1.0;
    
  first_init = 1;
}

int mp3_get_eq_band(int band)
{
  mp3_enable_eq(1);
  return (int)(equalizer[band]*100.0);
}

void mp3_set_eq_band(int band, int value)
{
  mp3_enable_eq(1);
  
  if (value > 0)
  {
    equalizer[band] = value / 100.0;
  }else
  {
    equalizer[band] = 0.0f;
  }
}

void mp3_set_eq(int setting)
{
   int i;
   mp3_enable_eq(1);
   switch (setting)
   {
      case 0: /* Scoop */
      	enable_equalizer = 1;
	/* Bass frequencies */
	equalizer[0] = 2.0;
	equalizer[1] = 2.0;
	equalizer[2] = 2.0;
      	for (i=3; i < 8; i++)
        	equalizer[i] = 2.0;
	equalizer[8] = 1.6;
	equalizer[9] = 1.4;
	equalizer[10]= 1.3;
	equalizer[11]= 1.1;
	
	/* Mid-range */
	equalizer[12]= 1.0;
	equalizer[13]= 0.6;
	equalizer[14]= 0.4;
	equalizer[15]= 0.2;
	equalizer[16]= 0.2;
	equalizer[17]= 0.4;
	equalizer[18]= 0.6;
	equalizer[19]= 1.0;
	
	/* Treble frequencies */     	
	equalizer[20] = 1.1;
	equalizer[21] = 1.3;
	equalizer[22] = 1.4;
	equalizer[23] = 1.6;
      	for (i=24; i < 29; i++)
        	equalizer[i] = 2.0;
	equalizer[29] = 2.0;
	equalizer[30] = 2.0;
	equalizer[31] = 2.0;
	 
      break;
      case 1:  /* Bass Boost */
      	enable_equalizer = 1;
	equalizer[0] = 2.0;
	equalizer[1] = 2.0;
	equalizer[2] = 2.0;
      	for (i=3; i < 8; i++)
        	equalizer[i] = 2.0;
	equalizer[8] = 1.6;
	equalizer[9] = 1.4;
	equalizer[10]= 1.2;
	equalizer[11]= 1.1;
	for (i=12; i < 32; i++)
	  equalizer[i] = 1.0;
      break;
      case 2:  /* Treble Reduce */
      	enable_equalizer = 1;
	for (i=0; i< 20; i++)
		equalizer[i] = 1.0;
	equalizer[20] = 0.8;
	equalizer[21] = 0.6;
	equalizer[22] = 0.4;
	equalizer[23] = 0.2;
      	for (i=24; i < 29; i++)
        	equalizer[i] = 0.05;
	equalizer[29] = 0.2;
	equalizer[30] = 0.4;
	equalizer[31] = 0.6;
			
      break;
      default:
      break;
   }
   
}

/* Does basically the same thing as mpg123's equalizer.c.  We don't really need a 'stereo' EQ, do we?
   Plus 'equalizer.c' isn't one of the files that's allowed to be used under the GPL (I don't get that,
   really, but whatever) so we have to write our own function anyway.  Not that there's much to it--just
   simple multiplication of two floating-point values.  This might be overkill as far as 'necessary features'
   goes, but whatever. */
void mp3_do_equalizer(real *bandPtr) 
{
   int i;
    if(enable_equalizer) {
	for(i=0;i<32;i++)
	  bandPtr[i] = bandPtr[i] * equalizer[i];
    }

}
