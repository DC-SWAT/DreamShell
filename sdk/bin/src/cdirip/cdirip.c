#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#include "common.h"
#include "cdirip.h"
#include "buffer.h"
#include "cdi.h"
#include "audio.h"

// Linux add-ons by Pedro Melo

#ifndef _WIN32
#define MAX_PATH PATH_MAX
#endif

#ifdef __BORLANDC__
#define strcasecmp stricmp
#endif

// Global variables

char *global_read_buffer_ptr;
char *global_write_buffer_ptr;

// Main function

int main( int argc, char **argv )
{

int  i;
char cuesheetname[13], filename[MAX_PATH], destpath[MAX_PATH];
FILE *fsource, *fcuesheet;

image_s image = { 0, };
track_s track = { 0, };
opts_s  opts  = { 0, };
flags_s flags = { 0, };

image.global_current_session = 0;
track.global_current_track   = 0;
track.position               = 0;

// Opciones

printf("CDIrip - (C) 2004 by DeXT/Lawrence Williams\n");
printf("modified v1 by fackue\n\n");

flags.ask_for_image = true;
flags.ask_for_dest_path = true;

opts.audio = WAV_FORMAT;

#ifndef _WIN32
opts.convert = ISO_FORMAT;  // Linux only!
#else
opts.convert = DEFAULT_FORMAT;
#endif

if (argc >= 2)
   {
   for(i = 1; i < argc; i++)
     {
#ifdef _WIN32
     if (argv[i][0] == '/' || argv[i][0] == '-')
#else
     if (argv[i][0] == '-')    // Linux absolute paths start with '/'
#endif
      {
      if (!strcasecmp(argv[i]+1,"iso"    )) opts.convert   = ISO_FORMAT;
      if (!strcasecmp(argv[i]+1,"bin"    )) opts.convert   = BIN_FORMAT;
      if (!strcasecmp(argv[i]+1,"mac"    )) opts.convert   = MAC_FORMAT; // ISO/2056
      if (!strcasecmp(argv[i]+1,"raw"    )) opts.audio     = RAW_FORMAT;
      if (!strcasecmp(argv[i]+1,"cda"    )) opts.audio     = CDA_FORMAT;
      if (!strcasecmp(argv[i]+1,"wav"    )) opts.audio     = WAV_FORMAT;
      if (!strcasecmp(argv[i]+1,"aiff"   )) opts.audio     = AIFF_FORMAT;
      if (!strcasecmp(argv[i]+1,"info"   )) opts.showinfo  = true;
      if (!strcasecmp(argv[i]+1,"cut"    )) opts.cutfirst  = true;
      if (!strcasecmp(argv[i]+1,"cutall" )) opts.cutall    = true;
      if (!strcasecmp(argv[i]+1,"full"   )) opts.fulldata  = true;
      if (!strcasecmp(argv[i]+1,"swap"   )) opts.swap      = true;
      if (!strcasecmp(argv[i]+1,"speed"  )) opts.showspeed = true;
      if (!strcasecmp(argv[i]+1,"pregap" )) opts.pregap    = true;

      if (!strcasecmp(argv[i]+1,"cdrecord"  )) { opts.cutall = true; opts.convert = ISO_FORMAT; }
      if (!strcasecmp(argv[i]+1,"winoncd"   )) { opts.cutall = true; opts.convert = ISO_FORMAT; opts.audio = RAW_FORMAT; }
      if (!strcasecmp(argv[i]+1,"fireburner")) { opts.cutall = true; opts.convert = BIN_FORMAT; }
      }
     else if (i == 1) flags.ask_for_image = false;
     else if (i == 2) flags.ask_for_dest_path = false;
     }
   }

if (flags.ask_for_image)
   {
#ifdef _WIN32
   if (askfilename(filename) != 0)
      {
      printf("Exit requested by user");
      FreeConsole();
      ExitProcess(0);
      }
#else
   printf("Error: Bad syntax\n\nUsage: cdirip image.cdi [dest_path] [options]\n");  // Linux only
   exit(EXIT_FAILURE);
#endif
   }
else
   strcpy(filename, argv[1]);

// Abrir fichero

printf("Searching file: '%s'\n",filename);

fsource = fopen(filename,"rb");

if (fsource == NULL)
   {
   if (errno == ENOENT)  // "No such file or directory"
      {
      strcat(filename, ".cdi");
      printf("Not found. Trying '%s' instead...\n",filename);
      fsource = fopen(filename,"rb");
      }
   }

if (fsource == NULL) error_exit(ERR_OPENIMAGE, filename);

errno = 0;
printf("Found image file. Opening...\n");

CDI_init (fsource, &image, filename);

if (image.version == CDI_V2)
   printf("This is a v2.0 image\n");
else if (image.version == CDI_V3)
   printf("This is a v3.0 image\n");
else if (image.version == CDI_V35)
   printf("This is a v3.5 image\n");
else
   error_exit(ERR_GENERIC, "Unsupported image version");

// Cambiar directorio de destino

if (!opts.showinfo)
   {
#ifdef _WIN32
   if (!(!flags.ask_for_image && flags.ask_for_dest_path))
      {
      if (flags.ask_for_dest_path)
         {
            if (askpath(destpath) == 0)
               {
               printf("Exit requested by user");
               FreeConsole();
               ExitProcess(0);
               }
         }
      else
         strcpy(destpath, argv[2]);

      printf("Destination path: '%s'\n", destpath);
      if (chdir(destpath) != 0) error_exit(ERR_PATH, destpath);
      }
#else
   if (!flags.ask_for_dest_path)  // en Linux solo cambiar si se especifico un path
      {
      strcpy(destpath, argv[2]);
      printf("Destination path: '%s'\n", destpath);
      if (chdir(destpath) != 0) error_exit(ERR_PATH, destpath);
      }
#endif
   }

// Sets proper audio format

switch (opts.audio)
       {
       case AIFF_FORMAT:
       case CDA_FORMAT:
            opts.swap = opts.swap ? false : true; // invert swap switch for AIFF/CDA (MSB by default)
            break;
       case WAV_FORMAT:
       case RAW_FORMAT:
       default:
            break;
       }

// Start parsing image

printf("\nAnalyzing image...\n");

CDI_get_sessions (fsource, &image);

if (image.sessions == 0)  error_exit(ERR_GENERIC, "Bad format: Could not find header");

printf("Found %d session(s)\n",image.sessions);

// Allocating buffers

global_read_buffer_ptr = (char *) malloc (READ_BUF_SIZE);
global_write_buffer_ptr = (char *) malloc (WRITE_BUF_SIZE);

if (global_read_buffer_ptr == NULL || global_write_buffer_ptr == NULL)
        error_exit(ERR_GENERIC, "Not enough free memory for buffers!");

// Start ripping

if (!opts.showinfo) printf("\nRipping image... (Press Ctrl-C at any time to exit)\n");

if (opts.pregap) printf("Pregap data will be saved\n");

image.remaining_sessions = image.sessions;

/////////////////////////////////////////////////////////////// Bucle sessions

while(image.remaining_sessions > 0)
  {
  image.global_current_session++;

  CDI_get_tracks (fsource, &image);

  image.header_position = ftell(fsource);

  printf("\nSession %d has %d track(s)\n",image.global_current_session,image.tracks);

  if (image.tracks == 0)
     printf("Open session\n");
  else
    {

// Decidir si crear cuesheet

    if (!opts.showinfo)
       {
       if (image.global_current_session == 1)
          {
          if (ask_type(fsource, image.header_position) == 2)
             {
             if (opts.convert == ISO_FORMAT || opts.convert == MAC_FORMAT)
                flags.create_cuesheet = false;  // Si "/iso" y Modo2 -> no cuesheet
             else
                {
                flags.create_cuesheet = true;
                opts.convert = BIN_FORMAT;     // Si Modo2 -> cuesheet + BIN
                }
             }
          else
             flags.create_cuesheet = true;      // Si Audio o Modo1
          }
       else
          {
          if (opts.convert == BIN_FORMAT)      // ojo! tb depende de lo anterior
             flags.create_cuesheet = true;
          else
             flags.create_cuesheet = false;
          }
       }
    else
       flags.create_cuesheet = false;

// Crear cuesheet

    if (flags.create_cuesheet)
       {
       printf("Creating cuesheet...\n");
       if (image.global_current_session == 1)
            sprintf(cuesheetname,STR_TDISC_CUE_FILENAME);
       else
            sprintf(cuesheetname,STR_TDISCN_CUE_FILENAME,image.global_current_session);
       fcuesheet = fopen(cuesheetname,"wb");
       }

    image.remaining_tracks = image.tracks;

///////////////////////////////////////////////////////////////// Bucle tracks

    while(image.remaining_tracks > 0)
         {
         track.global_current_track++;
         track.number = image.tracks - image.remaining_tracks + 1;

         CDI_read_track (fsource, &image, &track);

         image.header_position = ftell(fsource);

// Mostrar info

         if (!opts.showinfo) printf("Saving  ");
         printf("Track: %2d  ",track.global_current_track);
         printf("Type: ");
         switch(track.mode)
               {
               case 0 : printf("Audio/"); break;
               case 1 : printf("Mode1/"); break;
               case 2 :
               default: printf("Mode2/"); break;
               }
         printf("%d  ",track.sector_size);
         if (opts.pregap)
            printf("Pregap: %-3ld  ",track.pregap_length);
         printf("Size: %-6ld  ",track.length);
         printf("LBA: %-6ld  ",track.start_lba);

         if (opts.showinfo) printf("\n");

//       if (track.pregap_length != 150) printf("Warning! This track seems to have a non-standard pregap...\n");

         if (track.length < 0 && opts.pregap == false)
            error_exit(ERR_GENERIC, "Negative track size found\n"
                                    "You must extract image with /pregap option");

// Decidir si cortar


         if (!opts.fulldata && track.mode != 0 && image.global_current_session == 1 && image.sessions > 1)
            flags.do_cut = 2;
         else if (!(track.mode != 0 && opts.fulldata))
            {
            flags.do_cut = ((opts.cutall) ? 2 : 0) +
                           ((opts.cutfirst && track.global_current_track == 1) ? 2 : 0);
            }
         else flags.do_cut = 0;


// Decidir si convertir

         if (track.mode != 0 && track.sector_size != 2048)
            switch (opts.convert)
                   {
                   case BIN_FORMAT: flags.do_convert = false; break;
                   case ISO_FORMAT: flags.do_convert = true; break;
                   case MAC_FORMAT: flags.do_convert = true; break;
                   case DEFAULT_FORMAT:
                   default: if (track.mode == 1)
                               flags.do_convert = true;          // Modo1/2352 -> ISO
                            else
                               if (image.global_current_session > 1)
                                  flags.do_convert = true;       // Modo2 2ª sesion -> ISO
                               else
                                  flags.do_convert = false;      // Modo2 1ª sesion -> BIN (obsoleto)
                            break;
                   }
         else
            flags.do_convert = false;

         if (track.sector_size == 2048 || (track.mode != 0 && flags.do_convert))
            flags.save_as_iso = true;
         else
            flags.save_as_iso = false;

// Guardar la pista

         if (!opts.showinfo)
            {
            if (track.total_length < track.length + track.pregap_length)
               {
               printf("\nThis track seems truncated. Skipping...\n");
               fseek(fsource, track.position, SEEK_SET);
               fseek(fsource, track.total_length, SEEK_CUR);
               track.position = ftell(fsource);
               }
            else
               {
               savetrack(fsource, &image, &track, &opts, &flags);
               track.position = ftell(fsource);

// Generar entradas de cuesheet

               if (flags.create_cuesheet && !(track.mode == 2 && flags.do_convert))  // No generar entrada si convertimos (obsoleto)
                  savecuesheet(fcuesheet, &image, &track, &opts, &flags);

               }
            }

         fseek(fsource, image.header_position, SEEK_SET);


// Cerrar bucles

         image.remaining_tracks--;
         }

    if (flags.create_cuesheet) fclose(fcuesheet);
    }

  CDI_skip_next_session (fsource, &image);

  image.remaining_sessions--;
  }

// Liberar buffers

  free(global_write_buffer_ptr);
  free(global_read_buffer_ptr);

// Finalizar

printf("\nAll done!\n");

if (!opts.showinfo) printf("Good burnin'...\n");

fclose(fsource);

return 0;
}


