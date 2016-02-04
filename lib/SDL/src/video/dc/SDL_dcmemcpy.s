.text
.align 1
.global _dcsdl_memcpy
.type _dcsdl_memcpy,function

_dcsdl_memcpy:
       	mov	r4,r0
       	add	r6,r0
       	sub	r4,r5
     	mov	#11,r1
       	cmp/hs	r1,r6
       	add	#-1,r5
      	bf	L_cleanup
       	mov	r5,r3
       	add	r0,r3
      	shlr	r3
     	mov	r4,r7
       	bt	L_even
      	mov.b	@(r0,r5),r2
      	add	#-1,r3
      	mov.b	r2,@-r0
L_even:
       	tst	#1,r0
     	add	#-1,r5
       	add	#8,r7
   	bf	L_odddst
      	tst	#2,r0
      	bt	L_al4dst
      	add	#-1,r3
       	mov.w	@(r0,r5),r1
       	mov.w	r1,@-r0
L_al4dst:
    	shlr	r3
     	bt	L_al4both
       	mov.w	@(r0,r5),r1
       	swap.w	r1,r1
     	add	#4,r7
       	add	#-4,r5
L_2l_loop:
      	mov.l	@(r0,r5),r2
      	xtrct	r2,r1
      	mov.l	r1,@-r0
       	cmp/hs	r7,r0
      	mov.l	@(r0,r5),r1
      	xtrct	r1,r2
      	mov.l	r2,@-r0
      	bt	L_2l_loop
     	bra	L_cleanup
    	add	#5,r5
      	nop	
L_al4both:
       	add	#-2,r5
L_al4both_loop:
      	mov.l	@(r0,r5),r1
       	cmp/hs	r7,r0
     	mov.l	r1,@-r0
     	bt	L_al4both_loop
     	bra	L_cleanup
     	add	#3,r5
       	nop	
L_odddst:
       	shlr	r3
       	bt	L_al4src
     	mov.w	@(r0,r5),r1
       	mov.b	r1,@-r0
       	shlr8	r1
       	mov.b	r1,@-r0
L_al4src:
      	add	#-2,r5
L_odd_loop:
   	mov.l	@(r0,r5),r2
      	cmp/hs	r7,r0
      	mov.b	r2,@-r0
     	shlr8	r2
      	mov.w	r2,@-r0
     	shlr16	r2
      	mov.b	r2,@-r0
     	bt	L_odd_loop
      	add	#3,r5
L_cleanup:
      	cmp/eq	r4,r0
     	bt	L_ready
     	add	#1,r4
L_cleanup_loop:
     	mov.b	@(r0,r5),r2
     	cmp/eq	r4,r0
       	mov.b	r2,@-r0
      	bf	L_cleanup_loop
L_ready:
       	rts	
      	nop	
