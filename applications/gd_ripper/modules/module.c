/* DreamShell ##version##

   module.c - GD Ripper app module
   Copyright (C)2014 megavolt85
   Copyright (C)2025 SWAT

*/

#include "ds.h"
#include "isofs/isofs.h"
#include <stdint.h>

DEFAULT_MODULE_EXPORTS(app_gd_ripper);

#define SEC_BUF_SIZE 16
#define UI_UPDATE_INTERVAL 500

static int rip_sec(int tn,int first,int count,int type,char *dst_file, int disc_type);
static void* gd_ripper_thread(void *arg);
static int gdfiles(char *dst_folder,char *dst_file,char *text);
static int get_disc_status_and_type(int *status, int *disc_type);
static int prepare_destination_paths(char *dst_folder, char *dst_file, char *text, int disc_type);
static int cleanup_failed_rip(char *dst_folder, char *dst_file, int disc_type);
static int process_sessions_and_tracks(char *dst_folder, char *dst_file, int disc_type);

typedef struct {
    int track_num;
    int start_lba;
    int sector_count;
    int type;
    char filename[NAME_MAX];
} track_info_t;

static int get_track_info(int session, int disc_type, track_info_t *tracks, int *track_count);
static int calculate_total_sectors(int disc_type);
static void update_ui_display(double progress_percent);

static struct self 
{
	App_t *app;
	GUI_Widget *bad;
	GUI_Widget *sd_c;
	GUI_Widget *hdd_c;
	GUI_Widget *net_c;
	GUI_Widget *gname;
	GUI_Widget *pbar;
	GUI_Widget *track_label;
	GUI_Widget *read_error;
	GUI_Widget *num_read;
	GUI_Widget *start_btn;
	GUI_Widget *cancel_btn;
	GUI_Widget *speed_label;
	GUI_Widget *time_label;
	GUI_Widget *sectors_label;
	int lastgdtrack;
	volatile int rip_active;
	uint64_t start_time;
	uint64_t total_sectors;
	uint64_t processed_sectors;
	uint64_t last_ui_update;
} self;

static void cancel_callback(void)
{
	self.rip_active = 0;
}

static void reset_rip_state(void)
{
	self.rip_active = 0;
	GUI_WidgetSetEnabled(self.start_btn, 1);
	GUI_WidgetSetEnabled(self.cancel_btn, 0);
	GUI_LabelSetText(self.speed_label, " ");
	GUI_LabelSetText(self.time_label, " ");
	GUI_LabelSetText(self.track_label, " ");
	GUI_LabelSetText(self.sectors_label, " ");
	GUI_ProgressBarSetPosition(self.pbar, 0.0);
	self.start_time = 0;
	self.total_sectors = 0;
	self.processed_sectors = 0;
	self.last_ui_update = 0;
}

void gd_ripper_toggleSavedevice(GUI_Widget *widget)
{
    GUI_WidgetSetState(self.sd_c, 0);
    GUI_WidgetSetState(self.hdd_c, 0);
    GUI_WidgetSetState(self.net_c, 0);
    GUI_WidgetSetState(widget, 1);
}

void gd_ripper_Number_read()
{
	char name[4];
	
	if (atoi(GUI_TextEntryGetText(self.num_read)) > 50) 
	{
		GUI_TextEntrySetText(self.num_read, "50");
	}
	else 
	{
		snprintf(name, 4, "%d", atoi(GUI_TextEntryGetText(self.num_read)));
		GUI_TextEntrySetText(self.num_read, name);
	}
}

void gd_ripper_Gamename()
{
	int x ;
	char text[NAME_MAX];
	char txt[NAME_MAX];
	strcpy(txt,"\0");
	snprintf(txt,NAME_MAX,"%s", GUI_TextEntryGetText(self.gname));
	
	if(strlen(txt) == 0) 
		strcpy(txt,"Game");
		
	snprintf(text,NAME_MAX,"%s",txt);
	
	for (x=0;text[x] !=0;x++) 
	{
		if (text[x] == ' ') text[x] = '_' ;
	}
	GUI_TextEntrySetText(self.gname, text);
}

void gd_ripper_Delname(GUI_Widget *widget)
{
	GUI_TextEntrySetText(widget, "");
}

