/*
 * libcfg+ - precise command line & config file parsing library
 *
 * cfg+.h - main implementation header file
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

/* $Platon: libcfg+/src/cfg+.h,v 1.60 2004/01/12 06:03:08 nepto Exp $ */

/**
 * @file	cfg+.h
 * @brief	main implementation header file
 * @author	Ondrej Jombik <nepto@platon.sk>
 * @author	Lubomir Host <rajo@platon.sk>
 * @version	\$Platon: libcfg+/src/cfg+.h,v 1.60 2004/01/12 06:03:08 nepto Exp $
 * @date	2001-2004
 */

#ifndef _PLATON_CFG_H
#define _PLATON_CFG_H

#include <stdio.h>

/** End of options list marker */
#define CFG_END_OPTION { NULL, '\0', NULL, CFG_END, NULL, 0 }
#define CFG_END_OF_LIST CFG_END_OPTION /**< Alias for @ref CFG_END_OPTION */

/**
 * @name Error codes
 */
/*@{*/
/**
 * Possible return values returned by parsing functions.
 */
enum cfg_error { /* {{{ */

	/*@{*/
	/** OK, all is right. */
	CFG_ERR_OK = 0,
	CFG_ERROR_OK = 0,
	CFG_OK = 0,
	/*@}*/

	/** An argument is missing for an option. */
	CFG_ERR_NOARG = -1,
	CFG_ERROR_NOARG = -1,

	/** An argument is not allowed for an option. */
	CFG_ERR_NOTALLOWEDARG = -2,
	CFG_ERROR_NOTALLOWEDARG = -2,

	/** An option's argument could not be parsed. */
	CFG_ERR_BADOPT = -3,
	CFG_ERROR_BADOPT = -3,

	/** Error in quotations. */
	CFG_ERR_BADQUOTE = -4,
	CFG_ERROR_BADQUOTE = -4,

	/** An option could not be converted to appropriate numeric type. */
	CFG_ERR_BADNUMBER = -5,
	CFG_ERROR_BADNUMBER = -5,

	/** A given number was too big or too small. */
	CFG_ERR_OVERFLOW = -6,
	CFG_ERROR_OVERFLOW = -6,

	/** Multiple arguments used for single option. */
	CFG_ERR_MULTI = -7,
	CFG_ERROR_MULTI = -7,

	/** Not enough memory. */
	CFG_ERR_NOMEM = -8,
	CFG_ERROR_NOMEM = -8,

	/** Stop string was found. */
	CFG_ERR_STOP_STR = -9,
	CFG_ERR_STOP_STR_FOUND = -9,
	CFG_ERROR_STOP_STR = -9,
	CFG_ERROR_STOP_STR_FOUND = -9,

	/** No equal sign on the line. */
	CFG_ERR_NOEQUAL = -10,
	CFG_ERROR_NOEQUAL = -10,

	/** An unknown option. */
	CFG_ERR_UNKNOWN = -11,
	CFG_ERROR_UNKNOWN = -11,

	/** File not found error. */
	CFG_ERR_FILE_NOT_FOUND = -12,
	CFG_ERROR_FILE_NOT_FOUND = -12,

	/** Seek error (fseek() failure). */
	CFG_ERR_SEEK_ERROR = -13,
	CFG_ERROR_SEEK_ERROR = -13,

	/** Internal error. */
	CFG_ERR_INTERNAL = -20,
	CFG_ERROR_INTERNAL = -20

}; /* }}} */ /*@}*/

/**
 * @name Context flags
 */
/*@{*/
/**
 * By default are @ref CFG_PROCESS_FIRST, @ref CFG_POSIXLY_LEFTOVERS and
 * @ref CFG_NORMAL_LEFTOVERS initialized.
 * @todo CFG_APPEND, CFG_OVERWRITE, CFG_APPEND_MULTI_ONLY
 */
enum cfg_flag { /* {{{ */

	/** Ignore multiple arguments for single option. */
	CFG_IGNORE_MULTI = 1,

	/** Ignore unknown options. */
	CFG_IGNORE_UNKNOWN = 2,

	/** Process also the first argument on command line. */
	CFG_PROCESS_FIRST = 0,

	/** Skip processing of the first argument on command line. */
	CFG_SKIP_FIRST = 4,

