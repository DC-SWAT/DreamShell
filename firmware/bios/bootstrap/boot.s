! Basic ROM Bootstrap
! (c)2002 Dan Potter
! (c)2009-2014 SWAT
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions
! are met:
! 1. Redistributions of source code must retain the above copyright
!    notice, this list of conditions and the following disclaimer.
! 2. Redistributions in binary form must reproduce the above copyright
!    notice, this list of conditions and the following disclaimer in the
!    documentation and/or other materials provided with the distribution.
! 3. Neither the name of Cryptic Allusion nor the names of its contributors
!    may be used to endorse or promote products derived from this software
!    without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
! ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
! DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
! OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
! HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
! LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
! OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
! SUCH DAMAGE.
!                     


! Sets up the basic regs, copies the appended program into
!  RAM at 8c010000, and jumps to it (as usual).

! We also, incidentally, support a couple of syscalls through
!  the standard interface.

	.text

!!!!!!!!! Initial Setup !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! Should reside at 0xa0000000
	.org	0
start:
	! Clear MMUCR (in case a program was running and we reset)
	! and setup enough of CCR to be able to cache ROM.
	mov	#-1,r3		! r3 = 0xffffffff
	shll16	r3		! r3 = 0xffff0000
	shll8	r3		! r3 = 0xff000000
	mov	#0,r0		! r0 = 0
	mov.l	r0,@(16,r3)	! 0 -> MMUCR (0xff000010)
	mov	#9,r1		! Number nine, Number nine...
	shll8	r1		! r1 = 0x900
	add	#41,r1		! r1 = 0x929
	mov.l	r1,@(28,r3)	! 0x929 -> CCR (0xff00001c)
	shar	r3		! r3 = 0xff800000

	! These values for BCR1 and BCR2 don't make sense, but I'd
	! rather be superstitious and replicate them than to have a DC
	! with fried RAM or peripherals =)
	mov	#1,r0
	mov.w	r0,@(4,r3)	! 1->BCR2 (0xff800004)
	mov	#0xc3,r0	! r0 = 0xffffffc3
	shll16	r0		! r0 = 0xffc30000
	or	#0xcd,r0	! r0 = 0xffc300cd
	shll8	r0		! r0 = 0xc300cd00
	or	#0xb0,r0	! r0 = 0xc300cdb0
	shlr	r0		! r0 = 0x618066d8
	mov.l	r0,@(12,r3)	! 0x618066d8->BCR1 (0xff80000c)

	! Jump to cached ROM copy
	mov	#1,r5		! r5 = 1
	rotr	r5		! r5 = 0x80000000
	add	#0x60,r5	! r5 = 0x80000060
	mov	r5,r6		! r6 = 0x80000060
	add	#0x20,r6	! r6 = 0x80000080
	pref	@r5		! Prefetch our init table
	jmp	@r6		! Jump to 0x80000080
	nop

!!!!!!!!! RAM Setup !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	.org	0x60
! A table of values to load into the CPU regs
	.word	0xa504		! -> RTCOR
	.word	0xa55e		! -> RTCOR
	.long	0xa05f7480
	.long	0xa3020008	! -> BCR1
	.long	0x8c0000e0
	.long	0x01110111	! -> WCR1
	.long	0x800a0e24	! -> MCR
	.long	0xc00a0e24	! -> MCR
	.long	0xff940190

	.org	0x80
! Ok, we should be running in a cached ROM region now
inside_cache:
	! These values make a bit more sense, and again I am basically
	! duplicating the same process here to avoid any problems it might
	! cause to get one of these values wrong.
	mov.l	@(8,r5),r0	! 0xa3020008->BCR1 (0xff800000)
	mov.l	r0,@(0,r3)
	mov.l	@(16,r5),r0	! 0x01110111->WCR1 (0xff800008)
	mov.l	r0,@(8,r3)
	add	#16,r3		! r3 = 0xff800010
	mov.l	@(20,r5),r0	! 0x800a0e24->MCR (0xff800014)
	mov.l	r0,@(4,r3)
	mov.l	@(28,r5),r2	! 0x90->SDMR3 (0xff940190)
	mov.b	r2,@r2
	mov	#0xa4,r0	! 0xa400->RFCR (0xff800028)
	shll8	r0
	mov.w	r0,@(24,r3)
	mov.w	@(0,r5),r0	! 0xa504->RTCOR (0xff800024)
	mov.w	r0,@(20,r3)
	add	#12,r0		! r0 = 0xa510
	mov.w	r0,@(12,r3)	! 0xa510->RTCSR (0xff80001c)

	mov	#16,r4		! Wait for RAM to refresh 16 times (?)
