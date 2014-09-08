/*
 * libcfg+ - precise command line & config file parsing library
 *
 * shared.c - shared stuff for command line and config file
 * ____________________________________________________________
 *
 * Developed by Ondrej Jombik <nepto@platon.sk>
 *          and Lubomir Host <rajo@platon.sk>
 * Copyright (c) 2001-2004 Platon SDG, http://platon.sk/
 * All rights reserved.
 *
 * See README file for more information about this software.
 * See COPYING file for license information.
 *
 * Download the latest version from
 * http://platon.sk/projects/libcfg+/
 */

/* $Platon: libcfg+/src/shared.c,v 1.36 2004/01/12 06:03:09 nepto Exp $ */

/* Includes {{{ */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if STDC_HEADERS
#  include <stdlib.h>
#else
#  if HAVE_STDLIB_H
#    include <stdlib.h>
#  endif
#endif
#if HAVE_STRING_H
#  if !STDC_HEADERS && HAVE_MEMORY_H
#    include <memory.h>
#  endif
#  include <string.h>
#endif
#if HAVE_STRINGS_H
#  include <string.h>
#endif
#if HAVE_MATH_H
#  include <math.h>
#endif
#if HAVE_LIMITS_H
#  include <limits.h>
#endif
#if HAVE_FLOAT_H
#  include <float.h>
#endif
#if HAVE_CTYPE_H
#  include <ctype.h>
#endif
#if HAVE_ERRNO_H
#  include <errno.h>
#endif

#include "platon/str/strplus.h"
#include "platon/str/strdyn.h"

#include "cfg+.h"
/* }}} */

/* Static function declarations {{{ */
static int search_cur_opt_idx(const CFG_CONTEXT con);
static int add_to_used_opt_idx(const CFG_CONTEXT con, int opt_idx);
static int store_multi_arg(
		const int	type,
		const char	**multi_arg,
		void		***ar);
static int store_single_arg(
		const int	type,
		const char	*arg,
		const void	*where);
static int split_multi_arg(
		char		*arg,
		char		***ar,
		char		**quote_prefix_ar,
		char		**quote_postfix_ar,
		char		**separator_ar);
static int unquote_single_arg(
		char		*arg,
		char		**quote_prefix_ar,
		char		**quote_postfix_ar);
/* }}} */

/*
 * EXTERN PLATON_FUNCS
 */

	void
__cfg_free_currents(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	if (con->cur_opt != NULL)
		free(con->cur_opt);
	if (con->cur_arg != NULL)
		free(con->cur_arg);

	con->cur_opt = con->cur_arg = NULL;
	con->cur_opt_type = CFG_NONE_OPTION;
} /* }}} */

	int
