#ifndef __AUDIO_H__
#define __AUDIO_H__

void writewavheader(FILE *fdest, long track_length);
void writeaiffheader(FILE *fdest, long track_length);
void write_ieee_extended(FILE *fdest, double x);
void ConvertToIeeeExtended(double num, char *bytes);

#endif

