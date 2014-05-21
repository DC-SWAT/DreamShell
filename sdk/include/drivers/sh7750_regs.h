/**
 * SH-7750 memory-mapped registers
 * This file based on information provided in the following document:
 * "Hitachi SuperH (tm) RISC engine. SH7750 Series (SH7750, SH7750S)
 *  Hardware Manual"
 *  Document Number ADE-602-124C, Rev. 4.0, 4/21/00, Hitachi Ltd.
 *
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Alexandra Kossovsky <sasha@oktet.ru>
 *         Victor V. Vengerov <vvv@oktet.ru>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 * @(#) $Id: sh7750_regs.h,v 1.5 2006/09/13 11:22:13 ralf Exp $
 *
 * 2011-2014 Modified by SWAT
 */

#ifndef __SH7750_REGS_H__
#define __SH7750_REGS_H__

#include <arch/types.h>

/*
 * All register has 2 addresses: in 0xff000000 - 0xffffffff (P4 address)  and
 * in 0x1f000000 - 0x1fffffff (area 7 address)
 */
#define P4_BASE       0xff000000 /* Accessable only in
                                           priveleged mode */
#define A7_BASE       0x1f000000 /* Accessable only using TLB */

#define P4_REG32(ofs) (P4_BASE + (ofs))
#define A7_REG32(ofs) (A7_BASE + (ofs))


/**
 * Macros for r/w registers
 */
#define reg_read_8(a)          	(*(vuint8 *)(a))
#define reg_read_16(a)          (*(vuint16 *)(a))
#define reg_read_32(a)          (*(vuint32 *)(a))

#define reg_write_8(a, v)      	(*(vuint8 *)(a)  = (v))
#define reg_write_16(a, v)      (*(vuint16 *)(a) = (v))
#define reg_write_32(a, v)      (*(vuint32 *)(a) = (v))


/*
 * MMU Registers
 */

/* Page Table Entry High register - PTEH */
#define PTEH_REGOFS    0x000000 /* offset */
#define PTEH           P4_REG32(PTEH_REGOFS)
#define PTEH_A7        A7_REG32(PTEH_REGOFS)
#define PTEH_VPN       0xfffffd00 /* Virtual page number */
#define PTEH_VPN_S     10
#define PTEH_ASID      0x000000ff /* Address space identifier */
#define PTEH_ASID_S    0

/* Page Table Entry Low register - PTEL */
#define PTEL_REGOFS    0x000004 /* offset */
#define PTEL           P4_REG32(PTEL_REGOFS)
#define PTEL_A7        A7_REG32(PTEL_REGOFS)
#define PTEL_PPN       0x1ffffc00 /* Physical page number */
#define PTEL_PPN_S     10
#define PTEL_V         0x00000100 /* Validity (0-entry is invalid) */
#define PTEL_SZ1       0x00000080 /* Page size bit 1 */
#define PTEL_SZ0       0x00000010 /* Page size bit 0 */
#define PTEL_SZ_1KB    0x00000000 /*   1-kbyte page */
#define PTEL_SZ_4KB    0x00000010 /*   4-kbyte page */
#define PTEL_SZ_64KB   0x00000080 /*   64-kbyte page */
#define PTEL_SZ_1MB    0x00000090 /*   1-Mbyte page */
#define PTEL_PR        0x00000060 /* Protection Key Data */
#define PTEL_PR_ROPO   0x00000000 /*   read-only in priv mode */
#define PTEL_PR_RWPO   0x00000020 /*   read-write in priv mode */
#define PTEL_PR_ROPU   0x00000040 /*   read-only in priv or user mode*/
#define PTEL_PR_RWPU   0x00000060 /*   read-write in priv or user mode*/
#define PTEL_C         0x00000008 /* Cacheability
                                            (0 - page not cacheable) */
#define PTEL_D         0x00000004 /* Dirty bit (1 - write has been
                                            performed to a page) */
#define PTEL_SH        0x00000002 /* Share Status bit (1 - page are
                                            shared by processes) */
#define PTEL_WT        0x00000001 /* Write-through bit, specifies the
                                            cache write mode:
                                               0 - Copy-back mode
                                               1 - Write-through mode */

/* Page Table Entry Assistance register - PTEA */
#define PTEA_REGOFS    0x000034 /* offset */
#define PTEA           P4_REG32(PTEA_REGOFS)
#define PTEA_A7        A7_REG32(PTEA_REGOFS)
#define PTEA_TC        0x00000008 /* Timing Control bit
                                               0 - use area 5 wait states
                                               1 - use area 6 wait states */
#define PTEA_SA        0x00000007 /* Space Attribute bits: */
#define PTEA_SA_UNDEF  0x00000000 /*    0 - undefined */
#define PTEA_SA_IOVAR  0x00000001 /*    1 - variable-size I/O space */
#define PTEA_SA_IO8    0x00000002 /*    2 - 8-bit I/O space */
#define PTEA_SA_IO16   0x00000003 /*    3 - 16-bit I/O space */
#define PTEA_SA_CMEM8  0x00000004 /*    4 - 8-bit common memory space*/
#define PTEA_SA_CMEM16 0x00000005 /*    5 - 16-bit common memory space*/
#define PTEA_SA_AMEM8  0x00000006 /*    6 - 8-bit attr memory space */
#define PTEA_SA_AMEM16 0x00000007 /*    7 - 16-bit attr memory space */


/* Translation table base register */
#define TTB_REGOFS     0x000008 /* offset */
#define TTB            P4_REG32(TTB_REGOFS)
#define TTB_A7         A7_REG32(TTB_REGOFS)

/* TLB exeption address register - TEA */
#define TEA_REGOFS     0x00000c /* offset */
#define TEA            P4_REG32(TEA_REGOFS)
#define TEA_A7         A7_REG32(TEA_REGOFS)

/* MMU control register - MMUCR */
#define MMUCR_REGOFS   0x000010 /* offset */
#define MMUCR          P4_REG32(MMUCR_REGOFS)
#define MMUCR_A7       A7_REG32(MMUCR_REGOFS)
#define MMUCR_AT       0x00000001 /* Address translation bit */
#define MMUCR_TI       0x00000004 /* TLB invalidate */
#define MMUCR_SV       0x00000100 /* Single Virtual Mode bit */
#define MMUCR_SQMD     0x00000200 /* Store Queue Mode bit */
#define MMUCR_URC      0x0000FC00 /* UTLB Replace Counter */
#define MMUCR_URC_S    10
#define MMUCR_URB      0x00FC0000 /* UTLB Replace Boundary */
#define MMUCR_URB_S    18
#define MMUCR_LRUI     0xFC000000 /* Least Recently Used ITLB */
#define MMUCR_LRUI_S   26




/*
 * Cache registers
 *   IC -- instructions cache
 *   OC -- operand cache
 */

/* Cache Control Register - CCR */
#define CCR_REGOFS     0x00001c /* offset */
#define CCR            P4_REG32(CCR_REGOFS)
#define CCR_A7         A7_REG32(CCR_REGOFS)

#define CCR_IIX      0x00008000 /* IC index enable bit */
#define CCR_ICI      0x00000800 /* IC invalidation bit:
                                          set it to clear IC */
#define CCR_ICE      0x00000100 /* IC enable bit */
#define CCR_OIX      0x00000080 /* OC index enable bit */
#define CCR_ORA      0x00000020 /* OC RAM enable bit
                                          if you set OCE = 0,
                                          you should set ORA = 0 */
#define CCR_OCI      0x00000008 /* OC invalidation bit */
#define CCR_CB       0x00000004 /* Copy-back bit for P1 area */
#define CCR_WT       0x00000002 /* Write-through bit for P0,U0,P3 area */
#define CCR_OCE      0x00000001 /* OC enable bit */

/* Queue address control register 0 - QACR0 */

#ifndef QACR0

#define QACR0_REGOFS   0x000038 /* offset */
#define QACR0          P4_REG32(QACR0_REGOFS)
#define QACR0_A7       A7_REG32(QACR0_REGOFS)

/* Queue address control register 1 - QACR1 */
#define QACR1_REGOFS   0x00003c /* offset */
#define QACR1          P4_REG32(QACR1_REGOFS)
#define QACR1_A7       A7_REG32(QACR1_REGOFS)

#endif


/*
 * Exeption-related registers
 */

/* Immediate data for TRAPA instuction - TRA */
#define TRA_REGOFS     0x000020  /* offset */
#define TRA            P4_REG32(TRA_REGOFS)
#define TRA_A7         A7_REG32(TRA_REGOFS)

#define TRA_IMM      0x000003fd /* Immediate data operand */
#define TRA_IMM_S    2

/* Exeption event register - EXPEVT */
#define EXPEVT_REGOFS  0x000024
#define EXPEVT         P4_REG32(EXPEVT_REGOFS)
#define EXPEVT_A7      A7_REG32(EXPEVT_REGOFS)

#define EXPEVT_EX      0x00000fff /* Exeption code */
#define EXPEVT_EX_S    0

/* Interrupt event register */
#define INTEVT_REGOFS  0x000028
#define INTEVT         P4_REG32(INTEVT_REGOFS)
#define INTEVT_A7      A7_REG32(INTEVT_REGOFS)
#define INTEVT_EX    0x00000fff /* Exeption code */
#define INTEVT_EX_S  0

/*
 * Exception/interrupt codes
 */
#define EVT_TO_NUM(evt)  ((evt) >> 5)

/* Reset exception category */
#define EVT_POWER_ON_RST        0x000  /* Power-on reset */
#define EVT_MANUAL_RST          0x020  /* Manual reset */
#define EVT_TLB_MULT_HIT        0x140  /* TLB multiple-hit exception */

/* General exception category */
#define EVT_USER_BREAK          0x1E0 /* User break */
#define EVT_IADDR_ERR           0x0E0 /* Instruction address error */
#define EVT_TLB_READ_MISS       0x040 /* ITLB miss exception /
                                                DTLB miss exception (read) */
#define EVT_TLB_READ_PROTV      0x0A0 /* ITLB protection violation /
                                             DTLB protection violation (read)*/
#define EVT_ILLEGAL_INSTR       0x180 /* General Illegal Instruction
                                                exception */
#define EVT_SLOT_ILLEGAL_INSTR  0x1A0 /* Slot Illegal Instruction
                                                exception */
#define EVT_FPU_DISABLE         0x800 /* General FPU disable exception*/
#define EVT_SLOT_FPU_DISABLE    0x820 /* Slot FPU disable exception */
#define EVT_DATA_READ_ERR       0x0E0 /* Data address error (read) */
#define EVT_DATA_WRITE_ERR      0x100 /* Data address error (write) */
#define EVT_DTLB_WRITE_MISS     0x060 /* DTLB miss exception (write) */
#define EVT_DTLB_WRITE_PROTV    0x0C0 /* DTLB protection violation
                                                exception (write) */
#define EVT_FPU_EXCEPTION       0x120 /* FPU exception */
#define EVT_INITIAL_PGWRITE     0x080 /* Initial Page Write exception */
#define EVT_TRAPA               0x160 /* Unconditional trap (TRAPA) */

