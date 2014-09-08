/*
 * libcfg+ - precise command line & config file parsing library
 *
 * cmdline.c - command line parsing
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

/* $Platon: libcfg+/src/cmdline.c,v 1.36 2004/01/12 06:03:09 nepto Exp $ */

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
#  include <strings.h>
#endif

#include "platon/str/strplus.h"
#include "platon/str/strdyn.h"

#include "cfg+.h"
#include "shared.h"
/* }}} */

	int
cfg_cmdline_get_next_opt(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	int arg_used;
	int ret_val;

	con->error_code = CFG_OK;

	/* Initial position seek */
	if (! con->parsing_started) {
		con->parsing_started = 1;

		if (con->begin_pos < 0) {
			con->error_code = CFG_ERROR_SEEK_ERROR;
			return con->error_code;
		}

		if (con->begin_pos > 0) {
			for (; con->cur_idx < con->begin_pos; con->cur_idx++)
				if (con->argv[con->cur_idx] == NULL) {
					con->error_code = CFG_ERROR_SEEK_ERROR;
					return con->error_code;
				}
		}

		if (con->flags & CFG_SKIP_FIRST)
			con->cur_idx_tmp = 1;
		else
			con->cur_idx_tmp = 0;
	}

	/*
	 * Main loop
	 */
	while (1) {

		arg_used = 0;

		/* Updating cur_idx step by step and testing for NULL in argv. */
		for (; con->cur_idx_tmp > 0; con->cur_idx_tmp--, con->cur_idx++)
			if (con->argv[con->cur_idx] == NULL)
				break;

		/* Finished? (size is reached) */
		if (con->size >= 0 && con->cur_idx >= con->begin_pos + con->size)
			break;

		/* Finished? (NULL is detected) */
		if (con->argv[con->cur_idx] == NULL)
			break;

		if (con->cur_opt_type & CFG_SHORT_OPTIONS) {

			con->cur_opt[0] = con->cur_arg[0];
			PLATON_FUNC(strdel)(con->cur_arg);

			if (strlen(con->cur_arg) == 0) {
				con->cur_opt_type -= CFG_SHORT_OPTIONS;
				free(con->cur_arg);

				/* strdup() doesn't accept NULL as parameter */
				con->cur_arg = con->argv[con->cur_idx + 1] != NULL
					? strdup(con->argv[con->cur_idx + 1])
					: NULL;
			}
		}
		else {
			register int leftover_init = 0;

			/* Test if previous argument was leftover and also there is not
			   advanced leftovers initializations set in context flags. */
			if (! (con->flags & CFG_ADVANCED_LEFTOVERS)
					&& con->cur_opt_type == CFG_NONE_OPTION
					&& con->cur_opt == NULL && con->cur_arg != NULL
					&& con->argv[con->cur_idx - 1] != NULL
					&& ! strcmp(con->cur_arg, con->argv[con->cur_idx - 1]))
				leftover_init = 1;

			__cfg_free_currents(con);

			if (! PLATON_FUNC(strdyn_compare)(con->prop[CFG_LINE_STOP_STRING],
						con->argv[con->cur_idx])) {
				con->error_code = CFG_ERROR_STOP_STR_FOUND;
				return con->error_code;
			}

			/* Skip option analyze in __cfg_cmdline_set_currents(),
			   count it as leftover. */
			if (leftover_init) {
				con->cur_opt_type = CFG_NONE_OPTION;
				con->cur_opt = NULL;
				if ((con->cur_arg = strdup(con->argv[con->cur_idx])) == NULL) {
					con->error_code = CFG_ERROR_NOMEM;
					return con->error_code;
				}
			}
			else {
				if (__cfg_cmdline_set_currents(con) != CFG_OK) {
					con->error_code = CFG_ERROR_NOMEM;
					return con->error_code;
				}
			}
		}

		con->error_code = __cfg_process_currents(con, &ret_val, &arg_used);
		if (con->error_code != CFG_OK)
			return con->error_code;

		if (arg_used) {
			if (! (con->cur_opt_type & CFG_LONG_SEPINIT)
					&& !(con->cur_opt_type & CFG_SHORT_OPTIONS)
					&& !(con->cur_opt_type == CFG_NONE_OPTION))
				con->cur_idx_tmp++;

			if (con->cur_opt_type & CFG_SHORT_OPTIONS)
				con->cur_opt_type -= CFG_SHORT_OPTIONS;
		}

		if (! (con->cur_opt_type & CFG_SHORT_OPTIONS))
			con->cur_idx_tmp++;

		if (ret_val > 0)
			return ret_val;
	}

	return con->error_code; /* CFG_OK */
} /* }}} */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

