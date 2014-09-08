/*
 * libcfg+ - precise command line & config file parsing library
 *
 * example.c - example program
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

/* $Platon: libcfg+/doc/examples/example.c,v 1.7 2004/01/12 06:03:08 nepto Exp $ */

/* Includes {{{ */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>

#include <cfg+.h>
/* }}} */

/* Function int main(int argc, char **argv) {{{ */
int main(int argc, char **argv)
{
	/* libcfg+ parsing context */
	CFG_CONTEXT con;

	/* Parsing return code */
	register int ret;

	/* Option variables */
	int help, verbose, nice;
	char *cfg_file;
	char **command;

	/* Option set */
	struct cfg_option options[] = {
		{"help",    'h', NULL,      CFG_BOOL,          (void *) &help,     0},
		{"nice",    'n', "nice",    CFG_INT,           (void *) &nice,     0},
		{"verbose", 'v', "verbose", CFG_BOOL+CFG_MULTI,(void *) &verbose,  0},
		{NULL,      'f', NULL,      CFG_STR,           (void *) &cfg_file, 0},
		{NULL, '\0', "command", CFG_STR+CFG_MULTI_SEPARATED+CFG_LEFTOVER_ARGS,
			(void *) &command, 0},
		CFG_END_OF_LIST
	};

	/* Creating context */
	con = cfg_get_context(options);
	if (con == NULL) {
		puts("Not enough memory");
		return 1;
	}

	/* Parse command line from second argument to end (NULL) */
	cfg_set_cmdline_context(con, 1, -1, argv);

	/* Clearing option variables */
	help     = 0;
	verbose  = 0;
	nice     = 0;
	cfg_file = NULL;
	command  = NULL;

	/* Parsing command line */
	ret = cfg_parse(con);

	if (ret != CFG_OK) {
		printf("error parsing command line: ");
		cfg_fprint_error(con, stdout);
		putchar('\n');
		return ret < 0 ? -ret : ret;
	}

	/* Help screen */
	if (help) {
		printf("libcfg+ example program / compiled %s %s\n",
				__DATE__, __TIME__);
		puts("  Developed by Ondrej Jombik, http://nepto.sk/");
		puts("  Copyright (c) 2001-2002 Platon SDG, http://platon.sk/");
		puts("Usage:");
		printf("  %s [options] command\n", argv[0]);
		puts("Options:");
		puts("  -h, --help      print this help screen");
		puts("  -v, --verbose   verbose (more -v means more verbose)");
		puts("  -n, --nice      process run priority (from 20 to -20)");
		puts("  -f              configuration file load");

		return 1;
	}

	/* Config file testing */
	if (cfg_file != NULL) {
		/* Parse configuration file from the first line to end of file */
		cfg_set_cfgfile_context(con, 0, -1, cfg_file);

		/* Parsing config line */
		ret = cfg_parse(con);

		if (ret != CFG_OK) {
			printf("error parsing config file: ");
			cfg_fprint_error(con, stdout);
			putchar('\n');
			return ret < 0 ? -ret : ret;
		}
	}

	/* Testing if command to run was specified */
	if (command == NULL) {
		puts("error: you must specify program to run");
		return 1;
	}

	/* Nice parameter */
	if (nice != 0) {
		if (verbose)
			printf("info: setting priority (nice) to %d\n", nice);

		if (setpriority(PRIO_PROCESS, PRIO_PROCESS, nice) == -1)
			printf("warning: unable to set priority (nice) to %d\n", nice);
	}

	/* Verbose running info */
	if (verbose) {
		register int i;
		printf("info: running");
		for (i = 0; command[i] != NULL; i++)
			printf(" %s", command[i]);
		putchar('\n');
	}

	execvp(command[0], command);

	perror("error running program");
	return	errno == 0 ? 255 : (errno < 0 ? -errno : errno);
} /* }}} */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

