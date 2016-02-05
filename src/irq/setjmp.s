! Adapted from the version in Newlib's mach/sh dir.

	.text
	.align	2
	.globl	_setjmp
_setjmp:
	add	#(13*4),r4
	sts.l	pr,@-r4

	fmov.s	fr15,@-r4	! call saved floating point registers
	fmov.s	fr14,@-r4
	fmov.s	fr13,@-r4
	fmov.s	fr12,@-r4

	mov.l	r15,@-r4	! call saved integer registers
	mov.l	r14,@-r4
	mov.l	r13,@-r4
	mov.l	r12,@-r4

	mov.l	r11,@-r4
	mov.l	r10,@-r4
	mov.l	r9,@-r4
	mov.l	r8,@-r4

	rts
	mov    #0,r0

	.align	2
	.globl	_longjmp
_longjmp:
	mov.l	@r4+,r8
	mov.l	@r4+,r9
	mov.l	@r4+,r10
	mov.l	@r4+,r11

	mov.l	@r4+,r12
	mov.l	@r4+,r13
	mov.l	@r4+,r14
	mov.l	@r4+,r15

	fmov.s	@r4+,fr12	! call saved floating point registers
	fmov.s	@r4+,fr13
	fmov.s	@r4+,fr14
	fmov.s	@r4+,fr15

	lds.l	@r4+,pr

	mov	r5,r0
	tst	r0,r0
	bf	retr4
	movt	r0
retr4:	rts
	nop
