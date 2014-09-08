/*
 * libcfg+ - precise command line & config file parsing library
 *
 * props.c - context properties manipulation
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

/* $Platon: libcfg+/src/props.c,v 1.34 2004/01/12 06:03:09 nepto Exp $ */

/* Includes {{{ */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#include "platon/str/strdyn.h"
#include "cfg+.h"
/* }}} */

/* Default properties {{{ */
char *cfg_default_properties[CFG_N_PROPS][4] = {
	/* CFG_LINE_STOP_STRING */						{ NULL, NULL, NULL, NULL },
	/* CFG_LINE_SHORT_OPTION_PREFIX */				{ "-",  NULL, NULL, NULL },
	/* CFG_LINE_LONG_OPTION_PREFIX */				{ "--", NULL, NULL, NULL },
	/* CFG_LINE_OPTION_ARG_SEPARATOR */				{ "=",  NULL, NULL, NULL },
	/* CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR */		{ ",",  ";",  NULL, NULL },
	/* CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR */	{ NULL, NULL, NULL, NULL },
	/* CFG_LINE_QUOTE_PREFIX */						{ NULL, NULL, NULL, NULL },
	/* CFG_LINE_QUOTE_POSTFIX */					{ NULL, NULL, NULL, NULL },
	/* CFG_FILE_STOP_PREFIX */						{ NULL, NULL, NULL, NULL },
	/* CFG_FILE_COMMENT_PREFIX */					{ "#",  ";",  NULL, NULL },
	/* CFG_FILE_MULTI_LINE_POSTFIX */				{ "\\", NULL, NULL, NULL },
	/* CFG_FILE_OPTION_ARG_SEPARATOR */				{ "=",  NULL, NULL, NULL },
	/* CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR */		{ ",",  ";",  " ",  NULL },
	/* CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR */	{ " ",  NULL, NULL, NULL },
	/* CFG_FILE_QUOTE_PREFIX */						{ "\"", "'",  NULL, NULL },
	/* CFG_FILE_QUOTE_POSTFIX */					{ "\"", "'",  NULL, NULL },
	}; /* }}} */

/*
 * Static functions for strdyn manipulation
 */

	static int
strdyn_array_clear(where)
	char ***where;
{ /* {{{ */

#if 0
	if (*where != NULL)
#endif
		*where = PLATON_FUNC(strdyn_remove_all)(*where);

	return *where != NULL;
} /* }}} */

	static int
strdyn_array_add(where, str)
	char ***where;
	char *str;
{ /* {{{ */

#if 0
	if (*where == NULL)
		*where = strdyn_create();
#endif

	*where = PLATON_FUNC(strdyn_add)(*where, str);

	return *where != NULL;
} /* }}} */

	static int
strdyn_array_remove(where, str)
	char ***where;
	char *str;
{ /* {{{ */

#if 0
	if (*where != NULL)
#endif
		*where = PLATON_FUNC(strdyn_remove_str_all)(*where, str);

	return *where != NULL;
} /* }}} */


/*
 * Functions for context flags manipulation.
 */

	void
cfg_set_context_flag(con, flag)
	const CFG_CONTEXT con;
	int               flag;
{ /* {{{ */
	con->flags |= flag;
} /* }}} */

	void
cfg_clear_context_flag(con, flag)
	const CFG_CONTEXT con;
	int               flag;
{ /* {{{ */
	con->flags &= ~flag;
} /* }}} */

	int
cfg_get_context_flag(con, flag)
	const CFG_CONTEXT con;
	int               flag;
{ /* {{{ */
	return con->flags & flag;
} /* }}} */


	void
cfg_set_context_flags(con, flags)
	const CFG_CONTEXT con;
	int               flags;
{ /* {{{ */
	con->flags       = flags;
	con->cur_idx_tmp = con->flags & CFG_PROCESS_FIRST ? 0 : 1;
} /* }}} */

	int
cfg_get_context_flags(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	return con->flags;
} /* }}} */

/*
 * Macros
 */

#define cfg_normal_property_type(type) ((type) >= 0 && (type) < CFG_N_PROPS)
#define cfg_virtual_property_type(type) ((type) > CFG_N_PROPS)

/*
 * Clear functions
 */

	int
