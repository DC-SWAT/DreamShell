!   This file is part of DreamShell ISO Loader
!   Copyright (C)2010-2023 SWAT
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
	.globl _menu_syscall_enable
	.globl _menu_syscall_disable
	.globl _menu_syscall_save
	.globl _menu_saved_vector
.align 2
	
_menu_syscall_save:
	mov.l menu_saved_k, r0
	mov.l @r0, r0
	tst r0,r0
	bf already_saved
	mov.l menu_entry_k, r0
	mov.l @r0,r0
	mov.l menu_saved_k, r1
	mov.l r0, @r1
already_saved:
	rts
	nop
	
_menu_syscall_disable:
	mov.l menu_saved_k, r0
	mov.l @r0, r0
	mov.l menu_entry_k, r1
	mov.l r0, @r1
	rts
	nop

_menu_syscall_enable:
	mov.l menu_entry_k, r0
	mov.l menu_redir_k, r1
	mov.l r1, @r0
	rts
	nop
	
.align 4
menu_entry_k:
	.long 0xac0000e0
menu_saved_k:
	.long _menu_saved_vector
_menu_saved_vector:
	.long 0
menu_redir_k:
	.long menu_redir

menu_redir:
	mov		r4, r0
	cmp/eq	#1, r0
	bt		menu_reboot
	cmp/eq	#2, r0
	bt		menu_chk_disk
	rts
	mov		#0, r0

menu_chk_disk:
	mov.l	menu_check_disc, r0
	jmp		@r0
	nop

menu_reboot:
	mov		r15, r5
	mov.l   reg_sr_val, r0
	ldc     r0, sr
	mov.l   reg_gbr_val, r4
	ldc     r4, gbr
	mov.l   new_stack, r15
	ldc     r4, vbr
	sts.l	pr, @-r15
	mov.l	menu_exit, r0
	jmp		@r0
	mov.l	@r15+, r4

.align 4
menu_check_disc:
	.long _menu_check_disc
menu_exit:
	.long _menu_exit
reg_sr_val:
	.long 0x700000f0
reg_gbr_val:
	.long 0x8c000000
new_stack:
	.long 0x8d000000