//////////////////////////////////////////////////////////////////////////////


void savetrack(FILE *fsource, image_s *image, track_s *track, opts_s *opts, flags_s *flags)
{

unsigned long i, ii;
long track_length;
unsigned long header_length;
char tmp_val;
int all_fine;
char buffer[2352], filename [13];
FILE *fdest;

#ifdef _WIN32
LARGE_INTEGER Frequency, old_count;
#endif

struct buffer_s read_buffer;
struct buffer_s write_buffer;

#ifdef _WIN32
     if (opts->showspeed)
     {
     QueryPerformanceFrequency(&Frequency);
     QueryPerformanceCounter(&old_count);
     }
#endif

     fseek(fsource, track->position, SEEK_SET);

// begin changes by fackue
     if (track->mode == 0)
        {
        switch (opts->audio)
               {
               case RAW_FORMAT:
                    sprintf(filename,STR_TAUDIO_RAW_FILENAME,image->global_current_session,track->global_current_track);
                    break;
               case CDA_FORMAT:
                    sprintf(filename,STR_TAUDIO_CDA_FILENAME,image->global_current_session,track->global_current_track);
                    break;
               case AIFF_FORMAT:
                    sprintf(filename,STR_TAUDIO_AIFF_FILENAME,image->global_current_session,track->global_current_track);
                    break;
               case WAV_FORMAT:
               default:
                    sprintf(filename,STR_TAUDIO_WAV_FILENAME,image->global_current_session,track->global_current_track);
                    break;
               }
        }
     else
        {
        if (flags->save_as_iso)
              sprintf(filename,STR_TDATA_ISO_FILENAME,image->global_current_session,track->global_current_track);
        else
              sprintf(filename,STR_TDATA_BIN_FILENAME,image->global_current_session,track->global_current_track);
        }
// end changes by fackue

     fdest = fopen(filename,"wb");

     if (fdest == NULL) error_exit(ERR_SAVETRACK, filename);

     read_buffer.file = fsource;
     read_buffer.size = READ_BUF_SIZE;
     read_buffer.index = 0;
     read_buffer.ptr = global_read_buffer_ptr;
     write_buffer.file = fdest;
     write_buffer.size = WRITE_BUF_SIZE;
     write_buffer.index = 0;
     write_buffer.ptr = global_write_buffer_ptr;

     fseek(fsource, track->pregap_length*track->sector_size, SEEK_CUR);  // always skip pregap

     if (flags->do_cut != 0) printf("[cut: %d] ", flags->do_cut);

     track_length = track->length - flags->do_cut;     // para no modificar valor original

     if (opts->pregap && track->mode == 0 && image->remaining_tracks > 1)  // quick hack to save next track pregap (audio tracks only)
        track_length += track->pregap_length;                              // if this isn't last track in current session

     if (flags->do_convert)
         printf("[ISO]\n");
     else
         printf("\n");
/*
     if (track->mode == 2)
        switch (track->sector_size) {
               case 2352: sector_type = MODE2_2352; break;
               case 2336: sector_type = MODE2_2336; break;
               case 2056: sector_type = MODE2_2056; break;
               case 2048: sector_type = MODE2_2048; break;
               }
     else if (track->mode == 1)
        switch (track->sector_size) {
               case 2352: sector_type = MODE1_2352; break;
               case 2048: sector_type = MODE1_2048; break;
               }
     else if (track->mode == 0)
        switch (track->sector_size) {
               case 2352: sector_type = MODE0_2352; break;
               }
*/
     if (flags->do_convert) {
	if (track->mode == 2) {
		switch (track->sector_size) {
			case 2352: header_length = 24; break;
			case 2336: header_length =  8; break;
			default:   header_length =  0; break;
			}
	} else {
		switch (track->sector_size) {
			case 2352: header_length = 16; break;
			case 2048:
			default:   header_length =  0; break;
			}
	}
     }

     if (track->mode == 0)
        switch (opts->audio)
               {
               case WAV_FORMAT:
                    writewavheader(fdest, track_length);
                    break;
               case AIFF_FORMAT:
                    writeaiffheader(fdest, track_length);
                    break;
               default:
                    break;
               }

     for(i = 0; i < track_length; i++)
        {
#ifndef DEBUG_CDI
              if (!(i%128)) show_counter (i, track_length, image->length, ftell(fsource));
#endif
              BufRead (buffer, track->sector_size, &read_buffer, image->length);

              if (track->mode == 0 && opts->swap)
                 for (ii = 0; ii < track->sector_size; ++ii, ++ii)
                     {
                     tmp_val = buffer[ii];
                     buffer[ii] = buffer[ii+1];
                     buffer[ii+1] = tmp_val;
                     }

              if (flags->do_convert)
                 {
                 if (opts->convert == MAC_FORMAT) BufWrite("\0\0\x08\0\0\0\x08\0", 8, &write_buffer);
                 all_fine = BufWrite (buffer + header_length, 2048, &write_buffer);
                 }
              else
                 {
                 all_fine = BufWrite (buffer, track->sector_size, &write_buffer);
                 }

              if (!all_fine) error_exit(ERR_SAVETRACK, filename);

#ifdef _WIN32
              if (opts->showspeed && !((i+1)%SHOW_INTERVAL)) show_speed (track->sector_size, Frequency, &old_count);
#endif
        }

     printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            "                                                          "
            "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

//     if (flags->do_cut != 0) fseek(fsource, flags->do_cut*track->sector_size, SEEK_CUR);

// Calcular posicion (BufRead no seguro, tb cuttrack & pregap)

     fseek(fsource, track->position, SEEK_SET);
//     fseek(fsource, track->pregap_length * track->sector_size, SEEK_CUR);
//     fseek(fsource, track->length * track->sector_size, SEEK_CUR);
     fseek(fsource, track->total_length * track->sector_size, SEEK_CUR);

// Vaciar buffers

     BufWriteFlush(&write_buffer);

     fflush(fdest);
     fclose(fdest);
}