.rf_loop:
	mov.w	@(24,r3),r0
	cmp/hi	r4,r0
	bf	.rf_loop

	mov.w	@(2,r5),r0	! 0xa55e->RTCOR (0xff800024)
	mov.w	r0,@(20,r3)
	mov.l	@(24,r5),r0	! 0xc00a0e24->MCR (0xff800004)
	mov.l	r0,@(4,r3)
	mov.b	r2,@r2		! 0x90->SDMR3 (0xff940190)
	mov.l	@(4,r5),r1	! 0x0400->0xa05f7480 (ASIC bus setup?)
	mov	#4,r0
	shll8	r0
	mov.w	r0,@r1

! Ok, now the RAM-related hardware is all initialized, move the next stage of
! the boot loader into RAM so it will execute faster. Note that we are
! starting at 0x8c0000e0 so there's plenty of room left under that for
! the syscall pointers.
	mov.l	@(12,r5),r4		! dst address
	mova	in_ram,r0		! src address
	mov.l	.in_ram_words,r6	! Number of words
.copy_ram_loop:
	dt	r6
	mov.w	@r0+,r1
	mov.w	r1,@r4
	bf/s	.copy_ram_loop
	add	#2,r4

! Jump to it
	mov.l	@(12,r5),r4
	jmp	@r4
	nop

	.align	2
.in_ram_words:
	.long	(in_ram_end - in_ram)/2


!!!!!!!!! ASIC/Peripheral Setup !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! We should now be running in RAM instead of ROM, which is much faster.
	.org	0xe0
in_ram:

! Do a couple more hardware setup things so that externally attached
! hardware will work properly (among other things). These are various
! word sizes, so they don't fit as easily in the value table.
	mov.l	pctra,r0	! 0xa03f0 -> PCTRA
	mov.l	r0,@(0x1c,r3)
	mov	#0,r0		! r0 = 0
	add	#0x20,r3
	mov.w	r0,@(0x00,r3)	! 0 -> PDTRA
	mov.l	r0,@(0x10,r3)	! 0 -> PCTRB
	mov.w	r0,@(0x14,r3)	! 0 -> PDTRB
	mov.w	r0,@(0x18,r3)	! 0 -> GPIOIC

	! RTC
	mov.l	rtc_base,r3
	mov.b	r0,@(4,r3)	! 0 -> RMONAR
	mov.b	r0,@(8,r3)	! 0 -> RCR1

	! CPG
	mov.l	cpg_base,r3
	mov	#3,r0		! 3 -> STBCR
	mov.b	r0,@(4,r3)
	mov	#0x5a,r0
	shll8	r0
	mov.w	r0,@(8,r3)	! 0x5a00 -> WTCNT
	mov.w	r0,@(12,r3)	! 0x5a00 -> WTCSR
	mov	#0,r0		! 0 -> STBCR2
	add	#16,r3
	mov.b	r0,@(0,r3)

	! TMU
	mov.l	tmu_base,r3
	mov.b	r0,@(0,r3)	! 0 -> TOCR
	mov.b	r0,@(4,r3)	! 0 -> TSTR
	mov	#-1,r1
	mov.l	r1,@(8,r3)	! 0xffffffff -> TCOR0
	mov.l	r1,@(12,r3)	! "          -> TCNT0
	mov	#2,r0		! 2          -> TCR0
	mov.w	r0,@(16,r3)
	mov.l	r1,@(20,r3)	! 0xffffffff -> TCOR1
	mov.l	r1,@(24,r3)	! "          -> TCNT1
	mov	#0,r0		! 0          -> TCR1
	mov.w	r0,@(28,r3)
	add	#32,r3
	mov.l	r1,@(0,r3)	! Same as xxx1
	mov.l	r1,@(4,r3)
	mov.w	r0,@(8,r3)
	add	#-32,r3
	mov	#1,r0
	mov.b	r0,@(4,r3)	! 1 -> TSTR

	! INTC
	mov.l	intc_base,r3
	mov	#0,r0		! 0 -> ICR
	mov.w	r0,@(0,r3)
	mov.w	r0,@(4,r3)	! 0 -> IPRA
	mov.w	r0,@(8,r3)	! 0 -> IPRB
	mov.w	r0,@(12,r3)	! 0 -> IPRC

	! UBC
	mov.l	ubc_base,r3
	mov.w	r0,@(8,r3)	! 0 -> BBRA
	mov.w	r0,@(20,r3)	! 0 -> BBRB
	add	#32,r3
	mov.w	r0,@(0,r3)	! 0 -> BBCR

