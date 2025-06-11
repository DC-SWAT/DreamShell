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

static int rip_sec(int tn ,int first, int count, int type, char *dst_file);
static void* gd_ripper_thread(void *arg);
static int gdfiles(char *dst_folder, char *dst_file, char *text);
static int get_disc_status_and_type(int *status, int *disc_type);
static int safe_cdrom_read_toc(CDROM_TOC *toc, int session);
static int get_area_count(int disc_type);
static int prepare_destination_paths(char *dst_folder, char *dst_file, char *text, int disc_type);
static int cleanup_failed_rip(char *dst_folder, char *dst_file, int disc_type);
static int process_areas_and_tracks(char *dst_folder, char *dst_file, int disc_type, int area_count);

typedef struct {
    uint32_t track_num;
    uint32_t start_lba;
    uint32_t sector_count;
    uint32_t type;
    char filename[NAME_MAX];
} track_info_t;

static int get_track_info(int area, int disc_type, track_info_t *tracks, uint32_t *track_count);
static int calculate_total_sectors(int disc_type, int area_count);
static void update_ui_display(double progress_percent);

static struct self {
	App_t *app;
	GUI_Widget *bad;
	GUI_Widget *gname;
	GUI_Widget *pbar;
	GUI_Widget *track_label;
	GUI_Widget *num_read;
	GUI_Widget *start_btn;
	GUI_Widget *cancel_btn;
	GUI_Widget *speed_label;
	GUI_Widget *time_label;
	GUI_Widget *sectors_total_label;
	GUI_Widget *sectors_processed_label;
	GUI_Widget *destination_path;
	GUI_Widget *file_browser;
	GUI_Widget *pages;
	uint32_t last_track;
	volatile int rip_active;
	uint64_t start_time;
	uint64_t total_sectors;
	uint64_t processed_sectors;
	uint64_t last_ui_update;
	track_info_t tracks[99];
	char selected_path[NAME_MAX];
} self;

static void reset_rip_state(void) {
	self.rip_active = 0;
	GUI_WidgetSetEnabled(self.start_btn, 1);
	GUI_WidgetSetEnabled(self.cancel_btn, 0);
	GUI_LabelSetText(self.speed_label, " ");
	GUI_LabelSetText(self.time_label, " ");
	GUI_LabelSetText(self.track_label, "GD Ripper");
	GUI_LabelSetText(self.sectors_total_label, " ");
	GUI_LabelSetText(self.sectors_processed_label, " ");
	GUI_ProgressBarSetPosition(self.pbar, 0.0);
	self.start_time = 0;
	self.total_sectors = 0;
	self.processed_sectors = 0;
	self.last_ui_update = 0;
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
    char text[NAME_MAX];
    const char *input = GUI_TextEntryGetText(self.gname);

    if (!input || strlen(input) == 0) {
        strcpy(text, "RIPPED_GAME_DISC");
    }
    else {
        strncpy(text, input, NAME_MAX - 1);
        text[NAME_MAX - 1] = '\0';
    }

    for (char *p = text; *p; p++) {
        if (*p == ' ') {
            *p = '_';
        }
    }

    GUI_TextEntrySetText(self.gname, text);
}

void gd_ripper_ipbin_name()
{
	CDROM_TOC toc ;
	int status = 0, disc_type = 0, cdcr = 0, x = 0;
	uint8_t *pbuff;
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

	pbuff = (uint8_t *)memalign(32, 2048);

	if(!pbuff)
	{
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return;
	}
	
	if (disc_type == CD_CDROM_XA) 
	{
		if(safe_cdrom_read_toc(&toc, 0) != CMD_OK) 
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
		self.track_label = APP_GET_WIDGET("track-label");
		self.num_read = APP_GET_WIDGET("num-read");
		self.start_btn = APP_GET_WIDGET("start_btn");
		self.cancel_btn = APP_GET_WIDGET("cancel_btn");
		self.speed_label = APP_GET_WIDGET("speed-label");
		self.time_label = APP_GET_WIDGET("time-label");
		self.sectors_total_label = APP_GET_WIDGET("sectors-total-label");
		self.sectors_processed_label = APP_GET_WIDGET("sectors-processed-label");
		self.destination_path = APP_GET_WIDGET("destination-path");
		self.file_browser = APP_GET_WIDGET("file-browser");
		self.pages = APP_GET_WIDGET("pages");

		if(DirExists("/ide")) {
			strcpy(self.selected_path, "/ide");
		}
		else if(DirExists("/sd")) {
			strcpy(self.selected_path, "/sd");
		}
		else if(DirExists("/pc")) {
			strcpy(self.selected_path, "/pc");
		}
		else {
			strcpy(self.selected_path, "/ram");
		}

		GUI_LabelSetText(self.destination_path, self.selected_path);
		GUI_WidgetSetEnabled(self.cancel_btn, 0);
		gd_ripper_ipbin_name();
	} 
	else 
	{
		ds_printf("DS_ERROR: %s: Attempting to call %s is not by the app initiate.\n", 
					lib_get_name(), __func__);
	}
}


