!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!   This file is part of DreamShell ISO Loader								!
!   Copyright (C)2019 megavolt85											!
!																			!
!   This program is free software: you can redistribute it and/or modify	!
!   it under the terms of the GNU General Public License version 3 as		!
!   published by the Free Software Foundation.								!
!																			!
!   This program is distributed in the hope that it will be useful,			!
!   but WITHOUT ANY WARRANTY; without even the implied warranty of			!
!   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			!
!   GNU General Public License for more details.							!
!																			!
!   You should have received a copy of the GNU General Public License		!
!   along with this program. If not, see <http://www.gnu.org/licenses/>.	!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	.globl	_sysinfo_syscall
	.globl	_bios_font_syscall
	.globl	_flashrom_syscall
	.globl	_flashrom_lock
	.globl	_flashrom_unlock

.align 2
	.globl	_irq_disable
_irq_disable:
	stc		sr, r0
	mov		r0, r1
	or		#240, r0
	ldc		r0, sr
	rts
	mov		r1, r0
	
	.globl	_irq_restore
_irq_restore:
	ldc	r4,sr
	rts
	nop
	

.align 2
_bios_font_syscall:
	mov		#3, r2
	cmp/hs	r2, r1
	bt		.bios_font_syscall_ret
	tst		r1, r1
	bt		.bios_font_syscall_retadr
	
	mova	.romfont_adress, r0
	shll2	r1
	mov.l	@(r0, r1), r0
	jmp		@r0
	nop
	
.bios_font_syscall_retadr:
	mov.l	.romfont_adress, r0
.bios_font_syscall_ret:	
	rts
	nop

.align 2
.romfont_adress:
	.long	0xA0100020
	.long	_flashrom_lock 
	.long	_flashrom_unlock 

.align 2
_sysinfo_syscall:
	mov		#4, r0
	cmp/hs	r0, r7
	bt/s	.sysinfo_syscall_ret
	mov		#-1, r1
	mov		r7, r0
	cmp/eq	#0, r0
	bt/s	.sysinfo_syscall_case0
	mov		#0, r1
	cmp/eq	#1, r0
	bt		.sysinfo_syscall_case1
	cmp/eq	#3, r0
	bf		.sysinfo_syscall_case3
	
	mov		#10, r0
	cmp/hs	r0, r4
	bt		.sysinfo_syscall_ret
	
	mov		#44, r6
	shll2	r6
	shll2	r6
	mulu	r6, r4
	mov.l	.icon_offset, r4
	sts		macl, r2
	add		r2, r4
	mova	.flashrom_off, r0
	mov.l	@(4, r0), r0
	jmp		@r0
	nop
	
.sysinfo_syscall_case3:
	mov.l	.dc_id, r1
.sysinfo_syscall_ret:
	rts
	mov		r1, r0
	
.sysinfo_syscall_case1:
	mov		#140, r6
	shll16	r6
	shll8	r6
	trapa	#224
	sett
	
.sysinfo_syscall_case0:
	sts.l   pr, @-r15
	mova	.flashrom_off, r0
	mov.l	@(4, r0), r0
	mov		#8, r6
	mov.l	.dc_id, r5
	mov.l	.dc_id_offset, r4
	jsr		@r0
	add		#86, r4
	
	mova	.flashrom_off, r0
	mov.l	@(4, r0), r0
	mov.l	.dc_id, r5
	add		#8, r5
	mov.l	.dc_id_offset, r4
	jsr		@r0
	mov		#5, r6
	
	mov.l	.dc_id, r4
	add		#23, r4
	mov		#0, r5
	mov		#10, r0
	
.sysinfo_syscall_cp_loop:
	mov.b	r5, @(r0, r4)
	dt		r0
	bt		.sysinfo_syscall_cp_loop
	add		#-31, r4
	ocbwb	@r4
	lds.l   @r15+, pr
	rts
	mov		#0, r0
	
.align 2
.dc_id:
	.long 0x8C000068
.icon_offset:
	.long 0x0001a480
.dc_id_offset:
	.long 0x0001a000

