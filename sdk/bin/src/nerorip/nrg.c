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

#include "nrg.h"

// Allocates memory for an nrg_image
nrg_image *alloc_nrg_image() {
  // Do the malloc
  nrg_image *r = malloc(sizeof(nrg_image));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_image structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->nrg_version = UNPROCESSED;
  r->first_chunk_offset = 0x00;
  r->first_session = NULL;
  r->last_session = NULL;
  r->number_sessions = 0;
  r->number_tracks = 0;

  return r;
}


// Frees memory used by an nrg_image
void free_nrg_image(nrg_image *image) {
  // Make sure image was allocated in the first place
  if (!image)
    return;

  // Loop through all the sessions
  nrg_session *s;
  for (s = image->first_session; s != NULL; ) {

    // Loop through all the tracks in this session
    nrg_track *t;
    for (t = s->first_track; t != NULL; ) {

      // Get the next track before freeing this one
      nrg_track *next_track = t->next;

      // Free this track
      free(t);

      // reset t
      t = next_track;
    }

    // Now that all the tracks have been freed, free this session
    nrg_session *next_session = s->next;
    free(s);
    s = next_session;
  }

  // It's now safe to free the image
  free(image);
  image = NULL;
}


// Allocates memory for an nrg_session
nrg_session *alloc_nrg_session() {
  // Do the malloc
  nrg_session *r = malloc(sizeof(nrg_session));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_session structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->next = NULL;
  r->first_track = NULL;
  r->last_track = NULL;
  r->number_tracks = 0;

  return r;
}


// Allocates memory for an nrg_track
nrg_track *alloc_nrg_track() {
  // Do the malloc
  nrg_track *r = malloc(sizeof(nrg_track));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_track structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->next = NULL;

  return r;
}


// Adds an nrg_session to an nrg_image
void add_nrg_session(nrg_image *image, nrg_session *session) {
  if (!image || !session) {
    fprintf(stderr, "add_nrg_session got an unallocated nrg_image or nrg_session.\n");
    return;
  }

  // If this is the first session going on the image, this session is both the first and last
  if (image->first_session == NULL) {
    image->first_session = session;
    image->last_session = session;
  }
  // Otherwise, it goes on the end and is the new last_session
  else {
    image->last_session->next = session;
    image->last_session = session;
  }
}


// Adds an nrg_track to an nrg_session
void add_nrg_track(nrg_session *session, nrg_track *track) {
  if (!track || !session) {
    fprintf(stderr, "add_nrg_track got an unallocated nrg_session or nrg_track.\n");
    return;
  }

  // If this is the first track going on the session, this track is both the first and last
  if (session->first_track == NULL) {
    session->first_track = track;
    session->last_track = track;
  }
  // Otherwise, it goes on the end and is the new last_track
  else {
    session->last_track->next = track;
    session->last_track = track;
  }
}


