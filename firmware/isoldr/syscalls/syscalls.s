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
	.globl	start
	.globl	_Exit_to_game
	.globl	_allocate_GD
	.globl	_release_GD
	.globl	_get_GDS
	.globl	_lock_gdsys
	.globl	_flush_cache
	.globl	.gdsys_lock
	.globl	_gdGdcInitSystem
	.globl	_gd_gdrom_syscall
	.globl	_disc_type
	.globl	_gd_vector2
	.globl	_display_cable
	.globl	_disc_id
    .text
start:
.sleep_func:
	nop
	nop
	nop
.sleep_loop:
	sleep
	bra .sleep_loop
	nop

	.long 	0xFCE0FCE0

.rte_func:
	nop
	nop
	rte
	nop

.rts_func:
	nop
	nop
	rts
	nop

.irq_sem:
	.long 	0
.OS_type:
	.byte 	0
.menu_param:
	.byte 	0
_display_cable:
	.byte 	0xC1
.date_set:
	.byte 	1
.timer_count:
	.long 	0x00000023
	
	.org 	0x2C
	
	.byte 	0x16
.gdsys_lock:
	.byte 	0
.flash_lock:
	.word	0
	
	.org 	0x46
	.word	0x000a
	.long 	0x00000001
	.long 	0x00000080
	
	.org 	0x68
.dreamcast_id:
	.long 	0x04802FD0, 0x5524C317
	.byte 	0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00
	.byte 	0x00, 0x89, 0xFC, 0x5B, 0xFF, 0x01, 0x00, 0x00

	.org 	0x80
.irq_callback:
	.long 	0, 0, 0, 0
_disc_type:	
	.word	0
	.word	0xFF80
	.long 	0x8C0010F0
	.long 	0x00008000
	.long 	0x8C008160
	.long 	0
	
	.org 	0xA4
.fnt_addr:
	.long 	0xA0100000
.flash_addr:
	.long 	0xA0200000
.gd_base_reg:
	.long 	0xA05F7000
.cfg_vector:
	.long 	_sysinfo_syscall 
.fnt_vector:
	.long 	_bios_font_syscall 
.fl_vector:
	.long 	_flashrom_syscall 
.gd_vector:
	.long 	_gd_misc_syscall 
_gd_vector2:
	.long 	_gd_gdrom_syscall 
	
		
	.org 	0xCC
_disc_id:
	.long 	0, 0, 0, 0, 0
	
	.org 	0xE0
.system_vector:
	.long 	_sys_do_bioscall 

	.org 	0x100


	

	.org 	0x1000	
_gd_misc_syscall:
	mov		#-1, r0
	cmp/eq	r0, r6
	bt		.gd_misc_ret
	mov 	#8, r0
	cmp/hs	r0, r6
	bt/s	.gd_misc_ret
	mov		#-2, r0
	mov.l	.gd_vector_off, r0
	mov.l	@r0, r0
	jmp		@r0
	nop
	
.gd_misc_ret:
	rts
	add		#1, r0

.align 2	
.gd_vector_off:
	.long	0x8c0000c0
	
_allocate_GD:	!0x1018
	mova    .GD_mutex, r0
	tas.b   @r0
	bt      .allocate_GD_ret
	rts
	mov     #1, r0
	
.allocate_GD_ret:
	rts
	mov     #0, r0

.align 2
_release_GD:
	mova    .GD_mutex, r0
	mov     #0, r1
	rts
	mov.l   r1, @r0
	
	.align 2
.GD_mutex:
	.long 0

_get_GDS:
	nop
	mova	.gd_gds, r0
	rts
	nop

_lock_gdsys:
	mov.l   .gdsys_off, r2
	mov     #0, r5
	cmp/eq  r5, r4
	bt      .lock_gdsys_unlock
	
	mov     #1, r3
	mov.b   r3, @r2
	add     #1, r2
	mov.w   @r2, r1
	extu.w  r1, r1
	cmp/pl  r1
	bt      .lock_gdsys_err
	rts
	mov     #0, r0
	
.lock_gdsys_err:
	mov     #0, r1
	mov     #-1, r0
	add		r0, r2
	rts
	mov.b   r1, @r2
	
.lock_gdsys_unlock:
	rts
	mov.b   r5, @r2

.align 2	
.gdsys_off:	
	.long 0x8C00002D


.align 2
_sys_do_bioscall:
	mov		r4, r0
	
	cmp/eq	#1, r0
	bt		.sys_do_bioscall_reboot
	cmp/eq	#2, r0
	bt		.sys_do_bioscall_chk_disk
	