void gd_ripper_StartRip(GUI_Widget *widget) 
{
	(void)widget;
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

void gd_ripper_CancelRip(GUI_Widget *widget)
{
	(void)widget;
	ds_printf("DS_PROCESS: Cancelling ripping\n");
	self.rip_active = 0;

	if(self.app->thd)
	{
		thd_join(self.app->thd, NULL);
		self.app->thd = NULL;
		ds_printf("DS_INFO: Ripping cancelled\n");
	}

	reset_rip_state();
	cdrom_spin_down();
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

static int safe_cdrom_read_toc(CDROM_TOC *toc, int session)
{
    int terr = 0;

    while (cdrom_read_toc(toc, session) != ERR_OK) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: TOC reading cancelled\n");
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
    
    return CMD_OK;
}

static int get_area_count(int disc_type)
{
	if (disc_type == CD_CDROM_XA)
		return 1;
	else if (disc_type == CD_GDROM)
		return 2;
	else
		return 0;
}

static int prepare_destination_paths(char *dst_folder, char *dst_file, char *text, int disc_type) {
	file_t fd;

	if (disc_type == CD_CDROM_XA) {
		snprintf(dst_file, NAME_MAX, "%s.iso", dst_folder);
		fd = fs_open(dst_file, O_RDONLY);

		if (fd != FILEHND_INVALID) {
			fs_close(fd);
			if (fs_unlink(dst_file) != 0) {
				ds_printf("DS_ERROR: Failed to remove old file: %s\n", dst_file);
				return CMD_ERROR;
			}
		}
		return CMD_OK;
	}

	if (disc_type == CD_GDROM) {
		fd = fs_open(dst_folder, O_DIR);
		if (fd != FILEHND_INVALID) {
			fs_close(fd);
			if (cleanup_failed_rip(dst_folder, dst_file, disc_type) != CMD_OK) {
				ds_printf("DS_ERROR: Failed to remove old folder: %s\n", dst_folder);
				return CMD_ERROR;
			}
		}

		memset(dst_file, 0, NAME_MAX);
		snprintf(dst_file, NAME_MAX, "%s", dst_folder);

		if (fs_mkdir(dst_file) != 0) {
			ds_printf("DS_ERROR: Failed to create folder: %s\n", dst_file);
			return CMD_ERROR;
		}
		return CMD_OK;
	}

	ds_printf("DS_ERROR: This is not game disk\nInserted %d disk\n", disc_type);
	return CMD_ERROR;
}

static int cleanup_failed_rip(char *dst_folder, char *dst_file, int disc_type) {
	if (disc_type == CD_CDROM_XA) {
		if (fs_unlink(dst_file) != 0) {
			ds_printf("DS_ERROR: Failed to delete file: %s\n", dst_file);
		}
		return CMD_OK;
	}

	file_t fd = fs_open(dst_folder, O_DIR);
	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Folder '%s' not found\n", dst_folder);
		return CMD_ERROR;
	}

	dirent_t *dir;
	while ((dir = fs_readdir(fd))) {
		if (!strcmp(dir->name, ".") || !strcmp(dir->name, "..")) {
			continue;
		}

		snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, dir->name);
		if (fs_unlink(dst_file) != 0) {
			ds_printf("DS_ERROR: Failed to delete file: %s\n", dst_file);
		}
	}

	fs_close(fd);
	fs_rmdir(dst_folder);
	return CMD_OK;
}

