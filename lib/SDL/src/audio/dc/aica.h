#ifndef _SDL_DC_AICA_H_
#define _SDL_DC_AICA_H_

#include <arch/irq.h>
#include <sys/types.h>
#include <dc/g2bus.h>

#define	SDL_DC_AICA_MEM	0xa0800000

#define SDL_DC_SM_8BIT	1
#define SDL_DC_SM_16BIT	0
#define SDL_DC_SM_ADPCM	2

#define sdl_dc_snd_base ((volatile unsigned char *)0xa0700000) /* dc side */

#define	SDL_DC_SPU_RAM_BASE	0xa0800000

/* Some convienence macros */
#define	SDL_DC_SNDREGADDR(x)	(0xa0700000 + (x))
#define	SDL_DC_CHNREGADDR(ch,x)	SDL_DC_SNDREGADDR(0x80*(ch)+(x))


#define SDL_DC_SNDREG32(x)	(*(volatile unsigned long *)SDL_DC_SNDREGADDR(x))
#define SDL_DC_SNDREG8(x)	(*(volatile unsigned char *)SDL_DC_SNDREGADDR(x))
#define SDL_DC_CHNREG32(ch, x) (*(volatile unsigned long *)SDL_DC_CHNREGADDR(ch,x))
#define SDL_DC_CHNREG8(ch, x)	(*(volatile unsigned long *)SDL_DC_CHNREGADDR(ch,x))

#define SDL_DC_G2_LOCK_SIMPLE(OLD) \
	do { \
		if (!irq_inside_int()) \
			OLD = irq_disable(); \
		/* suspend any G2 DMA here... */ \
		while((*(volatile unsigned int *)0xa05f688c) & 0x20) \
			; \
	} while(0)

#define SDL_DC_G2_UNLOCK_SIMPLE(OLD) \
	do { \
		/* resume any G2 DMA here... */ \
		if (!irq_inside_int()) \
			irq_restore(OLD); \
	} while(0)

#define SDL_DC_DMAC_CHCR3 *((vuint32 *)0xffa0003c)

#define SDL_DC_G2_LOCK(OLD1, OLD2) \
	do { \
		OLD1 = irq_disable(); \
		OLD2 = SDL_DC_DMAC_CHCR3; \
		SDL_DC_DMAC_CHCR3 = OLD2 & ~1; \
		while((*(vuint32 *)0xa05f688c) & 0x20) \
			; \
	} while(0)

#define SDL_DC_G2_UNLOCK(OLD1, OLD2) \
	do { \
		SDL_DC_DMAC_CHCR3 = OLD2; \
		irq_restore(OLD1); \
	} while(0)

#define SDL_DC_G2_WRITE_32(ADDR,VALUE) \
	*((vuint32*)ADDR) = VALUE

#define SDL_DC_G2_READ_32(ADDR) \
	*((vuint32*)ADDR)

#define SDL_DC_G2_FIFO_WAIT() \
	{ \
		vuint32 const *g2_fifo = (vuint32*)0xa05f688c; \
		int i; \
		for (i=0; i<0x1800; i++) \
			if (!(*g2_fifo & 0x11)) break; \
	} 


/* Translates a volume from linear form to logarithmic form (required by
   the AICA chip */
const static unsigned char sdl_dc_logs[] = {
	0, 15, 22, 27, 31, 35, 39, 42, 45, 47, 50, 52, 55, 57, 59, 61,
	63, 65, 67, 69, 71, 73, 74, 76, 78, 79, 81, 82, 84, 85, 87, 88,
	90, 91, 92, 94, 95, 96, 98, 99, 100, 102, 103, 104, 105, 106,
	108, 109, 110, 111, 112, 113, 114, 116, 117, 118, 119, 120, 121,
	122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
	135, 136, 137, 138, 138, 139, 140, 141, 142, 143, 144, 145, 146,
	146, 147, 148, 149, 150, 151, 152, 152, 153, 154, 155, 156, 156,
	157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167,
	167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176,
	177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185,
	186, 187, 187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194,
	195, 195, 196, 197, 197, 198, 199, 199, 200, 200, 201, 202, 202,
	203, 204, 204, 205, 205, 206, 207, 207, 208, 209, 209, 210, 210,
	211, 212, 212, 213, 213, 214, 215, 215, 216, 216, 217, 217, 218,
	219, 219, 220, 220, 221, 221, 222, 223, 223, 224, 224, 225, 225,
	226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 232, 232, 233,
	233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 239, 239, 240,
	240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246,
	247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 254, 255
};