//////////////////////////////////////////////////////////////////////////////


void show_counter (unsigned long i, long track_length, unsigned long image_length, long pos)
{
     unsigned long progress, total_progress;

        progress = (i*100/track_length);
        total_progress = ((pos>>10)*100)/(image_length>>10);
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
        "[Current: %02d%%  Total: %02d%%]  ",progress, total_progress);
}


#ifdef _WIN32

void show_speed (unsigned long sector_size, LARGE_INTEGER Frequency, LARGE_INTEGER *old_count)
{
     unsigned long speed, elapsed;
     LARGE_INTEGER PerformanceCount, last_count;

        QueryPerformanceCounter(&PerformanceCount);
        last_count = *old_count;

        elapsed = (PerformanceCount.QuadPart - last_count.QuadPart)/(Frequency.QuadPart/1000);  // in ms
        speed = SHOW_INTERVAL*sector_size/(elapsed > 0 ? elapsed : 1);
        printf("[Speed: %6ld KB/s]  "
        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", speed);

        *old_count = PerformanceCount;
}

#endif


//////////////////////////////////////////////////////////////////////////////


void savecuesheet(FILE *fcuesheet, image_s *image, track_s *track, opts_s *opts, flags_s *flags)
{

char track_format_string[10];
char audio_file_ext[5];

     if (opts->swap)
        strcpy(track_format_string,"MOTOROLA");
     else
        strcpy(track_format_string,"BINARY");

     switch (opts->audio)
            {
            case AIFF_FORMAT:
                 strcpy(audio_file_ext,"aiff");
                 break;
            case CDA_FORMAT:
                 strcpy(audio_file_ext,"cda");
                 break;
            case RAW_FORMAT:
                 strcpy(audio_file_ext,"raw");
                 break;
            case WAV_FORMAT:
            default:
                 strcpy(audio_file_ext,"wav");
                 break;
            }

     if (track->mode == 0)
        {
        if (opts->audio == WAV_FORMAT)
              fprintf(fcuesheet, "FILE taudio%02d.wav WAVE\r\n"
                                 "  TRACK %02d AUDIO\r\n",
                                 track->global_current_track,
                                 track->number);
        else
              fprintf(fcuesheet, "FILE taudio%02d.%s %s\r\n"
                                 "  TRACK %02d AUDIO\r\n",
                                 track->global_current_track,
                                 audio_file_ext,
                                 track_format_string,
                                 track->number);

        if (track->global_current_track > 1 && !opts->pregap && track->pregap_length > 0)
              fprintf(fcuesheet, "    PREGAP 00:02:00\r\n");

        }
     else
        {
        if (flags->save_as_iso)
              fprintf(fcuesheet, "FILE tdata%02d.iso BINARY\r\n"
                                 "  TRACK %02d MODE%d/2048\r\n",
                                 track->global_current_track,
                                 track->number,
                                 track->mode);
        else
              fprintf(fcuesheet, "FILE tdata%02d.bin BINARY\r\n"
                                 "  TRACK %02d MODE%d/%d\r\n",
                                 track->global_current_track,
                                 track->number,
                                 track->mode,
                                 track->sector_size);
        }

     fprintf(fcuesheet, "    INDEX 01 00:00:00\r\n");

     if (opts->pregap && track->mode != 0 && image->remaining_tracks > 1) // instead of saving pregap
        fprintf(fcuesheet, "  POSTGAP 00:02:00\r\n");

}


//////////////////////////////////////////////////////////////////////////////


#ifdef _WIN32

int askfilename(char *string)
{

HWND hWnd = NULL;
OPENFILENAME ofn;
char szFile[MAX_PATH];

     memset(&ofn, 0, sizeof(ofn));
     szFile[0] = 0;

#ifdef __BORLANDC__
     ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
     ofn.lStructSize = sizeof(ofn);
#endif
     ofn.hwndOwner = hWnd;
     ofn.lpstrFile = szFile;
     ofn.nMaxFile = sizeof(szFile);
     ofn.lpstrFilter = "DiscJuggler image (*.cdi;*.cdr)\0*.cdi;*.cdr\0All files (*.*)\0*.*\0\0";
     ofn.nFilterIndex = 1;
     ofn.lpstrDefExt = "cdi";
     ofn.lpstrTitle = "Select CDI image to open...";
     ofn.lpstrFileTitle = NULL;
     ofn.nMaxFileTitle = 0;
     ofn.lpstrInitialDir = NULL;
     ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

     if (GetOpenFileName(&ofn))
        strcpy (string, szFile);
     else
        return 1;

     return 0;
}

#endif


//////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

int askpath(char *sz_buf)
{

// taken (w/small changes) from Olan Patrick's FILEDLGS library (PD)

    BROWSEINFO      bi;
    LPITEMIDLIST    pidlRoot, pidlChosen;
    char            pszBuffer[MAX_PATH];
    char            szChosenFolder[MAX_PATH] = "\\\\";
    int             retval;

    HWND            hwndf = NULL;
    DWORD           dwFlags = BIF_RETURNONLYFSDIRS;

    SHGetSpecialFolderLocation(hwndf, CSIDL_DESKTOP, &pidlRoot);

    memset(&bi, 0, sizeof(BROWSEINFO));
    bi.hwndOwner = hwndf;
    bi.pidlRoot = pidlRoot;
    bi.pszDisplayName = pszBuffer;
    bi.lpszTitle = "Select destination path...";
    bi.ulFlags = dwFlags;

    retval = 0;
    if ((pidlChosen = SHBrowseForFolder(&bi)) != NULL)
    {
        if (dwFlags & BIF_BROWSEFORCOMPUTER)
        {
            strncat(szChosenFolder, pszBuffer, MAX_PATH - 2);
            retval = 1;
        }
        else if (SHGetPathFromIDList(pidlChosen, pszBuffer))
        {
            strncpy(szChosenFolder, pszBuffer, MAX_PATH);
            retval = 1;
        }
    }
    else
        szChosenFolder[0] = '\0';

    strncpy(sz_buf, szChosenFolder, MAX_PATH);
    return (retval);

}

#endif

