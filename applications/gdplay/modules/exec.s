! DreamShell ##version##
!
!   exec.s
!   (c)2016 megavolt85
!

	.globl		_gdplay_run_game

	.text
	.align		2

_gdplay_run_game:

	mov.l		.ccraddr,r0	! Disable/invalidate cache
	mov.w		.ccrdata,r1
	mov.l		@r1,r1
	mov.l		r1,@r0
	
	mov.l	.start_copy,r3
	mov.w	.count,r1
.loop:
	mov.l	@r4,r2
	mov.l	r2,@r3
	add		#4,r4
	dt		r1
	bf.s	.loop
	add		#4,r3
	mov.l	.start_copy,r3
	add		#32,r3
	mov.w	.debug_hnd,r4
	jmp		@r3
	nop

	.align		2
.start_copy:
	.long		0x8C000100
.ccraddr:
	.long		0xff00001c
.ccrdata:
	.word		0x0808
.count:
	.word		0x3FC0
.debug_hnd:
	.word		0x0FFF
