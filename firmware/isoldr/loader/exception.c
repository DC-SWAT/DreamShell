/**
 * DreamShell ISO Loader
 * Exception handling
 * (c)2014-2023 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#include <main.h>
#include <exception.h>
#include <arch/cache.h>

extern uint32 exception_os_type;
extern uint32 interrupt_stack;
static exception_table exp_table;
static volatile int inside_int = 0;
static int inited = 0;

/* Pointers to the VBR Buffer */
static uint8 *vbr_buffer;
static uint8 *vbr_buffer_orig;

int exception_inited(void) {
	return inited; //exception_vbr_ok();
}

static int exception_vbr_ok(void) {
	
	uint32 int_changed;
#ifdef HAVE_UBC
	uint32 gen_changed;
#endif
//	uint32 cache_changed;

    /* Check to see if our VBR hooks are still installed. */
	int_changed = memcmp(
		VBR_INT(vbr_buffer) - (interrupt_sub_handler_base - interrupt_sub_handler),
		interrupt_sub_handler,
		interrupt_sub_handler_end - interrupt_sub_handler
	);
#ifdef HAVE_UBC
	gen_changed = memcmp(
		VBR_GEN (vbr_buffer) - (general_sub_handler_base - general_sub_handler),
		general_sub_handler,
		general_sub_handler_end - general_sub_handler
	);
#endif
//
//	cache_changed = memcmp(
//		VBR_GEN (vbr_buffer) - (cache_sub_handler_base - cache_sub_handler),
//		cache_sub_handler,
//		cache_sub_handler_end - cache_sub_handler
//	);

    /* After enough exceptions, allow the initialization. */
    return !(
		int_changed
#ifdef HAVE_UBC
		|| gen_changed
#endif
		/* || cache_changed*/
	);
}

int exception_init(uint32 vbr_addr) {

	vbr_buffer = vbr_addr > 0 ? (void *)vbr_addr : vbr();

	if (!vbr_buffer/* || exception_vbr_ok () || (exp_table.ubc_exception_count < 7)*/) {
		return -1;
	}
	
	if(exception_vbr_ok()) {
		LOGFF("already initialized\n");
		return 0;
	}

	exception_os_type = IsoInfo->exec.type;

	/* Relocate the VBR index - bypass our entry logic. */
	if (exception_os_type == BIN_TYPE_KATANA) {
		// Skip one more instruction because it in paired using with old replaced instruction.
		vbr_buffer_orig = vbr_buffer + (sizeof (uint16) * 4);
	} else if(exception_os_type == BIN_TYPE_KOS) {
		// Direct usage of _irq_save_regs by fixed offset.
		vbr_buffer_orig = vbr_buffer - 0x188;
	} else {
		// Normally skip only 3 replaced instruction.
		vbr_buffer_orig = vbr_buffer + (sizeof (uint16) * 3);
	}

	// if(IsoInfo->exec.type != BIN_TYPE_WINCE) {
	// 	interrupt_stack = (uint32)malloc(2048);
	// }
	// LOGFF("VBR buffer 0x%08lx -> 0x%08lx, stack 0x%08lx\n", vbr_buffer, vbr_buffer_orig, interrupt_stack);
	LOGFF("VBR buffer 0x%08lx -> 0x%08lx\n", vbr_buffer, vbr_buffer_orig);

	/* Interrupt hack for VBR. */
	memcpy(
		VBR_INT(vbr_buffer) - (interrupt_sub_handler_base - interrupt_sub_handler),
		interrupt_sub_handler,
		interrupt_sub_handler_end - interrupt_sub_handler
	);

	if (exception_os_type != BIN_TYPE_WINCE) {
		uint16 *change_stack_instr = VBR_INT(vbr_buffer) - (interrupt_sub_handler_base - interrupt_sub_handler);
		*change_stack_instr = 0x0009; // nop
	}

#ifdef HAVE_UBC
	/* General exception hack for VBR. */
	memcpy(
		VBR_GEN(vbr_buffer) - (general_sub_handler_base - general_sub_handler),
		general_sub_handler,
		general_sub_handler_end - general_sub_handler
	);
#endif

	/* Cache exception hack for VBR. */
//	memcpy(
//		VBR_CACHE(vbr_buffer) - (cache_sub_handler_base - cache_sub_handler),
//		cache_sub_handler,
//		cache_sub_handler_end - cache_sub_handler
//	);

	/* Flush cache after modifying application memory. */
	dcache_flush_range((uint32)vbr_buffer, 0xC08);
	icache_flush_range((uint32)vbr_buffer, 0xC08);

	inited = 1;
	return 0;
}

int exception_inside_int(void) {
	return inside_int;
}