.sys_do_bioscall_ret:
	rts
	mov		#0, r0
	
.sys_do_bioscall_chk_disk:
	mov.l	.syBtCheckDisc_off, r0
	jmp		@r0
	nop
	
.sys_do_bioscall_reboot:
	mov		r15, r5
	mov.l   .reg_sr_val, r0
	ldc     r0, sr
	mov.l   .reg_gbr_val, r4
	ldc     r4, gbr
	mov.l   .new_stack, r15
	ldc     r4, vbr
	
	sts.l	pr, @-r15
	
	mov.l	.syBtExit_off, r0
	jmp		@r0
	mov.l	@r15+, r4
	
.align 2
!!!!! WARNING use only from ACxxxxxx area
_flush_cache:
	stc.l	sr, @-r15
	stc		sr, r0
	or		#240, r0
	ldc		r0, sr
	mov		#244, r5
	shll16	r5
	mov		r5, r6
	add		#32, r6
	shll8	r5
	shll8	r6
	mov		#-1, r3
	shll8	r3
	shll16	r3
	add		#28, r3
	mov.l	@r3, r0
	tst		#32, r0
	bt/s	.flush_cache_p3
	mov		#2, r2
	
	mov		#1, r2
.flush_cache_p3:
	shll8	r2
	shll2	r2
	shll2	r2
	mov		#0, r1

.flush_cache_p4:
	add		#-32, r2
	mov		r2, r0
	shlr2	r0
	shll2	r0
	tst		r2, r2
	mov.l	r1, @(r0,r6)
	mov.l	r1, @(r0,r5)
	bf		.flush_cache_p4
	
	nop
	nop
	nop
	nop
	nop
	nop
	ldc.l	@r15+, sr
	rts
	nop


	.org 	0x10F0
	
_gd_gdrom_syscall:
	
	mov     #16, r1
	cmp/hs  r1,  r7
	bt 		.gd_gdrom_ret
	shll2   r7
	mova    .gd_func, r0
	mov.l	@(r0, r7), r0
	jmp     @r0
	nop

.gd_gdrom_ret:
	rts
	nop

.align 2
.global _gd_do_cmd
_gd_do_cmd:
	mov     #48, r1
	cmp/hs  r1, r6
	bt      .gd_do_cmd_ret
	shll2	r6
	mova	.gd_cmds, r0
	mov.l	@(r0, r6), r0
	jmp		@r0
	nop
	
.gd_do_cmd_ret:
	rts
	nop	
	
	
.align 2
.gd_func:
	.long _gdGdcReqCmd 
	.long _gdGdcGetCmdStat 
	.long _gdGdcExecServer 
	.long _gdGdcInitSystem 
	.long _gdGdcGetDrvStat 
	.long _gdGdcG1DmaEnd 
	.long _gdGdcReqDmaTrans 
	.long _gdGdcCheckDmaTrans 
	.long _gdGdcReadAbort 
	.long _gdGdcReset 
	.long _gdGdcChangeDataType 
	.long _gdGdcSetPioCallback 
	.long _gdGdcReqPioTrans 
	.long _gdGdcCheckPioTrans 
	.long _gdGdcDummy 
	.long _gdGdcChangeCD

.gd_cmds:
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gd_check_license 
	.long _gdGdcDummy 
	.long _gd_req_spi_cmds 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gd_pioread 
	.long _gd_dmaread 
	.long _gd_gettoc 
	.long _gd_gettoc2 
	.long _gd_play 
	.long _gd_play2 
	.long _gd_pause 
	.long _gd_release 
	.long _gd_init 
	.long _gd_read_abort 
	.long _gd_cd_open_tray 
	.long _gd_seek 
	.long _gd_dmaread_stream 
	.long _gd_nop_cmd 
	.long _gd_req_mode 
	.long _gd_set_mode 
	.long _gd_cd_scan 
	.long _gd_stop 
	.long _gd_getscd 
	.long _gd_gettracks 
	.long _gd_getstat 
	.long _gd_pioread_stream 
	.long _gd_dmaread_stream2 
	.long _gd_pioread_stream2 
	.long _gd_get_version 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy 
	.long _gdGdcDummy
	.long _gdGdcDummy