void gd_ripper_ipbin_name()
{
	CDROM_TOC toc ;
	int status = 0, disc_type = 0, cdcr = 0, x = 0;
	uint8 *pbuff;
	char text[NAME_MAX];
	uint32 lba;

	cdrom_set_sector_size(2048);
	if((cdcr = cdrom_get_status(&status, &disc_type)) != ERR_OK) 
	{
		switch (cdcr)
		{
			case ERR_NO_DISC :
				ds_printf("DS_ERROR: Disk not inserted\n");
				return;
			default:
				ds_printf("DS_ERROR: GD-rom error\n");
				return;
		}
	}

	pbuff = (uint8 *)memalign(32, 2048);

	if(!pbuff)
	{
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return;
	}
	
	if (disc_type == CD_CDROM_XA) 
	{
		if(cdrom_read_toc(&toc, 0) != ERR_OK) 
		{ 
			ds_printf("DS_ERROR: Toc read error\n"); 
			free(pbuff);
			return; 
		}
		if(!(lba = cdrom_locate_data_track(&toc))) 
		{
			ds_printf("DS_ERROR: Error locate data track\n"); 
			free(pbuff);
			return;
		}
		if (cdrom_read_sectors_ex(pbuff, lba, 1, CDROM_READ_DMA)) 
		{
			ds_printf("DS_ERROR: CD read error %d\n",lba); 
			free(pbuff);
			return;
		}
	} 
	else if (disc_type == CD_GDROM) 
	{
		if (cdrom_read_sectors_ex(pbuff, 45150, 1, CDROM_READ_DMA)) 
		{
			ds_printf("DS_ERROR: GD read error\n"); 
			free(pbuff);
			return;
		}
	}
	else 
	{
		ds_printf("DS_ERROR: not game disc\nInserted %d disk\n",disc_type);
		free(pbuff);
		return;
	}
	
	ipbin_meta_t *meta = (ipbin_meta_t*) pbuff;
	
	char *p;
	char *o;
	
	p = meta->title;
	o = text;

	// skip any spaces at the beginning
	while(*p == ' ' && meta->title + 29 > p) 
		p++;

	// copy rest to output buffer
	while(meta->title + 29 > p) 
	{ 
		*o = *p;
		p++; 
		o++; 
	}

	// make sure output buf is null terminated
	*o = '\0';
	o--;

	// remove trailing spaces
	while(*o == ' ' && o > text) 
	{
		*o='\0';
		o--;
	}
	
	if (strlen(text) == 0) 
	{
		GUI_TextEntrySetText(self.gname, "Game");	
	} 
	else 
	{
		for (x=0;text[x] !=0;x++) 
		{
			if (text[x] == ' ') text[x] = '_' ;
		}
		GUI_TextEntrySetText(self.gname, text);
	}
	//ds_printf("Image name - %s\n",text);
	free(pbuff);
}