.align 2
_flashrom_syscall:
	mov		#4, r0
	cmp/hs	r0, r7
	bt		.flashrom_syscall_ret
	shll2	r7
	mova	.flashrom_off, r0
	mov.l	@(r0, r7), r0
	jmp		@r0
	nop
	
.flashrom_syscall_ret:	
	rts
	mov		#-1, r0

.align 2
.flashrom_off:
	.long _flashrom_info 
	.long _flashrom_read 
	.long _flashrom_write 
	.long _flashrom_delete 





_flashrom_delete:
	sts.l   pr, @-r15
	add     #-8, r15
	bsr     _flashrom_lock
	mov.l   r4, @r15
	
	tst     r0, r0
	bt      .flashrom_delete_p2
	
	add     #8, r15
	lds.l   @r15+, pr
	rts
	mov     #-1, r0
	
	
.flashrom_delete_p2:
	mov.w   .valff0f, r3
	stc     sr, r0
	shlr2   r0
	shlr2   r0
	and     #15, r0
	mov.l   r0, @(4,r15)
	stc     sr, r0
	and     r3, r0
	or      #240, r0
	ldc     r0, sr
	mov.l   .flash_base, r0
	
	mov.l   @r15, r4
	mov     r0, r5
	mov.w   .val5555, r6
	add     r5, r4
	mov.w   .val2aaa, r3
	add     r5, r6
	mov     #170, r2
	extu.b	r2, r2
	add     r3, r5
	mov.b   r2, @r6
	mov     #85, r7
	mov.b   r7, @r5
	mov     #128, r1
	extu.b	r1, r1
	mov.b   r1, @r6
	mov.b   r2, @r6
	mov.b   r7, @r5
	mov     #48, r1
	mov     #-1, r5
	extu.b	r1, r1
	bsr     _flashrom_wr_sync
	mov.b   r1, @r4
	
	stc     sr, r3
	mov.w   .valff0f, r2
	mov.l   r0, @r15
	mov.l   @(4,r15), r0
	and     r2, r3
	and     #15, r0
	shll2   r0
	shll2   r0
	or      r3, r0
	ldc     r0, sr
	bsr     _flashrom_unlock
	nop
	
	mov.l   @r15, r0
	add     #8, r15
	lds.l   @r15+, pr
	rts
	nop


_flashrom_write:
	
	mov.l   r14, @-r15
	mov.l   r13, @-r15
	mov.l   r12, @-r15
	mov.l   r11, @-r15
	mov.l   r10, @-r15
	sts.l   pr, @-r15
	add     #-12, r15
	mov.l   r4, @r15
	mov.l   r5, @(8,r15)
	bsr     _flashrom_lock
	mov     r6, r10
	
	tst     r0, r0
	bt      .flashrom_write_go
	
	bra     .flashrom_write_ret
	mov     #-1, r0
	
.flashrom_write_go:
	stc     sr, r0
	mov.w   .valff0f, r3
	shlr2   r0
	shlr2   r0
	and     #15, r0
	mov.l   r0, @(4,r15)
	stc     sr, r0
	and     r3, r0
	or      #240, r0
	ldc     r0, sr
	mov.l   .flash_base, r0
	
	mov     r0, r11
	mov.l   @(8,r15), r13
	cmp/pl  r10
	mov.l   @r15, r12
	add     r11, r12
	bf/s    .flashrom_write_end
	mov     #0, r14
	
	mov     r0, r11
	mov.l   @(8,r15), r13
	cmp/pl  r10
	mov.l   @r15, r12
	add     r11, r12
	bf/s    .flashrom_write_end
	mov     #0, r14
	
.flashrom_write_loop:
	mov.b   @r13, r5
	mov.b   @r12, r4
	xor     r5, r4
	extu.b  r5, r5
	extu.b  r4, r4
	tst     r4, r5
	bf      .flashrom_write_end
	
	mov     r11, r6
	mov     r12, r5
	add     #1, r12
	mov     r13, r4
	bsr     _flashrom_write_byte_int
	add     #1, r13
	
	tst     r0, r0
	bf      .flashrom_write_end
	
	add     #1, r14
	cmp/ge  r10, r14
	bf      .flashrom_write_loop
	