/* Interrupt exception category */
#define EVT_NMI                 0x1C0 /* Non-maskable interrupt */
#define EVT_IRQ0                0x200 /* External Interrupt 0 */
#define EVT_IRQ1                0x220 /* External Interrupt 1 */
#define EVT_IRQ2                0x240 /* External Interrupt 2 */
#define EVT_IRQ3                0x260 /* External Interrupt 3 */
#define EVT_IRQ4                0x280 /* External Interrupt 4 */
#define EVT_IRQ5                0x2A0 /* External Interrupt 5 */
#define EVT_IRQ6                0x2C0 /* External Interrupt 6 */
#define EVT_IRQ7                0x2E0 /* External Interrupt 7 */
#define EVT_IRQ8                0x300 /* External Interrupt 8 */
#define EVT_IRQ9                0x320 /* External Interrupt 9 */
#define EVT_IRQA                0x340 /* External Interrupt A */
#define EVT_IRQB                0x360 /* External Interrupt B */
#define EVT_IRQC                0x380 /* External Interrupt C */
#define EVT_IRQD                0x3A0 /* External Interrupt D */
#define EVT_IRQE                0x3C0 /* External Interrupt E */

/* Peripheral Module Interrupts - Timer Unit (TMU) */
#define EVT_TUNI0               0x400 /* TMU Underflow Interrupt 0 */
#define EVT_TUNI1               0x420 /* TMU Underflow Interrupt 1 */
#define EVT_TUNI2               0x440 /* TMU Underflow Interrupt 2 */
#define EVT_TICPI2              0x460 /* TMU Input Capture Interrupt 2*/

/* Peripheral Module Interrupts - Real-Time Clock (RTC) */
#define EVT_RTC_ATI             0x480 /* Alarm Interrupt Request */
#define EVT_RTC_PRI             0x4A0 /* Periodic Interrupt Request */
#define EVT_RTC_CUI             0x4C0 /* Carry Interrupt Request */

/* Peripheral Module Interrupts - Serial Communication Interface (SCI) */
#define EVT_SCI_ERI             0x4E0 /* Receive Error */
#define EVT_SCI_RXI             0x500 /* Receive Data Register Full */
#define EVT_SCI_TXI             0x520 /* Transmit Data Register Empty */
#define EVT_SCI_TEI             0x540 /* Transmit End */

/* Peripheral Module Interrupts - Watchdog Timer (WDT) */
#define EVT_WDT_ITI             0x560 /* Interval Timer Interrupt
                                                (used when WDT operates in
                                                interval timer mode) */

/* Peripheral Module Interrupts - Memory Refresh Unit (REF) */
#define EVT_REF_RCMI            0x580 /* Compare-match Interrupt */
#define EVT_REF_ROVI            0x5A0 /* Refresh Counter Overflow
                                                interrupt */

/* Peripheral Module Interrupts - Hitachi User Debug Interface (H-UDI) */
#define EVT_HUDI                0x600 /* UDI interrupt */

/* Peripheral Module Interrupts - General-Purpose I/O (GPIO) */
#define EVT_GPIO                0x620 /* GPIO Interrupt */

/* Peripheral Module Interrupts - DMA Controller (DMAC) */
#define EVT_DMAC_DMTE0          0x640 /* DMAC 0 Transfer End Interrupt*/
#define EVT_DMAC_DMTE1          0x660 /* DMAC 1 Transfer End Interrupt*/
#define EVT_DMAC_DMTE2          0x680 /* DMAC 2 Transfer End Interrupt*/
#define EVT_DMAC_DMTE3          0x6A0 /* DMAC 3 Transfer End Interrupt*/
#define EVT_DMAC_DMAE           0x6C0 /* DMAC Address Error Interrupt */

/* Peripheral Module Interrupts - Serial Communication Interface with FIFO */
/*                                                                  (SCIF) */
#define EVT_SCIF_ERI            0x700 /* Receive Error */
#define EVT_SCIF_RXI            0x720 /* Receive FIFO Data Full or
                                                Receive Data ready interrupt */
#define EVT_SCIF_BRI            0x740 /* Break or overrun error */
#define EVT_SCIF_TXI            0x760 /* Transmit FIFO Data Empty */

/*
 * Power Management
 */
#define STBCR_REGOFS   0xC00004  /* offset */
#define STBCR          P4_REG32(STBCR_REGOFS)
#define STBCR_A7       A7_REG32(STBCR_REGOFS)

#define STBCR_STBY     0x80 /* Specifies a transition to standby mode:
                                      0 - Transition to SLEEP mode on SLEEP
                                      1 - Transition to STANDBY mode on SLEEP*/
#define STBCR_PHZ      0x40 /* State of peripheral module pins in
                                      standby mode:
                                         0 - normal state
                                         1 - high-impendance state */

#define STBCR_PPU      0x20 /* Peripheral module pins pull-up controls*/
#define STBCR_MSTP4    0x10 /* Stopping the clock supply to DMAC */
#define STBCR_DMAC_STP STBCR_MSTP4
#define STBCR_MSTP3    0x08 /* Stopping the clock supply to SCIF */
#define STBCR_SCIF_STP STBCR_MSTP3
#define STBCR_MSTP2    0x04 /* Stopping the clock supply to TMU */
#define STBCR_TMU_STP  STBCR_MSTP2
#define STBCR_MSTP1    0x02 /* Stopping the clock supply to RTC */
#define STBCR_RTC_STP  STBCR_MSTP1
#define STBCR_MSPT0    0x01 /* Stopping the clock supply to SCI */
#define STBCR_SCI_STP  STBCR_MSTP0

#define STBCR_STBY     0x80


#define STBCR2_REGOFS  0xC00010  /* offset */
#define STBCR2         P4_REG32(STBCR2_REGOFS)
#define STBCR2_A7      A7_REG32(STBCR2_REGOFS)

#define STBCR2_DSLP    0x80 /* Specifies transition to deep sleep mode:
                                      0 - transition to sleep or standby mode
                                          as it is specified in STBY bit
                                      1 - transition to deep sleep mode on
                                          execution of SLEEP instruction */
#define STBCR2_MSTP6   0x02 /* Stopping the clock supply to Store Queue
                                      in the cache controller */
#define STBCR2_SQ_STP  STBCR2_MSTP6
#define STBCR2_MSTP5   0x01 /* Stopping the clock supply to the User
                                      Break Controller (UBC) */
#define STBCR2_UBC_STP STBCR2_MSTP5

/*
 * Clock Pulse Generator (CPG)
 */
#define FRQCR_REGOFS   0xC00000  /* offset */
#define FRQCR          P4_REG32(FRQCR_REGOFS)
#define FRQCR_A7       A7_REG32(FRQCR_REGOFS)

#define FRQCR_CKOEN    0x0800 /* Clock Output Enable
                                          0 - CKIO pin goes to HiZ/pullup
                                          1 - Clock is output from CKIO */
#define FRQCR_PLL1EN   0x0400 /* PLL circuit 1 enable */
#define FRQCR_PLL2EN   0x0200 /* PLL circuit 2 enable */

#define FRQCR_IFC      0x01C0 /* CPU clock frequency division ratio: */
#define FRQCR_IFCDIV1  0x0000 /*    0 - * 1 */
#define FRQCR_IFCDIV2  0x0040 /*    1 - * 1/2 */
#define FRQCR_IFCDIV3  0x0080 /*    2 - * 1/3 */
#define FRQCR_IFCDIV4  0x00C0 /*    3 - * 1/4 */
#define FRQCR_IFCDIV6  0x0100 /*    4 - * 1/6 */
#define FRQCR_IFCDIV8  0x0140 /*    5 - * 1/8 */

#define FRQCR_BFC      0x0038 /* Bus clock frequency division ratio: */
#define FRQCR_BFCDIV1  0x0000 /*    0 - * 1 */
#define FRQCR_BFCDIV2  0x0008 /*    1 - * 1/2 */
#define FRQCR_BFCDIV3  0x0010 /*    2 - * 1/3 */
#define FRQCR_BFCDIV4  0x0018 /*    3 - * 1/4 */
#define FRQCR_BFCDIV6  0x0020 /*    4 - * 1/6 */
#define FRQCR_BFCDIV8  0x0028 /*    5 - * 1/8 */

#define FRQCR_PFC      0x0007 /* Peripheral module clock frequency
                                        division ratio: */
#define FRQCR_PFCDIV2  0x0000 /*    0 - * 1/2 */
#define FRQCR_PFCDIV3  0x0001 /*    1 - * 1/3 */
#define FRQCR_PFCDIV4  0x0002 /*    2 - * 1/4 */
#define FRQCR_PFCDIV6  0x0003 /*    3 - * 1/6 */
#define FRQCR_PFCDIV8  0x0004 /*    4 - * 1/8 */

/*
 * Watchdog Timer (WDT)
 */

/* Watchdog Timer Counter register - WTCNT */
#define WTCNT_REGOFS   0xC00008  /* offset */
#define WTCNT          P4_REG32(WTCNT_REGOFS)
#define WTCNT_A7       A7_REG32(WTCNT_REGOFS)
#define WTCNT_KEY      0x5A00  /* When WTCNT byte register written,
                                         you have to set the upper byte to
                                         0x5A */

/* Watchdog Timer Control/Status register - WTCSR */
#define WTCSR_REGOFS   0xC0000C  /* offset */
#define WTCSR          P4_REG32(WTCSR_REGOFS)
#define WTCSR_A7       A7_REG32(WTCSR_REGOFS)
#define WTCSR_KEY      0xA500  /* When WTCSR byte register written,
                                         you have to set the upper byte to
                                         0xA5 */
#define WTCSR_TME      0x80  /* Timer enable (1-upcount start) */
#define WTCSR_MODE     0x40  /* Timer Mode Select: */
#define WTCSR_MODE_WT  0x40  /*    Watchdog Timer Mode */
#define WTCSR_MODE_IT  0x00  /*    Interval Timer Mode */
#define WTCSR_RSTS     0x20  /* Reset Select: */
#define WTCSR_RST_MAN  0x20  /*    Manual Reset */
#define WTCSR_RST_PWR  0x00  /*    Power-on Reset */
#define WTCSR_WOVF     0x10  /* Watchdog Timer Overflow Flag */
#define WTCSR_IOVF     0x08  /* Interval Timer Overflow Flag */
#define WTCSR_CKS      0x07  /* Clock Select: */
#define WTCSR_CKS_DIV32   0x00 /*   1/32 of frequency divider 2 input */
#define WTCSR_CKS_DIV64   0x01 /*   1/64 */
#define WTCSR_CKS_DIV128  0x02 /*   1/128 */
#define WTCSR_CKS_DIV256  0x03 /*   1/256 */
#define WTCSR_CKS_DIV512  0x04 /*   1/512 */
#define WTCSR_CKS_DIV1024 0x05 /*   1/1024 */
#define WTCSR_CKS_DIV2048 0x06 /*   1/2048 */
#define WTCSR_CKS_DIV4096 0x07 /*   1/4096 */

/*
 * Real-Time Clock (RTC)
 */
/* 64-Hz Counter Register (byte, read-only) - R64CNT */
#define R64CNT_REGOFS  0xC80000  /* offset */
#define R64CNT         P4_REG32(R64CNT_REGOFS)
#define R64CNT_A7      A7_REG32(R64CNT_REGOFS)

/* Second Counter Register (byte, BCD-coded) - RSECCNT */
#define RSECCNT_REGOFS 0xC80004  /* offset */
#define RSECCNT        P4_REG32(RSECCNT_REGOFS)
#define RSECCNT_A7     A7_REG32(RSECCNT_REGOFS)

/* Minute Counter Register (byte, BCD-coded) - RMINCNT */
#define RMINCNT_REGOFS 0xC80008  /* offset */
#define RMINCNT        P4_REG32(RMINCNT_REGOFS)
#define RMINCNT_A7     A7_REG32(RMINCNT_REGOFS)

/* Hour Counter Register (byte, BCD-coded) - RHRCNT */
#define RHRCNT_REGOFS  0xC8000C  /* offset */
#define RHRCNT         P4_REG32(RHRCNT_REGOFS)
#define RHRCNT_A7      A7_REG32(RHRCNT_REGOFS)

