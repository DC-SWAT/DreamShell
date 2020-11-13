/**
 * DreamShell ISO Loader
 * ASIC IRQ handling
 * (c)2014-2020 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#include <main.h>
#include <exception.h>
#include <asic.h>
#include <cdda.h>

#if (defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)) && defined(NO_ASIC_LT)
void* g1_dma_handler(void *passer, register_stack *stack, void *current_vector);
#endif

static asic_lookup_table   asic_table;
static exception_handler_f old_handler;
//void dump_maple_dma_buffer();

static void* asic_handle_exception(register_stack *stack, void *current_vector) {
	
	uint32 code = *REG_INTEVT;
	// uint32 status = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
	
	// if(code == EXP_CODE_INT13 || code == EXP_CODE_INT11 || code == EXP_CODE_INT9) {
	// 	LOGF("IRQ: 0x%lx NRM: 0x%08lx EXT: 0x%08lx ERR: 0x%08lx\n", 
	// 				*REG_INTEVT & 0x0fff, status, 
	// 				ASIC_IRQ_STATUS[ASIC_MASK_EXT_INT], 
	// 				ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT]);
	// }
	// dump_regs(stack);

#ifdef NO_ASIC_LT
	/**
	 * Use ASIC handlers directly instead of lookup table
	 */

	void *back_vector = current_vector;
	uint32 status = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];

# if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	uint32 statusExt = ASIC_IRQ_STATUS[ASIC_MASK_EXT_INT];
	uint32 statusErr = ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT];

	if ((status & ASIC_NRM_GD_DMA) ||
		(statusExt & ASIC_EXT_GD_CMD) ||
		(statusErr & ASIC_ERR_G1DMA_ILLEGAL) || (statusErr & ASIC_ERR_G1DMA_OVERRUN) || (statusErr & ASIC_ERR_G1DMA_ROM_FLASH)
	) {
		back_vector = g1_dma_handler(NULL, stack, current_vector);
	}
# else
	(void)stack;
# endif

	if (code == EXP_CODE_INT13 || code == EXP_CODE_INT11 || code == EXP_CODE_INT9) {

		if (status & ASIC_NRM_VSYNC) {
# ifdef HAVE_CDDA
			CDDA_MainLoop();
# endif
			apply_patch_list();
		}

//		if(status & ASIC_NRM_MAPLE_DMA) {
//			dump_maple_dma_buffer();
//		}
	}

//	LOGFF("0x%08lx\n", (uint32)back_vector);
	return back_vector;

#else
	
	void *new_vector = current_vector;

	/* Handle exception table */
	for (uint32 index = 0; index < ASIC_TABLE_SIZE; index++) {
		
		asic_lookup_table_entry passer;

		/*
			Technically, this can cause matches on exceptions in cases
			where the SH4 combines them and/or the exceptions have been
			placed in a queue. However, this doesn't bother me too much.
		*/
		for (int i = 0; i < 3; i++) {
			passer.mask[i] = ASIC_IRQ_STATUS[i] & asic_table.table[index].mask[i];
		}
		
		passer.irq = code;
		passer.clear_irq = asic_table.table[index].clear_irq;

		if ((passer.mask[ASIC_MASK_NRM_INT] || passer.mask[ASIC_MASK_EXT_INT] || passer.mask[ASIC_MASK_ERR_INT]) && 
			(asic_table.table[index].irq == passer.irq || asic_table.table[index].irq == EXP_CODE_ALL)) {
			
			new_vector = asic_table.table[index].handler(&passer, stack, new_vector);

			/* Clear the IRQ by default - but the option is controllable. */
			if (passer.clear_irq) {
				
				if(passer.mask[ASIC_MASK_NRM_INT]) {
					ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = passer.mask[ASIC_MASK_NRM_INT];
				}
				
				if(passer.mask[ASIC_MASK_ERR_INT]) {
					ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT] = passer.mask[ASIC_MASK_ERR_INT];
				}
			}
		}
	}

	/* Return properly, depending if there is an older handler. */
	if (old_handler)
		return old_handler(stack, new_vector);
	else
		return new_vector;