void gd_ripper_Init(App_t *app, const char* fileName) 
{
	
	if(app != NULL) 
	{
		memset(&self, 0, sizeof(self));
		
		self.app = app;
		self.bad = APP_GET_WIDGET("bad_btn");
		self.gname = APP_GET_WIDGET("gname-text");
		self.pbar = APP_GET_WIDGET("progress_bar");
		self.sd_c = APP_GET_WIDGET("sd_c");
		self.hdd_c = APP_GET_WIDGET("hdd_c");
		self.net_c = APP_GET_WIDGET("net_c");
		self.track_label = APP_GET_WIDGET("track-label");
		self.read_error = APP_GET_WIDGET("read_error");
		self.num_read = APP_GET_WIDGET("num-read");
		self.start_btn = APP_GET_WIDGET("start_btn");
		self.cancel_btn = APP_GET_WIDGET("cancel_btn");
		self.speed_label = APP_GET_WIDGET("speed-label");
		self.time_label = APP_GET_WIDGET("time-label");
		self.sectors_label = APP_GET_WIDGET("sectors-label");
		
		if(!DirExists("/pc")) GUI_WidgetSetEnabled(self.net_c, 0);
		else GUI_WidgetSetState(self.net_c, 1);
		
		if(!DirExists("/sd")) 
		{
			GUI_WidgetSetEnabled(self.sd_c, 0);
		}
		else 
		{
			GUI_WidgetSetState(self.net_c, 0);
			GUI_WidgetSetState(self.sd_c, 1);
		}
		
		if(!DirExists("/ide")) 
		{
			GUI_WidgetSetEnabled(self.hdd_c, 0);
		}
		else 
		{
			GUI_WidgetSetState(self.sd_c, 0);
			GUI_WidgetSetState(self.net_c, 0);
			GUI_WidgetSetState(self.hdd_c, 1);
		}
		
		GUI_WidgetSetEnabled(self.cancel_btn, 0);
		
		gd_ripper_ipbin_name();
		
		cont_btn_callback(0, CONT_X, (cont_btn_callback_t)cancel_callback);
	} 
	else 
	{
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}


void gd_ripper_StartRip() 
{
	if(self.app->thd)
	{
		self.rip_active = 0;
		thd_join(self.app->thd, NULL);
		self.app->thd = NULL;
	}

	self.rip_active = 1;

	GUI_WidgetSetEnabled(self.start_btn, 0);
	GUI_WidgetSetEnabled(self.cancel_btn, 1);
	
	GUI_LabelSetText(self.track_label, "Starting...");
	GUI_LabelSetText(self.speed_label, "Preparing...");
	GUI_LabelSetText(self.time_label, "Please wait");

	self.app->thd = thd_create(0, gd_ripper_thread, NULL);
}

void gd_ripper_CancelRip()
{
	ds_printf("DS_PROCESS: Cancelling ripping\n");
	self.rip_active = 0;

	if(self.app->thd)
	{
		thd_join(self.app->thd, NULL);
		self.app->thd = NULL;
		ds_printf("DS_INFO: Ripping cancelled\n");
	}
	
	cdrom_spin_down();
	reset_rip_state();
}

static int get_disc_status_and_type(int *status, int *disc_type)
{
	int cdcr;
	
getstatus:	
	if((cdcr = cdrom_get_status(status, disc_type)) != ERR_OK) 
	{
		switch (cdcr)
		{
			case ERR_NO_DISC :
				ds_printf("DS_ERROR: Disk not inserted\n");
				return CMD_ERROR;
			case ERR_DISC_CHG :
				cdrom_reinit();
				goto getstatus;
			case CD_STATUS_BUSY :
				thd_sleep(200);
				goto getstatus;
			case CD_STATUS_SEEKING :
				thd_sleep(200);
				goto getstatus;
			case CD_STATUS_SCANNING :
				thd_sleep(200);
				goto getstatus;
			default:
				ds_printf("DS_ERROR: GD-rom error: %d\n", cdcr);
				return CMD_ERROR;
		}
	}
	
	return CMD_OK;
}

static int prepare_destination_paths(char *dst_folder, char *dst_file, char *text, int disc_type)
{
	file_t fd;
	CDROM_TOC toc;
	int terr = 0;

	if(disc_type == CD_CDROM_XA)
	{
		snprintf(dst_file, NAME_MAX, "%s.iso", dst_folder);
		if ((fd=fs_open(dst_file, O_RDONLY)) != FILEHND_INVALID) 
		{
			fs_close(fd); 
			if(fs_unlink(dst_file) != 0) 
			{
				ds_printf("DS_ERROR: Failed to remove old file: %s\n", dst_file);
				return CMD_ERROR;
			}
		}
		return 1;
	} 
	else if (disc_type == CD_GDROM)
	{
		if ((fd=fs_open(dst_folder, O_DIR)) != FILEHND_INVALID) 
		{
			fs_close(fd);
			if(cleanup_failed_rip(dst_folder, dst_file, 2) != CMD_OK)
			{
				ds_printf("DS_ERROR: Failed to remove old folder: %s\n", dst_folder);
				return CMD_ERROR;
			}
		}

		strcpy(dst_file, "\0");
		snprintf(dst_file, NAME_MAX, "%s", dst_folder); 
		if(fs_mkdir(dst_file) != 0) 
		{
			ds_printf("DS_ERROR: Failed to create folder: %s\n", dst_file);
			return CMD_ERROR;
		}

		while(cdrom_read_toc(&toc, 1) != ERR_OK) 
		{ 
			if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
				ds_printf("DS_INFO: Destination preparation cancelled\n");
				fs_rmdir(dst_folder);
				return CMD_ERROR;
			}
			
			terr++;
			if (terr == 5) {
				ds_printf("DS_INFO: Reinitializing CDROM for gdlast TOC read\n");
				cdrom_reinit();
			}
			
			if (terr > 20)
			{
				ds_printf("DS_ERROR: Toc read error for gdlast after %d attempts\n", terr);
				fs_rmdir(dst_folder);
				return CMD_ERROR;
			}
			
			thd_sleep(200);
		}

		self.lastgdtrack = TOC_TRACK(toc.last);
		return 2;
	} 
	else 
	{
		ds_printf("DS_ERROR: This is not game disk\nInserted %d disk\n", disc_type);
		return CMD_ERROR;
	}
}

