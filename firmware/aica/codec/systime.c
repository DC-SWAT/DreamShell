/******************************************************************************/
/*                                                                            */
/*  TIME.C:  Time Functions for 1000Hz Clock Tick                             */
/*                                                                            */
/******************************************************************************/
/*  ported to arm-elf-gcc / WinARM by Martin Thomas, KL, .de                  */
/*  <eversmith@heizung-thomas.de>                                             */
/*                                                                            */
/*  Based on a file that has been a part of the uVision/ARM development       */
/*  tools, Copyright KEIL ELEKTRONIK GmbH 2002-2004                           */
/******************************************************************************/

/*
  - mt: modified interrupt ISR handling, updated labels
*/

#include "AT91SAM7S64.h"
#include "Board.h"
#include "interrupt_utils.h"
#include "systime.h"
#include "keys.h"
#include "diskio.h"
#include "ir.h"

#ifdef ERAM  /* Fast IRQ functions Run in RAM  - see board.h */
#define ATTR RAMFUNC
#else
#define ATTR
#endif

volatile unsigned long systime_value;     /* Current Time Tick */            /* Current Time Tick */

void  NACKEDFUNC ATTR systime_isr(void) {        /* System Interrupt Handler */
  
	volatile AT91S_PITC * pPIT;
	
	ISR_ENTRY();
	
	pPIT = AT91C_BASE_PITC;
	
	if (pPIT->PITC_PISR & AT91C_PITC_PITS) {  /* Check PIT Interrupt */
		systime_value++;                       /* Increment Time Tick */
		//if ((systime_value % 500) == 0) {     /* 500ms Elapsed ? */
		//	*AT91C_PIOA_ODSR ^= LED4;          /* Toggle LED4 */
		//}
		if (systime_value % 128 == 0) {
			process_keys();
		}
		if (systime_value % 256 == 0) {
			disk_timerproc();
		}
		if (systime_value % 1 == 0) {
      ir_receive();
		}
		
		
		*AT91C_AIC_EOICR = pPIT->PITC_PIVR;    /* Ack & End of Interrupt */
	} 
	else {
		*AT91C_AIC_EOICR = 0;                   /* End of Interrupt */
	}
	
	ISR_EXIT();
}

void systime_init(void) {                    /* Setup PIT with Interrupt */
	volatile AT91S_AIC * pAIC = AT91C_BASE_AIC;
	
	//*AT91C_PIOA_OWER = LED4;     // LED4 ODSR Write Enable

	*AT91C_PITC_PIMR = AT91C_PITC_PITIEN |    /* PIT Interrupt Enable */ 
                     AT91C_PITC_PITEN  |    /* PIT Enable */
                     PIV;                   /* Periodic Interval Value */ 

	/* Setup System Interrupt Mode and Vector with Priority 7 and Enable it */
	pAIC->AIC_SMR[AT91C_ID_SYS] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | 7;
	pAIC->AIC_SVR[AT91C_ID_SYS] = (unsigned long) systime_isr;
	pAIC->AIC_IECR = (1 << AT91C_ID_SYS);
}

unsigned long systime_get(void)
{
	unsigned state;
	unsigned long ret;
	
	state = disableIRQ();
	ret = systime_value;
	restoreIRQ(state);
	
	return ret;
}

unsigned long systime_get_ms(void)
{
  return systime_get() / 10;
}