.globl _block8x8_copy

_block8x8_copy:
	fschg
	fmov	@r4,xd0
	add	r6,r4
	fmov	@r4,xd2
	add	r6,r4
	fmov	@r4,xd4
	add	r6,r4
	fmov	@r4,xd6
	add	r6,r4
	fmov	@r4,xd8
	add	r6,r4
	fmov	@r4,xd10
	add	r6,r4
	fmov	@r4,xd12
	add	r6,r4
	fmov	@r4,xd14
	add	r6,r4
	add	#64,r5
	fmov	xd14,@-r5
	fmov	xd12,@-r5
	fmov	xd10,@-r5
	fmov	xd8,@-r5
	fmov	xd6,@-r5
	fmov	xd4,@-r5
	fmov	xd2,@-r5
	fmov	xd0,@-r5
        rts
	fschg