static int get_track_info(int area, int disc_type, track_info_t *tracks, uint32_t *track_count) {
    CDROM_TOC toc;

    if (safe_cdrom_read_toc(&toc, area) != CMD_OK) {
        return CMD_ERROR;
    }

    uint32_t first = TOC_TRACK(toc.first);
    uint32_t last = TOC_TRACK(toc.last);
    uint32_t count = 0;

    for (uint32_t tn = first; tn <= last; tn++) {
        if (disc_type == CD_CDROM_XA) tn = last;

        uint32_t type = TOC_CTRL(toc.entry[tn-1]);
        uint32_t start = TOC_LBA(toc.entry[tn-1]);
        uint32_t s_end = TOC_LBA((tn == last ? toc.leadout_sector : toc.entry[tn]));
        uint32_t nsec = s_end - start;

        if (disc_type == CD_CDROM_XA && type == 4) nsec -= 2;
        else if (area == 1 && tn != last && type != TOC_CTRL(toc.entry[tn])) nsec -= 150;
        else if (area == 0 && type == 4) nsec -= 150;

        tracks[count].track_num = tn;
        tracks[count].start_lba = start;
        tracks[count].sector_count = nsec;
        tracks[count].type = type;

        if (disc_type == CD_GDROM) {
            snprintf(tracks[count].filename, NAME_MAX, "track%02ld.%s", 
                    tn, (type == 4 ? "iso" : "raw"));
        }

        count++;

        if (disc_type == CD_CDROM_XA) break;
    }

    *track_count = count;
    return CMD_OK;
}

static int calculate_total_sectors(int disc_type, int area_count) {
    uint32_t track_count;
    uint64_t total = 0;

    ds_printf("DS_PROCESS: Calculating total sectors for disc type %d\n", area_count);

    for (int area = 0; area < area_count; area++) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            ds_printf("DS_INFO: Total sectors calculation cancelled\n");
            return 0;
        }

        ds_printf("DS_PROCESS: Processing area %d\n", area);

        if (get_track_info(area, disc_type, self.tracks, &track_count) != CMD_OK) {
            ds_printf("DS_ERROR: Failed to get track info for area %d\n", area);
            return 0;
        }

        for (int i = 0; i < track_count; i++) {
            total += self.tracks[i].sector_count;
            ds_printf("DS_PROCESS: Track %d: %d sectors\n", self.tracks[i].track_num, self.tracks[i].sector_count);
        }
    }

    ds_printf("DS_PROCESS: Total sectors calculated: %u\n", (uint32_t)total);
    return total;
}

static int process_areas_and_tracks(char *dst_folder, char *dst_file, int disc_type, int area_count) {
    uint32_t track_count;
    char riplabel[64];

    for (int area = 0; area < area_count; area++) {
        if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
            return CMD_ERROR;
        }

        if (get_track_info(area, disc_type, self.tracks, &track_count) != CMD_OK) {
            ds_printf("DS_ERROR: Failed to get track info\n");
            return CMD_ERROR;
        }

        for (int i = 0; i < track_count; i++) {
            if (!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
                return CMD_ERROR;
            }

            int tn = self.tracks[i].track_num;

            if (disc_type == CD_GDROM) {
                snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, self.tracks[i].filename);
            }

            snprintf(riplabel, sizeof(riplabel), "Track %d of %ld", tn, self.last_track);
            GUI_LabelSetText(self.track_label, riplabel);

            if (rip_sec(tn, self.tracks[i].start_lba, self.tracks[i].sector_count, 
                       self.tracks[i].type, dst_file) != CMD_OK) {
                return CMD_ERROR;
            }
        }
    }

    return CMD_OK;
}

