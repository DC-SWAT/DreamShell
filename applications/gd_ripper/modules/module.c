/* DreamShell ##version##

   module.c - GD Ripper app module
   Copyright (C)2014 megavolt85 

*/

#include "ds.h"
#include "isofs/isofs.h"

DEFAULT_MODULE_EXPORTS(app_gd_ripper);

#define SEC_BUF_SIZE 8

static int rip_sec(int tn,int first,int count,int type,char *dst_file, int disc_type);
static int gdfiles(char *dst_folder,char *dst_file,char *text);

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
	int lastgdtrack;
} self;

static uint8_t rip_cancel = 0;

static void cancel_callback(void)
{
	rip_cancel = 1;
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
	//ds_printf("Image name - %s\n",text);
	
}

void gd_ripper_Delname(GUI_Widget *widget)
{
	GUI_TextEntrySetText(widget, "");
}

void gd_ripper_ipbin_name()
{
	CDROM_TOC toc ;
	int status = 0, disc_type = 0, cdcr = 0, x = 0;
	char pbuff[2048] , text[NAME_MAX];
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
	
	if (disc_type == CD_CDROM_XA) 
	{
		if(cdrom_read_toc(&toc, 0) != CMD_OK) 
		{ 
			ds_printf("DS_ERROR: Toc read error\n"); 
			return; 
		}
		if(!(lba = cdrom_locate_data_track(&toc))) 
		{
			ds_printf("DS_ERROR: Error locate data track\n"); 
			return;
		}
		if (cdrom_read_sectors(pbuff,lba , 1)) 
		{
			ds_printf("DS_ERROR: CD read error %d\n",lba); 
			return;
		}
	} 
	else if (disc_type == CD_GDROM) 
	{
		if (cdrom_read_sectors(pbuff,45150 , 1)) 
		{
			ds_printf("DS_ERROR: GD read error\n"); 
			return;
		}
	}
	else 
	{
		ds_printf("DS_ERROR: not game disc\n");
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
	file_t fd;
	CDROM_TOC toc ;
	int status, disc_type ,cdcr ,start ,s_end ,nsec ,type ,tn ,session, terr = 0;
	char dst_folder[NAME_MAX];
	char dst_file[NAME_MAX];
	char riplabel[64];
	char text[NAME_MAX];
	uint64 stoptimer, starttimer, riptime;
	
	rip_cancel = 0;
	
	starttimer = timer_ms_gettime64 (); // timer
	
	cdrom_reinit();
	
	snprintf(text ,NAME_MAX,"%s", GUI_TextEntryGetText(self.gname));
	
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
//ds_printf("Dst file1 %s\n" ,dst_folder);	
getstatus:	
	if((cdcr = cdrom_get_status(&status, &disc_type)) != ERR_OK) 
	{
		switch (cdcr)
		{
			case ERR_NO_DISC :
				ds_printf("DS_ERROR: Disk not inserted\n");
				return;
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
				ds_printf("DS_ERROR: GD-rom error\n");
				return;
		}
	}
	
	if(disc_type == CD_CDROM_XA)
	{
	rname:
		snprintf(dst_file,NAME_MAX, "%s.iso", dst_folder);
		if ((fd=fs_open(dst_file , O_RDONLY)) != FILEHND_INVALID) 
		{
			fs_close(fd); strcpy(dst_file,"\0"); 
			strcat(dst_folder , "0"); 
			goto rname ;
		} 
		else 
		{
			disc_type = 1;
		}
	} 
	else if (disc_type == CD_GDROM)
	{
	rname1:
		if ((fd=fs_open (dst_folder , O_DIR)) != FILEHND_INVALID) 
		{
			fs_close(fd); 
			strcat(dst_folder , "0"); 
			goto rname1 ;
		} 
		else 
		{
			strcpy(dst_file,"\0");
			snprintf(dst_file,NAME_MAX,"%s", dst_folder); 
			disc_type = 2; 
			fs_mkdir(dst_file);
		}
		
		while(cdrom_read_toc(&toc, 1) != CMD_OK) 
		{ 
			terr++;
			cdrom_reinit();
			if (terr > 100)
			{
				ds_printf("DS_ERROR: Toc read error for gdlast\n");
				fs_rmdir(dst_folder); 
				return; 
			}
		}
		
		terr=0;
		self.lastgdtrack = TOC_TRACK(toc.last);
	} 
	else 
	{
		ds_printf("DS_ERROR: This is not game disk\nInserted %d disk\n",disc_type);
		return;
	}
//ds_printf("Dst file %s\n" ,dst_file);
	
	for(session = 0; session < disc_type; session++) 
	{
		cdrom_set_sector_size(2048);
		while(cdrom_read_toc(&toc, session) != CMD_OK) 
		{
			terr++; 
			if(terr==5) cdrom_reinit(); 
			thd_sleep(500);
			
			if (terr > 10) 
			{ 
				ds_printf("DS_ERROR: Toc read error\n");
				if (disc_type == 1) 
				{
					if(fs_unlink(dst_file) != 0) 
					ds_printf("Error delete file: %s\n" ,dst_file);
				}
				else 
				{
					if ((fd=fs_open (dst_folder , O_DIR)) == FILEHND_INVALID) 
					{
						ds_printf("Error folder '%s' not found\n" ,dst_folder); 
						return;
					}
					
					dirent_t *dir;
					
					while ((dir = fs_readdir(fd)))
					{
						if (!strcmp(dir->name,".") || !strcmp(dir->name,"..")) continue;
						strcpy(dst_file,"\0");
						snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, dir->name);
						fs_unlink(dst_file);
					}
					
					fs_close(fd);
					fs_rmdir(dst_folder);
				}
				return; 
			}
		}
		
		terr = 0;
		
		int first = TOC_TRACK(toc.first);
		int last = TOC_TRACK(toc.last);
		
		for (tn = first; tn <= last; tn++ ) 
		{
			if (disc_type == 1) tn = last;
			
			type = TOC_CTRL(toc.entry[tn-1]);
			
			if (disc_type == 2) 
			{
				strcpy(dst_file,"\0"); 
				snprintf(dst_file,NAME_MAX,"%s/track%02d.%s", dst_folder, tn, (type == 4 ? "iso" : "raw"));
			}
			
			start = TOC_LBA(toc.entry[tn-1]);
			s_end = TOC_LBA((tn == last ? toc.leadout_sector : toc.entry[tn]));
			nsec = s_end - start;
			
			if(disc_type == 1 && type == 4) nsec -= 2 ;
			else if(session==1 && tn != last && type != TOC_CTRL(toc.entry[tn])) nsec -= 150;
			else if(session==0 && type == 4) nsec -= 150;
			
			if(disc_type == 2 && session == 0) 
			{
				strcpy(riplabel,"\0"); 
				sprintf(riplabel,"Track %d of %d\n",tn ,self.lastgdtrack );
			}
			else if(disc_type == 2 && session == 1 && tn != self.lastgdtrack) 
			{
				strcpy(riplabel,"\0"); 
				sprintf(riplabel,"Track %d of %d\n",tn ,self.lastgdtrack );
			} 
			else 
			{
				strcpy(riplabel,"\0"); 
				sprintf(riplabel,"Last track\n");
			}
			
			GUI_LabelSetText(self.track_label, riplabel);
			
			if (rip_sec(tn, start, nsec, type, dst_file, disc_type) != CMD_OK) 
			{
				GUI_LabelSetText(self.track_label, " ");
				stoptimer = timer_ms_gettime64 (); // timer
				GUI_ProgressBarSetPosition(self.pbar, 0.0);
				cdrom_spin_down();
				
				if (disc_type == 1) 
				{
					if(fs_unlink(dst_file) != 0) ds_printf("Error delete file: %s\n" ,dst_file);
				} 
				else 
				{
					if((fd=fs_open (dst_folder , O_DIR)) == FILEHND_INVALID) 
					{
						ds_printf("Error folder '%s' not found\n" ,dst_folder); 
						return;
					}
					
					dirent_t *dir;
					while ((dir = fs_readdir(fd)))
					{
						if (!strcmp(dir->name,".") || !strcmp(dir->name,"..")) continue;
						strcpy(dst_file,"\0");
						snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, dir->name);
						fs_unlink(dst_file);
					}
					fs_close(fd);
					fs_rmdir(dst_folder);
				}
				return ;
			}
			GUI_LabelSetText(self.track_label, " ");
			GUI_ProgressBarSetPosition(self.pbar, 0.0);
		}
	}
	
	GUI_LabelSetText(self.track_label, " ");
	GUI_WidgetMarkChanged(self.app->body);
	
	if (disc_type == 2)
	{
		if (gdfiles(dst_folder, dst_file, text) != CMD_OK)
		{
			cdrom_spin_down();
			
			if ((fd=fs_open (dst_folder , O_DIR)) == FILEHND_INVALID)
			{
				ds_printf("Error folder '%s' not found\n" ,dst_folder); 
				return;
			}
			
			dirent_t *dir;
			
			while ((dir = fs_readdir(fd)))
			{
				if(!strcmp(dir->name,".") || !strcmp(dir->name,"..")) continue;
				
				strcpy(dst_file,"\0");
				snprintf(dst_file, NAME_MAX, "%s/%s", dst_folder, dir->name);
				fs_unlink(dst_file);
			}
			fs_close(fd);
			fs_rmdir(dst_folder);
			return;	
		}
		
		
	}
	
	GUI_ProgressBarSetPosition(self.pbar, 0.0);
	cdrom_spin_down();
	stoptimer = timer_ms_gettime64 (); // timer
	riptime = stoptimer - starttimer;
	int ripmin = riptime / 60000 ;
	int ripsec = riptime % 60 ;
	ds_printf("DS_OK: End ripping. Save at %d:%d\n",ripmin,ripsec);
}