/* Day-of-Week Counter Register (byte) - RWKCNT */
#define RWKCNT_REGOFS  0xC80010  /* offset */
#define RWKCNT         P4_REG32(RWKCNT_REGOFS)
#define RWKCNT_A7      A7_REG32(RWKCNT_REGOFS)

#define RWKCNT_SUN     0  /* Sunday */
#define RWKCNT_MON     1  /* Monday */
#define RWKCNT_TUE     2  /* Tuesday */
#define RWKCNT_WED     3  /* Wednesday */
#define RWKCNT_THU     4  /* Thursday */
#define RWKCNT_FRI     5  /* Friday */
#define RWKCNT_SAT     6  /* Saturday */

/* Day Counter Register (byte, BCD-coded) - RDAYCNT */
#define RDAYCNT_REGOFS 0xC80014  /* offset */
#define RDAYCNT        P4_REG32(RDAYCNT_REGOFS)
#define RDAYCNT_A7     A7_REG32(RDAYCNT_REGOFS)

/* Month Counter Register (byte, BCD-coded) - RMONCNT */
#define RMONCNT_REGOFS 0xC80018  /* offset */
#define RMONCNT        P4_REG32(RMONCNT_REGOFS)
#define RMONCNT_A7     A7_REG32(RMONCNT_REGOFS)

/* Year Counter Register (half, BCD-coded) - RYRCNT */
#define RYRCNT_REGOFS  0xC8001C  /* offset */
#define RYRCNT         P4_REG32(RYRCNT_REGOFS)
#define RYRCNT_A7      A7_REG32(RYRCNT_REGOFS)

/* Second Alarm Register (byte, BCD-coded) - RSECAR */
#define RSECAR_REGOFS  0xC80020  /* offset */
#define RSECAR         P4_REG32(RSECAR_REGOFS)
#define RSECAR_A7      A7_REG32(RSECAR_REGOFS)
#define RSECAR_ENB     0x80    /* Second Alarm Enable */

/* Minute Alarm Register (byte, BCD-coded) - RMINAR */
#define RMINAR_REGOFS  0xC80024  /* offset */
#define RMINAR         P4_REG32(RMINAR_REGOFS)
#define RMINAR_A7      A7_REG32(RMINAR_REGOFS)
#define RMINAR_ENB     0x80    /* Minute Alarm Enable */

/* Hour Alarm Register (byte, BCD-coded) - RHRAR */
#define RHRAR_REGOFS   0xC80028  /* offset */
#define RHRAR          P4_REG32(RHRAR_REGOFS)
#define RHRAR_A7       A7_REG32(RHRAR_REGOFS)
#define RHRAR_ENB      0x80    /* Hour Alarm Enable */

/* Day-of-Week Alarm Register (byte) - RWKAR */
#define RWKAR_REGOFS   0xC8002C  /* offset */
#define RWKAR          P4_REG32(RWKAR_REGOFS)
#define RWKAR_A7       A7_REG32(RWKAR_REGOFS)
#define RWKAR_ENB      0x80    /* Day-of-week Alarm Enable */

#define RWKAR_SUN      0  /* Sunday */
#define RWKAR_MON      1  /* Monday */
#define RWKAR_TUE      2  /* Tuesday */
#define RWKAR_WED      3  /* Wednesday */
#define RWKAR_THU      4  /* Thursday */
#define RWKAR_FRI      5  /* Friday */
#define RWKAR_SAT      6  /* Saturday */

/* Day Alarm Register (byte, BCD-coded) - RDAYAR */
#define RDAYAR_REGOFS  0xC80030  /* offset */
#define RDAYAR         P4_REG32(RDAYAR_REGOFS)
#define RDAYAR_A7      A7_REG32(RDAYAR_REGOFS)
#define RDAYAR_ENB     0x80    /* Day Alarm Enable */

/* Month Counter Register (byte, BCD-coded) - RMONAR */
#define RMONAR_REGOFS  0xC80034  /* offset */
#define RMONAR         P4_REG32(RMONAR_REGOFS)
#define RMONAR_A7      A7_REG32(RMONAR_REGOFS)
#define RMONAR_ENB     0x80    /* Month Alarm Enable */

/* RTC Control Register 1 (byte) - RCR1 */
#define RCR1_REGOFS    0xC80038  /* offset */
#define RCR1           P4_REG32(RCR1_REGOFS)
#define RCR1_A7        A7_REG32(RCR1_REGOFS)
#define RCR1_CF        0x80  /* Carry Flag */
#define RCR1_CIE       0x10  /* Carry Interrupt Enable */
#define RCR1_AIE       0x08  /* Alarm Interrupt Enable */
#define RCR1_AF        0x01  /* Alarm Flag */

/* RTC Control Register 2 (byte) - RCR2 */
#define RCR2_REGOFS    0xC8003C  /* offset */
#define RCR2           P4_REG32(RCR2_REGOFS)
#define RCR2_A7        A7_REG32(RCR2_REGOFS)
#define RCR2_PEF        0x80  /* Periodic Interrupt Flag */
#define RCR2_PES        0x70  /* Periodic Interrupt Enable: */
#define RCR2_PES_DIS    0x00  /*   Periodic Interrupt Disabled */
#define RCR2_PES_DIV256 0x10  /*   Generated at 1/256 sec interval */
#define RCR2_PES_DIV64  0x20  /*   Generated at 1/64 sec interval */
#define RCR2_PES_DIV16  0x30  /*   Generated at 1/16 sec interval */
#define RCR2_PES_DIV4   0x40  /*   Generated at 1/4 sec interval */
#define RCR2_PES_DIV2   0x50  /*   Generated at 1/2 sec interval */
#define RCR2_PES_x1     0x60  /*   Generated at 1 sec interval */
#define RCR2_PES_x2     0x70  /*   Generated at 2 sec interval */
#define RCR2_RTCEN      0x08  /* RTC Crystal Oscillator is Operated */
#define RCR2_ADJ        0x04  /* 30-Second Adjastment */
#define RCR2_RESET      0x02  /* Frequency divider circuits are reset*/
#define RCR2_START      0x01  /* 0 - sec, min, hr, day-of-week, month,
                                            year counters are stopped
                                        1 - sec, min, hr, day-of-week, month,
                                            year counters operate normally */


/*
 * Timer Unit (TMU)
 */
/* Timer Output Control Register (byte) - TOCR */
#define TOCR_REGOFS    0xD80000  /* offset */
#define TOCR           P4_REG32(TOCR_REGOFS)
#define TOCR_A7        A7_REG32(TOCR_REGOFS)
#define TOCR_TCOE      0x01  /* Timer Clock Pin Control:
                                          0 - TCLK is used as external clock
                                              input or input capture control
                                          1 - TCLK is used as on-chip RTC
                                              output clock pin */

/* Timer Start Register (byte) - TSTR */
#define TSTR_REGOFS    0xD80004  /* offset */
#define TSTR           P4_REG32(TSTR_REGOFS)
#define TSTR_A7        A7_REG32(TSTR_REGOFS)
#define TSTR_STR2      0x04  /* TCNT2 performs count operations */
#define TSTR_STR1      0x02  /* TCNT1 performs count operations */
#define TSTR_STR0      0x01  /* TCNT0 performs count operations */
#define TSTR_STR(n)    (1 << (n))

/* Timer Constant Register - TCOR0, TCOR1, TCOR2 */
#define TCOR_REGOFS(n) (0xD80008 + ((n)*12)) /* offset */
#define TCOR(n)        P4_REG32(TCOR_REGOFS(n))
#define TCOR_A7(n)     A7_REG32(TCOR_REGOFS(n))
#define TCOR0          TCOR(0)
#define TCOR1          TCOR(1)
#define TCOR2          TCOR(2)
#define TCOR0_A7       TCOR_A7(0)
#define TCOR1_A7       TCOR_A7(1)
#define TCOR2_A7       TCOR_A7(2)

/* Timer Counter Register - TCNT0, TCNT1, TCNT2 */
#define TCNT_REGOFS(n) (0xD8000C + ((n)*12)) /* offset */
#define TCNT(n)        P4_REG32(TCNT_REGOFS(n))
#define TCNT_A7(n)     A7_REG32(TCNT_REGOFS(n))
#define TCNT0          TCNT(0)
#define TCNT1          TCNT(1)
#define TCNT2          TCNT(2)
#define TCNT0_A7       TCNT_A7(0)
#define TCNT1_A7       TCNT_A7(1)
#define TCNT2_A7       TCNT_A7(2)

/* Timer Control Register (half) - TCR0, TCR1, TCR2 */
#define TCR_REGOFS(n)  (0xD80010 + ((n)*12)) /* offset */
#define TCR(n)         P4_REG32(TCR_REGOFS(n))
#define TCR_A7(n)      A7_REG32(TCR_REGOFS(n))
#define TCR0           TCR(0)
#define TCR1           TCR(1)
#define TCR2           TCR(2)
#define TCR0_A7        TCR_A7(0)
#define TCR1_A7        TCR_A7(1)
#define TCR2_A7        TCR_A7(2)

#define TCR2_ICPF       0x200  /* Input Capture Interrupt Flag
                                        (1 - input capture has occured) */
#define TCR_UNF         0x100  /* Underflow flag */
#define TCR2_ICPE       0x0C0  /* Input Capture Control: */
#define TCR2_ICPE_DIS   0x000  /*   Input Capture function is not used*/
#define TCR2_ICPE_NOINT 0x080  /*   Input Capture function is used, but
                                           input capture interrupt is not
                                           enabled */
#define TCR2_ICPE_INT   0x0C0  /*   Input Capture function is used,
                                           input capture interrupt enabled */
#define TCR_UNIE        0x020  /* Underflow Interrupt Control
                                         (1 - underflow interrupt enabled) */
#define TCR_CKEG        0x018  /* Clock Edge selection: */
#define TCR_CKEG_RAISE  0x000  /*   Count/capture on rising edge */
#define TCR_CKEG_FALL   0x008  /*   Count/capture on falling edge */
#define TCR_CKEG_BOTH   0x018  /*   Count/capture on both rising and
                                           falling edges */
#define TCR_TPSC         0x007  /* Timer prescaler */
#define TCR_TPSC_DIV4    0x000  /*   Counts on peripheral clock/4 */
#define TCR_TPSC_DIV16   0x001  /*   Counts on peripheral clock/16 */
#define TCR_TPSC_DIV64   0x002  /*   Counts on peripheral clock/64 */
#define TCR_TPSC_DIV256  0x003  /*   Counts on peripheral clock/256 */
#define TCR_TPSC_DIV1024 0x004  /*   Counts on peripheral clock/1024 */
#define TCR_TPSC_RTC     0x006  /*   Counts on on-chip RTC output clk*/
#define TCR_TPSC_EXT     0x007  /*   Counts on external clock */

/* Input Capture Register (read-only) - TCPR2 */
#define TCPR2_REGOFS   0xD8002C /* offset */
#define TCPR2          P4_REG32(TCPR2_REGOFS)
#define TCPR2_A7       A7_REG32(TCPR2_REGOFS)

/*
 * Bus State Controller - BSC
 */
/* Bus Control Register 1 - BCR1 */
#define BCR1_REGOFS    0x800000 /* offset */
#define BCR1           P4_REG32(BCR1_REGOFS)
#define BCR1_A7        A7_REG32(BCR1_REGOFS)
#define BCR1_ENDIAN    0x80000000 /* Endianness (1 - little endian) */
#define BCR1_MASTER    0x40000000 /* Master/Slave mode (1-master) */
#define BCR1_A0MPX     0x20000000 /* Area 0 Memory Type (0-SRAM,1-MPX)*/
#define BCR1_IPUP      0x02000000 /* Input Pin Pull-up Control:
                                              0 - pull-up resistor is on for
                                                  control input pins
                                              1 - pull-up resistor is off */