	/** Posixly correct leftover arguments. */
	CFG_POSIXLY_LEFTOVERS = 0,

	/** Advanced leftover arguments. */
	CFG_ADVANCED_LEFTOVERS = 8,

	/** Normal leftover arguments initialization in file.
	  This flag is not used and it is kept from historical reasons. */
	CFG_NORMAL_LEFTOVERS = 0,

	/** Strict leftover arguments initialization in file.
	  This flag is not used and it is kept from historical reasons. */
	CFG_STRICT_LEFTOVERS = 16,

	/** Byte type position usage in file. */
	CFG_FILE_BYTE_POS_USAGE = 0,

	/** Line type position usage in file. */
	CFG_FILE_LINE_POS_USAGE = 32

		/* Ignore all quotations in options arguments. */
		/*
		   CFG_USE_QUOTE = 0,
		   CFG_IGNORE_QUOTE = 16
		   */
		/* Advanced quotations are things like	option = va"'l'"ue
		   which resolves to					va'l'ue.

		   We really want this strange stuff? Any volunter?

		   CFG_ADVANCED_QUOTE = 32 + 16
		   */

}; /* }}} */ /*@}*/

/**
 * @name Option types
 */
/*@{*/
/**
 * Possible types of options
 * @todo Thinking about case insensitivy of option
 * ("--input" is the same as "--INPUT")
 */
enum cfg_option_type { /* {{{ */

	/** Boolean */
	CFG_BOOL = 1,
	CFG_BOOLEAN = 1,

	/** Integer */
	CFG_INT = 2,
	CFG_INTEGER = 2,

	/** Unsigned int */
	CFG_UINT = 3,
	CFG_UNSIGNED = 3,
	CFG_UNSIGNED_INT = 3,

	/** Long */
	CFG_LONG = 4,

	/** Unsigned long */
	CFG_ULONG = 5,
	CFG_UNSIGNED_LONG = 5,

	/** Float */
	CFG_FLOAT = 6,

	/** Double */
	CFG_DOUBLE = 7,

	/** String */
	CFG_STR = 8,
	CFG_STRING = 8,

	/** End (to mark last item in list) */
	CFG_END = 0,

	/** Data type mask (used internally) */
	CFG_DATA_TYPE_MASK = 31,

	/**
	 * Single, multi or multi separated. Single by default.
	 * Tells if option can be repeated.
	 * In multi case value is array of poiters to type ended with NULL.
	 */
	CFG_SINGLE = 0,
	CFG_MULTI = 32,
	CFG_MULTI_ARRAY = 32,
	CFG_MULTI_SEPARATED = 32 + 64,

	/**
	 * Leftover arguments specification.
	 * Mark option which will contain leftover arguments.
	 */
	CFG_LAST_ARGS = 128,
	CFG_LAST_ARGUMENTS = 128,
	CFG_LEFTOVER_ARGS = 128,
	CFG_LEFTOVER_ARGUMENTS = 128

}; /* }}} */ /*@}*/

/**
 * @name Property types
 */
/*@{*/
enum cfg_property_type { /* {{{ */

	/**
	 * @name Property codes
	 */
	/*@{*/

	/** Array of strings which forces to stop command line parsing.
	  By default it is empty. */
	CFG_LINE_STOP_STRING = 0,

	/** Command line short option prefix.
	  By default is "-" initialized. */
	CFG_LINE_SHORT_OPTION_PREFIX = 1,

	/** Command line long option prefix.
	  By default is "--" initialized. */
	CFG_LINE_LONG_OPTION_PREFIX = 2,

	/** Command line option argument separator.
	  By default is "=" initialized. */
	CFG_LINE_OPTION_ARG_SEPARATOR = 3,

	/** Command line multi values separator.
	  By default are "," and ";" initialized. */
	CFG_LINE_NORMAL_MULTI_VALS_SEPARATOR = 4,

	/** Command line multi values leftover arguments separator.
	  By default it is empty. */
	CFG_LINE_LEFTOVER_MULTI_VALS_SEPARATOR = 5,

	/** Command line quote prefix & postfix.
	  By default are apostrophes (') and quotations (") initlized for both.*/
	CFG_LINE_QUOTE_PREFIX  = 6,
	CFG_LINE_QUOTE_POSTFIX = 7,

