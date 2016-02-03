#ifndef __CDIRIP_H__
#define __CDIRIP_H__


#define true  1
#define false 0

#define DEFAULT_FORMAT   0
#define ISO_FORMAT       1
#define BIN_FORMAT       2
#define MAC_FORMAT       3

#define WAV_FORMAT       0
#define RAW_FORMAT       1
#define CDA_FORMAT       2
#define AIFF_FORMAT      3

#define SHOW_INTERVAL 2000

#define READ_BUF_SIZE  1024*1024
#define WRITE_BUF_SIZE 1024*1024


typedef struct opts_s
       {
       char showinfo;
       char cutfirst;
       char cutall;
       char convert;
       char fulldata;
       char audio;
       char swap;
       char showspeed;
       char pregap;
       } opts_s;

typedef struct flags_s
       {
       char ask_for_image;
       char ask_for_dest_path;
       char do_cut;
       char do_convert;
       char create_cuesheet;
       char save_as_iso;
       } flags_s;


void savetrack(FILE *fsource, image_s *, track_s *, opts_s *, flags_s *);
void savecuesheet(FILE *fcuesheet, image_s *, track_s *, opts_s *, flags_s *);

void show_counter(unsigned long i, long track_length, unsigned long image_length, long pos);

#ifdef _WIN32
int askfilename(char *string);
int askpath(char *sz_buf);
void show_speed (unsigned long sector_size, LARGE_INTEGER Frequency, LARGE_INTEGER *old_count);
#endif


// Strings

// begin changes by fackue
static char STR_TDISC_CUE_FILENAME   [ ] = "tdisc.cue";
static char STR_TDISCN_CUE_FILENAME  [ ] = "s%dt%d.cue";
static char STR_TAUDIO_RAW_FILENAME  [ ] = "s%02dt%02d.raw";
static char STR_TAUDIO_WAV_FILENAME  [ ] = "s%02dt%02d.wav";
static char STR_TAUDIO_CDA_FILENAME  [ ] = "s%02dt%02d.cda";
static char STR_TAUDIO_AIFF_FILENAME [ ] = "s%02dt%02d.aiff";
static char STR_TDATA_ISO_FILENAME   [ ] = "s%02dt%02d.iso";
static char STR_TDATA_BIN_FILENAME   [ ] = "s%02dt%02d.bin";
// end changes by fackue



#endif
