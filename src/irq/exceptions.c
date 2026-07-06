/** 
 *  \file   exceptions.c
 *	\brief  Exception handling for DreamShell
 *	\date	2004-2015, 2026
 *	\author	SWAT www.dc-swat.ru
 */

#include "ds.h"
#include <arch/irq.h>
#include <arch/arch.h>
#include <arch/stack.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <dc/biosfont.h>
#include <dc/video.h>
#include <dc/memory.h>
#include <dc/pvr/pvr_regs.h>
#include "setjmp.h"

#define EXPT_SYM_MAX_OFFSET 0x800

#define EXPT_FB_MARGIN_X    32
#define EXPT_FB_MARGIN_Y    32
#define EXPT_FB_LINE_H      BFONT_HEIGHT
#define EXPT_FB_BUF_SIZE    128
#define EXPT_FB_MAX_LINES   (480 / BFONT_HEIGHT)

#define EXPT_FB_WIDTH       640
#define EXPT_FB_HEIGHT      480

static int expt_fb_y = EXPT_FB_MARGIN_Y;
static int expt_fb_stride = EXPT_FB_WIDTH;
static int expt_fb_h = EXPT_FB_HEIGHT;
static int expt_fb_inited = 0;
static char expt_fb_lines[EXPT_FB_MAX_LINES][EXPT_FB_BUF_SIZE];
static int expt_fb_line_count = 0;

static void expt_fb_draw_char(int x, int y, int c) {
	uint8_t *ch;
	uint16_t *buffer;
	uint16_t word;
	uint16_t fg = 0xFFFF;
	uint16_t bg = 0x0000;
	int row_x, row_y;

	if(!vram_s)
		return;

	ch = bfont_find_char(c);

	if(!ch)
		return;

	buffer = vram_s + (y * expt_fb_stride + x);

	for(row_y = 0; row_y < BFONT_HEIGHT; ) {
		word = (((uint16_t)ch[0]) << 4) | ((ch[1] >> 4) & 0x0f);

		for(row_x = 0; row_x < BFONT_THIN_WIDTH; row_x++) {
			buffer[row_x] = (word & (0x0800 >> row_x)) ? fg : bg;
		}

		buffer += expt_fb_stride;
		row_y++;

		word = ((((uint16_t)ch[1]) << 8) & 0xf00) | ch[2];

		for(row_x = 0; row_x < BFONT_THIN_WIDTH; row_x++) {
			buffer[row_x] = (word & (0x0800 >> row_x)) ? fg : bg;
		}

		buffer += expt_fb_stride;
		row_y++;
		ch += 3;
	}
}

static void expt_fb_clear(void) {
	uint16_t *fb = vram_s;
	size_t i;
	size_t pixels = (size_t)expt_fb_stride * (size_t)expt_fb_h;

	if(!fb)
		return;

	for(i = 0; i < pixels; i++) {
		fb[i] = 0;
	}
}

static void expt_fb_init(void) {
	uint32_t fb_addr;
	uint32_t fb_cfg;

	if(expt_fb_inited)
		return;

	fb_addr = PVR_GET(PVR_FB_ADDR) & (PVR_RAM_SIZE - 1);
	fb_cfg = PVR_GET(PVR_FB_CFG_1);

	PVR_SET(PVR_RESET, PVR_RESET_ALL);
	PVR_SET(PVR_RESET, PVR_RESET_NONE);

	PVR_SET(PVR_FB_ADDR, fb_addr);
	PVR_SET(PVR_FB_CFG_1, fb_cfg | 1);
	PVR_SET(PVR_VIDEO_CFG, PVR_GET(PVR_VIDEO_CFG) & ~8);

	if(vid_mode) {
		expt_fb_stride = vid_mode->width;
		expt_fb_h = vid_mode->height;
	}
	else {
		expt_fb_stride = EXPT_FB_WIDTH;
		expt_fb_h = EXPT_FB_HEIGHT;
	}

	vid_set_vram(fb_addr);

	expt_fb_clear();
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	expt_fb_y = EXPT_FB_MARGIN_Y;
	expt_fb_inited = 1;
}

static void expt_fb_print_line(const char *str) {
	int x = EXPT_FB_MARGIN_X;
	int max_chars;
	int i, len;

	if(!str)
		return;

	if(!expt_fb_inited)
		expt_fb_init();

	if(!vram_s)
		return;

	len = strlen(str);

	while(len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
		len--;
	}

	max_chars = (expt_fb_stride - EXPT_FB_MARGIN_X * 2) / BFONT_THIN_WIDTH;

	for(i = 0; i < len && i < max_chars; i++) {
		expt_fb_draw_char(x, expt_fb_y, str[i]);
		x += BFONT_THIN_WIDTH;
	}

	expt_fb_y += EXPT_FB_LINE_H;
}