static int cleanup_failed_rip(char *dst_folder, char *dst_file, int disc_type)
{
	file_t fd;
	dirent_t *dir;

	if (disc_type == 1) 
	{
		if(fs_unlink(dst_file) != 0) 
			ds_printf("Error delete file: %s\n", dst_file);
	}
	else 
	{
		if ((fd=fs_open(dst_folder, O_DIR)) == FILEHND_INVALID) 
		{
			ds_printf("Error folder '%s' not found\n", dst_folder);
			return CMD_ERROR;
		}

		while ((dir = fs_readdir(fd)))
		{
			if (!strcmp(dir->name,".") || !strcmp(dir->name,"..")) continue;
			strcpy(dst_file, "\0");
			snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, dir->name);
			fs_unlink(dst_file);
		}

		fs_close(fd);
		fs_rmdir(dst_folder);
	}

	return CMD_OK;
}

static int get_track_info(int session, int disc_type, track_info_t *tracks, int *track_count)
{
    CDROM_TOC toc;
    int terr = 0;

    while (cdrom_read_toc(&toc, session) != ERR_OK) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: Track info reading cancelled\n");
            return CMD_ERROR;
        }
        
        terr++;
        if (terr == 3) {
            ds_printf("DS_INFO: Reinitializing CDROM for TOC read\n");
            cdrom_reinit();
        }
        
        if (terr > 8) {
            ds_printf("DS_ERROR: Failed to read TOC after %d attempts\n", terr);
            return CMD_ERROR;
        }
        
        thd_sleep(200);
    }
    
    int first = TOC_TRACK(toc.first);
    int last = TOC_TRACK(toc.last);
    int count = 0;
    
    for (int tn = first; tn <= last; tn++) {
        if (disc_type == 1) tn = last;
        
        int type = TOC_CTRL(toc.entry[tn-1]);
        int start = TOC_LBA(toc.entry[tn-1]);
        int s_end = TOC_LBA((tn == last ? toc.leadout_sector : toc.entry[tn]));
        int nsec = s_end - start;
        
        if (disc_type == 1 && type == 4) nsec -= 2;
        else if (session == 1 && tn != last && type != TOC_CTRL(toc.entry[tn])) nsec -= 150;
        else if (session == 0 && type == 4) nsec -= 150;
        
        tracks[count].track_num = tn;
        tracks[count].start_lba = start;
        tracks[count].sector_count = nsec;
        tracks[count].type = type;
        
        if (disc_type == 2) {
            snprintf(tracks[count].filename, NAME_MAX, "track%02d.%s", 
                    tn, (type == 4 ? "iso" : "raw"));
        }
        
        count++;
        
        if (disc_type == 1) break;
    }
    
    *track_count = count;
    return CMD_OK;
}

static int calculate_total_sectors(int disc_type)
{
    track_info_t tracks[32];
    int track_count;
    uint64_t total = 0;
    
    ds_printf("DS_PROCESS: Calculating total sectors for disc type %d\n", disc_type);
    
    for (int session = 0; session < disc_type; session++) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: Total sectors calculation cancelled\n");
            return 0;
        }
        
        ds_printf("DS_PROCESS: Processing session %d\n", session);
        
        if (get_track_info(session, disc_type, tracks, &track_count) != CMD_OK) {
            ds_printf("DS_ERROR: Failed to get track info for session %d\n", session);
            return 0;
        }
        
        for (int i = 0; i < track_count; i++) {
            total += tracks[i].sector_count;
            ds_printf("DS_PROCESS: Track %d: %d sectors\n", tracks[i].track_num, tracks[i].sector_count);
        }
    }
    
    ds_printf("DS_PROCESS: Total sectors calculated: %u\n", (uint32_t)total);
    return total;
}

