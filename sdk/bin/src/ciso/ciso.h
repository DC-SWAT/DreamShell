/*
    This file is part of Ciso.

    Ciso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Ciso is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


    Copyright 2005 BOOSTER
    Copyright 2011-2013 SWAT
*/


#ifndef __CISO_H__
#define __CISO_H__
/*
	complessed ISO(9660) header format
*/
typedef struct ciso_header
{
	unsigned char magic[4];			/* +00 : 'C','I','S','O'                 */
	unsigned long header_size;		/* +04 : header size (==0x18)            */
	unsigned long long total_bytes;	/* +08 : number of original data size    */
	unsigned long block_size;		/* +10 : number of compressed block size */
	unsigned char ver;				/* +14 : version 01                      */
	unsigned char align;			/* +15 : align of index value            */
	unsigned char rsv_06[2];		/* +16 : reserved                        */
#if 0
// INDEX BLOCK
	unsigned int index[0];			/* +18 : block[0] index                  */
	unsigned int index[1];			/* +1C : block[1] index                  */
             :
             :
	unsigned int index[last];		/* +?? : block[last]                     */
	unsigned int index[last+1];		/* +?? : end of last data point          */
// DATA BLOCK
	unsigned char data[];			/* +?? : compressed or plain sector data */
#endif
}CISO_H;

/*
note:

file_pos_sector[n]  = (index[n]&0x7fffffff) << CISO_H.align
file_size_sector[n] = ( (index[n+1]&0x7fffffff) << CISO_H.align) - file_pos_sector[n]

if(index[n]&0x80000000)
  // read 0x800 without compress
else
  // read file_size_sector[n] bytes and decompress data
*/

#endif
