/** 
 * \file    ds.h
 * \brief   DreamShell core header
 * \date    2004-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_CORE_H
#define _DS_CORE_H

#include <kos.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "video.h"
#include "list.h"
#include "module.h"
#include "events.h"
#include "console.h"
#include "lua.h"
#include "app.h"
#include "utils.h"
#include "cfg+.h"
#include "exceptions.h"

/**
 * Get DreamShell version code
 */
uint32 GetVersion();

/**
 * Set DreamShell version code and update env string
 */
void SetVersion(uint32 ver);

/**
 * Same as getenv("VERSION")
 */
char *GetVersionString() __attribute__((deprecated));

/**
 * Get the build type as string (Alpha, Beta, RC and Release)
 */
const char *GetVersionBuildTypeString(int type);

/**
 * Initialize and shutdown DreamShell core
 */
int InitDS();
void ShutdownDS();

/**
 * Main(input) thread locking
 */
void LockInput();
void UnlockInput();
int InputIsLocked();
int InputMustLock();


#endif /* _DS_CORE_H */