cfg_clear_property(con, type)
	const CFG_CONTEXT con;
	enum cfg_property_type type;
{ /* {{{ */
	if (cfg_normal_property_type(type)) {
		return strdyn_array_clear(&(con->prop[type]));
	}
	else if (cfg_virtual_property_type(type)) {
		register int ret = 1;
		switch (type) {
			default: /* unknown special property; do nothing */
				ret = 0;
				break;
			case CFG_QUOTE:
				ret &= cfg_clear_property(con, CFG_LINE_QUOTE);
				ret &= cfg_clear_property(con, CFG_FILE_QUOTE);
				break;
			case CFG_LINE_QUOTE:
				ret &= cfg_clear_property(con, CFG_LINE_QUOTE_PREFIX);
				ret &= cfg_clear_property(con, CFG_LINE_QUOTE_POSTFIX);
				break;
			case CFG_FILE_QUOTE:
				ret &= cfg_clear_property(con, CFG_FILE_QUOTE_PREFIX);
				ret &= cfg_clear_property(con, CFG_FILE_QUOTE_POSTFIX);
				break;
			case CFG_QUOTE_PREFIX:
				ret &= cfg_clear_property(con, CFG_LINE_QUOTE_PREFIX);
				ret &= cfg_clear_property(con, CFG_FILE_QUOTE_PREFIX);
				break;
			case CFG_QUOTE_POSTFIX:
				ret &= cfg_clear_property(con, CFG_LINE_QUOTE_POSTFIX);
				ret &= cfg_clear_property(con, CFG_FILE_QUOTE_POSTFIX);
				break;
			case CFG_MULTI_VALS_SEPARATOR:
				ret &= cfg_clear_property(con, CFG_LINE_MULTI_VALS_SEPARATOR);
				ret &= cfg_clear_property(con, CFG_FILE_MULTI_VALS_SEPARATOR);
				break;
			case CFG_LINE_MULTI_VALS_SEPARATOR:
				ret &= cfg_clear_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR);
				ret &= cfg_clear_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR);
				break;
			case CFG_FILE_MULTI_VALS_SEPARATOR:
				ret &= cfg_clear_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR);
				ret &= cfg_clear_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR);
				break;
			case CFG_NORMAL_MULTI_VALS_SEPARATOR:
				ret &= cfg_clear_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR);
				ret &= cfg_clear_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR);
				break;
			case CFG_LEFTOVER_MULTI_VALS_SEPARATOR:
				ret &= cfg_clear_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR);
				ret &= cfg_clear_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR);
				break;
			case CFG_OPTION_ARG_SEPARATOR:
				ret &= cfg_clear_property(con, CFG_LINE_OPTION_ARG_SEPARATOR);
				ret &= cfg_clear_property(con, CFG_FILE_OPTION_ARG_SEPARATOR);
				break;
		}

		return ret;
	}

	return 0; /* failure */
} /* }}} */

int
cfg_clear_properties(
		const CFG_CONTEXT con,
		enum cfg_property_type type, ...)
{ /* {{{ */
	va_list ap;
	enum cfg_property_type tmp_type;
	int ret = 1;

	/* initialization */
	tmp_type = type;
	va_start(ap, type);

	/* argument list must be termited with typeN = CFG_EOT */
	while (tmp_type != CFG_EOT) {
		ret &= cfg_clear_property(con, tmp_type);
		if (!ret)
			break;
		tmp_type = va_arg(ap, enum cfg_property_type);
	}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

/*
 * Add functions
 */

