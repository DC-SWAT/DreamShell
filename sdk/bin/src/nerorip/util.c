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

#include "util.h"


/**
 * Global variable verbosity
 *
 * Indicates what level of verbosity the user wants to see.
 * Defaults to 1, can be dropped to 0 (print nothing) with the --quiet, -q option.
 * --verbose, -v increse it one value.
 */
int verbosity = 1;

// Increments verbosity
void inc_verbosity() {
  verbosity++;
}

// Decrements verbosity
void dec_verbosity() {
  verbosity--;
}

// Returns the verbosity
int get_verbosity() {
  return verbosity;
}


// printf wrapper to only print if verbosity requirment is met
int ver_printf(int v, char* fmt, ...) {
  // Make sure the verbosity level is ok
  if (v > verbosity)
    return 0;

  // Pass the rest off to printf
  va_list ap;
  va_start(ap, fmt);
  int r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}


// File reading convenience functions
uint8_t fread8u(FILE* input) {
  uint8_t r = 0;
  if (fread(&r, sizeof(uint8_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return r;
}

uint16_t fread16u(FILE* input) {
  uint16_t r = 0;
  if (fread(&r, sizeof(uint16_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_16(r);
}

uint32_t fread32u(FILE* input) {
  uint32_t r = 0;
  if (fread(&r, sizeof(uint32_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_32(r);
}

uint64_t fread64u(FILE* input) {
  uint64_t r = 0;
  if (fread(&r, sizeof(uint64_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_64(r);
}


size_t fwrite32u(uint32_t value, FILE* output) {
  return fwrite(&value, sizeof(uint32_t), 1, output);
}
size_t fwrite16u(uint16_t value, FILE* output) {
  return fwrite(&value, sizeof(uint16_t), 1, output);
}

void fwrite_wav_header(FILE *tf, unsigned int length)
{
  // Following WAV header format found at https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
  unsigned int written = fwrite("RIFF", 1, 4, tf);
  written += fwrite32u(length + 36, tf); // Length of data + 36
  written += fwrite("WAVE", 1, 4, tf);
  written += fwrite("fmt ", 1, 4, tf);
  written += fwrite32u(16,     tf);  // PCM
  written += fwrite16u(1,      tf);  // No Compression
  written += fwrite16u(2,      tf);  // 2 channels
  written += fwrite32u(44100,  tf);  // Sample Rate
  written += fwrite32u(176400, tf);  // Byte Rate
  written += fwrite16u(4,      tf);  // Block Align
  written += fwrite16u(16,     tf);  // Bits per sample
  written += fwrite("data", 1, 4, tf);
  written += fwrite32u(length, tf); // Data length

  // Make sure all 25 things were written properly
  if (written != 25)
    fprintf(stderr, "Error writing WAV header: %s\n", strerror(errno));
}


void fwrite_aiff_header(FILE *tf, unsigned int length)
{
  // Calculate some useful values
  unsigned long int source_length = length * 2352;
  uint32_t total_length  = source_length + 8 + 18 + 8 + 12; // COMM + SSND
  uint32_t number_frames = source_length / 4;
  uint32_t audio_size = source_length + 8;
  uint8_t sample_rate[10] = {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  unsigned int written = 0;
  written += fwrite("FORM", 1, 4, tf);
  written += fwrite32u(total_length, tf);
  written += fwrite   ("AIFF", 1, 4, tf);
  written += fwrite   ("COMM", 1, 4, tf);
  written += fwrite32u(18, tf);           // Comm size
  written += fwrite16u(2, tf);            // 2 channels
  written += fwrite32u(number_frames, tf);
  written += fwrite16u(16, tf);           // Bits per sample
  written += fwrite   (sample_rate, 1, 10, tf);

  written += fwrite("SSND", 1, 4, tf);
  written += fwrite32u(audio_size, tf);
  written += fwrite32u(0, tf);           // Audio offset
  written += fwrite32u(0, tf);           // Audio block size
}


// "Swaps" the data in the buffer
void swap_buffer(uint8_t *buffer, unsigned int length) {
  unsigned int i;
  for (i = 0; i < length; i += 2) {
    uint8_t t= buffer[i];
    buffer[i] = buffer[i + 1];
    buffer[i + 1] = t;
  }
}
