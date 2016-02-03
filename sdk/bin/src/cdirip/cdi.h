#ifndef __CDI_H__
#define __CDI_H__

#define CDI_V2  0x80000004
#define CDI_V3  0x80000005
#define CDI_V35 0x80000006

unsigned long ask_type(FILE *fsource, long header_position);
void CDI_init (FILE *fsource, image_s *image, char *fsourcename);
void CDI_get_sessions (FILE *fsource, image_s *image);
void CDI_get_tracks (FILE *fsource, image_s *image);
void CDI_read_track (FILE *fsource, image_s *image, track_s *track);
void CDI_skip_next_session (FILE *fsource, image_s *image);

#endif