	int
cfg_add_property(con, type, str)
	const CFG_CONTEXT con;
	enum cfg_property_type type;
	char *str;
{ /* {{{ */

	/* TODO:
	   remove cfg_add_property_old() calls from here,
	   substitute it by new function cfg_add_properties() */

	if (cfg_normal_property_type(type)) {
		return strdyn_array_add(&(con->prop[type]), str);
	}
	else if (cfg_virtual_property_type(type)) {
		register int ret = 1;
		switch (type) {
			default: /* unknown special property; do nothing */
				ret = 0;
				break;
			case CFG_QUOTE:
				ret &= cfg_add_property(con, CFG_LINE_QUOTE, str);
				ret &= cfg_add_property(con, CFG_FILE_QUOTE, str);
				break;
			case CFG_LINE_QUOTE:
				ret &= cfg_add_property(con, CFG_LINE_QUOTE_PREFIX, str);
				ret &= cfg_add_property(con, CFG_LINE_QUOTE_POSTFIX, str);
				break;
			case CFG_FILE_QUOTE:
				ret &= cfg_add_property(con, CFG_FILE_QUOTE_PREFIX, str);
				ret &= cfg_add_property(con, CFG_FILE_QUOTE_POSTFIX, str);
				break;
			case CFG_QUOTE_PREFIX:
				ret &= cfg_add_property(con, CFG_LINE_QUOTE_PREFIX, str);
				ret &= cfg_add_property(con, CFG_FILE_QUOTE_PREFIX, str);
				break;
			case CFG_QUOTE_POSTFIX:
				ret &= cfg_add_property(con, CFG_LINE_QUOTE_POSTFIX, str);
				ret &= cfg_add_property(con, CFG_FILE_QUOTE_POSTFIX, str);
				break;
			case CFG_MULTI_VALS_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_LINE_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_FILE_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_LINE_MULTI_VALS_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_FILE_MULTI_VALS_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_NORMAL_MULTI_VALS_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_LEFTOVER_MULTI_VALS_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_OPTION_ARG_SEPARATOR:
				ret &= cfg_add_property(con,
						CFG_LINE_OPTION_ARG_SEPARATOR, str);
				ret &= cfg_add_property(con,
						CFG_FILE_OPTION_ARG_SEPARATOR, str);
				break;
		}

		return ret;
	}

	return 0; /* failure */
} /* }}} */

int
cfg_add_properties(
		const CFG_CONTEXT con,
		enum cfg_property_type type,
		char *str, ...)
{ /* {{{ */
	va_list ap;
	enum cfg_property_type tmp_type;
	char * tmp_str;
	int ret = 1;

	/* initialization */
	tmp_type = type;
	tmp_str  = str;
	va_start(ap, str);

	/* argument list must be termited with typeN = CFG_EOT or
	 * str = NULL
	 */
	while (tmp_type != CFG_EOT && tmp_str != NULL) {
		ret &= cfg_add_property(con, tmp_type, tmp_str);
		if (!ret)
			break;
		tmp_type = va_arg(ap, enum cfg_property_type);
		if (tmp_type == CFG_EOT) /* if typeN == CFG_EOT, strN may be not specified! */
			break;
		tmp_str  = va_arg(ap, char *);
	}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

int
cfg_add_properties_str(
		const CFG_CONTEXT con,
		char *str,
		enum cfg_property_type type, ...)
{ /* {{{ */
	va_list ap;
	enum cfg_property_type tmp_type;
	int ret = 1;

	/* initialization */
	tmp_type = type;
	va_start(ap, type);

	/* argument list must be termited with typeN = CFG_EOT or */
	if (str != NULL)
		while (tmp_type != CFG_EOT) {
			ret &= cfg_add_property(con, tmp_type, str);
			if (!ret)
				break;
			tmp_type = va_arg(ap, enum cfg_property_type);
		}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

int
cfg_add_properties_type(
		const CFG_CONTEXT con,
		enum cfg_property_type type,
		char *str, ...)
{ /* {{{ */
	va_list ap;
	char * tmp_str;
	int ret = 1;

	/* initialization */
	tmp_str  = str;
	va_start(ap, str);

	/* argument list must be termited with str = NULL */
	if (type != CFG_EOT)
		while (tmp_str != NULL) {
			ret &= cfg_add_property(con, type, tmp_str);
			if (!ret)
				break;
			tmp_str  = va_arg(ap, char *);
		}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

/*
 * Remove functions
 */

	int
cfg_remove_property(con, type, str)
	const CFG_CONTEXT con;
	enum cfg_property_type type;
	char *str;
{ /* {{{ */

	/* TODO:
	   remove cfg_remove_property_old() calls from here,
	   substitute it by new function cfg_remove_properties() */

	if (cfg_normal_property_type(type)) {
		return strdyn_array_remove(&(con->prop[type]), str);
	}
	else if (cfg_virtual_property_type(type)) {
		register int ret = 1;
		switch (type) {
			default: /* unknown special property; do nothing */
				ret = 0;
				break;
			case CFG_QUOTE:
				ret &= cfg_remove_property(con, CFG_LINE_QUOTE, str);
				ret &= cfg_remove_property(con, CFG_FILE_QUOTE, str);
				break;
			case CFG_LINE_QUOTE:
				ret &= cfg_remove_property(con, CFG_LINE_QUOTE_PREFIX, str);
				ret &= cfg_remove_property(con, CFG_LINE_QUOTE_POSTFIX, str);
				break;
			case CFG_FILE_QUOTE:
				ret &= cfg_remove_property(con, CFG_FILE_QUOTE_PREFIX, str);
				ret &= cfg_remove_property(con, CFG_FILE_QUOTE_POSTFIX, str);
				break;
			case CFG_QUOTE_PREFIX:
				ret &= cfg_remove_property(con, CFG_LINE_QUOTE_PREFIX, str);
				ret &= cfg_remove_property(con, CFG_FILE_QUOTE_PREFIX, str);
				break;
			case CFG_QUOTE_POSTFIX:
				ret &= cfg_remove_property(con, CFG_LINE_QUOTE_POSTFIX, str);
				ret &= cfg_remove_property(con, CFG_FILE_QUOTE_POSTFIX, str);
				break;
			case CFG_MULTI_VALS_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_LINE_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_FILE_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_LINE_MULTI_VALS_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_FILE_MULTI_VALS_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_NORMAL_MULTI_VALS_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_LEFTOVER_MULTI_VALS_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR, str);
				break;
			case CFG_OPTION_ARG_SEPARATOR:
				ret &= cfg_remove_property(con,
						CFG_LINE_OPTION_ARG_SEPARATOR, str);
				ret &= cfg_remove_property(con,
						CFG_FILE_OPTION_ARG_SEPARATOR, str);
				break;
		}

		return ret;
	}

	return 0; /* failure */
} /* }}} */

int
cfg_remove_properties( /* code same like cfg_add_properties() */
		const CFG_CONTEXT con,
		enum cfg_property_type type,
		char *str, ...)
{ /* {{{ */
	va_list ap;
	enum cfg_property_type tmp_type;
	char * tmp_str;
	int ret = 1;

	/* initialization */
	tmp_type = type;
	tmp_str  = str;
	va_start(ap, str);

	/* argument list must be termited with typeN = CFG_EOT or
	 * str = NULL
	 */
	while (tmp_type != CFG_EOT && tmp_str != NULL) {
		ret &= cfg_remove_property(con, tmp_type, tmp_str);
		if (!ret)
			break;
		tmp_type = va_arg(ap, enum cfg_property_type);
		if (tmp_type == CFG_EOT) /* if typeN == CFG_EOT, strN may be not specified! */
			break;
		tmp_str  = va_arg(ap, char *);
	}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

int
cfg_remove_properties_str(
		const CFG_CONTEXT con,
		char *str,
		enum cfg_property_type type, ...)
{ /* {{{ */
	va_list ap;
	enum cfg_property_type tmp_type;
	int ret = 1;

	/* initialization */
	tmp_type = type;
	va_start(ap, type);

	/* argument list must be termited with typeN = CFG_EOT or */
	if (str != NULL)
		while (tmp_type != CFG_EOT) {
			ret &= cfg_remove_property(con, tmp_type, str);
			if (!ret)
				break;
			tmp_type = va_arg(ap, enum cfg_property_type);
		}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */

int
cfg_remove_properties_type(
		const CFG_CONTEXT con,
		enum cfg_property_type type,
		char *str, ...)
{ /* {{{ */
	va_list ap;
	char * tmp_str;
	int ret = 1;

	/* initialization */
	tmp_str  = str;
	va_start(ap, str);

	/* argument list must be termited with str = NULL */
	if (type != CFG_EOT)
		while (tmp_str != NULL) {
			ret &= cfg_remove_property(con, type, tmp_str);
			if (!ret)
				break;
			tmp_str  = va_arg(ap, char *);
		}

	/* cleanup */
	va_end(ap);

	return ret;
} /* }}} */


/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