#endif
}


void asic_enable_irq(const asic_lookup_table_entry *entry) {
	
	volatile uint32 *mask_base;
	int i;

	/* Determine which ASIC IRQ bank, if any, the given mask will be enabled on. */
	switch (entry->irq) {
		
		case EXP_CODE_INT9:
			mask_base = ASIC_IRQ9_MASK;
			break;

		case EXP_CODE_INT11:
			mask_base = ASIC_IRQ11_MASK;
			break;

		case EXP_CODE_INT13:
			mask_base = ASIC_IRQ13_MASK;
			break;

		case EXP_CODE_ALL:
		{
			/* Mask the first two ASIC banks. */
			mask_base = ASIC_IRQ9_MASK;

			for(i = 0; i < 3; i++) {
				mask_base[i] |= entry->mask[i];
			}

			mask_base = ASIC_IRQ11_MASK;
			
			for(i = 0; i < 3; i++) {
				mask_base[i] |= entry->mask[i];
			}

			/* Have the code further on take care of the last mask. */
			mask_base = ASIC_IRQ13_MASK;
			break;
		}

		/* Probably an empty entry. Either way, we can't do anything. */
		default:
			mask_base = NULL;
			break;
	}

	/* Enable the selected G2 IRQs on the ASIC. */

	if (mask_base) {
		for(i = 0; i < 3; i++) {
			mask_base[i] |= entry->mask[i];
		}
	}
}

#ifndef NO_ASIC_LT
int asic_add_handler(const asic_lookup_table_entry *new_entry, asic_handler_f *parent_handler, int enable_irq) {
	
	uint32 index;

	/* Don't allow adding of handlers until we've initialized. */
	if (!asic_table.inited)
		return 0;

	/* Scan the entire ASIC table an empty slot. */
	for (index = 0; index < ASIC_TABLE_SIZE; index++) {
		
		if ((asic_table.table[index].irq == new_entry->irq) &&
			(asic_table.table[index].mask[ASIC_MASK_NRM_INT] == new_entry->mask[ASIC_MASK_NRM_INT]) && 
			(asic_table.table[index].mask[ASIC_MASK_EXT_INT] == new_entry->mask[ASIC_MASK_EXT_INT]) &&
			(asic_table.table[index].mask[ASIC_MASK_ERR_INT] == new_entry->mask[ASIC_MASK_ERR_INT])) {
				
			if(parent_handler)
				*parent_handler = asic_table.table[index].handler;

			asic_table.table[index].handler = new_entry->handler;

			return 1;
			
		} else if (!(asic_table.table[index].irq)) {
			
			/* Ensure there isn't any parent handler given back. */
			if(parent_handler)
				*parent_handler = NULL;

			/* Enable the IRQ bank on the ASIC as specified by the entry. */
			if(enable_irq)
				asic_enable_irq(new_entry);

			/* Copy the new entry into our table. */
			memcpy(&asic_table.table[index], new_entry, sizeof(asic_lookup_table_entry));
			return 1;
		}
	}

	return 0;
}
#endif

void asic_init(void) {
	
	exception_table_entry new_entry;

	/* Ensure we can't initialize ourselves twice. */
	if (asic_table.inited) {
#ifndef NO_ASIC_LT
		/* Reinitialize the active IRQs on the ASIC. */
//		for (int index = 0; index < ASIC_TABLE_SIZE; index++)
//			asic_enable_irq(&asic_table.table[index]);
#endif
		return;
	}

	/* Works for all the interrupt types... */
	new_entry.type    = EXP_TYPE_INT;
	new_entry.handler = asic_handle_exception;

	/*
		ASIC handling of all interrupts.
		In our case, the handler itself checks the exception code and
		matches it to the interrupt.
	*/
	new_entry.code    = EXP_CODE_ALL;
	asic_table.inited = exception_add_handler(&new_entry, &old_handler);
}