// Parses the chunk data in the image file to fill in nrg_image data structure
int nrg_parse(FILE *image_file, nrg_image *image) {
  // Make sure properly allocated
  if (!image_file || !image)
    return NON_ALLOC;

  ver_printf(3, "Detecting NRG file version:\n");

  // Seek to 12 bytes from the end and try to read the footer.
  fseek(image_file, -12, SEEK_END);

  // If the value there is NER5, it's version 5.5
  if (fread32u(image_file) == NER5) {
    image->first_chunk_offset = fread64u(image_file);
    image->nrg_version = NRG_VER_55;
    ver_printf(3, "  File appears to be a Nero 5.5 image\n");
  }
  // Otherwise, try to read the next chunk, if it's NERO, then its version 5
  else if (fread32u(image_file) == NERO) {
    image->first_chunk_offset = (uint64_t) fread32u(image_file);
    image->nrg_version = NRG_VER_5;
    ver_printf(3, "  File appears to be a Nero 5 image\n");
  }
  // If it wasn't either of the above, it must not be a nero image.
  else {
    image->nrg_version = NOT_NRG;
    ver_printf(3, "  File does not appear to be a Nero image\n");
  }

  ver_printf(3, "Seeking to first chunk offset\n");
  fseek(image_file, image->first_chunk_offset, SEEK_SET);

  ver_printf(3, "Processing Chunk data:\n");

  // Keep track of what should be returned
  int r = 0;

  // Keep count of sessions and tracks
  unsigned int session_number = 1;
  unsigned int track_number = 1;

  // Don't let this loop forever: break if we reach the end of the file
  while (!feof(image_file)) {

    // Chunk data came from the source for libdiscmage available at http://sourceforge.net/projects/discmage/
    // The chunk ID and chunk size are always 32 bit integers
    const long int chunk_offset = ftell(image_file);
    const uint32_t chunk_id = fread32u(image_file);
    const uint32_t chunk_size = fread32u(image_file);

    if (chunk_id == CUES || chunk_id == CUEX) {
      /**
      * CUES / CUEX (Cue Sheet) Format:
      *   Indicates a disc at once image
      *   Chunk size = (# tracks + 1) * 16
      *
      *   5.0  | Description                    | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *   1 B  | Mode                           | 0x41 = mode2, 0x01 = audio
      *   1 B  | Track number                   | 0x00
      *   1 B  | Index                          | 0x00
      *   1 B  | 00
      *   4 B  | v5 MM:SS:FF = 0; v5.5 StartLBA | s1t1 is LBA = 0xffffff6a, MSF = 00:00:00
      * --------------------------------------------------------------------------------------------------------------------
      *   1 B  | Mode
      *   1 B  | Track number                   | First is 1, increments over tracks in ALL sessions
      *   1 B  | Index                          | 0x00, which is pregap for track
      *   1 B  | 00
      *   4 B  | V1: MMSSFF; V2: LBA            | MMSSFF = index, s1t1's LBA is 0xffffff6a
      *   1 B  | Mode
      *   1 B  | Track number                   | First is 1, increments over tracks in ALL sessions
      *   1 B  | Index                          | 0x01, which is main track
      *   1 B  | 00
      *   4 B  | V1: MMSSFF; V2: LBA            | MMSSFF = index, LBA is where track actually starts
      *   ... Repeat for each track in session
      * --------------------------------------------------------------------------------------------------------------------
      *   4 B  | mm AA 01 00                    | mm = mode (if version 2)
      *   4 B  | V1: Last MMSSFF; V2: Last LBA  | MMSSFF = index1 + length
      *
      * About the LBA: The first LBA is the starting LBA for this session. If it's the first session, it's usually 0xffffff6a.
      *                The middle part repeats once for each track. The first LBA is the pre-start LBA for the track and the
      *                second is indicates where the track actually begins.
      *                Unless the track is audio with an intro bit (where the player starts at a negative time), the second LBA
      *                in each loop is 0x00000000
      */

      // CUES / CUEX indicates the start of a new DAO session so create a new session and add it to the image
      nrg_session *session = alloc_nrg_session();
      add_nrg_session(image, session);
      session->burn_mode = DAO;

      // Set burn mode and session mode and number of tracks
      session->number_tracks = chunk_size / 16 - 1;
      session->session_mode = fread8u(image_file);

      // Skip junk
      assert(fread8u(image_file) == 0x00); // Track
      assert(fread8u(image_file) == 0x00); // 0x00
      assert(fread8u(image_file) == 0x00); // Index

      session->start_lba = fread32u(image_file);
      ver_printf(3, "  %s at 0x%X: Size - %d B\n", (chunk_id == CUES ? "CUES" : "CUEX"), chunk_offset, chunk_size);
      ver_printf(3, "    Session %d has %d track(s) using mode %s and starting at 0x%X.\n", session_number, session->number_tracks, (session->session_mode == 0x41 ? "Mode2" : (session->session_mode == 0x01 ? "Audio" : "Unknown")), session->start_lba);

      unsigned int i = 1;
      for (i = 1; i <= session->number_tracks; i++, track_number++) {
        // Each of these middle chunks holds a bit of data about one track for the session.
        // Allocate and add the new track
        nrg_track *track = alloc_nrg_track();
        add_nrg_track(session, track);

        track->pretrack_mode  = fread8u(image_file);
        assert(fread8u(image_file) == track_number);    // Track number
        assert(fread8u(image_file) == 0x00); // Track index
        assert(fread8u(image_file) == 0x00); // 0x00
        track->pretrack_lba = fread32u(image_file);

        track->track_mode = fread8u(image_file);
        assert(fread8u(image_file) == track_number);    // Track number
        assert(fread8u(image_file) == 0x01); // Track index
        assert(fread8u(image_file) == 0x00); // 0x00
        track->track_lba = fread32u(image_file);

        assert(track->pretrack_mode == track->track_mode);

        ver_printf(3, "      Track %d: Index 0 uses mode %s and starts at LBA 0x%X, ", i, (track->pretrack_mode == 0x41 ? "Mode2" : (track->pretrack_mode == 0x01 ? "Audio" : "Unknown")), track->pretrack_lba);
        ver_printf(3, "Index 1 uses mode %s and starts at LBA 0x%X.\n", (track->track_mode == 0x41 ? "Mode2" : (track->track_mode == 0x01 ? "Audio" : "Unknown")), track->track_lba);
      }

      // Skip junk
      // Nero 5.5 images always have the session mode here but 5.0 images generated by cdi2nero have a 0x00 here.
      if (image->nrg_version == NRG_VER_55)
        assert(fread8u(image_file) == session->session_mode);
      else
        fread8u(image_file);
      assert(fread8u(image_file) == 0xaa);         // 0xAA
      assert(fread8u(image_file) == 0x01);         // 0x01
      assert(fread8u(image_file) == 0x00);         // 0x00

      session->end_lba = fread32u(image_file);
      ver_printf(3, "    Session ends at LBA 0x%X\n", session->end_lba);

      session_number++;
    }
    else if (chunk_id == DAOI || chunk_id == DAOX) {
      /**
      * DAOI / DAOX (DAO Information) format:
      *   5.0  | 5.5  | Description                  | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *   4  B | 4  B | Chunk size (bytes) again     | v5 = (# tracks * 30) + 22,  v5.5 = (# tracks * 42) + 22
      *   14 B | 14 B | UPC?
      *   1  B | 1  B | Toc type                     | 0x20 = Mode2, 0x00 = Audio?
      *   1  B | 1  B | close_cd?                    | 1 = doesn't work, usually 0
      *   1  B | 1  B | First track                  | Usually 0x01
      *   1  B | 1  B | Last track                   | Usually = # tracks
      * --------------------------------------------------------------------------------------------------------------------
      *   10 B | 10 B | ISRC Code?
      *   4  B | 4  B | Sector size
      *   4  B | 4  B | Mode                         | mode2 = 0x03000001, audio = 0x07000001
      *   4  B | 8  B | Index0 start offset
      *   4  B | 8  B | Index1 start offset          | = index0 + pregap length
      *   4  B | 8  B | Next offset                  | = index1 + track length
      * ... Repeat for each track in this session
      */
      // When a DAOI / DAOX tag is encountered, it's talking about the last session described by a CUES / CUEX chunk
      nrg_session *session = image->last_session;

      // # tracks
      assert(session->number_tracks == (fread32u(image_file) - 22) / 30);

      // Skip UPC
      fseek(image_file, 14, SEEK_CUR);

      session->toc_type    = fread8u(image_file);
      fread8u(image_file); // close cd
      session->first_track_number = fread8u(image_file);
      session->last_track_number  = fread8u(image_file);

      ver_printf(3, "  %s at 0x%X:  Size - %dB\n", (chunk_id == DAOI ? "DAOI" : "DAOX"), chunk_offset, chunk_size);
      ver_printf(3, "    Toc Type - %s, First Track - %d, Last Track - %d, has %d track(s)\n", (session->toc_type == TOC_MODE2 ? "Mode2" : (session->toc_type == TOC_AUDIO ? "Audio" : "Unknown")), session->first_track_number, session->last_track_number, session->number_tracks);

      // Each track in the session should be described now
      int i = 0;
      nrg_track *track;
      for (track = session->first_track; track != NULL; track = track->next, i++)
      {
        // Skip ISRC Code
        fseek(image_file, 10, SEEK_CUR);

        // Read track data
        track->sector_size = fread32u(image_file);
        uint32_t mode       = fread32u(image_file);
        track->pretrack_offset      = (chunk_id == DAOI) ? (uint64_t) fread32u(image_file) : fread64u(image_file);
        track->track_offset      = (chunk_id == DAOI) ? (uint64_t) fread32u(image_file) : fread64u(image_file);
        track->next_offset = (chunk_id == DAOI) ? (uint64_t) fread32u(image_file) : fread64u(image_file);
        track->length = track->next_offset - track->track_offset;

        // Assert that the mode read here is the same as that read in CUES / CUEX
        if (mode == DAO_MODE2)
          assert(track->track_mode == MODE2);
        else if (mode == DAO_AUDIO)
          assert(track->track_mode == AUDIO);

        ver_printf(3, "      Track %d: Type - %s/%d, pretrack_offset start - 0x%X, track_offset start - 0x%X, Next offset - 0x%X\n", i, (mode == 0x03000001 ? "Mode2" : (mode == 0x07000001 ? "Audio" : "Other")), track->sector_size, track->pretrack_offset, track->track_offset, track->next_offset);
      }

      // Update number of tracks in the image
      image->number_tracks += session->number_tracks;
    }
    else if (chunk_id == ETNF || chunk_id == ETN2) {
      /**
      * ETNF / ENT2 (Extended Track Information) format:
      * Multisession / TAO Only
      *   5.0  | 5.5  | Description                  | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *   4 B  | 8 B  | Start offset
      *   4 B  | 8 B  | Length (bytes)
      *   4 B  | 4 B  | Mode                         | 0x03 = mode2/2336, 0x06 = mode2/2352, 0x07 = audio/2352
      *   4 B  | 4 B  | Start lba
      *   4 B  | 8 B  | 00
      * ... Repeat for each track (in session)
      */
      ver_printf(3, "  %s at 0x%X: Size - %d B\n", (chunk_id == ETNF ? "ETNF" : "ETN2"), chunk_offset, chunk_size);

       // ENTF / ENT2 chunks indicate the start of a new TAO session so create a new session and add it to the image
      nrg_session *session = alloc_nrg_session();
      add_nrg_session(image, session);
      session->burn_mode = TAO;

      // Calculate number of tracks from the chunk size
      session->number_tracks = chunk_size / ((chunk_id == ETNF) ? 20 : 32);

      unsigned int i;
      for (i = 0; i < session->number_tracks; i++) {
        // Make a new track and add it to the session
        nrg_track *track = alloc_nrg_track();
        add_nrg_track(session, track);

        // Read track data
        track->track_offset     = (chunk_id == ETNF) ? (uint64_t) fread32u(image_file) : fread64u(image_file);
        track->length     = (chunk_id == ETNF) ? (uint64_t) fread32u(image_file) : fread64u(image_file);
        track->track_mode = fread32u(image_file);
        track->track_lba  = fread32u(image_file);

        track->pretrack_mode = track->track_mode;
        track->pretrack_lba  = track->track_lba;

        // Convert the track mode
        if (track->track_mode == ENT_MODE2_2336) {
          track->track_mode = MODE2;
          track->sector_size = 2336;
        }
        else if (track->track_mode == ENT_MODE2_2352) {
          track->track_mode = MODE2;
          track->sector_size = 2352;
        }
        else if (track->track_mode == ENT_AUDIO) {
          track->track_mode = AUDIO;
          track->sector_size = 2352;
        }

        // Fill in the rest of the track data
        track->pretrack_mode = track->track_mode;
        assert( ((chunk_id == ETNF) ? fread32u(image_file) : fread64u(image_file)) == 0x00);

        ver_printf(3, "    Track Offset - 0x%X, Track Length - %d B, Type - %s/%d, Start LBA - 0x%X\n",  track->track_offset, track->length, (track->track_mode == MODE2 ? "Mode2" : (track->track_mode == AUDIO ? "Audio" : "Unknown")), track->sector_size, track->track_lba);
      }

      // Update number of tracks in the image
      image->number_tracks += session->number_tracks;
    }
    else if (chunk_id == CDTX) {
      /**
      * CDTX (CD Text) format:
      *   5.0  | 5.5  | Description                  | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *  18 B  | 18 B | CD-text pack
      */
      ver_printf(3, "  CDTX at 0x%X: Size - %dB\n", chunk_offset, chunk_size);
      ver_printf(2, "    Ignoring CDTX chunk (unsupported)\n");
      fseek(image_file, chunk_size, SEEK_CUR);
    }
    else if (chunk_id == SINF) {
      /**
      * SINF (Session Information) Format:
      *   5.0  | 5.5  | Description                  | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *   4 B  | 4 B  | Number tracks in session
      */
      static unsigned int sinf_number = 0;
      uint32_t number_tracks = fread32u(image_file);
      ver_printf(3, "  SINF at 0x%X: Size - %dB, Number of Tracks: %d\n", chunk_offset, chunk_size, number_tracks);

      // Get the session this SINF tag should be referring to
      unsigned int i;
      nrg_session *relevant_session = image->first_session;
      for (i = 0; i < sinf_number; i ++) {
        if (relevant_session->next)
          relevant_session = relevant_session->next;
        else {
          ver_printf(3, "    Warning: there are more SINF chunks than there are sessions\n");
          r = NRG_WARN;
          relevant_session = NULL;
          break;
        }
      }

      // See if the number of sessions reported by this SINF matches the number of sessions found
      if (relevant_session) {
        if (relevant_session->number_tracks == number_tracks)
          ver_printf(3, "    Matches number of tracks found for session %d\n", sinf_number + 1);
        else {
          ver_printf(3, "    Warning: Doesn't match number of tracks found for session %d\n", sinf_number);
          r = NRG_WARN;
        }
      }

      sinf_number++;
    }
    else if (chunk_id == MTYP) {
      /**
      * MTYP (Media Type?) Format:
      *   5.0  | 5.5  | Description                  | Notes
      * --------------------------------------------------------------------------------------------------------------------
      *   4 B  | 4 B  | unknown
      */
      image->media_type = fread32u(image_file);

      ver_printf(3, "  MTYP at 0x%X:  Size - %dB, Media Type - 0x%X\n", chunk_offset, chunk_size, image->media_type);
      ver_printf(2, "    Ignoring MTYP Chunk (unsupported)\n");
    }
    else if (chunk_id == END) {
      ver_printf(3, "  END! at 0x%X\n", chunk_offset);
      break;
    }
    else {
      fprintf(stderr, "  Unrecognized Chunk ID at 0x%X: 0x%X.\n", chunk_offset, chunk_id);
      r = NRG_WARN;
    }
  }

  // If the eof was reached, there was a problem so tell the user.
  if (feof(image_file)) {
    ver_printf(1,   "WARNING: End of file reached. This should not have happened.\n");
    if (get_verbosity() < 3)
      ver_printf(1, "         Try running again with -vv to see chunk processing output to see what went wrong\n");
    else
      ver_printf(3, "         See output above to see what went wrong\n");
    ver_printf(1,   "         This was likely a bug in nerorip so please report to %s\n", WEBSITE);

    r = NRG_WARN;
  }

  ver_printf(3, "Done processing chunk data.\n");
  return r;
}


