/*
** Copyright (C) 2000 Albert L. Faber
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

BOOL InitMP3(PMPSTR mp);
int	 decodeMP3(PMPSTR mp,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);
void ExitMP3(PMPSTR mp);

/* added decodeMP3_unclipped to support returning raw floating-point values of samples. The representation
   of the floating-point numbers is defined in mpg123.h as #define real. It is 64-bit double by default. 
   No more than 1152 samples per channel are allowed. */
int	 decodeMP3_unclipped(PMPSTR mp,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);

/* Moved this in here so we can decimate the audio on high-bitrate a/v streams--
  --Josh Sutherland 6/14/2006 */
int
decodeMP3_clipchoice( PMPSTR mp,unsigned char *in,int isize,char *out,int *done,      
                           int (*synth_1to1_mono_ptr)(PMPSTR,real *,unsigned char *,int *),
                           int (*synth_1to1_ptr)(PMPSTR,real *,int,unsigned char *, int *) );
			   
/* added remove_buf to support mpglib seeking */
void remove_buf(PMPSTR mp);

#ifdef __cplusplus
}
#endif

#endif

