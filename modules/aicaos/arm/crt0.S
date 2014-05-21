
.text
.align

.extern __aica_init
.extern aica_sh4_fiq_hdl
.extern bus_fiq_hdl
.extern timer_fiq_hdl
.extern current_task

#include "../aica_registers.h"

.globl reset
reset:
	# Exception vectors
	b start
	b undef
	b softint
	b pref_abort
	b data_abort
	b rsrvd
	b irq_hdl

	# Load the FIQ code, and jump to
	# the corresponding function.
	ldr r8, reg_arm_fiq_code
	ldr r8, [r8]
	and r8, r8, #7

	mov r9,#SH4_INTERRUPT_INT_CODE
	cmp r8,r9
	bne other_fiq

	# Save r0-r7 and r14 (old PC) inside the thread structure
	ldr r8, =current_task
	ldr r8, [r8]
	stmia r8, {r0-r7,r14}

	# Save the banked CPSR for later
	mrs r1, SPSR

	# Switch to Supervisor mode
	# and disable interrupts
	mrs r9, CPSR
	orr r9, r9, #0xd3
	msr CPSR_c, r9
	nop
	nop
	nop
	nop

	# Save r8-r14 and the CPSR
	ldr r0, =current_task
	ldr r0, [r0]
	add r0, r0, #36
	stmia r0!, {r8-r14}
	str r1, [r0]

	# Use the FIQ stack for the moment
	ldr sp,=__fiq_stack

	# Jump to the C handler
	b aica_sh4_fiq_hdl

other_fiq:
	mov r9,#TIMER_INTERRUPT_INT_CODE
	cmp r8,r9
	beq timer_fiq_hdl

	mov r9,#BUS_REQUEST_INT_CODE
	cmp r8,r9
	beq bus_fiq_hdl

undef:
softint:
	movs pc,lr

data_abort:
	subs pc,lr,#8

pref_abort:
rsrvd:
irq_hdl:
	subs pc,lr,#4

start:
	# Setup a basic stack
	ldr sp,=__stack

	# Switch to FIQ mode
	mrs r0,CPSR
	bic r0,r0,#0x1f
	orr r0,r0,#0x12
	msr CPSR_c,r0
	nop
	nop
	nop
	nop

	# Setup the FIQ stack
	ldr sp,=__fiq_stack

	# Switch back to supervisor mode
	orr r0,r0,#0x13
	msr CPSR_c,r0
	nop
	nop
	nop
	nop

	ldr r0,=__bss_end
	ldr r1,=__bss_start
	mov r2,#0

	clear_bss_loop:
		str r2,[r0,#-4]!
		cmp r0,r1
		bhi clear_bss_loop

	# Call the C "__aica_init" function
	bl __aica_init

program_end:
	# Loop infinitely when we get here
	b program_end

reg_arm_fiq_code:
	.word REG_ARM_FIQ_CODE
reg_arm_fiq_ack:
	.word REG_ARM_FIQ_ACK
reg_arm_int_reset:
	.word REG_ARM_INT_RESET

.end