#define BCR1_OPUP      0x01000000 /* Output Pin Pull-up Control:
                                              0 - pull-up resistor is on for
                                                  control output pins
                                              1 - pull-up resistor is off */
#define BCR1_A1MBC     0x00200000 /* Area 1 SRAM Byte Control Mode:
                                              0 - Area 1 SRAM is set to
                                                  normal mode
                                              1 - Area 1 SRAM is set to byte
                                                  control mode */
#define BCR1_A4MBC     0x00100000 /* Area 4 SRAM Byte Control Mode:
                                              0 - Area 4 SRAM is set to
                                                  normal mode
                                              1 - Area 4 SRAM is set to byte
                                                  control mode */
#define BCR1_BREQEN    0x00080000 /* BREQ Enable:
                                              0 - External requests are  not
                                                  accepted
                                              1 - External requests are
                                                  accepted */
#define BCR1_PSHR      0x00040000 /* Partial Sharing Bit:
                                              0 - Master Mode
                                              1 - Partial-sharing Mode */
#define BCR1_MEMMPX    0x00020000 /* Area 1 to 6 MPX Interface:
                                              0 - SRAM/burst ROM interface
                                              1 - MPX interface */
#define BCR1_HIZMEM    0x00008000 /* High Impendance Control. Specifies
                                            the state of A[25:0], BS\, CSn\,
                                            RD/WR\, CE2A\, CE2B\ in standby
                                            mode and when bus is released:
                                              0 - signals go to High-Z mode
                                              1 - signals driven */
#define BCR1_HIZCNT    0x00004000 /* High Impendance Control. Specifies
                                            the state of the RAS\, RAS2\, WEn\,
                                            CASn\, DQMn, RD\, CASS\, FRAME\,
                                            RD2\ signals in standby mode and
                                            when bus is released:
                                              0 - signals go to High-Z mode
                                              1 - signals driven */
#define BCR1_A0BST     0x00003800 /* Area 0 Burst ROM Control */
#define BCR1_A0BST_SRAM    0x0000 /*   Area 0 accessed as SRAM i/f */
#define BCR1_A0BST_ROM4    0x0800 /*   Area 0 accessed as burst ROM
                                              interface, 4 cosequtive access*/
#define BCR1_A0BST_ROM8    0x1000 /*   Area 0 accessed as burst ROM
                                              interface, 8 cosequtive access*/
#define BCR1_A0BST_ROM16   0x1800 /*   Area 0 accessed as burst ROM
                                              interface, 16 cosequtive access*/
#define BCR1_A0BST_ROM32   0x2000 /*   Area 0 accessed as burst ROM
                                              interface, 32 cosequtive access*/

#define BCR1_A5BST     0x00000700 /* Area 5 Burst ROM Control */
#define BCR1_A5BST_SRAM    0x0000 /*   Area 5 accessed as SRAM i/f */
#define BCR1_A5BST_ROM4    0x0100 /*   Area 5 accessed as burst ROM
                                              interface, 4 cosequtive access*/
#define BCR1_A5BST_ROM8    0x0200 /*   Area 5 accessed as burst ROM
                                              interface, 8 cosequtive access*/
#define BCR1_A5BST_ROM16   0x0300 /*   Area 5 accessed as burst ROM
                                              interface, 16 cosequtive access*/
#define BCR1_A5BST_ROM32   0x0400 /*   Area 5 accessed as burst ROM
                                              interface, 32 cosequtive access*/

#define BCR1_A6BST     0x000000E0 /* Area 6 Burst ROM Control */
#define BCR1_A6BST_SRAM    0x0000 /*   Area 6 accessed as SRAM i/f */
#define BCR1_A6BST_ROM4    0x0020 /*   Area 6 accessed as burst ROM
                                              interface, 4 cosequtive access*/
#define BCR1_A6BST_ROM8    0x0040 /*   Area 6 accessed as burst ROM
                                              interface, 8 cosequtive access*/
#define BCR1_A6BST_ROM16   0x0060 /*   Area 6 accessed as burst ROM
                                              interface, 16 cosequtive access*/
#define BCR1_A6BST_ROM32   0x0080 /*   Area 6 accessed as burst ROM
                                              interface, 32 cosequtive access*/

#define BCR1_DRAMTP        0x001C /* Area 2 and 3 Memory Type */
#define BCR1_DRAMTP_2SRAM_3SRAM   0x0000 /* Area 2 and 3 are SRAM or MPX
                                                   interface. */
#define BCR1_DRAMTP_2SRAM_3SDRAM  0x0008 /* Area 2 - SRAM/MPX, Area 3 -
                                                   synchronous DRAM */
#define BCR1_DRAMTP_2SDRAM_3SDRAM 0x000C /* Area 2 and 3 are synchronous
                                                   DRAM interface */
#define BCR1_DRAMTP_2SRAM_3DRAM   0x0010 /* Area 2 - SRAM/MPX, Area 3 -
                                                   DRAM interface */
#define BCR1_DRAMTP_2DRAM_3DRAM   0x0014 /* Area 2 and 3 are DRAM
                                                   interface */

#define BCR1_A56PCM    0x00000001 /* Area 5 and 6 Bus Type:
                                              0 - SRAM interface
                                              1 - PCMCIA interface */

/* Bus Control Register 2 (half) - BCR2 */
#define BCR2_REGOFS    0x800004 /* offset */
#define BCR2           P4_REG32(BCR2_REGOFS)
#define BCR2_A7        A7_REG32(BCR2_REGOFS)

#define BCR2_A0SZ      0xC000 /* Area 0 Bus Width */
#define BCR2_A0SZ_S    14
#define BCR2_A6SZ      0x3000 /* Area 6 Bus Width */
#define BCR2_A6SZ_S    12
#define BCR2_A5SZ      0x0C00 /* Area 5 Bus Width */
#define BCR2_A5SZ_S    10
#define BCR2_A4SZ      0x0300 /* Area 4 Bus Width */
#define BCR2_A4SZ_S    8
#define BCR2_A3SZ      0x00C0 /* Area 3 Bus Width */
#define BCR2_A3SZ_S    6
#define BCR2_A2SZ      0x0030 /* Area 2 Bus Width */
#define BCR2_A2SZ_S    4
#define BCR2_A1SZ      0x000C /* Area 1 Bus Width */
#define BCR2_A1SZ_S    2
#define BCR2_SZ_64     0 /* 64 bits */
#define BCR2_SZ_8      1 /* 8 bits */
#define BCR2_SZ_16     2 /* 16 bits */
#define BCR2_SZ_32     3 /* 32 bits */
#define BCR2_PORTEN    0x0001 /* Port Function Enable :
                                          0 - D51-D32 are not used as a port
                                          1 - D51-D32 are used as a port */

/* Wait Control Register 1 - WCR1 */
#define WCR1_REGOFS    0x800008 /* offset */
#define WCR1           P4_REG32(WCR1_REGOFS)
#define WCR1_A7        A7_REG32(WCR1_REGOFS)
#define WCR1_DMAIW     0x70000000 /* DACK Device Inter-Cycle Idle
                                            specification */
#define WCR1_DMAIW_S   28
#define WCR1_A6IW      0x07000000 /* Area 6 Inter-Cycle Idle spec. */
#define WCR1_A6IW_S    24
#define WCR1_A5IW      0x00700000 /* Area 5 Inter-Cycle Idle spec. */
#define WCR1_A5IW_S    20
#define WCR1_A4IW      0x00070000 /* Area 4 Inter-Cycle Idle spec. */
#define WCR1_A4IW_S    16
#define WCR1_A3IW      0x00007000 /* Area 3 Inter-Cycle Idle spec. */
#define WCR1_A3IW_S    12
#define WCR1_A2IW      0x00000700 /* Area 2 Inter-Cycle Idle spec. */
#define WCR1_A2IW_S    8
#define WCR1_A1IW      0x00000070 /* Area 1 Inter-Cycle Idle spec. */
#define WCR1_A1IW_S    4
#define WCR1_A0IW      0x00000007 /* Area 0 Inter-Cycle Idle spec. */
#define WCR1_A0IW_S    0

/* Wait Control Register 2 - WCR2 */
#define WCR2_REGOFS    0x80000C /* offset */
#define WCR2           P4_REG32(WCR2_REGOFS)
#define WCR2_A7        A7_REG32(WCR2_REGOFS)

#define WCR2_A6W       0xE0000000 /* Area 6 Wait Control */
#define WCR2_A6W_S     29
#define WCR2_A6B       0x1C000000 /* Area 6 Burst Pitch */
#define WCR2_A6B_S     26
#define WCR2_A5W       0x03800000 /* Area 5 Wait Control */
#define WCR2_A5W_S     23
#define WCR2_A5B       0x00700000 /* Area 5 Burst Pitch */
#define WCR2_A5B_S     20
#define WCR2_A4W       0x000E0000 /* Area 4 Wait Control */
#define WCR2_A4W_S     17
#define WCR2_A3W       0x0000E000 /* Area 3 Wait Control */
#define WCR2_A3W_S     13
#define WCR2_A2W       0x00000E00 /* Area 2 Wait Control */
#define WCR2_A2W_S     9
#define WCR2_A1W       0x000001C0 /* Area 1 Wait Control */
#define WCR2_A1W_S     6
#define WCR2_A0W       0x00000038 /* Area 0 Wait Control */
#define WCR2_A0W_S     3
#define WCR2_A0B       0x00000007 /* Area 0 Burst Pitch */
#define WCR2_A0B_S     0

#define WCR2_WS0       0   /* 0 wait states inserted */
#define WCR2_WS1       1   /* 1 wait states inserted */
#define WCR2_WS2       2   /* 2 wait states inserted */
#define WCR2_WS3       3   /* 3 wait states inserted */
#define WCR2_WS6       4   /* 6 wait states inserted */
#define WCR2_WS9       5   /* 9 wait states inserted */
#define WCR2_WS12      6   /* 12 wait states inserted */
#define WCR2_WS15      7   /* 15 wait states inserted */

#define WCR2_BPWS0     0   /* 0 wait states inserted from 2nd access */
#define WCR2_BPWS1     1   /* 1 wait states inserted from 2nd access */
#define WCR2_BPWS2     2   /* 2 wait states inserted from 2nd access */
#define WCR2_BPWS3     3   /* 3 wait states inserted from 2nd access */
#define WCR2_BPWS4     4   /* 4 wait states inserted from 2nd access */
#define WCR2_BPWS5     5   /* 5 wait states inserted from 2nd access */
#define WCR2_BPWS6     6   /* 6 wait states inserted from 2nd access */
#define WCR2_BPWS7     7   /* 7 wait states inserted from 2nd access */

/* DRAM CAS\ Assertion Delay (area 3,2) */
#define WCR2_DRAM_CAS_ASW1   0   /* 1 cycle */
#define WCR2_DRAM_CAS_ASW2   1   /* 2 cycles */
#define WCR2_DRAM_CAS_ASW3   2   /* 3 cycles */
#define WCR2_DRAM_CAS_ASW4   3   /* 4 cycles */
#define WCR2_DRAM_CAS_ASW7   4   /* 7 cycles */
#define WCR2_DRAM_CAS_ASW10  5   /* 10 cycles */
#define WCR2_DRAM_CAS_ASW13  6   /* 13 cycles */
#define WCR2_DRAM_CAS_ASW16  7   /* 16 cycles */

