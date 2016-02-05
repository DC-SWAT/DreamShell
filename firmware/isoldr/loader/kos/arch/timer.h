/* KallistiOS ##version##

   arch/dreamcast/include/timer.h
   Copyright(c)2000-2001,2004 Dan Potter

   $Id: timer.h,v 1.5 2003/02/15 02:45:52 bardtx Exp $

*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <arch/irq.h>

/* Timer sources -- we get four on the SH4 */
#define TMU0	0	/* Off limits during thread operation */
#define TMU1	1	/* Used for timer_spin_sleep() */
#define TMU2	2	/* Used for timer_get_ticks() */
#define WDT	3	/* Not supported yet */

/* The main timer for the task switcher to use */
#define TIMER_ID TMU0

/* Pre-initialize a timer; set values but don't start it */
int timer_prime(int which, uint32 speed, int interrupts);

/* Pre-initialize a timer for CDDA; set values but don't start it */
int timer_prime_cdda(int which, uint32 count);

/* Pre-initialize a timer as do it the BIOS; set values but don't start it */
int timer_prime_bios(int which);

/* Start a timer -- starts it running (and interrupts if applicable) */
int timer_start(int which);

/* Stop a timer -- and disables its interrupt */
int timer_stop(int which);

/* Returns the count value of a timer */
uint32 timer_count(int which);

/* Clears the timer underflow bit and returns what its value was */
int timer_clear(int which);

/* Spin-loop kernel sleep func: uses the secondary timer in the
   SH-4 to very accurately delay even when interrupts are disabled */
void timer_spin_sleep(int ms);

/* Spin-loop kernel sleep func: uses the first timer in the
   SH-4 to very accurately delay even when interrupts are disabled */
void timer_spin_sleep_bios(int ms);

/* Enable timer interrupts (high priority); needs to move
   to irq.c sometime. */
void timer_enable_ints(int which);

/* Disable timer interrupts; needs to move to irq.c sometime. */
void timer_disable_ints(int which);

/* Check whether ints are enabled */
int timer_ints_enabled(int which);

/* Enable the millisecond timer */
void timer_ms_enable();
void timer_ms_disable();

/* Return the number of ticks since KOS was booted */
void timer_ms_gettime(uint32 *secs, uint32 *msecs);

/* Does the same as timer_ms_gettime(), but it merges both values
   into a single 64-bit millisecond counter. May be more handy
   in some situations. */
uint64 timer_ms_gettime64();

/* Set the callback target for the primary timer. Set to NULL
   to disable callbacks. Returns the address of the previous
   handler. */
typedef void (*timer_primary_callback_t)(irq_context_t *);
timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t callback);

/* Request a wakeup in approximately N milliseconds. You only get one
   simultaneous wakeup. Any subsequent calls here will replace any 
   pending wakeup. */
void timer_primary_wakeup(uint32 millis);

/* Init function */
int timer_init();

/* Shutdown */
void timer_shutdown();

__END_DECLS

#endif	/* __ARCH_TIMER_H */

