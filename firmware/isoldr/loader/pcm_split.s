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
! TODO optimize:
! 1) Use movca.l once per 32 bytes for left and right
! 2) Purge cache for left and right every 32 bytes
! 3) Better pipelining
!
_pcm16_split:
	mov.l .shift5r, r3
	shld r3, r7
	mov.l r11, @-r15
	mov #4, r3
	mov.l r12, @-r15
	mov #0, r0
.pcm16_cache:
	pref @r4
.pcm16_copy:
	dt	r3
	mov.l @r4+, r1
	mov r1, r2
	shll16 r2
	shlr16 r1
	shlr16 r2
	mov.l @r4+, r11
	mov r11, r12
	shlr16 r11
	shll16 r12
	shll16 r11
	or r1, r11
	mov.l r11, @(r0,r5)
	or r12, r2
	mov.l r2, @(r0,r6)
	bf/s .pcm16_copy
	add #4, r0
	dt	r7
	add #-32, r4
	ocbi @r4 !WARNING! Invalidate cache for the source
	add #32, r4
	bf/s .pcm16_cache
	mov #4, r3
	mov.l @r15+, r12
	mov.l @r15+, r11
	rts
	nop

!
! void adpcm_split(uint8 *all, uint8 *left, uint8 *right, uint32 size);
!
! TODO optimize:
! 1) Use 32bit copy to/from memory
! 2) Use movca.l once per 32 bytes for left and right
! 3) Purge cache for left and right every 32 bytes
! 4) Better pipelining
!
_adpcm_split:
	mov.l .shift5r, r1
	shld r1, r7
	mov.l r10, @-r15
	mov #16, r1
.adpcm_cache:
	pref @r4
.adpcm_copy:
	dt	r1
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
	dt	r7
	add #-32, r4
	ocbi @r4 !WARNING! Invalidate cache for the source
	add #32, r4
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
.shift5r:
	.long 0xfffffffb
lock_cdda:
	.long 0