	/** Array of strings prefixes which forces to stop config file parsing.
	  By default it is empty. */
	CFG_FILE_STOP_PREFIX = 8,

	/** Array of string prefixes which mark comment line.
	  By default are "#" and ";" initialized.  */
	CFG_FILE_COMMENT_PREFIX = 9,

	/** Array of string postfixes to determine multi lines.
	  By default is "\\" initialized. */
	CFG_FILE_MULTI_LINE_POSTFIX = 10,

	/** Config file option argument separator.
	  By default is "=" initialized. */
	CFG_FILE_OPTION_ARG_SEPARATOR = 11,

	/** Config file multi values separator.
	  By default are ",", ";" and " " initialized. */
	CFG_FILE_NORMAL_MULTI_VALS_SEPARATOR = 12,

	/** Command line multi values leftover arguments separator.
	  By default is " " initialized. */
	CFG_FILE_LEFTOVER_MULTI_VALS_SEPARATOR = 13,

	/** Config file quote prefix & postfix.
	  By default are apostrophes (') and quotations (\") initlized for both.*/
	CFG_FILE_QUOTE_PREFIX  = 14,
	CFG_FILE_QUOTE_POSTFIX = 15,

	/*@}*/

	/**
	 * @name Count of normal properties
	 */
	/*@{*/
	/** Special properties count */
	CFG_N_PROPS = 16,
	/*@}*/

	/**
	 * @name Virtual property codes
	 */
	/*@{*/

	/** File quote prefix & postfix */
	CFG_QUOTE         = 50,
	CFG_LINE_QUOTE    = 51,
	CFG_FILE_QUOTE    = 52,
	CFG_QUOTE_PREFIX  = 53,
	CFG_QUOTE_POSTFIX = 54,

	/** Multi values separator */
	CFG_MULTI_VALS_SEPARATOR          = 55,
	CFG_FILE_MULTI_VALS_SEPARATOR     = 56,
	CFG_LINE_MULTI_VALS_SEPARATOR     = 57,
	CFG_NORMAL_MULTI_VALS_SEPARATOR   = 58,
	CFG_LEFTOVER_MULTI_VALS_SEPARATOR = 59,

	/** Option argument separator */
	CFG_OPTION_ARG_SEPARATOR = 60
		/*@}*/
}; /* }}} */

/**
 * Terminators of variable number arguments in functions cfg_add_properties(),
 * cfg_set_properties(), cfg_get_properties() and similar.
 */
#define CFG_EOT			CFG_N_PROPS
#define CFG_END_TYPE	CFG_N_PROPS

/*@}*/

/**
 * @name Internal enumerations
 */
/*@{*/
/**
 * Context type
 *
 * Possible types of context (used internally)
 */
enum cfg_context_type { /* {{{ */

	/** No context */
	CFG_NO_CONTEXT = 0,

	/** Command line context type */
	CFG_CMDLINE = 1,
	CFG_LINE = 1,

	/** Config file context type */
	CFG_CFGFILE = 2,
	CFG_FILE = 2
}; /* }}} */

/**
 * Command line option type.
 *
 * Possible types of command line option (used internally)
 */
enum cfg_line_option_type { /* {{{ */

	/** Not long and not short option */
	CFG_NONE_OPTION = 0,

	/** Short command line option */
	CFG_SHORT_OPTION = 1,

	/** Long command line option */
	CFG_LONG_OPTION = 2,

	/** Short command line options */
	CFG_SHORT_OPTIONS = 4,

	/** Long command line option argument initialized with separator */
	CFG_LONG_SEPINIT = 8,

	/** Long command line option argument initialized without separator (default) */
	CFG_LONG_NOSEPINIT = 0
}; /* }}} */ /*@}*/

/**
 * @brief Structure for defining one config option
 */
struct cfg_option { /* {{{ */
	/** Command line long name (may be NULL) */
	const char *cmdline_long_name;
	/** Command line short name (may be '\0') */
	const char  cmdline_short_name;
	/** Config file name (may be NULL) */
	const char *cfgfile_name;

	/** Option type
	  @see cfg_option_type */
	const enum cfg_option_type type;