int exception_add_handler(const exception_table_entry *new_entry, exception_handler_f *parent_handler) {
	
	uint32 index;

	/* Search the entire table for either a match or an opening. */
	for (index = 0; index < EXP_TABLE_SIZE; index++) {
		
		/*
			If the entry is configured with same type and code as the
			new handler, push it on the stack and let's roll!
		*/
		if ((exp_table.table[index].type == new_entry->type) && (exp_table.table[index].code == new_entry->code)) {

			if(parent_handler)
				*parent_handler = exp_table.table[index].handler;

			exp_table.table[index].handler = new_entry->handler;
			return 1;

		} else if (!(exp_table.table[index].type)) {
			/*
				We've reached the end of the filled entries, I guess we have to create our own.
			*/
			if(parent_handler)
				*parent_handler = NULL;

			memcpy(&exp_table.table[index], new_entry, sizeof(exception_table_entry));
			return 1;
		}
	}

	return 0;
}

void *exception_handler(register_stack *stack) {
	
	uint32 exception_code;
	uint32 index;
	void *back_vector;
	
	if (inside_int) {
		LOGFF("ERROR: already in IRQ\n");
		return my_exception_finish;
	}
	
	inside_int = 1;
	
#if 0
	LOGFF("0x%02x 0x%08lx\n",
			stack->exception_type & 0xff, 
			stack->exception_type == EXP_TYPE_INT ? *REG_INTEVT : *REG_EXPEVT);
//	dump_regs(stack);
#endif

	/* Ensure vbr buffer is set... */
	// vbr_buffer = (uint8 *) stack->vbr;

	/* Increase our counters and set the proper back_vectors. */
	switch (stack->exception_type)
	{
#ifdef HAVE_UBC
		case EXP_TYPE_GEN :
		{
			//exp_table.general_exception_count++;
			exception_code = *REG_EXPEVT;

			/* Never pass on UBC interrupts to the game. */
			if ((exception_code == EXP_CODE_UBC) || (exception_code == EXP_CODE_TRAP)) {
				//exp_table.ubc_exception_count++;
				back_vector = my_exception_finish;
			} else {
				back_vector = exception_os_type == BIN_TYPE_KOS ? vbr_buffer_orig : VBR_GEN(vbr_buffer_orig);
			}

			break;
		}
#endif
//
//		case EXP_TYPE_CACHE :
//		{
//			//exp_table.cache_exception_count++;
//			exception_code  = *REG_EXPEVT;
//			back_vector     = exception_os_type == BIN_TYPE_KOS ? vbr_buffer_orig : VBR_CACHE(vbr_buffer_orig);
//			break;
//		}

		case EXP_TYPE_INT :
		{
			//exp_table.interrupt_exception_count++;
			exception_code  = *REG_INTEVT;
			back_vector     = exception_os_type == BIN_TYPE_KOS ? vbr_buffer_orig : VBR_INT(vbr_buffer_orig);
			break;
		}

		default :
		{
			//exp_table.odd_exception_count++;
			exception_code  = EXP_CODE_BAD;
			back_vector     = exception_os_type == BIN_TYPE_KOS ? vbr_buffer_orig : VBR_INT(vbr_buffer_orig);
			break;
		}
	}

	/* Handle exception table */
	for (index = 0; index < EXP_TABLE_SIZE; index++) {
		
		if (((exp_table.table[index].code == exception_code) || (exp_table.table[index].code == EXP_CODE_ALL)) &&
			((exp_table.table[index].type == stack->exception_type) || (exp_table.table[index].type == EXP_TYPE_ALL))) {
			
			/* Call the handler and use whatever hook it returns. */
			back_vector = exp_table.table[index].handler(stack, back_vector);
		}
	}

//	irq_disable();
	inside_int = 0;

	/* We're all done. Return however we were instructed. */
	return back_vector;
}


#ifdef LOG

void dump_regs(register_stack *stack) {
	
	LOGF(" R0 = 0x%08lx   R1 = 0x%08lx   R2 = 0x%08lx\n R3 = 0x%08lx   R4 = 0x%08lx   R5 = 0x%08lx\n",
		stack->r0, stack->r1, stack->r2, stack->r3, stack->r4, stack->r5);

	LOGF(" R6 = 0x%08lx   R7 = 0x%08lx   R8 = 0x%08lx\n R9 = 0x%08lx  R10 = 0x%08lx  R11 = 0x%08lx\n",
		stack->r6, stack->r7, stack->r8, stack->r9, stack->r10, stack->r11);
				
	LOGF("R12 = 0x%08lx  R13 = 0x%08lx  R14 = 0x%08lx\n PR = 0x%08lx   PC = 0x%08lx   SR = 0x%08lx\n",
		stack->r12, stack->r13, stack->r14, stack->pr, stack->spc, stack->ssr);
		
//	LOGF(" SGR = 0x%08lx  STACK = 0x%08lx\n", sgr(), r15());
}

#endif