// Prints out all gathered information about the nrg image
void nrg_print(int ver, nrg_image *image) {
  ver_printf(ver, "Loaded a Nero %s image containing the following data:\n", (image->nrg_version == NRG_VER_55 ? "5.5" : "5.0"));

  // All session data
  nrg_session *session;
  unsigned int s = 1, t = 1;
  for (session = image->first_session; session != NULL; session = session->next, s++) {
    ver_printf(ver, "  Session %d has %d track(s):\n", s, session->number_tracks);

    // All tracks in this session
    nrg_track *track;
    for (track = session->first_track; track != NULL; track = track->next, t++) {
      ver_printf(ver, "    Track: %d\tType: %s/%d\tSize: %6d\t", t, (track->track_mode == MODE2 ? "Mode2" : (track->track_mode == AUDIO ? "Audio" : "Unknown")), track->sector_size, track->length / track->sector_size);
      if (session->burn_mode == TAO)
        ver_printf(ver, "Offset: 0x%06X\tLBA:%6d\n", track->track_offset, track->track_lba);
      else
        ver_printf(ver, "Pretrack offset: 0x%06X\tPretrack LBA: %6d\tTrack Offset: 0x%06X\tTrack LBA: %6d\n", track->pretrack_offset, track->pretrack_lba, track->track_offset, track->track_lba);
    }

    ver_printf(ver, "\n");
  }
}
