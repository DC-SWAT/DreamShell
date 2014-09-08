/*
 * libcfg+ - precise command line & config file parsing library
 *
 * cfgfile.c - config file parsing
 * ____________________________________________________________
 *
 * Developed by Ondrej Jombik <nepto@platon.sk>
 *          and Lubomir Host <rajo@platon.sk>
 * Copyright (c) 2001-2003 Platon SDG, http://platon.sk/
 * All rights reserved.
 *
 * See README file for more information about this software.
 * See COPYING file for license information.
 *
 * Download the latest version from
 * http://platon.sk/projects/libcfg+/
 */

/* $Platon: libcfg+/src/cfgfile.c,v 1.26 2003/11/07 17:26:48 nepto Exp $ */

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
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "platon/str/strdyn.h"
#include "platon/str/strplus.h"
#include "platon/str/dynfgets.h"

#include "cfg+.h"
#include "shared.h"
/* }}} */

/* Static function declarations {{{ */
static int get_multi_line(const CFG_CONTEXT con, char **buf);
/* }}} */

	int
cfg_cfgfile_get_next_opt(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	char *buf;
	int ret_val;

	con->error_code = CFG_OK;

	/* Initial position seek */
	if (con->fhandle == NULL) {
		con->fhandle = con->filename != NULL
			? fopen(con->filename, "r")
			: NULL;
		if (con->fhandle == NULL) {
			con->error_code = CFG_ERROR_FILE_NOT_FOUND;
			return con->error_code;
		}

		if (con->flags & CFG_FILE_LINE_POS_USAGE) {
			/* If negative line is specified, returns seek error.
			   If 0 is specified, do nothing.
			   If number > 0 is specified make appropriate line seek. */
			if (con->begin_pos < 0) {
				con->error_code = CFG_ERROR_SEEK_ERROR;
				return con->error_code;
			}

			if (con->begin_pos > 0) {
				con->cur_idx     = 0;
				con->cur_idx_tmp = 0;

				/* Moving to begin_pos line */
				while (con->cur_idx_tmp < con->begin_pos) {
					switch (fgetc(con->fhandle)) {
						case EOF:
							con->error_code = CFG_ERROR_SEEK_ERROR;
							return con->error_code;
						case '\n':
							con->cur_idx_tmp++;
					}
				}
			}
		}
		else {
			if (/* always do: con->begin_pos > 0 && */
					fseek(con->fhandle, con->begin_pos, SEEK_SET) != 0) {
				con->error_code = CFG_ERROR_SEEK_ERROR;
				return con->error_code;
			}
		}
	}

	/*
	 * Main loop
	 */
	while (1) {
		/* Updating cur_idx to set current line position */
		if (con->flags & CFG_FILE_LINE_POS_USAGE) {
			con->cur_idx     += con->cur_idx_tmp;
			con->cur_idx_tmp  = 0;
		}

		/* Reading multi line and exit on error */
		con->error_code = get_multi_line(con, &buf);
		if (con->error_code != CFG_OK) {
			if (buf != NULL)
				free(buf);

			return con->error_code;
		}

		/* Testing if file stop prefix was found */
		if (buf != NULL && con->prop[CFG_FILE_STOP_PREFIX] != NULL) {
			if (buf == PLATON_FUNC(strdyn_str2)(buf,
						con->prop[CFG_FILE_STOP_PREFIX], NULL)) {
				__cfg_free_currents(con);
				con->cur_opt    = buf;
				con->error_code = CFG_ERROR_STOP_STR;
				return con->error_code;
			}
		}

		/* Finished? */
		if ((con->size >= 0 && cfg_get_cur_idx(con) >= con->begin_pos + con->size)
				|| feof(con->fhandle)) {
			if (buf != NULL)
				free(buf);

			return con->error_code; /* always CFG_OK */
		}

		__cfg_free_currents(con);

		if (__cfg_cfgfile_set_currents(con, buf) != CFG_OK) {
			con->error_code = CFG_ERROR_NOMEM;
			return con->error_code;
		}

		free(buf);

		con->error_code = __cfg_process_currents(con, &ret_val, NULL);
		if (con->error_code != CFG_OK)
			return con->error_code;

		if (ret_val > 0)
			return ret_val;
	}

	return con->error_code; /* CFG_OK */
} /* }}} */

/*
 * get_multi_line()
 *
 * Gets single line from file. If line continues on next line, returns
 * the whole line concatenated. Coments, remarks and empty lines are
 * skipped. If end of file is reached CFG_OK is returned and higher
 * level should determine that fact using feof() function.
 */

	static int
get_multi_line(con, buf)
	const CFG_CONTEXT con;
	char              **buf;
{ /* {{{ */
	register char **ar    = NULL;
	register char *my_buf = NULL;
	register int   state  = 0;

	*buf = NULL;

	if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
		return CFG_ERROR_NOMEM;

	while (1) {
		if (state > 1)
			state = 1;

		if (my_buf != NULL)
			free(my_buf);

		my_buf = PLATON_FUNC(dynamic_fgets)(con->fhandle);
		if (my_buf == NULL) {
			if (feof(con->fhandle))
				return CFG_OK;

			return CFG_ERROR_NOMEM;
		}

		str_trim(my_buf);

		/* Is empty line or comment? */
		if (strlen(my_buf) == 0 || strdyn_str(my_buf,
					con->prop[CFG_FILE_COMMENT_PREFIX]) == my_buf) {

			if (con->flags & CFG_FILE_LINE_POS_USAGE)
				con->cur_idx++;

			if (state == 0)
				continue;
			else
				break;
		}
		else {
			if (con->flags & CFG_FILE_LINE_POS_USAGE)
				con->cur_idx_tmp++;
		}

		/* Multi line detection. */
		{
			register char **pos;
			register int max_len = 0;
			register int len;

			for (pos = con->prop[CFG_FILE_MULTI_LINE_POSTFIX];
					pos != NULL && *pos != NULL;
					pos++) {

				len = strlen(my_buf) - strlen(*pos);

				if (len > max_len && ! strcmp(*pos, my_buf + len))
					max_len = len;
			}

			/* Multi line postfix found? */
			if (max_len > 0) {
				my_buf[max_len] = '\0';
				state = 2;

				len = strlen(my_buf);
				PLATON_FUNC(str_right_trim)(my_buf);
				if (len - strlen(my_buf) > 0) {
					/* Could be replaced with
					   strcpy(my_buf + strlen(my_buf), " "); */
					my_buf[strlen(my_buf) + 1] = '\0';
					my_buf[strlen(my_buf)] = ' ';
				}
			}
		}

		ar = PLATON_FUNC(strdyn_add)(ar, my_buf);
		if (ar == NULL)
			return CFG_ERROR_NOMEM;

		if (state != 2)
			break;
	}

	if (my_buf != NULL)
		free(my_buf);

	my_buf = PLATON_FUNC(str_right_trim)(strdyn_implode(ar, ""));
	PLATON_FUNC(strdyn_free)(ar);

	if (my_buf == NULL)
		return CFG_ERROR_NOMEM;

	*buf = my_buf;

	return CFG_OK;
} /* }}} */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

