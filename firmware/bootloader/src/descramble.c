/**
 * DreamShell boot loader
 * Descramble binary
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#include "main.h"

#define MAXCHUNK 0x200000
static uint seed;

static inline void my_srand(uint n) {
    seed = n & 0xffff;
}

static inline uint my_rand(void) {
    seed = (seed * 2109 + 9273) & 0x7fff;
    return (seed + 0xc000) & 0xffff;
}

static void load(uint8 *dest, uint32 size) {
	
    static uint8 *source;

    if (!size) {
        source = dest;
        return;
    }

    memcpy(dest, source, size);
    source += size;
}

static inline void handle_chunk(uint8 *ptr, int sz) {
	
    int idx[MAXCHUNK / 32];
    int i;

    /* Convert chunk size to number of slices */
    sz /= 32;

    /* Initialize index table with unity,
        so that each slice gets loaded exactly once */
    for(i = 0; i < sz; i++)
        idx[i] = i;

    for(i = sz-1; i >= 0; --i)
    {
        /* Select a replacement index */
        int x = (my_rand() * i) >> 16;

        /* Swap */
        int tmp = idx[i];
        idx[i] = idx[x];
        idx[x] = tmp;

        /* Load resulting slice */
        load(ptr + 32 * idx[i], 32);
    }
}

void descramble(uint8 *source, uint8 *dest, uint32 size) {

    load(source, 0);
    my_srand(size);

    /* Descramble 2 meg blocks for as long as possible, then
        gradually reduce the window down to 32 bytes (1 slice) */
    for(uint32 chunksz = MAXCHUNK; chunksz >= 32; chunksz >>= 1)
    {
        while(size >= chunksz)
        {
	        handle_chunk(dest, chunksz);
	        size -= chunksz;
	        dest += chunksz;
        }
    }

    /* !!! Load final incomplete slice */
    if(size)
        load(dest, size);
}