/* SDRAM CAS\ Latency Cycles */
#define WCR2_SDRAM_CAS_LAT1  1   /* 1 cycle */
#define WCR2_SDRAM_CAS_LAT2  2   /* 2 cycles */
#define WCR2_SDRAM_CAS_LAT3  3   /* 3 cycles */
#define WCR2_SDRAM_CAS_LAT4  4   /* 4 cycles */
#define WCR2_SDRAM_CAS_LAT5  5   /* 5 cycles */

/* Wait Control Register 3 - WCR3 */
#define WCR3_REGOFS    0x800010 /* offset */
#define WCR3           P4_REG32(WCR3_REGOFS)
#define WCR3_A7        A7_REG32(WCR3_REGOFS)

#define WCR3_A6S       0x04000000 /* Area 6 Write Strobe Setup time */
#define WCR3_A6H       0x03000000 /* Area 6 Data Hold Time */
#define WCR3_A6H_S     24
#define WCR3_A5S       0x00400000 /* Area 5 Write Strobe Setup time */
#define WCR3_A5H       0x00300000 /* Area 5 Data Hold Time */
#define WCR3_A5H_S     20
#define WCR3_A4S       0x00040000 /* Area 4 Write Strobe Setup time */
#define WCR3_A4H       0x00030000 /* Area 4 Data Hold Time */
#define WCR3_A4H_S     16
#define WCR3_A3S       0x00004000 /* Area 3 Write Strobe Setup time */
#define WCR3_A3H       0x00003000 /* Area 3 Data Hold Time */
#define WCR3_A3H_S     12
#define WCR3_A2S       0x00000400 /* Area 2 Write Strobe Setup time */
#define WCR3_A2H       0x00000300 /* Area 2 Data Hold Time */
#define WCR3_A2H_S     8
#define WCR3_A1S       0x00000040 /* Area 1 Write Strobe Setup time */
#define WCR3_A1H       0x00000030 /* Area 1 Data Hold Time */
#define WCR3_A1H_S     4
#define WCR3_A0S       0x00000004 /* Area 0 Write Strobe Setup time */
#define WCR3_A0H       0x00000003 /* Area 0 Data Hold Time */
#define WCR3_A0H_S     0

#define WCR3_DHWS_0    0  /* 0 wait states data hold time */
#define WCR3_DHWS_1    1  /* 1 wait states data hold time */
#define WCR3_DHWS_2    2  /* 2 wait states data hold time */
#define WCR3_DHWS_3    3  /* 3 wait states data hold time */

#define MCR_REGOFS     0x800014 /* offset */
#define MCR            P4_REG32(MCR_REGOFS)
#define MCR_A7         A7_REG32(MCR_REGOFS)

#define MCR_RASD       0x80000000 /* RAS Down mode */
#define MCR_MRSET      0x40000000 /* SDRAM Mode Register Set */
#define MCR_PALL       0x00000000 /* SDRAM Precharge All cmd. Mode */
#define MCR_TRC        0x38000000 /* RAS Precharge Time at End of
                                            Refresh: */
#define MCR_TRC_0      0x00000000 /*    0 */
#define MCR_TRC_3      0x08000000 /*    3 */
#define MCR_TRC_6      0x10000000 /*    6 */
#define MCR_TRC_9      0x18000000 /*    9 */
#define MCR_TRC_12     0x20000000 /*    12 */
#define MCR_TRC_15     0x28000000 /*    15 */
#define MCR_TRC_18     0x30000000 /*    18 */
#define MCR_TRC_21     0x38000000 /*    21 */

#define MCR_TCAS       0x00800000 /* CAS Negation Period */
#define MCR_TCAS_1     0x00000000 /*    1 */
#define MCR_TCAS_2     0x00800000 /*    2 */

#define MCR_TPC        0x00380000 /* DRAM: RAS Precharge Period
                                            SDRAM: minimum number of cycles
                                            until the next bank active cmd
                                            is output after precharging */
#define MCR_TPC_S      19
#define MCR_TPC_SDRAM_1 0x00000000 /* 1 cycle */
#define MCR_TPC_SDRAM_2 0x00080000 /* 2 cycles */
#define MCR_TPC_SDRAM_3 0x00100000 /* 3 cycles */
#define MCR_TPC_SDRAM_4 0x00180000 /* 4 cycles */
#define MCR_TPC_SDRAM_5 0x00200000 /* 5 cycles */
#define MCR_TPC_SDRAM_6 0x00280000 /* 6 cycles */
#define MCR_TPC_SDRAM_7 0x00300000 /* 7 cycles */
#define MCR_TPC_SDRAM_8 0x00380000 /* 8 cycles */

#define MCR_RCD        0x00030000 /* DRAM: RAS-CAS Assertion Delay time
                                            SDRAM: bank active-read/write cmd
                                            delay time */
#define MCR_RCD_DRAM_2  0x00000000 /* DRAM delay 2 clocks */
#define MCR_RCD_DRAM_3  0x00010000 /* DRAM delay 3 clocks */
#define MCR_RCD_DRAM_4  0x00020000 /* DRAM delay 4 clocks */
#define MCR_RCD_DRAM_5  0x00030000 /* DRAM delay 5 clocks */
#define MCR_RCD_SDRAM_2 0x00010000 /* DRAM delay 2 clocks */
#define MCR_RCD_SDRAM_3 0x00020000 /* DRAM delay 3 clocks */
#define MCR_RCD_SDRAM_4 0x00030000 /* DRAM delay 4 clocks */

#define MCR_TRWL       0x0000E000 /* SDRAM Write Precharge Delay */
#define MCR_TRWL_1     0x00000000 /*    1 */
#define MCR_TRWL_2     0x00002000 /*    2 */
#define MCR_TRWL_3     0x00004000 /*    3 */
#define MCR_TRWL_4     0x00006000 /*    4 */
#define MCR_TRWL_5     0x00008000 /*    5 */

#define MCR_TRAS       0x00001C00 /* DRAM: CAS-Before-RAS Refresh RAS
                                            asserting period
                                            SDRAM: Command interval after
                                            synchronous DRAM refresh */
#define MCR_TRAS_DRAM_2         0x00000000 /* 2 */
#define MCR_TRAS_DRAM_3         0x00000400 /* 3 */
#define MCR_TRAS_DRAM_4         0x00000800 /* 4 */
#define MCR_TRAS_DRAM_5         0x00000C00 /* 5 */
#define MCR_TRAS_DRAM_6         0x00001000 /* 6 */
#define MCR_TRAS_DRAM_7         0x00001400 /* 7 */
#define MCR_TRAS_DRAM_8         0x00001800 /* 8 */
#define MCR_TRAS_DRAM_9         0x00001C00 /* 9 */

#define MCR_TRAS_SDRAM_TRC_4    0x00000000 /* 4 + TRC */
#define MCR_TRAS_SDRAM_TRC_5    0x00000400 /* 5 + TRC */
#define MCR_TRAS_SDRAM_TRC_6    0x00000800 /* 6 + TRC */
#define MCR_TRAS_SDRAM_TRC_7    0x00000C00 /* 7 + TRC */
#define MCR_TRAS_SDRAM_TRC_8    0x00001000 /* 8 + TRC */
#define MCR_TRAS_SDRAM_TRC_9    0x00001400 /* 9 + TRC */
#define MCR_TRAS_SDRAM_TRC_10   0x00001800 /* 10 + TRC */
#define MCR_TRAS_SDRAM_TRC_11   0x00001C00 /* 11 + TRC */

#define MCR_BE         0x00000200 /* Burst Enable */
#define MCR_SZ         0x00000180 /* Memory Data Size */
#define MCR_SZ_64      0x00000000 /*    64 bits */
#define MCR_SZ_16      0x00000100 /*    16 bits */
#define MCR_SZ_32      0x00000180 /*    32 bits */

#define MCR_AMX        0x00000078 /* Address Multiplexing */
#define MCR_AMX_S      3
#define MCR_AMX_DRAM_8BIT_COL    0x00000000 /* 8-bit column addr */
#define MCR_AMX_DRAM_9BIT_COL    0x00000008 /* 9-bit column addr */
#define MCR_AMX_DRAM_10BIT_COL   0x00000010 /* 10-bit column addr */
#define MCR_AMX_DRAM_11BIT_COL   0x00000018 /* 11-bit column addr */
#define MCR_AMX_DRAM_12BIT_COL   0x00000020 /* 12-bit column addr */
/* See SH7750 Hardware Manual for SDRAM address multiplexor selection */

#define MCR_RFSH       0x00000004 /* Refresh Control */
#define MCR_RMODE      0x00000002 /* Refresh Mode: */
#define MCR_RMODE_NORMAL 0x00000000 /* Normal Refresh Mode */
#define MCR_RMODE_SELF   0x00000002 /* Self-Refresh Mode */
#define MCR_RMODE_EDO    0x00000001 /* EDO Mode */

/* SDRAM Mode Set address */
#define SDRAM_MODE_A2_BASE  0xFF900000
#define SDRAM_MODE_A3_BASE  0xFF940000
#define SDRAM_MODE_A2_32BIT(x) (SDRAM_MODE_A2_BASE + ((x) << 2))
#define SDRAM_MODE_A3_32BIT(x) (SDRAM_MODE_A3_BASE + ((x) << 2))
#define SDRAM_MODE_A2_64BIT(x) (SDRAM_MODE_A2_BASE + ((x) << 3))
#define SDRAM_MODE_A3_64BIT(x) (SDRAM_MODE_A3_BASE + ((x) << 3))


/* PCMCIA Control Register (half) - PCR */
#define PCR_REGOFS     0x800018 /* offset */
#define PCR            P4_REG32(PCR_REGOFS)
#define PCR_A7         A7_REG32(PCR_REGOFS)

#define PCR_A5PCW      0xC000 /* Area 5 PCMCIA Wait - Number of wait
                                        states to be added to the number of
                                        waits specified by WCR2 in a low-speed
                                        PCMCIA wait cycle */
#define PCR_A5PCW_0    0x0000 /*    0 waits inserted */
#define PCR_A5PCW_15   0x4000 /*    15 waits inserted */
#define PCR_A5PCW_30   0x8000 /*    30 waits inserted */
#define PCR_A5PCW_50   0xC000 /*    50 waits inserted */

#define PCR_A6PCW      0x3000 /* Area 6 PCMCIA Wait - Number of wait
                                        states to be added to the number of
                                        waits specified by WCR2 in a low-speed
                                        PCMCIA wait cycle */
#define PCR_A6PCW_0    0x0000 /*    0 waits inserted */
#define PCR_A6PCW_15   0x1000 /*    15 waits inserted */
#define PCR_A6PCW_30   0x2000 /*    30 waits inserted */
#define PCR_A6PCW_50   0x3000 /*    50 waits inserted */

#define PCR_A5TED      0x0E00 /* Area 5 Address-OE\/WE\ Assertion Delay,
                                        delay time from address output to
                                        OE\/WE\ assertion on the connected
                                        PCMCIA interface */
#define PCR_A5TED_S    9
#define PCR_A6TED      0x01C0 /* Area 6 Address-OE\/WE\ Assertion Delay*/
#define PCR_A6TED_S    6

#define PCR_TED_0WS    0     /* 0 Waits inserted */
#define PCR_TED_1WS    1     /* 1 Waits inserted */
#define PCR_TED_2WS    2     /* 2 Waits inserted */
#define PCR_TED_3WS    3     /* 3 Waits inserted */
#define PCR_TED_6WS    4     /* 6 Waits inserted */
#define PCR_TED_9WS    5     /* 9 Waits inserted */
#define PCR_TED_12WS   6     /* 12 Waits inserted */
#define PCR_TED_15WS   7     /* 15 Waits inserted */

