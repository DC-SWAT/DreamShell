/*
 * This file is part of nerorip. (c)2011 Joe Balough
 *
 * Nerorip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nerorip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nerorip.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <getopt.h> // getopt_long
#include <ctype.h> // getopt_long
#include "util.h"
#include "nrg.h"

#define AUD_WAV  0
#define AUD_RAW  1
#define AUD_CDA  2
#define AUD_AIFF 3

#define DAT_ISO 0
#define DAT_BIN 1
#define DAT_MAC 2

#define TRIM_NONE  0
#define TRIM_FIRST 1
#define TRIM_ALL   2

/**
 * Whether only information about the file should be printed
 * Should be 0 or 1 for false or true respectively
 */
static int info_only = 0;

/**
 * What format the data tracks should be saved as
 * Should be DAT_ISO, DAT_BIN, or DAT_MAC
 */
static int data_output = DAT_ISO;

// An array of strings describing the data output options
static char data_output_str[3][26] = {"converted ISO/2048", "raw bin", "converted \"Mac\" ISO/2048"};
// An array of strings with the extension for each of those output types
static char data_output_ext[3][4] = {"iso", "bin", "iso"};

/**
 * What format the audio tracks should be output as
 * Should be AUD_WAV, AUD_RAW, AUD_CDA, or AUD_AIFF
 */
static int audio_output = AUD_WAV;

/**
 * Whether or not audio tracks should be swapped
 * Should be 0 or 1 for false or true respectively
 */
static int swap_audio_output = 0;

// An array of strings to represent the audio output types
static char audio_output_str[4][5] = {"wav", "raw", "cda", "aiff"};

/**
 * Whether or not tracks should be trimmed
 * Should be TRIM_NONE or TRIM_FIRST or TRIM_ALL or TRIM_FIRST & TRIM_ALL
 */
static int trim_tracks = TRIM_FIRST;

/**
 * Whether or not pretrack data should be moved to the end of the previous track
 * Should be 0 or 1 for false or true respectively
 */
static int move_pretrack = 0;


void usage(char *argv0) {
  // Used letters: a b c h i f m p q r s t T v
  printf("Usage: %s [OPTIONS]... [INPUT FILE] [OUTPUT DIRECTORY]\n", argv0);
  printf("Nerorip takes a nero image file (.nrt extension) as input\n");
  printf("and attempts to extract the track data as either ISO or audio data.\n\n");
  printf("  Audio track saving options:\n");
  printf("    -r, --raw\t\tSave audio data as little endian raw data\n");
  printf("    -c, --cda\t\tSwitches data to big endian and saves as RAW\n");
  printf("    -a, --aiff\t\tSwitches data to big endian and saves as an AIFF file\n");
  printf("    -s, --swap\t\tChanges data between big and little endian (only affects --aiff and --cda)\n");
  printf("  If omitted, Audio tracks will be exported as WAV files\n\n");

  printf("  Data track saving options:\n");
  printf("    -b, --bin\t\tExport data directly out of image file\n");
  printf("    -m, --mac\t\tConvert data to \"Mac\" ISO/2056 format\n");
  printf("  If omitted, Data tracks will be converted to ISO/2048 format\n\n");

  printf("  Data track trimming options:\n");
  printf("    -t, --trim\t\tTrim 2 sectors from the end of the first track\n");
  printf("    -T, --trimall\t\tTrim 2 sectors from the end of all tracks\n");
  printf("    -f, --full\t\tDo not cut any sectors from any tracks\n");
//  printf("    -p, --pregap\t\tAppend track's pregap data to the end of the previous track\n");
  printf("  --trim and --trimall can be combined, resulting in 4 sectors being trimmed from the first track\n");
  printf("  If omitted, only the first track will have 2 sectors trimmed. See readme for more information\n\n");

  printf("  General options:\n");
  printf("  -i, --info\t\tOnly disply information about the image file, do not rip\n");
  printf("  -v, --verbose\t\tIncrement program verbosity by one tick\n");
  printf("  -q, --quiet\t\tDecrement program verbosity by one tick\n");
  printf("             \t\tVerbosity starts at 1, a verbosity of 0 will print nothing.\n");
  printf("  -h, --help\t\tDisplay this help message and exit\n");
  printf("      --version\t\tOutput version information and exit.\n\n");
  printf("If output directory is omitted, image data is put in the same directory as the input file.\n\n");

  printf("For each track found in the image, nerorip will output the following:\n");
  printf("  one iso file named \"tdataTT.[iso/bin]\" if the track is data and\n");
  printf("  one wav file named \"taudioTT.[wav/bin/cda/aiff]\" if the track is audio\n");
  printf("where TT is the track number.\n");
  printf("Also, for each session in the image, nerorip will output one cue file.\n\n");

  printf("For example, if your disc image is like the following\n");
  printf("  Session 1:\n    Track 1: Audio\n    Track 2: Data\n  Session 2:\n    Track 1: Data\n");
  printf("nerorip will output the following files:\n");
  printf("  taudio01.wav, tdata02.iso, tdata03.iso\n");
  printf("Note that the track number does not reset between sessions.\n\n");

  printf("Report all bugs at %s\nVersion %s\n", WEBSITE, VERSION);
  exit(EXIT_SUCCESS);
}

