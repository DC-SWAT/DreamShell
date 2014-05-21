/*
from http://www.mikrocontroller.net/topic/48465
*/

#include "AT91SAM7S64.h"
#include "Board.h"

#include "keys.h"
#include "interrupt_utils.h"

#include <stdio.h>

#define KEY_INPUT		(*AT91C_PIOA_PDSR)

#define REPEAT_MASK (1<<KEY1)	// repeat: key1
#define REPEAT_START	50		// * 10ms
#define REPEAT_NEXT	22		// * 10ms


volatile unsigned long key_state;				// debounced and inverted key state:
							// bit = 1: key pressed
volatile unsigned long key_press;				// key press detect
volatile unsigned long key_rpt;				// key long press and repeat


void process_keys(void)		// every 10ms
{
  static unsigned long ct0, ct1, rpt;
  unsigned long i;

  i = key_state ^ ~KEY_INPUT;		// key changed ?
  ct0 = ~( ct0 & i );			// reset or count ct0
  ct1 = ct0 ^ (ct1 & i);		// reset or count ct1
  i &= ct0 & ct1;			// count until roll over ?
  key_state ^= i;			// then toggle debounced state
  key_press |= key_state & i;		// 0->1: key press detect

  if( (key_state & REPEAT_MASK) == 0 )	// check repeat function
     rpt = REPEAT_START;		// start delay
  if( --rpt == 0 ){
    rpt = REPEAT_NEXT;			// repeat delay
    key_rpt |= key_state & REPEAT_MASK;
  }
}


long get_key_press( long key_mask )
{
	unsigned state;
	state = disableIRQ();
	key_mask &= key_press;                        // read key(s)
	key_press ^= key_mask;                        // clear key(s)
	restoreIRQ(state);
	return key_mask;
}


long get_key_rpt( long key_mask )
{
	unsigned state;
	state = disableIRQ();
	key_mask &= key_rpt;                        	// read key(s)
	key_rpt ^= key_mask;                        	// clear key(s)
	restoreIRQ(state);
	return key_mask;
}


long get_key_short( long key_mask )
{
	long x;
	unsigned state;
	state = disableIRQ();
  x = get_key_press( ~key_state & key_mask );
  restoreIRQ(state);
  return x;
}


long get_key_long( long key_mask )
{
  return get_key_press( get_key_rpt( key_mask ));
}

void key_init(void)
{
	// enable PIO
	*AT91C_PIOA_PER = (1<<KEY0)|(1<<KEY1);
	// disable output
	*AT91C_PIOA_ODR = (1<<KEY0)|(1<<KEY1);
}
