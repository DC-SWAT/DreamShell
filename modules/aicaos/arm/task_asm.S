
.text
.align

.extern current_task
.extern __task_reschedule
.globl __task_select
.globl task_reschedule

__task_select:

	# Restore r8-r14
	add r1, r0, #36
	ldmia r1, {r8-r14}

	# Switch to FIQ mode, to mask r8-r14
	mrs r1, CPSR
	bic r1, r1, #0x1f
	orr r1, r1, #0x12
	msr CPSR_c, r1
	nop
	nop
	nop
	nop

	# Restore the CPSR
	ldr r8, [r0, #64]
	msr SPSR, r8

	# Restore r0-r7 and r15
	mov r8, r0
	ldmia r8, {r0-r7, r15}^
	nop
	nop
	nop
	nop

task_reschedule:
	
	# Save r0-r7 on the task structure
	ldr r0, =current_task
	ldr r0, [r0]
	stmia r0,{r0-r7}

	# Save r8-r14 and the CPSR
	add r0, r0, #36
	stmia r0!, {r8-r14}
	mrs r1, CPSR
	str r1, [r0]

	# Save r14 as PC
	str pc, [r0, #-32]
	b __task_reschedule
	nop
	mov pc, lr