static int process_sessions_and_tracks(char *dst_folder, char *dst_file, int disc_type)
{
    track_info_t tracks[32];
    int track_count;
    char riplabel[64];
    
    for (int session = 0; session < disc_type; session++) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: Ripping cancelled by user\n");
            return CMD_ERROR;
        }
        
        if (get_track_info(session, disc_type, tracks, &track_count) != CMD_OK) {
            ds_printf("DS_ERROR: Failed to get track info\n");
            cleanup_failed_rip(dst_folder, dst_file, disc_type);
            return CMD_ERROR;
        }
        
        for (int i = 0; i < track_count; i++) {
            if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
                ds_printf("DS_INFO: Ripping cancelled by user\n");
                return CMD_ERROR;
            }
            
            int tn = tracks[i].track_num;
            
            if (disc_type == 2) {
                snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, tracks[i].filename);
            }
            
            if (disc_type == 2 && session == 0) {
                snprintf(riplabel, sizeof(riplabel), "Track %d of %d\n", tn, self.lastgdtrack);
            }
            else if (disc_type == 2 && session == 1 && tn != self.lastgdtrack) {
                snprintf(riplabel, sizeof(riplabel), "Track %d of %d\n", tn, self.lastgdtrack);
            }
            else {
                snprintf(riplabel, sizeof(riplabel), "Last track\n");
            }
            
            GUI_LabelSetText(self.track_label, riplabel);
            
            if (rip_sec(tn, tracks[i].start_lba, tracks[i].sector_count, 
                       tracks[i].type, dst_file, disc_type) != CMD_OK) {
                reset_rip_state();
                cdrom_spin_down();
                cleanup_failed_rip(dst_folder, dst_file, disc_type);
                return CMD_ERROR;
            }

            GUI_LabelSetText(self.track_label, " ");
            GUI_ProgressBarSetPosition(self.pbar, 0.0);
        }
    }

    return CMD_OK;
}

static void* gd_ripper_thread(void *arg)
{
	int status, disc_type;
	char dst_folder[NAME_MAX];
	char dst_file[NAME_MAX];
	char text[NAME_MAX];

	ds_printf("DS_PROCESS: Starting disc ripping process\n");
	self.start_time = timer_ms_gettime64();
	self.processed_sectors = 0;

	GUI_LabelSetText(self.track_label, "Initializing...");
	cdrom_reinit();

	snprintf(text, NAME_MAX, "%s", GUI_TextEntryGetText(self.gname));

	if(GUI_WidgetGetState(self.sd_c)) 
	{
		snprintf(dst_folder, NAME_MAX, "/sd/%s", text);
	}
	else if(GUI_WidgetGetState(self.hdd_c)) 
	{
		snprintf(dst_folder, NAME_MAX, "/ide/%s", text);
	}
	else if(GUI_WidgetGetState(self.net_c)) 
	{
		snprintf(dst_folder, NAME_MAX, "/net/%s", text);
	}

	if(get_disc_status_and_type(&status, &disc_type) != CMD_OK)
	{
		reset_rip_state();
		return NULL;
	}

	GUI_LabelSetText(self.track_label, "Preparing...");
	ds_printf("DS_PROCESS: Preparing destination: %s\n", dst_folder);
	disc_type = prepare_destination_paths(dst_folder, dst_file, text, disc_type);
	if(disc_type < 0)
	{
		ds_printf("DS_ERROR: Failed to prepare destination paths\n");
		reset_rip_state();
		return NULL;
	}

	GUI_LabelSetText(self.track_label, "Calculating...");
	self.total_sectors = calculate_total_sectors(disc_type);
	ds_printf("DS_PROCESS: Total sectors to rip: %u\n", (uint32_t)self.total_sectors);

	GUI_LabelSetText(self.track_label, "Starting...");
	ds_printf("DS_PROCESS: Starting track extraction\n");
	if(process_sessions_and_tracks(dst_folder, dst_file, disc_type) != CMD_OK)
	{
		reset_rip_state();
		return NULL;
	}

	GUI_LabelSetText(self.track_label, " ");
	GUI_WidgetMarkChanged(self.app->body);
	
	if (disc_type == 2)
	{
		ds_printf("DS_PROCESS: Creating GDI files\n");
		if (gdfiles(dst_folder, dst_file, text) != CMD_OK)
		{
			cdrom_spin_down();
			cleanup_failed_rip(dst_folder, dst_file, disc_type);
			reset_rip_state();
			return NULL;	
		}
	}

	reset_rip_state();
	cdrom_spin_down();
	return NULL;
}

