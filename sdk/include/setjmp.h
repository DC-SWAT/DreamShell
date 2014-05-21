/** 
 * \file    setjmp.h
 * \brief   setjmp
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _JMPBUF_H_
#define _JMPBUF_H_

typedef char jmp_buf[13*4];


int longjmp(jmp_buf buf, int what_is_that);
int setjmp(jmp_buf buf);

#endif