.flashrom_write_end:
	mov.l   @(4,r15), r0
	stc     sr, r2
	mov.w   .valff0f, r3
	and     #15, r0
	shll2   r0
	shll2   r0
	and     r3, r2
	or      r2, r0
	ldc     r0, sr
	bsr     _flashrom_unlock
	nop
	
	mov     r14, r0
	nop
	
.flashrom_write_ret:
	add     #12, r15
	lds.l   @r15+, pr
	mov.l   @r15+, r10
	mov.l   @r15+, r11
	mov.l   @r15+, r12
	mov.l   @r15+, r13
	rts
	mov.l   @r15+, r14
	
	

_flashrom_write_byte_int:
	mov     #170, r3
	extu.b	r3, r3
	add     #-8, r15
	mov.l   r4, @r15
	mov.l   r5, @(4,r15)
	mov.w   .val5555, r4
	add     r6, r4
	mov.w   .val2aaa, r5
	add     r6, r5
	mov.b   r3, @r4
	mov     #85, r2
	mov.b   r2, @r5
	add     #-10, r3
	mov.b   r3, @r4
	mov.l   @(4,r15), r2
	mov.l   @r15, r3
	mov.b   @r3, r1
	mov.b   r1, @r2
	mov.l   @r15, r5
	mov.l   @(4,r15), r4
	mov.b   @r5, r5
	bra     _flashrom_wr_sync
	add     #8, r15
	

.val5555:
	.word	0x5555
.val2aaa:
	.word	0x2AAA
.valff0f:
	.word	0xFF0F

_flashrom_wr_sync:
	mov.l   r13, @-r15
	mov     #0, r13
	mov.l   r12, @-r15
	mov.l   r11, @-r15
	add     #-4, r15
	mov.b   @r4, r1
	mov     #32, r12
	mov     r12, r7
	add     #96, r7
	mov     #64, r6
	extu.b  r5, r11
	mov.l   r11, @r15
	and     r7, r11
	
.flashrom_wr_sync_loop:
	mov.b   @r4, r5
	extu.b  r5, r2
	and     r7, r2
	cmp/eq  r11, r2
	bt      .flashrom_wr_sync_end
	
	extu.b  r5, r3
	and     r6, r3
	extu.b  r1, r1
	and     r6, r1
	cmp/eq  r1, r3
	bt      .flashrom_wr_sync_end
	
	extu.b  r5, r2
	tst     r12, r2
	bt      .flashrom_wr_sync_continue
	
	bra     .flashrom_wr_sync_end
	mov     #1, r13
	
	
.flashrom_wr_sync_continue:
	bra     .flashrom_wr_sync_loop
	mov     r5, r1
	
.flashrom_wr_sync_end:
	mov.b   @r4, r5
	extu.b  r5, r5
	mov.l   @r15, r3
	cmp/eq  r3, r5
	bf      .flashrom_wr_sync_end2
	
	bra     .flashrom_wr_sync_ret
	mov     #0, r0
	
.flashrom_wr_sync_end2:
	tst     r13, r13
	bt      .flashrom_wr_sync_ret_err
	
	mov     #240, r2
	extu.b  r2, r2
	mov.b   r2, @r4
	
.flashrom_wr_sync_ret_err:
	mov     #-1, r0
	
.flashrom_wr_sync_ret:
	add     #4, r15
	mov.l   @r15+, r11
	mov.l   @r15+, r12
	rts
	mov.l   @r15+, r13
	
	

.align 2
_flashrom_unlock:
	mova    .frlock_mutex, r0
	mov.l   @r0, r2
	add     r0, r2
	mov     #0, r0
	rts
	mov.b   r0, @(2,r2)

.align 2
_flashrom_lock:
	mova    .frlock_mutex, r0
	mov.l   @r0, r2
	add     r0, r2
	mov     #1, r0
	mov.b   r0, @(2,r2)
	mov.b   @r2, r1
	extu.b  r1, r1
	tst     r1, r1
	bt/s    .flashrom_lock_ret
	mov     #0, r0

	mov.b   r0, @(2,r2)
	mov     #-1, r0

.flashrom_lock_ret:
	rts
	nop
	
.align 2
.frlock_mutex:
	.long	.gdsys_lock-.frlock_mutex
.flash_base:
	.long	0xA0200000