! Install the value table
	mova	value_tbl,r0
	mov	#0,r4
.value_loop:
	mov.l	@r0+,r1
	cmp/eq	r1,r4
	bt	.value_loop_end
	mov.l	@r0+,r2
	bra	.value_loop
	mov.l	r2,@r1
.value_loop_end:

! Setup a temporary stack
	mov.l	stack,r15

! Do AICA setup (a bit more involved unfortunately...)	
	bsr	aica_setup
	nop

!!!!!!!!! Program Load !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Setup our in-RAM syscalls
	mov.l	sc_base,r1
	mova	unsupp_syscall,r0
	mov.l	r0,@(0,r1)	! SYSINFO
	mov.l	r0,@(8,r1)	! FLASHROM
	mov.l	r0,@(12,r1)	! MISC/GDROM
	mova	font_syscall,r0
	mov.l	r0,@(4,r1)	! ROMFONT

! Copy the target program into RAM at 8c010000
	mov.l	src_addr,r0	! src addr
	mov.l	dst_addr,r1	! dst addr
	mov.l	cnt,r2		! count
.copy_prg_loop:
	dt	r2
	mov.l	@r0+,r3
	mov.l	r3,@r1
	bf/s	.copy_prg_loop
	add	#4,r1

! We shouldn't need to ocwb here because the processor just
! came out of reset -- it has no cached data for 8c01xxxx.

! Jump to it

	mov.l	pctra2,r0	! 0xac3f0 -> r0
	mov.l	pctrabase,r3 ! 0xff80002c -> r3
	mov.l	r0,@r3 ! 0xac3f0 -> PCTRA
	
	mov.l	delay,r1
.pause:	
	nop
	dt	r1
	bf	.pause
	nop


	mov.l	pctra,r0	! 0xa03f0 -> r0
	mov.l	r0,@r3 ! 0xa03f0 -> PCTRA
	
	mov.l	delay,r1	
.pause1:	
	nop
	dt	r1
	bf	.pause1
	nop

	mov.l	dst_addr,r0
	jsr	@r0
	nop

! If we make it back here, that's because the program returned.. just die
.dead_loop:	bra	.dead_loop
		nop

! Some setup vars
	.align	2
pctra:
	.long	0x000a03f0	! PCTRA value
pctra2:
	.long	0x000ac3f0	! PCTRA value
pctrabase:
	.long	0xff80002c	! PCTRA Base adress
src_addr:
	.long	0x80004000	! Whatever's after us in flash
dst_addr:
	.long	0x8c010000	! Destination address (normal RAM location)
cnt:
	.long	0x1FC000/4	! The number of dwords in the rest of ROM
delay:	
	.long	0x1FFFFF
stack:
	.long	0x8c004000	! Temporary stack location
cpg_base:
	.long	0xffc00000	! Register bases
rtc_base:
	.long	0xffc80030
tmu_base:
	.long	0xffd80000
intc_base:
	.long	0xffd00000
ubc_base:
	.long	0xff200000
