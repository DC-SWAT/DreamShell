!   This file is part of DreamShell ISO Loader
!   Copyright (C) 2014-2024 SWAT <http://www.dc-swat.ru>
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

!
! void pcm16_split(uint32 *data, uint32 *left, uint32 *right, uint32 size);
!
.align 2
_pcm16_split:
	mov #-5, r3
	shld r3, r7
	mov.l r8, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	mov r4, r8
	add #32, r8
	mov #31, r3
	mov #0, r0
.pcm16_pref:
	pref @r8
.pcm16_load:
	tst r3, r0
	mov.l @r4+, r1
	mov.l @r4+, r2
	swap.w r1, r11
	mov r2, r12
	xtrct r11, r12
	swap.w r2, r11
	bt/s .pcm16_store_alloc
	xtrct r1, r11
.pcm16_store:
	mov.l r11, @(r0,r5)
	mov.l r12, @(r0,r6)
.pcm16_loops:
	tst r3, r4
	bf/s .pcm16_load
	add #4, r0
	dt r7
	bf/s .pcm16_pref
	add #32, r8
.pcm16_exit:
	! ocbp @r5
	mov.l @r15+, r12
	! ocbp @r6
	mov.l @r15+, r11
	rts
	mov.l @r15+, r8
.pcm16_store_alloc:
	! ocbp @r5
	add r0, r5
	! ocbp @r6
	add r0, r6
	mov r11, r0
	movca.l r0, @r5
	mov r12, r0
	movca.l r0, @r6
	bra .pcm16_loops
	mov #0, r0

#ifdef HAVE_CDDA_ADPCM
!
! void adpcm_split(uint32 *data, uint32 *left, uint32 *right, uint32 size);
!
.align 2
_adpcm_split:
	mov #-5, r1
	shld r1, r7
	mov.l r8, @-r15
	mov.l r9, @-r15
	mov.l r10, @-r15
	mov.l r11, @-r15
	mov #4, r1
	mov #15, r11
.adpcm_pref:
	add #32, r4
	pref @r4
	add #-32, r4
.adpcm_copy:
	dt r1
	!!! src = *data++;
	mov.l @r4+, r0

	!!! dst_r = src & 0xf
	mov r0, r3
	and r11, r3

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! dst_l = src & 0xf
	mov r0, r2
	and r11, r2

	!!! src >>= 4
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 4,
	mov #4, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 8,
	shll8 r9
	shll8 r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 12,
	mov #12, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src = *data++;
	mov.l @r4+, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 16,
	shll16 r9
	shll16 r8

	!!! src >>= 4
	shld r10, r0

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 20,
	mov #20, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 24,
	mov #24, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 28,
	mov #28, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! *chan++ = dst;
	mov.l r2, @r5
	mov.l r3, @r6
.adpcm_loops:
	add #4, r5
	bf/s .adpcm_copy
	add #4, r6
	dt r7
	bf/s .adpcm_pref
	mov #4, r1
.adpcm_exit:
	mov.l @r15+, r11
	mov.l @r15+, r10
	mov.l @r15+, r9
	rts
	mov.l @r15+, r8
#endif

.align 2
_lock_cdda:
	mova lock_cdda, r0
	tas.b @r0
	bt lock_cdda_success
	rts
	mov #1, r0
lock_cdda_success:
	rts
	mov #0, r0

.align 2
_unlock_cdda:
	mova lock_cdda, r0
	mov #0, r2
	rts
	mov.l r2, @r0

.align 4
lock_cdda:
	.long 0
