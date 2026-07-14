!///////////////////////////////////////////////////////////////////////////////
!// Initialization of the SH-4 that is not covered by regular applications
!// Derived from JinGasa BIOS
!//
!// Copyright 2023 by T_chan
!// Copyright 2026 by SWAT
!//
!// R0 to R7: not preserved, R8 to R13: preserved, R14/R15: do not touch
!///////////////////////////////////////////////////////////////////////////////
    .globl _sh4_init
    .text

_sh4_init:

!///////////////////////////////////////////////////////////////////////////////
!// Cache Controler (CCR) Initialization
!///////////////////////////////////////////////////////////////////////////////
.sh4_ccr_init:
    mov.l CACHE_CCR_DATA, R1              ! Cache Control Register (CCR, 32 bits)
    mov.l CACHE_CCR_ADDR, R2
    mov.l R1, @R2

!///////////////////////////////////////////////////////////////////////////////
!// Bus State Control (BSC) Initialization
!///////////////////////////////////////////////////////////////////////////////
.sh4_bsc_init:
    mov.l BSC_BCR1_DATA, R1               ! Bus control register 1 (BCR1, 32bits)
    mov.l BSC_BCR1_ADDR, R2
    mov.l R1, @R2
    mov.l BSC_BCR2_DATA, R1               ! Bus control register 2 (BCR2, 16bits)
    mov.l BSC_BCR2_ADDR, R2
    mov.w R1, @R2

    mov.l BSC_WCR1_DATA, R1               ! Wait state control register 1 (WCR1, 32bits)
    mov.l BSC_WCR1_ADDR, R2
    mov.l R1, @R2
    mov.l BSC_WCR2_DATA, R1               ! Wait state control register 2 (WCR2, 32bits)
    mov.l BSC_WCR2_ADDR, R2
    mov.l R1, @R2
    mov.l BSC_WCR3_DATA, R1               ! Wait state control register 3 (WCR3, 32bits)
    mov.l BSC_WCR3_ADDR, R2
    mov.l R1, @R2

    ! (note: not set: PCMCIA control register (PCR, 0xFF800018))

    mov.l BSC_RTCSR_DATA, R1              ! Refresh timer control/status register (RTCSR, 16bits)
    mov.l BSC_RTCSR_ADDR, R2
    mov.w R1, @R2
    mov.l BSC_RTCNT_DATA, R1              ! Refresh timer counter (RTCNT, 16bits)
    mov.l BSC_RTCNT_ADDR, R2
    mov.w R1, @R2
    mov.l BSC_RTCOR_DATA, R1              ! Refresh time constant counter (RTCOR, 16bits)
    mov.l BSC_RTCOR_ADDR, R2
    mov.w R1, @R2
    mov.l BSC_RFCR_DATA, R1               ! Refresh count register (RFCR, 16bits)
    mov.l BSC_RFCR_ADDR, R2
    mov.w R1, @R2

_ram_size_init:
    mov.l   FLASH_ROM_ADDR, R0
    mov.l   @R0, R1
    mov.l   FLASH_MAGIC, R2
    cmp/eq  R1, R2
    bf/s    .ram_platform_naomi
    nop

    mov.l   STACK_INIT_16MB, R15
    mov.l   BSC_MCR_16MB, R1
    bra     _set_ram_size
    nop

.ram_platform_naomi:
    mov.l   STACK_INIT_32MB, R15
    mov.l   BSC_MCR_32MB, R1

_set_ram_size:
    mov.l BSC_MCR_ADDR, R2               ! Memory control register (MCR, 32bits)
    mov.l R1, @R2
    nop
    nop

    mov.l BSC_SDMR3_DATA, R1              ! SDRAM mode register for area 3 (SDMR3, 8bits)
    mov.l BSC_SDMR3_ADDR, R2      ! TODO ? SDMR2 ?
    mov.b R1, @R2

!///////////////////////////////////////////////////////////////////////////////
!// General-purpose I/O ports Initialization
!///////////////////////////////////////////////////////////////////////////////
.sh4_gpio_init:
    mov.l GPIO_PCTRA_DATA, R1             ! Port control register A (PCTRA, 32 bits)
    mov.l GPIO_PCTRA_ADDR, R2
    mov.w R1, @R2

