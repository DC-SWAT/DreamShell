/**
 * Unlimited dynamic fgets() routine
 *
 * @file	platon/str/dynfgets.h
 * @author	Yuuki Ninomiya <gm@debian.or.jp>
 * @author	Ondrej Jombik <nepto@platon.sk>
 * @version	\$Platon: libcfg+/src/platon/str/dynfgets.h,v 1.12 2004/01/12 06:03:09 nepto Exp $
 * @date	2001-2004
 */

#ifndef _PLATON_STR_DYNFGETS_H
#define _PLATON_STR_DYNFGETS_H

#include <stdio.h>

#ifndef PLATON_FUNC
# define PLATON_FUNC(_name) _name
#endif
#ifndef PLATON_FUNC_STR
# define PLATON_FUNC_STR(_name) #_name
#endif

/** Size of input buffer. In others words, size of realloc() step. */
#define DYNAMIC_FGETS_BUFSIZE	(128)

/** Macro alias */
#define dynfgets(f)		dynamic_fgets(f)

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * Dynamic fgets() with unlimited line length
	 *
	 * @param	fp	stream (FILE * pointer)
	 * @return	dynamically allocated buffer or NULL on not enough memory error
	 */
	char *PLATON_FUNC(dynamic_fgets)(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PLATON_STR_DYNFGETS_H */