.gd_gds:
	!gd_cmd 0
	.long 0
	!gd_cmd_stat 4
	.long 0
	!gd_cmd_err 8
	.long 0
	!gd_cmd_err2 12
	.long 0
	!param[4] 16
	.long 0
	.long 0
	.long 0
	.long 0
	!gd_hw_base 32
	.long 0xA05F7000
	!transfered 36
	.long 0
	!ata_status 40
	.long 0
	!drv_stat 44
	.long 0
	!drv_media 48
	.long 0
	!cmd_abort 52
	.long 0
	!requested 56
	.long 0
	!gdchn 60
	.long 1
	!dma_in_progress 64
	.long 0
	!need_reinit 68
	.long 0
	!callback 72
	.long 0
	!callback_param 76
	.long 0
	!pioaddr 80
	.long 0
	!piosize 84
	.long 0
	!cmdp 16-40 (+3)
	!cmdp offset = gd_gds + 0x58 (88)
	.byte 4, 4, 2, 2, 3, 3, 0, 0, 0, 0, 0, 1, 2, 4
	.byte 1, 4, 2, 0, 3, 3, 4, 2, 3, 3, 1, 0, 0, 0
	
	!TOC[0] offset = gd_gds + 0x74 (116)
	.long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long 0,0,0,0,0,0,0,0,0,0,0,0	
	
	!two lba offsets for data tracks used 32 bits up to 2 TB (0x20C 524)
	.long 0,0,0
	! DS_CORE LBA
	.long 0, 0
	! current track and LBA
	.long 0x41000096

.align 2
.syBtCheckDisc_off:
	.long _syBtCheckDisc
.syBtExit_off:
	.long _syBtExit + 0x20000000
.reg_sr_val:
	.long 0x700000F0
.reg_gbr_val:
	.long 0x8C000000
.new_stack:
	.long 0x8D000000

_gd_gettoc:
_gd_cd_open_tray:
_gd_cd_scan:
_gd_set_mode:
_gd_req_mode:
_gd_nop_cmd:
_gd_check_license:
_gd_req_spi_cmds:
	rts
	nop

.align 2
_gd_get_version:
	mov		#28, r1
	mov.l	@r4, r4
	mova	.ver_str, r0	
	
.gd_get_version_loop:
	mov.b	@r0+, r2
	mov.b	r2, @r4
	dt		r1
	bf/s	.gd_get_version_loop
	add		#1, r4
	rts
	nop
	
.ver_str:
	.byte 0x47,0x44,0x43,0x20,0x56,0x65,0x72,0x73,0x69,0x6F,0x6E,0x20,0x31,0x2E
	.byte 0x31,0x30,0x20,0x31,0x39,0x39,0x39,0x2D,0x30,0x33,0x2D,0x33,0x31,0x02

.align 2
_gd_pioread:
	mov		#0, r6
	mov		#36, r0
	mov.l	r6, @(r0, r5)
	
	mov.l	@(4, r4), r0
	shll8	r0
	shll2	r0
	shll	r0
	mov.l	r0, @(56, r5)
	mov.l	@(8, r4), r4
	
	mov.l	.pio_read_internal_adress, r0
	jmp		@r0
	nop

.align 2
_gd_pioread_stream:
	mov		#0, r0
	mov.l	r0, @(8, r4)
_gd_pioread_stream2:
	mov		#0, r1
	mov		#4, r6
	mov		#80, r0
	
	mov.l	r1, @(r0, r5)
	add		#4, r0
	mov.l	r1, @(r0, r5)
	
	mov.l	r6, @(4, r5)
	mov.l	@(4, r4), r0
	shll8	r0
	shll2	r0
	shll	r0
	mov.l	r0, @(56, r5)
	
	mov.l	.gd_execute_cmd_pio_adress, r0
	jmp		@r0
	mov		r5, r4

.align 2
.gd_execute_cmd_pio_adress:
	.long	_pio_stream_internal

.pio_read_internal_adress:
	.long	_pio_read_internal

.align 2
_gd_dmaread_stream:
	mov		#0, r0
	mov.l	r0, @(8, r4)
	mov.l	.gd_dmaread_stream2_adress, r0
	jmp		@r0
	nop
	
.align 2
.gd_dmaread_stream2_adress:
	.long	_gd_dmaread_stream2	


.align 2
_gdGdcReqCmd:
	sts.l   pr,  @-r15
	bsr		_allocate_GD
	mov		#0, r6
	cmp/eq	#0, r0
	bf		.gdGdcReqCmd_ret
	
	bsr		_get_GDS
	nop
	mov		r0, r6		!r6 = GDS
	
	mov.l	@(4,r6), r0
	cmp/eq	#0, r0
	bt		.gdGdcReqCmd_process
	bra		.gdGdcReqCmd_rel_ret
	mov		#0, r6
	
