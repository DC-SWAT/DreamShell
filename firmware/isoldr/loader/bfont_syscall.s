!   This file is part of DreamShell ISO Loader
!   Copyright (C)2014-2016 SWAT
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
	.globl _bfont_syscall_enable
	.globl _bfont_syscall_disable
	.globl _bfont_syscall_save
	.globl _bfont_saved_vector
	.globl _bfont_saved_addr
	.globl _get_font_address
.align 2

_get_font_address:
	mov.l bfont_entry_k, r0
	mov.l @r0, r0
	jmp @r0
	mov #0, r1
	rts
	nop

_bfont_syscall_save:
	mov.l bfont_saved_k, r0
	mov.l @r0, r0
	tst r0,r0
	bf already_saved
!	mov.l bfont_entry_k, r0
!	mov.l @r0, r0
!	jsr @r0
!	mov #0, r1
!	mov.l _bfont_saved_addr, r1
!	mov.l r0, @r1
	mov.l bfont_entry_k, r0
	mov.l @r0,r0
	mov.l bfont_saved_k, r1
	mov.l r0, @r1
already_saved:
	rts
	nop
	
_bfont_syscall_disable:
	mov.l bfont_saved_k, r0
	mov.l @r0, r0
	mov.l bfont_entry_k, r1
	mov.l r0, @r1
	rts
	nop

_bfont_syscall_enable:
	mov.l bfont_entry_k, r0
	mov.l bfont_redir_k, r1
	mov.l r1, @r0
	rts
	nop

bfont_redir:
	mov	#0,r0
	cmp/eq	r0,r1		! Are we querying for a font or doing the weird mutex thing?
	bt	.get_font_addr
	rts			       ! Yes, just return success
	nop
.get_font_addr:
	mov.l	_bfont_saved_addr,r0
	rts			       ! Otherwise return the font location
	nop

.align 4
bfont_entry_k:
	.long 0xac0000b4
bfont_saved_k:
	.long _bfont_saved_vector
_bfont_saved_vector:
	.long 0
_bfont_saved_addr:
	.long	0xa0100020
bfont_redir_k:
	.long bfont_redir