	/** Pointer where to store value of option */
	void *value;

	/** Return value (set to 0 for not return) */
	int val;
}; /* }}} */

/**
 * @brief Main structure for defining context
 */
struct cfg_context { /* {{{ */

	/**
	 * @name Shared properties
	 */
	/*@{*/

	/** Context type (command line or config file) */
	enum cfg_context_type type;

	/** Flags */
	int flags;

	/** Options table */
	const struct cfg_option *options;

	/** Starting parsing position */
	long begin_pos;

	/** Number of elements (array arguments, bytes or lines) to parse
	  (value of -1 means infinite) */
	long size;

	/** Array of used options indexes */
	int *used_opt_idx;

	/** Error code of last occured error. */
	enum cfg_error error_code;

	/** Special properties */
	char **prop[CFG_N_PROPS];

	/** Currents */
	long cur_idx;
	long cur_idx_tmp;
	int  cur_opt_type;

	/** Current option string */
	char *cur_opt;

	/** Current option argument*/
	char *cur_arg;

	/*@}*/

	/**
	 * @name Command line specific properties
	 */
	/*@{*/

	/** Flag to detect if parsing already started */
	int parsing_started:1;

	/** NULL terminated array of argument */
	char **argv;

	/*@}*/

	/**
	 * @name Config file specific properties.
	 */
	/*@{*/

	/** Filename (name of file) */
	char *filename;

	/** Pointer to FILE* structure of parsed file */
	FILE *fhandle;

	/*@}*/
}; /* }}} */

/**
 * @brief Context data type
 */
typedef struct cfg_context * CFG_CONTEXT;

/*
 * Functions
 */

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @name Functions and macros for creating and manipulating context
	 */
	/*@{*/ /* {{{ */

	/**
	 * Initialize core context
	 *
	 * @param options	pointer to options table
	 * @return			initialized core context; further specification
	 *					to command line or config file is required
	 */
	CFG_CONTEXT cfg_get_context(struct cfg_option *options);

	/**
	 * Initialize command line context
	 *
	 * @param begin_pos	index of beginning argument of arguments array
	 * @param size		maximal number of array elements to parse
	 *					(set value of -1 for infinite)
	 * @param argv		arguments array
	 * @param options	pointer to options table
	 * @return			initialized command line context
	 */
	CFG_CONTEXT cfg_get_cmdline_context(
			long begin_pos,
			long size,
			char **argv,
			struct cfg_option *options);

#define cfg_get_cmdline_context_pos(begin_pos, end_pos, argv, options) \
	cfg_get_cmdline_context( \
			begin_pos, \
			end_pos - begin_pos + 1, \
			argv, \
			options)

		/**
		 * Initialize command line context by argc and argv passed to main()
		 *
		 * @param argc		argumet count (argc) passed to function main()
		 * @param argv		arguments array (argv) passed to function main()
		 * @param options	pointer to options table
		 * @return			initialized command line context
		 */
		CFG_CONTEXT cfg_get_cmdline_context_argc(
				int argc,
				char **argv,
				struct cfg_option *options);

	/**
	 * Initialize configuration file context
	 *
	 * @param begin_pos	starting position in file to parse
	 * @param size		maximal number of bytes/lines in file to parse
	 *					(set value of -1 for infinite)
	 * @param filename	parsed filename
	 * @param options	pointer to options table
	 * @return			initialized command line context
	 */
	CFG_CONTEXT cfg_get_cfgfile_context(
			long begin_pos,
			long size,
			char *filename,
			struct cfg_option *options);

#define cfg_get_cfgfile_context_pos(begin_pos, end_pos, argv, options) \
	cfg_get_cfgfile_context( \
			begin_pos, \
			end_pos - begin_pos + 1, \
			argv, \
			options)

		/**
		 * Set context to command line
		 *
		 * @param con		initialized context
		 * @param begin_pos	index of beginning argument of arguments array
		 * @param size		maximal number of array elements to parse
		 *					(set value of -1 for infinite)
		 * @param argv		arguments array
		 * @return			void
		 */
		void cfg_set_cmdline_context(
				const CFG_CONTEXT con,
				long begin_pos,
				long size,
				char **argv);