__cfg_process_currents(con, ret_val, arg_used)
	const CFG_CONTEXT con;
	int               *ret_val;
	int               *arg_used;
{ /* {{{ */
	register int ret = 0;
	register int opt_idx;
	register int opt_type, opt_data_type, opt_f_multi, opt_f_multi_sep;
	register char **quote_prefix_ar, **quote_postfix_ar, **separator_ar;

	*ret_val = 0;
	if (arg_used != NULL)
		*arg_used = 0;

	opt_idx = search_cur_opt_idx(con);

#if defined(DEBUG) && DEBUG
	fprintf(stderr, "%s|%ld|%s|%s|%s|%d|flags=%x[",
			con->type == CFG_LINE ? "cmdline" : "cfgfile",
			con->type == CFG_LINE ? con->cur_idx : 0,
			con->type == CFG_LINE ? con->argv[con->cur_idx] : "N/A",
			con->cur_opt == NULL ? "[NULL]" : con->cur_opt,
			con->cur_arg == NULL ? "[NULL]" : con->cur_arg,
			opt_idx, con->cur_opt_type);

	if (con->cur_opt_type & CFG_LONG_OPTION)
		fputs("-LONG-", stderr);
	if (con->cur_opt_type & CFG_SHORT_OPTION)
		fputs("-SHORT-", stderr);
	if (con->cur_opt_type & CFG_SHORT_OPTIONS)
		fputs("-SHORTS-", stderr);
	if (con->cur_opt_type & CFG_LONG_SEPINIT)
		fputs("-SEPINIT-", stderr);

	fputs("]\n", stderr);
#endif

	if (opt_idx < 0)
		return CFG_ERROR_UNKNOWN; /* unknown option reached */

	if (! (con->flags & CFG_IGNORE_MULTI)
			&& ! (con->options[opt_idx].type & CFG_MULTI)) {
		ret = add_to_used_opt_idx(con, opt_idx);

		if (ret < 0)
			return CFG_ERROR_NOMEM;
		else if (ret > 0)
			return CFG_ERROR_MULTI;
	}

	/* Setting others opt_XXX constants according to *opt_idx. */
	/* opt_data_type - option data type, opt_f_XXX - option flags */
	opt_type        = con->options[opt_idx].type;
	opt_data_type   = opt_type & CFG_DATA_TYPE_MASK;
	opt_f_multi     = opt_type & CFG_MULTI;
	opt_f_multi_sep = opt_type & (CFG_MULTI_SEPARATED & ~CFG_MULTI);

	if (con->type == CFG_LINE) {
		quote_prefix_ar  = con->prop[CFG_LINE_QUOTE_PREFIX];
		quote_postfix_ar = con->prop[CFG_LINE_QUOTE_POSTFIX];
		separator_ar     = (opt_type & CFG_LEFTOVER_ARGS)
			? con->prop[CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR]
			: con->prop[CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR];
	}
	else {
		quote_prefix_ar  = con->prop[CFG_FILE_QUOTE_PREFIX];
		quote_postfix_ar = con->prop[CFG_FILE_QUOTE_POSTFIX];
		separator_ar     = (opt_type & CFG_LEFTOVER_ARGS)
			? con->prop[CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR]
			: con->prop[CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR];
	}

	switch (opt_data_type) {
		default:
			return CFG_ERROR_INTERNAL;
			break;

			/* Boolean data type */

		case CFG_BOOLEAN:
			if (con->cur_opt_type & CFG_LONG_SEPINIT
					&& con->cur_arg != NULL)
				return CFG_ERROR_NOTALLOWEDARG;

			if (con->options[opt_idx].value != NULL)
				(*((int *) con->options[opt_idx].value))++;
			break;

			/* Numeric (int, long, float and double) data types */

		case CFG_INT:
		case CFG_UINT:
		case CFG_LONG:
		case CFG_ULONG:
		case CFG_FLOAT:
		case CFG_DOUBLE:

			if (con->cur_arg == NULL)
				return CFG_ERROR_NOARG;

			if (opt_f_multi) {
				char **add;
				char *static_add[2] = {NULL, NULL};

				if (opt_f_multi_sep) {

					ret = split_multi_arg(con->cur_arg, &add,
							quote_prefix_ar, quote_postfix_ar, separator_ar);

					if (ret != CFG_OK) {
						PLATON_FUNC(strdyn_safe_free)(add);
						return ret;
					}
				}
				else {
					add = static_add;
					add[0] = con->cur_arg;
				}

				ret = store_multi_arg(opt_data_type,
						(const char **) add,
						con->options[opt_idx].value);

				if (add != static_add)
					PLATON_FUNC(strdyn_free)((char **) add);
			}
			else {
				ret = unquote_single_arg(con->cur_arg,
						quote_prefix_ar, quote_postfix_ar);

				if (ret != CFG_OK)
					return ret;

				ret = store_single_arg(opt_data_type, con->cur_arg,
						con->options[opt_idx].value);
			}

			if (ret != CFG_OK)
				return ret;

			if (arg_used != NULL)
				*arg_used = 1;
			break;

			/* String data type */

		case CFG_STRING:

			if (con->cur_arg == NULL)
				return CFG_ERROR_NOARG;

			if (con->options[opt_idx].value != NULL) {
				if (opt_f_multi) {
					register char ***p;
					p = (char ***) con->options[opt_idx].value;

					if (opt_f_multi_sep) {
						char **ar;
						ret = split_multi_arg(con->cur_arg, &ar,
								quote_prefix_ar, quote_postfix_ar,
								separator_ar);

						if (ret != CFG_OK) {
							PLATON_FUNC(strdyn_safe_free)(ar);
							return ret;
						}

						if ((*p = PLATON_FUNC(strdyn_add_ar)(*p, ar)) == NULL)
							return CFG_ERROR_NOMEM;

						PLATON_FUNC(strdyn_free)(ar);
					}
					else {
						ret = unquote_single_arg(con->cur_arg,
								quote_prefix_ar, quote_postfix_ar);

						if (ret != CFG_OK)
							return ret;

						if ((*p = PLATON_FUNC(strdyn_add)(*p, con->cur_arg)) == NULL)
							return CFG_ERROR_NOMEM;
					}
				}
				else {
					ret = unquote_single_arg(con->cur_arg,
							quote_prefix_ar, quote_postfix_ar);

					if (ret != CFG_OK)
						return ret;

					*((char **) con->options[opt_idx].value) =
						strdup(con->cur_arg);
				}
			}

			if (arg_used != NULL)
				*arg_used = 1;
			break;
	}

	*ret_val = con->options[opt_idx].val;

	return CFG_OK;
} /* }}} */

	int
