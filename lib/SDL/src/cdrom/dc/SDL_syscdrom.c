/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    BERO
    bero@geocities.co.jp

    based on win32/SDL_syscdrom.c by

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_syscdrom.c,v 1.2 2004/01/04 16:49:16 slouken Exp $";
#endif

/* Functions for system-level CD-ROM audio control */

#include <stdlib.h>
#include <stdio.h>
#include <dc/cdrom.h>
#include <dc/spu.h>

#include "SDL_error.h"
#include "SDL_cdrom.h"
#include "SDL_syscdrom.h"

//#define SDL_DC_CDROM_LAG 150
#define SDL_DC_CDROM_LAG 0

/* The system-dependent CD control functions */
static const char *SDL_SYS_CDName(int drive);
static int SDL_SYS_CDOpen(int drive);
static int SDL_SYS_CDGetTOC(SDL_CD *cdrom);
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position);
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length);
static int SDL_SYS_CDPause(SDL_CD *cdrom);
static int SDL_SYS_CDResume(SDL_CD *cdrom);
static int SDL_SYS_CDStop(SDL_CD *cdrom);
static int SDL_SYS_CDEject(SDL_CD *cdrom);
static void SDL_SYS_CDClose(SDL_CD *cdrom);


extern int SDL_numcds;

int  SDL_SYS_CDInit(void)
{
	/* Fill in our driver capabilities */
	SDL_CDcaps.Name = SDL_SYS_CDName;
	SDL_CDcaps.Open = SDL_SYS_CDOpen;
	SDL_CDcaps.GetTOC = SDL_SYS_CDGetTOC;
	SDL_CDcaps.Status = SDL_SYS_CDStatus;
	SDL_CDcaps.Play = SDL_SYS_CDPlay;
	SDL_CDcaps.Pause = SDL_SYS_CDPause;
	SDL_CDcaps.Resume = SDL_SYS_CDResume;
	SDL_CDcaps.Stop = SDL_SYS_CDStop;
	SDL_CDcaps.Eject = SDL_SYS_CDEject;
	SDL_CDcaps.Close = SDL_SYS_CDClose;

	SDL_numcds=1;
	return(0);
}

static const char *SDL_SYS_CDName(int drive)
{
	return "/cd";
}

static int SDL_SYS_CDOpen(int drive)
{
	return(drive);
}

#define	TRACK_CDDA	0
static int SDL_SYS_CDGetTOC(SDL_CD *cdrom)
{
	CDROM_TOC toc;
	int ret,i;

	ret = cdrom_read_toc(&toc,0);
	if (ret!=ERR_OK) {
		return -1;
	}

	cdrom->numtracks = TOC_TRACK(toc.last)-TOC_TRACK(toc.first)+1;
	for(i=0;i<cdrom->numtracks;i++) {
		unsigned long entry = toc.entry[i];
		cdrom->track[i].id = i+1;
		cdrom->track[i].type = (TOC_CTRL(toc.entry[i])==TRACK_CDDA)?SDL_AUDIO_TRACK:SDL_DATA_TRACK;
		cdrom->track[i].offset = TOC_LBA(entry)-SDL_DC_CDROM_LAG;
		cdrom->track[i].length = TOC_LBA((i+1<toc.last)?toc.entry[i+1]:toc.leadout_sector)-TOC_LBA(entry);
	}

	return 0;
}

/* Get CD-ROM status */
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position)
{
	CDstatus status;
	int ret,dc_status,disc_type;

	ret = cdrom_get_status(&dc_status,&disc_type);
	if (ret!=ERR_OK) return CD_ERROR;

	switch(dc_status) {
//	case CD_STATUS_BUSY:
	case CD_STATUS_PAUSED:
		return CD_PAUSED;
	case CD_STATUS_STANDBY:
		return CD_STOPPED;
	case CD_STATUS_PLAYING:
		return CD_PLAYING;
//	case CD_STATUS_SEEKING:
//	case CD_STATUS_SCANING:
	case CD_STATUS_OPEN:
	case CD_STATUS_NO_DISC:
		return CD_TRAYEMPTY;
	default:
		return	CD_ERROR;
	}
}

/* Start play */
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length)
{
	int ret = cdrom_cdda_play(start-SDL_DC_CDROM_LAG,start-SDL_DC_CDROM_LAG+length,1,CDDA_SECTORS);
	return ret==ERR_OK?0:-1;
}

/* Pause play */
static int SDL_SYS_CDPause(SDL_CD *cdrom)
{
	int ret=cdrom_cdda_pause();
	return ret==ERR_OK?0:-1;
}

/* Resume play */
static int SDL_SYS_CDResume(SDL_CD *cdrom)
{
	int ret=cdrom_cdda_resume();
	return ret==ERR_OK?0:-1;
}

/* Stop play */
static int SDL_SYS_CDStop(SDL_CD *cdrom)
{
	int ret=cdrom_spin_down();
	return ret==ERR_OK?0:-1;
}

/* Eject the CD-ROM */
static int SDL_SYS_CDEject(SDL_CD *cdrom)
{
	return -1;
}

/* Close the CD-ROM handle */
static void SDL_SYS_CDClose(SDL_CD *cdrom)
{
}

void SDL_SYS_CDQuit(void)
{

}
