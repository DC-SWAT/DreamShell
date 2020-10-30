/* DreamShell ##version##

   module.c - GDPlay app module
   Copyright (C)2014 megavolt85 

*/

#include "ds.h"
#include <dc/sound/sound.h>

DEFAULT_MODULE_EXPORTS(app_gdplay);

typedef struct ip_meta
{
	char hardware_ID[16];
	char maker_ID[16];
	char ks[5];
	char disk_type[6];
	char disk_num[5];
	char country_codes[8];
	char ctrl[4];
	char dev[1];
	char VGA[1];
	char WinCE[1];
	char unk[1];
	char product_ID[10];
	char product_version[6];
	char release_date[8];
	char unk2[8];
	char boot_file[16];
	char software_maker_info[16];
	char title[128];
} ip_meta_t;

enum
{
	SURF_GDROM 		= 0,
	SURF_MILCD 		= 1,
	SURF_AUDIOCD 	= 2,
	SURF_CDROM 		= 3,
	SURF_NODISC 	= 4,
	SURF_ONCD 		= 5,
};

enum
{
	TXT_REGION 	= 0,
	TXT_VGA 	= 1,
	TXT_DATE 	= 2,
	TXT_DISCNUM = 3,
	TXT_VERSION = 4,
	TXT_TITLE1 	= 5,
	TXT_TITLE2 	= 6,
	TXT_TITLE3 	= 7,
	TXT_TITLE4 	= 8,
	TXT_TITLE5 	= 9,
	TXT_END 	= 10,
};

#define COLUMN_LEN 26

static struct self 
{
	App_t *app;
	ip_meta_t *info;
	
	GUI_Widget 	*play_btn;
	GUI_Widget 	*text[10];
	GUI_Surface *gdtex[6];
	GUI_Surface *play;
	
	void *bios_patch;
} self;

kthread_t *check_gdrom_thd;
int kill_gdrom_thd = 0;

void gdplay_run_game(void *param);
void _atexit_call_all();

#if __GNUC__ >= 4
void init(void);
void fini(void);
#endif

static void clear_text(void)
{
	int i;
	
	for(i=0; i<TXT_END; i++)
		GUI_LabelSetText(self.text[i], " ");
	
	self.info = NULL;
}

static char *trim_spaces(char *txt, int len)
{
	int32_t i;
	
	while(txt[0] == ' ')
	{
		txt++;
	}
	
	if(!len)
		len = strlen(txt);
	
	for(i=len; i ; i--)
	{
		if(txt[i] > ' ') break;
		txt[i] = '\0';
	}
	
	return txt;
}

static void set_img(GUI_Surface *surface, int enable)
{
	GUI_ButtonSetNormalImage(self.play_btn, surface);
	GUI_ButtonSetHighlightImage(self.play_btn, self.play);
	GUI_ButtonSetPressedImage(self.play_btn, surface);
	GUI_ButtonSetDisabledImage(self.play_btn, surface);
	
	GUI_WidgetSetEnabled(self.play_btn, enable);
	
//	self.rotate = surface;
}

