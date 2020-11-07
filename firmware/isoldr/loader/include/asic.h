/**
 * DreamShell ISO Loader
 * ASIC interrupts handling
 * (c)2014-2020 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#ifndef __ASIC_H__
#define __ASIC_H__

#include <main.h>
#include <exception-lowlevel.h>

#define ASIC_TABLE_SIZE   1

#define ASIC_BASE         (0xa05f6900)
#define ASIC_IRQ_STATUS   (REGISTER(vuint32) (ASIC_BASE + 0x00))
#define ASIC_IRQ13_MASK   (REGISTER(vuint32) (ASIC_BASE + 0x10))
#define ASIC_IRQ11_MASK   (REGISTER(vuint32) (ASIC_BASE + 0x20))
#define ASIC_IRQ9_MASK    (REGISTER(vuint32) (ASIC_BASE + 0x30))

#define ASIC_MASK_NRM_INT 0
#define ASIC_MASK_EXT_INT 1
#define ASIC_MASK_ERR_INT 2

/* Errors

	bit 31 = SH4 i/f: accessing to Inhibited area
	bit 30 = Reserved
	bit 29 = Reserved
	bit 28 = DDT i/f : Sort-DMA Command Error
	bit 27 = G2 : Time out in CPU accessing
	bit 26 = G2 : Dev-DMA Time out
	bit 25 = G2 : Ext-DMA2 Time out
	bit 24 = G2 : Ext-DMA1 Time out
	bit 23 = G2 : AICA-DMA Time out
	bit 22 = G2 : Dev-DMA over run
	bit 21 = G2 : Ext-DMA2 over run
	bit 20 = G2 : Ext-DMA1 over run
	bit 19 = G2 : AICA-DMA over run
	bit 18 = G2 : Dev-DMA Illegal Address set
	bit 17 = G2 : Ext-DMA2 Illegal Address set
	bit 16 = G2 : Ext-DMA1 Illegal Address set
	bit 15 = G2 : AICA-DMA Illegal Address set
	bit 14 = G1 : ROM/FLASH access at GD-DMA
	bit 13 = G1 : GD-DMA over run
	bit 12 = G1 : Illegal Address set
	bit 11 = MAPLE : Illegal command
	bit 10 = MAPLE : Write FIFO over flow
	bit 9 = MAPLE : DMA over run
	bit 8 = MAPLE : Illegal Address set
	bit 7 = PVRIF : DMA over run
	bit 6 = PVRIF : Illegal Address set
	bit 5 = TA : FIFO Overflow
	bit 4 = TA : Illegal Parameter
	bit 3 = TA : Object List Pointer Overflow
	bit 2 = TA : ISP/TSP Parameter Overflow
	bit 1 = RENDER : Hazard Processing of Strip Buffer
	bit 0 = RENDER : ISP out of Cache(Buffer over flow)
*/

/* Interrupts

	bit 31 = Error interrupt ‘OR’ status
	bit 30 = G1,G2,External interrupt ‘OR’ status

	bit 21 = End of Transferring interrupt : Punch Through List
	bit 20 = End of DMA interrupt : Sort-DMA (Transferring for alpha sorting)
	bit 19 = End of DMA interrupt : ch2-DMA
	bit 18 = End of DMA interrupt : Dev-DMA(Development tool DMA)
	bit 17 = End of DMA interrupt : Ext-DMA2(External 2)
	bit 16 = End of DMA interrupt : Ext-DMA1(External 1)
	bit 15 = End of DMA interrupt : AICA-DMA
	bit 14 = End of DMA interrupt : GD-DMA
	bit 13 = Maple V blank over interrupt
	bit 12 = End of DMA interrupt : Maple-DMA
	bit 11 = End of DMA interrupt : PVR-DMA
	bit 10 = End of Transferring interrupt : Translucent Modifier Volume List
	bit 9 = End of Transferring interrupt : Translucent List
	bit 8 = End of Transferring interrupt : Opaque Modifier Volume List
	bit 7 = End of Transferring interrupt : Opaque List
	bit 6 = End of Transferring interrupt : YUV
	bit 5 = H Blank-in interrupt
	bit 4 = V Blank-out interrupt
	bit 3 = V Blank-in interrupt
	bit 2 = End of Render interrupt : TSP
	bit 1 = End of Render interrupt : ISP
	bit 0 = End of Render interrupt : Video
*/

#define ASIC_NRM_TADONE           0x04        /* Rendering complete */
#define ASIC_NRM_RASTER_BOTTOM    0x08        /* Bottom raster event */
#define ASIC_NRM_RASTER_TOP       0x10        /* Top raster event */
#define ASIC_NRM_VSYNC            0x20        /* Vsync event */

#define ASIC_NRM_OPAQUE_POLY      0x080       /* Opaque polygon binning complete */
#define ASIC_NRM_OPAQUE_MOD       0x100       /* Opaque modifier binning complete */
#define ASIC_NRM_TRANS_POLY       0x200       /* Transparent polygon binning complete */
#define ASIC_NRM_TRANS_MOD        0x400       /* Transparent modifier binning complete */

#define ASIC_NRM_MAPLE_DMA        0x01000     /* Maple DMA complete */
#define ASIC_NRM_MAPLE_ERROR      0x02000     /* Maple V blank over interrupt */
#define ASIC_NRM_GD_DMA           0x04000     /* GD-ROM DMA complete */
#define ASIC_NRM_AICA_DMA         0x08000     /* AICA DMA complete */
#define ASIC_NRM_EXT1_DMA         0x10000     /* External DMA 1 complete */
#define ASIC_NRM_EXT2_DMA         0x20000     /* External DMA 2 complete */
#define ASIC_NRM_DEV_DMA          0x40000     /* Dev DMA complete */
#define ASIC_NRM_PVR_DMA          0x80000     /* PVR (CH2) DMA complete */
#define ASIC_NRM_SORT_DMA         0x100000    /* Sort-DMA complete */
#define ASIC_NRM_PUNCHPOLY        0x200000    /* Punchthrough polygon binning complete */

#define ASIC_EXT_GD_CMD           0x01        /* GD-ROM command status */
#define ASIC_EXT_AICA             0x02        /* AICA */
#define ASIC_EXT_MODEM            0x04        /* Modem ? */
#define ASIC_EXT_PCI              0x08        /* Expansion port (PCI Bridge) */

#define ASIC_ERR_G1DMA_ILLEGAL    0x01000
#define ASIC_ERR_G1DMA_OVERRUN    0x02000
#define ASIC_ERR_G1DMA_ROM_FLASH  0x04000

typedef void *(*asic_handler_f) (void *, register_stack *, void *);

typedef struct {
	
    uint32 mask[3];
    uint32 irq;
    asic_handler_f handler;
    int clear_irq;
	
} asic_lookup_table_entry;


typedef struct {
	
    int inited;
	
#ifndef NO_ASIC_LT
    asic_lookup_table_entry table[ASIC_TABLE_SIZE];
#endif
	
} asic_lookup_table;


void asic_init(void);
int asic_add_handler(const asic_lookup_table_entry *new_entry, 
						asic_handler_f *parent_handler, int enable_irq);
void asic_enable_irq(const asic_lookup_table_entry *entry);

#ifdef DEV_TYPE_IDE
/* Initialize DMA interrupt handling for IDE and GD */
int g1_dma_init_irq();
#endif

int irq_disabled();

#endif
