/** 
 * \file    sfx.h
 * \brief   DreamShell sound FX
 * \date    2024
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_SFX_H
#define _DS_SFX_H

#include <kos.h>

typedef enum {

	DS_SFX_STARTUP,
	DS_SFX_LAST_STREAM,
	DS_SFX_CLICK = DS_SFX_LAST_STREAM,
	DS_SFX_CLICK2,
	DS_SFX_SCRSHOT,
	DS_SFX_LAST
} ds_sfx_t;

int ds_sfx_play(ds_sfx_t sfx);

#endif /* _DS_SFX_H */