static void expt_fb_collect(const char *fmt, ...) {
	char buf[EXPT_FB_BUF_SIZE];
	va_list args;

	if(expt_fb_line_count >= EXPT_FB_MAX_LINES)
		return;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	strncpy(expt_fb_lines[expt_fb_line_count], buf, EXPT_FB_BUF_SIZE - 1);
	expt_fb_lines[expt_fb_line_count][EXPT_FB_BUF_SIZE - 1] = '\0';
	expt_fb_line_count++;
}

static intptr_t expt_sym_offset(export_sym_t *symb, uintptr_t addr) {
	return (intptr_t)(addr - symb->ptr);
}

static bool expt_sym_valid(export_sym_t *symb, uintptr_t addr) {
	intptr_t off;

	if(!symb)
		return false;

	off = expt_sym_offset(symb, addr);
	return off >= 0 && off < EXPT_SYM_MAX_OFFSET;
}

static bool expt_is_data_fault(irq_t source) {
	return source == EXC_DATA_ADDRESS_READ || source == EXC_DATA_ADDRESS_WRITE;
}

static uint32_t expt_get_fault_addr(void) {
	return *((volatile uint32_t *)SH4_REG_MMU_TEA);
}

static void expt_log_symbol_addr(const char *label, uintptr_t addr) {
	export_sym_t *symb = export_lookup_addr(addr);

	if(expt_sym_valid(symb, addr)) {
		dbglog(DBG_INFO, " %7s = %s + 0x%08" PRIxPTR " (0x%08" PRIxPTR ")\n",
			label, symb->name, (uintptr_t)expt_sym_offset(symb, addr), addr);
	}
	else {
		dbglog(DBG_INFO, " %7s = 0x%08" PRIxPTR "\n", label, addr);
	}
}

static void expt_log_context(void) {
	if(thd_current) {
		const char *label = thd_get_label(thd_current);

		dbglog(DBG_INFO, "  THREAD = %s (%d)\n",
			label ? label : "?", thd_current->tid);
	}

	if(GetCurApp()) {
		dbglog(DBG_INFO, "     APP = %s\n", GetCurApp()->name);
	}
}

static void expt_log_addr2line_hint(irq_context_t *irq_ctx) {
	bool valid_pc = arch_valid_text_address(irq_ctx->pc);
	bool valid_pr = arch_valid_text_address(irq_ctx->pr);

	if(!valid_pc && !valid_pr)
		return;

	dbglog(DBG_INFO, "ADDR2LINE: $KOS_ADDR2LINE -f -C -i -e ds.elf");

	if(valid_pc)
		dbglog(DBG_INFO, " %08lx", irq_ctx->pc);

	if(valid_pr)
		dbglog(DBG_INFO, " %08lx", irq_ctx->pr);

	dbglog(DBG_INFO, "\n");
}

static void expt_fb_collect_symbol_addr(const char *label, uintptr_t addr) {
	export_sym_t *symb = export_lookup_addr(addr);

	if(expt_sym_valid(symb, addr)) {
		expt_fb_collect("%s=%s+0x%" PRIxPTR, label, symb->name,
			(uintptr_t)expt_sym_offset(symb, addr));
	}
	else {
		expt_fb_collect("%s=%08" PRIxPTR, label, addr);
	}
}

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
	irq_t code;
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

static void expt_fb_collect_dump(irq_context_t *irq_ctx, irq_t source) {
	uint32_t *stk = (uint32_t *)(uintptr_t)irq_ctx->r[15];
	uint32_t *regs = irq_ctx->r;
	int i;

	expt_fb_line_count = 0;
	expt_fb_collect("=== DreamShell Catching Exception ===");

	for (i = 15; i >= 0; i -= 2) {
		if (i > 0) {
			expt_fb_collect("#%02d=%08" PRIx32 " #%02d=%08" PRIx32,
				i, stk[i], i - 1, stk[i - 1]);
		}
		else {
			expt_fb_collect("#%02d=%08" PRIx32, i, stk[i]);
		}
	}

	expt_fb_collect_symbol_addr("PC", irq_ctx->pc);
	expt_fb_collect_symbol_addr("PR", irq_ctx->pr);

	if(expt_is_data_fault(source)) {
		expt_fb_collect("FAULT=%08" PRIx32, expt_get_fault_addr());
	}

	expt_fb_collect("R0-7  %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32,
		regs[0], regs[1], regs[2], regs[3]);
	expt_fb_collect("R8-15 %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32,
		regs[4], regs[5], regs[6], regs[7]);
	expt_fb_collect("      %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32,
		regs[8], regs[9], regs[10], regs[11]);
	expt_fb_collect("      %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32,
		regs[12], regs[13], regs[14], regs[15]);

	if(thd_current) {
		const char *label = thd_get_label(thd_current);

		if(GetCurApp()) {
			expt_fb_collect("THD=%s APP=%s", label ? label : "?", GetCurApp()->name);
		}
		else {
			expt_fb_collect("THD=%s", label ? label : "?");
		}
	}
	else if(GetCurApp()) {
		expt_fb_collect("APP=%s", GetCurApp()->name);
	}

	for (i = 0; exceptions_code[i].code; i++) {
		if (exceptions_code[i].code == source) {
			expt_fb_collect("EVENT: %s", exceptions_code[i].name);
			break;
		}
	}
}

