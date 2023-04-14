/* KallistiOS ##version##

   timer.c
   Copyright (C)2000, 2001, 2002,2004 Dan Potter
   Copyright (C)2014-2023 SWAT
   Copyright (C)2023 Andress Barajas
*/

//#include <stdio.h>
#include <arch/timer.h>
#include <arch/irq.h>

/* Quick access macros */
#define TIMER8(o) ( *((volatile uint8*)(0xffd80000 + (o))) )
#define TIMER16(o) ( *((volatile uint16*)(0xffd80000 + (o))) )
#define TIMER32(o) ( *((volatile uint32*)(0xffd80000 + (o))) )
#define TOCR	0x00
#define TSTR	0x04
#define TCOR0	0x08
#define TCNT0	0x0c
#define TCR0	0x10
#define TCOR1	0x14
#define TCNT1	0x18
#define TCR1	0x1c
#define TCOR2	0x20
#define TCNT2	0x24
#define TCR2	0x28
#define TCPR2	0x2c

static int tcors[] = { TCOR0, TCOR1, TCOR2 };
static int tcnts[] = { TCNT0, TCNT1, TCNT2 };
static int tcrs[] = { TCR0, TCR1, TCR2 };

/* Pre-initialize a timer; set values but don't start it */
int timer_prime(int which, uint32 speed, int interrupts) {
	/* P0/64 scalar, maybe interrupts */
	if (interrupts) {
		TIMER16(tcrs[which]) = 32 | 2;
	} else {
		TIMER16(tcrs[which]) = 2;
	}

	/* Initialize counters; formula is P0/(tps*64) */
	TIMER32(tcnts[which]) = 50000000 / (speed*64);
	TIMER32(tcors[which]) = 50000000 / (speed*64);

	if(interrupts) {
		timer_enable_ints(which);
	}
	return 0;
}

/* Pre-initialize a timer for CDDA; set values but don't start it */
int timer_prime_cdda(int which, uint32 count, int interrupts) {

	/* Stop timer */
	TIMER8(TSTR) &= ~(1 << which);

	/* External clock, maybe interrupts */
	if (interrupts) {
		TIMER16(tcrs[which]) = 32 | 3;
	} else {
		TIMER16(tcrs[which]) = 3;
	}

	/* Initialize counters */
	TIMER32(tcnts[which]) = count;
	TIMER32(tcors[which]) = count;

	/* Clears the timer underflow bit */
	TIMER16(tcrs[which]) &= ~0x100;

	if(interrupts) {
		timer_enable_ints(which);
	}
	return 0;
}

/* Pre-initialize a timer as do it the BIOS; set values but don't start it */
int timer_prime_bios(int which) {

	/* Stop timer */
	TIMER8(TSTR) &= ~(1 << which);
	
	TIMER16(tcrs[which]) = 2;
	TIMER32(tcnts[which]) = 0xFFFFFFFF;
	TIMER32(tcors[which]) = 0xFFFFFFFF;
	
	/* Clears the timer underflow bit */
	TIMER16(tcrs[which]) &= ~0x100;
	
	return 0;
}

/* Start a timer -- starts it running (and interrupts if applicable) */
int timer_start(int which) {
	TIMER8(TSTR) |= 1 << which;
	return 0;
}

/* Stop a timer -- and disables its interrupt */
int timer_stop(int which) {
	/* Stop timer */
	TIMER8(TSTR) &= ~(1 << which);

	return 0;
}

/* Clears the timer underflow bit and returns what its value was */
int timer_clear(int which) {
	uint16 value = TIMER16(tcrs[which]);
	TIMER16(tcrs[which]) &= ~0x100;
	
	return (value & 0x100) ? 1 : 0;
}

/* Returns the count value of a timer */
uint32 timer_count(int which) {
	return TIMER32(tcnts[which]);
}

/* Spin-loop kernel sleep func: uses the secondary timer in the
   SH-4 to very accurately delay even when interrupts are disabled */
