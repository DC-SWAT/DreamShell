/*
 * libcfg+ - precise command line & config file parsing library
 *
 * cfgfile.h - config file parsing header file
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

/* $Platon: libcfg+/src/cfgfile.h,v 1.13 2004/01/12 06:03:09 nepto Exp $ */

/**
 * @file	cfgfile.h
 * @brief	config file parsing header file
 * @author	Ondrej Jombik <nepto@platon.sk>
 * @author	Lubomir Host <rajo@platon.sk>
 * @version	\$Platon: libcfg+/src/cfgfile.h,v 1.13 2004/01/12 06:03:09 nepto Exp $
 * @date	2001-2004
 */

#ifndef _PLATON_CFG_CFGFILE_H
#define _PLATON_CFG_CFGFILE_H

/**
 * Parse next config file option(s) and return its value (if non-zero)
 * or error code.
 *
 * @param con	initialized config file context
 * @return		next option val, code of error (CFG_ERROR_*)
 *				or CFG_OK on end
 * @see			cfg_error
 * @see			cfg_context
 */
int cfg_cfgfile_get_next_opt(const CFG_CONTEXT con);

#endif /* _PLATON_CFG_CFGFILE_H */