static void set_info(int disc_type)
{
	int lba = 45150;
	char pbuff[2048];
	
	if(FileExists("/cd/0gdtex.pvr"))
	{
		self.gdtex[SURF_ONCD] = GUI_SurfaceLoad("/cd/0gdtex.pvr");
		set_img(self.gdtex[SURF_ONCD], 1);
	}
	else
	{
		set_img(self.gdtex[(disc_type == CD_CDROM_XA)], 1);
	}
	
	if(disc_type == CD_CDROM_XA)
	{
		CDROM_TOC toc;
		
		if(cdrom_read_toc(&toc, 0) != CMD_OK) 
		{ 
			set_img(self.gdtex[SURF_CDROM], 0);
			ds_printf("DS_ERROR: Toc read error\n"); 
			return; 
		}
		if(!(lba = cdrom_locate_data_track(&toc))) 
		{
			set_img(self.gdtex[SURF_CDROM], 0);
			ds_printf("DS_ERROR: Error locate data track\n"); 
			return;
		}
	}
	
	if (cdrom_read_sectors(pbuff,lba , 1)) 
	{
		ds_printf("DS_ERROR: CD read error %d\n",lba); 
		set_img(self.gdtex[SURF_CDROM], 0);
		return;
	}
	
	self.info = (ip_meta_t *) pbuff;
	
	if(strncmp(self.info->hardware_ID, "SEGA", 4))
	{
		set_img(self.gdtex[SURF_CDROM], 0);
		return;
	}
	
	char tmp[MAX_FN_LEN];
	
	if(strlen(trim_spaces(self.info->country_codes, sizeof(self.info->country_codes))) > 1)
	{
		strcpy(tmp, "FREE");
	}
	else
	{
		switch(self.info->country_codes[0])
		{
			case 'J':
				strcpy(tmp, "JAPAN");
				break;
			case 'U':
				strcpy(tmp, "USA");
				break;
			case 'E':
				strcpy(tmp, "EUROPE");
				break;
		}
	}
	
	GUI_LabelSetText(self.text[TXT_REGION], tmp);
	
	GUI_LabelSetText(self.text[TXT_VGA], self.info->VGA[0] == '1'? "YES":"NO");
	
	memset(tmp, 0, MAX_FN_LEN);
	
	snprintf(tmp, COLUMN_LEN, "%c%c%c%c-%c%c-%c%c", self.info->release_date[0], 
													self.info->release_date[1], 
													self.info->release_date[2], 
													self.info->release_date[3], 
													self.info->release_date[4], 
													self.info->release_date[5], 
													self.info->release_date[6], 
													self.info->release_date[7]);
	
	GUI_LabelSetText(self.text[TXT_DATE], trim_spaces(tmp, 8));
	
	snprintf(tmp, COLUMN_LEN, "%c OF %c", self.info->disk_num[0], self.info->disk_num[2]);
	
	GUI_LabelSetText(self.text[TXT_DISCNUM], tmp);
	
	memset(tmp, 0, MAX_FN_LEN);
	memcpy(tmp, self.info->product_version, 6);
	
	GUI_LabelSetText(self.text[TXT_VERSION], tmp);
	
	memset(tmp, 0, MAX_FN_LEN);
	strncpy(tmp, trim_spaces(self.info->title, 128), 128);
	
	int num_column = strlen(tmp) / COLUMN_LEN;
	
	if((strlen(tmp) % COLUMN_LEN))
		num_column++;
	
	char column_txt[COLUMN_LEN+1];
	
	for(int i=0; i<num_column; i++)
	{
		strncpy(column_txt, &tmp[i*COLUMN_LEN], COLUMN_LEN);
		GUI_LabelSetText(self.text[TXT_TITLE1 + i], column_txt);
	}
}

static void check_cd(void)
{
	int status, disc_type, cd_status;
	clear_text();
getstatus:	
	if((cd_status = cdrom_get_status(&status, &disc_type)) != ERR_OK) 
	{
		switch(cd_status)
		{
			case ERR_DISC_CHG:
				cdrom_reinit();
				goto getstatus;
				break;
			default:
				set_img(self.gdtex[SURF_NODISC], 0);
				return;
		}
	}
	
	switch(status)
	{
		case CD_STATUS_OPEN:
		case CD_STATUS_NO_DISC:
			set_img(self.gdtex[SURF_NODISC], 0);
			return;
	}
		
	switch(disc_type)
	{
		case CD_CDDA:
			set_img(self.gdtex[SURF_AUDIOCD], 0);
			break;
		case CD_GDROM:
		case CD_CDROM_XA:
			set_info(disc_type);
			break;
		case CD_CDROM:
		case CD_CDI:
		default:
			set_img(self.gdtex[SURF_CDROM], 0);
			break;
	}
	
	cdrom_spin_down();
}

static void *check_gdrom()
{
	int status, disc_type, cd_status;
	
	while(!kill_gdrom_thd)
	{
		cd_status = cdrom_get_status(&status, &disc_type);
		
		switch(cd_status)
		{
			case ERR_DISC_CHG:
				check_cd();
				break;
			default:
				switch(status)
				{
					case CD_STATUS_OPEN:
					case CD_STATUS_NO_DISC:
						if(self.info)
							clear_text();
							set_img(self.gdtex[SURF_NODISC], 0);
						break;
					default:
						if(!self.info)
							check_cd();
						break;
				}
		}
		thd_pass();
	}
	
	return NULL;
}

