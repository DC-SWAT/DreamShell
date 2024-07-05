/** 
 *  \file   	exceptions.c
 *	\brief  	Exception handling for DreamShell based on dcplaya code
 *	\date		2004-2015
 *	\author	SWAT www.dc-swat.ru
 */

#include "ds.h"
#include <arch/irq.h>
#include "setjmp.h"
#include "drivers/asic.h"

/*
 * This is the table where context is saved when an exception occure
 * After the exception is handled, context will be restored and 
 * an RTE instruction will be issued to come back to the user code.
 * Modifying the content of the table BEFORE returning from the handler
 * of an exception let us do interesting tricks :)
 *
 * 0x00 .. 0x3c : Registers R0 to R15
 * 0x40         : SPC (return adress of RTE)
 * 0x58         : SSR (saved SR, restituted after RTE)
 *
 */
 
#define EXPT_GUARD_STACK_COUNT 4

static expt_quard_stack_t expt_stack[EXPT_GUARD_STACK_COUNT];
static kthread_key_t expt_key;
static int expt_inited = 0;


/* This is the list of exceptions code we want to handle */
static struct {
	int code;
	const char *name;
} exceptions_code[] = {
	{EXC_DATA_ADDRESS_READ, "EXC_DATA_ADDRESS_READ"},   // 0x00e0	/* Data address (read) */
	{EXC_DATA_ADDRESS_WRITE,"EXC_DATA_ADDRESS_WRITE"},  // 0x0100	/* Data address (write) */
	{EXC_USER_BREAK_PRE,    "EXC_USER_BREAK_PRE"},      // 0x01e0	/* User break before instruction */
	{EXC_ILLEGAL_INSTR,     "EXC_ILLEGAL_INSTR"},       // 0x0180	/* Illegal instruction */
	{EXC_GENERAL_FPU,       "EXC_GENERAL_FPU"},         // 0x0800	/* General FPU exception */
	{EXC_SLOT_FPU,          "EXC_SLOT_FPU"},            // 0x0820	/* Slot FPU exception */
	{0, NULL}
};


static void guard_irq_handler(irq_t source, irq_context_t *context, void *data) {

	(void)data;
	dbglog(DBG_INFO, "\n=============== CATCHING EXCEPTION ===============\n");

	irq_context_t *irq_ctx = irq_get_context();

	if (source == EXC_FPU) {
		/* Display user friendly informations */
		export_sym_t * symb;

		symb = export_lookup_by_addr(irq_ctx->pc);
		
		if (symb) {
			dbglog(DBG_INFO, "FPU EXCEPTION PC = %s + 0x%08x (0x%08x)\n", 
					symb->name, 
					((int)irq_ctx->pc) - ((int)symb->ptr), 
					(int)irq_ctx->pc);
		}

		/* skip the offending FPU instruction */
		int *ptr = (int *)&irq_ctx->r[0x40/4];
		*ptr += 4;

		return;
	}

	/* Display user friendly informations */
	export_sym_t * symb;
	int i;
	uint32 *stk = (uint32 *)irq_ctx->r[15];

	for (i = 15; i >= 0; i--) {
		
		if((stk[i] < 0x8c000000) || (stk[i] > 0x8d000000) ||
			!(symb = export_lookup_by_addr(stk[i])) ||
			((int)stk[i] - ((int)symb->ptr) > 0x800)) {

			dbglog(DBG_INFO, "STACK#%2d = 0x%08x\n", i, (int)stk[i]);

		} else {

			dbglog(DBG_INFO, "STACK#%2d = 0x%08x (%s + 0x%08x)\n", 
					i, (int)stk[i], symb->name, (int)stk[i] - ((int)symb->ptr));
		}
	}

	symb = export_lookup_by_addr(irq_ctx->pc);
	
	if (symb && (int)stk[i] - ((int)symb->ptr) < 0x800) {
		dbglog(DBG_INFO, "      PC = %s + 0x%08x (0x%08x)\n", 
				symb->name, 
				((int)irq_ctx->pc) - ((int)symb->ptr), 
				(int)irq_ctx->pc);

	} else {
		dbglog(DBG_INFO, "      PC = 0x%08x\n", (int)irq_ctx->pc);
	}

	symb = export_lookup_by_addr(irq_ctx->pr);
	
	if (symb && (int)stk[i] - ((int)symb->ptr) < 0x800) {
		dbglog(DBG_INFO, "      PR = %s + 0x%08x (0x%08x)\n", 
				symb->name, 
				((int)irq_ctx->pr) - ((int)symb->ptr), 
				(int)irq_ctx->pr);
	} else {
		dbglog(DBG_INFO, "      PR = 0x%08x\n", (int)irq_ctx->pr);
	}

	uint32 *regs = irq_ctx->r;
	dbglog(DBG_INFO, " R0-R3   = %08lx %08lx %08lx %08lx\n", regs[0], regs[1], regs[2], regs[3]);
	dbglog(DBG_INFO, " R4-R7   = %08lx %08lx %08lx %08lx\n", regs[4], regs[5], regs[6], regs[7]);
	dbglog(DBG_INFO, " R8-R11  = %08lx %08lx %08lx %08lx\n", regs[8], regs[9], regs[10], regs[11]);
	dbglog(DBG_INFO, " R12-R15 = %08lx %08lx %08lx %08lx\n", regs[12], regs[13], regs[14], regs[15]);
	//arch_stk_trace_at(regs[14], 0);
	
	for (i = 0; exceptions_code[i].code; i++) {
		if (exceptions_code[i].code == source) {
			dbglog(DBG_INFO, "   EVENT = %s (0x%08x)\n", exceptions_code[i].name, (int)source);
			break;
		}
	}
	  
	expt_quard_stack_t *s = NULL;
	s = (expt_quard_stack_t *) kthread_getspecific(expt_key);

	if (s && s->pos >= 0) {

		// Simulate a call to longjmp by directly changing stored 
		// context of the exception
		irq_ctx->pc = (uint32)longjmp;
		irq_ctx->r[4] = (uint32)s->jump[s->pos];
		irq_ctx->r[5] = (uint32)(void *) -1;

	} else {
		//malloc_stats();
		//texture_memstats();

		/* not handled --> panic !! */
		//irq_dump_regs(0, source);
		
		dbgio_set_dev_fb();
		vid_clear(0, 0, 0);
		ConsoleInformation *con = GetConsole();
		
		for(i = 16; i > 0; i--) {
			dbglog(DBG_INFO, "%s\n", con->ConsoleLines[i]);
		}
		
		dbglog(DBG_ERROR, "Unhandled Exception. Reboot after 10 seconds.");
		
		//panic("Unhandled IRQ/Exception");
		timer_spin_sleep(10000);
		arch_reboot();
//		asic_sys_reset();
	}
}