sc_base:
	.long	0x8c0000b0	! Syscall pointer base


!!!!!!!!! BIOS Syscalls !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! Unsupported syscall: return error automatically
unsupp_syscall:
	rts
	mov	#-1,r0


! ROM Font syscall
font_syscall:
	mov	#0,r0
	cmp/eq	r0,r1		! Are we querying for a font or doing the weird mutex thing?
	bt	.get_font_addr
	
	rts			! Yes, just return success
	nop

.get_font_addr:
	mov.l	.font_addr,r0
	rts			! Otherwise return the font location
	nop

	.align	2
.font_addr:
	.long	0xa0001000


!!!!!!!!! AICA Setup !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! By the time we get here, the value table routine has already
! taken care of setting up the initial values for 2800, 289c,
! 28a4, 28b4, and 28bc. We just need to clear out the scratch
! RAM and put some initial values in the channel regs.
aica_setup:
	sts.l	pr,@-r15
	
	! Make sure the AICA's ARM is suspended
	mov.l	.rbase3,r1
	mov.l	@r1,r0
	or	#1,r0
	mov.l	r0,@r1

	! Setup some misc regs up front
	! Note, these seem to disable the SPU so we'll set them
	!   to different values at the bottom.
	mov.l	.rbase4,r1	! R1 = 0xa0702800
	mov	#0,r0
	mov.l	r0,@(0,r1)	! 0 -> 0xa0702800
	add	#78,r1
	add	#78,r1		! R1 = 0xa070289c
	mov.l	r0,@(0,r1)	! 0 -> 0xa070289c
	mov.l	r0,@(24,r1)	! 0 -> 0xa07028b4
	mov	#7,r0		! R0 = 7
	shll8	r0		! R0 = 0x0700
	or	#-1,r0		! R0 = 0x07ff
	mov.l	r0,@(8,r1)	! 0x7ff -> 0xa07028a4
	add	#32,r1		! R1 = 0xa07028bc
	mov.l	r0,@(0,r1)	! 0x7ff -> 0xa07028bc

	! Fifo wait
	bsr	g2fifo
	nop
	
	! Clear out scratch RAM areas
	mov.l	.srbase1,r4	! Base
	bsr	clear_ram
	mov	#0x17,r8	! Loop count (ds)
	
	mov.l	.srbase2,r4	! Base
	bsr	clear_ram
	mov	#63,r8		! Loop count (ds)
	
	mov.l	.srbase3,r4	! Base
	bsr	clear_ram
	mov	#45,r8		! Loop count (ds)

	! Setup some more registers
	mov.l	.rbase1,r4
	mov	#0,r0
	mov.l	r0,@(0,r4)
	mov.l	r0,@(4,r4)

	! Clear out the channel registers (65 of 'em, counting the CDDA channel)
	mov	#65,r8		! Outer loop count
	mov	#0,r1
	mov.l	.rbase2,r4
.chanloop:
	mov.l	r1,@(0,r4)	! First block
	mov.l	r1,@(4,r4)
	mov.l	r1,@(8,r4)
	mov.l	r1,@(12,r4)
	mov.l	r1,@(16,r4)
	bsr	g2fifo
	mov.l	r1,@(20,r4)

	add	#24,r4		! Second block
	mov.l	r1,@(0,r4)
	mov.l	r1,@(4,r4)
	mov.l	r1,@(8,r4)
	mov.l	r1,@(12,r4)
	mov.l	r1,@(16,r4)
	bsr	g2fifo
	mov.l	r1,@(20,r4)

	add	#24,r4		! Third block
	mov.l	r1,@(0,r4)
	mov.l	r1,@(4,r4)
	mov.l	r1,@(8,r4)
	mov.l	r1,@(12,r4)
	mov.l	r1,@(16,r4)
	bsr	g2fifo
	mov.l	r1,@(20,r4)

	add	#80,r4		! Skip up for the next channel (0x80*chan)
	dt	r8
	bf	.chanloop

	! All done
	lds.l	@r15+,pr
	rts
	nop

	.align	2
.srbase1:	.long	0xa0703000	! Scratch RAM bases
.srbase2:	.long	0xa0703400
.srbase3:	.long	0xa0704000
.rbase1:	.long	0xa07045c0	! Register bases
.rbase2:	.long	0xa0700000
.rbase3:	.long	0xa0702c00
.rbase4:	.long	0xa0702800


! Clears out one set of scratch RAM
! Uses r2, r3, r4, r8, and transitively, r0, r9, and r10.
clear_ram:
	sts.l	pr,@-r15

	mov	#0,r2
.clear_loop:
	mov.l	r2,@(0,r4)
	mov.l	r2,@(4,r4)
	mov.l	r2,@(8,r4)
	mov.l	r2,@(12,r4)
	mov.l	r2,@(16,r4)
	mov.l	r2,@(20,r4)
	mov.l	r2,@(24,r4)
	mov.l	r2,@(28,r4)
	bsr	g2fifo
	nop
	add	#32,r4
	dt	r8
	bf	.clear_loop

	lds.l	@r15+,pr
	rts
	nop

! This little routine will take care of waiting for the G2 fifo
! Uses r0, r9, and r10
g2fifo:
	mov.l	.g2fifo_addr,r9
	mov.w	.g2fifo_addr+4,r10
	mov.l	@r9,r0
.g2fifo_loop:
	dt	r10
	bt	.g2fifo_end
	tst	#1,r0
	bf/s	.g2fifo_loop
	mov.l	@r9,r0
.g2fifo_end:
	rts
	nop
	
	.align	2
.g2fifo_addr:
	.long	0xa05f688c
	.word	0x1800


!!!!!!!!! Value Table !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! This table is a bunch of random values that need to be set before
! we can be in a "normal" state. The DC BIOS usually sets this before
! loading a program, but we'll do it instead. The format is
!  <address>  <value>
	.align	2
value_tbl:
! More CPU reg setup
	! CCN
	.long	0xff000000, 0x00000000		! PTEH
	.long	0xff000004, 0x00000000		! PTEL
	.long	0xff000008, 0x00000000		! TTB
	.long	0xff00000c, 0x00000000		! TEA
	.long	0xff000020, 0x00000000		! TRA
	.long	0xff000024, 0x00000000		! EXPEVT
	.long	0xff000028, 0x00000000		! INTEVT
	.long	0xff000034, 0x00000000		! PTEA
	.long	0xff000038, 0x00000000		! QACR0
	.long	0xff00003c, 0x00000000		! QACR1

	! DMAC
	.long	0xffa00010, 0x00000000		! SAR1
	.long	0xffa00014, 0x00000000		! DAR1
	.long	0xffa00018, 0x00000000		! DMATCR1
	.long	0xffa0001c, 0x00005440		! CHCR1
	.long	0xffa00020, 0x00000000		! SAR2
	.long	0xffa00024, 0x00000000		! DAR2
	.long	0xffa00028, 0x00000000		! DMATCR2
	.long	0xffa0002c, 0x000052c0		! CHCR2
	.long	0xffa00030, 0x00000000		! SAR3
	.long	0xffa00034, 0x00000000		! DAR3
	.long	0xffa00038, 0x00000000		! DMATCR3
	.long	0xffa0003c, 0x00005440		! CHCR3
	.long	0xffa00040, 0x00008201		! DMAOR

! Initial ASIC setup (unknown, interrupts, maple, DMA)
	.long	0xa05f6800, 0x11ff0000
	.long	0xa05f6804, 0x00000020
	.long	0xa05f6808, 0x00000000
	.long	0xa05f6810, 0x0cff0000
	.long	0xa05f6814, 0x0cff0000
	.long	0xa05f6818, 0x00000000
	.long	0xa05f681c, 0x00000000
	.long	0xa05f6820, 0x00000000
	.long	0xa05f6840, 0x00000000
	.long	0xa05f6844, 0x00000000
	.long	0xa05f6848, 0x00000000
	.long	0xa05f684c, 0x00000000
	.long	0xa05f6884, 0x00000000
	.long	0xa05f6888, 0x00000000
	.long	0xa05f68a0, 0x80000000
	.long	0xa05f68a4, 0x00000000
	.long	0xa05f68ac, 0x00000000
	.long	0xa05f6910, 0x00000000
	.long	0xa05f6914, 0x00000000
	.long	0xa05f6918, 0x00000000
	.long	0xa05f6920, 0x00000000
	.long	0xa05f6924, 0x00000000
	.long	0xa05f6928, 0x00000000
	.long	0xa05f6930, 0x00000000
	.long	0xa05f6934, 0x00000000
	.long	0xa05f6938, 0x00000000
	.long	0xa05f6940, 0x00000000
	.long	0xa05f6944, 0x00000000
	.long	0xa05f6950, 0x00000000
	.long	0xa05f6954, 0x00000000
	.long	0xa05f6c04, 0x0cff0000
	.long	0xa05f6c10, 0x00000000
	.long	0xa05f6c14, 0x00000000
	.long	0xa05f6c18, 0x00000000
	.long	0xa05f6c80, 0xc3500000
	.long	0xa05f6c8c, 0x61557f00
	.long	0xa05f6ce8, 0x00000001
	.long	0xa05f7404, 0x0cff0000
	.long	0xa05f7408, 0x00000020
	.long	0xa05f740c, 0x00000000
	.long	0xa05f7414, 0x00000000
	.long	0xa05f7418, 0x00000000
	.long	0xa05f7484, 0x00000400
	.long	0xa05f7488, 0x00000200
	.long	0xa05f748c, 0x00000200
	.long	0xa05f7490, 0x00000222
	.long	0xa05f7494, 0x00000222
	.long	0xa05f74a0, 0x00001001
	.long	0xa05f74a4, 0x00001001
	.long	0xa05f74b4, 0x00000001
	.long	0xa05f74b8, 0x8843407f
	.long	0xa05f7800, 0x009f0000
	.long	0xa05f7804, 0x0cff0000
	.long	0xa05f7808, 0x00000020
	.long	0xa05f780c, 0x00000000
	.long	0xa05f7810, 0x00000005
	.long	0xa05f7814, 0x00000000
	.long	0xa05f7818, 0x00000000
	.long	0xa05f781c, 0x00000000
	.long	0xa05f7820, 0x009f0000
	.long	0xa05f7824, 0x0cff0000
	.long	0xa05f7828, 0x00000020
	.long	0xa05f782c, 0x00000000
	.long	0xa05f7830, 0x00000005
	.long	0xa05f7834, 0x00000000
	.long	0xa05f7838, 0x00000000
	.long	0xa05f783c, 0x00000000
	.long	0xa05f7840, 0x009f0000
	.long	0xa05f7844, 0x0cff0000
	.long	0xa05f7848, 0x00000020
	.long	0xa05f784c, 0x00000000
	.long	0xa05f7850, 0x00000005
	.long	0xa05f7854, 0x00000000
	.long	0xa05f7858, 0x00000000
	.long	0xa05f785c, 0x00000000
	.long	0xa05f7860, 0x009f0000
	.long	0xa05f7864, 0x0cff0000
	.long	0xa05f7868, 0x00000020
	.long	0xa05f786c, 0x00000000
	.long	0xa05f7870, 0x00000005
	.long	0xa05f7874, 0x00000000
	.long	0xa05f7878, 0x00000000
	.long	0xa05f787c, 0x00000000
	.long	0xa05f7890, 0x0000001b
	.long	0xa05f7894, 0x00000271
	.long	0xa05f7898, 0x00000000
	.long	0xa05f789c, 0x00000001
	.long	0xa05f78a0, 0x00000000
	.long	0xa05f78a4, 0x00000000
	.long	0xa05f78a8, 0x00000000
	.long	0xa05f78ac, 0x00000000
	.long	0xa05f78b0, 0x00000000
	.long	0xa05f78b4, 0x00000000
	.long	0xa05f78b8, 0x00000000
	.long	0xa05f78bc, 0x46597f00
	.long	0xa05f7c00, 0x04ff0000
	.long	0xa05f7c04, 0x0cff0000
	.long	0xa05f7c08, 0x00000020
	.long	0xa05f7c0c, 0x00000000
	.long	0xa05f7c10, 0x00000000
	.long	0xa05f7c14, 0x00000000
	.long	0xa05f7c18, 0x00000000
	.long	0xa05f7c80, 0x67027f00

! Clear ASIC interrupts
	.long	0xa05f6900, 0xffffffff
	.long	0xa05f6908, 0xffffffff

! G2EXT setup (reset any attached peripherals)
! This should technically have a delay between the two, but I don't
!   have a good way of enforcing delays here =)
	.long	0xa0600480, 0x00000000
	.long	0xa0600480, 0x00000001