__cfg_cmdline_set_currents(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	register int i, size;
	register char *s, *s_sep, *s_tmp;
	register char *ptr;

	size = -1; /* size of matched prefix (0 is also valid) */
	s = con->argv[con->cur_idx]; /* string to scan */

	/* Explicit `size_t' to `int' typecastings are required here near strlen()
	   calls, else strlen("") == 0 will be considered as smaller than -1. */

	for (i = 0; (ptr = con->prop[CFG_LINE_SHORT_OPTION_PREFIX][i]) != NULL; i++)
		if ((int) strlen(ptr) > size && strstr(s, ptr) == s) {
			size = strlen(ptr);
			con->cur_opt_type = CFG_SHORT_OPTION;
		}

	for (i = 0; (ptr = con->prop[CFG_LINE_LONG_OPTION_PREFIX][i]) != NULL; i++)
		if ((int) strlen(ptr) > size && strstr(s, ptr) == s) {
			size = strlen(ptr);
			con->cur_opt_type = CFG_LONG_OPTION;
		}

	s_sep = NULL;

	switch (con->cur_opt_type) {
		default:
		case CFG_NONE_OPTION: /* None option prefix */
			break;

		case CFG_SHORT_OPTION: /* Short option prefix */
			s += size;
			s_sep = strlen(s) > 0 ? s + 1 : s;

			if (strlen(s_sep) > 0) {
				con->cur_opt_type += CFG_SHORT_OPTIONS;

				if ((con->cur_arg = strdup(s_sep)) == NULL)
					return CFG_ERROR_NOMEM;
			}
			else {
				if (con->argv[con->cur_idx + 1] != NULL) {
					con->cur_arg = strdup(con->argv[con->cur_idx + 1]);
					if (con->cur_arg == NULL)
						return CFG_ERROR_NOMEM;
				}
				else
					con->cur_arg = NULL;
			}
			break;

		case CFG_LONG_OPTION: /* Long option prefix */
			s += size;
			size = 0;

			for (i = 0; (ptr = con->prop[CFG_LINE_OPTION_ARG_SEPARATOR][i])
					!= NULL; i++)
				if ((s_tmp = strstr(s, ptr)) != NULL)
					if (s_sep == NULL || s_tmp < s_sep
							|| (s_tmp == s_sep && strlen(ptr) > size)) {
						s_sep = s_tmp;
						size = strlen(ptr);
					}

			if (s_sep == NULL) {
				if (con->argv[con->cur_idx + 1] != NULL) {
					con->cur_arg = strdup(con->argv[con->cur_idx + 1]);
					if (con->cur_arg == NULL)
						return CFG_ERROR_NOMEM;
				}
				else
					con->cur_arg = NULL;
			}
			else {
				con->cur_opt_type += CFG_LONG_SEPINIT;
				if ((con->cur_arg = strdup(s_sep + size)) == NULL)
					return CFG_ERROR_NOMEM;
			}
	}

	if (s_sep == NULL)
		s_sep = s + strlen(s);

	con->cur_opt = (char *) malloc((s_sep - s + 1) * sizeof(char));
	if (con->cur_opt == NULL)
		return CFG_ERROR_NOMEM;

	strncpy(con->cur_opt, s, s_sep - s);
	con->cur_opt[s_sep - s] = '\0';

	if (con->cur_opt_type == CFG_NONE_OPTION) {
		register char *tmp_str;
		tmp_str      = con->cur_opt;
		con->cur_opt = con->cur_arg;
		con->cur_arg = tmp_str;
	}

	return CFG_OK;
} /* }}} */

	int