!    mov.w SCIF_SCSPTR2_DATA, R1          !(H'00c0)
!    mov.l SCIF_SCSPTR2_ADDR, R2          !(H'ffe80020) SCSPTR2    
!    mov.w R1, @R2

!///////////////////////////////////////////////////////////////////////////////
!// NAOMI 2 bus setup (original BIOS a00206d4): PDTRA bits 8-9, WCR2, BCR2 PORTEN
!///////////////////////////////////////////////////////////////////////////////
.sh4_n2_bus_init:
    mov.l N2_ELAN_MAGIC_ADDR, R1
    mov.l @R1, R1
    mov.l N2_ELAN_MAGIC_VALUE, R2
    cmp/eq R1, R2
    bf .sh4_dmac_init

    mov.l BSC_BCR2_ADDR, R2
    mov #1, R1
    mov.w R1, @R2

    mov.l BSC_PDTRA_ADDR, R4
    mov.w PDTRA_CLR_MASK, R5
    mov.w @R4, R3
    and R5, R3
    mov.w PDTRA_PB8DT, R2
    or R2, R3
    mov.w R3, @R4
    mov.w @R4, R3
    and R5, R3
    mov.w PDTRA_PB8DT_PB9DT, R2
    or R2, R3
    mov.w R3, @R4

    mov.l BSC_WCR2_N2_DATA, R1
    mov.l BSC_WCR2_ADDR, R2
    mov.l R1, @R2

!///////////////////////////////////////////////////////////////////////////////
!// DMAC Initialization - enable DMA, otherwise Maple won't work
!///////////////////////////////////////////////////////////////////////////////
.sh4_dmac_init:
    mov.l DMAC_DMAOR_DATA, R1            ! DMA operation register: 
    mov.l DMAC_DMAOR_ADDR, R2            !  enable all channels & 
    mov.l R1, @R2                        !  priority: CH2 > CH0 > CH1 > CH3

!///////////////////////////////////////////////////////////////////////////////
!// end
!///////////////////////////////////////////////////////////////////////////////
    rts
    nop

!///////////////////////////////////////////////////////////////////////////////
!// Constants
!///////////////////////////////////////////////////////////////////////////////
    .align 4


CACHE_CCR_ADDR:                          ! Cache control register (CCR):
    .long        0xFF00001C              !    Initial: 0x00000000
CACHE_CCR_DATA:                          !    Bios:    0x00000105
    .long        0x00000105              ! or 0x0000090b or  0x00000929 ? or 0x00000909 ?

BSC_BCR1_ADDR:                           ! Bus control register 1:
    .long        0xFF800000              !    Initial: 0x00000000
BSC_BCR1_DATA:                           !    Bios:    0xA3020008
    .long        0xA3020008
BSC_BCR2_ADDR:                           ! Bus control register 2:
    .long        0xFF800004              !    Initial: 0x3FFC
BSC_BCR2_DATA:                           !    Bios:    0x00000000
    .long        0x00000000

BSC_WCR1_ADDR:                           ! Wait state control register 1:
    .long        0xFF800008              !    Initial: 0x77777777
BSC_WCR1_DATA:                           !    Bios:    0x01110111
    .long        0x01110111
BSC_WCR2_ADDR:                           ! Wait state control register 2:
    .long        0xFF80000C              !    Initial: 0xFFFEEFFF
BSC_WCR2_DATA:                           !    Bios:    0x618066D8
    .long        0x018060D8              ! or 0x018060D8 ?
BSC_WCR3_ADDR:                           ! Wait state control register 3:
    .long        0xFF800010              !    Initial: 0x07777777
BSC_WCR3_DATA:                           !    Bios:    0x07777777
    .long        0x07777777

BSC_MCR_ADDR:                            ! Memory control register:
    .long        0xFF800014              !    Initial: 0x00000000
BSC_MCR_16MB:                            !    Bios: (16MB): 0xC00A0E24          32MB: 0xC0121214, 16MB: 0xC0091224
    .long        0xC00A0E24              !
BSC_MCR_32MB:                            !    Bios: (16MB): 0xC00A0E24          32MB: 0xC0121214, 16MB: 0xC0091224
    .long        0xC0121214              !

BSC_RTCSR_ADDR:                          ! Refresh timer control/status register:
    .long        0xFF80001C              !    Initial: 0x0000
BSC_RTCSR_DATA:                          !    Bios:    0x9400 
    .long        0x0000A510
BSC_RTCNT_ADDR:                          ! Refresh timer counter:
    .long        0xFF800020              !    Initial: 0x0000
BSC_RTCNT_DATA:                          !    Bios:    0x4300
    .long        0x0000A500
BSC_RTCOR_ADDR:                          ! Refresh time constant counter:
    .long        0xFF800024              !    Initial: 0x0000
BSC_RTCOR_DATA:                          !    Bios:    0x5E00
    .long        0x0000A55E
BSC_RFCR_ADDR:                           ! Refresh count register:
    .long        0xFF800028              !    Initial: 0x0000
BSC_RFCR_DATA:                           !    Bios:    0x2402
    .long        0x0000A400

BSC_SDMR3_ADDR:                          ! SDRAM mode register for area 3:
    .long        0xFF940190              !
BSC_SDMR3_DATA:                          !            
    .long        0x00000090

GPIO_PCTRA_ADDR:                         ! Port control register A:
    .long        0xFF80002C              !    Initial: 0x00000000
GPIO_PCTRA_DATA:                         !    Bios:    0x000A03F0           System SP: 0x000fef30 ?
    .long        0x000fef30              ! or 0efe         
!   .long        0x000feffe

GPIO_PCTRB_ADDR:                         ! Port control register B:
    .long        0xFF800040              !    Initial: 0x00000000

SCIF_SCSPTR2_ADDR:
    .long        0xffe80020
SCIF_SCSPTR2_DATA:
    .word        0x00c0
SCIF_DUMMY:
    .word        0x0000

N2_ELAN_MAGIC_ADDR:
    .long        0xA8800000
N2_ELAN_MAGIC_VALUE:
    .long        0xE1AD0000
BSC_PDTRA_ADDR:                          ! Port data register A:
    .long        0xFF800030
BSC_WCR2_N2_DATA:                        ! Naomi2 BIOS power-on WCR2
    .long        0x01046290
PDTRA_CLR_MASK:
    .word        0xFC3F
PDTRA_PB8DT:
    .word        0x0100
PDTRA_PB8DT_PB9DT:
    .word        0x0300
PDTRA_PAD:
    .word        0x0000

DMAC_DMAOR_ADDR:                         ! DMA operation register
    .long        0xFFA00040              !    Initial: 0x00000000
DMAC_DMAOR_DATA:                         !    Bios:    0x00008201  (kos-naomi: 0x8201)
    .long        0x00008201

STACK_INIT_16MB:
    .long        0x8d000000
STACK_INIT_32MB:
    .long        0x8e000000

FLASH_ROM_ADDR:
    .long        0xA0200000
FLASH_MAGIC:
    .long        0x4154414B
