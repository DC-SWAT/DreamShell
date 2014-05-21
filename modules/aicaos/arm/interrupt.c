
#include <stdint.h>

#include "interrupt.h"
#include "../aica_registers.h"

/* When F bit (resp. I bit) is set, FIQ (resp. IRQ) is disabled. */
#define F_BIT 0x40
#define I_BIT 0x80

void int_restore(uint32_t context)
{
	__asm__ volatile("msr CPSR_c,%0" : : "r"(context));
}

uint32_t int_disable(void)
{
	register uint32_t cpsr;
	__asm__ volatile("mrs %0,CPSR" : "=r"(cpsr) :);

	int_restore(cpsr | I_BIT | F_BIT);
	return cpsr;
}

uint32_t int_enable(void)
{
	register uint32_t cpsr;
	__asm__ volatile("mrs %0,CPSR" : "=r"(cpsr) :);

	int_restore(cpsr & ~(I_BIT | F_BIT));
	return cpsr;
}

int int_enabled(void)
{
	register uint32_t cpsr;
	__asm__ volatile("mrs %0,CPSR" : "=r"(cpsr) :);
	return !(cpsr & (I_BIT | F_BIT));
}

void int_acknowledge(void)
{
	*(unsigned int *) REG_ARM_INT_RESET = MAGIC_CODE;
	*(unsigned int *) REG_ARM_FIQ_ACK = 1;
}

/* Called from crt0.S */
void __attribute__((interrupt ("FIQ"))) bus_fiq_hdl(void)
{
	while(0x100 & *(volatile unsigned int *) REG_BUS_REQUEST);
}


/* Called from crt0.S */
void __attribute__((interrupt ("FIQ"))) timer_fiq_hdl(void)
{
}