void version(char *argv0) {
  printf("%s v%s\n", argv0, VERSION);
  printf("Licensed under GNU LGPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
  printf("This is free software: you are free to change and redistribute it.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  // Configure the long getopt options
  static struct option long_options[] = {
    // Audio
    {"raw",      no_argument, 0, 'r'},
    {"cda",      no_argument, 0, 'c'},
    {"aiff",     no_argument, 0, 'a'},
    {"swap",     no_argument, 0, 's'},
    // Data
    {"bin",      no_argument, 0, 'b'},
    {"mac",      no_argument, 0, 'm'},
    // Trim
    {"trim",      no_argument, 0, 't'},
    {"trimall",   no_argument, 0, 'T'},
    {"full",      no_argument, 0, 'f'},
    {"pregap",    no_argument, 0, 'p'},
    // General
    {"info",     no_argument, 0, 'i'},
    {"verbose",  no_argument, 0, 'v'},
    {"quiet",    no_argument, 0, 'q'},
    {"help",     no_argument, 0, 'h'},
    {"version",  no_argument, 0, 'V'},
    {0, 0, 0, 0}
  };

  // If any trim options are applied, the default falls back to TRIM_NONE
  int new_trim_tracks = TRIM_NONE;
  int use_new_trim_tracks = 0;

  // Loop through all the passed options
  int c;
  while ((c = getopt_long (argc, argv, "rcasbmtTfpivqhV", long_options, NULL)) != -1) {
    switch (c) {
      /*
       * Audio track options
       */

      // Raw
      case 'r': audio_output = AUD_RAW; break;
      // Cda
      case 'c':
        audio_output = AUD_CDA;
        swap_audio_output = !swap_audio_output;
        break;
      // Aiff
      case 'a':
        audio_output = AUD_AIFF;
        swap_audio_output = !swap_audio_output;
        break;
      // Swap
      case 's':
        swap_audio_output = !swap_audio_output;
        break;

      /*
       * Data track options
       */

      // Bin
      case 'b': data_output = DAT_BIN; break;
      // Mac
      case 'm': data_output = DAT_MAC; break;

      /*
       * Track trimming options
       */

      // Since trim and trimall can be used together, they need to be OR'd on
      // trim
      case 't':
        new_trim_tracks  |= TRIM_FIRST;
        use_new_trim_tracks = 1;
        break;
      // trimall
      case 'T':
        new_trim_tracks  |= TRIM_ALL;
        use_new_trim_tracks = 1;
        break;
      // full
      case 'f':
        new_trim_tracks   = TRIM_NONE;
        use_new_trim_tracks = 1;
        break;
      // pregap
      case 'p': move_pretrack = 1; break;


      /*
       * General options
       */

      // Verbose
      case 'v': inc_verbosity(); break;
      // Quiet
      case 'q': dec_verbosity(); break;
      // Info
      case 'i': info_only = 1; break;
      // Help
      case 'h': usage(argv[0]); break;
      // Version
      case 'V': version(argv[0]); break;
      default: break;
    }
  }

  // Reset the trim_tracks option now that all the options have been parsed
  if (use_new_trim_tracks)
    trim_tracks = new_trim_tracks;

  // Print simple welcome message
  ver_printf(1, "neorip v%s\n", VERSION);

  // Note any enabled options

  // Note if info_only is on. If it is, that's the only option to tell the user about.
  if (info_only)
    ver_printf(1, "Will only print disc image information.\n");

  else {
    // Audio track information
    ver_printf(1, "Saving audio tracks as %s %s files\n", (swap_audio_output ? "swapped" : "non-swapped"), audio_output_str[audio_output]);

    // Data track information
    ver_printf(1, "Saving data tracks as %s files.\n", data_output_str[data_output]);

    // Data trimming information
    if (trim_tracks == TRIM_NONE)
      ver_printf(1, "Not trimming any track data.\n");
    else {
      int trim_first = (trim_tracks & TRIM_FIRST) ? 2 : 0;
      int trim_all = (trim_tracks & TRIM_ALL) ? 2 : 0;
      ver_printf(1, "Trimming %d sectors from first track and %d sectors from all other tracks\n", trim_first + trim_all, trim_all);
    }

    // Moving pretrack
//    if (move_pretrack) ver_printf(1, "Appending all tracks' pregap data to the end of the previous track\n");
  }

  // Now that all the getopt options have been parsed, that only leaves the input file and output directory.
  // Those two values should be in argv[optind] and argv[optind + 1]
  // Make sure they were actually provided before accessing them
  if (optind == argc) {
    fprintf(stderr, "Error: No input file provided\n\n");
    usage(argv[0]);
  }

  char *input_str = argv[optind];
  ver_printf(2, "Opening file %s\n", input_str);
  FILE *image_file = fopen(input_str, "rb");
  if (image_file == NULL) {
    fprintf(stderr, "Error opening %s: %s\n", input_str, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Figure out output directory
  char *output_dir = getenv("PWD");
  if (optind + 2 == argc)
    output_dir = argv[optind + 1];
  if (!info_only)
    ver_printf(2, "Outputing data to %s\n", output_dir);

  ver_printf(3, "Allocating memory\n");
  nrg_image *image = alloc_nrg_image();

  // Parse the image file
  nrg_parse(image_file, image);
  ver_printf(3, "\n");

  // Print the collected information
  nrg_print(1, image);

  if (info_only)
    goto quit;

  ver_printf(1, "Saving track data:\n");
  // Try to extract that data
  unsigned int track = 1;
  nrg_session *s;
  for(s = image->first_session; s != NULL; s=s->next) {
    nrg_track *t;
    for (t = s->first_track; t!=NULL; t=t->next) {

      // Seek to the track lba
      fseek(image_file, t->track_offset, SEEK_SET);

      char filename[256];
      sprintf(filename, "%s/%s%02d.%s", output_dir, (t->track_mode == AUDIO ? "taudio" : "tdata"), track, (t->track_mode == AUDIO ? audio_output_str[audio_output] : data_output_ext[data_output]));
      ver_printf(1, "  %s: 00%%", filename);

      // Open up a file to dump stuff into
      FILE *tf = fopen(filename, "wb");

      // Determine the number of sectors to write depending on the trimming options
      unsigned trimmed_track_length = t->length;
      // First track trimming
      if (track == 1 && (trim_tracks & TRIM_FIRST))
        trimmed_track_length -= 2 * t->sector_size;
      if (trim_tracks & TRIM_ALL)
        trimmed_track_length -= 2 * t->sector_size;


      // Add the proper header if the track is AUDIO
      if (t->track_mode == AUDIO) {
        switch (audio_output) {
          case AUD_WAV:
            fwrite_wav_header(tf, trimmed_track_length);
            break;
          case AUD_AIFF:
            fwrite_aiff_header(tf, trimmed_track_length / t->sector_size);
            break;
        }
      }


      // Write length bytes
      unsigned int b;
      for (b = 0; b < t->length; b += t->sector_size)
      {
        // Update status
        ver_printf(1, "\b\b\b%02d%%", (int)( ((float) b / (float) t->length) * 100.0));

        // Read one sector of data
        uint8_t *buffer = malloc(sizeof(uint8_t) * t->sector_size);
        if (fread(buffer, sizeof(uint8_t), t->sector_size, image_file) != t->sector_size) {
          fprintf(stderr, "Error reading track: %s\n", strerror(errno));
        }

        // Swap buffer if necessary
        if (swap_audio_output && t->track_mode == AUDIO)
          swap_buffer(buffer, t->sector_size);

        // If the track isn't audio and a conversion is to be done, figure out how long the header is
        unsigned int header_length = 0;
        unsigned int write_length = t->sector_size;
        if (t->track_mode != AUDIO && data_output != DAT_BIN) {

          // The header length depends on the track mode
          if (t->track_mode == MODE2) {
            switch (t->sector_size) {
              case 2352: header_length = 24; break;
              case 2336: header_length = 8;  break;
              default:   header_length = 0;  break;
            }
          }
          else {
            switch (t->sector_size) {
              case 2352: header_length = 16; break;
              default:   header_length = 0;  break;
            }
          }

          // Since we're converting to ISO/2048, the sector length is now 2048
          write_length = 2048;
        }
        // If converting to the "Mac" iso type, Write that header
        if (data_output == DAT_MAC) {
          if (fwrite("\0\0\x08\0\0\0\x08\0", sizeof(uint8_t), 8, tf) != 8) {
            fprintf(stderr, "Error writing Mac header to track: %s\n Skipping this track.\n", strerror(errno));
            fclose(tf);
            free(buffer);
            continue;
          }
        }


        // Only write the sector if it isn't to be trimmed
        if (b < trimmed_track_length) {

          // Write one sector of data skipping the header (if converting)
          if (fwrite(buffer + header_length, sizeof(uint8_t), write_length, tf) != write_length) {
            fprintf(stderr, "Error writing track: %s\n  Skipping this track.\n", strerror(errno));
            fclose(tf);
            free(buffer);
            continue;
          }

        }
        // If the sector is to be trimmed, have a look and see if it might contain some useful data
        else {
          unsigned int d;
          for (d = header_length; d < write_length; d++)
            if (buffer[d]) {
              ver_printf(1, "\n  WARNING: Might be trimming relevant data from the end of this track. Consider using the --full option.\n");
              break;
            }
        }

        // Clean up buffer
        free(buffer);
      }

      ver_printf(1, "\b\b\b100%%\n");

      // Close that file
      fclose(tf);

      track++;
    }
  }

quit:
  // Close file and free ram
  ver_printf(3, "Cleaning up\n");
  fclose(image_file);
  free_nrg_image(image);

  return EXIT_SUCCESS;
}

