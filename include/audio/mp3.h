/** 
 *  \file   	mp3.h
 *	\brief  	MP3 playing
 *	\date		2004-2014
 *	\author		SWAT
 *	\copyright	http://www.dc-swat.ru
 */
 
#ifndef _DS_MP3_H
#define _DS_MP3_H

int sndmp3_start(const char * fname, int loop);
int sndmp3_start_block(const char * fname);
int sndmp3_stop();
int sndmp3_fastforward();
int sndmp3_rewind();
int sndmp3_restart();
int sndmp3_pause();


#endif