#define PCR_A5TEH      0x0038 /* Area 5 OE\/WE\ Negation Address delay,
                                        address hold delay time from OE\/WE\
                                        negation in a write on the connected
                                        PCMCIA interface */
#define PCR_A5TEH_S    3

#define PCR_A6TEH      0x0007 /* Area 6 OE\/WE\ Negation Address delay*/
#define PCR_A6TEH_S    0

#define PCR_TEH_0WS    0     /* 0 Waits inserted */
#define PCR_TEH_1WS    1     /* 1 Waits inserted */
#define PCR_TEH_2WS    2     /* 2 Waits inserted */
#define PCR_TEH_3WS    3     /* 3 Waits inserted */
#define PCR_TEH_6WS    4     /* 6 Waits inserted */
#define PCR_TEH_9WS    5     /* 9 Waits inserted */
#define PCR_TEH_12WS   6     /* 12 Waits inserted */
#define PCR_TEH_15WS   7     /* 15 Waits inserted */

/* Refresh Timer Control/Status Register (half) - RTSCR */
#define RTCSR_REGOFS   0x80001C /* offset */
#define RTCSR          P4_REG32(RTCSR_REGOFS)
#define RTCSR_A7       A7_REG32(RTCSR_REGOFS)

#define RTCSR_KEY      0xA500 /* RTCSR write key */
#define RTCSR_CMF      0x0080 /* Compare-Match Flag (indicates a
                                        match between the refresh timer
                                        counter and refresh time constant) */
#define RTCSR_CMIE     0x0040 /* Compare-Match Interrupt Enable */
#define RTCSR_CKS      0x0038 /* Refresh Counter Clock Selects */
#define RTCSR_CKS_DIS          0x0000 /* Clock Input Disabled */
#define RTCSR_CKS_CKIO_DIV4    0x0008 /* Bus Clock / 4 */
#define RTCSR_CKS_CKIO_DIV16   0x0010 /* Bus Clock / 16 */
#define RTCSR_CKS_CKIO_DIV64   0x0018 /* Bus Clock / 64 */
#define RTCSR_CKS_CKIO_DIV256  0x0020 /* Bus Clock / 256 */
#define RTCSR_CKS_CKIO_DIV1024 0x0028 /* Bus Clock / 1024 */
#define RTCSR_CKS_CKIO_DIV2048 0x0030 /* Bus Clock / 2048 */
#define RTCSR_CKS_CKIO_DIV4096 0x0038 /* Bus Clock / 4096 */

#define RTCSR_OVF      0x0004 /* Refresh Count Overflow Flag */
#define RTCSR_OVIE     0x0002 /* Refresh Count Overflow Interrupt
                                        Enable */
#define RTCSR_LMTS     0x0001 /* Refresh Count Overflow Limit Select */
#define RTCSR_LMTS_1024 0x0000 /* Count Limit is 1024 */
#define RTCSR_LMTS_512  0x0001 /* Count Limit is 512 */

/* Refresh Timer Counter (half) - RTCNT */
#define RTCNT_REGOFS   0x800020 /* offset */
#define RTCNT          P4_REG32(RTCNT_REGOFS)
#define RTCNT_A7       A7_REG32(RTCNT_REGOFS)

#define RTCNT_KEY      0xA500 /* RTCNT write key */

/* Refresh Time Constant Register (half) - RTCOR */
#define RTCOR_REGOFS   0x800024 /* offset */
#define RTCOR          P4_REG32(RTCOR_REGOFS)
#define RTCOR_A7       A7_REG32(RTCOR_REGOFS)

#define RTCOR_KEY      0xA500 /* RTCOR write key */

/* Refresh Count Register (half) - RFCR */
#define RFCR_REGOFS    0x800028 /* offset */
#define RFCR           P4_REG32(RFCR_REGOFS)
#define RFCR_A7        A7_REG32(RFCR_REGOFS)

#define RFCR_KEY       0xA400 /* RFCR write key */

/*
 * Direct Memory Access Controller (DMAC)
 */

/* DMA Source Address Register - SAR0, SAR1, SAR2, SAR3 */
#define SAR_REGOFS(n)  (0xA00000 + ((n)*16)) /* offset */
#define SAR(n)         P4_REG32(SAR_REGOFS(n))
#define SAR_A7(n)      A7_REG32(SAR_REGOFS(n))
#define SAR0           SAR(0)
#define SAR1           SAR(1)
#define SAR2           SAR(2)
#define SAR3           SAR(3)
#define SAR0_A7        SAR_A7(0)
#define SAR1_A7        SAR_A7(1)
#define SAR2_A7        SAR_A7(2)
#define SAR3_A7        SAR_A7(3)

/* DMA Destination Address Register - DAR0, DAR1, DAR2, DAR3 */
#define DAR_REGOFS(n)  (0xA00004 + ((n)*16)) /* offset */
#define DAR(n)         P4_REG32(DAR_REGOFS(n))
#define DAR_A7(n)      A7_REG32(DAR_REGOFS(n))
#define DAR0           DAR(0)
#define DAR1           DAR(1)
#define DAR2           DAR(2)
#define DAR3           DAR(3)
#define DAR0_A7        DAR_A7(0)
#define DAR1_A7        DAR_A7(1)
#define DAR2_A7        DAR_A7(2)
#define DAR3_A7        DAR_A7(3)

/* DMA Transfer Count Register - DMATCR0, DMATCR1, DMATCR2, DMATCR3 */
#define DMATCR_REGOFS(n)  (0xA00008 + ((n)*16)) /* offset */
#define DMATCR(n)      P4_REG32(DMATCR_REGOFS(n))
#define DMATCR_A7(n)   A7_REG32(DMATCR_REGOFS(n))
#define DMATCR0_P4     DMATCR(0)
#define DMATCR1_P4     DMATCR(1)
#define DMATCR2_P4     DMATCR(2)
#define DMATCR3_P4     DMATCR(3)
#define DMATCR0_A7     DMATCR_A7(0)
#define DMATCR1_A7     DMATCR_A7(1)
#define DMATCR2_A7     DMATCR_A7(2)
#define DMATCR3_A7     DMATCR_A7(3)

/* DMA Channel Control Register - CHCR0, CHCR1, CHCR2, CHCR3 */
#define CHCR_REGOFS(n)  (0xA0000C + ((n)*16)) /* offset */
#define CHCR(n)        P4_REG32(CHCR_REGOFS(n))
#define CHCR_A7(n)     A7_REG32(CHCR_REGOFS(n))
#define CHCR0          CHCR(0)
#define CHCR1          CHCR(1)
#define CHCR2          CHCR(2)
#define CHCR3          CHCR(3)
#define CHCR0_A7       CHCR_A7(0)
#define CHCR1_A7       CHCR_A7(1)
#define CHCR2_A7       CHCR_A7(2)
#define CHCR3_A7       CHCR_A7(3)

#define CHCR_SSA       0xE0000000 /* Source Address Space Attribute */
#define CHCR_SSA_PCMCIA  0x00000000 /* Reserved in PCMCIA access */
#define CHCR_SSA_DYNBSZ  0x20000000 /* Dynamic Bus Sizing I/O space */
#define CHCR_SSA_IO8     0x40000000 /* 8-bit I/O space */
#define CHCR_SSA_IO16    0x60000000 /* 16-bit I/O space */
#define CHCR_SSA_CMEM8   0x80000000 /* 8-bit common memory space */
#define CHCR_SSA_CMEM16  0xA0000000 /* 16-bit common memory space */
#define CHCR_SSA_AMEM8   0xC0000000 /* 8-bit attribute memory space */
#define CHCR_SSA_AMEM16  0xE0000000 /* 16-bit attribute memory space */

#define CHCR_STC       0x10000000 /* Source Address Wait Control Select,
                                            specifies CS5 or CS6 space wait
                                            control for PCMCIA access */

#define CHCR_DSA       0x0E000000 /* Source Address Space Attribute */
#define CHCR_DSA_PCMCIA  0x00000000 /* Reserved in PCMCIA access */
#define CHCR_DSA_DYNBSZ  0x02000000 /* Dynamic Bus Sizing I/O space */
#define CHCR_DSA_IO8     0x04000000 /* 8-bit I/O space */
#define CHCR_DSA_IO16    0x06000000 /* 16-bit I/O space */
#define CHCR_DSA_CMEM8   0x08000000 /* 8-bit common memory space */
#define CHCR_DSA_CMEM16  0x0A000000 /* 16-bit common memory space */
#define CHCR_DSA_AMEM8   0x0C000000 /* 8-bit attribute memory space */
#define CHCR_DSA_AMEM16  0x0E000000 /* 16-bit attribute memory space */

#define CHCR_DTC       0x01000000 /* Destination Address Wait Control
                                            Select, specifies CS5 or CS6
                                            space wait control for PCMCIA
                                            access */

#define CHCR_DS        0x00080000 /* DREQ\ Select : */
#define CHCR_DS_LOWLVL 0x00000000 /*     Low Level Detection */
#define CHCR_DS_FALL   0x00080000 /*     Falling Edge Detection */

#define CHCR_RL        0x00040000 /* Request Check Level: */
#define CHCR_RL_ACTH   0x00000000 /*     DRAK is an active high out */
#define CHCR_RL_ACTL   0x00040000 /*     DRAK is an active low out */

#define CHCR_AM        0x00020000 /* Acknowledge Mode: */
#define CHCR_AM_RD     0x00000000 /*     DACK is output in read cycle */
#define CHCR_AM_WR     0x00020000 /*     DACK is output in write cycle*/

#define CHCR_AL        0x00010000 /* Acknowledge Level: */
#define CHCR_AL_ACTH   0x00000000 /*     DACK is an active high out */
#define CHCR_AL_ACTL   0x00010000 /*     DACK is an active low out */

#define CHCR_DM        0x0000C000 /* Destination Address Mode: */
#define CHCR_DM_FIX    0x00000000 /*     Destination Addr Fixed */
#define CHCR_DM_INC    0x00004000 /*     Destination Addr Incremented */
#define CHCR_DM_DEC    0x00008000 /*     Destination Addr Decremented */

#define CHCR_SM        0x00003000 /* Source Address Mode: */
#define CHCR_SM_FIX    0x00000000 /*     Source Addr Fixed */
#define CHCR_SM_INC    0x00001000 /*     Source Addr Incremented */
#define CHCR_SM_DEC    0x00002000 /*     Source Addr Decremented */

#define CHCR_RS        0x00000F00 /* Request Source Select: */
#define CHCR_RS_ER_DA_EA_TO_EA   0x000 /* External Request, Dual Address
                                                 Mode (External Addr Space->
                                                 External Addr Space) */
#define CHCR_RS_ER_SA_EA_TO_ED   0x200 /* External Request, Single
                                                 Address Mode (External Addr
                                                 Space -> External Device) */
#define CHCR_RS_ER_SA_ED_TO_EA   0x300 /* External Request, Single
                                                 Address Mode, (External
                                                 Device -> External Addr
                                                 Space)*/
#define CHCR_RS_AR_EA_TO_EA      0x400 /* Auto-Request (External Addr
                                                 Space -> External Addr Space)*/

#define CHCR_RS_AR_EA_TO_OCP     0x500 /* Auto-Request (External Addr
                                                 Space -> On-chip Peripheral
                                                 Module) */
#define CHCR_RS_AR_OCP_TO_EA     0x600 /* Auto-Request (On-chip
                                                 Peripheral Module ->
                                                 External Addr Space */
#define CHCR_RS_SCITX_EA_TO_SC   0x800 /* SCI Transmit-Data-Empty intr
                                                 transfer request (external
                                                 address space -> SCTDR1) */