void expt_print_place(char *file, int line, const char *func) {
	dbglog(DBG_INFO, "   PLACE = %s:%d %s\n"
						"================"
						" END OF EXCEPTION "
						"================\n\n",
						file, line, func);
}


static expt_quard_stack_t *expt_get_free_stack() {
	int i;
	
	for(i = 0; i < EXPT_GUARD_STACK_COUNT; i++) {
		if(expt_stack[i].type == 0) {
			expt_stack[i].type = i + 1;
			return &expt_stack[i];
		}
	}
	
	return NULL;
}


expt_quard_stack_t *expt_get_stack() {
	
	expt_quard_stack_t *s = NULL;
	s = (expt_quard_stack_t *) kthread_getspecific(expt_key);
	
	if(s == NULL) {
		
		s = expt_get_free_stack();
		
		if(s == NULL) {
			s = (expt_quard_stack_t *) malloc(sizeof(expt_quard_stack_t));
			
			if(s == NULL) {
				EXPT_GUARD_THROW;
			}
			
			memset_sh4(s, 0, sizeof(expt_quard_stack_t));
			s->type = -1;
		}

		s->pos = -1;
		kthread_setspecific(expt_key, (void *)s);
	}

	return s;
}


static void expt_key_free(void *p) {
	
	expt_quard_stack_t *s = (expt_quard_stack_t *)p;
	
	if(s->type < 0) {
		free(p); // TODO check thread magic correct
	} else {
		memset_sh4(s, 0, sizeof(expt_quard_stack_t));
	}
}


int expt_init() {
	
    if(kthread_key_create(&expt_key, &expt_key_free)) {
        printf("Error in creating exception key for tls\n");
		return -1;
    }

	int i;
	
	for(i = 0; i < EXPT_GUARD_STACK_COUNT; i++) {
		memset_sh4(&expt_stack[i], 0, sizeof(expt_quard_stack_t));
	}

	// TODO : save old values
	for (i = 0; exceptions_code[i].code; i++)
		irq_set_handler(exceptions_code[i].code, guard_irq_handler, NULL);

	expt_inited = 1;
	return 0;
}

void expt_shutdown() {

	if(!expt_inited) {
		return;
	}
	
	int i;
	
//	for(i = 0; i < EXPT_GUARD_STACK_COUNT; i++) {
//		memset_sh4(&expt_stack[i], 0, sizeof(expt_quard_stack_t));
//	}

	for (i = 0; exceptions_code[i].code; i++)
		irq_set_handler(exceptions_code[i].code, NULL, NULL);
	
	kthread_key_delete(expt_key);
}
