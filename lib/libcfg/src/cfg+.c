/*
 * libcfg+ - precise command line & config file parsing library
 *
 * cfg+.c - main implementation
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

/* $Platon: libcfg+/src/cfg+.c,v 1.61 2004/01/12 06:03:08 nepto Exp $ */

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

#include "platon/str/strdyn.h"
#include "platon/str/strplus.h"

#include "cfg+.h"
#include "shared.h"
/* }}} */

extern char *cfg_default_properties[CFG_N_PROPS][4];

	CFG_CONTEXT
cfg_get_context(options)
	struct cfg_option *options;
{ /* {{{ */
	register int i;
	CFG_CONTEXT con;

	con = (CFG_CONTEXT) malloc(sizeof(*con));
	if (con == NULL)
		return NULL;

	/* Setting all struct values to 0 or NULL */
	memset(con, '\0', sizeof(*con));

	/* Initializing context type and options set */
	con->type    = CFG_NO_CONTEXT;
	con->options = options;

	/* Initializaing properties to default values */
	for (i = 0; i < CFG_N_PROPS; i++) {
		con->prop[i] = PLATON_FUNC(strdyn_create_ar)(cfg_default_properties[i]);
		if (con->prop[i] == NULL) {
			/* TODO: possible freeing on failure */
			return NULL;
		}
	}

	return con;
} /* }}} */

	CFG_CONTEXT
cfg_get_cmdline_context(begin_pos, size, argv, options)
	long              begin_pos;
	long              size;
	char              **argv;
	struct cfg_option *options;
{ /* {{{ */
	CFG_CONTEXT con;

	con = cfg_get_context(options);
	if (con == NULL)
		return NULL;

	cfg_set_cmdline_context(con, begin_pos, size, argv);

	return con;
} /* }}} */

	CFG_CONTEXT
cfg_get_cmdline_context_argc(argc, argv, options)
	int               argc;
	char              **argv;
	struct cfg_option *options;
{ /* {{{ */
	CFG_CONTEXT con;

	/* Starting from the beginning and parsing till the end */
	con = cfg_get_cmdline_context((long) 0, (long) argc, argv, options);
	if (con == NULL)
		return con;

	/* When parsing by argc/argv we must skip first argument argv[0],
	   because there is the name of program. */
	cfg_set_context_flag(con, CFG_SKIP_FIRST);

	return con;
} /* }}} */

	CFG_CONTEXT
cfg_get_cfgfile_context(begin_pos, size, filename, options)
	long              begin_pos;
	long              size;
	char              *filename;
	struct cfg_option *options;
{ /* {{{ */
	CFG_CONTEXT con;

	con = cfg_get_context(options);
	if (con == NULL)
		return NULL;

	cfg_set_cfgfile_context(con, begin_pos, size, filename);

	return con;
} /* }}} */

	void
cfg_set_cmdline_context(con, begin_pos, size, argv)
	const CFG_CONTEXT con;
	long              begin_pos;
	long              size;
	char              **argv;
{ /* {{{ */
	cfg_reset_context(con);

	con->type      = CFG_CMDLINE;
	con->begin_pos = begin_pos;
	con->size      = size;
	con->argv      = argv;

} /* }}} */

	void
cfg_set_cmdline_context_argc(con, argc, argv)
	const CFG_CONTEXT con;
	int               argc;
	char              **argv;
{ /* {{{ */

	/* This function could be macro. But appropriate `get' equivalent, function
	   cfg_get_cmdline_context_argc(), is in fact function. So this is function
	   analogically too. */

	cfg_set_cmdline_context(con, (long) 0, (long) argc, argv);

} /* }}} */

	void
