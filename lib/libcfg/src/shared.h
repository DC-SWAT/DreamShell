/*
 * libcfg+ - precise command line & config file parsing library
 *
 * shared.h - shared stuff for command line and config file
 *            parsing header file
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

/* $Platon: libcfg+/src/shared.h,v 1.13 2004/01/12 06:03:09 nepto Exp $ */

/**
 * @file	shared.h
 * @brief	shared stuff for command line and config file parsing header file
 * @author	Ondrej Jombik <nepto@platon.sk>
 * @author	Lubomir Host <rajo@platon.sk>
 * @version	\$Platon: libcfg+/src/shared.h,v 1.13 2004/01/12 06:03:09 nepto Exp $
 * @date	2001-2004
 */

#ifndef _PLATON_CFG_SHARED_H
#define _PLATON_CFG_SHARED_H

/**
 * Free current variables (cur_opt, cur_arg) in context
 * and sets cur_opt_type to CFG_NONE_OPTION.
 *
 * @param con	initialized context with initialized current variables
 * @return		void
 */
void __cfg_free_currents(const CFG_CONTEXT con);

/**
 * Process current option and argument. It suppose that in context con
 * are cur_opt, cur_arg and cur_type set.
 *
 * @param con		initialized context
 * @param ret_val	option return value (val) @see cfg_context
 * @param arg_used	if option argument was used
 * @return			CFG_OK on success, CFG_ERROR_* on error
 */
int  __cfg_process_currents(const CFG_CONTEXT con, int *ret_val, int *arg_used);

/**
 * Allocate and initialize variables cur_opt and cur_arg in initialized
 * command line context according to con->argv[con->cur_idx].
 *
 * @param con	initialized command line context
 * @return		CFG_OK on success, CFG_ERR_NOMEM on not enough memory error
 */
int  __cfg_cmdline_set_currents(const CFG_CONTEXT con);

/**
 * Allocate and initialize variables cur_opt and cur_arg in initialized
 * config file context according to input string (parameter buf).
 *
 * @param con	initialized command line context
 * @param buf	input string
 * @return		CFG_OK on success, CFG_ERR_NOMEM on not enough memory error
 */
int  __cfg_cfgfile_set_currents(const CFG_CONTEXT con, char *buf);

#endif /* #ifndef _PLATON_CFG_SHARED_H */

