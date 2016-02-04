/* This file is part of the Dreamcast function library.
 * Please see libdream.c for further details.
 *
 * (c)2000 Dan Potter
 * modify BERO
 */
#include "aica.h"

/*
void SDL_DC_aica_init() {
	int i, j, old;
	
	SDL_DC_G2_LOCK_SIMPLE(old);
	SDL_DC_SNDREG32(0x2800) = 0x0000;
	
	for (i=0; i<64; i++) {
		for (j=0; j<0x80; j+=4) {
			if ((j&31)==0) SDL_DC_G2_FIFO_WAIT();
			SDL_DC_CHNREG32(i, j) = 0;
		}
		SDL_DC_G2_FIFO_WAIT();
		SDL_DC_CHNREG32(i,0) = 0x8000;
		SDL_DC_CHNREG32(i,20) = 0x1f;
	}

	SDL_DC_SNDREG32(0x2800) = 0x000f;
	SDL_DC_G2_FIFO_WAIT();
	SDL_DC_G2_UNLOCK_SIMPLE(old);
}
*/


/* Stop the sound on a given channel */
void SDL_DC_aica_stop(int ch) {
	g2_write_32(SDL_DC_CHNREGADDR(ch, 0),(g2_read_32(SDL_DC_CHNREGADDR(ch, 0)) & ~0x4000) | 0x8000);
	SDL_DC_G2_FIFO_WAIT();
}


/* The rest of these routines can change the channel in mid-stride so you
   can do things like vibrato and panning effects. */
   
/* Set channel volume */
void SDL_DC_aica_vol(int ch,int vol) {
	g2_write_32(SDL_DC_CHNREGADDR(ch, 40),(g2_read_32(SDL_DC_CHNREGADDR(ch, 40))&0xffff00ff)|(SDL_DC_AICAVOL(vol)<<8) );
	SDL_DC_G2_FIFO_WAIT();
}

/* Set channel pan */
void SDL_DC_aica_pan(int ch,int pan) {
	g2_write_32(SDL_DC_CHNREGADDR(ch, 36),(g2_read_32(SDL_DC_CHNREGADDR(ch, 36))&0xffffff00)|(SDL_DC_AICAPAN(pan)) );
	SDL_DC_G2_FIFO_WAIT();
}

/* Set channel frequency */
void SDL_DC_aica_freq(int ch,int freq) {
	g2_write_32(SDL_DC_CHNREGADDR(ch, 24),SDL_DC_AICAFREQ(freq));
	SDL_DC_G2_FIFO_WAIT();
}