static void expt_fb_show(void) {
	int i;

	expt_fb_init();

	for(i = 0; i < expt_fb_line_count; i++) {
		expt_fb_print_line(expt_fb_lines[i]);
	}
}

static void guard_irq_handler(irq_t source, irq_context_t *context, void *data) {

	(void)data;
	dbglog(DBG_INFO, "\n========== DreamShell Catching Exception ==========\n");

	irq_context_t *irq_ctx = irq_get_context();

	if (source == EXC_FPU) {
		expt_log_symbol_addr("FPU EXCEPTION PC", irq_ctx->pc);

		/* skip the offending FPU instruction */
		uint32_t *spc = &irq_ctx->r[0x40 / sizeof(uint32_t)];
		*spc += 4;

		return;
	}

	expt_log_context();

	expt_quard_stack_t *s = (expt_quard_stack_t *) kthread_getspecific(expt_key);
	bool guarded = (s && s->pos >= 0);

	if(!guarded) {
		expt_fb_collect_dump(irq_ctx, source);
		expt_fb_collect("Unhandled Exception. Reboot after 3 seconds.");
		expt_fb_show();
	}

	/* Display user friendly informations */
	export_sym_t *symb;
	int i;
	uint32_t *stk = (uint32_t *)(uintptr_t)irq_ctx->r[15];

	for (i = 15; i >= 0; i--) {
		symb = export_lookup_addr(stk[i]);

		if(!arch_valid_address((uintptr_t)stk[i]) || !expt_sym_valid(symb, stk[i])) {
			dbglog(DBG_INFO, "STACK#%2d = 0x%08" PRIx32 "\n", i, stk[i]);
		}
		else {
			dbglog(DBG_INFO, "STACK#%2d = 0x%08" PRIx32 " (%s + 0x%08" PRIxPTR ")\n",
				i, stk[i], symb->name, (uintptr_t)expt_sym_offset(symb, stk[i]));
		}
	}

	expt_log_symbol_addr("PC", irq_ctx->pc);
	expt_log_symbol_addr("PR", irq_ctx->pr);

	{
		uint32_t *regs = irq_ctx->r;

		dbglog(DBG_INFO, " R0-R3   = %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
			regs[0], regs[1], regs[2], regs[3]);
		dbglog(DBG_INFO, " R4-R7   = %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
			regs[4], regs[5], regs[6], regs[7]);
		dbglog(DBG_INFO, " R8-R11  = %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
			regs[8], regs[9], regs[10], regs[11]);
		dbglog(DBG_INFO, " R12-R15 = %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
			regs[12], regs[13], regs[14], regs[15]);

		if(expt_is_data_fault(source)) {
			dbglog(DBG_INFO, "  FAULT = 0x%08" PRIx32 "\n", expt_get_fault_addr());
		}

		for (i = 0; exceptions_code[i].code; i++) {
			if (exceptions_code[i].code == source) {
				dbglog(DBG_INFO, "   EVENT = %s (0x%04" PRIx32 ")\n",
					exceptions_code[i].name, (uint32_t)source);
				break;
			}
		}

		arch_stk_trace_at(regs[14], 0);
	}
	expt_log_addr2line_hint(irq_ctx);

	if(guarded) {

		// Simulate a call to longjmp by directly changing stored 
		// context of the exception
		irq_ctx->pc = (uint32_t)(uintptr_t)longjmp;
		irq_ctx->r[4] = (uint32_t)(uintptr_t)s->jump[s->pos];
		irq_ctx->r[5] = (uint32_t)-1;

	}
	else {

		dbglog(DBG_ERROR, "Unhandled Exception. Reboot after 3 seconds.");

		uint64_t timeout = timer_ms_gettime64() + 3000;
		while(timer_ms_gettime64() < timeout);

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