/* For the moment this is going to have to suffice, until we really
   figure out what these mean. */
#define SDL_DC_AICAPAN(x) ((x)==0x80?(0):((x)<0x80?(0x1f):(0x0f)))
#define SDL_DC_AICAVOL(x) (0xff - sdl_dc_logs[128 + (((x) & 0xff) / 2)])

static __inline__ unsigned  SDL_DC_AICAFREQ(unsigned freq)	{
	unsigned long freq_lo, freq_base = 5644800;
	int freq_hi = 7;

	/* Need to convert frequency to floating point format
	   (freq_hi is exponent, freq_lo is mantissa)
	   Formula is ferq = 44100*2^freq_hi*(1+freq_lo/1024) */
	while (freq < freq_base && freq_hi > -8) {
		freq_base >>= 1;
		--freq_hi;
	}
	while (freq < freq_base && freq_hi > -8) {
		freq_base >>= 1;
		freq_hi--;
	}
	freq_lo = (freq<<10) / freq_base;
	return (freq_hi << 11) | (freq_lo & 1023);
}

static __inline__ void SDL_DC_aica_play(int ch,int mode,unsigned long smpptr,int loopst,int loopend,int freq,int vol,int pan,int loopflag) {
	int val;
	int old;

	SDL_DC_G2_LOCK_SIMPLE(old);
	SDL_DC_G2_WRITE_32(SDL_DC_CHNREGADDR(ch, 0),(SDL_DC_G2_READ_32(SDL_DC_CHNREGADDR(ch, 0)) & ~0x4000) | 0x8000);
	SDL_DC_CHNREG32(ch, 8) = loopst & 0xffff;
	SDL_DC_CHNREG32(ch, 12) = loopend & 0xffff;
	SDL_DC_CHNREG32(ch, 24) = SDL_DC_AICAFREQ(freq);
	SDL_DC_CHNREG32(ch, 36) = SDL_DC_AICAPAN(pan) | (0xf<<8);
	vol = SDL_DC_AICAVOL(vol);
	SDL_DC_CHNREG32(ch, 40) = 0x24 | (vol<<8);
	SDL_DC_CHNREG32(ch, 16) = 0x1f;	/* No volume envelope */
	SDL_DC_CHNREG32(ch, 4) = smpptr & 0xffff;
	val = 0xc000 | 0x0000 | (mode<<7) | (smpptr >> 16);
	if (loopflag) val|=0x200;
	SDL_DC_CHNREG32(ch, 0) = val;
	SDL_DC_G2_UNLOCK_SIMPLE(old);
	SDL_DC_G2_FIFO_WAIT();
}

static __inline__ int SDL_DC_aica_get_pos(int ch) {
	int ret;
	unsigned old1,old2;
	SDL_DC_G2_LOCK(old1, old2);
	SDL_DC_G2_WRITE_32(SDL_DC_SNDREGADDR(0x280c),(SDL_DC_G2_READ_32(SDL_DC_SNDREGADDR(0x280c))&0xffff00ff) | (ch<<8));
	SDL_DC_G2_FIFO_WAIT();
	ret = SDL_DC_G2_READ_32(SDL_DC_SNDREGADDR(0x2814)) & 0xffff;
	SDL_DC_G2_UNLOCK(old1, old2);
	return ret;
}


void SDL_DC_aica_stop(int ch);
void SDL_DC_aica_vol(int ch,int vol);
void SDL_DC_aica_pan(int ch,int pan);
void SDL_DC_aica_freq(int ch,int freq);

#endif
