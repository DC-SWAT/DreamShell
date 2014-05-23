/* mp3junk.h
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
#include "id3.h"


/* This is mainly for displaying the stuff found in ID3 tags on the screen
   during mp3 file playback.  Nothing too extraordinary.
*/


int mp3_get_eq_band (int band);
int mp3_set_eq_band (int band, int value);