.gdGdcReqCmd_process:
	mov.l	r0, @(16,r6)
	mov.l	r0, @(20,r6)
	mov.l	r0, @(24,r6)
	mov.l	r0, @(28,r6)
	mov.l	r4, @r6
	mov		#16, r0
	cmp/hi	r4, r0
	bt		.gdGdcReqCmd_cmdnoparam
	mov		#40, r0
	cmp/hi	r0, r4
	bt		.gdGdcReqCmd_cmdnoparam
	
	add		#72, r4
	add		r6, r4
	mov.b	@r4, r0
	
	cmp/eq	#0, r0
	bt		.gdGdcReqCmd_cmdnoparam
	
	mov		r6, r4
	add		#16, r4
	
.gdGdcReqCmd_cp:
	
	mov.l	@r5+, r1
	mov.l	r1, @r4
	dt		r0
	bf/s	.gdGdcReqCmd_cp
	add		#4, r4
	
.gdGdcReqCmd_cmdnoparam:	
	mov		#2, r0
	mov.l	r0, @(4,r6)
	
	mov.l	@(60,r6), r0
	add		#1, r0
	cmp/eq	#0, r0
	bf		.gdGdcReqCmd_gdchn
	add		#1, r0
.gdGdcReqCmd_gdchn:	
	mov.l	r0, @(60,r6)
	mov		r0, r6
	
.gdGdcReqCmd_rel_ret:	
	bsr		_release_GD
	nop
	
.gdGdcReqCmd_ret:
	lds.l   @r15+, pr
	rts
	mov		r6, r0




.align 2
_gdGdcInitSystem:
	mov.l    .mutexGD, r0
	mov     #1, r2
	mov.l   r2, @r0
	
	sts.l   pr, @-r15
	sts.l   mach, @-r15
	sts.l   macl, @-r15
	mov.l   r14, @-r15
	mov.l   r13, @-r15
	mov.l   r12, @-r15
	mov.l   r11, @-r15
	mov.l   r10, @-r15
	mov.l   r9, @-r15
	mov.l   r8, @-r15
	mova    .svar_stack, r0
	mov.l   r15, @r0
	mova    .svars_end, r0
	mov     r0, r2
	mova    .svar_adres, r0
	bra		_Init_GDS
	mov.l   r2, @r0
	
!	mov.l	.initgds_func, r0
!	jmp		@r0
!	nop

!.align 2
!.initgds_func:
!	.long	_Init_GDS


.align 2
_Exit_to_game:
	mova    .svar_stack, r0
	mov.l   @r0, r2
	sub     r15, r2
	shlr2   r2
	mov     r2, r0
	cmp/eq  #0, r0
	mova    .svar_adres, r0
	mov.l   @r0, r0
	mov     r2, r1
	sts.l   pr, @-r0
	sts.l   mach, @-r0
	sts.l   macl, @-r0
	mov.l   r14, @-r0
	mov.l   r13, @-r0
	mov.l   r12, @-r0
	mov.l   r11, @-r0
	mov.l   r10, @-r0
	mov.l   r9, @-r0
	mov.l   r8, @-r0
	bt      .Exit_to_game_skip
	
.Exit_to_game_loop:
	mov.l   @r15+, r3
	dt      r2
	mov.l   r3, @-r0
	bf      .Exit_to_game_loop
	
.Exit_to_game_skip:
	mov.l   r1, @-r0
	mov     r0, r2
	mova    .svar_adres, r0
	mov.l   r2, @r0
	mov.l   @r15+, r8
	mov.l   @r15+, r9
	mov.l   @r15+, r10
	mov.l   @r15+, r11
	mov.l   @r15+, r12
	mov.l   @r15+, r13
	mov.l   @r15+, r14
	lds.l   @r15+, macl
	lds.l   @r15+, mach
	lds.l   @r15+, pr
	mov.l	.mutexGD, r0
	mov     #0, r2
	mov.l   r2, @r0
	rts
	nop


	.align 2
_gdGdcExecServer:
	mov.l    .mutexGD, r0
	tas.b   @r0
	bt      .gdGdcExecServer_process
	rts
	nop
	
.gdGdcExecServer_process:
	sts.l	pr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15
	mov.l	r14, @-r15
	mov.l	r13, @-r15
	mov.l	r12, @-r15
	mov.l	r11, @-r15
	mov.l	r10, @-r15
	mov.l	r9, @-r15
	mov.l	r8, @-r15
	mova	.svar_stack, r0
	mov.l	r15, @r0
	mova	.svar_adres, r0
	mov.l	@r0, r0
	mov.l	@r0+, r2
	mov		r0, r3
	mov		r2, r0
	cmp/eq	#0, r0
	bt/s	.gdGdcExecServer_cpend
	mov		r3, r0
	