! PVR2 initial setup
! KOS does some of these later, but I suspect we need them earlier than
!   3D =), Especially the M values. Some of them are just plain
!   useless until we get to 3D, but might as well put some reasonable
!   defaults in them now.
	.long	0xa05f8044, 0x00800000	! FB_CFG_1
	.long	0xa05f8008, 0xffffffff	! PVR_RESET
	.long	0xa05f8008, 0x00000000	! PVR_RESET
	.long	0xa05f80a8, 0x15d1c951	! M
	.long	0xa05f80a0, 0x00000020	! M
	.long	0xa05f8048, 0x00000009	! FB_CFG_2
	.long	0xa05f80e8, 0x00160008	! VIDEO_CFG
	.long	0xa05f80ec, 0x000000a8	! M
	.long	0xa05f80f0, 0x00280028	! M
	.long	0xa05f80c8, 0x03450000	! M
	.long	0xa05f80cc, 0x00150208	! VPOS_IRQ
	.long	0xa05f80d0, 0x00000100	! M
	.long	0xa05f80d4, 0x007e0345	! M
	.long	0xa05f80d8, 0x020c0359	! M
	.long	0xa05f80dc, 0x00280208	! M
	.long	0xa05f80e0, 0x03f1933f	! M
	.long	0xa05f807c, 0x0027df77	! M
	.long	0xa05f8080, 0x00000007	! M
	.long	0xa05f8084, 0x00000000	! TEXTURE_CLIP
	.long	0xa50f8098, 0x00800408	! M
	.long	0xa05f8110, 0x00093f39	! M
	.long	0xa05f8114, 0x00200000	! M
	.long	0xa05f8118, 0x00008040	! M
	.long	0xa05f8160, 0x00000000	! M
	.long	0xa05f8018, 0x00000000	! M
	.long	0xa05f8030, 0x00000101	! SPANSORT_CFG
	.long	0xa05f80b0, 0x007f7f7f	! FOG_TABLE_COLOR
	.long	0xa05f80b4, 0x007f7f7f	! FOG_VERTEX_COLOR
	.long	0xa05f80c0, 0x00000000	! COLOR_CLAMP_MIN
	.long	0xa05f80bc, 0xffffffff	! COLOR_CLAMP_MAX
	.long	0xa05f8074, 0x00000001	! CHEAP_SHADOW
	.long	0xa05f80e4, 0x00000000	! TEXTURE_MODULO
	.long	0xa05f80b8, 0x0000ff07	! FOG_DENSITY
	.long	0xa05f80f4, 0x00000400	! SCALER_CFG
	.long	0xa05f8050, 0x00000000	! FB Start Addr
! Set these again to enable video output
	.long	0xa05f8044, 0x00800001	! FB_CFG_1
	.long	0xa05f80e8, 0x00160000	! VIDEO_CFG

! Sentinel
	.long	0

! This marks the end of what will be copied into RAM for setup
in_ram_end:

! Font; most DC programs expect there to be a BIOS font available, so we
! kind of have to oblige to see text on the screen in such cases. Note
! that our font table only includes a tiny subset of the original font
! data because of space constraints (just the ASCII chars).
	.org		0x1000
	.include	"font.s"

! The program to load goes after this; we put it up this far because
! we can program this little bootstrap into the boot block and then
! forget about it. This helps out with flash wear and tear.
	.org	0x4000
begin_data:
.include	"prog.s"