static void* gd_ripper_thread(void *arg) {
	int status, disc_type;
	char dst_folder[NAME_MAX];
	char dst_file[NAME_MAX];
	char text[NAME_MAX];
	int area_count;

	ds_printf("DS_PROCESS: Starting disc ripping process\n");
	self.start_time = timer_ms_gettime64();
	self.processed_sectors = 0;

	GUI_LabelSetText(self.track_label, "Initializing...");
	cdrom_reinit();

	snprintf(text, NAME_MAX, "%s", GUI_TextEntryGetText(self.gname));
	snprintf(dst_folder, NAME_MAX, "%s/%s", self.selected_path, text);

	if(get_disc_status_and_type(&status, &disc_type) != CMD_OK) {
		goto out;
	}

	GUI_LabelSetText(self.track_label, "Preparing...");
	ds_printf("DS_PROCESS: Preparing destination: %s\n", dst_folder);

	if(prepare_destination_paths(dst_folder, dst_file, text, disc_type) != CMD_OK) {
		ds_printf("DS_ERROR: Failed to prepare destination paths\n");
		goto out;
	}

	area_count = get_area_count(disc_type);
	if(area_count == 0) {
		ds_printf("DS_ERROR: Unknown disc type\n");
		goto out;
	}

	CDROM_TOC toc;
	if (safe_cdrom_read_toc(&toc, disc_type == CD_GDROM ? 1 : 0) != CMD_OK) {
		goto out;
	}
	self.last_track = TOC_TRACK(toc.last);
	self.total_sectors = calculate_total_sectors(disc_type, area_count);

	if (disc_type == CD_GDROM) {
		ds_printf("DS_PROCESS: Creating GDI files\n");
		GUI_LabelSetText(self.track_label, "Creating GDI");

		if (gdfiles(dst_folder, dst_file, text) != CMD_OK) {
			goto out;
		}
	}

	ds_printf("DS_PROCESS: Starting track extraction\n");

	if(process_areas_and_tracks(dst_folder, dst_file, disc_type, area_count) != CMD_OK) {
		cleanup_failed_rip(dst_folder, dst_file, disc_type);
		goto out;
	}

out:
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
	char total_sectors_text[64];
	char processed_sectors_text[64];

	double speed_sectors_per_sec = (double)self.processed_sectors / (elapsed_time / 1000.0);
	double speed_kb_per_sec = speed_sectors_per_sec * 2048 / 1024.0;

	snprintf(speed_text, sizeof(speed_text), "Speed: %.1f KB/s", speed_kb_per_sec);

	if (self.total_sectors > 0) {
		snprintf(total_sectors_text, sizeof(total_sectors_text), "Total: %llu", self.total_sectors);
	}
	else {
		snprintf(total_sectors_text, sizeof(total_sectors_text), "Total: --");
	}
	
	snprintf(processed_sectors_text, sizeof(processed_sectors_text), "Done: %llu", self.processed_sectors);

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
	GUI_LabelSetText(self.sectors_total_label, total_sectors_text);
	GUI_LabelSetText(self.sectors_processed_label, processed_sectors_text);

	self.last_ui_update = current_time;
}