#define cfg_set_cmdline_context_pos(con, begin_pos, end_pos, argv) \
	cfg_get_cmdline_context( \
			con \
			begin_pos, \
			end_pos - begin_pos + 1, \
			argv)

		/**
		 * Set context to command line by argc and argv passed to main()
		 *
		 * @param con		initialized context
		 * @param argc		argumet count (argc) passed to function main()
		 * @param argv		arguments array (argv) passed to function main()
		 * @return			initialized command line context
		 */
		void cfg_set_cmdline_context_argc(
				const CFG_CONTEXT con,
				int argc,
				char **argv);

	/**
	 * Set context to configuration file
	 *
	 * @param con		initialized context
	 * @param begin_pos	starting position in file to parse
	 * @param size		maximal number of bytes/lines in file to parse
	 *					(set value of -1 for infinite)
	 * @param filename	parsed filename
	 * @return			void
	 */
	void cfg_set_cfgfile_context(
			const CFG_CONTEXT con,
			long begin_pos,
			long size,
			char *filename);

#define cfg_set_cfgfile_context_pos(con, begin_pos, end_pos, argv) \
	cfg_get_cfgfile_context( \
			con \
			begin_pos, \
			end_pos - begin_pos + 1, \
			argv)

		/**
		 * Reinitialize popt context
		 *
		 * @param con		initialized context
		 * @return			void
		 */
		void cfg_reset_context(const CFG_CONTEXT con);

	/**
	 * Destroy context
	 *
	 * @param con		initialized context
	 * @return			void
	 */
	void cfg_free_context(const CFG_CONTEXT con);

	/* }}} */ /*@}*/

	/**
	 * @name Functions for setting and clearing context flags
	 */
	/*@{*/ /* {{{ */

	/**
	 * Set context flag
	 *
	 * @param con	initialized context
	 * @param flag	context flag
	 * @return		void
	 */
	void cfg_set_context_flag(const CFG_CONTEXT con, int flag);

	/**
	 * Clear context flag
	 *
	 * @param con	initialized context
	 * @param flag	context flag
	 * @return		void
	 */
	void cfg_clear_context_flag(const CFG_CONTEXT con, int flag);

	/**
	 * Get context flag
	 *
	 * @param con	initialized context
	 * @param flag	context flag
	 * @return		0 on false, non-zero on true
	 */
	int cfg_get_context_flag(const CFG_CONTEXT con, int flag);