#define CHCR_RS_SCIRX_SC_TO_EA   0x900 /* SCI Receive-Data-Full intr
                                                 transfer request (SCRDR1 ->
                                                 External Addr Space) */
#define CHCR_RS_SCIFTX_EA_TO_SC  0xA00 /* SCIF Transmit-Data-Empty intr
                                                 transfer request (external
                                                 address space -> SCFTDR1) */
#define CHCR_RS_SCIFRX_SC_TO_EA  0xB00 /* SCIF Receive-Data-Full intr
                                                 transfer request (SCFRDR2 ->
                                                 External Addr Space) */
#define CHCR_RS_TMU2_EA_TO_EA    0xC00 /* TMU Channel 2 (input capture
                                                 interrupt), (external address
                                                 space -> external address
                                                 space) */
#define CHCR_RS_TMU2_EA_TO_OCP   0xD00 /* TMU Channel 2 (input capture
                                                 interrupt), (external address
                                                 space -> on-chip peripheral
                                                 module) */
#define CHCR_RS_TMU2_OCP_TO_EA   0xE00 /* TMU Channel 2 (input capture
                                                 interrupt), (on-chip
                                                 peripheral module -> external
                                                 address space) */

#define CHCR_TM        0x00000080 /* Transmit mode: */
#define CHCR_TM_CSTEAL 0x00000000 /*     Cycle Steal Mode */
#define CHCR_TM_BURST  0x00000080 /*     Burst Mode */

#define CHCR_TS        0x00000070 /* Transmit Size: */
#define CHCR_TS_QUAD   0x00000000 /*     Quadword Size (64 bits) */
#define CHCR_TS_BYTE   0x00000010 /*     Byte Size (8 bit) */
#define CHCR_TS_WORD   0x00000020 /*     Word Size (16 bit) */
#define CHCR_TS_LONG   0x00000030 /*     Longword Size (32 bit) */
#define CHCR_TS_BLOCK  0x00000040 /*     32-byte block transfer */

#define CHCR_IE        0x00000004 /* Interrupt Enable */
#define CHCR_TE        0x00000002 /* Transfer End */
#define CHCR_DE        0x00000001 /* DMAC Enable */

/* DMA Operation Register - DMAOR */
#define DMAOR_REGOFS   0xA00040 /* offset */
#define DMAOR          P4_REG32(DMAOR_REGOFS)
#define DMAOR_A7       A7_REG32(DMAOR_REGOFS)

#define DMAOR_DDT      0x00008000 /* On-Demand Data Transfer Mode */

#define DMAOR_PR       0x00000300 /* Priority Mode: */
#define DMAOR_PR_0123  0x00000000 /*     CH0 > CH1 > CH2 > CH3 */
#define DMAOR_PR_0231  0x00000100 /*     CH0 > CH2 > CH3 > CH1 */
#define DMAOR_PR_2013  0x00000200 /*     CH2 > CH0 > CH1 > CH3 */
#define DMAOR_PR_RR    0x00000300 /*     Round-robin mode */

#define DMAOR_COD      0x00000010 /* Check Overrun for DREQ\ */
#define DMAOR_AE       0x00000004 /* Address Error flag */
#define DMAOR_NMIF     0x00000002 /* NMI Flag */
#define DMAOR_DME      0x00000001 /* DMAC Master Enable */

/*
 * Serial Communication Interface - SCI
 * Serial Communication Interface with FIFO - SCIF
 */
/* SCI Receive Data Register (byte, read-only) - SCRDR1, SCFRDR2 */
#define SCRDR_REGOFS(n) ((n) == 1 ? 0xE00014 : 0xE80014) /* offset */
#define SCRDR(n)       P4_REG32(SCRDR_REGOFS(n))
#define SCRDR1         SCRDR(1)
#define SCRDR2         SCRDR(2)
#define SCRDR_A7(n)    A7_REG32(SCRDR_REGOFS(n))
#define SCRDR1_A7      SCRDR_A7(1)
#define SCRDR2_A7      SCRDR_A7(2)

/* SCI Transmit Data Register (byte) - SCTDR1, SCFTDR2 */
#define SCTDR_REGOFS(n) ((n) == 1 ? 0xE0000C : 0xE8000C) /* offset */
#define SCTDR(n)       P4_REG32(SCTDR_REGOFS(n))
#define SCTDR1         SCTDR(1)
#define SCTDR2         SCTDR(2)
#define SCTDR_A7(n)    A7_REG32(SCTDR_REGOFS(n))
#define SCTDR1_A7      SCTDR_A7(1)
#define SCTDR2_A7      SCTDR_A7(2)

/* SCI Serial Mode Register - SCSMR1(byte), SCSMR2(half) */
#define SCSMR_REGOFS(n) ((n) == 1 ? 0xE00000 : 0xE80000) /* offset */
#define SCSMR(n)       P4_REG32(SCSMR_REGOFS(n))
#define SCSMR1         SCSMR(1)
#define SCSMR2         SCSMR(2)
#define SCSMR_A7(n)    A7_REG32(SCSMR_REGOFS(n))
#define SCSMR1_A7      SCSMR_A7(1)
#define SCSMR2_A7      SCSMR_A7(2)

#define SCSMR1_CA       0x80 /* Communication Mode (C/A\): */
#define SCSMR1_CA_ASYNC 0x00 /*     Asynchronous Mode */
#define SCSMR1_CA_SYNC  0x80 /*     Synchronous Mode */
#define SCSMR_CHR       0x40 /* Character Length: */
#define SCSMR_CHR_8     0x00 /*     8-bit data */
#define SCSMR_CHR_7     0x40 /*     7-bit data */
#define SCSMR_PE        0x20 /* Parity Enable */
#define SCSMR_PM        0x10 /* Parity Mode: */
#define SCSMR_PM_EVEN   0x00 /*     Even Parity */
#define SCSMR_PM_ODD    0x10 /*     Odd Parity */
#define SCSMR_STOP      0x08 /* Stop Bit Length: */
#define SCSMR_STOP_1    0x00 /*     1 stop bit */
#define SCSMR_STOP_2    0x08 /*     2 stop bit */
#define SCSMR1_MP       0x04 /* Multiprocessor Mode */
#define SCSMR_CKS       0x03 /* Clock Select */
#define SCSMR_CKS_S     0
#define SCSMR_CKS_DIV1  0x00 /*     Periph clock */
#define SCSMR_CKS_DIV4  0x01 /*     Periph clock / 4 */
#define SCSMR_CKS_DIV16 0x02 /*     Periph clock / 16 */
#define SCSMR_CKS_DIV64 0x03 /*     Periph clock / 64 */

/* SCI Serial Control Register - SCSCR1(byte), SCSCR2(half) */
#define SCSCR_REGOFS(n) ((n) == 1 ? 0xE00008 : 0xE80008) /* offset */
#define SCSCR(n)       P4_REG32(SCSCR_REGOFS(n))
#define SCSCR1         SCSCR(1)
#define SCSCR2         SCSCR(2)
#define SCSCR_A7(n)    A7_REG32(SCSCR_REGOFS(n))
#define SCSCR1_A7      SCSCR_A7(1)
#define SCSCR2_A7      SCSCR_A7(2)

#define SCSCR_TIE      0x80 /* Transmit Interrupt Enable */
#define SCSCR_RIE      0x40 /* Receive Interrupt Enable */
#define SCSCR_TE       0x20 /* Transmit Enable */
#define SCSCR_RE       0x10 /* Receive Enable */
#define SCSCR1_MPIE    0x08 /* Multiprocessor Interrupt Enable */
#define SCSCR2_REIE    0x08 /* Receive Error Interrupt Enable */
#define SCSCR1_TEIE    0x04 /* Transmit End Interrupt Enable */
#define SCSCR1_CKE     0x03 /* Clock Enable: */
#define SCSCR_CKE_INTCLK            0x00 /* Use Internal Clock */
#define SCSCR_CKE_EXTCLK            0x02 /* Use External Clock from SCK*/
#define SCSCR1_CKE_ASYNC_SCK_CLKOUT 0x01 /* Use SCK as a clock output
                                                   in asynchronous mode */

/* SCI Serial Status Register - SCSSR1(byte), SCSSR2(half) */
#define SCSSR_REGOFS(n) ((n) == 1 ? 0xE00010 : 0xE80010) /* offset */
#define SCSSR(n)       P4_REG32(SCSSR_REGOFS(n))
#define SCSSR1         SCSSR(1)
#define SCSSR2         SCSSR(2)
#define SCSSR_A7(n)    A7_REG32(SCSSR_REGOFS(n))
#define SCSSR1_A7      SCSSR_A7(1)
#define SCSSR2_A7      SCSSR_A7(2)

#define SCSSR1_TDRE    0x80 /* Transmit Data Register Empty */
#define SCSSR1_RDRF    0x40 /* Receive Data Register Full */
#define SCSSR1_ORER    0x20 /* Overrun Error */
#define SCSSR1_FER     0x10 /* Framing Error */
#define SCSSR1_PER     0x08 /* Parity Error */
#define SCSSR1_TEND    0x04 /* Transmit End */
#define SCSSR1_MPB     0x02 /* Multiprocessor Bit */
#define SCSSR1_MPBT    0x01 /* Multiprocessor Bit Transfer */

#define SCSSR2_PERN    0xF000 /* Number of Parity Errors */
#define SCSSR2_PERN_S  12
#define SCSSR2_FERN    0x0F00 /* Number of Framing Errors */
#define SCSSR2_FERN_S  8
#define SCSSR2_ER      0x0080 /* Receive Error */
#define SCSSR2_TEND    0x0040 /* Transmit End */
#define SCSSR2_TDFE    0x0020 /* Transmit FIFO Data Empty */
#define SCSSR2_BRK     0x0010 /* Break Detect */
#define SCSSR2_FER     0x0008 /* Framing Error */
#define SCSSR2_PER     0x0004 /* Parity Error */
#define SCSSR2_RDF     0x0002 /* Receive FIFO Data Full */
#define SCSSR2_DR      0x0001 /* Receive Data Ready */

/* SCI Serial Port Register - SCSPTR1(byte) */
#define SCSPTR1_REGOFS 0xE0001C /* offset */
#define SCSPTR1        P4_REG32(SCSPTR1_REGOFS)
#define SCSPTR1_A7     A7_REG32(SCSPTR1_REGOFS)

#define SCSPTR1_EIO    0x80 /* Error Interrupt Only */
#define SCSPTR1_SPB1IO 0x08 /* 1: Output SPB1DT bit to SCK pin */
#define SCSPTR1_SPB1DT 0x04 /* Serial Port Clock Port Data */
#define SCSPTR1_SPB0IO 0x02 /* 1: Output SPB0DT bit to TxD pin */
#define SCSPTR1_SPB0DT 0x01 /* Serial Port Break Data */

/* SCIF Serial Port Register - SCSPTR2(half) */
#define SCSPTR2_REGOFS 0xE80020 /* offset */
#define SCSPTR2        P4_REG32(SCSPTR2_REGOFS)
#define SCSPTR2_A7     A7_REG32(SCSPTR2_REGOFS)

#define SCSPTR2_RTSIO  0x0080 /* 1: Output RTSDT bit to RTS2\ pin */
#define SCSPTR2_RTSDT  0x0040 /* RTS Port Data */
#define SCSPTR2_CTSIO  0x0020 /* 1: Output CTSDT bit to CTS2\ pin */
#define SCSPTR2_CTSDT  0x0010 /* CTS Port Data */
#define SCSPTR2_SPB2IO 0x0002 /* 1: Output SPBDT bit to TxD2 pin */
#define SCSPTR2_SPB2DT 0x0001 /* Serial Port Break Data */

