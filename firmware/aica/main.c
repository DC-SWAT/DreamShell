/* DreamShell ##version##

   main.c
   Copyright (C) 2000-2002 Dan Potter
   Copyright (C) 2009-2014 SWAT
   
   Generic sound driver with streaming capabilities

   This slightly more complicated version allows for sound effect channels,
   and full sampling rate, panning, and volume control for each.

*/

#include "drivers/aica_cmd_iface.h"
#include "aica.h"
#include "dec.h"


/****************** Timer *******************************************/

#define timer (*((volatile uint32 *)AICA_MEM_CLOCK))

void timer_wait(int jiffies) {
	int fin = timer + jiffies;
	while (timer <= fin)
		;
}

/****************** Tiny Libc ***************************************/

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t count) {
	unsigned char *tmp = (unsigned char *) dest;
	unsigned char *s = (unsigned char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

void *memset(void *dest, int c, size_t count) {
	unsigned char *tmp = (unsigned char *) dest;

	while (count--)
		*tmp++ = c;

	return dest;
}

void *memmove (void *dest0, void const *source0, size_t length) {
  char *dest = dest0;
  char const *source = source0;
  if (source < dest)
    /* Moving from low mem to hi mem; start at end.  */
    for (source += length, dest += length; length; --length)
      *--dest = *--source;
  else if (source != dest)
    {
      /* Moving from hi mem to low mem; start at beginning.  */
      for (; length; --length)
        *dest++ = *source++;
    }
  return dest0;
}

static void *memptr = (void*)AICA_RAM_START;

void *malloc(size_t size) {
	void *ptr = memptr;
	memset(ptr, 0, size);
	memptr += size + 4;
	return ptr;
}

void free(void *p) {
	return;
}

/****************** Main Program ************************************/

/* Our SH-4 interface (statically placed memory structures) */
volatile aica_queue_t	*q_cmd = (volatile aica_queue_t *)AICA_MEM_CMD_QUEUE;
volatile aica_queue_t	*q_resp = (volatile aica_queue_t *)AICA_MEM_RESP_QUEUE;
volatile aica_channel_t	*chans = (volatile aica_channel_t *)AICA_MEM_CHANNELS;

/* Process a CHAN command */
void process_chn(uint32 chn, aica_channel_t *chndat) {
	switch(chndat->cmd & AICA_CH_CMD_MASK) {
	case AICA_CH_CMD_NONE:
		break;
	case AICA_CH_CMD_START:
		if (chndat->cmd & AICA_CH_START_SYNC) {
			aica_sync_play(chn);
		} else {
			memcpy((void*)(chans+chn), chndat, sizeof(aica_channel_t));
			chans[chn].pos = 0;
			aica_play(chn, chndat->cmd & AICA_CH_START_DELAY);
		}
		break;
	case AICA_CH_CMD_STOP:
		aica_stop(chn);
		break;
	case AICA_CH_CMD_UPDATE:
		if (chndat->cmd & AICA_CH_UPDATE_SET_FREQ) {
			chans[chn].freq = chndat->freq;
			aica_freq(chn);
		}
		if (chndat->cmd & AICA_CH_UPDATE_SET_VOL) {
			chans[chn].vol = chndat->vol;
			aica_vol(chn);
		}
		if (chndat->cmd & AICA_CH_UPDATE_SET_PAN) {
			chans[chn].pan = chndat->pan;
			aica_pan(chn);
		}
		break;
	default:
		/* error */
		break;
	}
}


void process_decoder(aica_decoder_t *dat) {
	
	switch(dat->cmd & AICA_DECODER_CMD_MASK) {
		case AICA_DECODER_CMD_NONE:
			break;
		case AICA_DECODER_CMD_INIT:
		
			memptr = (void*)dat->base;
			
			if(dat->codec & AICA_CODEC_MP3) {
				init_mp3();
			}
			
			if(dat->codec & AICA_CODEC_AAC) {
				//init_aac();
			}
		
			break;
		case AICA_DECODER_CMD_SHUTDOWN:
			if(dat->codec & AICA_CODEC_MP3) {
				shutdown_mp3();
			}
			
			if(dat->codec & AICA_CODEC_AAC) {
				//shutdown_aac();
			}
			break;
		case AICA_DECODER_CMD_DECODE:
			switch(dat->codec) {
				case AICA_CODEC_MP3:
				
					decode_mp3(dat);
					
					break;
				case AICA_CODEC_AAC:
					break;
				default:
					break;
			}
			break;
		default:
			/* error */
			break;
	}
}

/* Process one packet of queue data */
uint32 process_one(uint32 tail) {
	uint32		pktdata[AICA_CMD_MAX_SIZE], *pdptr, size, i;
	volatile uint32	* src;
	aica_cmd_t	* pkt;

	src = (volatile uint32 *)(q_cmd->data + tail);
	pkt = (aica_cmd_t *)pktdata;
	pdptr = pktdata;

	/* Get the size field */
	size = *src;
	if (size > AICA_CMD_MAX_SIZE)
		size = AICA_CMD_MAX_SIZE;

	/* Copy out the packet data */
	for (i=0; i<size; i++) {
		*pdptr++ = *src++;
		if ((uint32)src >= (q_cmd->data + q_cmd->size))
			src = (volatile uint32 *)q_cmd->data;
	}
	
	/* Figure out what type of packet it is */
	switch (pkt->cmd) {
	case AICA_CMD_NONE:
		break;
	case AICA_CMD_CPU_CLOCK:
	/*
		// 0x28a8 - ARMClock - GUESS
		+--------------+
		| 31-8  |  7-0 |
		| n/a   |  mhz |
		+--------------+
			
			mhz:		sets the speed of the ARM7 CPU in MHz steps. 
						00 = 1 MHz, 24 = 25 MHz
						don't try to overclock the ARM7 it may get burned or something.	
	*/
		SNDREG32(0x28a8) = pkt->cmd_id; //0x0019
		break;
	case AICA_CMD_DECODER:
		process_decoder((aica_decoder_t *)pkt->cmd_data);
		break;
	case AICA_CMD_PING:
		/* Not implemented yet */
		break;
	case AICA_CMD_CHAN:
		process_chn(pkt->cmd_id, (aica_channel_t *)pkt->cmd_data);
		break;
	case AICA_CMD_SYNC_CLOCK:
		/* Reset our timer clock to zero */
		timer = 0;
		break;
	default:
		/* error */
		break;
	}

	return size;
}

/* Look for an available request in the command queue; if one is there
   then process it and move the tail pointer. */
void process_cmd_queue() {
	uint32		head, tail, tsloc, ts;

	/* Grab these values up front in case SH-4 changes head */
	head = q_cmd->head;
	tail = q_cmd->tail;

	/* Do we have anything to process? */
	while (head != tail) {
		/* Look at the next packet. If our clock isn't there yet, then
		   we won't process anything yet either. */
		tsloc = tail + offsetof(aica_cmd_t, timestamp);
		if (tsloc >= q_cmd->size)
			tsloc -= q_cmd->size;
		ts = *((volatile uint32*)(q_cmd->data + tsloc));
		if (ts > 0 && ts >= timer)
			return;

		/* Process it */
		ts = process_one(tail);

		/* Ok, skip over the packet */
		tail += ts * 4;
		if (tail >= q_cmd->size)
			tail -= q_cmd->size;
		q_cmd->tail = tail;
	}
}

int arm_main() {
	int i;

	/* Setup our queues */
	q_cmd->head = q_cmd->tail = 0;
	q_cmd->data = AICA_MEM_CMD_QUEUE + sizeof(aica_queue_t);
	q_cmd->size = AICA_MEM_RESP_QUEUE - q_cmd->data;
	q_cmd->process_ok = 1;
	q_cmd->valid = 1;

	q_resp->head = q_resp->tail = 0;
	q_resp->data = AICA_MEM_RESP_QUEUE + sizeof(aica_queue_t);
	q_resp->size = AICA_MEM_CHANNELS - q_resp->data;
	q_resp->process_ok = 1;
	q_resp->valid = 1;

	/* Initialize the AICA part of the SPU */
	aica_init();

	/* Wait for a command */
	for( ; ; ) {
		/* Update channel position counters */
		for (i=0; i<64; i++)
			aica_get_pos(i);

		/* Check for a command */
		if (q_cmd->process_ok)
			process_cmd_queue();

		/* Little delay to prevent memory lock */
		timer_wait(10);
	}
}