cfg_set_cfgfile_context(con, begin_pos, size, filename)
	const CFG_CONTEXT con;
	long              begin_pos;
	long              size;
	char              *filename;
{ /* {{{ */
	cfg_reset_context(con);

	con->type      = CFG_CFGFILE;
	con->begin_pos = begin_pos;
	con->size      = size;

	/* TODO: filename copying? (if yes than don't forget memory freeing) */
	if (filename != NULL)
		con->filename = (char *) filename;

	con->fhandle = NULL;
} /* }}} */

	void
cfg_reset_context(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	con->error_code      = CFG_OK;
	con->cur_idx         = 0;
	con->cur_idx_tmp     = 0;
	con->parsing_started = 0;

	if (con->used_opt_idx != NULL) {
		free(con->used_opt_idx);
		con->used_opt_idx = NULL;
	}

	__cfg_free_currents(con);

	/* Problematic code? */
	if (con->fhandle != NULL) {
		fclose(con->fhandle);
		con->fhandle = NULL;
	}
} /* }}} */

	void
cfg_free_context(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	register int i;

	cfg_reset_context(con);

	for (i = 0; i < CFG_N_PROPS; i++) {
#if 0
		(void) cfg_clear_property(con, i);
		free(con->prop[i]);
#else
		PLATON_FUNC(strdyn_free)(con->prop[i]);
#endif
		con->prop[i] = NULL;
	}

	con->begin_pos = 0;
	con->size      = 0;
	con->argv      = NULL;
	con->filename  = NULL;
	con->type      = CFG_NO_CONTEXT;

	free((void *) con);
} /* }}} */

	void
cfg_print_error(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	cfg_fprint_error(con, stderr);
} /* }}} */

	void
cfg_fprint_error(con, fh)
	const CFG_CONTEXT con;
	FILE *fh;
{ /* {{{ */
	char *s;

	s = cfg_get_error_str(con);

	if (s == NULL) {
		fputs("not enough memory for error printing\n", fh);
	}
	else {
		fputs(s, fh);
		free(s);
	}
} /* }}} */

	char *