__cfg_cfgfile_set_currents(con, buf)
	const CFG_CONTEXT con;
	char              *buf;
{ /* {{{ */
	register char **pos;
	register char *s_sep, *s_tmp;
	register int size;

	s_sep = NULL;
	size  = 0;
	for (pos = con->prop[CFG_FILE_OPTION_ARG_SEPARATOR];
			pos != NULL && *pos != NULL; pos++) {

		if ((s_tmp = strstr(buf, *pos)) != NULL)
			if (s_sep == NULL || s_tmp < s_sep
					|| (s_tmp == s_sep && strlen(*pos) > size)) {
				s_sep = s_tmp;
				size = strlen(*pos);
			}
	}

	if (s_sep == NULL) {
		con->cur_arg = NULL;

		con->cur_opt = strdup(buf);
		if (con->cur_opt == NULL)
			return CFG_ERROR_NOMEM;
	}
	else {
		con->cur_opt = (char *) malloc((s_sep - buf + 1) * sizeof(char));
		if (con->cur_opt == NULL)
			return CFG_ERROR_NOMEM;

		strncpy(con->cur_opt, buf, s_sep - buf);
		con->cur_opt[s_sep - buf] = '\0';

		if ((con->cur_arg = strdup(s_sep + size)) == NULL)
			return CFG_ERROR_NOMEM;

		PLATON_FUNC(str_right_trim)(con->cur_opt);
		PLATON_FUNC(str_left_trim)(con->cur_arg);
	}

	return CFG_OK;
} /* }}} */

/*
 * STATIC PLATON_FUNCS
 */

/*
 * search_cur_opt_idx()
 *
 * Function searches con->options for con->cur_opt.
 * Returns index on success or -1 if not found.
 */

	static int
search_cur_opt_idx(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	register int i;

	for (i = 0; con->options[i].cmdline_long_name != NULL
			|| con->options[i].cmdline_short_name != '\0'
			|| con->options[i].cfgfile_name != NULL
			|| con->options[i].type != CFG_END
			|| con->options[i].value != NULL
			|| con->options[i].val != 0;
			i++)
	{
		if (con->type == CFG_CMDLINE) { /* Command line context type */
			if ((con->cur_opt_type & CFG_LONG_OPTION && con->cur_opt != NULL
						&& con->options[i].cmdline_long_name != NULL
						&& ! strcmp(con->cur_opt,
							con->options[i].cmdline_long_name))
					|| (con->cur_opt_type & CFG_SHORT_OPTION &&
						con->cur_opt[0] != '\0' && /* to prevent '-' option */
						con->cur_opt[0] == con->options[i].cmdline_short_name)
					|| (con->cur_opt_type == CFG_NONE_OPTION &&
						con->options[i].type & CFG_LEFTOVER_ARGS))

				return i;
		}
		else { /* Configuration file context type */
			int len;

			if (con->cur_opt != NULL
					&& con->options[i].cfgfile_name != NULL
					&& con->cur_opt == PLATON_FUNC(str_white_str)(con->cur_opt,
						(char *)(con->options[i].cfgfile_name), &len)
					&& len == strlen(con->cur_opt))

				return i;
		}
	}

	return -1;
} /* }}} */


/*
 * Size of realloc() step for used_opt member of cfg_context structure.
 * Value must be grater than 0.
 */

#ifdef CFG_USED_OPT_IDX_STEP
#	undef CFG_USED_OPT_IDX_STEP
#endif

#if defined(DEBUG) && DEBUG
#	define CFG_USED_OPT_IDX_STEP	(1)
#else
#	define CFG_USED_OPT_IDX_STEP	(10)
#endif

/*
 * add_to_used_opt_idx()
 *
 * Function adds opt_idx to dynamic array con->used_opt_idx. If array is full,
 * it is realloc()-ed according to CFG_USED_OPT_IDX_STEP constant. Array
 * used_opt_idx is primary used for detecting multiple options on command line.
 *
 * Note, that in array -1 means empty cell, which can be overwritten by option
 * index value and -255 means end of array. If there is no -1 left in array,
 * array must be resized to add new option index value.
 *
 * Retuns -1 on not enough memory error, 1 if opt_idx founded in array and 0 if
 * opt_idx was sucessfully added to array.
 */

	static int
