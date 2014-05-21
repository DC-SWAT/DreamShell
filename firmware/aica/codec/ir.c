/*
 *  based on Peter Danneggers Code (http://www.mikrocontroller.net/topic/12216)
*/

#include "AT91SAM7S64.h"
#include "Board.h"
#include "ir.h"

#include "systime.h"
#include "interrupt_utils.h"

static unsigned int ir_address(unsigned int received);
static unsigned int ir_command(unsigned int received);

unsigned int	rc5_bit;				// bit value
unsigned int	rc5_time;				// count bit time
unsigned int	rc5_tmp;				// shift bits in
volatile unsigned int	rc5_data;				// store received data

#define IRINPUT   (*AT91C_PIOA_PDSR)
#define IRPIN     30

#define RC5TIME   1.778e-3		// 1.778msec
#define PULSE_MIN	(unsigned int)((float)TCK * RC5TIME * 0.4 + 0.5)
#define PULSE_1_2	(unsigned int)((float)TCK * RC5TIME * 0.8 + 0.5)
#define PULSE_MAX	(unsigned int)((float)TCK * RC5TIME * 1.2 + 0.5)

void ir_receive()
{
  unsigned int tmp = rc5_tmp;				// for faster access

  if( ++rc5_time > PULSE_MAX ){			// count pulse time
    // only if 14 bits received
    if( !(tmp & 0x4000) && tmp & 0x2000 )	{ 
      // check for correct address (CD player, see http://www.sbprojects.com/knowledge/ir/rc5.htm)
      if(ir_address(tmp) >= 0x14 && ir_address(tmp) <= 0x20) {
        rc5_data = tmp;
      }
    }
    tmp = 0;
  }

  if( (rc5_bit ^ IRINPUT) & 1<<IRPIN ){		// change detect
    rc5_bit = ~rc5_bit;				// 0x00 -> 0xFF -> 0x00

    if( rc5_time < PULSE_MIN )			// to short
      tmp = 0;

    if( !tmp || rc5_time > PULSE_1_2 ){		// start or long pulse time
      if( !(tmp & 0x4000) )			// not to many bits
        tmp <<= 1;				// shift
      if( !(rc5_bit & 1<<IRPIN) )		// inverted bit
        tmp |= 1;				// insert new bit
      rc5_time = 0;				// count next pulse time
    }
  }

  rc5_tmp = tmp;
}

// return last received command; -1 if no new command is available
int ir_get_cmd(void)
{
	int x;
	unsigned state;
	state = disableIRQ();
	if (rc5_data) {
    x = ir_command(rc5_data);
    rc5_data = 0;
  } else {
    x = -1;
  }
  restoreIRQ(state);
  return x;
}

void ir_init(void)
{
	// enable PIO
	*AT91C_PIOA_PER = (1<<IRPIN);
	// disable output
	*AT91C_PIOA_ODR = (1<<IRPIN);
}

static unsigned int ir_address(unsigned int received)
{
  // 5 address bits, starting from bit 7
  return (received >> 6) & 0x1F;
}

static unsigned int ir_command(unsigned int received)
{
  // 6 command bits
  return received & 0x3F;
}
