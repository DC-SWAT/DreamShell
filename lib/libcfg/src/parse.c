/*
 * libcfg+ - precise command line & config file parsing library
 *
 * parse.c - universal parsing functions
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

/* $Platon: libcfg+/src/parse.c,v 1.24 2004/01/12 06:03:09 nepto Exp $ */

/* Includes {{{ */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
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
#  include <strings.h>
#endif

#include "cfg+.h"
#include "cmdline.h"
#include "cfgfile.h"
/* }}} */

/* Processing context options. */

	int
cfg_parse(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	register int ret;

	while ((ret = cfg_get_next_opt(con)) > 0)
		;

	return ret;
} /* }}} */

	int
cfg_get_next_opt(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	return con->type == CFG_CMDLINE
		? cfg_cmdline_get_next_opt(con)
		: cfg_cfgfile_get_next_opt(con);
} /* }}} */

	char *
cfg_get_cur_opt(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	return con->cur_opt;
} /* }}} */

	char *
cfg_get_cur_arg(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	return con->cur_arg;
} /* }}} */

	int
cfg_get_cur_idx(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	return con->type == CFG_CMDLINE
		? con->cur_idx
		: (con->flags & CFG_FILE_LINE_POS_USAGE
				? con->cur_idx
				: (con->fhandle != NULL ? (int) ftell(con->fhandle) : 0));
} /* }}} */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