static void update_ui_display(double progress_percent) {
    uint64_t current_time = timer_ms_gettime64();
    uint64_t elapsed_time = current_time - self.start_time;

    GUI_ProgressBarSetPosition(self.pbar, progress_percent);

    if (current_time - self.last_ui_update < UI_UPDATE_INTERVAL) {
		return;
	}

	if (elapsed_time < UI_UPDATE_INTERVAL || !self.processed_sectors) {
		return;
	}

	char speed_text[64];
	char time_text[64];
	char sectors_text[64];

	double speed_sectors_per_sec = (double)self.processed_sectors / (elapsed_time / 1000.0);
	double speed_kb_per_sec = speed_sectors_per_sec * 2048 / 1024.0;

	snprintf(speed_text, sizeof(speed_text), "Speed: %.1f KB/s", speed_kb_per_sec);

	if (self.total_sectors > 0) {
		snprintf(sectors_text, sizeof(sectors_text), "%llu / %llu sectors", 
				self.processed_sectors, self.total_sectors);
	}
	else {
		snprintf(sectors_text, sizeof(sectors_text), "%llu sectors", 
				self.processed_sectors);
	}

	if (self.total_sectors > 0 && speed_sectors_per_sec > 0) {
		uint64_t remaining_sectors = self.total_sectors - self.processed_sectors;
		uint64_t remaining_time_sec = remaining_sectors / speed_sectors_per_sec;
		uint32_t remaining_hours = (uint32_t)(remaining_time_sec / 3600);
		uint32_t remaining_min = (uint32_t)((remaining_time_sec % 3600) / 60);

		if (remaining_hours > 0) {
			snprintf(time_text, sizeof(time_text), "Time left: %luh %lum",
					remaining_hours, remaining_min);
		}
		else {
			snprintf(time_text, sizeof(time_text), "Time left: %lum", remaining_min);
		}
	}
	else {
		snprintf(time_text, sizeof(time_text), "Time left: --");
	}

	GUI_LabelSetText(self.speed_label, speed_text);
	GUI_LabelSetText(self.time_label, time_text);
	GUI_LabelSetText(self.sectors_label, sectors_text);

	self.last_ui_update = current_time;
}

static int rip_sec(int tn,int first,int count,int type,char *dst_file, int disc_type)
{
	double percent;
	file_t hnd;
	int secbyte = (type == 4 ? 2048 : 2352) , count_old=count, bad=0, cdstat, readi;
	int secmode = type == 4 ? CDROM_READ_DMA : CDROM_READ_PIO;
	uint8 *buffer = (uint8 *)memalign(32, SEC_BUF_SIZE * secbyte);

	if(!buffer)
	{
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return CMD_ERROR;
	}

	cdrom_set_sector_size(secbyte);

	if ((hnd = fs_open(dst_file,O_WRONLY | O_TRUNC | O_CREAT)) == FILEHND_INVALID)
	{
		ds_printf("Error open file %s\n" ,dst_file); 
		cdrom_spin_down(); 
		free(buffer); 
		return CMD_ERROR;
	}
	while(count)
	{
		if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) 
		{
			ds_printf("DS_INFO: Sector ripping cancelled at sector %d\n", first);
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}

		int nsects = count > SEC_BUF_SIZE ? SEC_BUF_SIZE : count;
		count -= nsects;

		cdstat = cdrom_read_sectors_ex(buffer, first, nsects, secmode);

		while(cdstat != ERR_OK) 
		{
			if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) 
			{
				ds_printf("DS_INFO: Sector reading cancelled\n");
				free(buffer);
				fs_close(hnd);
				return CMD_ERROR;
			}

			if (atoi(GUI_TextEntryGetText(self.num_read)) == 0) break;
			readi++;

			if (readi > 5) break;

			if (readi == 3) {
				ds_printf("DS_INFO: Reinitializing CDROM for sector read\n");
				cdrom_reinit();
				cdrom_set_sector_size(secbyte);
			}

			thd_sleep(100);
			cdstat = cdrom_read_sectors_ex(buffer, first, nsects, secmode);
		}

		readi = 0;

		if (cdstat != ERR_OK) 
		{
			memset(buffer, 0, nsects * secbyte);
			bad++;
			ds_printf("DS_ERROR: Can't read sector %ld\n", first);
		}

		first += nsects;

		if(fs_write(hnd, buffer, nsects * secbyte) < 0) 
		{
			ds_printf("DS_ERROR: Write error to file\n");
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}
		
		self.processed_sectors += nsects;
		
		percent = 1-(float)(count) / count_old;
		update_ui_display(percent);
		
		if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) 
		{
			ds_printf("DS_INFO: Ripping cancelled during progress update\n");
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}
	}

	free(buffer);
	fs_close(hnd);
	return CMD_OK;
}