add_to_used_opt_idx(con, opt_idx)
	const CFG_CONTEXT con;
	int               opt_idx;
{ /* {{{ */
	int *p_i = NULL;

	if (opt_idx < 0)
		return 1;

	if (con->used_opt_idx != NULL)
		for (p_i = con->used_opt_idx; *p_i >= 0; p_i++)
			if (*p_i == opt_idx)
				return 1;

	if (p_i == NULL || *p_i == -255) {
		register int *p_j;
		register int new_size;

		new_size = (p_i == NULL ? 0 : p_i - con->used_opt_idx)
			+ 1 + CFG_USED_OPT_IDX_STEP;

		con->used_opt_idx = (int *) realloc(con->used_opt_idx,
				new_size * sizeof(int));

#if defined(DEBUG) && DEBUG
		printf("add_to_used_opt_idx(con, %d): realloc(%d) = %p\n",
				opt_idx, new_size, (void *) con->used_opt_idx);
#endif

		if (con->used_opt_idx == NULL)
			return -1;

		if (p_i == NULL)
			p_i = con->used_opt_idx;
		else /* Searching for new p_i after realloc(). */
			for (p_i = con->used_opt_idx; *p_i >= 0; p_i++) ;

		for (p_j = p_i; p_j - p_i < CFG_USED_OPT_IDX_STEP; p_j++)
			*p_j = -1;

		*p_j = -255;
	}

	*p_i = opt_idx;

	return 0;
} /* }}} */

/*
 * store_multi_arg()
 *
 * According to option date type (type) parses and stores multiple arguments
 * (multi_arg) by adding to *ar dynamic array. Array resizing is done as first
 * and than is store_single_arg() function called with appropriate parameters.
 */

	static int
store_multi_arg(type, multi_arg, ar)
	const int  type;
	const char **multi_arg;
	void       ***ar;
{ /* {{{ */
	register int ptr_len, item_len;
	register int size_plus, size;
	register int k, ret;

	switch (type) {
		default:
			return CFG_ERROR_INTERNAL;
			break;

		case CFG_INT:
			ptr_len  = sizeof(int *);
			item_len = sizeof(int);
			break;

		case CFG_UINT:
			ptr_len  = sizeof(unsigned int *);
			item_len = sizeof(unsigned int);
			break;

		case CFG_LONG:
			ptr_len  = sizeof(long *);
			item_len = sizeof(long);
			break;

		case CFG_ULONG:
			ptr_len  = sizeof(unsigned long *);
			item_len = sizeof(unsigned long);
			break;

		case CFG_FLOAT:
			ptr_len  = sizeof(float *);
			item_len = sizeof(float);
			break;

		case CFG_DOUBLE:
			ptr_len  = sizeof(double *);
			item_len = sizeof(double);
			break;
	}

	for (size_plus = 0; multi_arg[size_plus] != NULL; size_plus++) ;
	for (size = 0; *ar != NULL && (*ar)[size] != NULL; size++) ;

	*ar = realloc(*ar, (size + 1 + size_plus) * ptr_len);

	if (*ar == NULL)
		return CFG_ERROR_NOMEM;

	/* Array terminated NULL pointer (end of array). */
	(*ar)[size + size_plus] = NULL;

	for (k = 0; k < size_plus; k++) {
		(*ar)[size + k] = malloc(item_len);
		if ((*ar)[size + k] == NULL)
			return CFG_ERROR_NOMEM;

		ret = store_single_arg(type, multi_arg[k], (*ar)[size + k]);

		if (ret != CFG_OK) {
			/* To prevent having random value at size + k position in *ar. */
			(*ar)[size + k] = NULL;
			return ret;
		}
	}

	return CFG_OK;
} /* }}} */

/*
 * store_single_arg()
 *
 * According to option date type (type) parses and stores single argument (arg)
 * to specified place pointed by where.
 */

	static int
