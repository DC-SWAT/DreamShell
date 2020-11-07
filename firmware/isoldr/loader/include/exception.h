/**
 * DreamShell ISO Loader
 * Exception handling
 * (c)2014-2020 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <main.h>
#include <exception-lowlevel.h>

#ifdef USE_GDB
#	define EXP_TABLE_SIZE 8
#else
#	define EXP_TABLE_SIZE 1
#endif

typedef void *(* exception_handler_f) (register_stack *, void *);


typedef struct {
	
    uint32 type;
    uint32 code;
    exception_handler_f handler;
	
} exception_table_entry;


typedef struct {
	
    /* Exception counters */
//  uint32 general_exception_count;
//  uint32 cache_exception_count;
//  uint32 interrupt_exception_count;
//  uint32 ubc_exception_count;
//  uint32 odd_exception_count;

    /* Function hooks for various interrupts */
    exception_table_entry table[EXP_TABLE_SIZE];
	
} exception_table;


int exception_init(uint32 vbr_addr);
int exception_inited(void);

int exception_add_handler(const exception_table_entry *new_entry, exception_handler_f *parent_handler);
void *exception_handler(register_stack *stack);

int exception_inside_int(void);

void dump_regs(register_stack *stack);

#endif
