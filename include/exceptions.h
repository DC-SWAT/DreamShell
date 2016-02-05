/** 
 * \file    exceptions.h
 * \brief   Exception handling for DreamShell
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */
     
#ifndef _DS_EXECEPTIONS_H_
#define _DS_EXECEPTIONS_H_

#include <kos/thread.h>
#include "setjmp.h"


#define USE_DS_EXCEPTIONS          1
#define EXPT_GUARD_STACK_SIZE      16

#define EXPT_GUARD_ST_DYNAMIC     -1
#define EXPT_GUARD_ST_STATIC_FREE  0

typedef struct expt_guard_stack {
	
	int pos;
	jmp_buf jump[EXPT_GUARD_STACK_SIZE];
	
	/**
	 * If equal to EXPT_GUARD_ST_DYNAMIC, then dynamic. 
	 * If equal to EXPT_GUARD_ST_STATIC_FREE, then static and not used
	 * Otherwise index in static
	 */
	int type;
	
} expt_quard_stack_t;


/** 
 * Initialize the exception system. 
 */
int expt_init();

/** 
 * Shutdown the exception system.
 */
void expt_shutdown();

/** 
 * Get guard stack from current thread.
 */
expt_quard_stack_t *expt_get_stack();

/**
 * Print catched exception place
 */
void expt_print_place(char *file, int line, const char *func);


/** \name Protected section.
 *
 *  To protect a code from exception :
 * \code
 *  char * buffer = malloc(32);
 *  EXPT_GUARD_BEGIN;
 *  // Run code to protect here. This instruction makes a bus error or
 *  // something like that on most machine.
 *  *(int *) (buffer+1) = 0xDEADBEEF;
 *
 *  EXPT_GUARD_CATCH;
 *  // Things to do if hell happen
 *  printf("Error\n");
 *  free(buffer);
 *  return -1;
 *
 *  EXPT_GUARD_END;
 *  // Life continue ... 
 *  memset(buffer,0,32);
 *  // ...
 * \endcode
 *
 */


#ifdef USE_DS_EXCEPTIONS

/**
 * Throw exception.
 */
#define EXPT_GUARD_THROW *(int *)1 = 0xdeadbeef

/**
 * Get exceptions stack from current thread.
 */
#define EXPT_GUARD_INIT	expt_quard_stack_t *__expt = expt_get_stack()

/** 
 * Start a protected section.
 * \warning : it is FORBIDEN to do "return" inside a guarded section, 
 * use EXPT_GUARD_RETURN instead. 
 */
#define EXPT_GUARD_BEGIN_NEXT                                   \
	do {                                                        \
		__expt->pos++;                                          \
		if (__expt->pos >= EXPT_GUARD_STACK_SIZE)               \
			EXPT_GUARD_THROW;                                   \
		if (!setjmp(__expt->jump[__expt->pos])) {               \
	
/** 
 * Get stack and start a protected section.
 */
#define EXPT_GUARD_BEGIN                                        \
	EXPT_GUARD_INIT;                                            \
	EXPT_GUARD_BEGIN_NEXT

/** 
 * Catch a protected section.
 */
#define EXPT_GUARD_CATCH                                        \
		} else {                                                \
			expt_print_place(__FILE__, __LINE__, __FUNCTION__)

/** 
 * End of protected section.
 */
#define EXPT_GUARD_END                                          \
		}                                                       \
		__expt->pos--;                                          \
	} while(0)

/** 
 * Return in middle of a guarded section. 
 * \warning : to be used exclusively inbetween EXPT_GUARD_BEGIN and
 * EXPT_GUARD_END. Never use normal "return" in this case. 
 */
#define EXPT_GUARD_RETURN __expt->pos--; return


#define EXPT_GUARD_ASSIGN(dst, src, catched)                    \
	EXPT_GUARD_BEGIN;                                           \
		dst = src;                                              \
	EXPT_GUARD_CATCH;                                           \
		catched;                                                \
	EXPT_GUARD_END


#else /* ifdef USE_DS_EXCEPTIONS */


#define EXPT_GUARD_THROW

#define EXPT_GUARD_BEGIN                                        \
  if (1) {                                                      \

#define EXPT_GUARD_CATCH                                        \
    } else {                                                    \

#define EXPT_GUARD_END }

#define EXPT_GUARD_RETURN                                       \
  return

#define EXPT_GUARD_ASSIGN(dst, src, errval) dst = src

#endif /* ifdef USE_DS_EXCEPTIONS */

#endif /* ifndef _DS_EXECEPTIONS_H_*/
