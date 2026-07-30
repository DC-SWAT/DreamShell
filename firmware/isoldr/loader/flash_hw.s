!   This file is part of DreamShell ISO Loader
!   Copyright (C)2019-2024 megavolt85
!   Copyright (C)2026 SWAT
!
!   This program is free software: you can redistribute it and/or modify
!   it under the terms of the GNU General Public License version 3 as
!   published by the Free Software Foundation.
!
!   This program is distributed in the hope that it will be useful,
!   but WITHOUT ANY WARRANTY; without even the implied warranty of
!   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!   GNU General Public License for more details.
!
!   You should have received a copy of the GNU General Public License
!   along with this program. If not, see <http://www.gnu.org/licenses/>.
!
	.section .text
	.globl	_flashrom_write_hw
	.globl	_flashrom_delete_hw

.align 2
_flashrom_delete_hw:
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

_flashrom_write_hw:
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
	mov.l   .unlock_gdsys_k, r0
	jmp     @r0
	nop

.align 2
_flashrom_lock:
	mov.l   .lock_gdsys_k, r0
	jmp     @r0
	nop

.align 2
.flash_base:
	.long	0xA0200000
.lock_gdsys_k:
	.long	_lock_gdsys
.unlock_gdsys_k:
	.long	_unlock_gdsys
