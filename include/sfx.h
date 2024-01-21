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

	DS_SFX_STARTUP

} ds_sfx_t;

int ds_sfx_play(ds_sfx_t sfx);

#endif /* _DS_SFX_H */