store_single_arg(type, arg, where)
	const int  type;
	const char *arg;
	const void *where;
{ /* {{{ */
	register long          long_val   = 0;
	register unsigned long ulong_val  = 0;
	register double        double_val = 0.0;
	register int           f_integer  = 0; /* Searching for integer value? */
	char *end;

	if (where == NULL)
		return CFG_OK;

	/* Conveting to numeric value. */
	switch (type) {
		default:
			return CFG_ERROR_INTERNAL;
			break;

		case CFG_INT:
		case CFG_UINT:
		case CFG_LONG:
		case CFG_ULONG:
			f_integer = 1;
			if (type == CFG_ULONG) {
				ulong_val = strtoul(arg, &end, 0);
			} else {
				long_val = strtol(arg, &end, 0);
			}
			if (! (end == NULL || *end != '\0'))
				/* Decimal number conversion succeed */
				break;

			/* If conversion for integer number failed, we are going to try
			   further integer number initialization in float-like style. */

		case CFG_FLOAT:
		case CFG_DOUBLE:
			double_val = strtod(arg, &end);
			if (*end)
				return CFG_ERROR_BADNUMBER;

			if (double_val == +HUGE_VAL || double_val == -HUGE_VAL)
				/* Purpously always return overflow error */
				return f_integer ? CFG_ERROR_OVERFLOW : CFG_ERROR_OVERFLOW;
#if HAVE_ERRNO_H
			if (double_val == 0.0 && errno == ERANGE)
#else
				if (double_val == 0.0 && end == arg)
#endif
					/* Again always return overflow error */
					return f_integer ? CFG_ERROR_OVERFLOW : CFG_ERROR_OVERFLOW;

			/* Validating coversion results */
			if (end == NULL || *end != '\0')
				return CFG_ERROR_BADNUMBER;

			if (! f_integer)
				break;

			if (type == CFG_ULONG) {
				register double diff;
				ulong_val = (unsigned long) double_val;
				diff      = double_val - (double) long_val;
				if (diff >= 1.0 || diff <= -1.0)
					return CFG_ERROR_OVERFLOW;
				if (diff != 0.0)
					return CFG_ERROR_BADNUMBER;
			} else {
				register double diff;
				long_val = (long) double_val;
				diff     = double_val - (double) long_val;
				if (diff >= 1.0 || diff <= -1.0)
					return CFG_ERROR_OVERFLOW;
				if (diff != 0.0)
					return CFG_ERROR_BADNUMBER;
			}
			break;
	}

	/* Range checking and value storing. */
	switch (type) {
		default:
			return CFG_ERROR_INTERNAL;
			break;

		case CFG_INT:
			if (long_val >= INT_MAX || long_val <= INT_MIN)
				return CFG_ERROR_OVERFLOW;

			*((int *) where) = (int) long_val;
			break;

		case CFG_UINT:
			if (long_val > UINT_MAX || long_val < 0)
				return CFG_ERROR_OVERFLOW;

			*((unsigned int *) where) = (unsigned int) long_val;
			break;

		case CFG_LONG:
			if (long_val == LONG_MIN || long_val == LONG_MAX)
				return CFG_ERROR_OVERFLOW;

			*((long *) where) = long_val;
			break;

		case CFG_ULONG:
			/* Fix strange strtoul() behaviour. */
			for (end = (char *) arg; isspace(*end); end++) ;

			/* Testing errno after strtoul() is not needed here. */
			if (*end == '-' || ulong_val == ULONG_MAX /* || ulong_val == 0 */)
				return CFG_ERROR_OVERFLOW;

			*((unsigned long *) where) = (unsigned long) ulong_val;
			break;

		case CFG_FLOAT:
#ifdef ABS /* Borrowed from popt library. */
#	undef ABS
#endif
#define ABS(a) (((a) < 0) ? -(a) : (a))

			if (double_val != 0.0)
				if (ABS(double_val) > FLT_MAX || ABS(double_val) < FLT_MIN)
					return CFG_ERROR_OVERFLOW;

			*((float *) where) = (float) double_val;
			break;

		case CFG_DOUBLE:
			*((double *) where) = (double) double_val;
			break;
	}

	return CFG_OK;
} /* }}} */

/*
 * split_multi_arg()
 *
 * Splits multi argument with separators with focusing on quotations.
 * Returns cfg_error type and in ar is result array stored.
 */

	static int
