/** 
 * \file    sfx.h
 * \brief   DreamShell sound FX
 * \date    2024-2025
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_SFX_H
#define _DS_SFX_H

typedef enum {

	DS_SFX_STARTUP,
	DS_SFX_LAST_STREAM,
	DS_SFX_CLICK = DS_SFX_LAST_STREAM,
	DS_SFX_CLICK2,
	DS_SFX_SCRSHOT,
	DS_SFX_MOVE,
	DS_SFX_CH_PAGE,
	DS_SFX_SLIDE,
	DS_SFX_ERROR,
	DS_SFX_SUCCESS,
	DS_SFX_LAST
} ds_sfx_t;

int ds_sfx_play(ds_sfx_t sfx);
int ds_sfx_is_enabled(ds_sfx_t sfx);
void ds_sfx_get_wav(char sfx_path[], ds_sfx_t sfx_sel);
void ds_sfx_preload(void);

#endif /* _DS_SFX_H */