#define cfg_is_context_flag(con, flag) cfg_get_context_flag(con, flag)

	/**
	 * Overwrite context flags
	 *
	 * @param con	initialized context
	 * @param flags	context flags
	 * @return		void
	 */
	void cfg_set_context_flags(const CFG_CONTEXT con, int flags);

	/**
	 * Get all context flags
	 *
	 * @param con	initialized context
	 * @return		all context flags
	 */
	int cfg_get_context_flags(const CFG_CONTEXT con);

	/* }}} */ /*@}*/

	/**
	 * @name Functions and macros for properties manipulation
	 */
	/*@{*/ /* {{{ */

	/**
	 * Clear all strings of property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 */
	int cfg_clear_property(
			const CFG_CONTEXT con, enum cfg_property_type type);

	/**
	 * Clear all strings of property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 */
	int cfg_clear_properties(
			const CFG_CONTEXT con, enum cfg_property_type type, ...);


	/**
	 * Add string to property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @param str	string for addition
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 */
	int cfg_add_property(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str);

	/**
	 * Add multiple strings to particular properties
	 *
	 * @param con	initialized context
	 * @param type  property type(s)
	 * @param str	string(s) for addition
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with typeN = CFG_EOT or strN = NULL.
	 * Use constructions like this:<br>
	 * cfg_add_properties(con, type1, str1, type2, str2, type3=CFG_EOT)
	 */
	int cfg_add_properties(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str, ...);

	/**
	 * Add string to multiple properties
	 *
	 * @param con	initialized context
	 * @param str	string for addition
	 * @param type  property type(s)
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with typeN = CFG_EOT. Use constructions
	 * like this:<br>
	 * cfg_add_properties(con, str, type1, type2, type3=CFG_EOT)
	 */
	int cfg_add_properties_str(
			const CFG_CONTEXT con, char *str, enum cfg_property_type type, ...);

	/**
	 * Add multiple strings to one property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @param str	string(s) for addition
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with strN = NULL. Use constructions
	 * like this:<br>
	 * cfg_add_properties(con, type, str1, str2, str3=NULL)
	 */
	int cfg_add_properties_type(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str, ...);

	/**
	 * Remove string from property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @param str	string for removal
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 */
	int cfg_remove_property(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str);

	/**
	 * Remove multiple strings from particular properties
	 *
	 * @param con	initialized context
	 * @param type  property type(s)
	 * @param str	string(s) for removal
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with typeN = CFG_EOT or strN = NULL.
	 * Use constructions like this:<br>
	 * cfg_remove_properties(con, type1, str1, type2, str2, type3=CFG_EOT)
	 */
	int cfg_remove_properties(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str, ...);

	/**
	 * Remove string from multiple properties
	 *
	 * @param con	initialized context
	 * @param str	string for removal
	 * @param type  property type(s)
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with typeN = CFG_EOT. Use constructions
	 * like this:<br>
	 * cfg_remove_properties(con, str, type1, type2, type3=CFG_EOT)
	 */
	int cfg_remove_properties_str(
			const CFG_CONTEXT con, char *str, enum cfg_property_type type, ...);

	/**
	 * Remove multiple strings from one property
	 *
	 * @param con	initialized context
	 * @param type  property type
	 * @param str	string(s) for removal
	 * @return		1 on success, 0 on not enough memory error
	 * @see			cfg_property_type
	 *
	 * Argument list must be terminated with strN = NULL. Use constructions
	 * like this:<br>
	 * cfg_add_properties(con, type, str1, str2, str3=NULL)
	 */
	int cfg_remove_properties_type(
			const CFG_CONTEXT con, enum cfg_property_type type, char *str, ...);

	/* }}} */ /*@}*/

	/**
	 * @name Functions for processing context options
	 */
	/*@{*/ /* {{{ */

	/**
	 * Parse context
	 *
	 * @param con	initialized context
	 * @return		code of error (CFG_ERROR_*)
	 *				or CFG_OK if parsing was sucessfull
	 * @see			cfg_error
	 */
	int cfg_parse(const CFG_CONTEXT con);

	/**
	 * Parse next option(s) and return its value (if non-zero) or error code.
	 *
	 * @param con	initialized context
	 * @return		next option val, code of error (CFG_ERROR_*)
	 *				or CFG_OK on end
	 * @see			cfg_error
	 * @see			cfg_context
	 */
	int cfg_get_next_opt(const CFG_CONTEXT con);

	/**
	 * Return currently processed option name
	 *
	 * @param con	initialized context
	 * @return		pointer to current option name
	 */
	char *cfg_get_cur_opt(const CFG_CONTEXT con);

	/**
	 * Return currently processed option argument
	 *
	 * @param con	initialized context
	 * @return		pointer to current option argument
	 */
	char *cfg_get_cur_arg(const CFG_CONTEXT con);

	/**
	 * Return currently processed option index (argv index in command line
	 * context, file byte position or line position in config file context)
	 *
	 * @param con	initialized context @return		index of current option
	 */
	int cfg_get_cur_idx(const CFG_CONTEXT con);

	/* }}} */ /*@}*/

	/**
	 * @name Error handling functions
	 */
	/*@{*/ /* {{{ */

	/**
	 * Print error string to stderr
	 *
	 * @param con		initialized context
	 * @return			void
	 */
	void cfg_print_error(const CFG_CONTEXT con);

	/**
	 * Print error string to stream
	 *
	 * @param con		initialized context
	 * @param fh		stream opened for writting
	 * @return			void
	 */
	void cfg_fprint_error(const CFG_CONTEXT con, FILE *fh);

	/**
	 * Get error string; error string is dynamically allocated, it needs to be
	 * freed after use.
	 *
	 * @param con		initialized context
	 * @return			dynamically allocated error string
	 */
	char *cfg_get_error_str(const CFG_CONTEXT con);

	/**
	 * Get static error string
	 *
	 * @param errorcode	code of libcfg error
	 * @return			static error string
	 */
	char *cfg_get_static_error_str(const int errorcode);

	/* }}} */ /*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _PLATON_CFG_H */

/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=0:
 * }}} */