static int rip_sec(int tn,int first,int count,int type,char *dst_file) {
	double percent;
	file_t hnd;
	int secbyte = (type == 4 ? 2048 : 2352) , count_old=count, bad=0, cdstat, readi;
	int secmode = type == 4 ? CDROM_READ_DMA : CDROM_READ_PIO;
	uint8_t *buffer = (uint8_t *)memalign(32, SEC_BUF_SIZE * secbyte);

	if(!buffer) {
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return CMD_ERROR;
	}

	cdrom_set_sector_size(secbyte);
	hnd = fs_open(dst_file, O_WRONLY | O_TRUNC | O_CREAT);

	if (hnd == FILEHND_INVALID) {
		ds_printf("Error open file %s\n" ,dst_file); 
		cdrom_spin_down(); 
		free(buffer); 
		return CMD_ERROR;
	}

	while(count) {
		if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}

		int nsects = count > SEC_BUF_SIZE ? SEC_BUF_SIZE : count;
		count -= nsects;

		cdstat = cdrom_read_sectors_ex(buffer, first, nsects, secmode);

		while(cdstat != ERR_OK) {
			if(!self.rip_active || !(self.app->state & APP_STATE_OPENED)) {
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

		if (cdstat != ERR_OK) {
			memset(buffer, 0, nsects * secbyte);
			bad++;
			ds_printf("DS_ERROR: Can't read sector %ld\n", first);
		}

		first += nsects;

		if(fs_write(hnd, buffer, nsects * secbyte) < 0) {
			ds_printf("DS_ERROR: Write error to file\n");
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}

		self.processed_sectors += nsects;

		percent = 1-(float)(count) / count_old;
		update_ui_display(percent);
	}

	free(buffer);
	fs_close(hnd);
	return CMD_OK;
}

int gdfiles(char *dst_folder, char *dst_file, char *text) {
	file_t gdfd;
	FILE *gdifd;
	CDROM_TOC gdtoc, cdtoc;
	int track, lba, gdtype, cdtype;
	uint8_t *buff;

	cdrom_set_sector_size(2048);

	if (safe_cdrom_read_toc(&cdtoc, 0) != ERR_OK) {
		ds_printf("DS_ERROR: CD TOC read error\n");
		return CMD_ERROR;
	}

	if (safe_cdrom_read_toc(&gdtoc, 1) != ERR_OK) {
		ds_printf("DS_ERROR: GD TOC read error\n");
		return CMD_ERROR;
	}

	buff = memalign(32, 32768);

	if (!buff) {
		ds_printf("DS_ERROR: Failed to allocate buffer\n");
		return CMD_ERROR;
	}

	ds_printf("DS_PROCESS: Reading IP.BIN\n");
	int cdstat = cdrom_read_sectors_ex(buff, 45150, 16, CDROM_READ_DMA);

	if (cdstat != ERR_OK) {
		ds_printf("DS_ERROR: Failed to read IP.BIN\n");
		free(buff);
		return CMD_ERROR;
	}

	ds_printf("DS_OK: IP.BIN read successfully\n");

	snprintf(dst_file, NAME_MAX, "%s/IP.BIN", dst_folder);
	gdfd = fs_open(dst_file, O_WRONLY | O_TRUNC | O_CREAT);

	if (gdfd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Error open IP.BIN for write\n");
		free(buff);
		return CMD_ERROR;
	}

	if (fs_write(gdfd, buff, 32768) != 32768) {
		ds_printf("DS_ERROR: Error write IP.BIN\n");
		free(buff);
		fs_close(gdfd);
		return CMD_ERROR;
	}

	fs_close(gdfd);
	free(buff);
	ds_printf("DS_OK: IP.BIN dumped successfully\n");

	snprintf(dst_file, NAME_MAX, "%s/%s.gdi", dst_folder, text);

	gdifd = fopen(dst_file, "w");
	if (!gdifd) {
		ds_printf("DS_ERROR: Error open %s.gdi for write\n", text);
		return CMD_ERROR;
	}

	int cdfirst = TOC_TRACK(cdtoc.first);
	int cdlast = TOC_TRACK(cdtoc.last);
	int gdfirst = TOC_TRACK(gdtoc.first);
	int gdlast = TOC_TRACK(gdtoc.last);

	fprintf(gdifd, "%d\n", gdlast);

	for (track = cdfirst; track <= cdlast; track++) {
		lba = TOC_LBA(cdtoc.entry[track - 1]) - 150;
		cdtype = TOC_CTRL(cdtoc.entry[track - 1]);
		fprintf(gdifd, "%d %d %d %d track%02d.%s 0\n", 
			track, lba, cdtype, (cdtype == 4 ? 2048 : 2352), 
			track, (cdtype == 4 ? "iso" : "raw"));
	}

	for (track = gdfirst; track <= gdlast; track++) {
		lba = TOC_LBA(gdtoc.entry[track - 1]) - 150;
		gdtype = TOC_CTRL(gdtoc.entry[track - 1]);
		fprintf(gdifd, "%d %d %d %d track%02d.%s 0\n", 
			track, lba, gdtype, (gdtype == 4 ? 2048 : 2352), 
			track, (gdtype == 4 ? "iso" : "raw"));
	}

	fclose(gdifd);
	ds_printf("DS_OK: %s.gdi created successfully\n", text);

	return CMD_OK;
}

void gd_ripper_Exit()  {
	if(self.rip_active) {
		self.rip_active = 0;

		if(self.app->thd) {
			thd_join(self.app->thd, NULL);
			self.app->thd = NULL;
		}
	}
	cdrom_spin_down();
}

void gd_ripper_ShowFileBrowser(GUI_Widget *widget) {
	(void)widget;
	if (self.pages) {
		GUI_CardStackShowIndex(self.pages, 1);
	}
}

void gd_ripper_ShowMainPage(GUI_Widget *widget) {
	(void)widget;
	if (self.pages) {
		GUI_CardStackShowIndex(self.pages, 0);
	}
}

void gd_ripper_FileBrowserItemClick(dirent_fm_t *fm_ent) {
	if (!fm_ent) {
		return;
	}
	dirent_t *ent = &fm_ent->ent;
	GUI_FileManagerChangeDir(self.file_browser, ent->name, ent->size);
}

void gd_ripper_FileBrowserConfirm(GUI_Widget *widget) {
	(void)widget;

	if (self.file_browser && self.destination_path && self.pages) {
		const char *path = GUI_FileManagerGetPath(self.file_browser);
		if (path) {
			strncpy(self.selected_path, path, NAME_MAX - 1);
			self.selected_path[NAME_MAX - 1] = '\0';
			GUI_LabelSetText(self.destination_path, self.selected_path);
		}
		GUI_CardStackShowIndex(self.pages, 0);
	}
}