/*
int rotate_en = 0;
kthread_t *rotate_thd;

static void *rotate_gdtex()
{
	SDL_Surface *surf;
	rotate_en = 1;
	
	while(rotate_en)
	{
		surf = GUI_SurfaceGet(self.rotate);
		GUI_Surface *s = GUI_SurfaceFrom("rotated-surface", rotozoomSurface(surf, 1.0, 1.0, 1));
		GUI_ButtonSetHighlightImage(self.play_btn, s);
		self.rotate = s;
		GUI_WidgetMarkChanged(self.app->body);
	}
	
	return NULL;
}

void gdplay_rotate_on(GUI_Widget *widget)
{
	(void) widget;
	ds_printf("gdplay_rotate_on\n");
	if(!rotate_en)
		rotate_thd = thd_create(1, rotate_gdtex, NULL);
}

void gdplay_rotate_off(GUI_Widget *widget)
{
	(void) widget;
	ds_printf("gdplay_rotate_off\n");
	rotate_en = 0;
	thd_join(rotate_thd, NULL);
}
*/

void gdplay_play(GUI_Widget *widget)
{
	(void) widget;
	
	kill_gdrom_thd = 1;
	thd_join(check_gdrom_thd, NULL);
	
	ShutdownVideoThread();
	expt_shutdown();
	g1_ata_shutdown();
	
#if __GNUC__ < 4
    arch_dtors();
#else
# if __GNUC__ < 8
    fini();
# endif
#endif

    ubc_disable_all();
    fs_dclsocket_shutdown();
	net_shutdown();
	irq_disable();
	snd_shutdown();
	timer_shutdown();
	la_shutdown();
	bba_shutdown();
	maple_shutdown();
	cdrom_shutdown();
	spu_dma_shutdown();
	spu_shutdown();
	pvr_shutdown();
	library_shutdown();
	fs_dcload_shutdown();
	fs_vmu_shutdown();
	vmufs_shutdown();
	fs_iso9660_shutdown();
	fs_ramdisk_shutdown();
	fs_romdisk_shutdown();
	fs_pty_shutdown();
	fs_shutdown();
	thd_shutdown();
	rtc_shutdown();
	irq_shutdown();
	
	gdplay_run_game(self.bios_patch);
}

void gdplay_Init(App_t *app) 
{	
	if(app != NULL) 
	{
		memset(&self, 0, sizeof(self));
		
		self.app = app;
		
		self.play_btn 			= APP_GET_WIDGET("play-btn");
		self.text[TXT_REGION] 	= APP_GET_WIDGET("region-txt");
		self.text[TXT_VGA] 		= APP_GET_WIDGET("vga-txt");
		self.text[TXT_DATE] 	= APP_GET_WIDGET("date-txt");
		self.text[TXT_DISCNUM] 	= APP_GET_WIDGET("disk-num-txt");
		self.text[TXT_VERSION] 	= APP_GET_WIDGET("version-txt");
		self.text[TXT_TITLE1] 	= APP_GET_WIDGET("title1-txt");
		self.text[TXT_TITLE2] 	= APP_GET_WIDGET("title2-txt");
		self.text[TXT_TITLE3] 	= APP_GET_WIDGET("title3-txt");
		self.text[TXT_TITLE4] 	= APP_GET_WIDGET("title4-txt");
		self.text[TXT_TITLE5] 	= APP_GET_WIDGET("title5-txt");
		
		self.gdtex[SURF_GDROM] 	= APP_GET_SURFACE("gdrom");
		self.gdtex[SURF_MILCD] 	= APP_GET_SURFACE("milcd");
		self.gdtex[SURF_AUDIOCD]= APP_GET_SURFACE("cdaudio");
		self.gdtex[SURF_CDROM] 	= APP_GET_SURFACE("cdrom");
		self.gdtex[SURF_NODISC] = APP_GET_SURFACE("nodisc");
		self.play 				= APP_GET_SURFACE("play");
		
		char path[MAX_FN_LEN];
		
		snprintf(path, MAX_FN_LEN, "%s/firmware/rungd.bin", getenv("PATH"));
		
		FILE *f = fopen(path, "rb");
		
		if(!f)
		{
			ds_printf("DS_ERROR: Can't open %s\n", path);
			ShowConsole();
			return;
		}
		
		self.bios_patch = memalign(32, 65280);
		
		fread(self.bios_patch, 65280, 1, f);
		
		fclose(f);
		
		check_cd();
		
		check_gdrom_thd = thd_create(1, check_gdrom, NULL);
	} 
	else 
	{
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}

void gdplay_Exit() 
{
/*	rotate_en = 0;
	thd_join(rotate_thd, NULL);
*/	
	kill_gdrom_thd = 1;
	thd_join(check_gdrom_thd, NULL);
	
	if(self.bios_patch)
		free(self.bios_patch);
}

