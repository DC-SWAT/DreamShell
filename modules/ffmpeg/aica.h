#ifndef _AICA_H_
#define _AICA_H_

#define	AICA_MEM	0xa0800000

#define SM_8BIT		1
#define SM_16BIT	0
#define SM_ADPCM	2

void aica_play(int ch,int mode,unsigned long smpptr,int looptst,int loopend,int freq,int vol,int pan,int loopflag);
void aica_stop(int ch);
void aica_vol(int ch,int vol);
void aica_pan(int ch,int pan);
void aica_freq(int ch,int freq);
int aica_get_pos(int ch);

#endif