split_multi_arg(arg, ar, quote_prefix_ar, quote_postfix_ar, separator_ar)
	char	*arg;
	char	***ar;
	char	**quote_prefix_ar;
	char	**quote_postfix_ar;
	char	**separator_ar;
{ /* {{{ */
	register int i;
	int sep_ar_idx, quote_idx, sep_size, tmp_sep_size;
	char *p_quote, *p_sep, *tmp_s;
	char *arg_base = arg;

	if ((*ar = PLATON_FUNC(strdyn_create)()) == NULL)
		return CFG_ERROR_NOMEM;

	do {

		/* Searching first quotation string (p_quote)
		   and set quotation variables */
		p_quote    = PLATON_FUNC(strdyn_str2)(arg, quote_prefix_ar, &quote_idx);
		p_sep      = NULL; /* pointer to separator */
		sep_ar_idx = -1;   /* index of separator */
		sep_size   =  0;   /* length of separator string */

		/* Searching first separator string (p_sep) */
		for (i = 0; separator_ar[i] != NULL; i++) {
			if ((tmp_s = PLATON_FUNC(str_white_str)(arg, separator_ar[i], &tmp_sep_size))
					!= NULL && (p_sep == NULL || tmp_s < p_sep)) {
				p_sep      = tmp_s;
				sep_ar_idx = i;
				sep_size   = tmp_sep_size;
			}
		}

		/* Process quotation
		   if is is on lower position than separator */
		if ((p_quote != NULL && p_sep == NULL)
				|| (p_quote != NULL && p_sep != NULL && p_quote < p_sep)) {

			register char *end_ptr, *prefix, *postfix;
			register int prefix_len, postfix_len;

			if (quote_idx < 0 /* not optimized */
					|| quote_idx > PLATON_FUNC(strdyn_get_size)(quote_prefix_ar) - 1
					|| quote_idx > PLATON_FUNC(strdyn_get_size)(quote_postfix_ar) - 1
					|| (prefix  = quote_prefix_ar[quote_idx]) == NULL
					|| (postfix = quote_postfix_ar[quote_idx]) == NULL)
				return CFG_ERROR_INTERNAL;

			prefix_len  = strlen(prefix);
			postfix_len = strlen(postfix);

			memmove(p_quote, p_quote + prefix_len,
					strlen(p_quote + prefix_len) + 1);

			end_ptr = strstr(p_quote, postfix);

			if (end_ptr == NULL)
				return CFG_ERROR_BADQUOTE;

			memmove(end_ptr, end_ptr + postfix_len,
					strlen(end_ptr + postfix_len) + 1);

			arg = end_ptr;
		}
		/* Separator processing otherwise */
		else if ((p_sep != NULL && p_quote == NULL)
				|| (p_sep != NULL && p_quote != NULL && p_sep <= p_quote)) {

			register char c;

			c      = *p_sep;
			*p_sep = '\0';
			*ar    = PLATON_FUNC(strdyn_add_va)(*ar, arg_base, NULL);
			*p_sep = c;
			arg    = arg_base = p_sep + sep_size;

			if (*ar == NULL)
				return CFG_ERROR_NOMEM;
		}

	} while (p_quote != NULL || p_sep != NULL);

	if ((*ar = PLATON_FUNC(strdyn_add_va)(*ar, arg_base, NULL)) == NULL)
		return CFG_ERROR_NOMEM;

	return CFG_OK;
} /* }}} */

/*
 * unquote_single_arg()
 *
 * Unquotes signle argument passed by reference as arg parameter.
 * Returns cfg_error type and modyfies arg parameter.
 */

	static int
unquote_single_arg(arg, quote_prefix_ar, quote_postfix_ar)
	char	*arg;
	char	**quote_prefix_ar;
	char	**quote_postfix_ar;
{ /* {{{ */
	register char *p_quote;
	int quote_idx;

	do {
		p_quote = PLATON_FUNC(strdyn_str2)(arg, quote_prefix_ar, &quote_idx);

		/* If beginning of quotation was found */
		if (p_quote != NULL) {
			register char *end_ptr, *prefix, *postfix;
			register int prefix_len, postfix_len;

			if (quote_idx < 0 /* not optimized */
					|| quote_idx > PLATON_FUNC(strdyn_get_size)(quote_prefix_ar) - 1
					|| quote_idx > PLATON_FUNC(strdyn_get_size)(quote_postfix_ar) - 1
					|| (prefix  = quote_prefix_ar[quote_idx]) == NULL
					|| (postfix = quote_postfix_ar[quote_idx]) == NULL)
				return CFG_ERROR_INTERNAL;

			prefix_len  = strlen(prefix);
			postfix_len = strlen(postfix);

			memmove(p_quote, p_quote + prefix_len,
					strlen(p_quote + prefix_len) + 1);

			end_ptr = strstr(p_quote, postfix);

			if (end_ptr == NULL)
				return CFG_ERROR_BADQUOTE;

			memmove(end_ptr, end_ptr + postfix_len,
					strlen(end_ptr + postfix_len) + 1);

			arg = end_ptr;
		}
	} while (p_quote != NULL);

	return CFG_OK;
} /* }}} */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