cfg_get_error_str(con)
	const CFG_CONTEXT con;
{ /* {{{ */
	char *s;
	char *str_type  = con->type == CFG_LINE ? "on command line" : "in config file";
	char *str_pos   = con->type == CFG_LINE ? /* "near" */ "at position"
		: (con->flags & CFG_FILE_LINE_POS_USAGE ? "on line" : "at position");
	char *str_opt   = cfg_get_cur_opt(con);
	char *str_arg   = cfg_get_cur_arg(con);
	char *str_filename = con->filename;
	int idx = cfg_get_cur_idx(con) + 1;
	int size;

	str_opt = str_opt == NULL ? "" : str_opt;
	str_arg = str_arg == NULL ? "" : str_arg;
	str_filename = str_filename == NULL ? "" : str_filename;

	/* WARNING: pay attention on possible buffer owerflow here; used constant
	   must be enough to cover str_type, str_pos, str_idx and the rest
	   of error message with deliminators. */

#ifndef max /* Borrowed from libplaton */
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

	size = 300 + max(strlen(str_opt) + strlen(str_arg), strlen(str_filename));

	if ((s = (char *) malloc(sizeof(char) * size)) == NULL)
		return NULL;

	switch (con->error_code) {

		case CFG_OK:
			sprintf(s, "no error on %s", str_type);
			break;

		case CFG_ERROR_NOARG:
			sprintf(s, "argument is missing for option '%s' %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_NOTALLOWEDARG:
			sprintf(s, "option '%s' does not have allowed argument %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_BADOPT:
			sprintf(s, "argument '%s' for option '%s' could not be parsed"
					" %s %d %s",
					str_arg, str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_BADQUOTE:
			sprintf(s, "error in quotations in option '%s' %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_BADNUMBER:
			sprintf(s, "argument '%s' for option '%s' could not be converted"
					" to appropriate numeric type %s %d %s",
					str_arg, str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_OVERFLOW:
			sprintf(s, "given number '%s' was too big or too small in option"
					" '%s' %s %d %s",
					str_arg, str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_MULTI:
			sprintf(s, "multiple arguments used for single option '%s' %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_NOMEM:
			sprintf(s, "not enough memory");
			break;

		case CFG_ERROR_STOP_STR_FOUND:
			sprintf(s, "stop string '%s' was found %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_NOEQUAL:
			sprintf(s, "no equal sign founded %s %d %s",
					str_pos, idx, str_type);
			break;

		case CFG_ERROR_UNKNOWN:
			sprintf(s, "unknown option '%s' %s %d %s",
					str_opt, str_pos, idx, str_type);
			break;

		case CFG_ERROR_FILE_NOT_FOUND:
			sprintf(s, "config file '%s' was not found", str_filename);
			break;

		case CFG_ERROR_SEEK_ERROR:
			sprintf(s, "seek error in %s", str_type);
			break;

		case CFG_ERROR_INTERNAL:
			sprintf(s, "libcfg internal error");
		default:
			sprintf(s, "unknown error (%d)", con->error_code);
			break;
	}

	return s;
} /* }}} */

	char *
cfg_get_static_error_str(errorcode)
	const int errorcode;
{ /* {{{ */
	switch (errorcode) {

		case CFG_OK:
			return "no error";

		case CFG_ERROR_NOARG:
			return "argument is missing for option";

		case CFG_ERROR_NOTALLOWEDARG:
			return "argument is not allowed for option";

		case CFG_ERROR_BADOPT:
			return "option's argument could not be parsed";

		case CFG_ERROR_BADQUOTE:
			return "error in quotations";

		case CFG_ERROR_BADNUMBER:
			return "option could not be converted to appropriate numeric type";

		case CFG_ERROR_OVERFLOW:
			return "given number was too big or too small";

		case CFG_ERROR_MULTI:
			return "multiple arguments used for single option";

		case CFG_ERROR_NOMEM:
			return "not enough memory";

		case CFG_ERROR_STOP_STR_FOUND:
			return "stop string was found";

		case CFG_ERROR_NOEQUAL:
			return "no equal sign on the line";

		case CFG_ERROR_UNKNOWN:
			return "unknown option";

		case CFG_ERROR_FILE_NOT_FOUND:
			return "file not found";

		case CFG_ERROR_SEEK_ERROR:
			return "file seek error";

		case CFG_ERROR_INTERNAL:
			return "internal error";
	}

	return "unknown error";
} /* }}} */

#if defined(SELF) || defined(SELFTEST) || defined(SELF_CFG)

/*
 * Selftest compilation:
 * gcc -Wall -I. -I.. -L. -DHAVE_CONFIG_H -DSELF -o test cfg+.c -lcfg+
 */

	void
ar_print(name, format, vals)
	char *name;
	char *format;
	void **vals;
{ /* {{{ */
	if (vals != NULL) {
		if (strcmp(format, "%s")) {
			register int i;

			printf("%s: ", name);
			for (i = 0; vals[i] != NULL; i++) {

				if (! strcmp(format, "%e") || ! strcmp(format, "%f"))
					printf(format, *(((float **) vals)[i]));
				else
					printf(format, *(((int **) vals)[i]));

				printf(", ");
			}

			puts("NULL");
		}
		else {
			register char *s;
			s = PLATON_FUNC(strdyn_implode_str)((char **) vals, "][");
			printf("%s = [%s]\n", name, s);
			if (s != NULL)
				free(s);
		}

		PLATON_FUNC(strdyn_free)((char **) vals);
	}
	else
		printf("%s uninitialized\n", name);
} /* }}} */

	int
main(argc, argv)
	int  argc;
	char **argv;
{ /* {{{ */

	CFG_CONTEXT con;
	int ret, i;

	int o_nice = 20, o_verbose = 0;
	char *o_cfgfile = NULL, *o_string = NULL;
	char **o_name = NULL, **o_leftover = NULL;
	int o_boolean = 0;
	int o_int = -33, **o_multi_int = NULL;
	unsigned int o_uint = 33, **o_multi_uint = NULL;
	long o_long = -333, **o_multi_long = NULL;
	unsigned long o_ulong = 333, **o_multi_ulong = NULL;
	float o_float = -3333.33, **o_multi_float = NULL;
	double o_double = -33333.33, **o_multi_double = NULL;


	struct cfg_option options[] = {
		{ "nice",			'n', "nice",
			CFG_INT,							(void *) &o_nice,			0 },
		{ "verbose",		'v', "verbose",
			CFG_BOOLEAN	+ CFG_MULTI,			(void *) &o_verbose,		0 },
		{ "name",			'a', "name",
			CFG_STRING	+ CFG_MULTI_SEPARATED,	(void *) &o_name,			0 },
		{ "config-file",	'f', NULL,
			CFG_STRING,							(void *) &o_cfgfile,		0 },

		{ "bool",			'b', "boolean",
			CFG_BOOLEAN,						(void *) &o_boolean,		1 },
		{ "int",			'i', "int",
			CFG_INT,							(void *) &o_int,			0 },
		{ "multi_int",		'I', "multi int",
			CFG_INT + CFG_MULTI_SEPARATED,		(void *) &o_multi_int,		0 },
		{ "uint",			'u', "unsigned int",
			CFG_UINT,							(void *) &o_uint,			0 },
		{ "multi_uint",		'U', "multi unsigned int",
			CFG_UINT + CFG_MULTI,				(void *) &o_multi_uint,		0 },
		{ "long",			'l', "long",
			CFG_LONG,							(void *) &o_long,			0 },
		{ "multi_long",		'L', "multi long",
			CFG_LONG + CFG_MULTI_SEPARATED,		(void *) &o_multi_long,		0 },
		{ "ulong",			'o', "unsigned long",
			CFG_ULONG,							(void *) &o_ulong,			0 },
		{ "multi_ulong",	'O', "multi unsigned long",
			CFG_ULONG + CFG_MULTI,				(void *) &o_multi_ulong,	0 },
		{ "float",			'f', "float",
			CFG_FLOAT,							(void *) &o_float,			0 },
		{ "multi_float",	'F', "multi float",
			CFG_FLOAT + CFG_MULTI_SEPARATED,	(void *) &o_multi_float,	0 },
		{ "double",			'd', "double",
			CFG_DOUBLE,							(void *) &o_double,			0 },
		{ "multi_double",	'D', "mdouble",
			CFG_DOUBLE + CFG_MULTI_SEPARATED,	(void *) &o_multi_double,	0},
		{ "string",			's', "string",
			CFG_STRING,							(void *) &o_string,			0 },
		{ NULL,				'\0',"argv",
			CFG_STRING + CFG_MULTI_SEPARATED + CFG_LEFTOVER_ARGS,	(void*) &o_leftover, 0 },
		CFG_END_OF_LIST
	};

	con = cfg_get_context(options);

	for (i = 0; ; i++) {

		/* This is powerful testing stuff :)

		   printf("advanced leftovers: %d\n", cfg_get_context_flag(con, CFG_ADVANCED_LEFTOVERS));
		   cfg_set_context_flag(con, CFG_ADVANCED_LEFTOVERS);
		   cfg_set_context_flag(con, CFG_ADVANCED_LEFTOVERS);
		   printf("advanced leftovers: %d\n", cfg_get_context_flag(con, CFG_ADVANCED_LEFTOVERS));
		   cfg_clear_context_flag(con, CFG_ADVANCED_LEFTOVERS);
		   printf("advanced leftovers: %d\n", cfg_get_context_flag(con, CFG_ADVANCED_LEFTOVERS));
		   cfg_set_context_flag(con, CFG_ADVANCED_LEFTOVERS);
		   printf("advanced leftovers: %d\n", cfg_get_context_flag(con, CFG_ADVANCED_LEFTOVERS));
		   */

		if (i == 0) {
			cfg_set_cmdline_context(con, 1, argc, argv);
			cfg_set_context_flag(con, CFG_ADVANCED_LEFTOVERS);
			cfg_set_context_flag(con, CFG_FILE_LINE_POS_USAGE);
			if (0) { /* Testing if short command line options
						works also without '-' prefix */
				cfg_clear_property(con, CFG_LINE_SHORT_OPTION_PREFIX);
				cfg_add_property(con, CFG_LINE_SHORT_OPTION_PREFIX, "");
			}
			if (0) { /* Testing if long  command line options
						works also without '--' prefix */
				cfg_clear_property(con, CFG_LINE_LONG_OPTION_PREFIX);
				cfg_add_property(con, CFG_LINE_LONG_OPTION_PREFIX, "");
			}
		}
		else {
			cfg_set_cfgfile_context(con, 1, 0, o_cfgfile);
			/* cfg_remove_cfgfile_comment_prefix(con, "//");
			   cfg_add_cfgfile_comment_prefix(con, "//"); */
			cfg_remove_property(con, CFG_FILE_COMMENT_PREFIX, "//");
			cfg_add_property(con, CFG_FILE_COMMENT_PREFIX, "//");
		}

		while ((ret = cfg_get_next_opt(con)) > 0) {
			printf("ret = %d, opt = '%s', arg = '%s'", ret,
					cfg_get_cur_opt(con), cfg_get_cur_arg(con));

			if (i == 0)
				printf(", idx = %d, argv[idx] = '%s'",
						cfg_get_cur_idx(con),
						argv[cfg_get_cur_idx(con)]);

			putchar('\n');
		}

		if (ret != CFG_OK) {
			puts(
					"+------------------+\n"
					"|  Error printing  |\n"
					"+------------------+");

			printf("***** ");
			cfg_fprint_error(con, stdout);
			putchar('\n');

			printf("***** %s error %d at %d: %s\n",
					i == 0 ? "cmdline" : "cfgfile",
					ret, cfg_get_cur_idx(con),
					cfg_get_static_error_str(ret));

			printf("***** opt = '%s', arg = '%s'",
					cfg_get_cur_opt(con), cfg_get_cur_arg(con));

			if (i == 0)
				printf(", idx = %d, argv[idx] = '%s'",
						cfg_get_cur_idx(con), argv[cfg_get_cur_idx(con)]);

			putchar('\n');
		}

		if (i == 0 && o_cfgfile == NULL)
			break;

		if (i > 0)
			break;
	}

	cfg_free_context(con);

	puts(
			"+--------------------------+\n"
			"|  Common values printing  |\n"
			"+--------------------------+");

	printf("o_verbose = %d, o_nice = %d\n", o_verbose, o_nice);
	printf("o_boolean = %d\n", o_boolean);
	printf("o_int = %d, o_uint = %u\n", o_int, o_uint);
	printf("o_long = %ld, o_ulong = %lu\n", o_long, o_ulong);
	printf("o_float = %f, o_double = %e\n", o_float, o_double);
	printf("o_string = <%s>\n", o_string);

	puts(
			"+-------------------------+\n"
			"|  Multi values printing  |\n"
			"+-------------------------+");

	ar_print("o_multi_int", "%d",  (void **) o_multi_int);
	ar_print("o_multi_uint", "%u", (void **) o_multi_uint);
	ar_print("o_multi_long", "%ld", (void **) o_multi_long);
	ar_print("o_multi_ulong", "%lu", (void **) o_multi_ulong);
	ar_print("o_multi_float", "%f", (void **) o_multi_float);
	ar_print("o_multi_double", "%e", (void **) o_multi_double);
	ar_print("o_name", "%s", (void **) o_name);
	ar_print("o_leftover", "%s", (void **) o_leftover);

	return 0;
} /* }}} */

#endif

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