/* SCI Bit Rate Register - SCBRR1(byte), SCBRR2(byte) */
#define SCBRR_REGOFS(n) ((n) == 1 ? 0xE00004 : 0xE80004) /* offset */
#define SCBRR(n)       P4_REG32(SCBRR_REGOFS(n))
#define SCBRR1         SCBRR_P4(1)
#define SCBRR2         SCBRR_P4(2)
#define SCBRR_A7(n)    A7_REG32(SCBRR_REGOFS(n))
#define SCBRR1_A7      SCBRR(1)
#define SCBRR2_A7      SCBRR(2)

/* SCIF FIFO Control Register - SCFCR2(half) */
#define SCFCR2_REGOFS  0xE80018 /* offset */
#define SCFCR2         P4_REG32(SCFCR2_REGOFS)
#define SCFCR2_A7      A7_REG32(SCFCR2_REGOFS)

#define SCFCR2_RSTRG   0x700 /* RTS2\ Output Active Trigger; RTS2\
                                       signal goes to high level when the
                                       number of received data stored in
                                       FIFO exceeds the trigger number */
#define SCFCR2_RSTRG_15 0x000 /* 15 bytes */
#define SCFCR2_RSTRG_1  0x000 /* 1 byte */
#define SCFCR2_RSTRG_4  0x000 /* 4 bytes */
#define SCFCR2_RSTRG_6  0x000 /* 6 bytes */
#define SCFCR2_RSTRG_8  0x000 /* 8 bytes */
#define SCFCR2_RSTRG_10 0x000 /* 10 bytes */
#define SCFCR2_RSTRG_14 0x000 /* 14 bytes */

#define SCFCR2_RTRG    0x0C0 /* Receive FIFO Data Number Trigger,
                                       Receive Data Full (RDF) Flag sets
                                       when number of receive data bytes is
                                       equal or greater than the trigger
                                       number */
#define SCFCR2_RTRG_1  0x000 /* 1 byte */
#define SCFCR2_RTRG_4  0x040 /* 4 bytes */
#define SCFCR2_RTRG_8  0x080 /* 8 bytes */
#define SCFCR2_RTRG_14 0x0C0 /* 14 bytes */

#define SCFCR2_TTRG    0x030 /* Transmit FIFO Data Number Trigger,
                                       Transmit FIFO Data Register Empty (TDFE)
                                       flag sets when the number of remaining
                                       transmit data bytes is equal or less
                                       than the trigger number */
#define SCFCR2_TTRG_8  0x000 /* 8 bytes */
#define SCFCR2_TTRG_4  0x010 /* 4 bytes */
#define SCFCR2_TTRG_2  0x020 /* 2 bytes */
#define SCFCR2_TTRG_1  0x030 /* 1 byte */

#define SCFCR2_MCE     0x008 /* Modem Control Enable */
#define SCFCR2_TFRST   0x004 /* Transmit FIFO Data Register Reset,
                                       invalidates the transmit data in the
                                       transmit FIFO */
#define SCFCR2_RFRST   0x002 /* Receive FIFO Data Register Reset,
                                       invalidates the receive data in the
                                       receive FIFO data register and resets
                                       it to the empty state */
#define SCFCR2_LOOP    0x001 /* Loopback Test */

/* SCIF FIFO Data Count Register - SCFDR2(half, read-only) */
#define SCFDR2_REGOFS  0xE8001C /* offset */
#define SCFDR2         P4_REG32(SCFDR2_REGOFS)
#define SCFDR2_A7      A7_REG32(SCFDR2_REGOFS)

#define SCFDR2_T       0x1F00 /* Number of untransmitted data bytes
                                        in transmit FIFO */
#define SCFDR2_T_S     8
#define SCFDR2_R       0x001F /* Number of received data bytes in
                                        receive FIFO */
#define SCFDR2_R_S     0

/* SCIF Line Status Register - SCLSR2(half, read-only) */
#define SCLSR2_REGOFS  0xE80024 /* offset */
#define SCLSR2         P4_REG32(SCLSR2_REGOFS)
#define SCLSR2_A7      A7_REG32(SCLSR2_REGOFS)

#define SCLSR2_ORER    0x0001 /* Overrun Error */

/*
 * SCI-based Smart Card Interface
 */
/* Smart Card Mode Register - SCSCMR1(byte) */
#define SCSCMR1_REGOFS 0xE00018 /* offset */
#define SCSCMR1        P4_REG32(SCSCMR1_REGOFS)
#define SCSCMR1_A7     A7_REG32(SCSCMR1_REGOFS)

#define SCSCMR1_SDIR   0x08 /* Smart Card Data Transfer Direction: */
#define SCSCMR1_SDIR_LSBF 0x00 /* LSB-first */
#define SCSCMR1_SDIR_MSBF 0x08 /* MSB-first */

#define SCSCMR1_SINV   0x04 /* Smart Card Data Inversion */
#define SCSCMR1_SMIF   0x01 /* Smart Card Interface Mode Select */

/* Smart-card specific bits in other registers */
/* SCSMR1: */
#define SCSMR1_GSM     0x80 /* GSM mode select */

/* SCSSR1: */
#define SCSSR1_ERS     0x10 /* Error Signal Status */

/*
 * I/O Ports
 */
/* Port Control Register A - PCTRA */
#define PCTRA_REGOFS   0x80002C /* offset */
#define PCTRA          P4_REG32(PCTRA_REGOFS)
#define PCTRA_A7       A7_REG32(PCTRA_REGOFS)

#define PCTRA_PBPUP(n) 0                 /* Bit n is pulled up */
#define PCTRA_PBNPUP(n) (1 << ((n)*2+1)) /* Bit n is not pulled up */
#define PCTRA_PBINP(n) 0                 /* Bit n is an input */
#define PCTRA_PBOUT(n) (1 << ((n)*2))    /* Bit n is an output */

/* Port Data Register A - PDTRA(half) */
#define PDTRA_REGOFS   0x800030 /* offset */
#define PDTRA          P4_REG32(PDTRA_REGOFS)
#define PDTRA_A7       A7_REG32(PDTRA_REGOFS)

#define PDTRA_BIT(n) (1 << (n))

/* Port Control Register B - PCTRB */
#define PCTRB_REGOFS   0x800040 /* offset */
#define PCTRB          P4_REG32(PCTRB_REGOFS)
#define PCTRB_A7       A7_REG32(PCTRB_REGOFS)

#define PCTRB_PBPUP(n) 0                    /* Bit n is pulled up */
#define PCTRB_PBNPUP(n) (1 << ((n-16)*2+1)) /* Bit n is not pulled up */
#define PCTRB_PBINP(n) 0                    /* Bit n is an input */
#define PCTRB_PBOUT(n) (1 << ((n-16)*2))    /* Bit n is an output */

/* Port Data Register B - PDTRB(half) */
#define PDTRB_REGOFS   0x800044 /* offset */
#define PDTRB          P4_REG32(PDTRB_REGOFS)
#define PDTRB_A7       A7_REG32(PDTRB_REGOFS)

#define PDTRB_BIT(n) (1 << ((n)-16))

/* GPIO Interrupt Control Register - GPIOIC(half) */
#define GPIOIC_REGOFS  0x800048 /* offset */
#define GPIOIC         P4_REG32(GPIOIC_REGOFS)
#define GPIOIC_A7      A7_REG32(GPIOIC_REGOFS)

#define GPIOIC_PTIREN(n) (1 << (n)) /* Port n is used as a GPIO int */

/*
 * Interrupt Controller - INTC
 */
/* Interrupt Control Register - ICR (half) */
#define ICR_REGOFS     0xD00000 /* offset */
#define ICR            P4_REG32(ICR_REGOFS)
#define ICR_A7         A7_REG32(ICR_REGOFS)

#define ICR_NMIL       0x8000 /* NMI Input Level */
#define ICR_MAI        0x4000 /* NMI Interrupt Mask */

#define ICR_NMIB       0x0200 /* NMI Block Mode: */
#define ICR_NMIB_BLK   0x0000 /*   NMI requests held pending while
                                          SR.BL bit is set to 1 */
#define ICR_NMIB_NBLK  0x0200 /*   NMI requests detected when SR.BL bit
                                          set to 1 */

#define ICR_NMIE       0x0100 /* NMI Edge Select: */
#define ICR_NMIE_FALL  0x0000 /*   Interrupt request detected on falling
                                          edge of NMI input */
#define ICR_NMIE_RISE  0x0100 /*   Interrupt request detected on rising
                                          edge of NMI input */

#define ICR_IRLM       0x0080 /* IRL Pin Mode: */
#define ICR_IRLM_ENC   0x0000 /*   IRL\ pins used as a level-encoded
                                          interrupt requests */
#define ICR_IRLM_RAW   0x0080 /*   IRL\ pins used as a four independent
                                          interrupt requests */

/* Interrupt Priority Register A - IPRA (half) */
#define IPRA_REGOFS    0xD00004 /* offset */
#define IPRA           P4_REG32(IPRA_REGOFS)
#define IPRA_A7        A7_REG32(IPRA_REGOFS)

#define IPRA_TMU0      0xF000 /* TMU0 interrupt priority */
#define IPRA_TMU0_S    12
#define IPRA_TMU1      0x0F00 /* TMU1 interrupt priority */
#define IPRA_TMU1_S    8
#define IPRA_TMU2      0x00F0 /* TMU2 interrupt priority */
#define IPRA_TMU2_S    4
#define IPRA_RTC       0x000F /* RTC interrupt priority */
#define IPRA_RTC_S     0

/* Interrupt Priority Register B - IPRB (half) */
#define IPRB_REGOFS    0xD00008 /* offset */
#define IPRB           P4_REG32(IPRB_REGOFS)
#define IPRB_A7        A7_REG32(IPRB_REGOFS)

#define IPRB_WDT       0xF000 /* WDT interrupt priority */
#define IPRB_WDT_S     12
#define IPRB_REF       0x0F00 /* Memory Refresh unit interrupt
                                        priority */
#define IPRB_REF_S     8
#define IPRB_SCI1      0x00F0 /* SCI1 interrupt priority */
#define IPRB_SCI1_S    4

/* Interrupt Priority Register C - IPRC (half) */
#define IPRC_REGOFS    0xD00004 /* offset */
#define IPRC           P4_REG32(IPRC_REGOFS)
#define IPRC_A7        A7_REG32(IPRC_REGOFS)

#define IPRC_GPIO      0xF000 /* GPIO interrupt priority */
#define IPRC_GPIO_S    12
#define IPRC_DMAC      0x0F00 /* DMAC interrupt priority */
#define IPRC_DMAC_S    8
#define IPRC_SCIF      0x00F0 /* SCIF interrupt priority */
#define IPRC_SCIF_S    4
#define IPRC_HUDI      0x000F /* H-UDI interrupt priority */
#define IPRC_HUDI_S    0


/*
 * User Break Controller registers
 */

#ifndef BARA
 
#define BARA           0x200000 /* Break address regiser A */
#define BAMRA          0x200004 /* Break address mask regiser A */
#define BBRA           0x200008 /* Break bus cycle regiser A */
#define BARB           0x20000c /* Break address regiser B */
#define BAMRB          0x200010 /* Break address mask regiser B */
#define BBRB           0x200014 /* Break bus cycle regiser B */
#define BASRB          0x000018 /* Break ASID regiser B */
#define BDRB           0x200018 /* Break data regiser B */
#define BDMRB          0x20001c /* Break data mask regiser B */
#define BRCR           0x200020 /* Break control register */

#define BRCR_UDBE        0x0001 /* User break debug enable bit */

#endif /* BARA */


#endif /* __SH7750_REGS_H__ */
