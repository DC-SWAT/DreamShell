/** 
 * \file    list.h
 * \brief   DreamShell settings
 * \date    2016
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_SETTINGS_H
#define _DS_SETTINGS_H

#include <kos.h>

typedef struct VideoSettings {

	/* Native video mode */
	vid_mode_t mode;
	
	/* Bits per pixel */
	int bpp;

	/* Screen texture size and filter */
	int tex_width;
	int tex_height;
	int tex_filter; /* -1 (auto) */

	// Virtual screen size
	int virt_width;
	int virt_height;

	int reserved[16];

}  VideoSettings_t;


typedef struct Settings {

	/* Core version */
	uint32 version;

	/* Root directory with resources (leave empty for auto detect) */
	char root[MAX_FN_LEN / 2];

	/* Default startup lua script path (without root) or console/lua script content */
	char startup[MAX_FN_LEN / 2];

	/* Default app name */
	char app[64];

	/* Video */
	VideoSettings_t video;

}  Settings_t;


Settings_t *GetSettings();
void SetSettings(Settings_t *settings);
void ResetSettings();

int LoadSettings();
int SaveSettings(int dev); // 0 for VMU, other for HDD/SD

#endif /* _DS_SETTINGS_H */