static int rip_sec(int tn,int first,int count,int type,char *dst_file, int disc_type)
{

	double percent,percent_last = 0.0;
	maple_device_t *cont;
	cont_state_t *state;
	file_t hnd;
	int secbyte = (type == 4 ? 2048 : 2352) , i , count_old=count, bad=0, cdstat, readi;
	uint8 *buffer = (uint8 *)memalign(32, SEC_BUF_SIZE * secbyte);
	
	GUI_WidgetMarkChanged(self.app->body);
//	ds_printf("Track %d		First %d	Count %d	Type %d\n",tn,first,count,type);
/*	if (secbyte == 2048) cdrom_set_sector_size (secbyte);
	else _cdrom_reinit (1);
*/
	cdrom_set_sector_size(secbyte);
	
	thd_sleep(200);
	
	if ((hnd = fs_open(dst_file,O_WRONLY | O_TRUNC | O_CREAT)) == FILEHND_INVALID)
	{
		ds_printf("Error open file %s\n" ,dst_file); 
		cdrom_spin_down(); 
		free(buffer); 
		return CMD_ERROR;
	}
	
	LockVideo();
	
	while(count)
	{
	
		if(rip_cancel == 1) 
		{
			SDL_WarpMouse(60, 400);
			return CMD_ERROR;
		}
		
		int nsects = count > SEC_BUF_SIZE ? SEC_BUF_SIZE : count;
		count -= nsects;
		
		
		while((cdstat=cdrom_read_sectors(buffer, first, nsects)) != ERR_OK ) 
		{
			if (atoi(GUI_TextEntryGetText(self.num_read)) == 0) break;
			readi++ ;
			if (readi > 5) break ;
			thd_sleep(200);
		}
		
		readi = 0;
		
		if (cdstat != ERR_OK) 
		{
			if (!GUI_WidgetGetState(self.bad)) 
			{
				UnlockVideo();
				GUI_ProgressBarSetPosition(self.read_error, 1.0);
				for(;;) 
				{
					cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
					
					if(!cont) continue;
					state = (cont_state_t *)maple_dev_status(cont);
					
					if(!state) continue;
					
					if(state->buttons & CONT_A) 
					{
						GUI_ProgressBarSetPosition(self.read_error, 0.0);
						GUI_WidgetMarkChanged(self.app->body); 
						ds_printf("DS_ERROR: Can't read sector %ld\n", first); 
						free(buffer); 
						fs_close(hnd); 
						return CMD_ERROR;
					} 
					else if(state->buttons & CONT_B) 
					{
						GUI_ProgressBarSetPosition(self.read_error, 0.0);
						GUI_WidgetMarkChanged(self.app->body); 
						break;
					} 
					else if(state->buttons & CONT_Y) 
					{
						GUI_ProgressBarSetPosition(self.read_error, 0.0); 
						GUI_WidgetSetState(self.bad, 1);
						GUI_WidgetMarkChanged(self.app->body); 
						break;
					}
				}
			}
			// Ошибка, попробуем по одному
			uint8 *pbuffer = buffer;
			LockVideo();
			for(i = 0; i < nsects; i++) 
			{
				
				while((cdstat=cdrom_read_sectors(pbuffer, first, 1)) != ERR_OK ) 
				{
					readi++ ;
					if (readi > atoi(GUI_TextEntryGetText(self.num_read))) break ;
					if (readi == 1 || readi == 6 || readi == 11 || readi == 16 || readi == 21 || readi == 26 || readi == 31 || readi == 36 || readi == 41 || readi == 46) cdrom_reinit();
					thd_sleep(200);
				}
				readi = 0;
				
				if (cdstat != ERR_OK) 
				{
					// Ошибка, заполним нулями и игнорируем
					UnlockVideo();
					cdrom_reinit();
					memset(pbuffer, 0, secbyte);
					bad++;
					ds_printf("DS_ERROR: Can't read sector %ld\n", first);
					LockVideo();
				}
			
				pbuffer += secbyte;
				first++;
			}
		
		} 
		else 
		{
			// Все ок, идем дальше
			first += nsects;
		}
	
		if(fs_write(hnd, buffer, nsects * secbyte) < 0) 
		{
			// Ошибка записи, печально, прерываем процесс
			UnlockVideo();
			free(buffer);
			fs_close(hnd);
			return CMD_ERROR;
		}
		
		UnlockVideo();
		percent = 1-(float)(count) / count_old;
		
		if ((percent = ((int)(percent*100 + 0.5))/100.0) > percent_last) 
		{
			percent_last = percent;
			GUI_ProgressBarSetPosition(self.pbar, percent);
			LockVideo();
		}
	}

	UnlockVideo();
	free(buffer);
	fs_close(hnd);
	ds_printf("%d Bad sectors on track\n", bad);
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
	
	cdrom_set_sector_size (2048);
	if(cdrom_read_toc(&cdtoc, 0) != CMD_OK) 
	{ 
		ds_printf("DS_ERROR:CD Toc read error\n"); 
		free(buff); 
		return CMD_ERROR; 
	}
	
	if(cdrom_read_toc(&gdtoc, 1) != CMD_OK) 
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
	cdrom_spin_down();
}