int gdfiles(char *dst_folder,char *dst_file,char *text)
{
	file_t gdfd;
	FILE *gdifd;

	CDROM_TOC gdtoc;
	CDROM_TOC cdtoc;
	int track ,lba ,gdtype ,cdtype;
	uint8 *buff = memalign(32, 32768);

	if(!buff)
	{
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return CMD_ERROR;
	}

	cdrom_set_sector_size(2048);
	if(cdrom_read_toc(&cdtoc, 0) != ERR_OK) 
	{ 
		ds_printf("DS_ERROR:CD Toc read error\n"); 
		free(buff); 
		return CMD_ERROR; 
	}

	if(cdrom_read_toc(&gdtoc, 1) != ERR_OK) 
	{ 
		ds_printf("DS_ERROR:GD Toc read error\n"); 
		free(buff); 
		return CMD_ERROR; 
	}

	if(cdrom_read_sectors(buff,45150,16)) 
	{ 
		ds_printf("DS_ERROR: IP.BIN read error\n"); 
		free(buff); 
		return CMD_ERROR; 
	}

	strcpy(dst_file,"\0"); 
	snprintf(dst_file,NAME_MAX,"%s/IP.BIN",dst_folder); 

	if ((gdfd=fs_open(dst_file,O_WRONLY | O_TRUNC | O_CREAT)) == FILEHND_INVALID) 
	{ 
		ds_printf("DS_ERROR: Error open IP.BIN for write\n"); 
		free(buff); 
		return CMD_ERROR; 
	}

	if (fs_write(gdfd,buff,32768) == -1) 
	{
		ds_printf("DS_ERROR: Error write IP.BIN\n"); 
		free(buff); 
		fs_close(gdfd); 
		return CMD_ERROR;
	}

	fs_close(gdfd); 
	free(buff);
	ds_printf("IP.BIN succes dumped\n");

	fs_chdir(dst_folder);
	strcpy(dst_file,"\0"); 
	snprintf(dst_file,NAME_MAX,"%s.gdi",text);

	if ((gdifd=fopen(dst_file,"w")) == NULL)
	{
		ds_printf("DS_ERROR: Error open %s.gdi for write\n",text); 
		return CMD_ERROR; 
	}

	int cdfirst = TOC_TRACK(cdtoc.first);
	int cdlast = TOC_TRACK(cdtoc.last);
	int gdfirst = TOC_TRACK(gdtoc.first);
	int gdlast = TOC_TRACK(gdtoc.last);
	fprintf(gdifd,"%d\n",gdlast);

	for (track=cdfirst; track <= cdlast; track++ ) 
	{
		lba = TOC_LBA(cdtoc.entry[track-1]);
		lba -= 150;
		cdtype = TOC_CTRL(cdtoc.entry[track-1]);
		fprintf(gdifd, "%d %d %d %d track%02d.%s 0\n",track,lba,cdtype,(cdtype == 4 ? 2048 : 2352),track,(cdtype == 4 ? "iso" : "raw"));
	}

	for (track=gdfirst; track <= gdlast; track++ ) 
	{
		lba = TOC_LBA(gdtoc.entry[track-1]);
		lba -= 150;
		gdtype = TOC_CTRL(gdtoc.entry[track-1]);
		fprintf(gdifd, "%d %d %d %d track%02d.%s 0\n",track,lba,gdtype,(gdtype == 4 ? 2048 : 2352),track,(gdtype == 4 ? "iso" : "raw"));
	}

	fclose(gdifd); 
	fs_chdir("/");
	ds_printf("%s.gdi succes writen\n",text);

	return CMD_OK;	
}

void gd_ripper_Exit() 
{
	if(self.rip_active)
	{
		self.rip_active = 0;
		if(self.app->thd)
		{
			thd_join(self.app->thd, NULL);
			self.app->thd = NULL;
		}
	}

	cdrom_spin_down();
}