void timer_spin_sleep(int ms) {
	timer_stop(TMU1);
	timer_prime(TMU1, 1000, 0);
	timer_clear(TMU1);
	timer_start(TMU1);

	while (ms > 0) {
		while (!(TIMER16(tcrs[TMU1]) & 0x100))
			;
		timer_clear(TMU1);
		ms--;
	}

	timer_stop(TMU1);
}

void timer_spin_sleep_bios(int ms) {
	
	uint32 before, cur, cnt = 782 * ms;
	before = timer_count(TMU0);
	
	do {
		cur = before - timer_count(TMU0);
	} while(cur < cnt);
}

/* Enable timer interrupts (high priority); needs to move
   to irq.c sometime. */
void timer_enable_ints(int which) {
	volatile uint16 *ipra = (uint16*)0xffd00004;
	*ipra |= (0x000f << (12 - 4 * which));
}

/* Disable timer interrupts; needs to move to irq.c sometime. */
void timer_disable_ints(int which) {
	volatile uint16 *ipra = (uint16*)0xffd00004;
	*ipra &= ~(0x000f << (12 - 4 * which));
}

/* Check whether ints are enabled */
int timer_ints_enabled(int which) {
	volatile uint16 *ipra = (uint16*)0xffd00004;
	return (*ipra & (0x000f << (12 - 4 * which))) != 0;
}

/* Init */
int timer_init() {
	/* Disable all timers */
	TIMER8(TSTR) = 0;
	
	/* Set to internal clock source */
	TIMER8(TOCR) = 0;

	return 0;
}

/* Shutdown */
void timer_shutdown() {
	/* Disable all timers */
	TIMER8(TSTR) = 0;
	timer_disable_ints(TMU0);
	timer_disable_ints(TMU1);
	timer_disable_ints(TMU2);
}


/* Quick access macros */
#define PMCR_CTRL(o)  ( *((volatile uint16*)(0xff000084) + (o << 1)) )
#define PMCTR_HIGH(o) ( *((volatile uint32*)(0xff100004) + (o << 1)) )
#define PMCTR_LOW(o)  ( *((volatile uint32*)(0xff100008) + (o << 1)) )

#define PMCR_CLR        0x2000
#define PMCR_PMST       0x4000
#define PMCR_PMENABLE   0x8000
#define PMCR_RUN        0xc000
#define PMCR_PMM_MASK   0x003f

#define PMCR_CLOCK_TYPE_SHIFT 8

/* 5ns per count in 1 cycle = 1 count mode(PMCR_COUNT_CPU_CYCLES) */
#define NS_PER_CYCLE      5

/* Get a counter's current configuration */
uint16 perf_cntr_get_config(int which) {
	return PMCR_CTRL(which);
}

/* Start a performance counter */
int perf_cntr_start(int which, int mode, int count_type) {
	perf_cntr_clear(which);
	PMCR_CTRL(which) = PMCR_RUN | mode | (count_type << PMCR_CLOCK_TYPE_SHIFT);
	
	return 0;
}

/* Stop a performance counter */
int perf_cntr_stop(int which) {
	PMCR_CTRL(which) &= ~(PMCR_PMM_MASK | PMCR_PMENABLE);

	return 0;
}

/* Clears a performance counter.  Has to stop it first. */
int perf_cntr_clear(int which) {
	perf_cntr_stop(which);
	PMCR_CTRL(which) |= PMCR_CLR;

	return 0;
}

/* Returns the count value of a counter */
inline uint64 perf_cntr_count(int which) {
	return (uint64)(PMCTR_HIGH(which) & 0xffff) << 32 | PMCTR_LOW(which);
}

void timer_ns_enable() {
	perf_cntr_start(PRFC0, PMCR_ELAPSED_TIME_MODE, PMCR_COUNT_CPU_CYCLES);
}

void timer_ns_disable() {
	uint16 config = PMCR_CTRL(PRFC0);

	/* If timer is running, disable it */
	if((config & PMCR_ELAPSED_TIME_MODE)) {
		perf_cntr_clear(PRFC0);
	}
}

inline uint64 timer_ns_gettime64() {
	uint64 cycles = perf_cntr_count(PRFC0);
	return cycles * NS_PER_CYCLE;
}