.gdGdcExecServer_cploop:
	mov.l	@r0+, r3
	dt		r2
	mov.l	r3, @-r15
	bf		.gdGdcExecServer_cploop
.gdGdcExecServer_cpend:
	mov.l	@r0+, r8
	mov.l	@r0+, r9
	mov.l	@r0+, r10
	mov.l	@r0+, r11
	mov.l	@r0+, r12
	mov.l	@r0+, r13
	mov.l	@r0+, r14
	lds.l	@r0+, macl
	lds.l	@r0+, mach
	lds.l	@r0+, pr
	mov		r0, r2
	mova	.svar_adres, r0
	mov.l	r2, @r0
	rts
	nop

.align 2	
.mutexGD:
	.long	.GD_mutex

.svar_stack:
	.long	0
.svar_adres:
	.long	0

.svars:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
.svars_end:
	.long	0

_Init_GDS:
	bsr		_get_GDS
	nop
	mov		r0, r6
	add		#88, r0
	
	mov		#0, r1
	mov		#2, r2
	mov		#13, r3
	
!	mov.l	@(48,r6), r4
	
.Init_GDS_loop:	
	mov.l	r1, @-r0
	dt		r3
	bf		.Init_GDS_loop
	dt		r2
	add		#-4, r0
	bf/s	.Init_GDS_loop
	mov		#8, r3
	
!	mov.l	r4, @(48,r6)
	
	mov		#1, r0
	mov.l	r0, @(44,r6)
	mov.l	r0, @(60,r6)
	
	mov.l	.gdcmd_func, r8
	mov.l	.thd_pass, r9
	mov		#3, r10
	mov		#1, r12
	mov		#0, r13
	!!!bra		_Mainloop
	mov		r6, r14
	
_Mainloop:
	mov.l   @(4,r14), r0
	cmp/eq  #2, r0
	bf      .Mainloop_thdpass
	mov.l   r12, @(4,r14)
	mov.l   r13, @(8,r14)
	mov.l   r13, @(12,r14)
	mov.l   r13, @(36,r14)
	
	mov		#68, r0
	mov.l	@(r0,r14), r0
	cmp/eq	#1, r0
	bf		.Mainloop_exec
	mov.l	@r14, r0
	cmp/eq	#24, r0
	bt		.Mainloop_exec
	
	mov		#6, r0
	bra		.Mainloop_skip
	mov.l	r0, @(8,r14)
	
.Mainloop_exec:
	mov.l   @r14, r6
	mov     r14, r5
	mov     r14, r4
	jsr     @r8
	add     #16, r4
	
.Mainloop_skip:
	mov.l   r10, @(4,r14)
	mov.l   r13, @(52,r14)
	
	mov.l	@(8,r14), r0
	cmp/eq	#6, r0
	bf		.Mainloop_thdpass
	mov		#68, r0
	mov.l	r12, @(r0,r14)
	
.Mainloop_thdpass:
	jsr		@r9
	nop
	bra		_Mainloop
	nop

.align 2
.thd_pass:
	.long _Exit_to_game
.gdcmd_func:
	.long _gd_do_cmd


.align 2
_gdGdcChangeDataType:
	mov		#8, r1
	shll8	r1
	mov.l	@r4, r0
	cmp/eq	#0, r0
	bt		.gdGdcChangeDataType_set
	cmp/eq	#1, r0
	bt		.gdGdcChangeDataType_get
	rts
	mov		#-1, r0
	
.gdGdcChangeDataType_set:	
	mov.l	@(12, r4), r0
	cmp/eq	r0, r1
	bt		.gdGdcChangeDataType_ret
	
	rts
	mov		#-1, r0
	
.gdGdcChangeDataType_get:
	mov.l	r1, @(12, r4)
	
	shlr	r1
	mov.l	r1, @(8, r4)
	
	shll2	r1
	shll	r1
	mov.l	r1, @(4, r4)
	
	
.gdGdcChangeDataType_ret:
	rts
	mov		#0, r0
	

.align 2
_gdGdcDummy:
	mov     #5, r3
	mov.l   r3, @(8,r5)
	rts
	nop

.align 2
_gdGdcG1DmaEnd:
	mov.l	.extint_reg, r1
	mov		#64, r2
	shll8	r2
	mov.l	r2, @r1
	tst		r4, r4
	bt		.gdGdcG1DmaEnd_ret
	mov		r4, r0
	jmp		@r0
	mov		r5, r4
	
.gdGdcG1DmaEnd_ret:
	rts
	nop

.align 2
.extint_reg:
	.long	0xA05F6900
