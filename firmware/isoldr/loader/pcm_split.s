!   This file is part of DreamShell ISO Loader
!   Copyright (C) 2014-2023 SWAT <http://www.dc-swat.ru>
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
.globl _pcm16_split
.globl _adpcm_split

.globl _lock_cdda
.globl _unlock_cdda
.align 2

!
! void pcm16_split(int16 *all, int16 *left, int16 *right, uint32 size);
!
_pcm16_split:
	mov #-5, r3
	shld r3, r7
	mov.l r8, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	sts.l fpul, @-r15
	sts.l fpscr, @-r15
	mov #0, r0
	lds r0, fpscr
	fmov.s fr0, @-r15
	fmov.s fr1, @-r15
	fmov.s fr2, @-r15
	fmov.s fr3, @-r15
	fmov.s fr4, @-r15
	fmov.s fr5, @-r15
	mov	#0x10, r0
	shll16 r0
	lds	r0, fpscr
	mov #2, r8
	mov #2, r3
	mov #0, r0
.pcm16_cache:
	add #32, r4
	pref @r4
	add #-32, r4
.pcm16_load:
	dt r8
	fmov.d @r4+, dr0
	flds fr0, fpul
	sts fpul, r1
	flds fr1, fpul
	mov r1, r2
	sts fpul, r11
	shll16 r2
	mov r11, r12
	shlr16 r1
	shlr16 r2
	shlr16 r11
	shll16 r12
	shll16 r11
	or r2, r12
	bt/s .pcm16_save
	or r1, r11
	lds r12, fpul
	fsts fpul, fr4
	lds r11, fpul
	bra .pcm16_load
	fsts fpul, fr2
.pcm16_save:
	mov #2, r8
	lds r12, fpul
	fsts fpul, fr5
	lds r11, fpul
	fsts fpul, fr3
	fmov.d dr2, @(r0,r5)
	fmov.d dr4, @(r0,r6)
.pcm16_loops:
	dt r3
	bf/s .pcm16_load
	add #8, r0
	dt r7
	bf/s .pcm16_cache
	mov #2, r3
.pcm16_exit:
	mov #0, r0
	lds r0, fpscr
	fmov.s @r15+, fr5
	fmov.s @r15+, fr4
	fmov.s @r15+, fr3
	fmov.s @r15+, fr2
	fmov.s @r15+, fr1
	fmov.s @r15+, fr0
	lds.l @r15+, fpscr
	lds.l @r15+, fpul
	mov.l @r15+, r12
	mov.l @r15+, r11
	mov.l @r15+, r8
	rts
	nop

!
! void adpcm_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);
!
_adpcm_split:
	mov #-5, r1
	shld r1, r7
	mov.l r10, @-r15
	mov #16, r1
.adpcm_cache:
	add #32, r4
	pref @r4
	add #-32, r4
.adpcm_copy:
	dt r1
	mov.w @r4+, r10
	mov r10, r0
	and #0xf0, r0
	mov r0, r2
	shlr2 r2
	mov r10, r0
	shlr2 r2
	and #0x0f, r0
	mov r0, r3
	shlr8 r10
	mov r10, r0
	and #0xf0, r0
	or r0, r2
	mov.b r2, @r5
	add #1, r5
	mov r10, r0
	and #0x0f, r0
	shll2 r0
	shll2 r0
	or r0, r3
	mov.b r3, @r6
	bf/s .adpcm_copy
	add #1, r6
	dt r7
	bf/s .adpcm_cache
	mov #16, r1
	mov.l @r15+, r10
	rts
	nop

_lock_cdda:
	mova    lock_cdda, r0
	tas.b   @r0
	bt      lock_cdda_success
	rts
	mov     #1, r0
lock_cdda_success:
	rts
	mov     #0, r0
	
_unlock_cdda:
	mova    lock_cdda, r0
	mov     #0, r2
	rts
	mov.l   r2, @r0

.align 4
lock_cdda:
	.long 0
